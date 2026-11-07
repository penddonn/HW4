# Code Style and Repository Rules

Your code must follow the Recitation 0 clang-format configuration, just as the starter and teacher code do.

## Naming

Use descriptive names that explain what each value or function does. Short coordinate names (`x`, `y`) and loop indices (`i`) are fine. Keep system and third-party API names unchanged.

| Kind | Rule | Examples |
| --- | --- | --- |
| Struct/enum types and type aliases | `UpperCamelCase` | `SnakeNode`, `Game`, `StepResult` |
| Struct members | `m_` plus lowercase words separated by underscores | `m_head`, `m_next`, `m_gap_y` |
| Functions | `lowerCamelCase` | `snakeInit`, `advancePipes`, `scorePassedPipes` |
| Local/file variables and parameters | `lowerCamelCase` | `headCol`, `game`, `nextPos` |
| Constants and macros | `UPPER_SNAKE_CASE` | `PIPE_WIDTH`, `RANDOM_MODULUS`, `MIN_BOARD_SIZE` |

```c
typedef struct SnakeNode {
    Coord m_pos;
    struct SnakeNode *m_next;
} SnakeNode;

int scorePassedPipes(Game *game);
```

## No magic numbers

Use meaningful constants or enums for speeds, gravity, sizes, tolerances, intervals, capacities, and protocol states. Check `snake.h` and `flappy.h` first; do not repeat a parameter as a raw number or give it a meaningless name like `NUMBER_8`.

In P2, use `BIRD_WIDTH`, `BIRD_HEIGHT`, `PIPE_WIDTH`, `PIPE_GAP_HEIGHT`, `PIPE_COUNT`, and `PIPE_SPEED`. Write pipe movement as `PIPE_SPEED * dt` instead of repeating a numeric speed. Two numbers that happen to have the same value for different reasons should not share one constant.

You may write `0`, `1`, or `0.5` (or `/ 2`) directly when they express a mathematical relationship: zero initialization, comparing with zero, adding or subtracting one, or taking a half. Keep colors, pixel art, file-format data, and test inputs grouped together with clear meanings or units rather than scattering game parameters through your logic.

## Structure and memory ownership

- Write standard C17 code with 4-space indentation and a 100-column line limit.
- Put braces around `if`, `else`, `for`, and `while` bodies, and write one statement per line.
- Leave a blank line between functions. Split long conditions into named boolean variables when that makes them easier to read.
- Use `static` helper functions for distinct subtasks, but do not split every single line into its own function.
- Write comments that explain ring-link rules, boundary cases, units, and failure handling. Remove or update finished `TODO` comments.
- Keep the provided types, struct members, function names, and signatures unchanged. Do not modify the course framework to work around a requirement.

### Memory ownership in Problem 1

- Release every allocation you own exactly once. Do not access freed memory.
- Follow the initialization, failure rollback, and destruction contracts in [P1](p1.md).

## Required online format gate

The online grader checks your source files with **clang-format 18** using the teacher's Recitation 0 configuration. It checks each problem independently, including any C helper `.c` and `.h` files you add. The grader ignores any `.clang-format` changes or overrides in your repository.

> **Important:** A formatting failure acts like a compilation failure for that problem: **that problem receives zero automatic points**, while the other problem is graded independently. The grader shows the offending file and diff, does not modify your code for you, and skips that problem's functional tests until you fix the formatting and resubmit. Do not change `.clang-format` or use `clang-format off/on` comments to bypass the check.

The supplied `.clang-format` in `hw4/` is:

```yaml
Language:        Cpp
BasedOnStyle:    LLVM
IndentWidth:     4
AccessModifierOffset: -4
ColumnLimit:     100
```

`Language: Cpp` is intentional and applies to both C and C++ files in clang-format. Do not change it to `C`. Both `p1/` and `p2/` share this root configuration.

To format your files from the command line, run from `hw4/`:

```bash
clang-format-18 -i p1/snake.c p2/flappy.c
```

Remember to include any extra student `.c` and `.h` files you created.

> **Tip:** Using VS Code is optional. If you use VS Code, open `hw4/` as the workspace folder and install the Microsoft C/C++ extension. The supplied `.vscode/settings.json` enables format-on-save automatically, or you can run **Format Document** manually. If your editor's built-in formatter disagrees with the online grader, run `clang-format-18` directly.

Passing the format gate is a prerequisite, not a separate point item. Teaching staff still review naming, constants, code organization, and resource ownership during manual grading.

## Compile and check with sanitizers

