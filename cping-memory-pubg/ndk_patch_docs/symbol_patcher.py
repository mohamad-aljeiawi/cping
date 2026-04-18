#!/usr/bin/env python3
"""
symbol_patcher.py
=================

Automated patcher for `a_native_window_creator.h` when Android introduces
incompatible symbol-signature changes in libgui.so / libutils.so.

ARCHITECTURE
------------
The tool is split into small, single-responsibility classes so each step of
the pipeline is independently testable:

    AdbClient        : thin wrapper around `adb` (shell, pull, ls)
    Nm               : runs llvm-nm -D and caches per-file export lists
    MangledName      : parse-just-enough-Itanium helper (class::method, tail)
    SymbolMatcher    : ranks candidate replacements by class::method match
                       + tail edit-distance
    HeaderPatcher    : locates the descriptor in the header and rewrites it
                       following the 5-step pattern from README.md
                       (split version range, add typedef/slot/invoker,
                       branch the call site)
    CLI              : argparse-driven entry point; dry-run by default

Design notes:
  * Python 3.8+, no third-party deps.  Works on Windows/macOS/Linux.
  * Dry-run by default.  A unified diff is always printed.
  * A timestamped `.bak` is written next to the header before --apply writes.
  * `--scan-libs` only pulls a short curated list by default
    (libgui.so, libutils.so, libbinder.so, libutils_binder.so).  Use
    --scan-libs=full to walk every *.so in /system/lib64 + /system_ext/lib64.

USAGE
-----
    # Look up a replacement, print the candidate(s) and the would-be patch:
    python symbol_patcher.py \
        --header jni/include/native/a_native_window_creator.h \
        --old-symbol _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE \
        --scan-libs

    # Apply it in place (min-new-version = first Android release using the
    # new mangling; 15 is the right guess for almost all 15/16 deltas):
    python symbol_patcher.py \
        --header jni/include/native/a_native_window_creator.h \
        --old-symbol _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE \
        --scan-libs --apply --min-new-version 15

    # Skip device scan - just split the descriptor using a known new symbol:
    python symbol_patcher.py \
        --header jni/include/native/a_native_window_creator.h \
        --old-symbol _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE \
        --new-symbol _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_ \
        --apply --min-new-version 15
"""

from __future__ import annotations

import argparse
import datetime as _dt
import difflib
import os
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterable, Optional


# ---------------------------------------------------------------------------
# Logging helpers
# ---------------------------------------------------------------------------

class _Log:
    _COLORS = {
        "info":  "\033[36m",
        "ok":    "\033[32m",
        "warn":  "\033[33m",
        "err":   "\033[31m",
        "dim":   "\033[90m",
        "reset": "\033[0m",
    }
    _enabled = sys.stdout.isatty() or os.environ.get("FORCE_COLOR")

    @classmethod
    def _paint(cls, tag: str, msg: str) -> str:
        if not cls._enabled:
            return f"[{tag.upper()}] {msg}"
        return f"{cls._COLORS[tag]}[{tag.upper()}]{cls._COLORS['reset']} {msg}"

    @classmethod
    def info(cls, msg: str) -> None:  print(cls._paint("info", msg))
    @classmethod
    def ok(cls,   msg: str) -> None:  print(cls._paint("ok",   msg))
    @classmethod
    def warn(cls, msg: str) -> None:  print(cls._paint("warn", msg))
    @classmethod
    def err(cls,  msg: str) -> None:  print(cls._paint("err",  msg), file=sys.stderr)
    @classmethod
    def dim(cls,  msg: str) -> None:  print(cls._paint("dim",  msg))


# ---------------------------------------------------------------------------
# AdbClient
# ---------------------------------------------------------------------------

