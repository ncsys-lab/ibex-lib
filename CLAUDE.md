# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository identity

This is the **dReal team's canonical fork** of [ibex-team/ibex-lib](https://github.com/ibex-team/ibex-lib) (Ibex 2.9.1), tracked under three remotes:

- `origin` → upstream `ibex-team/ibex-lib` (the canonical Ibex repo; rebase target)
- `fork` → `ncsys-lab/ibex-lib` (this fork's hosting; push target)
- `soonho-upstream` → `dreal-deps/ibex-lib` (Soonho's old downstream — abandoned 2016, not authoritative)

Ibex itself is a C++ interval-arithmetic / constraint-programming library — symbolic functions, contractors, a system solver (`ibexsolve`), and a global optimizer (`ibexopt`). This fork is consumed by dReal4 via `FetchContent` (see `dreal4-cmake/CMakeLists.txt`).

**The authoritative divergence catalog lives in [MIGRATION.md](MIGRATION.md)** — read it before making non-trivial changes to the fork. It explains which 14 commits make up the dReal patch set, which tarball-time patches are applied to gaol/filib/soplex, the default-`INTERVAL_LIB` policy, the branch archaeology, and the soundness-investigation findings on gaol rounding.

## Branch layout

- `master` — tracks `origin/master` (= `ibex-team/ibex-lib@HEAD`). Do not commit local changes here.
- `master-modernized` — the rebase target: 14 dReal patches applied on top of `master`. WIP — see MIGRATION.md "Status of master-modernized" for the per-file porting work that remains before this builds cleanly.
- `archive/cav26-base` — the cav26-era `be485777`. Currently consumed directly by dReal4 until `master-modernized` ports cleanly.
- `archive/pre-modernization-filib` — `v2.8.9-upgrade-filib-rosetta` (filib-default variant of cav26-base).
- `archive/v2.8.9-m1-superseded`, `archive/v2.7.4-m1-superseded` — older platform-port branches; superseded.
- `fork/fmcad25-tools-bench-CONTROL` (on the `fork` remote) — paper-artifact benchmark snapshot; preserve.

## Modernization cadence

Rebase `master-modernized` atop `ibex-team/ibex-lib@HEAD` on every mainline minor release. Re-run the dReal-side regression suite at `dreal4-cmake/test/dreal/contractor/test/ibex_*_test.cc` (28 tests; the standing gate) after each rebase. New mainline acceptance of any of our patches → drop the corresponding commit from the rebase.

### Fork-specific delta (read before changing the build)

- **Default `INTERVAL_LIB` is `filib`, not `gaol`** (see `interval_lib_wrapper/CMakeLists.txt`). The motivation is filib's SSE-based native rounding under Rosetta (x86_64 on Apple Silicon). Upstream defaults to gaol.
- The filib subproject **applies `filibsrc-3.0.2.2.all.all.patch`** after extracting the source tarball (see `interval_lib_wrapper/filib/CMakeLists.txt`). This fixes operator-precedence warnings that would become errors under `-Werror`. The upstream had this as a TODO.

When syncing from upstream, preserve both changes.

## Build, test, install

This is a CMake project (>=3.5.1, C++11). The `waf`-based flow mentioned in `.travis.yml` is upstream's legacy path; **use CMake**.

```bash
mkdir build && cd build
cmake -DLP_LIB=soplex ..      # filib is now the default interval lib
make -j
make check                     # runs ctest; only present if cppunit was found at configure time
sudo make install              # default prefix /usr/local; override with -DCMAKE_INSTALL_PREFIX
```

Key cmake options (see `doc/install-cmake.rst` for the full list):

| Option | Notes |
|---|---|
| `-DINTERVAL_LIB=` | `filib` (default in this fork), `gaol`, `bias`, or `direct`. `direct` is non-rigorous. |
| `-DLP_LIB=` | `soplex` (recommended), `clp` (experimental), `cplex`, or `none` (default). |
| `-DBUILD_SHARED_LIBS=ON` | Build shared libs (also required for the Java interface). |
| `-DBUILD_TESTING=0` | Skip the test tree entirely. |
| `-DBUILD_JAVA_INTERFACE=ON` | Requires `BUILD_SHARED_LIBS=ON` and `JAVA_HOME` set. |
| `-DCMAKE_BUILD_TYPE=Debug` | Drops `-O0 -g -pg` and enables internal asserts. |

There are pre-existing CLion build dirs (`cmake-build-debug-rosetta-default/`, `cmake-build-release-rosetta-default/`) — these are the IDE's, not authoritative; create your own `build/` rather than reusing them unless you know they're current.

### Running a single test

`make check` compiles every test into its own executable named after the test class (see `tests/CMakeLists.txt`). After `make check`, run an individual test directly:

```bash
./tests/TestInterval                 # run all cases in one suite
ctest -R TestInterval --output-on-failure   # same thing via ctest
```

Adding a new test: add `TestFoo` to the `TESTS_LIST` in `tests/CMakeLists.txt` and provide `tests/TestFoo.{cpp,h}` following the cppunit pattern of existing tests. Tests require **cppunit**; if it's missing at configure time, `make check` becomes a no-op with a warning.

## Architecture

`libibex` is a single library built from `src/`, organized by responsibility. The layering, from low-level to user-facing, is:

| Layer | Subdir(s) | Purpose |
|---|---|---|
| Pluggable backends | `interval_lib_wrapper/`, `lp_lib_wrapper/` | One backend selected per build via `INTERVAL_LIB` / `LP_LIB`. Each wrapper is its own subdir (`gaol/`, `filib/`, `soplex/`, ...). |
| Arithmetic | `src/arithmetic` | `Interval`, `IntervalVector`, `IntervalMatrix`, inner arithmetic. The narrow waist over the chosen interval backend. |
| Symbolic | `src/symbolic`, `src/operators` | Expression DAG, simplification (`ExprSimplify`, `ExprSimplify2`), polynomial form, automatic differentiation (`ExprDiff`), Minibex pretty-printing. |
| Function | `src/function` | `Function` compiles a symbolic expression into a `CompiledFunction`. Forward/backward evaluation (`Eval`, `Gradient`, `HC4Revise`, `InHC4Revise`). `NumConstraint` ties an expression to a comparison op. |
| Numerics | `src/numeric` | Interval Newton, LP solver wrapper (`LPSolver`), linearizers (Compo, Duality, X-Taylor, Fixed). |
| Contractors | `src/contractor` | Contractor programming — `Ctc` and combinators (`CtcCompo`, `CtcUnion`, `CtcFixPoint`), constraint propagators (`CtcHC4`, `CtcAcid`, `Ctc3BCid`), Newton, polytope hull, quantifier handling (`CtcExist`, `CtcForAll`). |
| Search support | `src/bisector`, `src/cell`, `src/combinatorial` | Bisection strategies (`LargestFirst`, `RoundRobin`, `SmearFunction`, `LSmear`) and search-node containers (heaps, stacks, beam search). |
| Set characterization | `src/set`, `src/predicate` | Inner/outer set approximations and boolean predicates over boxes. |
| Optimization helpers | `src/loup` | "Loup" (loup = upper-bound) finders — `LoupFinderXTaylor`, `LoupFinderInHC4`, etc. |
| Top-level engines | `src/solver`, `src/optim` | `Solver` / `DefaultSolver` (system solving) and `Optimizer` / `DefaultOptimizer` (global optim). These compose contractors + bisectors + loup finders into a default strategy. |
| Strategy plumbing | `src/strategy`, `src/system` | Box properties (`Bxp*`), box events, system representations (`System`, `KuhnTuckerSystem`, normalized forms). |
| Frontend | `src/parser` | Flex (`lexer.l`) + Bison (`parser.yc`) for the Minibex DSL. Generated into the build dir, so any lexer/parser change requires a reconfigure. Parser-side AST nodes are `ibex_P_*`. |
| Binaries | `src/bin` | `ibexsolve.cpp`, `ibexopt.cpp`, plus shared `parse_args.h`. |
| Java | `src/java` | JNI bindings, built only with `BUILD_JAVA_INTERFACE=ON`. |
| Misc | `src/tools` | Utilities. |

### Key compose pattern

User code typically: builds a `Function` symbolically (or parses a Minibex file via `System`) → wraps it in a `NumConstraint` → builds `Ctc` contractors over the constraint → composes them with `CtcCompo` / `CtcUnion` / `CtcFixPoint` → drops them into a `Solver` (or `Optimizer`) along with a `Bsc` bisector and a `CellBuffer`. `DefaultSolver` and `DefaultOptimizer` do this composition for the common case.

### Header generation

`src/CMakeLists.txt` generates a single umbrella `ibex.h` at configure time by globbing every `*.h` / `*.hpp` in the build's source set and `#include`-ing them. New public headers are picked up automatically once a re-configure happens.

### Parser regeneration

Edits to `src/parser/lexer.l` or `src/parser/parser.yc` are processed by Flex and Bison during configure (`flex_target` / `bison_target` in `src/CMakeLists.txt`). The generated `.cc` / `.hh` land in `${CMAKE_CURRENT_BINARY_DIR}/parser/`. Both tools are required at configure time — the configure step `FATAL_ERROR`s if either is missing.

## Examples

`examples/` contains the upstream tutorial programs (`doc-*.cpp`, `lab*.cpp`, `dynibex/`, etc.) plus a `makefile` that builds individual examples against an installed Ibex via `pkg-config`. To use the makefile, `PKG_CONFIG_PATH` must point at the installed `share/pkgconfig` (`ibex.pc`). Useful as scratch space when reproducing user reports.

## Things worth knowing

- C++ standard is **C++11** (`CMAKE_CXX_STANDARD 11` in `cmake.utils/ibex-dev-utils.cmake`). Don't introduce C++14+ features without bumping that, and verify it still compiles on the configurations CI cares about.
- `filib` on macOS/arm64 is **disabled upstream** (it gives unreliable results — see `doc/install-cmake.rst`). This fork's filib default assumes the build is happening under Rosetta (x86_64), not native arm64. If you're configuring on Apple Silicon, run the build under Rosetta or override with `-DINTERVAL_LIB=gaol`.
- `benchs/` is present in the tree but commented out in `CMakeLists.txt` (`# TODO benchs`). It is not part of any default target.
- The `waf`-based commands in `.travis.yml` are upstream legacy; do not use them. CI exists but is not the source of truth for this fork.
