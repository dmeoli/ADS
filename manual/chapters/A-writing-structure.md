# Appendix A. Writing a new structure {#app-A}

[Source: `include/ADS/stack/MinStack.h`, `stack/impl/MinStack.tpp`,
`ADT.h`, `Contract.h`, `docs/STYLE.md`, `docs/theory/stack.dox`,
`test/ods_lists.cpp`, `test/tests.txt`, `bench/queues.cpp`, `src/ADS.cppm`]

We are concerned in this appendix with the steps by which a structure
enters ADS, from its header to the pages of this manual, in the order in
which a contributor takes them. Each step is described as the repository
does it at the time of writing. Our running example is `MinStack`, the
STACK with its least item of [Morin 2013, exercise
3.14](Z-bibliography.md#ref-morin-cpp). It is small enough to be read in
full (a header of 167 lines and a `.tpp` of 66), and yet it goes through
almost all the steps: a concept that it models, contracts of three kinds,
two operations with pseudocode, a lemma on its theory page, a test in the
registry, a line of the module and one of the changelog. It has no
benchmark, and hence [§A.7](A-writing-structure.md#sec-A-7) takes as its
example the stacks that have one. A structure written for speed (the
*engineered* level of [§1.9](01-introduction.md#sec-1-9)) goes through the
same steps, with the differences noted where they arise; the method of the
benchmarks is the subject of [Appendix B](B-benchmarks.md#app-B).

## A.1 The folder and the files {#sec-A-1}

A structure goes in the folder of the abstract data type that it
realizes, which is also the area of its chapter. Thus, `MinStack` is a
STACK and lives in `include/ADS/stack`, next to `ArrayStack` and
`MultiStack`, while the queues and the deques with the minimum of
exercises 3.15 and 3.16 live in `include/ADS/queue`. A structure that
realizes no abstract data type of the book goes in the folder of the
problem it solves (e.g., `succinct` or `external`). A new folder needs a
new theory page ([§A.5](A-writing-structure.md#sec-A-5)) and a row
in the map of the areas of `manual/build/structmap.py`.

Each structure is one header, here `MinStack.h`, which declares the class
with its contracts. At its end, the header includes the definitions of its
templates from the folder `impl/` next to it, here `impl/MinStack.tpp`.
Small members (the observers, `clear()`) are defined in the class, while
the operations that carry pseudocode and contracts are defined in the
`.tpp`. Both files have the frame of all the sources of ADS, which opens
with a box of 78 columns holding the name of the file:

```cpp
/*--------------------------------------------------------------------------*/
/*---------------------------- File MinStack.h -----------------------------*/
/*--------------------------------------------------------------------------*/
```

After it come the comment `/** @file`, which Doxygen shows as the
description of the header, and then the boxes `DEFINITIONS`, `INCLUDES`,
`NAMESPACE` and `CLASS MinStack`. We include the headers of ADS by their
path from `include/` (e.g., `"ADS/Contract.h"`), before those of the
standard library in alphabetical order. Its guard is `#ifndef
__ADS_MinStack`, followed by the comment `/* self-identification: #endif
constant */`. A box `IMPLEMENTATION` precedes the line `#include
"ADS/stack/impl/MinStack.tpp"`, and the box `End File MinStack.h` closes
the file. As for the `.tpp`, it has no guard and only three boxes, that of its
name, `IMPLEMENTATION` before the definitions and `End File MinStack.tpp`,
and its `@file` comment is one sentence ("The operations of MinStack [see
MinStack.h], with their contracts.").

Inside the class, the members are named `f_` followed by the name of the text
(`f_A` the items, `f_M` the minima, `f_less` the order). Furthermore, the type
of the items is the alias `Item`, which the concepts require, and the order is
a parameter `C` with `std::less<>` as its default, stored as
`[[no_unique_address]]`, whereby an empty comparator takes no room. We format
the code by the rules of the repository `code-style`, which the continuous
integration applies to each pull request
([§A.10](A-writing-structure.md#sec-A-10)).

In the `@file` comment a reader learns what the structure is before
reading any code, and it follows a fixed order: (i) what the header
realizes and from which source, with the section or the exercise ("The
STACK with the minimum of exercise 3.14 of Morin's book, beyond the book
[see `ADT.h`]"); (ii) the representation, its invariant and the costs,
each with its reason when that is not obvious (e.g., the minima are one
item per record low from the bottom, hence $n$ at most); (iii) the order
of the items and the contracts, with the cost of the invariant; (iv) the
`\see` lines of the sources; (v) the author and the copyright. In it, the words
"beyond the book" mark a structure that the main textbook of the library,
[Montresor 2025](Z-bibliography.md#ref-montresor2025), does not describe.
Hence the tests of such a structure go in the programs of the source it comes
from ([§A.6](A-writing-structure.md#sec-A-6)), not in those of the
chapters of that book (`ch01` to `ch19`).

A structure at the textbook level follows its pseudocode line by line,
and its comment states it. An engineered one, instead, cites the paper it
follows, and its comment states what it changes with respect to the code
of the textbook and why that change should be faster. Where the code of
the textbook is available, the comment compares with it explicitly. For
instance, the headers of the skip list, of the scapegoat tree, of the
treap and of the left-leaning Red-Black tree each have a paragraph "Where
it is meant to be faster than the ... of Morin, for the benchmarks to
check", whose claims are tested by the campaign of
[§B.6](B-benchmarks.md#sec-B-6).

## A.2 The concept {#sec-A-2}

A structure models the concept of its abstract data type, i.e., the names
and the types of its operations (the syntactic part of the specification,
in the terms of [§1.2](01-introduction.md#sec-1-2)). We give the concepts
of the book in `ADS/ADT.h` (`Sequence`, `Set`, `Dictionary`, `Stack` and
`Queue`). Those of the interfaces of Morin's book are in
`ADS/set/SetInterfaces.h` (`USet` and `SSet`, with the `KeyedDictionary`
and `OrderedDictionary` on which `DictionarySet` builds them) and in
`ADS/sequence/IndexedList.h` (`IndexedList` and `IndexedDeque`). The
concept of the STACK is

```cpp
--8<-- "generated/ADT-Stack.cpp"
```

and `MinStack` models it, since it has the type `Item`, `empty()`,
`push()`, `top()` and a `pop()` that returns the item removed. Its
`min()` is an addition that the concept neither requires nor forbids;
hence the algorithms written on `Stack` (e.g., those of
[§3.7](03-stack.md#sec-3-7)) run on a `MinStack` unchanged. A concept is
checked where this is cheapest, by a `static_assert` in the program of the
tests that includes the header:

    static_assert( Stack< MinStack< int > > );

This line of `test/ods_lists.cpp` stands next to those of `MinQueue` and
`RealTimeMinQueue` on `Queue` and to those of the lists of Morin on
`IndexedList` and `IndexedDeque`. A wrong name or type makes the
`static_assert` fail at compile time. However, it does not check the meaning of
the operations: the axioms in the comment of each concept (for the STACK,
$\mathrm{top}( \mathrm{push}( S , v ) ) = v$ and $\mathrm{pop}( \mathrm{push}(
S , v ) ) = S$) are checked by the tests
([§A.6](A-writing-structure.md#sec-A-6)), and the constructive part of the
specification by the contracts.

## A.3 The contracts {#sec-A-3}

In ADS, the contracts of an operation are its precondition, its postcondition
and the invariant of the structure, in the sense of [Meyer
1992](Z-bibliography.md#ref-meyer1992). ADS checks them at run time by
the four macros of `ADS/Contract.h`, which mirror the contracts of C++26
([Berne et al. 2025](Z-bibliography.md#ref-berne2025)), so that the
compilers that support those can replace them. All four expand to the
macro `ADS_CONTRACT_CHECK( level , kind , cond )`, each with its level and
its kind, which tests the condition under `if constexpr( ADS_CONTRACTS >=
level )` and, when the condition is false, calls a function of
`ADS::detail` with the kind, the text of the condition, the function, the
file and the line. `ADS_PRE` is checked from the level
1 and `ADS_POST` from the level 2, while `ADS_INVARIANT` and `ADS_AUDIT`
(a precondition or a postcondition that costs more than the operation,
e.g., that a vector is sorted before a binary search) are checked from the
level 3. This level is the macro `ADS_CONTRACTS`, which is 1 by default and
0 when `NDEBUG` is defined (as for `assert`); we compile the programs of
the tests at 3 and the benchmarks at 0. A violated contract throws
`ADS::contract_violation`, a `std::logic_error` whose message names the
kind, the condition, the function and the place. Since a condition is
compiled at every level but evaluated only from its level up, it must remain
correct C++ and have no side effect.

`MinStack` has contracts of three kinds (it needs no audit). The
precondition `! f_A.empty()` opens `top()`, `min()` and `pop()`, the three
partial operations of the STACK with the minimum. Also, the postconditions
of `push()` and `pop()` check that the size grows or shrinks by one, at a
cost of $O( 1 )$, as befits a postcondition. Finally, the invariant is
the private member `ordered()`, which checks in $O( n )$ time that the
minima do not increase from the bottom and that their top is equivalent,
under the order, to the least item of the vector. Indeed, the tests rely on it,
since at the level 3 each `push()` and `pop()` of the test of
[§A.6](A-writing-structure.md#sec-A-6) checks it; it is also the statement
of [Lemma A.1](A-writing-structure.md#lem-A-1). Note that the copy of the
size that the postconditions compare, `[[maybe_unused]] const std::size_t
n = f_A.size();`, carries its attribute because the compiler would
otherwise warn at the levels where no contract reads it.

## A.4 The pseudocode, the code and the references {#sec-A-4}

Each operation that is an algorithm (i.e., not an accessor, nor a
constructor that only copies) carries its pseudocode in its comment,
after the description and before `\pre` and `\post`. Its conventions are
those of section 2 of `docs/STYLE.md`: one display `\f[
\class{pseudocode}{ ... } \f]`, an array with the numbers of the lines in
its first column and its rows ended by `\cr`, the keywords in
`\textbf{}`, the calls in `\mathrm{}` and the assignment as `\gets`.
Also, the comment lines stay within 79 columns, and none of them may end with a
backslash. The paragraph before the display names the source ("In
pseudocode (Morin, exercise 3.14), $A$ the items and $M$ the minima:"),
and for a paper already cited by a `\see` line it reads "(Kahn's
algorithm, beyond the book)" or the like. We write the pseudocode of the
works under copyright (the books of Knuth, the notes of Ferragina, the
papers) in our own words, from the description of the algorithm and from
the code of ADS. That of Montresor's book, under CC BY-NC 4.0, may instead
be adapted with its section cited. For `MinStack` the two displays read
as in [Algorithm A.1](A-writing-structure.md#alg-A-1).

<span id="alg-A-1"></span>**Algorithm A.1.** The operations of `MinStack`
that change it ([Morin 2013, exercise 3.14](Z-bibliography.md#ref-morin-cpp));
$A$ is the vector of the items and $M$ that of the minima.

--8<-- "generated/MinStack-push.pseudo.md"

--8<-- "generated/MinStack-pop.pseudo.md"

Both procedures test $\mathbf{not}\ \mathrm{top}( M ) < x$, rather than
$x \le \mathrm{top}( M )$, since the class has only `less`, which is a
strict weak order. This test sends to $M$ also an item equivalent to the
minimum, and this is why `pop()` is correct when two equal minima are on
the stack:

<span id="lem-A-1"></span>**Lemma A.1.** *After any sequence of
operations of [Algorithm A.1](A-writing-structure.md#alg-A-1) on an empty
`MinStack`, $M$ does not increase from its bottom, and if the stack is not
empty then $\mathrm{top}( M )$ is a least item of $A$. Moreover, $M$ holds
one item for each item of $A$ that is not larger than all those below it,
hence at most $n$ items.*

*Proof.* By induction on the operations, the claim being true of the empty
stack. A `push()` of $x$ with $\mathrm{top}( M ) < x$ leaves both the
least item and $M$ as they were. Otherwise $x$ is not larger than the
least item of $A$, and it goes on top of $M$, which remains not
increasing. A `pop()` of $x$ finds $\mathrm{top}( M ) \le x$ by the
inductive hypothesis. If $\mathrm{top}( M ) < x$, then $x$ never entered
$M$ (an item enters $M$ only when it is not larger than its top), and $M$
stays as it is. If instead $x$ is equivalent to $\mathrm{top}( M )$, then
$x$ is the item that put it there or an equivalent one pushed later,
which entered $M$ too; hence popping $M$ leaves on its top the least of
the items below $x$. Finally, the last claim follows, since an item enters $M$
exactly when it is a record low from the bottom of $A$. $\square$

Hence `min()` costs $O( 1 )$ in the worst case, and `push()` and `pop()` cost
$O( 1 )$ but for the doublings of the two vectors (the header states "$O( 1 )$
amortized time"). On a decreasing input $M$ has $n$ items, while on a random
permutation (cf. the theory page) it has $H_n = O( \log n )$ expected, the
number of its records. [Figure A.1](A-writing-structure.md#fig-A-1) shows the
two branches of `push()` and a `pop()` that removes a minimum.

![Figure A.1. A `MinStack` with the items 5, 3, 7, 3 after `push(9)`, `push(2)` and `pop()`; what each operation adds is in orange, what it reads or removes in violet, the rest in the faint tint. The 9 does not enter $M$, while the 2 does, and leaves it with `pop()`.](../figures/stack-min.svg){#fig-A-1}

The code of the two operations is the pseudocode with the names of the
class. We show it here, as the example of how a structure is written,
without the contracts, which the text states (the chapters of the areas,
instead, give only the pseudocode, the signatures and short examples of
use, see [§A.11](A-writing-structure.md#sec-A-11)):

```cpp
--8<-- "generated/MinStack-push.cpp"
```

```cpp
--8<-- "generated/MinStack-pop.cpp"
```

These listings come from the `.tpp`, where two comments that change no
code, the markers `//! [push]` and `//! [pop]`, delimit them. Such markers
are those of the command `\snippet` of Doxygen, and
[§A.11](A-writing-structure.md#sec-A-11) shows how the manual extracts
them.

Finally, each source of a header has its `\see` line, with the section,
the exercise or the pages, and with the DOI whenever the work has one
(e.g., `doi:10.1109/2.161279` for the paper of Meyer in `Contract.h`).
A DOI is checked on `doi.org`, never guessed. Morin's book is cited by the
URL of its C++ edition, `https://opendatastructures.org/ods-cpp.pdf`,
which the code of ADS follows. Indeed, the DOI of the book belongs to its
edition in pseudocode ([Morin 2013](Z-bibliography.md#ref-morin2013)),
whose numbering of the exercises may differ.

## A.5 The theory page {#sec-A-5}

An area collects its definitions, its notation, its invariants and the
arguments behind its costs in a theory page, `docs/theory/<folder>.dox`.
Section 1 of `docs/STYLE.md` fixes its form: one Doxygen group
`theory_<folder>`, titled "Theory of" followed by the title of the area
and placed in the group `area_<folder>`, with sections
`theory_<folder>_<name>` for the definitions, the realizations and their
costs, each family of algorithms, and the references. A new structure
adds two things to it: (i) its row in the table of the realizations and
their costs, with the reason of each cost that is not obvious; (ii) the
statement on which its correctness rests, as a numbered paragraph in
italics followed by its argument.

For `MinStack`, `docs/theory/stack.dox` has the row of the table in its
section `theory_stack_repr` (two vectors, the items and the record lows).
Its statement is in the section `theory_stack_ods`, "The stacks of
Morin's book", as Lemma 3 (the minimum of a stack, exercise 3.14), with
the argument of [Lemma A.1](A-writing-structure.md#lem-A-1); also, the
theory page of the queues refers to it for the queue with the minimum,
which is made of two such stacks. A header refers to its theory page by
`\ref`, and the build of the two sites finds and links a new page with no
change of `docs/apiref.py`. This manual supersedes the
page without replacing it, since the page remains the documentation that
the API reference shows next to the headers.

## A.6 The tests {#sec-A-6}

The tests are programs of `test/`, grouped by their source: `ch01.cpp` to
`ch19.cpp` for the chapters of Montresor's book, `ods_ints.cpp`,
`ods_lists.cpp` and `ods_trees.cpp` for Morin's, `taocp.cpp` for Knuth's,
`pearls_*.cpp` for Ferragina's and `exam_*.cpp` for the exams of the
course (and `conclusion.cpp` for the conclusion of Montresor's book).
Furthermore, `beyond.cpp` tests what no book covers, `lib_*.cpp` the
problems that the library adds in an area (e.g., `lib_graphs.cpp`),
`contract_<area>.cpp` the contracts and the axioms of an area,
`interop.cpp` the standard interface of the containers, and
`engineered*.cpp` (with a few programs by family, e.g., `heaps.cpp`,
`hashing.cpp`, `join_trees.cpp`, `ordered.cpp` and `sorting.cpp`) the
engineered structures. A test is a function registered
with a name by the macro `ADS_TEST` of `test/ADSTest.h`. This header also
gives `CHECK( c )`, whose failure names its file and line, `CHECK_THROWS(
e , E )`, which checks that a precondition throws, and `scaled( n , small
)`, the size of a case that is there for its scale only (the build of the
coverage makes it small through `ADS_TEST_SMALL`).

A test is named after its source, and that of `MinStack` is
`ods-ex3.14.min-stack`, in `test/ods_lists.cpp`. It runs 4000 random
operations, about 60% of which are pushes (all of them on an empty stack)
of keys in $[ 0 , 50 )$, where equal minima occur often. A `std::vector`
holds the same items, and the test compares with it the item that each
`pop()` returns and, after each operation, the size, the top and `min()`
(the latter against `std::min_element`). Then it pushes 0 to 99 in increasing
order and checks by `minima()`, the size of $M$, that a single minimum is
kept, i.e., the bound of
[Lemma A.1](A-writing-structure.md#lem-A-1) at its best. Since the
program is compiled with `ADS_CONTRACTS=3`, each operation also checks
the invariant of [§A.3](A-writing-structure.md#sec-A-3).

In `test/tests.txt`, the registry of the tests, there is one line per test
with its program, its name and its source, e.g.,

    ods_lists  ods-ex3.14.min-stack         MinStack

Both builds read it. On one side, `test/CMakeLists.txt` makes a program
for each name of the first column and a ctest `<program>.<name>` for each
line; on the other, the makefile builds the same programs (`make`) and
runs the same tests (`make test`). Hence a new test, or a new program,
takes one line of the registry and no change of either build. Note that the two
build files are kept specular by hand only for what the registry cannot
express, i.e., the programs of MPI (`engineered_mpi`, run on 1 to 4
processes), the test of the order of the includes of FastFlow and the test
of the module, which both files list. Then the ctest `<program>.registry` (in
the makefile, the target `test-<program>`) checks that the tests of a
program, as `<program> -l` lists them, are those of the registry; thus,
no test can be written and never run.

One runs the test of `MinStack` by itself with `ctest --test-dir build -R
ods_lists.ods-ex3.14.min-stack`, or by `build-make/ods_lists
ods-ex3.14.min-stack` after `make`. An exercise of a chapter of this
manual names a ctest of this form, which must exist.

## A.7 The benchmark {#sec-A-7}

An engineered structure enters a benchmark of `bench/`, and so does a textbook
one that has a counterpart in the standard library or in a well-known library.
It goes in the program of its family, as one more structure measured on the
same cases as the others. `MinStack` has no benchmark, since no library that
ADS is compared with has a stack with the minimum; however, the other stacks of
the library show the pattern. For instance, the program `bench/queues.cpp` has
a function `stack( who , n , r , make , push , pop )`, which measures $n$
pushes followed by $n$ pops (the case `push-pop`) on the structure that `make`
builds. Its `main()` calls it once per structure: `std::stack` on a
`std::vector` twice, under the labels `std::vector` and `std::vector A/A`, then
`std::stack` on a `std::deque`, `boost::container::devector` (if Boost is
found), `ADS::ArrayStack`, `ADS::MultiStack` and `ADS::RingDeque`. A new stack
would be one more call, with its label (e.g., `"ADS::MinStack"`), its
constructor and its two operations as lambdas.

Three functions of `bench/Bench.h` do the rest. First, `skip( who )` leaves
out the cells of any structure other than the one that `ADS_BENCH_WHO`
names, and under `ADS_BENCH_LIST` it prints the labels instead; this
allows `bench/run.sh` to run each structure in a separate process.
Then, `median( prepare , work , r , ops )` times `work` $r$ times, on what
the untimed `prepare` returns, and gives the median in nanoseconds per
operation. Finally, `print()` writes the line of a cell, separated by
tabs, as `( benchmark , case , n , structure , ns/op )`. A competitor that
may be missing goes under a macro of the form `ADS_HAS_<LIBRARY>` (which
the makefile sets from a variable naming its prefix or its sources) or
under `__has_include`; hence the program builds wherever ADS does. All the
benchmarks are compiled with the contracts off.

A new program `bench/<name>.cpp` takes four more lines. It goes in the
list `BENCHES` of the makefile and in the `foreach` of the benchmarks of
`CMakeLists.txt`, which are kept specular by hand. It also needs a line
`run` in `bench/run.sh`, without which no campaign measures it, and a row in
the list `PAIRS` of `bench/summary.py` for each structure of ADS that the
summary of the README compares with a library. [Appendix
B](B-benchmarks.md#app-B) gives the method by which these programs are
run.

## A.8 The module {#sec-A-8}

The C++20 module `ads` exports all the names that the headers declare in
the namespace `ADS` and in those nested in it, but for the namespaces
`detail`. Its interface, `src/ADS.cppm`, is not written by hand. Instead,
the script `cmake/ads_module_exports.py` reads the AST of all the headers,
as `clang` dumps it, and writes for each header an `#include` in the
global module fragment and a block of using-declarations, e.g.,

    // ADS/stack/MinStack.h
    using ADS::MinStack;

Hence a new header, or one that gains or loses a public name, needs the
script to be run again (`python3 cmake/ads_module_exports.py`, with the
flags of the optional executors when their names are to be exported). We
build the module by `-DADS_BUILD_MODULE=ON` with CMake, or by `make test
ADSMOD=1`, and the test `module` imports it in place of the headers; a
name that the script missed shows as a program that does not compile.

## A.9 The changelog {#sec-A-9}

`CHANGELOG.md` follows Keep a Changelog, with the changes since the last
release under `## [Unreleased]` and its subsections `### Added`,
`### Changed`, `### Removed` and `### Fixed`; the versions follow semantic
versioning. Each entry of a new structure names the header and the
classes, the source (the exercise or the paper), and the tests and the
benchmark that come with it. `MinStack` entered with the other structures
of Morin's exercises, in the item that lists "MinStack, MinQueue and
MinDeque (exercises 3.14 to 3.16)". An entry describes the library as it
is after the change, not the steps by which the change was made.

## A.10 The checks before pushing {#sec-A-10}

The workflow `.github/workflows/ci.yml` builds all the programs and runs
all the tests with g++ and clang++ on Linux, macOS and Windows (and with
MSVC by CMake only), by CMake and by the makefile, with and without the
backends of GMP and of a CBLAS. Its job `sanitizers` runs all the tests
under AddressSanitizer and UndefinedBehaviorSanitizer (`make test` with
`-fsanitize=address,undefined` and `ADS_TEST_SMALL`). Other jobs run the
concurrent structures under ThreadSanitizer (`threads`), measure the
coverage, build the module by CMake and by the makefile, run the programs
of MPI on 1 to 4 processes, and build the executors and the backends that
are optional (`executors`, `backends`); on a pull request, the
action of `code-style` also checks the formatting of the lines changed.

A push is expected to leave the workflow green, and usually the cheapest way to
ensure this is to run its steps first. These are `make -j4 test` with g++ and
with clang++ (the two compilers often diagnose somewhat different errors in the
templates), and the same with `CXXFLAGS="-std=c++20 -O1 -g
-fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra
-DADS_TEST_SMALL"`, as the job does; a concurrent structure adds a run under
ThreadSanitizer. Also, the files must keep their frame: no line may exceed 79
columns, and the boxes must be aligned, which a checker of the boxes, where one
has it, verifies on the files changed (section 3 of `docs/STYLE.md`). Then the documentation must build
with no warning by `bash docs/build.sh`, which also fails on a comment line
that ends with a backslash. Section 3 of `docs/STYLE.md` adds the screenshots
of the pseudocode on both sites, in the light and in the dark theme.

## A.11 The pages of this manual {#sec-A-11}

A structure enters the chapter of its area (for `MinStack`, [Chapter
3](03-stack.md#ch-3)) with the parts that `manual/MAINTENANCE.md` and the
model chapter fix. These are the abstract data type and its axioms, the
the representation and its invariant, a figure of the state before and after
the operation that matters, the pseudocode, the lemmas and the theorems with
their proofs, the signatures of the operations that a user calls with short
examples of use, the engineered variants, and the exercises, each naming
its ctest. A chapter shows no extract of an implementation, which the
pseudocode replaces, and it copies neither pseudocode nor code: instead, the script `manual/build/snarf.py` (run by `make snippets` in
`manual/build`) writes into `manual/generated` two kinds of files: (i) a
file `<File>-<name>.cpp` for each pair of markers `//! [name]` of a `.h`
or a `.tpp`, dedented and without the contracts and the copies that only
they read; (ii) a file `<File>-<name>.pseudo.md` for each pseudocode of
the comments, named after its first procedure or function. A member of a
class not named as the file takes the class in front of its name, and a
name that occurs twice in a header takes `-2`, `-3`, and so on.

The chapter includes these files with a line `--8<-- "generated/<file>"`,
in a separate paragraph for the pseudocode and as the only line of a
fenced `cpp` block for the code. On the site the line is expanded by
`pymdownx.snippets`, and in the PDF by `build/snippets.lua`. For instance,
[§A.4](A-writing-structure.md#sec-A-4) includes in this way
`MinStack-push.pseudo.md` and `MinStack-pop.pseudo.md`, and, as the example
of this appendix only, `MinStack-push.cpp` and `MinStack-pop.cpp`. A
function whose comment has no pseudocode gets one in its header (a change
of the comments only) before a chapter describes it.

A figure is a standalone TikZ file, `manual/figures/<area>-<name>.tex`
(here `stack-min.tex`), that loads `lmodern`, `amsmath` and `amssymb`,
`tikz` and `lamalunga`, and then inputs `tikz-ads.tex`. It draws with the
styles of that file only: `touched` (with its variants for nodes,
vertices and edges) for what the operation reads or follows, `added` for
what it adds, and `cell`, `node`, `vertex`, `edge` and `free` for the
rest, with no outline and no colour outside the palette that lamalunga
computes from the primary. Then `make figures` writes its `.pdf` and its
`.svg`, and the chapter shows it in a separate paragraph,
`![Figure N.M. caption](../figures/<name>.svg){#fig-N-M}`.

A work that the chapter cites goes in `manual/chapters/Z-bibliography.md`,
in alphabetical order, with the anchor `ref-<surname><year>` and its DOI.
Two checks follow by `make check`: `build/bibcheck.py` verifies that the
bibliography holds all the works of the `\see` lines of the headers, of
the theory pages and of the README, and `build/structmap.py` that each
header and each public class appears in the map of the structures of
[§1.12](01-introduction.md#sec-1-12), whose tables it also writes. Then we
build the chapter with the others by `make pdf`, which must give no
overfull box and no undefined reference after a second run of LaTeX, and
by `make html`; we check its prose on the PDF against the conventions of
the front matter.

## A.12 Checklist {#sec-A-12}

[Table A.1](A-writing-structure.md#tab-A-1) collects the files that a new
structure touches, with the check by which one verifies each step;
its last column gives what `MinStack` has for each of them.

<span id="tab-A-1"></span>**Table A.1.** The files of a new structure,
in the order of this appendix, with the check of each step and the
corresponding file or entry of `MinStack`.

| Step | File | Check | `MinStack` |
|------|--------------|----------|------------|
| header | `include/ADS/<area>/<Name>.h` | boxes, 79 columns | `stack/MinStack.h` |
| operations | `include/ADS/<area>/impl/<Name>.tpp` | contracts at level 3 | `stack/impl/MinStack.tpp` |
| concept | a test program | `static_assert` | `Stack< MinStack< int > >` |
| pseudocode | the comments of the header | `docs/build.sh` | `push()`, `pop()` |
| theory | `docs/theory/<area>.dox` | `docs/build.sh` | Lemma 3 of `stack.dox` |
| test | `test/<source>.cpp`, `test/tests.txt` | `<program>.registry` | `ods_lists.ods-ex3.14.min-stack` |
| benchmark | `bench/<family>.cpp`, `bench/run.sh`, `bench/summary.py` | a campaign (Appendix B) | none |
| module | `src/ADS.cppm` | the test `module` | `using ADS::MinStack;` |
| changelog | `CHANGELOG.md` | under `[Unreleased]` | exercises 3.14 to 3.16 |
| manual | `manual/chapters/NN-<area>.md`, `manual/figures` | `make check`, `make pdf` | `stack-min.tex`, two pseudocodes |