class AdbClient:
    """Minimal adb wrapper - only what this tool needs."""

    def __init__(self, adb_bin: str = "adb", timeout: int = 60) -> None:
        self.adb_bin = adb_bin
        self.timeout = timeout
        if shutil.which(self.adb_bin) is None:
            raise FileNotFoundError(
                f"'{self.adb_bin}' not found in PATH. Install Android Platform-Tools."
            )

    def _run(self, args: list[str], capture: bool = True) -> subprocess.CompletedProcess:
        return subprocess.run(
            [self.adb_bin, *args],
            capture_output=capture,
            text=True,
            timeout=self.timeout,
        )

    def check_device(self) -> None:
        cp = self._run(["devices"])
        lines = [l for l in cp.stdout.splitlines()[1:] if l.strip()]
        authorized = [l for l in lines if l.endswith("\tdevice")]
        if not authorized:
            raise RuntimeError("No authorized ADB device. Connect USB + accept RSA prompt.")
        if len(authorized) > 1:
            _Log.warn(f"{len(authorized)} devices connected - adb will pick one.")

    def getprop(self, key: str) -> str:
        return self._run(["shell", f"getprop {key}"]).stdout.strip()

    def shell_list_so(self, directory: str) -> list[str]:
        cp = self._run(["shell", f"ls {directory}/*.so 2>/dev/null"])
        return [line.strip() for line in cp.stdout.splitlines() if line.strip()]

    def pull(self, remote: str, local: Path) -> bool:
        local.parent.mkdir(parents=True, exist_ok=True)
        cp = self._run(["pull", remote, str(local)])
        return cp.returncode == 0 and local.exists() and local.stat().st_size > 0


# ---------------------------------------------------------------------------
# Nm (llvm-nm invoker)
# ---------------------------------------------------------------------------

class Nm:
    """Runs llvm-nm -D and returns the *defined* symbols (T/W/R/D) of a lib."""

    _DEFINED_CODES = set("TWRVDB")  # U = undefined, skip

    def __init__(self, ndk_path: Optional[str] = None) -> None:
        ndk_path = ndk_path or os.environ.get("NDK_PATH")
        if not ndk_path:
            raise EnvironmentError(
                "NDK_PATH env var not set and --ndk not passed. "
                "Point it at your NDK root (where toolchains/llvm/prebuilt lives)."
            )
        host_triples = ("windows-x86_64", "linux-x86_64", "darwin-x86_64", "darwin-arm64")
        self.nm_bin: Optional[str] = None
        for host in host_triples:
            candidate = Path(ndk_path) / "toolchains" / "llvm" / "prebuilt" / host / "bin"
            for name in ("llvm-nm.exe", "llvm-nm"):
                exe = candidate / name
                if exe.exists():
                    self.nm_bin = str(exe)
                    break
            if self.nm_bin:
                break
        if not self.nm_bin:
            raise FileNotFoundError(f"llvm-nm not found under {ndk_path}")
        self._cache: dict[Path, list[tuple[str, str]]] = {}

    def exports(self, so_path: Path) -> list[tuple[str, str]]:
        """Returns list of (code, mangled_name) for *defined* exports."""
        if so_path in self._cache:
            return self._cache[so_path]
        cp = subprocess.run(
            [self.nm_bin, "-D", str(so_path)],
            capture_output=True, text=True, timeout=60,
        )
        out: list[tuple[str, str]] = []
        for line in cp.stdout.splitlines():
            parts = line.split()
            if len(parts) < 2:
                continue
            if len(parts) == 2:
                code, name = parts
            else:
                code, name = parts[1], parts[2]
            if code in self._DEFINED_CODES:
                out.append((code, name))
        self._cache[so_path] = out
        return out


# ---------------------------------------------------------------------------
# MangledName - just-enough Itanium demangler for matching
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class MangledName:
    raw: str
    nested_prefix: str   # '_ZN...E' portion including the terminating E
    params_tail: str     # whatever trails nested_prefix (could be empty)

    _NESTED_RE = re.compile(r"^(_ZN(?:[A-Z]+)?(?:\d+[A-Za-z0-9_]+)+E)(.*)$")

    @classmethod
    def parse(cls, raw: str) -> Optional["MangledName"]:
        m = cls._NESTED_RE.match(raw)
        if not m:
            return None
        return cls(raw=raw, nested_prefix=m.group(1), params_tail=m.group(2))

    @property
    def class_method_key(self) -> str:
        """Everything identifying the method but not its parameters."""
        return self.nested_prefix

    def tail_similarity(self, other: "MangledName") -> float:
        """0.0 = completely different parameters, 1.0 = identical."""
        a, b = self.params_tail, other.params_tail
        if not a and not b:
            return 1.0
        return difflib.SequenceMatcher(None, a, b).ratio()


# ---------------------------------------------------------------------------
# SymbolMatcher
# ---------------------------------------------------------------------------

@dataclass
class MatchCandidate:
    library: Path
    remote_path: str
    mangled: str
    score: float

    def __repr__(self) -> str:  # pragma: no cover
        return f"<{self.remote_path}:{self.mangled} score={self.score:.2f}>"


