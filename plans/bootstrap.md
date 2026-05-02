# Bootstrap the C++20 Doom Project

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log`, and `Outcomes & Retrospective` must be kept up to date as work proceeds.

This plan follows `PLANS.md` at the repository root. It describes Phase 0: creating the smallest real C++20 project shape that can be built, linted, formatted, tested, and run before any Doom-specific runtime systems are added.

## Purpose / Big Picture

After this phase, a developer can clone the repository, run `make check` from the repository root, and see a complete quality gate execute against a tiny runnable program. The observable player-facing behavior is intentionally minimal for this phase: running the built `doom_cpp` executable prints a boot message that proves the application entry point is wired. This bootstraps the contract that later phases depend on: every meaningful change must pass formatting, static analysis, compilation with warnings as errors, and tests.

## Progress

- [x] (2026-05-02 20:45+01:00) Read `AGENTS.md` and `PLANS.md` in full before implementation.
- [x] (2026-05-02 20:49+01:00) Confirmed the repository only contained `AGENTS.md`, `PLANS.md`, and Git metadata before Phase 0 changes.
- [x] (2026-05-02 20:58+01:00) Added the Phase 0 repository layout, CMake project, quality scripts, hello-world executable, and bootstrap integration test.
- [x] (2026-05-02 21:04+01:00) Ran `make check`; it entered the project and failed because `CONFIGURE_DEPENDS` is invalid in CMake script mode.
- [x] (2026-05-02 21:06+01:00) Removed script-mode `CONFIGURE_DEPENDS` usage from the format and tidy source discovery scripts.
- [x] (2026-05-02 21:08+01:00) Reran `make check`; it failed because child CMake script invocations did not receive `SOURCE_DIR`.
- [x] (2026-05-02 21:09+01:00) Passed `SOURCE_DIR` and `BUILD_DIR` explicitly to child CMake scripts.
- [x] (2026-05-02 21:10+01:00) Reran `make check`; clang-format reported style violations in `include/doomcpp/config.hpp` and `tests/bootstrap_test.cpp`.
- [x] (2026-05-02 21:11+01:00) Applied clang-format to the C++ source and header files touched by Phase 0.
- [x] (2026-05-02 21:12+01:00) Reran `make check`; CMake selected the Visual Studio generator, which did not provide the compilation database clang-tidy needed.
- [x] (2026-05-02 21:13+01:00) Forced the check build to use Ninja and disabled clang-tidy's trailing-return-type modernization rule.
- [x] (2026-05-02 21:14+01:00) Made `make check` recreate its own generated `build/check` directory so stale generator state cannot poison reruns.
- [x] (2026-05-02 21:16+01:00) Reran `make check`; clang-tidy reported concise-preprocessor and exception-escape issues in `tests/bootstrap_test.cpp`.
- [x] (2026-05-02 21:17+01:00) Updated the bootstrap test to use `#ifdef _WIN32` and catch exceptions in `main`.
- [x] (2026-05-02 21:18+01:00) Reran `make check`; clang-format, clang-tidy, Ninja build, and CTest all passed with `1/1` tests green.
- [x] (2026-05-02 21:19+01:00) Ran `build/check/doom_cpp.exe` and observed the boot message `doom-cpp bootstrap: engine entry point online`.
- [x] (2026-05-02 21:20+01:00) Reran `make check` after the plan update; it passed with `1/1` tests green.
- [x] (2026-05-02 21:21+01:00) Added `.gitignore` so generated `build/` output is not committed.
- [x] (2026-05-02 21:22+01:00) Reran `make check` after adding `.gitignore`; it passed with `1/1` tests green.
- [ ] Commit Phase 0 with a present-tense milestone message.

## Surprises & Discoveries

- Observation: The Windows host has CMake and MSYS2 GCC on PATH, but does not have `make`, `clang++`, `clang-format`, or `clang-tidy` on PATH.
  Evidence: `cmake --version` reported 4.3.1, `g++` resolved to `C:\msys64\ucrt64\bin\g++.exe`, and PowerShell reported the Clang and Make tools as unrecognized commands.

- Observation: `file(GLOB_RECURSE CONFIGURE_DEPENDS ...)` is not accepted by CMake when the file is executed with `cmake -P`.
  Evidence: `make check` failed with `CONFIGURE_DEPENDS is invalid for script and find package modes` before format verification could run.

- Observation: Variables set in one CMake script-mode process are not inherited by child `cmake -P` processes.
  Evidence: The format script searched an empty source root and failed with `No C++ source files found for clang-format` until `SOURCE_DIR` was passed with `-D`.

