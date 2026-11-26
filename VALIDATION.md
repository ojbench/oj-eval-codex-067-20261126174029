# Implementation and validation

`refcell.hpp` implements the required C++17 interface. A shared control block
tracks the immutable borrow count and exclusive mutable borrow flag. Copying a
Ref adds a reader; moving either handle transfers its borrow without adding one.
Assignment releases the destination's previous borrow. The cell owns its value
directly and reports active borrows at destruction with DestructionError.
All borrow operations take O(1) time; each cell and handle use O(1) storage.

## Local checks

The sample directory mentioned in README.md was absent. Added regression tests
cover conflicts, optional borrows, copy/move construction and assignment,
cross-cell assignment, vector relocation, const cells, non-default-constructible
and move-only values, and destruction errors.

Commands (all passed):

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
g++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -g -I. tests/refcell_test.cpp -o /tmp/refcell_sanitized
/tmp/refcell_sanitized
```

## Judge results

The problem API accepts C++ source, with the submitted header included by judge
drivers as src.hpp. The provided client's missing submit_code method was added.
The CMake executable is the local regression suite; submit refcell.hpp to OJ.

| Attempt | Submission ID | Score | Result |
| --- | --- | --- | --- |
| 1 | 879515 | 20/100 | Judge drivers required the original template's iostream include. |
| 2 | 879516 | 100/100 | Accepted on all 18 test points, including memory checks. |

Accepted source commit: `41e57e30d6e947b9ee58614c185299c72ca6549e`.
Each implementation revision was committed and pushed. Remote branch hashes
were checked with git ls-remote. No files or remote branches were modified while
a submission was being judged.