class SymbolMatcher:
    """Finds the closest-matching *defined* symbol to an 'old' mangled name."""

    DEFAULT_CURATED = [
        "/system/lib64/libgui.so",
        "/system/lib64/libutils.so",
        "/system/lib64/libbinder.so",
        "/system/lib64/libutils_binder.so",
    ]
    FULL_DIRS = [
        "/system/lib64",
        "/system_ext/lib64",
        "/vendor/lib64",
        "/product/lib64",
    ]

    def __init__(self, adb: AdbClient, nm: Nm, workdir: Path) -> None:
        self.adb = adb
        self.nm = nm
        self.workdir = workdir

    def _collect_lib_paths(self, mode: str) -> list[str]:
        if mode == "curated":
            return list(self.DEFAULT_CURATED)
        collected: list[str] = []
        for d in self.FULL_DIRS:
            collected.extend(self.adb.shell_list_so(d))
        return collected

    def _pull_many(self, remotes: Iterable[str]) -> list[tuple[str, Path]]:
        pulled: list[tuple[str, Path]] = []
        for r in remotes:
            local = self.workdir / Path(r).name
            if self.adb.pull(r, local):
                pulled.append((r, local))
            else:
                _Log.dim(f"skip (pull failed): {r}")
        return pulled

    def find_best(
        self,
        old_symbol: str,
        scan_mode: str = "curated",
        top_k: int = 5,
    ) -> list[MatchCandidate]:
        old = MangledName.parse(old_symbol)
        if old is None:
            raise ValueError(f"Could not parse mangled name: {old_symbol!r}")

        self.adb.check_device()
        _Log.info(f"Enumerating device libs ({scan_mode})...")
        remotes = self._collect_lib_paths(scan_mode)
        _Log.info(f"  {len(remotes)} candidate .so files")
        _Log.info("Pulling libs into temp workspace...")
        pulled = self._pull_many(remotes)
        _Log.info(f"  pulled {len(pulled)} libs")

        exact = old.raw
        prefix = old.class_method_key
        results: list[MatchCandidate] = []
        for remote, local in pulled:
            for code, name in self.nm.exports(local):
                if name == exact:
                    results.append(MatchCandidate(local, remote, name, score=1.0))
                    continue
                if not name.startswith(prefix):
                    continue
                parsed = MangledName.parse(name)
                if parsed is None:
                    continue
                score = 0.5 + 0.5 * old.tail_similarity(parsed)
                if name == exact:
                    score = 1.0
                results.append(MatchCandidate(local, remote, name, score))

        results.sort(key=lambda c: c.score, reverse=True)
        seen: set[str] = set()
        unique: list[MatchCandidate] = []
        for c in results:
            if c.mangled in seen:
                continue
            seen.add(c.mangled)
            unique.append(c)
            if len(unique) >= top_k:
                break
        return unique


# ---------------------------------------------------------------------------
# HeaderPatcher
# ---------------------------------------------------------------------------

@dataclass
class PatchPlan:
    old_symbol: str
    new_symbol: str
    min_new_version: int
    header_path: Path
    original_text: str
    patched_text: str
    descriptor_old_repr: str
    descriptor_new_repr: str

    def unified_diff(self) -> str:
        return "".join(difflib.unified_diff(
            self.original_text.splitlines(keepends=True),
            self.patched_text.splitlines(keepends=True),
            fromfile=str(self.header_path),
            tofile=str(self.header_path) + " (patched)",
            n=3,
        ))