- Observation: The repository's LLVM-based `.clang-format` style reflows namespace closing comments and short multiline constants differently than the initial hand-written formatting.
  Evidence: `make check` reported `-Wclang-format-violations` for `include/doomcpp/config.hpp` and `tests/bootstrap_test.cpp`.

- Observation: CMake's default Visual Studio generator on this host does not create `compile_commands.json` for clang-tidy in the expected build directory.
  Evidence: `make check` configured with `Visual Studio 17 2022`, then clang-tidy reported `No compilation database found in C:/Users/giorg/Documents/codex/doom-cpp/build/check or any parent directory`.

- Observation: With warnings treated as errors, clang-tidy also enforces test harness robustness.
  Evidence: `make check` failed until the integration test caught exceptions inside `main` and used concise preprocessor directives.

## Decision Log

- Decision: Use GCC through CMake for the Phase 0 build while still requiring `clang-format` and `clang-tidy` in `make check`.
  Rationale: `AGENTS.md` allows Clang or GCC as the compiler, and the user's requested quality gate explicitly requires clang-format and clang-tidy. The build should use the available standards-compliant compiler, while missing quality tools should fail clearly instead of being skipped.
  Date/Author: 2026-05-02 / Codex

- Decision: Add `plans/bootstrap.md` even though the user's phase list only names ExecPlans beginning at Phase 1.
  Rationale: The engineering contract says to write the ExecPlan in `plans/` before implementing anything. Phase 0 changes the build system and repository structure, so it needs its own living plan.
  Date/Author: 2026-05-02 / Codex

- Decision: Keep quality helper scripts in CMake script mode but avoid configure-only options in those scripts.
  Rationale: Script mode lets `make check` run consistently without generating helper projects, but CMake's `CONFIGURE_DEPENDS` glob option only applies during project configuration. Explicit reruns of `make check` already re-evaluate the globs, so correctness does not depend on configure-time dependency tracking.
  Date/Author: 2026-05-02 / Codex

- Decision: Pass source and build paths into child quality scripts explicitly.
  Rationale: CMake script mode starts a fresh process for each `cmake -P` invocation. Explicit `-D` arguments make each script self-contained and avoid hidden dependency on parent-process variables.
  Date/Author: 2026-05-02 / Codex

- Decision: Use the Ninja CMake generator for `make check`.
  Rationale: Ninja is available after installing the MSYS2 package and produces the compilation database needed by clang-tidy. It also gives a single-config build where `CMAKE_BUILD_TYPE=Debug` is meaningful.
  Date/Author: 2026-05-02 / Codex

- Decision: Disable clang-tidy's `modernize-use-trailing-return-type` rule.
  Rationale: The project coding rules require modern C++20 but do not require trailing return types for ordinary functions. Enforcing that rule would make simple functions like `int main()` less idiomatic without improving correctness or performance.
  Date/Author: 2026-05-02 / Codex

- Decision: Allow `make check` to delete and recreate only `build/check`.
  Rationale: `build/check` is generated output owned by the check pipeline. Recreating it prevents stale generator metadata from a prior failed configure from breaking future runs while avoiding any source or user-authored file deletion.
  Date/Author: 2026-05-02 / Codex

- Decision: Ignore the root `build/` directory in Git.
  Rationale: CMake and Ninja generate compiler outputs, caches, and test binaries there. Those artifacts are reproducible from source with `make check` and should not be versioned.
  Date/Author: 2026-05-02 / Codex

## Outcomes & Retrospective

Phase 0 now provides a real, runnable C++20 project skeleton. The repository has the required top-level layout, a CMake build, a strict `make check` quality gate, a minimal executable, and an integration test that launches the actual built binary. The main lesson from this phase is that the check command must own enough environment determinism to be useful: forcing Ninja and recreating `build/check` made the pipeline reproducible on Windows, while keeping clang-format and clang-tidy as hard requirements preserved the quality contract.

## Context and Orientation

The repository starts as an empty C++ project with only process documents. `AGENTS.md` defines the intended layout: public headers live under `include/`, engine implementation under `engine/`, game-specific implementation under `game/`, tests under `tests/`, bundled runtime assets under `assets/`, and living plans under `plans/`.

For Phase 0, the application has no Doom-specific map, renderer, or input system. The only public interface is `include/doomcpp/config.hpp`, which exposes named constants for the project name and boot message. The only executable source is `engine/main.cpp`, which prints the boot message. The only test source is `tests/bootstrap_test.cpp`, which launches the built executable and verifies that the boot message appears. The test is an integration test because it executes the real binary instead of mocking the entry point.

