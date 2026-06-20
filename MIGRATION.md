# MIGRATION.md — divergence catalog for `ncsys-lab/ibex-lib`

This fork carries **eleven surgical patches** on top of mainline `ibex-team/ibex-lib`. Each is intended as an independent upstream PR; once any/all merge upstream, drop the corresponding commit. When all eleven land, **delete the fork**.

Baseline: `master = origin/master = ibex-team/ibex-lib@65ed5877`.
Patch branch: `dreal-perf-patches` (11 code commits + per-patch docs commits, 23 files / +1019/-399 vs mainline excluding docs).

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

### 8. `33eb6676` — `gaol: fix Interval::log and Interval::pow soundness gaps`

Two corrections to the gaol wrapper that affect rigorous overapproximation:

- **`Interval::log`** previously guarded `x.ub() <= 0` and returned `EMPTY_SET`. For `x = [0,0]` (and any input whose upper bound is exactly 0), that short-circuits before gaol's own `(-oo,-DBL_MAX]` result can surface, so `log([0,0])` came back empty. dReal's HC4 contractor on transcendentals would then declare a sound branch infeasible. Switch to strict `<`.
- **`Interval::pow(x, double d)`** dispatched to gaol's scalar exponent overload, which silently mishandled fractional `d` (e.g. `pow([1,4], 0.5)` returned `[1,1]` instead of `[1,2]`). Wrap `d` in a degenerate `gaol::interval(d, d)` to take the (interval,interval) overload, which handles fractional exponents correctly.

Updates `tests/TestArith.cpp` `log04` and `log10` to expect `(-oo,-DBL_MAX]` instead of `empty_set()` — the previously-commented-out alternative expectations become canonical under the new contract.

Both wrapper fixes originate in `dreal-deps/ibex-lib` commits by Soonho Kong (Sep 2018): `fe3eb925` (log) and `60218733` (pow). Previously bundled as cav26-era `ebc65b84`; re-applied here onto modernized mainline with matching test updates.

Files: `interval_lib_wrapper/gaol/ibex_IntervalLibWrapper.inl`, `tests/TestArith.cpp`. 8 lines (+ ~4 lines of explanatory comment in the wrapper).

### 9. `e0311233` — gaol: inline aarch64 FPCR rounding-mode fast-path

The directed interval transcendentals (`gaol_double_op_apmathlib.h`) toggle the FPU rounding mode nearest⟷upward around every correctly-rounded IBM-mathlib call. On ARM64 each toggle is a libc `fesetround()` call (`gaol_fpu_fenv.h` `round_downward`/`round_upward`/`round_nearest`); profiling dReal's transcendental-dense (`odeexpr`) workloads attributes **23–44% of solve time** to it. Replace the three setters' bodies, under `#if defined(__aarch64__)`, with an inline `mrs`/`msr` read-modify-write of just the FPCR RMode field (bits [23:22]) — the primitive CAPD's NATIVE `DoubleRounding` already uses. **Bit-identical** to `fesetround`: identical RMode encoding (00=nearest, 01=+∞, 10=−∞), every other FPCR bit preserved. macOS `fesetround` is itself literally `mrs`/`msr` with no `isb`, so this is the same instruction sequence minus the non-inlined call frame. The `fesetround` body is retained for every other architecture.

Lives in the vendored-gaol build patch, not ibex source: a new `gaol_fpu_fenv.h` hunk in `gaol-4.2.3alpha0.all.all.patch`. Upstreamable to gaol (Frédéric Goualard) or to ibex's vendored gaol patch. Verified bit-identical to baseline by dReal's `gaol_transcendental_bitidentity` gate (in libgaol's `gaol_interval.o`: baseline's external `_fesetround` → 121 inline `msr fpcr`, 0 `fesetround`).

Files: `interval_lib_wrapper/gaol/3rd/gaol-4.2.3alpha0.all.all.patch`. +56 patch lines.

### 10. `3902fa35` — gaol: batch the nearest-rounding region in transcendentals

