# Android 15 / 16 NDK Symbol Patching Guide

> How to keep `a_native_window_creator.h` (and similar private-NDK consumers)
> working on new Android versions without reverse-engineering the whole stack
> every time.

> **NOTE:** <span style="color:red">This article was written by my friend to solve this problem. You can view the complete open-source code for this solution.</span>
>
> **GitHub:** <https://github.com/mohamad-aljeiawi/cping/tree/main/cping-memory-pubg>

---

## 0. Read this first (plain-English overview)

If you have never touched Android system libraries before, here is the short story.

Android ships a set of **public NDK headers** — the ones Google promises will not change. Under the hood, however, a lot of the interesting work (creating a window you can draw ImGui on, composing a surface, talking to `SurfaceFlinger`) is done by **private C++ classes** inside `libgui.so` and `libutils.so`. Our overlay uses those private classes directly by looking up their functions at runtime with `dlsym()`.

The problem is that the names of those private functions are not plain text like `mirrorSurface`. The C++ compiler encodes the full class, namespace, and parameter list into a single **mangled** string such as:

```
_ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE
```

Every time Google adds a parameter, renames a class, or moves something into a sub-namespace, that mangled string changes. `dlsym()` returns `nullptr`, our resolver throws, and the app crashes on boot. That is exactly what happens going from Android 14 → 15 → 16.

This document explains, step by step:

1. **How to see which symbol is missing** on a fresh Android version (logcat + `adb`).
2. **How to find the new name** inside the updated system library (`llvm-nm` + `llvm-cxxfilt`).
3. **How to read the mangled string** so you can understand what actually changed in the function signature.
4. **How to patch the resolver** so both old and new Android versions keep working — without deleting the old entry and breaking older devices.

**Glossary used throughout this text:**

- **mangled name** — the encoded string a C++ symbol has in the `.so` file. `llvm-cxxfilt` turns it back into a human-readable signature.
- **`dlopen` / `dlsym`** — the two libc calls used to load a shared library and look up a function inside it by name at runtime.
- **descriptor** — one row in our resolver's table. It says "for Android versions X..Y, look up symbol S and store the pointer at slot T".
- **signature** — the parameter list of a function. Changing it changes the mangled name.
- **visibility** — whether a symbol is exported (`T` in `nm`) or hidden inside the library (`t`, or simply absent from `nm -D`).

If any of the commands below mention `$env:NDK_PATH` it means we are in PowerShell on Windows; the same binaries exist on Linux/macOS under `$NDK_PATH/toolchains/llvm/prebuilt/<host>/bin/`.

---

## 1. Background

`a_native_window_creator.h` builds a native ImGui/overlay window by calling
**private C++ functions** inside the system libraries:

- `/system/lib64/libgui.so` — `SurfaceComposerClient`, `SurfaceControl`,
  `Surface`, `LayerMetadata`
- `/system/lib64/libutils.so` — `RefBase`, `String8`

