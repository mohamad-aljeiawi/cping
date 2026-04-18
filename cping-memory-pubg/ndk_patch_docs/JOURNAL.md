# Session Journal — Porting `cping-memory-pubg` to Android 16

> A chronological, reproduce-from-scratch log of everything we tried, every
> command we ran, every assumption (right *and* wrong), and every correction
> that got us from "hardcoded paths + crashing binary" to "clean build + a
> working overlay with reusable patching tooling".
>
> Nothing in this file is theoretical. Each command was actually executed
> during the session it documents.

---

## Table of contents

0. [Starting state](#0-starting-state)
1. [Phase 1 — `build.bat`: static paths → dynamic paths](#phase-1--buildbat-static-paths--dynamic-paths)
2. [Phase 2 — Dynamic module name from `Android.mk`](#phase-2--dynamic-module-name-from-androidmk)
3. [Phase 3 — Robustness: pre-flight checks & clear messaging](#phase-3--robustness-pre-flight-checks--clear-messaging)
4. [Phase 4 — The "build OK but binary missing" mystery](#phase-4--the-build-ok-but-binary-missing-mystery)
5. [Phase 5 — Runtime crash: `Failed to resolve symbol`](#phase-5--runtime-crash-failed-to-resolve-symbol)
   - 5.1 [Wrong hypothesis #1: `String8` symbols missing](#51-wrong-hypothesis-1-string8-symbols-missing)
   - 5.2 [PowerShell syntax correction detour](#52-powershell-syntax-correction-detour)
   - 5.3 [Re-verification — `String8` symbols are in fact present](#53-re-verification--string8-symbols-are-in-fact-present)
   - 5.4 [Getting the real missing symbol from logcat](#54-getting-the-real-missing-symbol-from-logcat)
   - 5.5 [Root cause: `mirrorSurface` signature changed on Android 15+](#55-root-cause-mirrorsurface-signature-changed-on-android-15)
6. [Phase 6 — The 5-part header patch](#phase-6--the-5-part-header-patch)
7. [Phase 7 — Secondary compile error (`StrongPointer` namespace)](#phase-7--secondary-compile-error-strongpointer-namespace)
8. [Phase 8 — Documentation & automated patcher](#phase-8--documentation--automated-patcher)
9. [Lessons learned](#9-lessons-learned)

---

## 0. Starting state

### Repository layout (relevant portions)

```
cping-memory-pubg/
├── build.bat                                # build+push+run launcher (Windows)
├── jni/
│   ├── Android.mk                           # ndk-build module definition
│   └── include/native/
│       └── a_native_window_creator.h        # dlsym-based private NDK glue
└── libs/arm64-v8a/                          # ndk-build output (generated)
```

### `jni/Android.mk` — relevant line

```makefile
LOCAL_MODULE := cping_memory_pubg
```

### `build.bat` — how it looked when we started

```batch
call "C:/Users/mohamadaljeiawi/AppData/Local/Android/Sdk/ndk/29.0.14206865/ndk-build"
adb push "C:/Users/mohamadaljeiawi/Desktop/project/cping/cping-memory-pubg/libs/arm64-v8a/cping_memory_pubg" "/data/local/tmp/"
adb shell su -c "kill -9 $(pidof cping_memory_pubg)"
adb shell su -c "chmod +x /data/local/tmp/cping_memory_pubg"
adb shell su -c "/data/local/tmp/cping_memory_pubg"
```

Three separate hard-coded things: (1) an absolute NDK path, (2) an absolute
project path **containing a typo** (`project` instead of `projects`), and
(3) the module name `cping_memory_pubg` repeated four times.

### Environment
- Windows 10 (win32 10.0.26200)
- PowerShell 5.1
- NDK r29.0.14206865 (`NDK_PATH` env var already configured)
- Device: Xiaomi (codename `garnet`), Android 16, rooted (`su` available)

---

## Phase 1 — `build.bat`: static paths → dynamic paths

### Goal
Make the script work on any machine where `NDK_PATH` is set, regardless of
where the project is checked out.

### Rationale
- `%~dp0` expands to "directory the script lives in, with trailing slash" —
  perfect for resolving artefacts relative to the script itself.
- `%NDK_PATH%` is already the conventional env var for the Android NDK
  root.

### Patch (simplified)

```batch
set "SCRIPT_DIR=%~dp0"

if not defined NDK_PATH (
    echo NDK_PATH environment variable is not set!
    pause
    exit /b 1
)

call "%NDK_PATH%\ndk-build"
adb push "%SCRIPT_DIR%libs\arm64-v8a\cping_memory_pubg" "/data/local/tmp/"
```

### Result
Build ran from any working directory without path edits.

---

## Phase 2 — Dynamic module name from `Android.mk`

### Goal
Stop hardcoding `cping_memory_pubg` in `build.bat`. Read the module name
from `LOCAL_MODULE :=` in `jni/Android.mk` so the two files never drift.

### Attempt (parser)

```batch
setlocal EnableDelayedExpansion
set "ANDROID_MK=%SCRIPT_DIR%jni\Android.mk"

set "MODULE_NAME="
for /f "tokens=2 delims==" %%A in ('findstr /R /C:"^[ \t]*LOCAL_MODULE[ \t]*:=" "%ANDROID_MK%"') do (
    set "RAW=%%A"
    for /f "tokens=* delims= \t" %%B in ("!RAW!") do set "MODULE_NAME=%%B"
)

adb push "%SCRIPT_DIR%libs\arm64-v8a\%MODULE_NAME%" "/data/local/tmp/"
adb shell su -c "kill -9 $(pidof %MODULE_NAME%)"
```

### What the regex does
`^[ \t]*LOCAL_MODULE[ \t]*:=` matches "start of line, optional whitespace,
`LOCAL_MODULE`, optional whitespace, `:=`" — tolerant of any amount of
indentation the user might add.

### Tokenisation choice
`delims==` splits on `=`. Since the make syntax is `LOCAL_MODULE := value`,
`%%A` receives `" value"` (with the leading space from ` := value`). We
*try* to trim it with an inner `for /f "tokens=* delims= \t"` — this mostly
works but turned out to miss trailing carriage returns (see Phase 4).

---

## Phase 3 — Robustness: pre-flight checks & clear messaging

### Goal (verbatim from user)
> "Add clear messages and script checks to ensure it is very stable and
> clear to any new splitter without having to read it."

### What we added
- A prominent header banner when the script starts.
- `[STEP N/X] <what we're doing>` before each action.
- `[OK]` / `[WARN]` / `[ERROR]` status lines after.
- A `:fail` subroutine that prints a consistent error block + `pause` +
  `exit /b 1`.
- Pre-flight checks for: `NDK_PATH`, `adb` on PATH, `Android.mk` exists,
  `Android.mk` has a `LOCAL_MODULE`, at least one authorised device, root
  available (`adb shell su -c id` exits 0).
- Post-build check: `if not exist "%SCRIPT_DIR%libs\arm64-v8a\%MODULE_NAME%"
  call :fail "Build reported success but the binary is missing"` —
  catches ndk-build silent failures.
- Smarter kill: use `pidof` with fall-back to `pkill`, don't treat
  "process not running" as an error.

### Example step emission

```batch
call :step 3 "Reading LOCAL_MODULE from jni\Android.mk"
... parse ...
echo   [OK] module = %MODULE_NAME%
goto :eof

:step
echo.
echo [STEP %~1/%TOTAL_STEPS%] %~2
goto :eof

:fail
echo.
echo [ERROR] %~1
echo.
pause
exit /b 1
```

---

## Phase 4 — The "build OK but binary missing" mystery

### Symptom (from terminal)

```
[STEP 6/9] Verifying the built binary
[ERROR] Build reported success but the binary is missing at:
        C:\Users\mohamadaljeiawi\Desktop\projects\cping\cping-memory-pubg\libs\arm64-v8a\ cping_memory_pubg
```

### The clue
Look **very** carefully at the path — there's a **leading space** between
`arm64-v8a\` and `cping_memory_pubg`:

```
...arm64-v8a\ cping_memory_pubg
            ^ here
```

### Wrong assumption (first 30 seconds)
> "ndk-build must be writing the binary to a different folder on newer NDK."

### Actual root cause
Our `MODULE_NAME` variable contained a **leading space** — and possibly a
trailing `\r` from Windows line endings in `Android.mk`. The inner
`for /f "tokens=* delims= \t"` wasn't stripping what we thought it was.

When you then interpolate `%MODULE_NAME%` inside a path, CMD happily
concatenates the space in and looks for a file whose name *starts with a
space* — which obviously doesn't exist.

### Diagnostic command

```batch
echo "%MODULE_NAME%"
REM  →  " cping_memory_pubg"   ← notice the quotes, they prove the leading space
```

### Fix — a dedicated `:trim` subroutine

```batch
:trim
setlocal EnableDelayedExpansion
set "s=!%~1!"
:_trim_lead
if not defined s goto :_trim_done
if "!s:~0,1!"==" "  set "s=!s:~1!" & goto :_trim_lead
if "!s:~0,1!"=="	" set "s=!s:~1!" & goto :_trim_lead
:_trim_tail
if not defined s goto :_trim_done
if "!s:~-1!"==" "  set "s=!s:~0,-1!" & goto :_trim_tail
if "!s:~-1!"=="	" set "s=!s:~0,-1!" & goto :_trim_tail
rem strip a trailing carriage return that may sneak in from CRLF parsing
for /f "delims=" %%C in ("!s!") do set "s=%%C"
:_trim_done
endlocal & set "%~1=%s%"
goto :eof
```

Usage after the parser:

```batch
call :trim MODULE_NAME
```

### Lesson
Whenever you parse values out of a text file with `for /f` on Windows,
**always** run them through an explicit trim. Don't trust `tokens=*`
alone. CRLF endings and embedded tabs will bite you.

---

## Phase 5 — Runtime crash: `Failed to resolve symbol`

After the script was clean, the executable finally pushed, ran, and…
crashed.

### Reproducer

```
garnet:/data/local/tmp # ./cping_memory_pubg
libc++abi: terminating due to uncaught exception of type St13runtime_error:
Failed to resolve symbol
Aborted
```

### Where the exception lives (source search)

```powershell
rg "Failed to resolve symbol" jni\include\native
```

Output pointed at:

```
jni\include\native\a_native_window_creator.h:
  throw std::runtime_error("Failed to resolve symbol: " + signature);
```

— inside `compat::ApiResolver::Resolve()`. So the binary starts, begins
resolving dynamic symbols, and one of them misses. The question is
**which one**.

---

### 5.1 Wrong hypothesis #1: `String8` symbols missing

We initially guessed (based on pattern-matching on past Android NDK breaks)
that `libutils.so` no longer exports the `android::String8` constructor /
destructor. That matters because the resolver table requests them
unconditionally.

#### Attempted diagnostic (CMD-style, first try)

```cmd
adb pull /system/lib64/libutils.so
"%NDK_PATH%\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-nm.exe" -D libutils.so ^
    | findstr "String8"
```

Expected: a list of `_ZN7android7String8...` entries. What actually
happened — nothing came out. We jumped to:
> "Right, Google removed `String8` from `libutils.so`, we need to find it
> elsewhere or reimplement it."

This was **wrong**. See 5.2 and 5.3.

---

### 5.2 PowerShell syntax correction detour

The first thing we missed: the user wasn't running the command in CMD.
They were in PowerShell, where the CMD syntax silently misbehaves.

#### What PowerShell does differently

| CMD                                   | PowerShell equivalent                           |
|---------------------------------------|-------------------------------------------------|
| `cmd1 && cmd2`                        | `cmd1; if ($?) { cmd2 }` (or just `;`)          |
| `%NDK_PATH%`                          | `$env:NDK_PATH`                                 |
| `"C:\path with space\tool.exe" -flag` | `& "C:\path with space\tool.exe" -flag`         |
| `findstr "pattern"`                   | `Select-String "pattern"` (different semantics) |
| `^` for line continuation             | `` ` `` (backtick)                              |

The PowerShell-native form of the command is:

```powershell
$nm = "$env:NDK_PATH\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-nm.exe"
adb pull /system/lib64/libutils.so
& $nm -D libutils.so | Select-String "String8"
```

Only after this correction did `Select-String` actually get a chance to
filter output from `llvm-nm` properly. Previously PowerShell was trying
to execute `"C:\...\llvm-nm.exe"` as a **string literal** (not a command),
producing no output, which we misread as "symbol missing".

---

### 5.3 Re-verification — `String8` symbols are in fact present

With the corrected command:

```powershell
& $nm -D libutils.so | Select-String " T .*String8"
```

Output (trimmed):

```
0000000000001f20 T _ZN7android7String8C1Ev
0000000000001f40 T _ZN7android7String8C1EPKc
0000000000001fa0 T _ZN7android7String8C1ERKS0_
0000000000001fc0 T _ZN7android7String8C2Ev
0000000000001fe0 T _ZN7android7String8C2EPKc
0000000000002050 T _ZN7android7String8D1Ev
0000000000002070 T _ZN7android7String8D2Ev
...
```

Correction committed: `String8` is fine. The original "missing symbol"
lead was a **tooling artefact** (PowerShell quoting) masquerading as a
missing symbol. It cost us ~10 minutes.

**Lesson**: before concluding "symbol X is missing from library Y",
confirm that your diagnostic command is actually executing and producing
output. A completely empty filter result is almost always the tool
misbehaving, not the library.

---

### 5.4 Getting the real missing symbol from logcat

The resolver doesn't just throw — it *logs* the symbol it failed on
first. We only needed to capture logcat around the crash:

```powershell
adb logcat -c
adb shell "su -c '/data/local/tmp/cping_memory_pubg'"
adb logcat -d | Select-String "AImGui|NativeWindowCreator|libc"
```

The line that mattered:

```
E AImGui  : [!] Version[Android 16] [libgui] failed to resolve symbol:
            _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE
```

So the missing symbol is a **`libgui`** function, specifically:

```
android::SurfaceComposerClient::mirrorSurface(android::SurfaceControl*)
```

(one argument).

---

### 5.5 Root cause: `mirrorSurface` signature changed on Android 15+

#### Pull the library that actually owns the symbol

```powershell
adb pull /system/lib64/libgui.so
& $nm -D libgui.so | Select-String "mirrorSurface"
```

Output:

```
00000000002ca2c0 T _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_
```

One hit. Compare to the one we were trying to resolve:

```
WANT: _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE
HAVE: _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_
                                                                          ^^^^
                                                                   extra parameter
```

#### Demangling confirms it

```powershell
$cxxfilt = "$env:NDK_PATH\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-cxxfilt.exe"
echo "_ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_" | & $cxxfilt
```

```
android::SurfaceComposerClient::mirrorSurface(android::SurfaceControl*, android::SurfaceControl*)
```

Two `SurfaceControl*` arguments. Cross-checked against AOSP
`frameworks/native/libs/gui/SurfaceComposerClient.cpp` (tag
`android-15.0.0_r*` and up): the second `SurfaceControl*` is the
**parent** surface, and the function now attaches the mirror directly to
it — the caller no longer needs a follow-up `Transaction::reparent(...)`.

So we had two facts to encode simultaneously:
1. Android 15+ exports a different mangling, so `dlsym` of the old name
   returns `nullptr`.
2. Even if we fix the resolver, the *calling code* must change because
   the new function has one more argument **and** eliminates the need for
   the reparent transaction.

This is why we need the full 5-step patch pattern and not just a rename.

---

## Phase 6 — The 5-part header patch

Applied to `jni/include/native/a_native_window_creator.h`.

### Step 1 — New typedef in `types::apis::libgui::generic::`

```cpp
using SurfaceComposerClient__MirrorSurfaceWithParent =
    StrongPointer<void> (*)(void *thiz, void *mirrorFromSurface, void *parent);
```

Placed right next to the existing one-arg `SurfaceComposerClient__MirrorSurface`.

### Step 2 — New slot in `SurfaceComposerClient::ApiTable`

```cpp
struct ApiTable {
  void *Constructor;
  void *CreateSurface;
  void *MirrorSurface;
  void *MirrorSurfaceWithParent;   // NEW
  void *GetInternalDisplayToken;
  ...
};
```

Critical detail: we **kept** `MirrorSurface`. Removing it would break
Android 11–14 devices that still export the one-arg version. Each slot
belongs to a *specific mangled name*, not a *logical method*.

### Step 3 — New `ApiInvoker` branch

```cpp
if constexpr ("SurfaceComposerClient::MirrorSurfaceWithParent" == descriptor) {
  return reinterpret_cast<types::apis::libgui::generic::
                          SurfaceComposerClient__MirrorSurfaceWithParent>(
      apis::libgui::SurfaceComposerClient::Api.MirrorSurfaceWithParent);
}
```

The invoker is the only place where the raw `void*` is cast back to a
typed function pointer. One descriptor string → one cast → one typed
callable.

### Step 4 — Split the descriptor table entry

Before:

```cpp
ApiDescriptor{
    11, UINT_MAX,
    &apis::libgui::SurfaceComposerClient::Api.MirrorSurface,
    "_ZN7android21SurfaceComposerClient13mirrorSurfaceE"
    "PNS_14SurfaceControlE"},
```

After:

```cpp
ApiDescriptor{
    11, 14,
    &apis::libgui::SurfaceComposerClient::Api.MirrorSurface,
    "_ZN7android21SurfaceComposerClient13mirrorSurfaceE"
    "PNS_14SurfaceControlE"},
ApiDescriptor{
    15, UINT_MAX,
    &apis::libgui::SurfaceComposerClient::Api.MirrorSurfaceWithParent,
    "_ZN7android21SurfaceComposerClient13mirrorSurfaceE"
    "PNS_14SurfaceControlES2_"},
```

Two invariants kept:
- The old descriptor's `minVersion` (`11`) is unchanged — devices on
  Android 11–14 keep working.
- Exactly **one** descriptor ends at `UINT_MAX` — the currently-latest
  form.

### Step 5 — Branch the call site

```cpp
types::StrongPointer<void> mirrorSurface;
if (compat::SystemVersion >= 15) {
  mirrorSurface =
      ApiInvoker<"SurfaceComposerClient::MirrorSurfaceWithParent">()(
          data, surface.data, mirrorRootSurface.data);
} else {
  mirrorSurface = ApiInvoker<"SurfaceComposerClient::MirrorSurface@v14">()(
      data, surface.data);
}

if (nullptr == mirrorSurface.get()) {
  throw std::runtime_error("mirrorSurface() returned nullptr");
}

// Android 15+ already attaches the mirror to `parent`, so skip reparenting.
if (compat::SystemVersion < 15) {
  transaction.Reparent(mirrorSurface, mirrorRootSurface);
}
```

The two code paths converge afterwards — everything that follows
(`SetLayerStack`, `Show`, `Apply`) is identical and version-independent.

---

## Phase 7 — Secondary compile error (`StrongPointer` namespace)

After applying the 5-part patch, a rebuild produced **13 errors**. The
compiler's top suggestion was:

```
error: use of undeclared identifier 'mirrorSurface'; did you mean 'mirrorSurfaces'?
```

### The trap
`mirrorSurfaces` exists — it's a `static std::vector<mirror_surfaces_proxy_t>`
defined on line 1161 of the same file. The compiler's "did you mean" hint
was a **red herring**: it kept trying to reinterpret later uses of
`mirrorSurface` as the plural vector, which cascaded into nonsense
messages like "no viable `operator=` for vector".

### The real cause
Line 1170, inside the `detail::compat` namespace:

```cpp
StrongPointer<void> mirrorSurface;       // ← fails
```

`StrongPointer` is declared in the sibling namespace `types::`, not in
`detail::compat`. Elsewhere in the same function we always wrote
`types::StrongPointer<void>` (line 1213). Missing that qualifier here
made the declaration itself invalid, so `mirrorSurface` never actually
existed — which caused every later reference to it to fail, which the
compiler then "helpfully" suggested renaming to `mirrorSurfaces`.

### Fix

```1170:1170:jni/include/native/a_native_window_creator.h
    types::StrongPointer<void> mirrorSurface;
```

All 13 errors disappeared with that one qualifier.

### Lesson
When the compiler insists a variable doesn't exist, always check whether
its *declaration* is well-formed before chasing its uses. And treat
"did you mean X?" as a hint, not a truth.

---

## Phase 8 — Documentation & automated patcher

### Artefacts created

- `ndk_patch_docs/README.md` — how the resolver works, the diagnostic
  runbook, the Itanium mangled-name cheat sheet, and the 5-step patch
  pattern generalised.
- `ndk_patch_docs/symbol_patcher.py` — Python 3.8+, zero-dependency CLI
  that automates the diagnostic + patch loop.
- `ndk_patch_docs/JOURNAL.md` — this file.

### Architectural choice: Python over PowerShell
- Cross-platform — same file works on Windows, macOS, Linux.
- Real regex engine + `difflib.SequenceMatcher` for tail-similarity
  scoring without reimplementing edit-distance.
- `subprocess` / `argparse` / `pathlib` all in stdlib — no `pip install`
  step.

### Component breakdown (short version)

| Class          | Responsibility                                                                          |
|----------------|-----------------------------------------------------------------------------------------|
| `AdbClient`    | `adb devices/shell/pull`, version probing, timeout-guarded                              |
| `Nm`           | `llvm-nm -D` wrapper, host-triple autodetect, per-file caching                          |
| `MangledName`  | Split any `_ZN…E<tail>` mangling into `{class_method_prefix, params_tail}`              |
| `SymbolMatcher`| Rank candidates by class::method prefix + `SequenceMatcher`-scored tail similarity      |
| `HeaderPatcher`| Locate descriptor, split its version range, preserve indentation, emit unified diff     |
| CLI            | `argparse`, dry-run default, timestamped `.bak` on `--apply`                            |

### Verification run (dry-run with synthetic inputs)

```powershell
python ndk_patch_docs\symbol_patcher.py `
    --header jni\include\native\a_native_window_creator.h `
    --old-symbol "_ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_" `
    --new-symbol "_ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_S2_" `
    --min-new-version 17
```

Produced a clean unified diff that splits the `15, UINT_MAX` descriptor
into `15, 16` and a new `17, UINT_MAX` entry, with string literals broken
at Itanium `E` boundaries so the output matches the existing house style.

### End-to-end usage for a real regression

```powershell
adb logcat -c
adb shell "su -c '/data/local/tmp/cping_memory_pubg'"
$missing = (adb logcat -d |
            Select-String 'failed to resolve symbol' |
            ForEach-Object { ($_ -split '\s+')[-1] } |
            Select-Object -First 1)

python ndk_patch_docs\symbol_patcher.py `
    --header jni\include\native\a_native_window_creator.h `
    --old-symbol $missing `
    --scan-libs --apply --min-new-version 16
```

For pure rename deltas this writes a complete fix. For deltas that change
arity (add/remove parameters) it emits a `TODO` comment inside the new
descriptor because re-guessing the typedef blindly would hide a
SIGSEGV — that judgement call stays with a human, guided by
`README.md` section 5.

---

## 9. Lessons learned

1. **Always qualify cross-namespace types.** In a multi-namespace header
   like `a_native_window_creator.h`, forgetting a single `types::`
   qualifier can produce an O(N) cascade of unrelated-looking errors and
   a `did you mean …?` suggestion that points at something completely
   innocent.

2. **Trim string values parsed from text files.** Windows `for /f` on a
   CRLF file leaves trailing `\r` in the captured token. Always route
   parsed identifiers through a `:trim` helper before interpolating them
   into a path or a command.

3. **Don't conclude "symbol missing" from an empty filter output.**
   Confirm your tool is actually running first. In PowerShell that means
   using `& "..."` to execute a quoted path and `$env:VAR` instead of
   `%VAR%`. A `Select-String` that finds nothing could mean *the tool
   produced nothing*, not *the symbol is absent*.

4. **The resolver already tells you which symbol failed — read it.**
   The exception text is generic (`Failed to resolve symbol`), but the
   log line emitted *immediately before* the throw contains the exact
   mangled name. Don't guess; `adb logcat -d | Select-String 'failed to
   resolve'` is always faster.

5. **Private-NDK symbols are versioned by *mangling*, not by name.**
   When Android changes a function's parameter list, the old mangled
   name disappears from `.dynsym` and a new one takes its place. The
   function "is still there" from Google's point of view but it's a new
   symbol from `dlsym`'s.

6. **Keep old descriptors alive when you add new ones.** Support for
   Android 11 isn't free to throw away — it still works if you don't
   touch its `ApiDescriptor`. Every time we "upgrade", we split the
   existing entry (`11, UINT_MAX` → `11, 14` + `15, UINT_MAX`), we never
   replace.

7. **Two code paths, one surface.** Wherever possible, the version
   branching lives *inside* `MirrorSurface()` (or the equivalent
   wrapper), not at call sites scattered through the codebase. The
   caller doesn't care whether the mirror was created with one or two
   arguments — it just gets a `StrongPointer<void>` back.

8. **Write the tool you'll need next time, before you need it.**
   The patcher is only useful *if Android 17 introduces another
   delta* — and it will. Having `symbol_patcher.py` ready means the next
   regression is a 3-minute fix, not a 3-hour archaeology dig.
