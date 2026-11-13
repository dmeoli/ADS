# ADS — A User Manual: the plan

One chapter for each area of `include/ADS`, in the order in which a
course on data structures and algorithms meets them; each chapter is
written from the headers of its area, its theory page in `docs/theory`
and the tests that solve its exercises. A chapter covers, for each
structure: the abstract data type and its axioms, the representation and
its invariants, figures of the state in memory before and after the
operations, the pseudocode of the operations (extracted from the
headers), the analysis of the costs as lemmas with proofs, the code of the
core operations (extracted from the `//! [name]` markers), the engineered
variants and what they change, the benchmark against the best
implementations available, and exercises tied to the tests.

* **Part I — Foundations.**
    1. Introduction: abstract data types as concepts, contracts, the model
       of computation, asymptotic, amortized and expected analysis
       (`ADT.h`, `Contract.h`, `Iterators.h`, `Portable.h`).
    2. Sequences: arrays, linked lists, the array-based lists of Morin,
       tiered and rootish vectors, space-efficient lists (`sequence`).
    3. Stacks (`stack`).
    4. Queues and deques (`queue`).
    5. Memory management: pools, arenas, allocators (`memory`).
* **Part II — Searching and ordering.**
    6. Searching in sorted data, learned indexes (`search`).
    7. Hashing and dictionaries, tries, integer structures (`dictionary`).
    8. Trees: binary, search, balanced, randomized, B-trees (`tree`).
    9. Priority queues (`priority`).
    10. Sets and filters (`set`).
    11. Disjoint sets (`mfset`).
    12. Sorting (`sort`).
* **Part III — Algorithms.**
    13. Algorithm design: divide and conquer, dynamic programming,
        greedy (`algorithm`, `optimization`).
    14. Graphs (`graph`).
    15. Strings (`string`).
    16. Geometry (`geometry`).
    17. Numerical algorithms (`numeric`).
    18. Combinatorial generation (`combinatorial`).
    19. Backtracking (`backtrack`).
    20. Complexity and intractable problems (`complexity`,
        `intractable`).
* **Part IV — Engineering.**
    21. Compression (`compression`).
    22. Succinct structures (`succinct`).
    23. External memory (`external`).
    24. Concurrency (`concurrent`).
    25. Parallelism (`parallel`).
    26. Distributed computing (`distributed`).
* **Appendices.**
    * A. Writing a new structure: concept, contracts, tests, benchmark.
    * B. The benchmarks: machine, method, and the comparison with the
      other libraries.
    * Y. Glossary.
    * Z. Bibliography.
