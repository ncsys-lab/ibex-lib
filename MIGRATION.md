# MIGRATION.md — divergence catalog for `ncsys-lab/ibex-lib`

This fork carries **seven surgical patches** on top of mainline `ibex-team/ibex-lib`. Each is intended as an independent upstream PR; once any/all merge upstream, drop the corresponding commit. When all seven land, **delete the fork**.

Baseline: `master = origin/master = ibex-team/ibex-lib@65ed5877`.
Patch branch: `dreal-perf-patches` (7 code commits + 1 docs commit, 11 files / +163/-19 vs mainline excluding docs).

## The patches (chronological on branch)

### 1. `f5bf3361` — `function: lazy-init gradient`

`Function::init` eagerly constructs a `Gradient` object even when callers only use `Function::backward()`. For dReal's HC4 hot loop (`dreal4-cmake/src/dreal/contractor/contractor_ibex_fwdbwd.cc`), profiling traced ~65% of `Function::init` wall time to this allocation (see `dreal4-cmake/CODAC_MIGRATION.md`).

Defers the allocation via a small `lazy_grad()` private accessor that mutates `_grad` on first call using the same drop-const cast pattern as `Function::ibwd()`. **No behavior change for gradient API callers.** ibex's own 62-test gradient/Newton/Optimizer suite passes unchanged.

Files: `src/function/ibex_Function.h`, `src/function/ibex_FunctionBuild.cpp`. ~24 lines.

(Reworked from the cav26-era `edbd8159` patch, which set `_grad = nullptr` unconditionally and broke ibex's own tests. The lazy-accessor version is upstream-quality.)

### 2. `a512418b` — `Function::backward` callback

Adds an optional `std::function<void(int, const Interval&, const Interval&)>` parameter to `Function::backward()` (and the `HC4Revise` it delegates to). When provided, the callback fires once per narrowed variable with `(var_idx, before, after)`. Defaulted to a no-op so existing 2-arg call sites are unchanged.

This lets contractor consumers (dReal, others) track changed variables without the before/after `IntervalVector` snapshot pattern they currently use.

Files: `src/function/ibex_{ExprDomain.h,Function.h,HC4Revise.h,HC4Revise.cpp}`. ~16 lines.

(Cherry-picked from cav26-era `4d61b841`. Source-compatible; no caller side needs to change unless they want the perf win.) **Patches 5–7 below extend this commit's contract to handle vector/matrix args, reference aliasing, and the empty-box exception path.**

### 3. `9a23379c` — `parser.yc` namespace fix

Two unqualified `apply(...)` calls in `src/parser/parser.yc` (lines 317-318) resolve via ADL in modern C++ standards (C++17+). Under libc++ and libstdc++ that may surface `std::apply` or other candidates as ambiguities. Fix: qualify as `ibex::parser::apply(...)`. Also adds two missing `#include`s (`<vector>`, `<iostream>`) and corrects an incorrect Bison error message ("Flex" → "Bison").

Files: `src/parser/parser.yc`, `src/CMakeLists.txt`. ~6 lines.

(Cherry-picked from cav26-era `4646bc80` which itself cherry-picked `95e85666`. The smallest possible fix for this issue.)

### 4. `ca21f309` — mathlib aarch64/arm64 Linux support

`interval_lib_wrapper/gaol/3rd/mathlib-2.1.1/CMakeLists.txt` enumerates only x86_64/i386 Linux, Cygwin, Darwin, and PPC. arm64/aarch64 Linux falls through to `UNSUPPORTED_ARCH` and fails with "The target system is not supported by MathLib". Reuse the existing `MATHLIB_AARCH64` flag (already used by Darwin arm64) — the gated code paths are platform-agnostic within the aarch64 ISA, so no source changes are needed.

On Linux arm64 we leave `MATHLIB_LINUX` off: `DPChange.c` checks `(MATHLIB_LINUX && !X86_64BITS)` first and would route to `LINUX_DPChange.c` (x86 inline assembly) before reaching the `MATHLIB_AARCH64` branch.

Surfaced when building inside `dreal4-cmake/Dockerfile.dreal_ubuntu` on Apple Silicon (Docker defaults to native `linux/arm64`).

Files: `interval_lib_wrapper/gaol/3rd/mathlib-2.1.1/CMakeLists.txt`. ~8 lines.

### 5. `f04f5db5` — `function: fire backward callback for non-scalar args` (callback audit fix)

`ExprTemplateDomain::read_arg_domains`'s non-scalar branch (`src/function/ibex_ExprDomain.h:210-212`) delegated component copy-back to the callback-less `load(box, args, used)` overload. **Functions with vector- or matrix-typed `ExprSymbol` arguments would silently miss every per-component narrowing notification** — a real soundness gap for any consumer using the callback to track changed variables for downstream reasoning (e.g. SMT theory-lemma generation).

Adds a callback-aware overload `load(box, domains, used, callback)` to `src/arithmetic/ibex_TemplateDomain.h` mirroring the existing scalar/vector/matrix switch; fires the callback per component before each `x[i] = ...` assignment. Wires the non-scalar branch of `read_arg_domains` to use it. Existing callers of the no-callback overload see no behavior change.

Files: `src/arithmetic/ibex_TemplateDomain.h`, `src/function/ibex_ExprDomain.h`. ~100 lines additive (~85 lines for the new overload).

### 6. `1836b569` — `function: copy old-value in backward callback to avoid alias` (callback audit fix)

`ExprDomain::read_arg_domains`'s scalar branch bound `old_value` as `const auto&` to `box[*j]`, then overwrote `box[*j]` on the next line. The reference is valid during the callback invocation but becomes stale immediately after: a callback that retains `old_value` would silently see the new value.

Copy by value: `const Interval old_value = box[*j];`.

Files: `src/function/ibex_ExprDomain.h`. 1 line + 3 lines of comment.

### 7. `d2b978b9` — `HC4Revise: report partial narrowings on EmptyBoxException` (callback audit fix)

`HC4Revise::proj`'s catch block called `x.set_empty()` and returned, losing any narrowings that completed before the contradiction. Callbacks tracking which variables narrowed got no events for the partial progress; consumers (e.g. SMT theory-lemma generation) had to over-approximate to "every variable in the box might have caused this," reducing lemma precision.

Calls `d.read_arg_domains(x, callback)` in the catch block before `set_empty()`. The callback fires for each component whose domain narrowed during the partial backward sweep; the subsequent `set_empty()` still drives the final infeasibility signal. Sound by construction: the callback contract is "old ≠ new per box index", which holds regardless of inter-component consistency in the mid-propagation domain state.

Files: `src/function/ibex_HC4Revise.cpp`. ~7 lines.

## Build & rebase cadence

- Build: `cd build && cmake -DINTERVAL_LIB=gaol -DLP_LIB=none .. && make -j && make check`. 62/62 tests pass on macOS arm64 native with clang.
- Rebase: re-apply the 7 commits onto `ibex-team/ibex-lib@HEAD` on every mainline minor release. If one of the patches lands upstream, drop it from the rebase.

## Archive branches (historical reference, not maintained)

- `archive/cav26-base` — cav26-era `be485777`, the canonical pre-Codac dReal IBEX pin
- `archive/pre-modernization-filib`, `archive/v2.8.9-m1-superseded`, `archive/v2.7.4-m1-superseded` — older platform-port branches

These exist to make `git blame` archaeology possible; they're not part of any maintenance cycle.
