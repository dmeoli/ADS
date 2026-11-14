# ADS — A User Manual

**Donato Meoli**
*Dipartimento di Informatica, Università di Pisa*

*Version 0.1 — for ADS 0.1.0 (November 2026)*

---

## Abstract

ADS is a header-only C++20 library of algorithms and data structures. It
contains the structures of the textbooks (sequences, stacks, queues,
dictionaries, search trees, priority queues, sets, graphs), each one in the
form in which the textbook presents it, and beside them the engineered
variants of the literature on algorithm engineering (cache-conscious,
succinct, compressed, external-memory, concurrent and parallel), written to
be compared with the best implementations available. Each structure is
specified by the abstract data type it realizes, as a C++20 concept, and
checks its preconditions, postconditions and invariants through contracts
whose cost is chosen at compile time.

This manual describes the library area by area. For each structure it
gives the definitions and the invariants, figures of its state in memory
before and after the operations, the pseudocode of the operations (the
same that the comments of the headers carry), the analysis of their costs
with its proofs, the code of the operations that matter, and exercises,
each one solved by a test of the library. It is a companion to the API
reference generated from the headers, not a replacement for it.

## License

ADS is distributed under the
[GNU Lesser General Public License v3.0](https://opensource.org/licenses/lgpl-3.0.html)
(LGPL-3); this manual is distributed under the same terms.

---

## Conventions

* **Class and function names** are typeset in `monospace`, e.g.,
  `ArrayStack`, `push(v)`; the names of the operations of an abstract
  data type are written as in the pseudocode, e.g., $\mathrm{push}( v )$.
* **File-path and source-tree references** are typeset in `monospace`
  too, e.g., `ADS/stack/ArrayStack.h`, relative to `include/`.
* **Abstract data types** are written in capitals, e.g., STACK,
  DICTIONARY, as in the textbooks; the C++20 concept that formalizes each
  of them is in `ADS/ADT.h`.
* **Pseudocode** uses the conventions of the comments of the headers,
  from which it is extracted: numbered lines, $\gets$ for the assignment,
  **bold** keywords, $\triangleright$ for a comment.
* **Costs** are in the worst case unless they are said to be amortized or
  expected; $n$ is the number of items of the structure at hand.
* **Exercises** end each chapter; each one names the test of the library
  that solves it, which can be run with `ctest -R <name>`.
* **Status admonitions** flag the maturity of a feature explicitly.
  Four forms appear in the text:
    * **Status — mature.** The feature is stable, tested, in `main` and
      in tagged releases. Implicit unless stated.
    * **Status — beta.** The feature is in `main` but its interface or
      semantics may change in incompatible ways.
    * **Status — under development.** The feature is partly present but
      is actively being reworked; the manual describes both what exists
      today and the direction of the intended revision.
    * **Status — planned.** The feature is documented as intended but is
      not yet released. The manual indicates explicitly that it cannot be
      used today.

When in doubt about a claim made in this manual, the authoritative
sources are, in order of decreasing specificity: the headers of the
library; the API reference generated from them; this manual.

---

## Table of contents (high-level)

* **Part I — Foundations.**
    * [Chapter 1](chapters/01-introduction.md#ch-1). Introduction.
    * [Chapter 2](chapters/02-sequence.md#ch-2). Sequences.
    * [Chapter 3](chapters/03-stack.md#ch-3). Stacks.
    * [Chapter 4](chapters/04-queue.md#ch-4). Queues and deques.
    * [Chapter 5](chapters/05-memory.md#ch-5). Memory management.
* **Part II — Searching and ordering.**
    * [Chapter 6](chapters/06-search.md#ch-6). Searching.
    * [Chapter 7](chapters/07-dictionary.md#ch-7). Hashing and dictionaries.
    * [Chapter 8](chapters/08-tree.md#ch-8). Trees.
    * [Chapter 9](chapters/09-priority.md#ch-9). Priority queues.
    * [Chapter 10](chapters/10-set.md#ch-10). Sets and filters.
    * [Chapter 11](chapters/11-mfset.md#ch-11). Disjoint sets.
    * [Chapter 12](chapters/12-sort.md#ch-12). Sorting.
* **Part III — Algorithms.**
    * [Chapter 13](chapters/13-algorithm.md#ch-13). Algorithm design.
    * [Chapter 14](chapters/14-graph.md#ch-14). Graphs.
    * [Chapter 15](chapters/15-string.md#ch-15). Strings.
    * [Chapter 16](chapters/16-geometry.md#ch-16). Geometry.
    * [Chapter 17](chapters/17-numeric.md#ch-17). Numerical algorithms.
    * [Chapter 18](chapters/18-combinatorial.md#ch-18). Combinatorial generation.
    * [Chapter 19](chapters/19-backtrack.md#ch-19). Backtracking.
    * [Chapter 20](chapters/20-complexity.md#ch-20). Complexity and intractable problems.
* **Part IV — Engineering.**
    * [Chapter 21](chapters/21-compression.md#ch-21). Compression.
    * [Chapter 22](chapters/22-succinct.md#ch-22). Succinct structures.
    * [Chapter 23](chapters/23-external.md#ch-23). External memory.
    * [Chapter 24](chapters/24-concurrent.md#ch-24). Concurrency.
    * [Chapter 25](chapters/25-parallel.md#ch-25). Parallelism.
    * [Chapter 26](chapters/26-distributed.md#ch-26). Distributed computing.
* **Appendices.**
    * [Appendix A](chapters/A-writing-structure.md#app-A). Writing a new structure.
    * [Appendix B](chapters/B-benchmarks.md#app-B). The benchmarks.

Glossary and bibliography follow.