Build with the provided Makefiles and fix all compiler warnings. Then build the sanitizer-checked executables from `hw4/` and test both games:

The online grader adds `-Werror`, so compiler warnings become build errors there. Fix every warning locally before submitting.

```bash
make debug
(cd p1 && ./build/snake-debug)
(cd p2 && ./build/flappy-debug)
```

Design your own boundary tests and check memory ownership as well as gameplay. See [Debugging](debugging.md) for sanitizer commands, reports, and platform limits.

## Repository contents and extensions

You are welcome to add helper source files, headers, notes, normal assets, and creative game variants. Adding extra files or directories does not lose points on its own; large structural changes or edits to course framework files simply trigger a staff review notice.

You must keep the required file paths and public interfaces intact and stay compatible with the teacher's C17 framework. Missing required source files, incompatible signatures, or compilation errors give the affected problem zero automatic points. Extra features must not break the required core behavior.

### Where to place extra files

- **Graded helpers:** Put helper `.c` and `.h` files used by your solution inside `p1/` or `p2/` (or a subdirectory such as `p1/helpers/`). The grader format-checks and compiles added C logic files with your solution. Remember to add them to your local `Makefile` and verify that the full game still builds.
- **Standalone drivers and extensions:** Put independent test programs or extra entry points (files with their own `main`) in `p1/extras/`, `p1/extensions/`, `p2/extras/`, or `p2/extensions/`. The grader does not link those directories with the teacher's test runner, but any student C code inside them must still pass `clang-format-18`. The provided terminal UI and physics files are not treated as added student logic.

> **Important:** A custom driver with its own `main` must go in the problem's `extras/` or `extensions/` directory, not directly in `p1/` or `p2/`. Otherwise the grader links it with its own test entry point, causing a link failure and zero automatic points for that problem.

### Prohibited files

> **Important:** Submitting a nonempty `build/` directory at any depth, compiled artifacts, or any prohibited generated/cache file listed below gives **zero for the entire automatic 80 points**. Fix the repository and resubmit. An empty `build/` directory is allowed, but a `build/` containing even `.gitkeep` counts as nonempty.

Do not commit or submit any of the following:

- **Compiled artifacts and binaries:** object files, static/dynamic libraries, or executables (including executables with no file extension, such as `a.out`), and any file matching `*.o`, `*.obj`, `*.a`, `*.so`, `*.dylib`, `*.dll`, `*.exe`, `*.class`, `*.pyc`, or `*.pyo`.
- **Caches, build trees, and tool directories:** `__pycache__/`, `.pytest_cache/`, `.mypy_cache/`, `.ruff_cache/`, `.cache/`, `.idea/`, `node_modules/`, `.venv/`, `venv/`, `ipch/`, `CMakeFiles/`, `cmake-build-*/`, `CMakeCache.txt`, `cmake_install.cmake`, and `compile_commands.json`.
- **Editor, OS, and local temporary files:** `*.swp`, `*.swo`, `*.swx`, `.snake-ui.conf`, `.DS_Store`, `__MACOSX/`, `._*`, `Thumbs.db`, `desktop.ini`, `a.out`, and `*~`.
- **Downloaded archives:** `HW4_student_starter.zip` and `HW4_gradescope.zip`. Also keep the demo download `HW4_demos.zip` and its extracted `hw4-demo/` folder out of your repository: the programs inside are compiled executables.

The supplied `.vscode/settings.json`, `.clang-format`, and PNG/SVG figures are allowed, as are normal new assets such as images. Binary asset formats such as images are not treated as compiled executables just because they are binary files.

The two zero-score gates have different effects:

- **Formatting failure** zeroes only the affected problem (P1 or P2).
- **Prohibited repository contents** zero the entire automatic 80 points across both problems.

Manual grading items are explained in [Grading](grading.md).

## Before you submit

`.gitignore` ignores common generated files, but it does **not** untrack files that you already committed with `git add`. An untracked local build on your disk is not sent to GitHub, whereas a tracked generated file stays in your submission even if it matches `.gitignore`.

Run these commands from `hw4/` before every submission:

```bash
make clean
clang-format-18 -i p1/snake.c p2/flappy.c
git status --short
git ls-files
```

- Confirm `clang-format-18` has formatted `p1/snake.c`, `p2/flappy.c`, and any extra student `.c` or `.h` files.
- Check `git status --short` and `git ls-files` to make sure Git is not tracking any nonempty `build/` directory, compiled binary, prohibited cache, or downloaded archive listed above. Keep local debug logs outside the repository too.