## Plan of Work

Create `CMakeLists.txt` at the root with CMake 3.22 minimum, C++20 enabled, warnings enabled, and warnings treated as errors for project targets. Define an executable target named `doom_cpp` from `engine/main.cpp`. Define a test executable named `bootstrap_tests` from `tests/bootstrap_test.cpp`, register it with CTest, and pass the built application path into the test through a preprocessor definition.

Create `Makefile` with a `check` target that delegates to scripts in `tools/`. The check command must run formatting verification, CMake configure, clang-tidy, build verification, and CTest. The Makefile exists to preserve the required user command `make check`; it does not replace CMake as the build system.

Create `tools/check-format.cmake`, `tools/check-tidy.cmake`, and `tools/run-check.cmake`. These scripts are written in CMake script mode so they run consistently from GNU Make, MSYS2 Make, or other Make variants. `check-format.cmake` discovers C++ files and runs `clang-format --dry-run --Werror`. `check-tidy.cmake` discovers C++ files and runs `clang-tidy` against the generated compilation database. `run-check.cmake` configures the build directory, invokes the format and tidy scripts, builds, and runs CTest.

Create `.clang-format` and `.clang-tidy` at the repository root so both tools have explicit project policy. The style should be conservative and readable for systems C++.

## Concrete Steps

From `C:\Users\giorg\Documents\codex\doom-cpp`, run:

    make check

The expected successful result is that format verification produces no changes, clang-tidy produces no diagnostics, CMake builds `doom_cpp` and `bootstrap_tests`, and CTest reports one passing test. On Windows, if `make`, `clang-format`, or `clang-tidy` are not installed or not on PATH, the failure is an environment failure. Install the missing MSYS2 packages or add the existing executables to PATH, then rerun the same command.

## Validation and Acceptance

Phase 0 is accepted only when `make check` exits with status 0. The command must prove more than compilation: it must run format verification, static analysis, build verification, and the real integration test. The executable must be runnable and must print:

    doom-cpp bootstrap: engine entry point online

The integration test `bootstrap_tests` must launch the actual `doom_cpp` binary built by CMake and verify that exact line appears in stdout.

## Idempotence and Recovery

The build directory is `build/check`, which is generated output and can be deleted at any time if CMake configuration becomes stale. The scripts create that directory as needed and can be re-run safely. If a quality tool is missing, install the missing tool and rerun `make check`; no source cleanup is required.

## Artifacts and Notes

The initial tool probe on this host found:

    cmake version 4.3.1
    g++.exe at C:\msys64\ucrt64\bin\g++.exe
    clang++ not found on PATH
    clang-format not found on PATH
    clang-tidy not found on PATH
    make not found on PATH

## Interfaces and Dependencies

`include/doomcpp/config.hpp` declares compile-time constants under namespace `doomcpp`. Public functions and constants in this repository carry contract comments describing ownership and preconditions. `engine/main.cpp` uses only this public header and the C++ standard library.

The external tool dependencies for Phase 0 are CMake 3.22 or newer, a C++20 compiler supported by CMake, GNU Make or a compatible `make` command, clang-format, and clang-tidy. No runtime third-party library is introduced in this phase.

Revision note 2026-05-02 / Codex: Created this plan before adding Phase 0 code so the bootstrap work is recoverable from a self-contained ExecPlan.

Revision note 2026-05-02 / Codex: Updated progress, discoveries, and decisions after the first real `make check` failure exposed CMake script-mode incompatibility.

Revision note 2026-05-02 / Codex: Updated the plan after the second `make check` failure exposed missing variable propagation between CMake script-mode processes.

Revision note 2026-05-02 / Codex: Updated progress and discoveries after clang-format produced the first source-level quality failure.

Revision note 2026-05-02 / Codex: Updated the plan after clang-tidy exposed that the default Visual Studio generator did not provide a usable compilation database.

Revision note 2026-05-02 / Codex: Updated the plan after making the generated check build directory disposable and reproducible.

Revision note 2026-05-02 / Codex: Updated progress and discoveries after clang-tidy flagged test harness issues.

Revision note 2026-05-02 / Codex: Recorded the green `make check` result, direct executable smoke test, and Phase 0 retrospective.

Revision note 2026-05-02 / Codex: Recorded the final pre-commit `make check` pass after updating this ExecPlan.

Revision note 2026-05-02 / Codex: Recorded the generated-output ignore rule before staging Phase 0.

Revision note 2026-05-02 / Codex: Recorded the green `make check` result after adding `.gitignore`.
