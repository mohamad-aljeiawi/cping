# Android 15 / 16 NDK Symbol Patching Guide

> How to keep `a_native_window_creator.h` (and similar private-NDK consumers)
> working on new Android versions without reverse-engineering the whole stack
> every time.

---

## 1. Background

`a_native_window_creator.h` builds a native ImGui/overlay window by calling
**private C++ functions** inside the system libraries:

- `/system/lib64/libgui.so` — `SurfaceComposerClient`, `SurfaceControl`,
  `Surface`, `LayerMetadata`
- `/system/lib64/libutils.so` — `RefBase`, `String8`

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

This document captures the **exact diagnostic loop** and the **patching
pattern** we worked out while moving the header from "supports up to Android
14" to "supports Android 16 too".

---

## 2. How the resolver works

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

So failure always comes down to: **this version's exported mangled name does
not match what the descriptor asks for**. Our job is to find the new name
and teach the resolver about it — while keeping the old name working for
older devices.

---

## 3. Diagnostic loop (runbook)

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

### 3.2 Pull the device's lib and list its exports

```powershell
$nm = "$env:NDK_PATH\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-nm.exe"
adb pull /system/lib64/libgui.so
& $nm -D libgui.so | Measure-Object -Line     # total export count
```

> **Important on newer Android**: `U ...` lines are **undefined references**
> (this lib consumes that symbol but does not define it). Only `T ...`
> (text/defined) lines mean "this lib exports it and `dlsym` will find it".
> Use `Select-String " T "` if the output is noisy.

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

### 3.4 If the symbol is missing everywhere

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

Use `llvm-cxxfilt` to demangle if ever in doubt:

```powershell
echo _ZN7android21SurfaceComposerClient13mirrorSurfaceEPNS_14SurfaceControlES2_ `
    | & "$env:NDK_PATH\toolchains\llvm\prebuilt\windows-x86_64\bin\llvm-cxxfilt.exe"
# → android::SurfaceComposerClient::mirrorSurface(android::SurfaceControl*, android::SurfaceControl*)
```

---

## 5. The 5-step surgical patch pattern

Apply this **same template** every time a symbol's signature changes.

### Step 1 — Add a new typedef (in the `generic::` sub-namespace)

```cpp
using SurfaceComposerClient__MirrorSurfaceWithParent =
    StrongPointer<void> (*)(void *thiz, void *mirrorFromSurface, void *parent);
```

### Step 2 — Add an `Api` slot

```cpp
struct ApiTable {
  ...
  void *MirrorSurface;
  void *MirrorSurfaceWithParent;   // NEW
  ...
};
```

### Step 3 — Add an `ApiInvoker` branch that casts the slot to the new type

```cpp
if constexpr ("SurfaceComposerClient::MirrorSurfaceWithParent" == descriptor) {
  return reinterpret_cast<types::apis::libgui::generic::
                              SurfaceComposerClient__MirrorSurfaceWithParent>(
      apis::libgui::SurfaceComposerClient::Api.MirrorSurfaceWithParent);
}
```

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

> Watch out for namespace scope. Every bare type referenced inside the
> `detail::compat` namespace that actually lives in `types::` must be
> qualified — otherwise you get a cascade of "undeclared identifier" errors
> that *look* like they're about the call site but are really about the
> missing qualifier on one declaration a few lines above.

---

## 6. Supporting multiple Android versions simultaneously

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

---

## 7. Known good Android 15/16 deltas (cheat sheet)

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