class HeaderPatcher:
    """
    Locates an ApiDescriptor whose apiSignature matches `old_symbol`, then
    rewrites that descriptor following the README.md 5-step pattern:

        ApiDescriptor{ MIN, UINT_MAX, &slot, "old-signature" }
    becomes
        ApiDescriptor{ MIN, min_new_version - 1, &slot, "old-signature" },
        ApiDescriptor{ min_new_version, UINT_MAX, &slot, "new-signature" }

    The Api slot is reused (same `&slot`) because the user-visible call-site
    change is dispatched separately in the header via `compat::SystemVersion`
    checks. If the signature change requires a *different* Api slot (e.g. a
    new typedef with more params), we emit a clearly-marked TODO comment
    next to the new descriptor so a human can wire it - we intentionally
    do NOT try to rewrite typedefs blindly because getting the parameter
    list wrong leads to SIGSEGV at runtime.
    """

    _DESCRIPTOR_RE = re.compile(
        r"""
        (?P<prefix>ApiDescriptor\s*\{\s*)
        (?P<min>\d+|UINT_MAX)\s*,\s*
        (?P<max>\d+|UINT_MAX)\s*,\s*
        (?P<slot>&[^,]+?)\s*,\s*
        (?P<sig_quoted>(?:"[^"]*"\s*)+)
        \s*\}
        """,
        re.VERBOSE | re.DOTALL,
    )

    @staticmethod
    def _dequote_concat(sig_quoted: str) -> str:
        parts = re.findall(r'"([^"]*)"', sig_quoted)
        return "".join(parts)

    @staticmethod
    def _requote(literal: str, indent: str = "                          ") -> str:
        """
        Pretty-print a long mangled string as chunked C++ string-literals.
        Prefer to break right after an Itanium scope-end 'E' so the chunks
        stay on token boundaries and the output reads like hand-written code.
        """
        max_chunk = 60
        if len(literal) <= max_chunk:
            return f'"{literal}"'
        chunks: list[str] = []
        i = 0
        while i < len(literal):
            end = min(i + max_chunk, len(literal))
            if end < len(literal):
                window = literal[i:end]
                last_e = window.rfind("E")
                if last_e >= max_chunk // 2:
                    end = i + last_e + 1
            chunks.append(literal[i:end])
            i = end
        glue = f'"\n{indent}"'
        return '"' + glue.join(chunks) + '"'

    def plan(
        self,
        header_path: Path,
        old_symbol: str,
        new_symbol: str,
        min_new_version: int,
    ) -> PatchPlan:
        text = header_path.read_text(encoding="utf-8")

        target_match = None
        target_sig = None
        for m in self._DESCRIPTOR_RE.finditer(text):
            if self._dequote_concat(m.group("sig_quoted")) == old_symbol:
                target_match = m
                target_sig = old_symbol
                break

        if target_match is None:
            raise LookupError(
                f"Could not find an ApiDescriptor whose signature equals the "
                f"given --old-symbol:\n  {old_symbol}\n"
                f"Double-check the header and the logcat output."
            )

        old_min = target_match.group("min")
        old_max = target_match.group("max")
        slot = target_match.group("slot").strip()

        if old_max not in ("UINT_MAX",) and old_max.isdigit() and int(old_max) < min_new_version:
            raise RuntimeError(
                f"Refusing to patch: existing descriptor already caps at "
                f"version {old_max}, which is below --min-new-version "
                f"{min_new_version}. This likely means the file was already "
                f"patched for this symbol."
            )
        new_old_max = str(min_new_version - 1)

        line_start = text.rfind("\n", 0, target_match.start()) + 1
        indent_match = re.match(r"[ \t]*", text[line_start:target_match.start()])
        base_indent = indent_match.group(0) if indent_match else "            "
        sig_indent = base_indent + "    "

        old_descriptor_repr = (
            f"ApiDescriptor{{\n"
            f"{base_indent}    {old_min}, {new_old_max},\n"
            f"{base_indent}    {slot},\n"
            f"{base_indent}    {self._requote(old_symbol, sig_indent)}}}"
        )
        new_descriptor_repr = (
            f"ApiDescriptor{{\n"
            f"{base_indent}    {min_new_version}, UINT_MAX,\n"
            f"{base_indent}    /* TODO: if the new signature takes different "
            f"args, add a new\n"
            f"{base_indent}       Api slot + typedef + ApiInvoker branch and "
            f"point this &slot at it. */\n"
            f"{base_indent}    {slot},\n"
            f"{base_indent}    {self._requote(new_symbol, sig_indent)}}}"
        )

        replacement = f"{old_descriptor_repr},\n{base_indent}{new_descriptor_repr}"
        patched = text[:target_match.start()] + replacement + text[target_match.end():]

        return PatchPlan(
            old_symbol=old_symbol,
            new_symbol=new_symbol,
            min_new_version=min_new_version,
            header_path=header_path,
            original_text=text,
            patched_text=patched,
            descriptor_old_repr=old_descriptor_repr,
            descriptor_new_repr=new_descriptor_repr,
        )

    def apply(self, plan: PatchPlan) -> Path:
        stamp = _dt.datetime.now().strftime("%Y%m%d-%H%M%S")
        backup = plan.header_path.with_suffix(plan.header_path.suffix + f".{stamp}.bak")
        shutil.copy2(plan.header_path, backup)
        plan.header_path.write_text(plan.patched_text, encoding="utf-8")
        return backup


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def _build_argparser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(
        prog="symbol_patcher.py",
        description="Patch a_native_window_creator.h for Android symbol changes.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    p.add_argument("--header", required=True, type=Path,
                   help="Path to a_native_window_creator.h")
    p.add_argument("--old-symbol", required=True,
                   help="Mangled name that currently fails to resolve "
                        "(copy verbatim from logcat).")
    p.add_argument("--new-symbol",
                   help="Mangled name of the replacement. If omitted, use "
                        "--scan-libs to have the tool find it.")
    p.add_argument("--scan-libs", nargs="?", const="curated",
                   choices=["curated", "full"],
                   help="Scan the connected device for the replacement "
                        "symbol. 'curated' (default) checks a short list; "
                        "'full' walks every .so in system + system_ext.")
    p.add_argument("--min-new-version", type=int, default=15,
                   help="First Android release that uses the new mangling "
                        "(default: 15).")
    p.add_argument("--ndk", help="NDK root. Defaults to $NDK_PATH.")
    p.add_argument("--apply", action="store_true",
                   help="Actually write changes. Without this, runs as dry-run.")
    p.add_argument("--top-k", type=int, default=5,
                   help="How many candidates to show from the library scan.")
    return p