Builds on #9. Each interval transcendental computed its two directed bounds via separate `<f>_dn`/`<f>_up` helpers, and each helper does `round_nearest()` … `round_upward()` — so one interval transcendental toggled the FPU mode nearest⟷upward **twice** (~4 mode writes). Add paired `<f>_dn_up(x_dn, x_up, *lo, *hi)` helpers in `gaol_double_op_apmathlib.h` that evaluate both correctly-rounded calls inside **one** `round_nearest()`/`round_upward()` pair, and switch the `gaol_interval.cpp` call sites to them (exp, log, tan, acos, asin, atan, cosh, sinh, tanh, cos; `sin` routes through `cos`). Halves the mode toggles per transcendental (~4→2). The soundness-critical argument reduction (`Il/pi_up`, `Ir/pi_dn`) stays in FE_UPWARD, never pulled into the nearest window. apmathlib is the only active backend (the gaol build copies `gaol_double_op_apmathlib.h` to `gaol_double_op.h`), so the helpers live there; the rare `cos` `nm==2` branch and single-call `cosh` branches are left unbatched.

**Bit-identical** to the unbatched helpers by construction: same mathlib routine, same FE_TONEAREST window, same `previous_float`/`next_float` outward bump, same order (lo from `x_dn`, then hi from `x_up`) — only the placement/count of the FPU mode switches changes. Verified by the same `gaol_transcendental_bitidentity` gate. Separate commit from #9 so the two levers can be benchmarked/reverted independently.

Files: `interval_lib_wrapper/gaol/3rd/gaol-4.2.3alpha0.all.all.patch`. +250/−14 patch lines.

### 11. `cc6fb001` — `function: signal empty domains via return-status instead of EmptyBoxException`

The forward-backward contractor (`HC4Revise`) and its siblings signalled "a domain emptied" by **throwing** a protected, nested `EmptyBoxException`, caught in `proj()`/`iproj()`. On systems that prune to empty at high frequency (UNSAT-style decrease/positivity proofs), the per-throw C++ unwinding (`__cxa_throw`/`_Unwind_*`, table-based on ARM64) is paid on the contraction hot path — macOS `sample` measured it at up to **~27%** of CPU on the throw-densest benchmarks.

Replaces the exception control flow with a `bool` return-status threaded through the shared backward engine: `BwdAlgorithm`'s `*_bwd` interface returns `bool`; `CompiledFunction::backward<V>` short-circuits on the first `false` (an emptied domain) and returns it; `HC4Revise`/`InHC4Revise` `*_bwd`/`backward`/`iproj` return their primitive's bool and the nested `EmptyBoxException` classes + all `try/catch` are removed; `Gradient`'s `*_bwd` return `true` (gradient never contradicts); `Function::backward<V>` returns `bool`. The **public** `Function::backward(y, x, callback)` signature/semantics are unchanged, so external callers (`CtcFwdBwd`, `CtcInverse`, `SepInverse`) and downstream users (dReal) are source-compatible — they already detect emptiness via `x.is_empty()`. No globals/thread-locals; `Eval`'s forward `*_fwd` path is untouched (it catches its own forward exceptions internally; the resulting empty root is reported by `backward()`).

**Bit-identical contraction** (a control-flow refactor, not numeric): all existing HC4/InHC4/Gradient/contractor tests pass unchanged. Adds `TestHC4Revise::empty01/empty02` and `TestInHC4Revise::empty01` (direct empty-propagation assertions). Landed via a measured Tier-0 → escalate strategy in dReal (convert the shallow root throw, profile, then convert the deep `*_bwd` throws); the final patch is the coherent whole. Eliminates `__cxa_throw` from the dReal odeexpr hot path entirely (26.6%/5.2% → 0% on the two throw-densest benchmarks); see dReal's `OPTIMIZATION_LOG.md`.

Files: `src/function/ibex_{BwdAlgorithm,CompiledFunction,Function,Gradient,HC4Revise,InHC4Revise}.{h,cpp}`, `tests/Test{HC4Revise,InHC4Revise}.{h,cpp}`. +432/−370.

## Build & rebase cadence

- Build: `cd build && cmake -DINTERVAL_LIB=gaol -DLP_LIB=none .. && make -j && make check`. 62/62 tests pass on macOS arm64 native with clang.
- Rebase: re-apply the 11 commits onto `ibex-team/ibex-lib@HEAD` on every mainline minor release. If one of the patches lands upstream, drop it from the rebase.

## Archive branches (historical reference, not maintained)

- `archive/cav26-base` — cav26-era `be485777`, the canonical pre-Codac dReal IBEX pin
- `archive/pre-modernization-filib`, `archive/v2.8.9-m1-superseded`, `archive/v2.7.4-m1-superseded` — older platform-port branches

These exist to make `git blame` archaeology possible; they're not part of any maintenance cycle.
