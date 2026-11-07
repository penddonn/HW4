# Grading

Homework 4 is worth **100 points**: **40 automatic points for Problem 1**, **40 automatic points for Problem 2**, and a separate **20 points for the staff check and code organization**. There are no separate objective questions. Submit your GitHub repository to [HW4_programming](https://www.gradescope.com/courses/1402185/assignments/8980667).

- **Automatic tests:** 80 points total (40 in P1, 40 in P2).
- **Manual assessment (staff check and code organization):** a separate 20-point block.

## Problem 1: Circular-list Snake (40 automatic points)

| Item | Points | What is checked |
| --- | ---: | --- |
| Initialization | 6 | Positions, direction, length, valid ring, partial-allocation cleanup |
| Next position | 4 | Four directions, reverse/invalid requests, no mutation |
| Cell query | 4 | Empty/one-node/full ring, occupied and absent cells, traversal ends |
| Movement and growth | 16 | Tail reuse, one-node growth, wall/body collisions, tail-cell rules, failure rollback |
| Destroy | 6 | Full release, empty and repeated cleanup, no double free or use after free |
| Random state | 4 | Default state/consecutive calls (2), reseeding/range/full period (2) |

## Problem 2: Flappy Bird (40 automatic points)

| Item | Points | What is checked |
| --- | ---: | --- |
| Advance pipes | 10 | Movement/zero time (3), recycling after all movement (3.5), several slots/exact off-screen edge (3.5) |
| Collision | 14 | Top/floor bounds (4), both pipe rectangles (5), safe gaps and touching edges (5) |
| Score new passes | 12 | One pass/no double count (3), multiple passes/strict edge (4.5), reused slot scores again (4.5) |
| Random state | 4 | Default state/consecutive calls (1.5), reseeding/large seeds (1.5), range/full period (1) |

## Gates and memory checks

> **Important:** Two online gates run before functional grading:
>
> - **Per-problem format gate:** Each problem must pass the teacher's **clang-format 18** check before its functional tests run. A formatting failure gives **that problem** 0 automatic points and prints a file and diff report, while the other problem is graded independently. Fix the formatting and resubmit. Grading-environment errors are reported separately from student formatting errors.
> - **Repository-wide cleanliness gate:** A nonempty `build/` directory (at any depth; even `.gitkeep` alone counts as nonempty), compiled artifacts, or explicitly prohibited generated/cache files give **0 for the entire automatic 80 points**. An empty `build/` directory is fine.

Adding extra source files, normal assets, or creative game variants does not lose points for file count or directory layout alone; large structural changes simply trigger a staff review notice. You must still preserve all required file paths and public interfaces. See [Code style and repository rules](code-style.md) for the full list of prohibited files.

The online grader compiles with `-Werror`: compiler warnings are build errors and give the affected problem zero automatic points. Fix all warnings locally; the starter Makefiles do not enable `-Werror`.

Online functional tests run with **ASan** and **UBSan** enabled. Memory errors and undefined behavior fail the functional test items where they occur; the same bug is not deducted a second time under a separate sanitizer category. Build with `make debug` and test boundary states locally. Memory leak checks also matter; see [Debugging](debugging.md).

## Staff check and code organization (20 points)

| Manual item | Points |
| --- | ---: |
| P1 staff check | 6 |
| P1 code organization | 4 |
| P2 staff check | 6 |
| P2 code organization | 4 |

Teaching staff review the last version you submitted. Manual items stay pending on Gradescope until staff complete their review.

- **P1 staff check (6 points):** An in-person check on your last submission where you must explain and modify your own code.
- **P2 staff check (6 points):** An in-person check on your last submission where you must explain and modify your own code.
- **Code organization (4 points per problem):** Staff review your naming, named constants, comments, helper functions, function responsibilities, and resource ownership. Because clang-format is already enforced by the online gate, formatting is not deducted again here.

## General notes

- **Core data structures and responsibilities:** P1 must use a genuine circular linked list with valid node connections. P2 is graded on `advancePipes`, `birdHitsObstacle`, `scorePassedPipes`, and `nextRandom`; the provided framework handles input and bird physics.
- **Independent random tests:** The online `nextRandom` tests run separately for P1 and P2.
- **No unlimited error stacking:** Memory errors count under the functional test items they affect; deductions are not stacked without limit for the same underlying issue.
- **Local environment choices:** Terminal, editor, font, and color differences do not lose points. You do not need any specific workspace file. Always test against the provided framework, because online grading calls the required logic functions directly.
- **Extensions:** Extra features and visual decoration are welcome, provided they do not replace the required public interfaces or break core behavior when linked with the teacher's framework.