def main(argv: Optional[list[str]] = None) -> int:
    args = _build_argparser().parse_args(argv)

    if not args.header.exists():
        _Log.err(f"Header not found: {args.header}")
        return 2

    new_symbol = args.new_symbol

    if args.scan_libs:
        try:
            adb = AdbClient()
            nm = Nm(args.ndk)
        except (FileNotFoundError, EnvironmentError) as e:
            _Log.err(str(e))
            return 2

        release = adb.getprop("ro.build.version.release")
        sdk = adb.getprop("ro.build.version.sdk")
        _Log.info(f"Device Android release={release} sdk={sdk}")

        with tempfile.TemporaryDirectory(prefix="sym_patcher_") as td:
            matcher = SymbolMatcher(adb, nm, Path(td))
            try:
                candidates = matcher.find_best(
                    args.old_symbol, scan_mode=args.scan_libs, top_k=args.top_k
                )
            except ValueError as e:
                _Log.err(str(e)); return 2

        if not candidates:
            _Log.err("No candidate replacement symbols found on device.")
            _Log.dim("Try --scan-libs full, or check if the symbol is hidden.")
            return 1

        _Log.ok(f"Top candidates for {args.old_symbol}:")
        for i, c in enumerate(candidates, 1):
            marker = "*" if i == 1 else " "
            print(f"  {marker} [{c.score:.2f}]  {c.mangled}")
            _Log.dim(f"        from {c.remote_path}")

        if new_symbol is None:
            new_symbol = candidates[0].mangled
            _Log.info(f"Using top candidate as --new-symbol: {new_symbol}")

    if new_symbol is None:
        _Log.err("Either --new-symbol or --scan-libs is required.")
        return 2

    if new_symbol == args.old_symbol:
        _Log.err("--new-symbol equals --old-symbol - nothing to patch.")
        return 1

    patcher = HeaderPatcher()
    try:
        plan = patcher.plan(
            args.header, args.old_symbol, new_symbol, args.min_new_version
        )
    except (LookupError, RuntimeError) as e:
        _Log.err(str(e)); return 1

    _Log.ok("Patch plan:")
    print(f"  old: {plan.old_symbol}")
    print(f"  new: {plan.new_symbol}")
    print(f"  min-new-version: {plan.min_new_version}")
    print()
    print("=== unified diff ===")
    print(plan.unified_diff() or "(no changes computed)")
    print("=== end diff ===")

    if not args.apply:
        _Log.info("Dry run complete. Re-run with --apply to write changes.")
        return 0

    backup = patcher.apply(plan)
    _Log.ok(f"Patched {plan.header_path}")
    _Log.dim(f"Backup saved to {backup}")
    _Log.warn(
        "If the new signature has different arity than the old, remember to:"
    )
    print("  1. Add a new typedef in types::apis::libgui::generic::")
    print("  2. Add a new slot to the ApiTable")
    print("  3. Add an ApiInvoker branch that casts to the new typedef")
    print("  4. Branch the call site on compat::SystemVersion")
    print("(See ndk_patch_docs/README.md section 5 for the full template.)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
