# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repo is

This is the dReal team's fork of [ibex-team/ibex-lib](https://github.com/ibex-team/ibex-lib) (Ibex 2.9.1) — a C++ interval-arithmetic / constraint-programming library. The fork exists **only** to host eight surgical patches that are pending upstream submission.

Remotes:
- `origin` → upstream `ibex-team/ibex-lib` (the canonical Ibex repo and rebase target)
- `fork` → `ncsys-lab/ibex-lib` (this fork's hosting; push target)

**Read [MIGRATION.md](MIGRATION.md) before making non-trivial changes.** It catalogs the 8 patches (what / why / file scope) and their upstream-PR status.

## Branches

- `master` — tracks `origin/master`. Do not commit local changes here.
- `dreal-perf-patches` — the 8-patch series (12 files / +169/−21 vs mainline, excluding docs). This is the branch `dreal4-cmake` should depend on.
- `archive/cav26-base`, `archive/pre-modernization-filib`, `archive/v2.8.9-m1-superseded`, `archive/v2.7.4-m1-superseded` — historical reference, not maintained.

## Build & test

```bash
mkdir build && cd build
cmake -DINTERVAL_LIB=gaol -DLP_LIB=none ..      # gaol default; soplex/clp/cplex available
make -j
make check                                       # 62 cppunit tests, requires brew install cppunit
```

Key cmake options (see `doc/install-cmake.rst` for the full list):

| Option | Notes |
|---|---|
| `-DINTERVAL_LIB=` | `gaol` (default, works on arm64 since mainline `971f8eb0`), `filib` (blocked on arm64-native by `f4b98ccf`), `bias`, `direct` (non-rigorous) |
| `-DLP_LIB=` | `none` (default for dReal's use), `soplex`, `clp`, `cplex` |
| `-DBUILD_SHARED_LIBS=ON` | required for the Java interface |
| `-DBUILD_TESTING=0` | skip the test tree |
| `-DCMAKE_BUILD_TYPE=Debug` | drops `-O0 -g -pg`, enables internal asserts |

Running a single test after `make check`: `./tests/TestFoo` or `ctest -R TestFoo --output-on-failure`.

## Rebase cadence

Re-apply the 7 commits onto `ibex-team/ibex-lib@HEAD` on every mainline minor release:

```bash
git fetch origin
git checkout dreal-perf-patches
git rebase origin/master
make -j && make check                            # verification gate
```

If any of the 8 patches lands upstream, drop the corresponding commit from the rebase.

## Architecture

`libibex` is a single library built from `src/`, organized by responsibility (low-level → user-facing):

| Layer | Subdir | Purpose |
|---|---|---|
| Pluggable backends | `interval_lib_wrapper/`, `lp_lib_wrapper/` | One interval lib + one LP lib selected per build |
| Arithmetic | `src/arithmetic` | `Interval`, `IntervalVector`, `IntervalMatrix` |
| Symbolic | `src/symbolic`, `src/operators` | Expression DAG, simplification, automatic differentiation, Minibex pretty-printing |
| Function | `src/function` | `Function` compiles a symbolic expression into a `CompiledFunction`. Eval / Gradient / HC4Revise / InHC4Revise. **6 of 7 fork patches touch this directory or `src/arithmetic/ibex_TemplateDomain.h`.** |
| Numerics | `src/numeric` | Interval Newton, LP solver wrapper, linearizers |
| Contractors | `src/contractor` | `Ctc` + combinators (`CtcCompo`, `CtcUnion`, `CtcFixPoint`), HC4, Acid, 3B, Newton, polytope hull, quantifier handling |
| Search support | `src/bisector`, `src/cell`, `src/combinatorial` | Bisection strategies, search-node containers |
| Set characterization | `src/set`, `src/predicate` | Inner/outer set approximations |
| Optimization | `src/loup` | Loup finders |
| Top-level engines | `src/solver`, `src/optim` | `Solver` / `Optimizer` and their `Default*` strategies |
| Frontend | `src/parser` | Flex + Bison Minibex parser. **`parser.yc` patch lives here.** Generated files land in the build dir; lexer/parser changes require a reconfigure. |
| Binaries | `src/bin` | `ibexsolve.cpp`, `ibexopt.cpp` |
| Java | `src/java` | JNI bindings, built only with `-DBUILD_JAVA_INTERFACE=ON` |

`src/CMakeLists.txt` generates an umbrella `ibex.h` at configure time by globbing public headers — new headers are picked up on re-configure.

## Upstream reference docs (`doc/`)

`doc/` contains the upstream Sphinx documentation (User Guide + Programmer Guide). Useful when you need conceptual background or API semantics beyond what the headers convey. Index in `doc/index.rst`. Most relevant for fork work:

| File | What it covers |
|---|---|
| `function.rst` | `Function` semantics: eval, gradient, HC4Revise, InHC4Revise — the subsystem 6/8 patches touch |
| `contractor.rst` | `Ctc` interface, HC4, Newton, combinators (compo/union/fixpoint) |
| `minibex.rst` | Minibex grammar — relevant to the `parser.yc` patch |
| `interval.rst` | `Interval` / `IntervalVector` arithmetic |
| `solver.rst`, `optim.rst` | Top-level `Solver` / `Optimizer` engines and CLI |
| `install-cmake.rst` | Full CMake option reference (this CLAUDE.md only lists the dReal-relevant subset) |

These docs describe **upstream** Ibex behavior — for patch-specific semantics, MIGRATION.md is authoritative.

## Things worth knowing

- C++ standard is **C++11** (`CMAKE_CXX_STANDARD 11`). Don't introduce C++14+ features without bumping that and verifying CI.
- Mainline `971f8eb0` (March 2025) added arm64-Darwin support to the gaol wrapper. That makes `gaol` the practical default on Apple Silicon (filib is blocked on arm64-native by `f4b98ccf`).
- `benchs/` is present but commented out in the top-level CMakeLists. Not part of any default target.
- The `waf` flow in `.travis.yml` is upstream legacy; use CMake.

## Verification discipline

When reporting "tests pass" or "build green," confirm which artifact (branch, build dir) the verification actually ran against. The trap: ibex's `make check` is run from `build-perf-verify/` configured against a previous branch state, while the current changes are on `dreal-perf-patches` HEAD that the build dir hasn't picked up (especially after a `git rebase` or branch switch).

Always re-run from the right build dir against the right HEAD, and cite both in the report. If in doubt, `rm -rf <build dir>` and reconfigure.

## Fork-diff minimization

This repo is a fork of `ibex-team/ibex-lib`. Every patch we carry has to be re-rebased on every upstream release — the fork is a maintenance liability, not an asset. Keep the diff small and PR-able.

Before adding a commit to the fork, evaluate it against:

> "Would the upstream maintainers accept this as a standalone PR?"

If no, find the minimal subset that achieves the goal. Sweeping refactors (whole-codebase `std::` qualifications, tarball replacements, broad style cleanups) don't belong here.

Before cherry-picking from any downstream history (`archive/cav26-base` etc.), check whether mainline `ibex-team/ibex-lib` has already addressed the underlying issue. If yes, drop the cherry-pick.
