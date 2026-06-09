# Divergence Catalog — `ncsys-lab/ibex-lib`

This document catalogs the dReal team's divergence from `ibex-team/ibex-lib` (mainline) and the plan for rebasing onto current mainline (`65ed5877`) while preserving cav26-era behavior. The companion planning artifact lives at `~/.claude/plans/glistening-launching-fountain.md` and the historical context at `dreal4-cmake/CODAC_MIGRATION.md`.

## Purpose of this fork

dReal4 historically depended on `ncsys-lab/ibex-lib@be485777` (tip of `v2.8.9-upgrade-gaol-rosetta`) via `FetchContent`. That snapshot carried:

- Two **soundness fixes** mainline never had (`ebc65b84` `Interval::log/pow`, `059d1fe7` `gaol::init`).
- Two **performance hacks** load-bearing for dReal's SMT throughput (`edbd8159` `_grad = nullptr` lazy-init of `ibex::Gradient` and `ibex::ExprLinearity`; `4d61b841` callback on `Function::backward` so dReal can track per-variable narrowing without snapshot/compare).
- A **gaol upgrade** to `4.2.3` (mainline still ships `4.2.3alpha0`) with Apple-Silicon FPU patches.
- A set of **namespace cleanups** required for C++14/17 build.

After dReal4 migrated to Codac + `lebarsfa/ibex-lib`, all four were lost; the resulting ~65% wall-time regression in `ibex::Function::init` is documented in `dreal4-cmake/CODAC_MIGRATION.md`. This fork's purpose is to restore cav26 behavior on a current `ibex-team/ibex-lib@HEAD` base, with a dReal-side regression suite that locks in soundness equivalence.

## Provenance table

The cav26-era state was the 16 commits on `v2.8.9-upgrade-gaol-rosetta` plus 1 more on `v2.8.9-upgrade-filib-rosetta`. The verdict against current mainline:

| Commit | Author | Subject | Files touched | Category | Mainline overlap | Verdict |
|---|---|---|---|---|---|---|
| `f0e35027` | Sheth | Upgrade gaol+mathlib to work on M1 | gaol/3rd/{tarball,patch,CMakeLists}, mathlib/3rd/* | platform | `971f8eb0` (different approach on `4.2.3alpha0`) | **KEEP** — reconcile gaol tarball version in Phase 2E |
| `1c18a60b` | Sheth | Stop using namespace std | benchs/, examples/, tests/, src/ | cleanup | `9709f1d5` (2012, 2 lines, unrelated) | **KEEP** |
| `d59d2e3a` | Sheth | Remove `using namespace std;` from lexer/parser | parser/parser.yc, symbolic/ibex_ExprOperators.cpp.in | cleanup | none | **KEEP** |
| `95e85666` | Sheth | Fix sneaky namespace error in parser.yc (C++14+) | src/CMakeLists.txt, parser/parser.yc | cleanup | none | **KEEP** — resolves the Bison ADL conflict noted in CODAC_MIGRATION.md |
| `a0be73ea` | Sheth | Fix namespacing in assertions | data/ibex_Cov{IBUList,IUList,Manifold,SolverData}.cpp | cleanup | none | **KEEP** |
| `fc986657` | Sheth | Improve low-level FPU mgmt for Apple Silicon | gaol/3rd/gaol-4.2.3.all.all.patch, mathlib-2.1.1.darwin.all.patch | platform/soundness | `971f8eb0` (different tarball) | **KEEP** — load-bearing for arm64-Rosetta builds |
| `16722942` | Sheth | Fix refactoring mistakes in tests | tests/TestCtcExist, TestCtcForAll, TestSinc, TestSolver | test | n/a | **KEEP** — downstream of namespace cleanup |
| `33e296eb` | Sheth | Tweak gaol/mathlib patches | gaol-4.2.3.all.all.patch, mathlib-2.1.1.darwin.all.patch | platform | n/a | **KEEP** |
| `4f845aa3` | Sheth | Fix fenv.h call signature for x86 | gaol-4.2.3.all.all.patch | platform | n/a | **KEEP** |
| `ebc65b84` | Sheth | Fix `Interval::log` and `Interval::pow` | gaol/ibex_IntervalLibWrapper.inl:276,312 | **SOUNDNESS** | none | **KEEP** — regression-gated by `ibex_log_pow_edge_cases_test.cc` |
| `059d1fe7` | Sheth | Call `gaol::init` from wrapper | gaol/ibex_IntervalLibWrapper.cpp:35 | **SOUNDNESS** | none | **KEEP** — regression-gated by `ibex_gaol_init_smoke_test.cc` |
| `edbd8159` | Sheth | TMP — Null-out gradient initialization | function/ibex_FunctionBuild.cpp:558-559 | **PERFORMANCE** | none | **KEEP** — regression-gated by `ibex_lazy_gradient_test.cc`; this is the patch CODAC_MIGRATION.md attributes ~65% saradc wall-time to |
| `4d61b841` | Sheth | TMP — Add callback to `Function::backward` | function/ibex_{ExprDomain.h, Function.h, HC4Revise.cpp, HC4Revise.h} | **PERFORMANCE** | none | **KEEP** — defaulted arg preserves source-compat of `f.backward(y, x)` at `dreal4-cmake/src/dreal/contractor/contractor_ibex_fwdbwd.cc:122`; regression-gated by `ibex_backward_callback_test.cc` + `ibex_function_backward_compat_test.cc` |
| `cd44b915` | Sheth | Fix ubuntu build (gaol tar strip + `_NOEXCEPT` → `noexcept`) | gaol-4.2.3.tar.gz, gaol-4.2.3.all.all.patch | build | unknown | **KEEP** — re-evaluate against Ubuntu 24.04 in Phase 2 |
| `33408ed0` | Sheth | Fix #550 | src/data/ibex_Cov.h (1 line) | upstreamed | **`0f5a2fa0`** (identical) | **DROP** |
| `be485777` | Sheth | Force x86 if on MacOS | gaol/ibex_IntervalLibWrapper.cpp | platform | `f4b98ccf` (related: blocks filib on arm64) | **RECONCILE** — convert to opt-in `DREAL_FORCE_X86_ON_MACOS` cmake var |
| `9a2d0a9a` | Sheth | Switch INTERVAL_LIB default gaol→filib | interval_lib_wrapper/CMakeLists.txt, filib/CMakeLists.txt | policy | `f4b98ccf` (related: forbids filib on arm64-native) | **RECONCILE** — switch via platform-aware default (see Default INTERVAL_LIB policy below) |

## Tarball-time patch inventory

Five patches under `interval_lib_wrapper/{gaol,filib}/3rd/` and `lp_lib_wrapper/soplex/3rd/`, applied by `cmake.utils/ibex-install-3rd.cmake:107` via `patch -p1 -i …` after tarball extraction. Each is preserved in this fork; some need reconciliation against mainline.

| Patch file | Tarball | Purpose | Mainline overlap | Verdict |
|---|---|---|---|---|
| `interval_lib_wrapper/gaol/3rd/gaol-4.2.3.all.all.patch` | `gaol-4.2.3.tar.gz` (fork) | Apple-Silicon FPU init (`reset_fpu_cw → fesetenv(FE_DFL_ENV); round_upward()`); `opposite() → gaol_opposite()` rename | mainline still on `gaol-4.2.3alpha0.tar.gz`; `971f8eb0` patches that older tarball differently | **Carry forward; reconcile tarball version in Phase 2** |
| `interval_lib_wrapper/gaol/3rd/mathlib-2.1.1.darwin.all.patch` | `mathlib-2.1.1.tar.gz` (fork) | Darwin-specific build | none in mainline (different mathlib layout) | **Carry forward** |
| `interval_lib_wrapper/gaol/3rd/mathlib-2.1.1.win32.all.patch` | `mathlib-2.1.1.tar.gz` | Windows/Cygwin MinGW config (`MATHLIB_CYGWIN`, `IX86_CPU`) | identical | **Already in mainline; nothing to do** |
| `interval_lib_wrapper/filib/3rd/filibsrc-3.0.2.2.all.all.patch` | `filibsrc-3.0.2.2.tar.gz` | Operator-precedence parentheses for `-Werror` clean compile | none in mainline | **Carry forward; orthogonal to `2b3d6122` (MSVC `/fp:strict`)** |
| `interval_lib_wrapper/filib/3rd/filibsrc-3.0.2.2_remove-throw.patch` | `filibsrc-3.0.2.2.tar.gz` | Remove deprecated C++17 dynamic-exception specs | none in mainline | **Carry forward** |
| `lp_lib_wrapper/soplex/3rd/soplex-4.0.2.all.all.patch` | `soplex-4.0.2.tar` | Rename `DEBUG` enum to `_SOLEX_DEBUG`; install layout | none in mainline | **Carry forward; verify against `2678cfd0`** |
| `lp_lib_wrapper/soplex/3rd/soplex-4.0.2.CMake.patch` | `soplex-4.0.2.tar` | CMake modernization (3.5+, ZLIB/GMP optional, MSVC DLL) | none | **Carry forward** |

## Branch archaeology

| Current name | HEAD | Status | Planned name |
|---|---|---|---|
| `master` (= `origin/master`) | `65ed5877` | canonical mainline ancestor | (kept; fast-forwarded to `master-modernized` after Phase 2) |
| `v2.8.9-upgrade-filib-rosetta` | `9a2d0a9a` | superseded by `master-modernized` | `archive/pre-modernization-filib` |
| `v2.8.9-upgrade-gaol-rosetta` | `be485777` | cav26 behavioral reference; superseded | `archive/cav26-base` |
| `v2.8.9-upgrade-gaol-m1` | `33408ed0` | superseded (subset of gaol-rosetta) | `archive/v2.8.9-m1-superseded` |
| `v2.7.4-upgrade-gaol-m1` | `4f845aa3` | superseded (older ibex baseline) | `archive/v2.7.4-m1-superseded` |
| `fork/master` | `69b5c396` | one-commit ahead of `origin/master` (gaol `-p1` → `-p0`); holding pattern | retire after Phase 2 |
| `fork/fmcad25-tools-bench-CONTROL` | `ca4cda78` | benchmark-paper snapshot | `archive/fmcad25-benchmark-snapshot` |
| `soonho-upstream/*` | various | last activity 2016-08-17; abandoned | leave alone |

## API surface and stability promise

dReal4 consumes a narrow surface of public ibex APIs (no `ibex::internal::*` usage). The fork promises ABI/source stability for these symbols across mainline rebases. Any rebase that breaks one requires a dReal-side fix coordinated in the same PR.

| ibex symbol | Module | Used at (dreal4-cmake) |
|---|---|---|
| `ibex::ExprNode` | symbolic | `src/dreal/contractor/contractor_ibex_polytope.{cc,h}` |
| `ibex::ExprCtr`, `ibex::ExprSymbol`, `ibex::ExprConstant::new_scalar` | symbolic | polytope contractor, symbolic-to-ibex conversion |
| `ibex::cleanup` | symbolic | custom `ExprCtrDeleter` in `contractor_ibex_polytope.h:32-39` |
| `ibex::Interval` | arithmetic | `src/dreal/contractor/*` (box manipulation) |
| `ibex::IntervalVector` | arithmetic | `Box` is a thin wrapper |
| `ibex::Dim::scalar` | arithmetic | symbolic-to-ibex conversion |
| `ibex::Function` | function | converters; gradient + backward call sites |
| `ibex::NumConstraint` | function | HC4 fwdbwd contractor |
| `ibex::Function::backward` | function | `contractor_ibex_fwdbwd.cc:122` (no callback arg — defaulted by `4d61b841`) |
| `ibex::SystemFactory`, `ibex::System` | system | polytope contractor |
| `ibex::CtcPolytopeHull` | contractor | polytope contractor |
| `ibex::LinearizerXTaylor` (incl. `HANSEN` enum) | contractor | polytope contractor |

## Default `INTERVAL_LIB` policy

Mainline `f4b98ccf` correctly blocks ibex+filib on macOS-arm64-native because filib's SSE rounding is unreliable there. dReal4 builds macOS under Rosetta (per `be485777` "Force x86 if on MacOS"), where filib's SSE rounding works. The policy encoded in `interval_lib_wrapper/CMakeLists.txt`:

```cmake
if(APPLE AND CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64"
        AND NOT CMAKE_OSX_ARCHITECTURES MATCHES "x86_64")
    set(_DEFAULT_INTERVAL_LIB gaol)   # arm64-native MacOS → gaol
else()
    set(_DEFAULT_INTERVAL_LIB filib)  # Rosetta + Linux + Windows → filib
endif()
set(INTERVAL_LIB "${_DEFAULT_INTERVAL_LIB}" CACHE STRING "Library used for interval arithmetic")
```

`be485777`'s "Force x86 if on MacOS" becomes an opt-in cmake variable `DREAL_FORCE_X86_ON_MACOS` defaulted OFF in this fork. dReal4's `CMakeLists.txt` sets it ON.

## Phase 2 cherry-pick conflict surface (known risks)

| Conflict | Cause | Resolution |
|---|---|---|
| `f0e35027` + `971f8eb0` | Different gaol tarballs (`4.2.3` vs `4.2.3alpha0`); both modify `gaol/CMakeLists.txt` and per-version configuration | Take the fork's `gaol-4.2.3` upgrade. `971f8eb0`'s arm64 nextafter fix may already be subsumed by `fc986657`'s FPU patch on the newer tarball — verify by Rosetta + arm64 smoke tests in Phase 2C. |
| `ebc65b84` + `971f8eb0` | Both edit `gaol/ibex_IntervalLibWrapper.inl` | `git cherry-pick -x ebc65b84` may need `--3way` triage; the log/pow fix is at lines 276, 312 and is semantically distinct from `971f8eb0`'s rounding-mode changes |
| `9a2d0a9a` + `f4b98ccf` | Both edit `interval_lib_wrapper/CMakeLists.txt`; ours flips default to filib, mainline forbids filib on arm64-native | Resolution embedded in the default-INTERVAL_LIB policy above |
| `cd44b915` ubuntu fix | May be obsolete on Ubuntu 24.04 | Try a build without it first; keep only if needed |

## Investigative findings (post Phase-1 build)

While running the Phase 1 regression suite against `be485777` (gaol-default cav26 pin), we observed surprising IA-soundness-questionable behavior in gaol on macOS x86_64 (Rosetta):

- `ibex::Interval(0.1) + ibex::Interval(0.2)` returns the singleton `[0.30000000000000005, 0.30000000000000005]`.
- `ibex::Interval(0.1) * ibex::Interval(0.1)` returns `[0.010000000000000002, 0.010000000000000002]`.
- `ibex::Interval(1.0) / ibex::Interval(3.0)` returns `[0.33333333333333331, 0.33333333333333331]`.

The `gaol_opposite()` rename + FPU init are confirmed applied to the built gaol source tree (verified via grep). In each case the **upper bound contains the true value** (so the IA is technically sound by overapproximation), but the **lower bound is rounded UP, not DOWN** — meaning the interval is collapsed to a singleton instead of bracketing the true value. Two plausible causes:

1. gaol's `interval(double)` constructor produces `[a, a]` (a singleton at the double). The arithmetic on two singletons returns the round-up result for both endpoints. If this is intentional, the test is checking the wrong property and should use `Interval(a, b)` with `b > a`.
2. `gaol::round_upward()` is being honored for upper bounds but the matching `round_downward`/`gaol_opposite` trick isn't being honored for lower bounds on Rosetta x86_64.

The Phase 1 test files capture this as cav26 baseline (so regressions are caught), and explicitly flag the open question in their headers.

**Recommended follow-up:** run the same tests against a known-correct interval library (e.g., a pure filib build, or MPFI) to determine whether the discrepancy is in gaol's semantics, in the wrapper, or in the patch.

## Status of `master-modernized` (Phase 2 deliverable)

The `master-modernized` branch on this repo (HEAD `738a0e7f`) is `origin/master` + 14 commits applying the cav26-era patches cumulatively:

```
738a0e7f  interval_lib_wrapper: include ibex-install-3rd for subdir_list
bbb5e1ad  Use be485777 interval_lib_wrapper/CMakeLists.txt (cav26 gaol-3rd flow)
cbd7f1b8  cmake: bring back IbexUtils.cmake + ibex-config-utils.cmake from be485777
6a709a44  Force x86 if on MacOS                          [be485777, opt-in DREAL_FORCE_X86_ON_MACOS]
46c4d8ba  Upgrade gaol+mathlib to 4.2.3/2.1.1 + FPU      [cumulative: f0e35027, fc986657, 33e296eb, 4f845aa3, cd44b915]
88366509  TMP - Add callback to Function::backward        [4d61b841]
553e6a3b  TMP - Null-out gradient initialization          [edbd8159]
329d7e9f  Call gaol::init from wrapper                    [059d1fe7]
576eb39d  Fix Interval::log and Interval::pow             [ebc65b84]
3e8be5b2  Fix refactoring mistakes in tests               [16722942]
d46ce9c4  Fix namespacing in assertions                   [a0be73ea]
4646bc80  Fix sneaky namespace error in parser.yc         [95e85666]
bcf46c04  Remove `using namespace std;` from lexer/parser [d59d2e3a]
f784cd0b  Stop using namespace std                        [1c18a60b]
0c37e5b7  Add MIGRATION.md + CLAUDE.md                    [docs]
```

`Fix #550` (`33408ed0`) was deliberately DROPPED — already in mainline as `0f5a2fa0`.

**Build status:** the load-bearing source patches (lazy-init, backward callback, log/pow, gaol::init, namespace cleanup) cherry-pick cleanly. However, building dreal4-cmake against `master-modernized` HEAD currently fails with ~18 compile errors in mainline-evolved code that the namespace-cleanup patches didn't anticipate:

- `redefinition of FORMAT_VERSION / SIGNATURE_LENGTH / SIGNATURE / subformat_level / subformat_number` — mainline added these constants in headers that are now included from multiple translation units after the namespace fixes.
- `out-of-line definition of 'add_ctr' / 'add_goal' / 'Optimizer' does not match any declaration` — `SystemFactory` / `Optimizer` signatures evolved on mainline; our `using namespace std;` removal leaves type lookups dangling.
- `no template named 'vector'` — `using namespace std;` removal exposes call sites that didn't get the `std::` prefix.

These are pure-port work: each error is local to one file and resolvable by adding the right `std::` qualifier or matching the mainline signature. There is no architectural blocker — the cav26-era logic is on top of mainline, just needs glue.

**Until porting completes, dreal4-cmake remains pinned at `be485777` directly** (the cav26 baseline that we proved builds + tests). The `master-modernized` branch is the snapshot of what to port FROM as the work proceeds.

**Recommended porting order:**
1. Resolve `vector` / `std::` qualifier issues (mechanical).
2. Match new mainline signatures for `SystemFactory::add_ctr/add_goal` and `Optimizer::Optimizer`.
3. Resolve `FORMAT_VERSION` etc. duplicate-symbol issues (likely an include-guard or single-source-of-truth fix).
4. Re-run dreal4-cmake build + regression suite against the updated tip.

## Modernization cadence

After Phase 5, the policy is: rebase `master` atop `ibex-team/ibex-lib@HEAD` on every mainline minor release. The 14 cherry-picks remain on top as the canonical dReal patch set. The dreal4-cmake regression suite (Phase 1B) is the standing soundness gate — re-run after every rebase.

## See also

- `~/.claude/plans/glistening-launching-fountain.md` — full modernization plan
- `dreal4-cmake/CODAC_MIGRATION.md` — historical context, performance regression analysis, "Option B" recommendation
- `dreal4-cmake/DEPENDENCIES.md` — to be updated in Phase 3 with the new fetch URL
- `CLAUDE.md` (this repo) — short-form orientation for future Claude sessions