*In beginner terms:* `libgui.so` is the shared library that owns everything related to composing and displaying surfaces on screen (it is the client side of `SurfaceFlinger`, Android's window compositor). `libutils.so` provides the foundation utilities — smart pointers, strings — that almost every other AOSP library builds on. Both live in `/system/lib64/` on 64-bit devices and they are already loaded in every process, so `dlopen()` just gives us a handle to them.

These are not stable NDK APIs. Google can (and does) change them between
Android releases — renaming, adding parameters, moving to new namespaces,
hiding them behind `__attribute__((visibility("hidden")))`, or removing
them entirely. When that happens, `dlsym()` returns `nullptr`, the resolver
throws `std::runtime_error("Failed to resolve symbol")` and the process
aborts with:

```
libc++abi: terminating due to uncaught exception of type St13runtime_error
Aborted
```

*What the error actually means:* `libc++abi` is the C++ runtime. "Uncaught exception of type `St13runtime_error`" is just `std::runtime_error` in mangled form (`St` = `std::`, `13` = length of `runtime_error`). The process aborts because nobody above us in the call stack is ready to catch that exception. Translation: we asked the dynamic linker for a function by name, it could not find it, we gave up, and the app died.

This document captures the **exact diagnostic loop** and the **patching
pattern** we worked out while moving the header from "supports up to Android
14" to "supports Android 16 too".

---

## 2. How the resolver works

Before diving into the numbered steps, here is the big picture. The resolver is a small piece of code that runs once at startup. Its job is to take a big list of "I need this function on this Android version" entries (descriptors) and, for each one, ask `dlsym()` to find the actual address of that function in the loaded system library. It stores each address into a pre-defined slot inside an `ApiTable` struct. Later, when normal code wants to call (for example) `mirrorSurface`, it does not call it directly — it reads the pointer out of the `ApiTable` and calls that. This indirection is the whole trick that lets one binary support many Android versions at once.

`android::anative_window_creator::detail::compat::ApiResolver::Resolve()`:

1. Reads `ro.build.version.release` → stores it in
   `compat::SystemVersion` (e.g. `16`).
2. `dlopen`s `libgui.so` and `libutils.so`.
3. Walks two descriptor tables (`libutilsApis`, `libguiApis`).
   Each descriptor has shape:

   ```cpp
   ApiDescriptor{
     /*minVersion*/ 11,
     /*maxVersion*/ UINT_MAX,
     /*storeToTarget*/ &apis::libgui::SurfaceComposerClient::Api.MirrorSurface,
     /*apiSignature*/  "_ZN7android21SurfaceComposerClient13mirrorSurface..."
   };
   ```
4. For each descriptor whose `[min,max]` covers `SystemVersion`:
   `dlsym(handle, apiSignature)` → store into the `Api` slot.
5. Any `nullptr` → log the missing symbol + throw.
6. `resolved = true`; later code calls into the stored pointers via
   `ApiInvoker<"TypedName">()(...)`.

*Reading the descriptor:* `minVersion=11, maxVersion=UINT_MAX` means "use this entry on Android 11 and everything newer". `storeToTarget` is the address of a `void*` slot that will receive the function pointer. `apiSignature` is the exact mangled symbol the resolver will search for with `dlsym()`. Later, the templated `ApiInvoker<"SurfaceComposerClient::MirrorSurface">()(...)` call at the use-site casts that `void*` back to the correct function-pointer type and invokes it with the given arguments — so switching to a new signature is a matter of switching to a new `ApiInvoker` template literal.

So failure always comes down to: **this version's exported mangled name does
not match what the descriptor asks for**. Our job is to find the new name
and teach the resolver about it — while keeping the old name working for
older devices.

---

## 3. Diagnostic loop (runbook)

This is the part you repeat every time a new Android version comes out and something breaks. The loop has four steps: **capture → pull → grep → interpret**. Do them in order and you will almost always end up with either a new mangled name to patch in, or a confirmed "the symbol is gone" verdict that forces a different approach.

### 3.1 Capture the missing symbol name

Run the binary and read logcat — the resolver **logs the exact symbol it
failed on** immediately before throwing:

```powershell
adb logcat -c
adb shell "su -c '/data/local/tmp/cping_memory_pubg'"
adb logcat -d | Select-String "failed to resolve"
```

Expected form:

```
E AImGui  : [!] Version[Android 16] [libgui] failed to resolve symbol:
  _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE
```

*How to read this line:* `E` is the logcat severity (error). `AImGui` is our tag. `[Android 16]` is the value the resolver read from `ro.build.version.release`, and `[libgui]` is which of the two libraries it was searching in. Everything after `symbol:` is the mangled string `dlsym()` just failed on — **copy that verbatim**; it is the input for the next step.

### 3.2 Pull the device's lib and list its exports

```powershell
$nm = "$env:NDK_PATH\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-nm.exe"
adb pull /system/lib64/libgui.so
& $nm -D libgui.so | Measure-Object -Line     # total export count
```

*What each piece does:* `llvm-nm -D` lists the **dynamic** symbol table (only `-D` symbols are visible to `dlsym`; the default symbol table often has stripped or local-only symbols that you could never link against at runtime). `adb pull` copies the exact `.so` that is actually running on the device — do not rely on a shell container image or an AOSP build; OEMs patch these libraries and the running copy is the source of truth.

> **Important on newer Android**: `U ...` lines are **undefined references**
> (this lib consumes that symbol but does not define it). Only `T ...`
> (text/defined) lines mean "this lib exports it and `dlsym` will find it".
> Use `Select-String " T "` if the output is noisy.

*nm letter key (a short one):* `T` = text, exported function. `t` = text, local (not reachable by `dlsym`). `D`/`d` = data. `U` = undefined, resolved from another library. `W` = weak. For our purposes only `T` matters.

### 3.3 Find the class::method the library still provides

The easy trick is to take everything up to the parameter list in the old
mangled name and use it as a prefix. For our example
`...SurfaceComposerClient13mirrorSurface...`:

```powershell
& $nm -D libgui.so | Select-String " T .*mirrorSurface"
```

We got one hit:

```
00000000002ca2c0 T _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_
```

The tail `EPNS_14SurfaceControlES2_` tells us the signature changed from
**one** `SurfaceControl*` to **two** (`S2_` is Itanium's back-reference to
the second namespace/type substitution — in this case the previous
`SurfaceControl*`).

*Walking that tail byte by byte:* `E` = end of the nested name (we're leaving `SurfaceComposerClient::mirrorSurface`). `P` = pointer. `NS_14SurfaceControlE` = nested namespace `android::` (`S_` back-refers to `android`) then the 14-character class name `SurfaceControl`, closed by `E`. That gives us the first parameter: `android::SurfaceControl*`. Then `S2_` is a shortcut meaning "re-use the 3rd substitution already seen" — which in this symbol is again `android::SurfaceControl*`. So the new function is `mirrorSurface(android::SurfaceControl*, android::SurfaceControl*)`: a second argument was added. If this looks cryptic now, the cheat sheet in Section 4 plus one `llvm-cxxfilt` call will make it obvious every time.

### 3.4 If the symbol is missing everywhere

Sometimes the grep returns nothing. That is the hard case: the function is not just renamed, it is gone from the library's public surface. Three explanations cover almost every real case.

When `nm` finds no `T` definition for the class::method on any device lib,
the symbol may have been:

- **Hidden** with `visibility("hidden")` — impossible to `dlsym`, has to be
  reimplemented or reached via Binder.
- **Moved to another lib** — e.g. `String8` sometimes lives in
  `libutils_binder.so` instead of `libutils.so`. Hunt with:

  ```powershell
  foreach ($lib in (adb shell "ls /system/lib64/*.so" ) -split "`n") {
      $lib = $lib.Trim(); if (!$lib) { continue }
      $local = "tmp_" + [IO.Path]::GetFileName($lib)
      adb pull $lib $local 2>$null | Out-Null
      if (Test-Path $local) {
          if ((& $nm -D $local | Select-String "<your symbol regex>")) {
              Write-Host "FOUND in $lib" -Foreground Green
          }
          Remove-Item $local
      }
  }
  ```

  See `symbol_patcher.py --scan-libs` for the automated version of this.

- **Inlined/header-only** — last-resort fix: reimplement in the header
  using the known memory layout of the class.

---

## 4. Mangled-name cheat sheet

The C++ standard does not specify how symbols are encoded in object files — each ABI picks a scheme. On every platform we care about (Linux, Android, macOS) clang and g++ both use the **Itanium C++ ABI**. "Mangling" just means packing the namespace, class name, method name, `const`-ness and parameter types into a single ASCII string that the linker and `dlsym()` can match exactly. Knowing how to read 90% of the encoding takes about ten minutes — the table below is that 90%.

| Prefix | Meaning |
|---|---|
| `_ZN` | Itanium namespaced symbol |
| `7android` | length-prefixed namespace: `android` |
| `21SurfaceComposerClient` | length-prefixed class: `SurfaceComposerClient` |
| `13mirrorSurface` | length-prefixed method name: `mirrorSurface` |
| `E` | end of nested name — parameters follow |
| `P` | pointer |
| `RK` | const reference |
| `NS_XXXClassE` | namespace-qualified type, nested (`android::XXXClass`) |
| `S0_`, `S1_`, `S2_`, … | back-reference to the N-th substitution already seen |
| `C1`, `C2` | complete / base constructor |
| `D0`, `D1`, `D2` | deleting / complete / base destructor |

*The "length-prefixed" pattern:* every identifier in Itanium mangling is written as `<length><text>`. `7android` means "the next 7 characters are a namespace/class/method name: `android`". This is why you can read a mangled string left-to-right even without a demangler: find a number, read that many characters, repeat.

*About substitutions (`S_`, `S0_`, `S1_`, …):* to keep mangled names shorter, every new namespace/class encountered is assigned a slot. `S_` is the very first one (slot 0), `S0_` is slot 1, `S1_` is slot 2, and so on. After `_ZN7android...` appears, `S_` thereafter means `android::`. That is why you see the same `S2_` pop up repeatedly in complex symbols — it is just "the 3rd thing I already spelled out".

Use `llvm-cxxfilt` to demangle if ever in doubt:

```powershell
echo _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_ `
    | & "$env:NDK_PATH\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-cxxfilt.exe"
# → android::SurfaceComposerClient::mirrorSurface(android::SurfaceControl*, android::SurfaceControl*)
```

---

## 5. The 5-step surgical patch pattern

Apply this **same template** every time a symbol's signature changes.

*Why five steps and not one?* Because the function pointer has to flow through three different layers before it can be called: a **type** (so the compiler knows the calling convention), a **storage slot** (so the resolver has somewhere to write the address), an **invoker** (so the call-site code can type-cast that slot back to a real function pointer), a **descriptor** (so the resolver knows which mangled name to look up on which Android version), and a **call-site branch** (so the right invoker is chosen at runtime). Steps 1–5 add exactly those five pieces, in order. Skip a step and you get either a compile error or — worse — a silent wrong-cast that crashes inside the vendor library.

### Step 1 — Add a new typedef (in the `generic::` sub-namespace)

```cpp
using SurfaceComposerClient__MirrorSurfaceWithParent =
    StrongPointer<void> (*)(void *thiz, void *mirrorFromSurface, void *parent);
```

*Reading the typedef:* C++ non-static member functions take a hidden `this` pointer as their first argument. When you cross an ABI boundary via `dlsym`, you have to spell that `this` out explicitly — that is what `void *thiz` is. Then come the real arguments (`mirrorFromSurface`, `parent`). The return type `StrongPointer<void>` is the Android smart pointer (`sp<T>`) returned by value. Matching the real signature exactly is critical: one missing argument = stack corruption.

### Step 2 — Add an `Api` slot

```cpp
struct ApiTable {
  ...
  void *MirrorSurface;
  void *MirrorSurfaceWithParent;   // NEW
  ...
};
```

*Why a new slot instead of overwriting `MirrorSurface`?* Because on Android 14 the old symbol still exists and we still want to call it there. Keeping old and new as two separate fields lets the same binary support both. Each slot is a raw `void*` on purpose — the type information lives in the typedef from Step 1, not here.

### Step 3 — Add an `ApiInvoker` branch that casts the slot to the new type

```cpp
if constexpr ("SurfaceComposerClient::MirrorSurfaceWithParent" == descriptor) {
  return reinterpret_cast<types::apis::libgui::generic::
                              SurfaceComposerClient__MirrorSurfaceWithParent>(
      apis::libgui::SurfaceComposerClient::Api.MirrorSurfaceWithParent);
}
```

*What `if constexpr` buys us:* this is compile-time dispatch on a string template parameter. Every `ApiInvoker<"...">()` call-site picks exactly one branch and the rest are eliminated — zero runtime overhead, and a typo in the string is a compile error rather than a silent wrong branch. The `reinterpret_cast` converts the raw `void*` back to the function-pointer type we defined in Step 1.

### Step 4 — Split the descriptor into version ranges

```cpp
// was: {11, UINT_MAX, &MirrorSurface, "...SurfaceControlE"}
ApiDescriptor{11, 14,
              &apis::libgui::SurfaceComposerClient::Api.MirrorSurface,
              "_ZN7android21SurfaceComposerClient13mirrorSurface"
              "EPNS_14SurfaceControlE"},
ApiDescriptor{15, UINT_MAX,
              &apis::libgui::SurfaceComposerClient::Api.MirrorSurfaceWithParent,
              "_ZN7android21SurfaceComposerClient13mirrorSurface"
              "EPNS_14SurfaceControlES2_"},
```

**Never** change the old descriptor's `min`. Only trim its `max` and add a
fresh `{max+1, UINT_MAX, ...}` record for the new API. This keeps older
devices working.

*Why the adjacent string literals?* C and C++ automatically concatenate `"abc" "def"` into `"abcdef"` at compile time. Splitting a long mangled name across two lines keeps the code inside sane column widths without introducing runtime concatenation cost.

### Step 5 — Branch at the call site on `compat::SystemVersion`

```cpp
types::StrongPointer<void> mirrorSurface;
if (compat::SystemVersion >= 15) {
  mirrorSurface =
      ApiInvoker<"SurfaceComposerClient::MirrorSurfaceWithParent">()(
          data, surface.data, parent.data);
} else {
  mirrorSurface = ApiInvoker<"SurfaceComposerClient::MirrorSurface@v14">()(
      data, surface.data);
}
```

*Reading the runtime gate:* `compat::SystemVersion` was set once in step 1 of `Resolve()` from the system property. The branch picks which invoker to use, which picks which slot to read, which was populated by which descriptor — the entire chain is tied together by the Android version number. The `@v14` suffix on the old invoker is just a tag we give the older form so it stays distinguishable after the new one takes the plain name.

> Watch out for namespace scope. Every bare type referenced inside the
> `detail::compat` namespace that actually lives in `types::` must be
> qualified — otherwise you get a cascade of "undeclared identifier" errors
> that *look* like they're about the call site but are really about the
> missing qualifier on one declaration a few lines above.

---

## 6. Supporting multiple Android versions simultaneously

The goal here is one binary that runs correctly on Android 11 through 16 (and beyond). The invariants below are the small set of rules that, if you keep all of them, guarantee exactly that — and if you break even one of them, some subset of users will crash on startup.

### Invariants to preserve
1. Each descriptor's `[min, max]` range must be **contiguous and
   non-overlapping** with any other descriptor targeting the same `Api`
   slot or the same logical method.
2. Version `UINT_MAX` is the open upper bound. Exactly one descriptor per
   "logical method" should end at `UINT_MAX` — the current-latest form.
3. Older descriptors must keep their original lower bound so devices that
   still have the old symbol continue to resolve.

### A typical lineage for a single method
```
mirrorSurface @ versions 11..14   → 1-arg, old mangling
mirrorSurface @ versions 15..N    → 2-arg, new mangling (current)
```

### Anti-patterns
- ❌ Adding a new entry without trimming the old one's `max` — both try to
  resolve, one fails, resolver throws.
- ❌ Changing `min`/`max` without matching the call site's version gate —
  the resolver succeeds but the code calls through the wrong signature and
  you get a SIGSEGV inside the vendor lib.
- ❌ Deleting an old descriptor — breaks every user still on that Android
  version.

*How to avoid each mistake in practice:*

- After editing a descriptor table, scan for the same `Api.<Slot>` and make sure the union of its `[min,max]` ranges is a single contiguous interval. A 30-second eyeball review catches most overlaps.
- Whenever you add a descriptor with a new mangled name, grep for every `ApiInvoker<"...">` that targets the old name and make sure there is a matching `if (compat::SystemVersion >= N)` guarding the call. If the gate is missing, the resolver will silently hand you the wrong function pointer and you will crash inside `libgui`.
- If you think an old descriptor is "dead code", it probably isn't — devices on Android 12/13 still exist. Leave it in unless you are deliberately dropping support.

---

## 7. Known good Android 15/16 deltas (cheat sheet)

A running log of real-world changes we have already patched. When you see a similar failure on a new device, check here first — the symbol you need may already be listed.

| Method | Old mangling tail | New mangling tail | Behavioural change |
|---|---|---|---|
| `SurfaceComposerClient::mirrorSurface` | `EPNS_14SurfaceControlE` | `EPNS_14SurfaceControlES2_` | Takes explicit `parent` — returned mirror is already attached; skip the separate `Reparent` transaction. |
| `SurfaceComposerClient::createSurface` (watch list) | `NS_13LayerMetadataE` | `NS_3gui13LayerMetadataE` | `LayerMetadata` moved into the `android::gui::` sub-namespace. |

> Add new rows as you find them. Keep the mangled tails short and exact
> — they're the input `symbol_patcher.py` uses when auto-matching.

---

## 8. Tooling — `symbol_patcher.py`

All of the above is captured in `ndk_patch_docs/symbol_patcher.py`.
See that script's `--help` and the `ARCHITECTURE` section inside the file
for detailed usage. TL;DR:

*What the tool automates for you:* it re-implements Section 3 (capture → pull → grep → interpret) plus Section 5 (the five-step patch) end-to-end. Given the failing mangled name and the header file, it pulls the system libraries, finds the best candidate replacement, writes out the new typedef + slot + invoker + descriptor + call-site gate, and leaves a `.bak` next to the header so you can diff or revert.

```powershell
# 1. Let the tool find a replacement for an unresolved symbol:
python ndk_patch_docs/symbol_patcher.py `
    --header jni/include/native/a_native_window_creator.h `
    --old-symbol "_ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlE"

# 2. Also let it scan device libs, choose the best new mangling, show a unified diff:
python ndk_patch_docs/symbol_patcher.py ... --scan-libs

# 3. Apply the patch in place (backups go next to the file with .bak extension):
python ndk_patch_docs/symbol_patcher.py ... --scan-libs --apply --min-new-version 15
```

---

## 9. References

- [Itanium C++ ABI — Name Mangling](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling)
- `$NDK_PATH/toolchains/llvm/prebuilt/<host>/bin/llvm-{nm,cxxfilt,readelf}`
- AOSP source: `frameworks/native/libs/gui/SurfaceComposerClient.cpp`
  (checks in against tag `android-16.0.0_r*`)

*What each reference is good for:* the Itanium ABI page is the authoritative grammar for the mangling — bookmark the "Type encodings" and "Compression" (substitution) sub-sections. `llvm-nm` lists symbols, `llvm-cxxfilt` demangles them back to readable C++, `llvm-readelf -d` shows a library's `NEEDED` entries and `SONAME` (useful when a symbol migrates between `.so` files). The AOSP source is the ground truth — if a signature change surprises you, read the commit log on the file for the matching Android release tag and you will find the rationale.
