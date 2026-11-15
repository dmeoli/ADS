# 1. Introduction {#ch-1}

[Source: `include/ADS/ADT.h`, `Contract.h`, `Iterators.h`, `Portable.h`,
`Modification.h`, `Capabilities.h`, `Algebra.h`,
`ADS/sequence/IndexedList.h`, `ADS/sequence/ObservableArray.h`,
`ADS/sequence/Fenwick.h`, `ADS/queue/SlidingAggregate.h`,
`ADS/set/SetInterfaces.h`, `ADS/set/Bag.h`]

We are concerned in this manual with ADS, a header-only library of C++20 that
realizes the data structures and the algorithms of the textbooks, each in the
form in which a textbook presents it, together with the engineered variants
that the literature on algorithm engineering has proposed for many of them.
This chapter fixes what the other chapters take for granted. We first recall
why the organization of the data matters
([§1.1](01-introduction.md#sec-1-1)), how an abstract data type is specified
and how the library states it as a concept of C++20
([§1.2](01-introduction.md#sec-1-2)), and which interfaces the structures
realize ([§1.3](01-introduction.md#sec-1-3)), together with their simplest
realizations ([§1.4](01-introduction.md#sec-1-4)). Then come the mathematics
of the analyses ([§1.5](01-introduction.md#sec-1-5)), the model of
computation in which the costs are counted
([§1.6](01-introduction.md#sec-1-6)), and the meaning of correctness, time
and space for an operation, with the contracts by which the library checks
correctness ([§1.7](01-introduction.md#sec-1-7)). In the last sections we
describe the iterators ([§1.8](01-introduction.md#sec-1-8)), the two levels
of the library and its conventions ([§1.9](01-introduction.md#sec-1-9)), how
to build, test and use it ([§1.10](01-introduction.md#sec-1-10)), how this
manual is organized ([§1.11](01-introduction.md#sec-1-11)), the map of
all the classes of the library ([§1.12](01-introduction.md#sec-1-12)), the
typed modifications by which a structure tells its observers what changed
([§1.13](01-introduction.md#sec-1-13)), the capabilities that an
algorithm may ask of a structure ([§1.14](01-introduction.md#sec-1-14)), and
the first structures that tell their changes, with the aggregates that their
events feed ([§1.15](01-introduction.md#sec-1-15)).

## 1.1 The need for efficiency {#sec-1-1}

Each of the abstract data types of [§1.3](01-introduction.md#sec-1-3) can be
realized by a vector, or by a list, whose items are examined one by one at
each operation: a search scans them all, an insertion scans them to rule
out a duplicate, and a removal scans them to find its item. Such
realizations are "easy" to write correctly (those of
[§1.4](01-introduction.md#sec-1-4) are of this kind), and one may wonder
whether anything faster is needed on a machine that executes billions of
instructions per second, i.e., on a "fast" one; a rough count shows that it
usually is. Suppose that a program keeps $n = 10^6$ items and looks up each
of them once, i.e., makes $m = 10^6$ searches; if each search examines all
the items, the program makes $n m = 10^{12}$ examinations. A core of a
current processor runs at a few billion cycles per second and makes at most
a few simple operations per cycle, and each examination reads an item,
compares it and moves to the next. Hence, the core makes of the order of
$10^9$ examinations per second, and all the searches take of the order of
$10^3$ seconds, about a quarter of an hour. Doubling $n$ and $m$ makes it an
hour, and a data set of $10^9$ items (which is not "large" by today's
standards) would take decades. We remark that these figures are rather crude
orders of magnitude, not measures (the measures of the library are the
subject of [appendix B](B-benchmarks.md#app-B)); yet, no constant factor of
a faster machine changes the product $n m$.

A remedy (the "right" one, in most cases) is to organize the items so that an
operation need not examine all of them. A hash table (chapter
[7](07-dictionary.md#ch-7)) examines a constant number of items per search on
average, whatever $n$ is, and the $10^6$ searches above then take a few
million examinations, i.e., some milliseconds. A balanced search tree
(chapter [8](08-tree.md#ch-8)) keeps the items in order and examines at most
about $2 \log_2 n$ of them per operation, i.e., about 40 for $n = 10^6$ and
60 for $n = 10^9$. However, on a real machine the cost of an examination is
not uniform: a read that misses all the levels of the caches costs of the
order of a hundred nanoseconds, i.e., some hundreds of operations ([Drepper
2007](Z-bibliography.md#ref-drepper2007)). Two structures that examine the
same number of items may thus differ in running time by one order of
magnitude (typically, not more), according to how the items they examine lie
in memory. In ADS, the textbook structures are written to show the first
effect (the organization of the items), and the engineered ones also take the
second (somewhat harder to predict) into account
([§1.9](01-introduction.md#sec-1-9)).

## 1.2 Abstract data types {#sec-1-2}

A *specification* of an abstract data type is a triple $( D , F
, A )$ ([Montresor 2025,
§3.2](Z-bibliography.md#ref-montresor2025)). Here $D$ is a set
of domains, one of which is the *designated* domain, i.e., the
type being defined (e.g., the stacks), while the others are the
auxiliary ones (e.g., the items and the booleans). Then $F$ is
a set of operations, each a function among the domains of $D$;
$A$ is a set of axioms, which give the meaning of the
operations ([Liskov and Zilles
1974](Z-bibliography.md#ref-liskov1974); [Guttag and Horning
1978](Z-bibliography.md#ref-guttag1978)). The pair $( D , F )$
is the *syntactic* specification, and $A$ the *semantic* one.
The latter may be given in two ways: in the *algebraic* way, it
is a set of equations among the operations, in which those that
observe or remove are written on the terms built by those that
construct, as in $\mathrm{top}( \mathrm{push}( S , v ) ) = v$.
In the *constructive* way, it is the precondition and the
postcondition of each operation, written in terms of a type
already known (e.g., the finite sequences of items). As for the
operations, they are of four kinds, i.e., the *constructors*,
which make a value of the type (`create()`, `insert()`,
`push()`), the *modifiers*, which change it, the *observers*,
which return a property of it (`empty()`, `top()`, `read()`),
and the *iterators*, which visit its items.

A realization of the type meets the requirement of *abstraction* if a
program that uses the type does not depend on how it is realized, and that
of *protection* if only the operations of the type act on its values
([Parnas 1972](Z-bibliography.md#ref-parnas1972); [Liskov and Zilles
1974](Z-bibliography.md#ref-liskov1974)). In ADS we therefore write each
realization as a class whose data are private, and two realizations of the
same type can replace each other in a program. In C++20, the syntactic
specification is a *concept*, i.e., a predicate on types, checked at compile
time, which requires some expressions to be valid and to have some types
([Sutton
and Stroustrup 2012](Z-bibliography.md#ref-sutton2012)). That of the STACK,
in `ADS/ADT.h`, requires a type of the items, an observer `empty()` that
gives a `bool`, and the operations `push()`, `pop()` and `top()`:

```cpp
// declarations of <ADS/ADT.h>
template< class S >
concept Stack = requires( S s , const S cs , const typename S::Item & v )
{
 typename S::Item;
 { cs.empty() } -> std::same_as< bool >;
 s.push( v );
 { s.pop() } -> std::convertible_to< typename S::Item >;
 { cs.top() } -> std::convertible_to< typename S::Item >;
 };
```

A class models a concept by having its operations, with no declaration of
the fact. Hence, a realization written outside the library models the
concepts of the library as well as those inside it. A function template
constrained by the concept, e.g., `template< Stack S > void f( S & s )`,
takes any realization. A call that gives it a type that is not a stack
is rejected at the point of the call, with the name of the requirement that
fails. However, a concept checks the syntax and nothing more: two types with
the same operations and different axioms (say, a stack and a queue whose
operations had the same names) model the same concepts, and the compiler
cannot tell them apart. We therefore state the axioms of each type in the
comments of `ADS/ADT.h`, and the library checks them in two ways, i.e., the
tests check them on each realization (the tests `contract_*.*-axioms`), and
the code checks the constructive form, i.e., the preconditions and the
postconditions of the operations ([§1.7](01-introduction.md#sec-1-7)).

We write the axioms of the STACK, with $\mathrm{create}$ the empty stack,
as

$$
\begin{array}{ll}
\mathrm{empty}( \mathrm{create} ) = \mathbf{true} , &
\mathrm{empty}( \mathrm{push}( S , v ) ) = \mathbf{false} , \\
\mathrm{top}( \mathrm{push}( S , v ) ) = v , &
\mathrm{pop}( \mathrm{push}( S , v ) ) = S ,
\end{array} \tag{1.1}
$$

while $\mathrm{top}( \mathrm{create} )$ and $\mathrm{pop}( \mathrm{create}
)$ are undefined, i.e., `top()` and `pop()` are partial operations, whose
precondition is that the stack is not empty. In ADS `pop()` also returns the
item it removes, and the last axiom is about the stack it leaves. As for the
QUEUE, it has `enqueue()`, which adds at the back, `dequeue()`, which
removes from the front, and `top()`, which reads the front; besides
$\mathrm{empty}( \mathrm{create} ) = \mathbf{true}$ and $\mathrm{empty}(
\mathrm{enqueue}( Q , v ) ) = \mathbf{false}$ for any $Q$, its axioms are,
for $Q$ not empty,

$$
\mathrm{top}( \mathrm{enqueue}( Q , v ) ) = \mathrm{top}( Q ) , \qquad
\mathrm{dequeue}( \mathrm{enqueue}( Q , v ) ) =
\mathrm{enqueue}( \mathrm{dequeue}( Q ) , v ) , \tag{1.2}
$$

with $\mathrm{top}( \mathrm{enqueue}( \mathrm{create} , v ) ) = v$ and
$\mathrm{dequeue}( \mathrm{enqueue}( \mathrm{create} , v ) ) =
\mathrm{create}$. Since the syntactic specifications of the two types are
the same up to the names of the operations, (1.1) and (1.2) are the whole
difference between a stack and a queue. Note that the axioms must also
decide the value of each observer on each value of the type, and they are
then *sufficiently complete*, in the terms of [Guttag and Horning
1978](Z-bibliography.md#ref-guttag1978), which makes them a specification
rather than a list of properties; for the stacks this follows from a normal
form of their terms.

<span id="lem-1-1"></span>**Lemma 1.1.** *By (1.1), each stack built by
$\mathrm{create}$, $\mathrm{push}$ and $\mathrm{pop}$, each $\mathrm{pop}$
applied to a stack that is not empty, is equal to a term*

$$
\mathrm{push}( \cdots \mathrm{push}( \mathrm{push}( \mathrm{create} ,
v_1 ) , v_2 ) \cdots , v_k ) , \quad k \ge 0 , \tag{1.3}
$$

*and the observers of a term (1.3) are determined: it is empty if and only
if $k = 0$, and its top is $v_k$.*

*Proof.* By induction on the number of operations of the term, since the
equations of (1.1) may be applied to any subterm. A term with no operation
is $\mathrm{create}$, i.e., (1.3) with $k = 0$. Otherwise, its outermost
operation is applied to a subterm that, by induction, is equal to a term
(1.3); a $\mathrm{push}$ applied to a term (1.3) is a term (1.3), and a
$\mathrm{pop}$ applied to a term (1.3), which has $k \ge 1$ by hypothesis,
is, by the last axiom of (1.1), the term (1.3) of $v_1 , \ldots , v_{k-1}$.
The values of $\mathrm{empty}$ and $\mathrm{top}$ on (1.3) are given by the
first three axioms of (1.1). $\square$

The same argument holds for the queues by (1.2), with a normal form
$\mathrm{enqueue}( \cdots \mathrm{enqueue}( \mathrm{create} , v_1 ) \cdots ,
v_k )$ made of $\mathrm{enqueue}$ only. A $\mathrm{dequeue}$ of such a term
with $k \ge 1$ is, by induction on $k$ (the second axiom of (1.2) for $k \ge
2$, and $\mathrm{dequeue}( \mathrm{enqueue}( \mathrm{create} , v ) ) =
\mathrm{create}$ for $k = 1$), the normal form of $v_2 , \ldots , v_k$. The
axioms on $\mathrm{empty}$ and $\mathrm{top}$ give that the term is empty if
and only if $k = 0$ and that its front is $v_1$. For the sets, the axioms
(1.6) below decide $\mathrm{contains}( x , A )$ for each item $x$ and each
term $A$, by induction on $A$. Hence, if we take two sets as equal when they
contain the same items (the only equality that (1.6) can define, since it has
no equation among sets), each term is equal to a term made of
$\mathrm{insert}$ only, on the distinct items that it contains, in any order.
Of course, the axioms say nothing about the cost of the operations, which is
rather the business of the realizations; a stack on a vector and a stack on a
list satisfy (1.1) in the same way, and differ in time and space
([§3.2](03-stack.md#sec-3-2) and [§3.3](03-stack.md#sec-3-3)).

## 1.3 The interfaces {#sec-1-3}

An *interface* (i.e., an abstract data type, as a program sees it) states
what a structure does, and an *implementation* (or realization) how it does
it. There are typically several realizations of one interface, which differ
in the cost of the operations, and choosing among them is the first decision
in the use of a library ([Morin 2013,
§1.2](Z-bibliography.md#ref-morin-cpp)). ADS has the interfaces of two
textbooks. Those of [Montresor 2025](Z-bibliography.md#ref-montresor2025)
name the operations as its pseudocode does, and are the concepts of
`ADS/ADT.h`. Those of [Morin 2013, §1.2](Z-bibliography.md#ref-morin-cpp)
read the sequences by rank and the sets through the items they find, and are
the concepts of `ADS/sequence/IndexedList.h` and `ADS/set/SetInterfaces.h`,
the latter with two more concepts of the library that view the keys of a
dictionary as such a set. [Table 1.1](01-introduction.md#tab-1-1) lists them,
with the chapters where their realizations are described. The concepts of the
other areas (the PRIORITY QUEUE of `ADS/priority/PriorityQueue.h`, the GRAPH
of `ADS/graph/Graph.h`, the executors of `ADS/parallel/Executor.h`) are
described with their areas instead.

<span id="tab-1-1"></span>**Table 1.1.** The concepts of the interfaces of
chapter 1, their operations and the chapters of their realizations; M is
[Montresor 2025](Z-bibliography.md#ref-montresor2025), O is [Morin
2013](Z-bibliography.md#ref-morin-cpp), and ADS marks a concept of the
library.

| Concept | Interface | Operations | Source | Chapters |
|----------------|------------|---------------------------|-------|------|
| `Sequence` | SEQUENCE | `empty()`, `finished()`, `head()`, `tail()`, `next()`, `prev()`, `insert()`, `remove()`, `read()`, `write()` | M, §3.2.1 | [2](02-sequence.md#ch-2) |
| `Set` | SET | `size()`, `contains()`, `insert()`, `remove()` | M, §3.2.2 | [10](10-set.md#ch-10) |
| `Dictionary` | DICTIONARY | `lookup()`, `insert()`, `remove()` | M, §3.2.3 | [7](07-dictionary.md#ch-7), [8](08-tree.md#ch-8) |
| `Stack` | STACK | `empty()`, `push()`, `pop()`, `top()` | M, §4.3.1 | [3](03-stack.md#ch-3) |
| `Queue` | QUEUE | `empty()`, `enqueue()`, `dequeue()`, `top()` | M, §4.3.1 | [4](04-queue.md#ch-4) |
| `IndexedList` | LIST | `size()`, `operator[]`, `insert( i , x )`, `erase( i )` | O, §1.2.2 | [2](02-sequence.md#ch-2), [4](04-queue.md#ch-4) |
| `IndexedDeque` | DEQUE | `size()`, `push_front()`, `push_back()`, `pop_front()`, `pop_back()`, `front()`, `back()` | O, §1.2.1 | [2](02-sequence.md#ch-2), [4](04-queue.md#ch-4) |
| `USet` | USet | `size()`, `insert()`, `remove()`, `find()` | O, §1.2.3 | [1](01-introduction.md#ch-1) |
| `SSet` | SSet | those of `USet` and `lowerBound()` | O, §1.2.4 | [1](01-introduction.md#ch-1) |
| `KeyedDictionary` | USet of keys | `findKey()`, `insert( k , v )`, `remove( k )` | ADS | [10](10-set.md#sec-10-4) |
| `OrderedDictionary` | SSet of keys | those of `KeyedDictionary` and `lowerBoundKey()` | ADS | [10](10-set.md#sec-10-4) |

### 1.3.1 Stacks, queues and deques {#sec-1-3-1}

A queue, in the wide sense of [Morin 2013,
§1.2.1](Z-bibliography.md#ref-morin-cpp), is a collection to which items are
added and from which the next item is removed, its *queueing discipline*
deciding which item is the next one. A first in, first out discipline (FIFO)
removes the items in the order in which they were added, and gives the QUEUE
of (1.2). A last in, first out discipline (LIFO) removes the item added last,
and gives the STACK of (1.1); a discipline of priorities removes the item of
least priority, and gives the PRIORITY QUEUE of chapter
[9](09-priority.md#ch-9). In ADS we keep the names of [Montresor 2025,
§4.3.1](Z-bibliography.md#ref-montresor2025), i.e., `push()`, `pop()` and
`top()` for the stack and `enqueue()`, `dequeue()` and `top()` for the queue,
while Morin calls both pairs `add()` and `remove()`. A DEQUE is a sequence
changed at both ends, by `push_front()`, `push_back()`, `pop_front()` and
`pop_back()`, and read by `front()` and `back()`; these are the names of
`std::deque` for the `addFirst()`, `addLast()`, `removeFirst()` and
`removeLast()` of Morin, and the concept is `IndexedDeque`. A deque realizes
both of the other two, since `push_front()` and `pop_front()` give a stack,
and `push_back()` with `pop_front()` a queue.

A stack is the "natural" structure of the nested computations, and [Morin
2013, exercises 1.2 and 1.3](Z-bibliography.md#ref-morin-cpp) make this
precise. A *Dyck word* is a sequence $w_1 , \ldots , w_n$ of $+1$ and $-1$
whose prefix sums $s_j = w_1 + \cdots + w_j$ are never negative (with no
condition on $s_n$, as in the exercise). If we read $+1$ as a
$\mathrm{push}$ and $-1$ as a $\mathrm{pop}$, $w$ becomes a sequence of
operations on a stack; by (1.1) each $\mathrm{push}$ adds one item and each
$\mathrm{pop}$ removes one, and hence the height of the stack after $j$
operations is $s_j$. Since the items pushed play no role, the function
`isDyckWord()` of `ADS/stack/StackAlgorithms.h` keeps the stack as its
height $h$ ([Algorithm 1.1](01-introduction.md#alg-1-1)).

<span id="lem-1-2"></span>**Lemma 1.2.** *The sequence $w$ is a Dyck word
if and only if the operations it encodes, applied to an empty stack, never
apply $\mathrm{pop}$ to the empty stack; the stack is empty at the end if
and only if, in addition, $s_n = 0$. Hence `isDyckWord()` decides whether
$w$ is a Dyck word in $O( n )$ time and $O( 1 )$ space.*

*Proof.* Let $h_j$ be the height of the stack after the first $j$
operations. As long as no $\mathrm{pop}$ is applied to the empty stack, one
has $h_j = s_j$, by induction on $j$ and (1.1). The first $\mathrm{pop}$
applied to the empty stack, if any, is thus the operation $j$ for the least
$j$ with $s_{j-1} = 0$ and $w_j = -1$, i.e., for the least $j$ with $s_j <
0$, since the prefix sums change by 1 at each step and the first negative
one is therefore $-1$, reached from 0. Hence no $\mathrm{pop}$ is applied to
the empty stack if and only if no prefix sum is negative, and in that case
the final height is $s_n$, which is 0 exactly when the stack is empty at the
end. Finally, `isDyckWord()` ([Algorithm 1.1](01-introduction.md#alg-1-1))
keeps $h = s_j$ and returns false at the first $j$ with $h = 0$ and $w_j =
-1$; it makes $O( 1 )$ operations per item, and its counter $h \le n$ takes
one word ([§1.6](01-introduction.md#sec-1-6)). $\square$

<span id="alg-1-1"></span>**Algorithm 1.1.** The Dyck words and the
matched strings of brackets by a stack ([Morin 2013, exercises 1.2 and
1.3](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/StackAlgorithms-isDyckWord.pseudo.md"

--8<-- "generated/StackAlgorithms-isMatched.pseudo.md"

A string of the brackets `()`, `[]` and `{}` is *matched* when the brackets
pair up properly. For instance, `{{()[]}}` is matched, while `{{()]}` is
not, since its second `{` faces a `]`. Similarly, `isMatched()`
([Algorithm 1.1](01-introduction.md#alg-1-1)) pushes each open bracket and
pops one at each closed bracket, which must be of the same kind, and answers
that the string is matched if no pop fails or mismatches and the stack is
empty at the end, in $O( 1 )$ time per character and $O( n )$ in all. In
fact, read with $+1$ for an open bracket and $-1$ for a closed one, a
matched string is a Dyck word with $s_n = 0$ by [Lemma
1.2](01-introduction.md#lem-1-2); the stack, which here keeps the brackets
themselves, also checks that each pair is of one kind. A third exercise
reverses a stack through a queue (`reverseWithQueue()`). We pop all the
items into the queue, and push them back in the order in which they leave
it. Indeed, the queue keeps the order of the pops, i.e., from the top down,
and the first item pushed back (the old top) ends at the bottom; hence, the
order is reversed, in $O( n )$ time
([Algorithm 3.7](03-stack.md#alg-3-7)).

### 1.3.2 Sequences and lists {#sec-1-3-2}

The SEQUENCE of [Montresor 2025,
§3.2.1](Z-bibliography.md#ref-montresor2025) is a dynamic sequence of items
$s_0 , \ldots , s_{n-1}$ reached through *positions*. Here $\mathit{pos}_i$
is the position of $s_i$, of the type `Pos` of the realization, and two more
positions, $\mathit{pos}_{-1}$ and $\mathit{pos}_n$, come before the first
item and after the last one, the observer `finished( p )` being true on
these two and only on them. The operations `head()`, `tail()`, `next()` and
`prev()` move among the positions, while `read()` and `write()` read and
change the item of a position. With $p = \mathit{pos}_i$, `insert( p , v )`
puts $v$ at the index $i$, the old items from the index $i$ on moving one
place on, and returns the position of the new item. Similarly, `remove( p )`
removes the item of index $i$ and returns the position of the new item of
index $i$; hence the axioms

$$
\mathrm{read}( \mathrm{insert}( p , v ) ) = v , \qquad
\mathrm{remove}( \mathrm{insert}( p , v ) ) = s , \qquad
\mathrm{read}( \mathrm{write}( p , v ) ) = v , \tag{1.4}
$$

the last one with the other items unchanged, together with
$\mathrm{empty}( s ) = \mathrm{finished}( \mathrm{head}( s ) )$, which makes
`empty()` redundant. What a position is depends on the realization (an
index of a vector, the address of a cell, a cursor in a vector of cells),
and so does the cost of each operation, while (1.4) holds on all of them.

The LIST of [Morin 2013, §1.2.2](Z-bibliography.md#ref-morin-cpp) reaches the
items by their *rank* instead: its `get( i )` and `set( i , x )` read and
write $x_i$, its `add( i , x )` sets $x_{j+1} \gets x_j$ for
$j = n - 1 , \ldots , i$, then $x_i \gets x$ and $n \gets n + 1$, and its
`remove( i )` sets $x_j \gets x_{j+1}$ for $j = i , \ldots , n - 2$ and
$n \gets n - 1$ ([Figure 1.1](01-introduction.md#fig-1-1)). The concept
`IndexedList` of `ADS/sequence/IndexedList.h` requires these operations with
the names of the standard library, i.e., `size()`, `operator[]` for both
`get()` and `set()`, `insert( i , x )` and `erase( i )`; it is not called
`List`, since that is the name of the doubly linked list of
`ADS/sequence/List.h`. A LIST realizes a DEQUE by

$$
\begin{array}{ll}
\mathrm{push\_front}( x ) = \mathrm{insert}( 0 , x ) , &
\mathrm{pop\_front}() = \mathrm{erase}( 0 ) , \\
\mathrm{push\_back}( x ) = \mathrm{insert}( n , x ) , &
\mathrm{pop\_back}() = \mathrm{erase}( n - 1 ) ,
\end{array} \tag{1.5}
$$

and a realization whose name contains "stack" or "deque" (e.g.,
`RootishArrayStack`, `DualArrayDeque`) is a LIST that is especially "fast" in
(1.5) at one end or at both. In ADS, the realizations of `IndexedList` are
`RingDeque` (chapter [4](04-queue.md#ch-4)), `DualArrayDeque`, `Treque`,
`ContiguousDeque`, `RootishArrayStack`, `TieredVector`, `CubishArrayStack`,
`SEList`, `SkipListSequence` and `TreapList` (chapter
[2](02-sequence.md#ch-2)), and those of `IndexedDeque` are the first four
of them, together with `TieredVector`, `SEList` and `XorList`. The test
program `ods_lists` checks at compile time that the first eight are models
of `IndexedList` and the seven realizations of the DEQUE models of
`IndexedDeque`, and its test `ods_lists.ods-1.2.interfaces` runs one
function template on those eight lists. Furthermore, the header has two
rotations of a LIST, `rotateArray()` and `rotate()`, which solve [Morin
2013, exercises 2.4 and 2.5](Z-bibliography.md#ref-morin-cpp) and are
described with the sequences in chapter [2](02-sequence.md#ch-2).

![Figure 1.1. A LIST of six items read by rank, `l[2]` reading `c` (in violet), and the same list after `insert(2, x)`, which writes `x` at the rank 2 (in orange) and moves the items from the rank 2 on by one rank; a DEQUE changes the two ends only.](../figures/intro-list.svg){#fig-1-1}

Note that the two interfaces differ somewhat in what is "cheap". A position
of a linked list is the address of its cell, and `insert()` and `remove()` at
a position take $O( 1 )$ time, while reaching the rank $i$ takes $O( 1 +
\min\{ i , n - i \} )$ steps from the nearer end. In a vector, on the other
hand, a rank is an index, reached in $O( 1 )$ time, while an insertion at the
rank $i$ moves $n - i$ items. Roughly speaking, a program that walks a
sequence and changes it where it stands had better use the positions of a
SEQUENCE, and one that jumps among ranks the LIST.

### 1.3.3 Sets, dictionaries and bags {#sec-1-3-3}

The SET of [Montresor 2025, §3.2.2](Z-bibliography.md#ref-montresor2025) is a
finite collection of distinct items, with `size()`, `contains( x )`,
`insert( x )` and `remove( x )`. Its axioms define the observer on each
constructor,

$$
\mathrm{contains}( x , \mathrm{insert}( y , A ) ) =
( x = y ) \lor \mathrm{contains}( x , A ) , \qquad
\mathrm{contains}( x , \mathrm{remove}( y , A ) ) =
( x \ne y ) \land \mathrm{contains}( x , A ) , \tag{1.6}
$$

with $\mathrm{contains}( x , \mathrm{create} ) = \mathbf{false}$; inserting an
item already there, or removing one that is not, thus leaves the set as it is,
and in this way the library relaxes the preconditions $x \notin A$ and
$x \in A$ that some specifications state. The DICTIONARY of [Montresor 2025,
§3.2.3](Z-bibliography.md#ref-montresor2025) is a univocal relation from keys
to values. Its `lookup( k )` gives the value of the key $k$ or nothing (the
nil of the book, `std::nullopt` in the code), `insert( k , v )` associates $v$
with $k$, replacing the value that $k$ had, and `remove( k )` removes $k$. Its
axioms are those of (1.6) with `lookup()` in place of `contains()`, and both
`lookup()` and `remove()` are total, i.e., the key need not be there.

The USet of [Morin 2013, §1.2.3](Z-bibliography.md#ref-morin-cpp) has `size()`
and three more operations. Its `insert( x )` (Morin's `add( x )`) adds $x$
unless an item equal to $x$ is there, and returns whether it did,
`remove( x )` returns whether it removed an item equal to $x$, and `find( x )`
gives the address of the item $y$ of the set equal to $x$, or nil. The
distinction between $x$ and $y$ matters, since two items may be equal without
being identical. If a pair $( k , v )$ is equal to another when the keys are
equal, the pair stored in a USet is found by $\mathrm{find}( ( k , \cdot ) )$
with any value, and the USet then serves as a dictionary. The SSet of [Morin
2013, §1.2.4](Z-bibliography.md#ref-morin-cpp) keeps items of a total order,
and adds the *successor search* of the least item $y \ge x$, which is
meaningful even when no item equals $x$. It is Morin's `find( x )` of an SSet,
and `lowerBound( x )` in ADS, where `find( x )` keeps the meaning it has in
the USet. Order usually costs something (perhaps more than one would like):
for instance, the search trees of chapter [8](08-tree.md#ch-8) answer a
successor search in $O( \log n )$ time, while a hash table gives the `find()`
of a USet in $O( 1 )$ expected time (chapter [7](07-dictionary.md#ch-7)); a
program had better use an SSet only when it needs the order.

In `ADS/set/SetInterfaces.h`, the concepts `USet` and `SSet` require these
operations, with `insert()` and `remove()` that give a `bool`, and `find()`
and `lowerBound()` that give something convertible to a pointer to a constant
item, nil when there is none; `SSet` refines `USet`, i.e., an `SSet` is also
a `USet`. The same header has two concepts of the library for the
dictionaries that show their keys as a set. A `KeyedDictionary` has the
`insert( k , v )` and the `remove( k )` of a DICTIONARY, and `findKey( k )`,
the address of the key equal to $k$ that it holds, or nil, a search that may
change the representation (as that of a splay tree does) but not the set of
the keys. An `OrderedDictionary` is a `KeyedDictionary` that also has
`lowerBoundKey( k )`, the address of the least key not smaller than $k$, or
nil. On a `KeyedDictionary`, the adaptor `DictionarySet` is a USet, and on
the second an SSet ([§10.4](10-set.md#sec-10-4)), which is how each hash
table, search tree, skip list and B-tree of the library, whose `lowerBound()`
gives a pair or an iterator rather than the address of an item, serves as a
set of Morin. Finally, a BAG is a USet that keeps all the items equal to one
another ([Morin 2013, exercise 1.5](Z-bibliography.md#ref-morin-cpp)). Its
`insert( x )` always adds $x$, `find( x )` gives one of the items equal to
$x$, and `findAll( x )` gives them all. Its realization `Bag` models `USet`,
with an `insert()` that is always true ([§1.4](01-introduction.md#sec-1-4)).

## 1.4 Reference realizations {#sec-1-4}

The simplest realizations of the USet and of the SSet keep
the items in a vector, as [Morin 2013, exercise
1.6](Z-bibliography.md#ref-morin-cpp) suggests. They are
rather "slow", but short enough to be evidently correct, and
the tests use them as oracles for the faster ones.
`ArrayUSet` keeps the $n$ items in the places $0 , \ldots , n
- 1$ of a vector $A$, in no order, with no two of them equal
by its equality `Eq` (`std::equal_to<>` by default). Its
`find( x )` scans $A$, `insert( x )` appends $x$ after a
`find()` has ruled it out, and `remove( x )` moves the last
item into the place of the item it removes, which keeps $A$
without gaps ([Figure 1.2](01-introduction.md#fig-1-2) and
[Algorithm 1.2](01-introduction.md#alg-1-2)).

![Figure 1.2. An `ArrayUSet` of five items, and `remove(3)`: the scan reads the places 0 and 1 (in violet), and the last item, 8, moves into the place 1 (in orange); the free places are in the faintest tint.](../figures/intro-uset.svg){#fig-1-2}

<span id="alg-1-2"></span>**Algorithm 1.2.** The removal from an
`ArrayUSet` and the insertion into an `ArraySSet` ([Morin 2013, exercises
1.6 and 1.7](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/SetInterfaces-ArrayUSet-remove.pseudo.md"

--8<-- "generated/SetInterfaces-ArraySSet-insert.pseudo.md"

In the code, we move the last item instead of copying it, and we do
not move it at all when the item removed is the last one.

`ArraySSet`, instead, keeps the items in strictly increasing order by the
order `C` (`std::less<>` by default), two items being equal when neither is
smaller than the other. This is the invariant that its contracts check at the
level 3 ([§1.7](01-introduction.md#sec-1-7)), and both classes give their
vector, read-only, by `items()`. Hence we can make `lowerBound( x )` a binary
search, which [Morin 2013, exercise 1.7](Z-bibliography.md#ref-morin-cpp)
suggests among the improvements of the realization on a vector, and
`find( x )` is a `lowerBound( x )` followed by a test of equality. We search
for the place of an insertion (or of a removal) in the same way. Then we move
the items after that place by one place, as a single block, which is the other
improvement that the exercise suggests ([Figure
1.3](01-introduction.md#fig-1-3)); the `insert()` of `std::vector` makes the
move, and it becomes one `memmove()` when the items are trivially copyable.

![Figure 1.3. An `ArraySSet` of six items, and `insert(5)`: the binary search probes the places 3, 1 and 2 (in violet, in this order), the items 7, 8 and 9 move by one place as one block, and 5 is written in the place 3 (in orange).](../figures/intro-sset.svg){#fig-1-3}

<span id="lem-1-3"></span>**Lemma 1.3.** *On a set of $n$ items,
`ArrayUSet` takes $O( n )$ time for `find()`, `insert()` and `remove()`, and
`remove()` takes $O( 1 )$ time after its scan. `ArraySSet` takes at most
$\lceil \log_2 ( n + 1 ) \rceil$ comparisons for `lowerBound()` and
`find()` (one more for the test of equality), and $O( \log n + n - i )$
time for an `insert()` or a `remove()` at the place $i$. Each of the two
takes $\Theta( n )$ space, and $O( 1 )$ amortized time for the growth of its
vector.*

*Proof.* In `ArrayUSet`, `find()` compares $x$ with at most the $n$ items
of $A$, `insert()` makes one `find()` and one append, and `remove()` makes
one `find()` and then moves at most one item and shortens $A$ by one, in
$O( 1 )$ time. In `ArraySSet`, `lowerBound()` is a binary search, which
keeps an interval of candidate places halved at each comparison
([Lemma 1.5](01-introduction.md#lem-1-5)), and `find()` adds one test of
equality; an `insert()` or a `remove()` at the place $i$ makes one binary
search and moves the $n - i$ items after the place (fewer by one for a
removal), as one block. Also, the vector is a
`std::vector`, whose growth by a constant factor costs $O( 1 )$ amortized
time per insertion ([Lemma 3.3](03-stack.md#lem-3-3)), and which never
shrinks on a removal. $\square$

A BAG is a USet of groups, as [Morin 2013, exercise
1.5](Z-bibliography.md#ref-morin-cpp) suggests. `Bag` keeps
the number $n$ of its items and a hash table $G$ (a
`FlatHashTable`, chapter [7](07-dictionary.md#ch-7)) from an
item to the vector of the items equal to it, in the order of
their insertions. Its invariant is that no group is empty and
that the groups hold $n$ items in all. An `insert( x )`
appends $x$ to the group of $x$, or makes a new group
$\langle x \rangle$ when there is none. A `remove( x )`
removes the last item of the group, and the group itself when
the item was its only one, and `find( x )` gives the first
item of the group, and `findAll( x )` a copy of the group
([Figure 1.4](01-introduction.md#fig-1-4) and [Algorithm
1.3](01-introduction.md#alg-1-3)). Also, the class gives
`count( x )`, the size of the group, and `distinct()`, the
number of groups, besides `size()` and `empty()`, which take
$O( 1 )$ time.

![Figure 1.4. A `Bag` of five items in three groups, before and after `insert(b3)`: the place of the table that the search reads (in violet) gives the group of `b`, to which `b3` is appended (in orange); the free places of the table are in the faintest tint.](../figures/intro-bag.svg){#fig-1-4}

<span id="alg-1-3"></span>**Algorithm 1.3.** The operations of `Bag`, with
$G$ the table of the groups ([Morin 2013, exercise
1.5](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/Bag-insert.pseudo.md"

--8<-- "generated/Bag-remove.pseudo.md"

<span id="lem-1-4"></span>**Lemma 1.4.** *If the operations of the table
$G$ take $O( 1 )$ expected amortized time, then `insert()` and `remove()`
of `Bag` take $O( 1 )$ expected amortized time, `find()` and `count()`
$O( 1 )$ expected time, and `findAll( x )` $O( 1 + c )$ expected time,
where $c$ is the number of items equal to $x$; the space is $O( n )$. After
each operation no group is empty, and the groups hold $n$ items in all.*

*Proof.* The invariant holds on the empty bag; an `insert()` adds one item
to a group, or a new group of one item, and increments $n$, while a
`remove()` that finds a group removes one of its items, or the group
itself when it has one item only, and decrements $n$. Each operation makes
one search in $G$. Besides, `insert()` and
`remove()` make at most one insertion or removal in $G$, and an append or a
removal at the end of one vector, whose amortized cost is $O( 1 )$ ([Lemma
3.3](03-stack.md#lem-3-3)); `findAll()` copies $c$ items. Finally, the groups
hold $n$ items, and there are at most $n$ of them, each with its key in $G$.
$\square$

The hypothesis of [Lemma 1.4](01-introduction.md#lem-1-4) is that of the
hash tables of chapter [7](07-dictionary.md#ch-7), where it is stated with
its assumptions on the hash function. We remark that a group keeps all the
items equal to $x$, not a counter, since equal items need not be identical
(in the test of the exercise they are records equal by their key), which
sets `Bag` apart from the multiset of counters `HashMultiset` of chapter
[10](10-set.md#ch-10).

In `ADS/set/SetInterfaces.h`, the declarations of `ArrayUSet` and
`ArraySSet` that a user calls are

```cpp
// declarations of <ADS/set/SetInterfaces.h>
template< class T , class Eq = std::equal_to<> >
class ArrayUSet
{
 public:
 using Item = T;
 explicit ArrayUSet( Eq eq = Eq() );
 std::size_t size( void ) const;
 const T * find( const T & x ) const;
 bool insert( const T & x );
 bool remove( const T & x );
 const std::vector< T > & items( void ) const;
 };
template< class T , class C = std::less<> >
class ArraySSet
{
 public:
 using Item = T;
 explicit ArraySSet( C less = C() );
 std::size_t size( void ) const;
 const T * find( const T & x ) const;
 const T * lowerBound( const T & x ) const;
 bool insert( const T & x );
 bool remove( const T & x );
 const std::vector< T > & items( void ) const;
 };
```

In `ADS/set/Bag.h`, the declarations of `Bag` that a user calls are

```cpp
// declarations of <ADS/set/Bag.h>
template< class T , class H = DefaultHash< T > ,
          class Eq = std::equal_to< T > >
class Bag
{
 public:
 using Item = T;
 explicit Bag( H h = H() , Eq eq = Eq() );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::size_t distinct( void ) const;
 bool insert( const T & x );
 bool remove( const T & x );
 const T * find( const T & x ) const;
 std::vector< T > findAll( const T & x ) const;
 std::size_t count( const T & x ) const;
 };
```

## 1.5 Mathematical background {#sec-1-5}

We recall here the notation and the few facts that the analyses of the
following chapters use, while the textbooks of [Graham, Knuth and Patashnik
1994](Z-bibliography.md#ref-graham1994) and of [Lehman, Leighton and Meyer
2018](Z-bibliography.md#ref-lehman2018) treat them at length, and [Knuth
1997, §1.2](Z-bibliography.md#ref-knuth1997) gathers those of the analysis
of algorithms.

### 1.5.1 Exponentials and logarithms {#sec-1-5-1}

For a real $b > 0$ and an integer $x > 0$, we denote by $b^x$ the product
of $x$ factors equal to $b$, while $b^0 = 1$ and $b^{-x} = 1 / b^x$; for a real
$x$, $b^x = e^{x \ln b}$, by the exponential function $e^x$. For $b > 1$
and $k > 0$, the logarithm $\log_b k$ is the unique real $x$ such that $b^x
= k$. Two manipulations recur in the analyses, i.e., the removal of a
logarithm from an exponent and the change of its base,

$$
b^{\log_b k} = k , \qquad \log_b k = \frac{\log_a k}{\log_a b} .
\tag{1.7}
$$

In this manual we write $\log k$ for the binary logarithm $\log_2 k$, and
$\ln k$ for the natural one, $\log_e k$, where $e = \lim_{n \to \infty} ( 1 +
1 / n )^n \approx 2.71828$; by (1.7), $\ln k = ( \ln 2 ) \log k \approx 0.693
\log k$. The natural logarithm appears as the integral $\int_1^k \mathrm{d}x
/ x = \ln k$, and the binary one as a number of halvings. Indeed, $\lceil
\log k \rceil$ is the least number of times that $k$ must be halved (rounding
up) to reach 1, and $\lfloor \log k \rfloor + 1$ is the number of bits of the
integer $k \ge 1$. On a word of the machine the latter is one instruction,
which C++20 gives as `std::bit_width( k )`. In ADS it is used wherever one
needs $\lfloor \log k \rfloor$ of an integer, e.g., in the codes of the
integers and in the rank and select of chapters [21](21-compression.md#ch-21)
and [22](22-succinct.md#ch-22).

<span id="lem-1-5"></span>**Lemma 1.5.** *A search that keeps a set of $a
\ge 1$ candidate answers and replaces it, after each comparison, with a set
of at most $\lceil a / 2 \rceil$ of them ends after at most $\lceil \log a
\rceil$ comparisons. Hence the binary search for the place of an item among
$n$ sorted items, which has $a = n + 1$ answers, takes at most $\lceil \log
( n + 1 ) \rceil$ comparisons.*

*Proof.* After $j$ comparisons at most $\lceil a / 2^j \rceil$ candidates
remain, by induction on $j$, since $\lceil \lceil a / 2^j \rceil / 2 \rceil
= \lceil a / 2^{j+1} \rceil$; for $j = \lceil \log a \rceil$ one has $a
\le 2^j$, and hence a single candidate, after which the search makes no more
comparisons. In fact, the search of `std::lower_bound()`, which `ArraySSet`
calls, keeps $c$ places, i.e., $a = c + 1$ answers, makes one comparison
while $c > 0$, and replaces them with $\lfloor c / 2 \rfloor$ or $c -
\lfloor c / 2 \rfloor - 1 \le \lfloor c / 2 \rfloor$ places, i.e., with at
most $\lfloor c / 2 \rfloor + 1 = \lceil a / 2 \rceil$ answers. $\square$

### 1.5.2 Factorials and binomial coefficients {#sec-1-5-2}

For an integer $n \ge 0$, $n! = 1 \cdot 2 \cdots n$ (with $0! = 1$) is the
number of the permutations of $n$ distinct items. Stirling's approximation,
with the bounds of [Robbins 1955](Z-bibliography.md#ref-robbins1955), is

$$
n! = \sqrt{2 \pi n} \left( \frac{n}{e} \right)^n e^{\alpha( n )} , \qquad
\frac{1}{12 n + 1} < \alpha( n ) < \frac{1}{12 n} , \tag{1.8}
$$

for $n \ge 1$. Hence $\ln( n! ) = n \ln n - n + \frac{1}{2} \ln( 2 \pi n ) +
\alpha( n )$, and $\log( n! ) = n \log n - n \log e + O( \log n )$, the form
that gives the lower bound on the comparisons of a sort. A sort by
comparisons must tell apart the $n!$ orders of its input, and each
comparison has two outcomes; hence it needs at least
$\lceil \log( n! ) \rceil$ comparisons in the worst case, which
[Theorem 12.1](12-sort.md#thm-12-1) proves by the comparison tree of the
sort. For integers $0 \le k \le n$, the binomial coefficient

$$
\binom{n}{k} = \frac{n!}{k! ( n - k )!} \tag{1.9}
$$

is the number of the subsets of $k$ items of a set of $n$ items (a
"small" number only when $k$ or $n - k$ is "small"), and our analyses use
the identity $\binom{n}{k} = \binom{n-1}{k-1} + \binom{n-1}{k}$
(the subsets with the item $n$ and those without it), together with $k
\binom{n}{k} = n \binom{n-1}{k-1}$ and $\sum_{k=0}^n \binom{n}{k} = 2^n$.
They also use the bounds $( n / k )^k \le \binom{n}{k} \le ( e n / k )^k$
for $1 \le k \le n$ ([Graham, Knuth and Patashnik 1994,
§5.1](Z-bibliography.md#ref-graham1994)). The functions of the library that
compute these numbers exactly, on integers of any length, are in chapter
[17](17-numeric.md#ch-17).

### 1.5.3 Asymptotic notation {#sec-1-5-3}

The running time of an operation depends on the machine, on the compiler
and on the run, and even its count of instructions is hard to compute
exactly. We therefore bound it up to a constant factor, by the
asymptotic notation of [Knuth 1976](Z-bibliography.md#ref-knuth1976). For a
function $f( n )$, one has

$$
O( f( n ) ) = \{ g( n ) : \text{there are } c > 0 \text{ and } n_0
\text{ such that } g( n ) \le c f( n ) \text{ for all } n \ge n_0 \} ,
\tag{1.10}
$$

and $\Omega( f( n ) )$ is defined in the same way with $g( n ) \ge c f( n
)$, while $\Theta( f( n ) ) = O( f( n ) ) \cap \Omega( f( n ) )$, and $o( f(
n ) )$ is the set of the $g( n )$ such that $g( n ) / f( n ) \to 0$. As
usual, we write $g( n ) = O( f( n ) )$ for $g( n ) \in O( f( n ) )$, and an
expression such as $T( n ) = 2 \log n + O( 1 )$ means that $T( n ) - 2 \log
n$ is a function of $O( 1 )$. For instance, $3 n \log n + 10 n + 50 = O( n
\log n )$, since for $n \ge 2$ one has $\log n \ge 1$ and $n \log n \ge 2$,
and hence $3 n \log n + 10 n + 50 \le 3 n \log n + 10 n \log n + 25 n \log n =
38 n \log n$; (1.10) thus holds with $c = 38$ and $n_0 = 2$. The inclusions

$$
O( 1 ) \subset O( \log n ) \subset O( n^b ) \subset O( n^b \log n )
\subset O( n^{b'} ) \subset O( c^n ) , \tag{1.11}
$$

for constants $0 < b < b'$ and $c > 1$, compare the usual costs.

<span id="lem-1-6"></span>**Lemma 1.6.** *Each inclusion of (1.11) is
strict, and it still holds, strictly, when both of its sides are
multiplied by the same positive function $h( n )$ (e.g., $h( n ) = n$ gives
$O( n ) \subset O( n \log n )$).*

*Proof.* For $\varepsilon > 0$ and $x \ge 1$ one has $\ln x = ( \ln
x^\varepsilon ) / \varepsilon \le ( x^\varepsilon - 1 ) / \varepsilon <
x^\varepsilon / \varepsilon$, since $\ln y \le y - 1$ for $y > 0$; hence
$\log n / n^\varepsilon < 1 / ( \varepsilon \ln 2 )$ for $n \ge 1$, and
by the same bound with $\varepsilon / 2$ the ratio tends to 0. Now let
$f_1 , f_2$ be two consecutive functions of (1.11), say $f_1( n ) = n^b
\log n$ and $f_2( n ) = n^{b'}$. We show that $f_1( n ) / f_2( n ) \to
0$, which gives both $f_1 = O( f_2 )$ (hence $O( f_1 ) \subseteq O( f_2 )$,
since $O( \cdot )$ is transitive) and $f_2 \notin O( f_1 )$. The ratios
are $1 / \log n$, $\log n / n^b$, $1 / \log n$ and $\log n /
n^{b' - b}$, which tend to 0 by the bound above with $\varepsilon = b$ and
$\varepsilon = b' - b$; the last one is $n^{b'} / c^n = 2^{b' \log n - n
\log c}$, whose exponent tends to $-\infty$, since $b' \log n / n \to
0$. A positive factor $h( n )$ common to $f_1$ and $f_2$ leaves their ratio
unchanged. $\square$

With more than one variable, as in the $O( n +
k )$ space of $k$ stacks of $n$ items in all ([§3.4](03-stack.md#sec-3-4)),
$g = O( f )$ means that there are $c > 0$ and $z$ such that $g( n_1 ,
\ldots , n_k ) \le c f( n_1 , \ldots , n_k )$ whenever $g( n_1 , \ldots ,
n_k ) \ge z$, which agrees with (1.10) when $f$ increases, while other texts
define it otherwise ([Morin 2013, §1.3.3](Z-bibliography.md#ref-morin-cpp)).

Two costs of the same order cannot be ranked by their orders, since the
constants hidden by (1.10) depend on the machine. Two costs of different
orders can, but only for $n$ "large" enough, and the threshold may matter in
practice. [Figure 1.5](01-introduction.md#fig-1-5) compares $50 n \log n$,
the count of a sort of order $n \log n$ with a "large" constant, with $n^2$,
that of a quadratic sort with a small one. Here the quadratic sort is the
faster up to $n \approx 439$ (where $n = 50 \log n$), and the slower beyond.
This is why the quicksorts of chapter [12](12-sort.md#ch-12) switch to the
insertion sort on the "short" ranges, where its "small" constant usually
wins.

![Figure 1.5. The functions $50 n \log n$ and $n^2$ for $n \le 1000$: $n^2$ (in violet) is the smaller up to $n \approx 439$, where they cross, and the larger beyond, by a margin that grows with $n$.](../figures/intro-growth.svg){#fig-1-5}

### 1.5.4 Randomization and probability {#sec-1-5-4}

We call *randomized* the structures of the library that make random choices
that depend neither on the items they store nor on the operations they
receive (the levels of a skip list, the priorities of a treap, the function
of a universal hash), and the running time of an operation is then a random
variable. For a discrete random variable $X$ on a countable set $U$, its
*expected value* is

$$
\mathrm{E}[ X ] = \sum_{x \in U} x \Pr\{ X = x \} . \tag{1.12}
$$

In the analyses of this manual we take the probabilities over the random
choices of the structure, never over its input, unless the contrary is said
(as for the average cases of chapter [12](12-sort.md#ch-12)). The property
that the analyses use most is the *linearity of expectation*, i.e.,
$\mathrm{E}[ X_1 + \cdots + X_k ] = \mathrm{E}[ X_1 ] + \cdots + \mathrm{E}[
X_k ]$ for any random variables, independent or not ([Motwani and Raghavan
1995, §1.3](Z-bibliography.md#ref-motwani1995)). It is often applied to a
count written as a sum of *indicator variables*, each one 1 when an event
happens and 0 otherwise, whose expected value is the probability of its
event.

<span id="lem-1-7"></span>**Lemma 1.7.** *Let the $n$ distinct items of a
vector be in an order drawn uniformly among the $n!$ orders, and let a scan
keep the least item seen so far. The expected number of times that the
least item changes (the first item included) is the harmonic number $H_n =
1 + 1/2 + \cdots + 1/n$, and $\ln( n + 1 ) \le H_n \le 1 + \ln n$.*

*Proof.* Let $I_i$ be 1 when the item of the place $i$ is smaller than all
the items before it (the places counted from 1, so that $I_1 = 1$). In a
uniform order the relative order of the first $i$ items is uniform, and
their least is in any of the $i$ places with the same probability; hence
$\mathrm{E}[ I_i ] = \Pr\{ I_i = 1 \} = 1 / i$, and by the linearity of
expectation the number of changes $X = I_1 + \cdots + I_n$ has $\mathrm{E}[
X ] = H_n$. The bounds follow by summing $\int_k^{k+1} \mathrm{d}x / x \le 1
/ k$ for $k = 1 , \ldots , n$, which gives $\ln( n + 1 ) \le H_n$, and $1 /
k \le \int_{k-1}^k \mathrm{d}x / x$ for $k = 2 , \ldots , n$, which gives
$H_n - 1 \le \ln n$. $\square$

The proof needs neither the distribution of $X$ nor the independence of the
$I_i$ (which happen to be independent), and the same device also bounds the
expected depth of a node of a treap and the expected cost of a randomized
quicksort (chapters [8](08-tree.md#ch-8) and [12](12-sort.md#ch-12)). A bound
that holds *with high probability* holds with probability at least $1 -
n^{-c}$ for a constant $c > 0$, and it is stronger than a bound on the
expected value (when, as usual, the cost is also bounded in the worst case).
In ADS, the random choices come from the generators of
`ADS/numeric/RandomGenerators.h` (chapter [17](17-numeric.md#ch-17)), seeded
by the caller, and a run can thus be repeated exactly; the tests fix their
seeds.

## 1.6 The model of computation {#sec-1-6}

We count the costs in the *word-RAM* with words of $w$ bits ([Hagerup
1998](Z-bibliography.md#ref-hagerup1998); [Morin 2013,
§1.4](Z-bibliography.md#ref-morin-cpp)). This is a random access machine
([Cook and Reckhow 1973](Z-bibliography.md#ref-cook1973)) whose memory is a
sequence of cells of one word each, any of which is read or written in
constant time. All the operations on words take constant time, i.e., the
arithmetic ($+$, $-$, $\cdot$, the division and the remainder), the
comparisons, the bitwise operations (and, or, exclusive or, negation) and the
shifts. We assume that a block of $k$ words is allocated (and initialized) in
$O( k )$ time, and that its address fits in one word. Our only assumption on
$w$ is $w \ge \log n$, where $n$ is the number of items of the structure at
hand, since otherwise a word could not even count the items. A model in which
$w$ grows with $n$ in this way is called *transdichotomous* ([Fredman and
Willard 1993](Z-bibliography.md#ref-fredman1993)). The space is counted in
words, and an item counts as one word. In ADS an item is of any type `T`, and
the costs count each copy, move or comparison of an item as one operation.
Thus, the cost of a structure of strings, say, is its count of operations
multiplied by the cost of one operation on two strings.

On the machines that the library targets $w = 64$, and the model usually fits
them rather well. There is one exception, which the library meets quite
often: the product of two words is a number of $2 w$ bits, which the
processors give in one instruction and C++20 does not (the integers of 128
bits are an extension of GCC and Clang). In ADS, the header `ADS/Portable.h`
hides the difference behind three tools in the namespace `ADS::detail`, out
of the API reference. These are `wideProduct( a , b , hi )`, the product of
two words of 64 bits, whose low word it returns and whose high word it writes
in `hi`, `byteSwap( x )`, the 8 bytes of a word in the reverse order, and the
signed integer `Int128`. The product is that of `unsigned __int128` where the
compiler has it, the intrinsic of MSVC on x64 and ARM64 otherwise, and a
computation on halves of 32 bits elsewhere (and in a constant expression of
MSVC), which is also `wideProductHalves( a , b , hi )`. The swap is the builtin of GCC and Clang, else the `std::byteswap()`
of C++23, else the intrinsic of MSVC, and else three exchanges by masks (of
the bytes, of the pairs of bytes and of the halves of the word). With $a =
a_1 2^{32} + a_0$ and $b = b_1 2^{32} + b_0$, where $0 \le a_0 , a_1 , b_0 ,
b_1 < 2^{32}$, one has

$$
a b = a_1 b_1 2^{64} + ( a_0 b_1 + a_1 b_0 ) 2^{32} + a_0 b_0 , \tag{1.13}
$$

i.e., four products of 32 bits, each of which fits in a word
([Figure 1.6](01-introduction.md#fig-1-6) and
[Algorithm 1.4](01-introduction.md#alg-1-4)).

<span id="alg-1-4"></span>**Algorithm 1.4.** The product of two words by
the products of their halves (after [Knuth 1998, Algorithm
4.3.1M](Z-bibliography.md#ref-knuth1998v2)).

--8<-- "generated/Portable-wideProductHalves.pseudo.md"

![Figure 1.6. The product of two words on halves of 32 bits, as in (1.13): the four partial products at their shifts, the three halves summed into $\mathit{mid}$ in violet, and the two halves of $\mathit{mid}$ in orange: its low half, the bits 32 to 63 of the product, and its high half, the carry into the high word.](../figures/intro-product.svg){#fig-1-6}

<span id="lem-1-8"></span>**Lemma 1.8.** *The computation of
`wideProductHalves()` ([Algorithm 1.4](01-introduction.md#alg-1-4)) gives the product $a b$ exactly, i.e., its low
word and its high word, with no overflow of an intermediate word.*

*Proof.* Each partial product $p_{ij} = a_i b_j$ is below $( 2^{32} - 1 )^2
< 2^{64}$; write $p_{ij} = h_{ij} 2^{32} + l_{ij}$ with $0 \le h_{ij} ,
l_{ij} < 2^{32}$. The middle sum $\mathit{mid} = h_{00} + l_{01} + l_{10}$
adds three numbers below $2^{32}$, and is thus below $3 \cdot 2^{32} <
2^{64}$; write it as $\mathit{mid} = m_1 2^{32} + m_0$, with $0 \le m_0 <
2^{32}$. Substituting in (1.13), one has
$$
a b = ( p_{11} + h_{01} + h_{10} + m_1 ) 2^{64} + m_0 2^{32} + l_{00} ,
$$
where $m_0 2^{32} + l_{00} < 2^{64}$; hence $m_0 2^{32} + l_{00}$, which
line 5 returns, is the low word of $a b$, and $p_{11} + h_{01} + h_{10} +
m_1$, which line 4 writes in $\mathit{hi}$, is $\lfloor a
b / 2^{64} \rfloor < 2^{64}$, the high word. Since its terms are not
negative, each partial sum of the high word is below $2^{64}$ as well, and
no intermediate word overflows. $\square$

Where the compiler has no `__int128`, the signed integer of 128 bits is a
pair of words in two's complement. It is built from any integer by extending
its sign, and it has the sums, the differences and the products of `__int128`
(those modulo $2^{128}$, exact when the result fits), its equality and its
order (the high words compared as signed and then the low ones as unsigned),
and an explicit conversion to `long double`.

<span id="lem-1-9"></span>**Lemma 1.9.** *Let $a = a_h 2^{64} + a_l$ and
$b = b_h 2^{64} + b_l$, with $0 \le a_h , a_l , b_h , b_l < 2^{64}$, be
the two pairs of words of two values of `Int128`, which stand for $a$ and
$b$ minus $2^{128}$ when the bit 63 of the high word is set. The low word of
$a + b$ is $r_l = ( a_l + b_l ) \bmod 2^{64}$, and the carry into the high
word is 1 if and only if $r_l < a_l$; the low word of $a - b$ is $( a_l -
b_l ) \bmod 2^{64}$, with a borrow if and only if $a_l < b_l$; and
$a b \bmod 2^{128} = a_l b_l + ( ( a_l b_h + a_h b_l ) \bmod 2^{64} )
2^{64} \bmod 2^{128}$. Moreover, the order of the high words read as
signed words, and then of the low words read as unsigned ones, is the order
of the values.*

*Proof.* Since $0 \le a_l + b_l < 2^{65}$, the sum of the low words is
either below $2^{64}$, and then $r_l = a_l + b_l \ge a_l$, or at least
$2^{64}$, and then $r_l = a_l + b_l - 2^{64} < a_l$, since $b_l < 2^{64}$;
the carry is 1 exactly in the second case. In the same way, $a_l - b_l$ is
negative, and is then increased by $2^{64}$, exactly when $a_l < b_l$. As
for the product, $a b = a_l b_l + ( a_l b_h + a_h b_l ) 2^{64} + a_h b_h
2^{128}$, and modulo $2^{128}$ the last term vanishes, while of the middle
one only $( a_l b_h + a_h b_l ) \bmod 2^{64}$ counts; the low word and the
part of $a_l b_l$ beyond it are those of `wideProduct()`
([Lemma 1.8](01-introduction.md#lem-1-8) for its portable form). Finally,
the value of a pair is $\hat{a}_h 2^{64} + a_l$, with $\hat{a}_h$ the high
word read as a signed word, and $0 \le a_l < 2^{64}$. If $\hat{a}_h <
\hat{b}_h$, then the value of $a$ is at most $\hat{b}_h 2^{64} - 2^{64} +
a_l < \hat{b}_h 2^{64} \le$ the value of $b$; if the high words are equal,
the order is that of the low words. $\square$

The sums, the differences and the products are thus those of `__int128`,
i.e., modulo $2^{128}$ and exact when the result fits. The random
queue of chapter [4](04-queue.md#ch-4) uses `wideProduct()` (for its draw of
an index), and the integers of any length of chapter
[17](17-numeric.md#ch-17) use it, or `wideProductHalves()` under
`ADS_BIGINT_PORTABLE`, for their products of digits; the learned index of chapter [6](06-search.md#ch-6) and the branch
and bound of chapter [20](20-complexity.md#ch-20) use `Int128`, and the sorts
of strings of chapter [12](12-sort.md#ch-12) and the search of several
patterns of chapter [15](15-string.md#ch-15) use `byteSwap()`.

The word-RAM ignores the memory hierarchy, which (as
[§1.1](01-introduction.md#sec-1-1) recalls) may make two equal
counts of operations differ by one order of magnitude in time.
Instead, the *external-memory model* of [Aggarwal and Vitter
1988](Z-bibliography.md#ref-aggarwal1988) counts the transfers of
blocks of $B$ words between a memory of $M$ words and a disk.
Furthermore, the *cache-oblivious* model ([Frigo et al.
2012](Z-bibliography.md#ref-frigo2012)) requires algorithms that are
efficient in that model without knowing $B$ and $M$. Chapter
[23](23-external.md#ch-23) uses both, and the engineered structures
of the other chapters cite them where their design follows them,
while the models of the parallel computations (work and span, the
shared memory and the passing of messages) are introduced in
chapters [24](24-concurrent.md#ch-24) to
[26](26-distributed.md#ch-26).

## 1.7 Correctness, time and space {#sec-1-7}

Three properties of a realization matter ([Morin 2013,
§1.5](Z-bibliography.md#ref-morin-cpp)): the first is *correctness*, i.e.,
that it realizes its interface, the second is the *time* of its operations,
and the third its *space*, both of which should be as small as possible. Of
course, the three may conflict, since a structure may be faster in more
space, or smaller and slower. Some structures (e.g., the filters of chapter
[10](10-set.md#ch-10)) even trade a controlled probability of a wrong answer
for space. Apart from those, we take correctness for granted in the analyses,
and the library checks it at run time by contracts.

### 1.7.1 Correctness and contracts {#sec-1-7-1}

A *contract* of an operation $\mathit{op}$ is a pair of a precondition $P$
and a postcondition $Q$, written $\{ P \}\ \mathit{op}\ \{ Q \}$ after [Hoare
1969](Z-bibliography.md#ref-hoare1969). Here the caller ensures $P$, and the
operation, under $P$, ensures $Q$. A structure also has an *invariant* $I$, a
property of its representation that each operation which changes the
structure may assume at its start and must restore at its end, i.e., $\{ P
\land I \}\ \mathit{op}\ \{ Q \land I \}$. In the *design by contract* of
[Meyer 1992](Z-bibliography.md#ref-meyer1992), a violated precondition is a
defect of the caller, and a violated postcondition or invariant a defect of
the operation. In ADS, the header `ADS/Contract.h` checks the contracts at
run time by four macros. They mirror the `pre`, `post` and `contract_assert`
of the contracts of C++26 ([Berne et al.
2025](Z-bibliography.md#ref-berne2025)), by which they can be replaced when
the compilers have them. Among them, `ADS_PRE( c )` states a precondition,
`ADS_POST( c )` a postcondition, `ADS_INVARIANT( c )` the invariant of a
structure, and `ADS_AUDIT( c )` a precondition or a postcondition that costs
more than the operation (e.g., that a vector is sorted, before a binary
search).

Checking a contract costs time, and the costs may differ by orders of
magnitude. For instance, $\mathrm{empty}()$ before a $\mathrm{pop}()$ costs
$O( 1 )$, the order of a heap after an insertion $\Theta( n )$, and the order
of a vector before a binary search $\Theta( n )$ against the $O( \log n )$ of
the search. The library therefore sorts its checks by their cost with respect
to the operation, and the macro `ADS_CONTRACTS` chooses which are evaluated
([Table 1.2](01-introduction.md#tab-1-2)).

<span id="tab-1-2"></span>**Table 1.2.** The contracts of `Contract.h` and
the level of `ADS_CONTRACTS` from which each is evaluated; the default level
is 1, and 0 when `NDEBUG` is defined (as for `assert`).

| Macro | Contract | Its cost | Level |
|---|---|---|---|
| `ADS_PRE` | a precondition | at most that of the operation | 1 |
| `ADS_POST` | a postcondition | at most that of the operation | 2 |
| `ADS_INVARIANT` | the invariant of a structure | usually $\Theta( n )$ | 3 |
| `ADS_AUDIT` | a precondition or a postcondition | more than the operation | 3 |

<span id="lem-1-10"></span>**Lemma 1.10.** *A check of cost $g( n )$ added to
an operation of cost $t( n )$ makes it cost $O( t( n ) + g( n ) )$, which is
$O( t( n ) )$ if and only if $g( n ) = O( t( n ) )$. Hence at the levels 1
and 2 the bounds of the operations hold as stated, while at the level 3 a
sequence of $k$ operations of $O( 1 )$ time that each check an invariant of
$\Theta( n )$ takes $\Theta( k n )$ time.*

*Proof.* The first claim follows from the definition (1.10), since $t( n ) +
g( n ) \le 2 \max\{ t( n ) , g( n ) \}$. The second rests on the rule of
[Table 1.2](01-introduction.md#tab-1-2), which the library follows and its tests cannot prove, i.e., a condition checked by `ADS_PRE` or `ADS_POST` costs at
most as much as its operation. The third follows from the first, with $t(
n ) = O( 1 )$ and $g( n ) = \Theta( n )$, summed over the $k$ operations.
$\square$

The level 3 is thus meant for the tests, which are all compiled at it (both
the CMake files and the makefile define `ADS_CONTRACTS=3` for them), since a
linear algorithm may become quadratic there. Each macro expands to one check,
`ADS_CONTRACT_CHECK( level , kind , cond )`, which calls `detail::violated()`
when the condition is false
([Algorithm 1.5](01-introduction.md#alg-1-5)).

<span id="alg-1-5"></span>**Algorithm 1.5.** The check of a contract, the
body of the macro `ADS_CONTRACT_CHECK`.

--8<-- "generated/Contract-check.pseudo.md"

The test of the level is an `if constexpr`. A condition is therefore compiled
at each level, and stays correct C++ (a change of the code that breaks it is
noticed at once), but it is evaluated only from its level up. Hence a
condition must have no side effects, since a program must behave in the same
way at all the levels. A violated contract throws `ADS::contract_violation`,
a `std::logic_error` whose message names the kind of contract, the condition,
the function and the place in the source; a test can thus check that a
precondition is enforced. Two helpers of the namespace `ADS::detail` serve
the conditions. The first, `equal( a , b )`, is `a == b` when the items have
an equality, and true otherwise, two items unequal to themselves (as two NaN)
counting as equal, so that a postcondition on a stored NaN holds. The second,
`snapshot( x )`, is a copy of `x`, wrapped in a `std::optional`, made only at
the level 3 (below it, `std::nullopt`), for the audits that compare a
structure with its state before the operation.

### 1.7.2 Running times {#sec-1-7-2}

We meet three kinds of bounds on the running time ([Morin 2013,
§1.5](Z-bibliography.md#ref-morin-cpp)). A *worst-case* bound $f( n )$ holds
for each single operation, which never takes longer. An *amortized* bound
$f( n )$ holds on average over each sequence of operations: any sequence of
$m$ operations that starts from an empty structure takes at most $m f( n )$
time, although some of its operations may take longer ([Tarjan
1985](Z-bibliography.md#ref-tarjan1985)). An *expected* bound $f( n )$
bounds the expected value of the running time, which is a random variable
over the choices of the structure ([§1.5.4](01-introduction.md#sec-1-5-4)).
These kinds may combine; for instance, the insertion into a hash table that is
rebuilt when it grows takes $O( 1 )$ expected amortized time (chapter
[7](07-dictionary.md#ch-7), and the `Bag` of [Lemma
1.4](01-introduction.md#lem-1-4)).

We usually prove an amortized bound by a *potential*, i.e., a function $\Phi
\ge 0$ of the state of the structure, equal to $0$ on the empty one, and
then the $i$-th operation, of actual cost $c_i$, has the *amortized cost*
$c_i + \Phi_i - \Phi_{i-1}$, where $\Phi_{i-1}$ and $\Phi_i$ are the
potentials before and after it. Consider, for instance, a vector that
starts empty with capacity $c = 0$, and doubles its capacity when it is
full (from 0 to 1 at the first insertion), with the potential $\Phi = 2 n
- c$ when it holds $n$ items.

<span id="lem-1-11"></span>**Lemma 1.11.** *If each operation of every
sequence that starts from the empty structure has amortized cost at most
$f( n )$, then any $m$ such operations cost at most $m f( n )$ in all. On
the vector that doubles, every sequence of $m$ insertions costs at most
$3 m$, counting one per item written or moved, and $n \le c \le 2 n$ after
the first insertion.*

*Proof.* Summing the amortized costs, the differences of $\Phi$
telescope, and $\sum_i c_i \le m f( n ) - \Phi_m + \Phi_0 \le m f( n )$,
since $\Phi_m \ge 0 = \Phi_0$. On the vector, $n \le c \le 2 n$ holds
after the first insertion, which leaves $n = c = 1$, and it is kept by
each insertion: one that finds $n < c$ makes $n + 1 \le c$, and one that
finds $n = c$ doubles $c$ to $2 n < 2 ( n + 1 )$; hence $\Phi \ge 0$. An
insertion that finds $n < c$ costs 1 and raises $\Phi$ by 2, i.e., it has
amortized cost 3. One that finds $n = c \ge 1$ moves the $n$ items and
writes one, at the cost $n + 1$, while $\Phi$ goes from $2 n - n = n$ to
$2 ( n + 1 ) - 2 n = 2$, i.e., the amortized cost is $n + 1 + 2 - n = 3$;
the first insertion costs 1 and leaves $\Phi = 1$. The first part, with
$f( n ) = 3$, gives the bound. $\square$

The growth of `std::vector` by any constant factor $\gamma > 1$ behaves in
the same way ([Lemma 3.3](03-stack.md#lem-3-3)). A worst-case bound is
the strongest of the three, and the other two are accepted since they often
(but not always) give "simpler" structures, or a lower cost on average, or
both. An amortized bound suits a program that needs a bound on the total
time of its operations, not one with a deadline on each response. An
expected bound, on the other hand, may be exceeded by an "unlucky" run, with
a probability that the analysis bounds.

### 1.7.3 Space {#sec-1-7-3}

We count the space of a structure in words. It comprises the space of its
items and that of the data it adds (pointers, indices, capacities, the free
places of a vector), and the latter is its *overhead*. Typically, a list
with pointers takes one or two words per item besides the items, and a
vector that doubles its capacity up to twice the words of its items
([Lemma 1.11](01-introduction.md#lem-1-11)). In chapter [2](02-sequence.md#ch-2) the
rootish and tiered arrays bring the "wasted" free places down to $O(
\sqrt{n} )$, and the blocked lists bring the overhead down to $O( b + n / b
)$, with blocks of $b$ items. On the other hand, the succinct structures of
chapter [22](22-succinct.md#ch-22) count the space in bits, against the
information-theoretic minimum $Z$ of the objects they represent, and take
$Z + o( Z )$ bits, while answering their queries in constant or almost
constant time.

## 1.8 Iterators and the standard interface {#sec-1-8}

Beside the operations of their interfaces, the containers of ADS have the
interface of the standard library, i.e., `begin()` and `end()` of the
same type, the usual member types, and iterators of the strongest
category that the representation allows. An *iterator* is a position in a
container that can be read and moved. The standard library sorts the
iterators into *categories* by the moves they allow in constant time
([Stroustrup 2013, ch. 33](Z-bibliography.md#ref-stroustrup2013)): a
forward iterator moves on, a bidirectional one also back, and a
random-access one by any number of places, while a contiguous one reads
items that lie one after the other in memory. An algorithm states the
category it needs (`std::sort()` a random-access iterator,
`std::reverse()` a bidirectional one). In ADS, the iterators of
`ADS/Iterators.h` give the containers of the library the category of
their representation. Hence the range-for, the algorithms of `std` and of
`std::ranges`, and those of Boost visit them as they visit the containers
of the standard library ([Table 1.3](01-introduction.md#tab-1-3)); the
tests `interop.*` check the category of each container at compile time
and run those algorithms on it.

<span id="tab-1-3"></span>**Table 1.3.** The iterators of `Iterators.h`,
what they walk, their categories and the cost of a step; $m$ is the number
of places of a table, $n$ the number of its occupied places and $h$ the
height of a tree.

| Iterator | Walks | Category | Cost of a step |
|---|---|---|---|
| `LinkedIterator` | cells linked by pointers | forward | $O( 1 )$ |
| `IndexIterator` | cells of a vector linked by indices | forward | $O( 1 )$ |
| `CircularIterator` | a circular vector | random access | $O( 1 )$ |
| `RankIterator` | a container read by rank, `c[ i ]` | random access | that of `c[ i ]` |
| `IndirectIterator` | a vector of pointers, read through them | random access | $O( 1 )$ |
| `SlotIterator` | the occupied places of a table | bidirectional | $O( m / n )$ amortized over a whole visit |
| `KeyIterator` | the keys of the pairs that another iterator reads | that of the other, up to bidirectional | that of the other |
| `PathIterator` | a binary tree in symmetric order, by the path from the root | bidirectional | $O( 1 )$ amortized over a whole visit, $O( h )$ at most |
| `WalkIterator` | the nodes of a tree, by a policy | forward | that of the policy |

`LinkedIterator` walks cells linked by a pointer, its template parameters
naming the members of the link and of the item, with nil as the end. It is
the iterator of the singly linked lists, queues and stacks. In turn,
`IndexIterator` walks cells of a vector linked by an index (an `int`), with
$-1$ as the end, any two ends being equal; it is the iterator of the
structures whose cells are a vector with a free list (e.g., the stacks of
`MultiStack`, [§3.4](03-stack.md#sec-3-4)). `CircularIterator` reads a
circular vector of capacity $c$ whose first item is at the place
$\mathit{head}$, where the item of index $i$, counted from the head, is at
the place

$$
( \mathit{head} + i ) \bmod c , \quad 0 \le i < n \le c . \tag{1.14}
$$

It keeps $i$ instead of the place, and its moves, its differences
and its comparisons are thus those of the integers, and the order of
the items is that of the queue instead of that of the places
([Figure 1.7](01-introduction.md#fig-1-7)). `RankIterator` is a rank
with a pointer to its container, which computes the place of a rank,
as the rootish, dual and cubish arrays of chapter
[2](02-sequence.md#ch-2) do. `IndirectIterator` reads through a
random-access iterator on pointers, smart or not, as the heaps of
handles of chapter [9](09-priority.md#ch-9) need, and `KeyIterator`
reads the member `first` of what a forward iterator reads, i.e., the
keys of a dictionary as the items of a set, with the category of
that iterator up to bidirectional. Each iterator also gives what it
keeps, i.e., `cell()` for `LinkedIterator`, `cells()` and `index()`
for `IndexIterator`, `base()`, `head()`, `capacity()` and `index()`
for `CircularIterator`, `owner()` and `rank()` for `RankIterator`,
`base()` for `IndirectIterator` and `place()` for `SlotIterator`.

![Figure 1.7. A circular vector of capacity $c = 8$ whose head is at the place 5, with the index $i$ of each item from the head; `*it`, with `it` on $i = 2$, reads the place 7, and `it[2]` the place $( 5 + 4 ) \bmod 8 = 1$ (both in violet).](../figures/intro-circular.svg){#fig-1-7}

`SlotIterator` walks the places $0 , \ldots , m - 1$ of a table, skipping
those that a policy declares empty (or deleted, for the open addressing with
lazy deletion), and reads each place through the policy. Since the static
functions `size()`, `occupied()` and `read()` of the policy take the owner
and the place, a policy nested in its owner can read its private fields
([Algorithm 1.6](01-introduction.md#alg-1-6)).

<span id="alg-1-6"></span>**Algorithm 1.6.** The steps of `SlotIterator`,
$i$ the place and $m$ the size of the table.

--8<-- "generated/Iterators-SlotIterator-next.pseudo.md"

--8<-- "generated/Iterators-SlotIterator-prev.pseudo.md"

<span id="lem-1-12"></span>**Lemma 1.12.** *On a table of $m$ places, $n
\ge 1$ of them occupied, a whole visit by `SlotIterator`, from the place 0
to the end, tests each place once, and takes $\Theta( m )$ time, i.e.,
$O( m / n )$ amortized time per item; a single step tests at most $m$
places.*

*Proof.* The constructor and each `next()` ([Algorithm
1.6](01-introduction.md#alg-1-6)) only increase $i$, by one at each test
of a place that is not occupied and by one at each step, and the visit
stops at $i = m$; hence each of the places $0 , \ldots , m - 1$ is tested
once, and the $n$ steps read $n$ items. A single step tests the places
between two consecutive occupied ones, at most $m$. $\square$

The bound is $O( 1 )$ per item when the load $n / m$ is bounded from below
by a constant, as in the hash tables that halve their table when they get
"too empty"; a single step, however, may test $\Theta( m )$ places, and
`prev()`, which tests the places backward, behaves in the same way.

`PathIterator` visits a binary tree in symmetric order keeping the
path $\pi$ from the root to the current node, the end being the
empty path. It therefore needs no link to the parent in the nodes,
and it goes both ways (a step back from the end reaching the last
node), and a policy gives the root, the children (nil for none) and
the reading of a node ([Algorithm 1.7](01-introduction.md#alg-1-7)
and [Figure 1.8](01-introduction.md#fig-1-8)).

<span id="alg-1-7"></span>**Algorithm 1.7.** The steps of `PathIterator`,
$\pi$ the path from the root to the current node.

--8<-- "generated/Iterators-PathIterator-next.pseudo.md"

--8<-- "generated/Iterators-PathIterator-prev.pseudo.md"

In the code, we write the two steps once, with the side as a parameter,
since `prev()` is `next()` with the two children exchanged.

![Figure 1.8. A `PathIterator` on a binary search tree of seven nodes: on the node 3, with the path $\pi$ from the root in violet; after one step, which pops 3 and 2 and stops on 4, the node from whose left subtree it came; after another, which pushes the right child 6 of 4 and the leftmost node 5 below it.](../figures/intro-path.svg){#fig-1-8}

<span id="lem-1-13"></span>**Lemma 1.13.** *The next node in symmetric order
is the leftmost node of the right subtree of the current node $x$ if $x$
has a right child, and otherwise the lowest ancestor of $x$ whose left
subtree contains $x$. A whole visit of a tree of $n$ nodes by `PathIterator`
costs $\Theta( n )$, i.e., $O( 1 )$ amortized per step. A single step makes
at most $h + 1$ pushes or pops, and the path takes $O( h )$ space.*

*Proof.* In symmetric order the nodes of each subtree are consecutive, those
of the left subtree of a node $z$ before $z$ and those of its right subtree
after it. If $x$ has a right child, the nodes that follow $x$ within its
subtree are those of its right subtree, the leftmost first. Otherwise $x$ is
the last node of its own subtree. It stays the last node of the subtree
of each ancestor reached going up from a right child; the first ancestor $y$
reached from a left child has $x$ as the last node of its left subtree, and
hence follows $x$ (if there is no such ancestor, $x$ is the last node of the
tree). The code of `step()` does exactly this. It descends from the right
child pushing the left path (lines 4 and 5 of `next()`), or it pops $\pi$ as
long as the node popped is the right child of the new last node of $\pi$
(lines 7 to 9), leaving $y$ last, or $\pi$ empty at the end. Hence $\pi$ is always the path from the root to the current node. In a whole visit, from
the first node to the end, each node is pushed once, when the visit first
reaches it going down from its parent (or as the root), and popped once,
when the visit leaves its subtree, and the visit therefore makes $n$ pushes and $n$ pops. A single step pushes or pops nodes of one path from the root,
at most $h + 1$ of them, and $\pi$ never holds more. $\square$

The amortized bound holds for a whole visit (or for a visit of $s$
consecutive steps, at $O( s + h )$), not for any sequence of steps, since a
sequence that goes forward and back across the root of a tall subtree pays
$\Theta( h )$ at each step. We note that the path takes $O( h )$ space,
against the $\Theta( n )$ space of the links to the parents that an iterator
without a path needs, and on a balanced tree $h = O( \log n )$. Finally,
`WalkIterator` follows the nodes of a tree (pointers or indices) by a policy,
whose `next( o , n , root )`, with `o` a pointer to the owner (nil if the
nodes need none), gives the node after $n$ in a walk below $\mathit{root}$,
or the end `none`. Such policies give the children of a node, and the
preorder and the symmetric order by the links to the parents, with no stack.

`LinkedIterator`, `IndexIterator`, `CircularIterator` and `RankIterator`
have a constant and a mutable form, by the parameter `Const`, and the
mutable one converts to the constant one. `IndirectIterator`,
`SlotIterator`, `KeyIterator` and `PathIterator` read constant items, while
`WalkIterator` gives the nodes by value. Containers that keep an invariant
on their items (a heap, an ordered set, a dictionary) expose only the
constant form, since the operations of their interfaces are then the only
"safe" way to change them, and an iterator stays valid as long as the cells
(or the vector) it refers to do. In some containers the item read is built
on the fly (a pair of references, a key rebuilt along a trie, the number of
a node). Such an iterator models the concepts of C++20, but it declares
the category of an input iterator to the older algorithms, as the views of
the standard library do; with it, one should use `std::ranges::prev` and
the algorithms of `std::ranges`. Also, the iterators are adaptors with no
contracts, and the containers check theirs.

The pair that the iterators of a dictionary read is a `PairRef< A , B >`,
two references into the dictionary (or a key rebuilt and a reference),
while the `value_type` of the iterator and of the dictionary is the pair
of the copies, `std::pair< K , V >`, as in the containers of the standard
library, which also give `key_type` and `mapped_type`. A vector of
`std::ranges::range_value_t` of a dictionary, or the one that
`std::ranges::to< std::vector >()` builds from it, thus holds copies, which
neither a rebuilding nor the destruction of the dictionary invalidates
(with a pair of references as its value type, it would hold references
into the dictionary, left dangling by either). Where C++23 declares the
common reference of a pair of references and of a pair of values,
`PairRef` is the `std::pair` itself; otherwise it is a class derived from
the pair that declares that common reference, so that the iterators model
the concepts of C++20, and it is tuple-like as the pair, for the
structured bindings, `std::views::keys` (of C++20) and `std::format`.

### 1.8.1 Boost and the standard library {#sec-1-8-1}

We now describe the headers of `ADS/interop/`. By them, a program written
on Boost or on the standard library runs the algorithms of that library on
the structures of ADS, and those of ADS on the structures of that library,
usually by changing one type. The headers of `ADS/interop/boost/` are
compiled only with `ADS_WITH_BOOST` (the component `Boost` of
`find_package(ADS)`), and no other header of the library includes them.
There is one header per library of Boost, since some of these may not share
a translation unit; for instance, `boost/range/algorithm.hpp` declares a
function `boost::sort`, which clashes with the namespace that
`boost/sort/sort.hpp` opens under the same name.

`Graph.h` specializes `boost::graph_traits` and `boost::property_map` for
`StaticGraph` and `ArrayGraph`, which then model the incidence, adjacency,
vertex-list and edge-list graphs of Boost.Graph with no copy. An edge is
the pair of its tail and of its place in the compressed rows, which is also
its `edge_index` (stable, since the graph is static); a scan of the
out-arcs is the scan of a row, and a weight is read where it lies. Hence
the visits, the algorithms of Dijkstra and of Bellman and Ford, the strong
components, the topological order and the three maximum flows of
Boost.Graph run on these graphs. (The flows of Boykov and Kolmogorov and
the push-relabel one need a separate reverse for each arc.) The tests
`engineered_interop.bgl-row-graphs` and
`.bgl-max-flows` compare their results with those on
`compressed_sparse_row_graph` and with the algorithms of ADS. Also,
`ListGraph` and `ObservedGraph` on it are mutable graphs of Boost.Graph,
whose edge is the pair ( tail , head ) and whose weight is looked up in
$O( \deg u )$ time. Their `add_edge()`, `remove_edge()`, `add_vertex()`,
`clear_vertex()` and `remove_vertex()` are the operations of the graph;
hence, a program of Boost.Graph that edits an `ObservedGraph` sends the
events of [§1.13](01-introduction.md#sec-1-13) to its observers (e.g., to a
reoptimizing observer of the shortest paths, as in
`engineered_interop.bgl-list-graphs`).

Two choices of the adapter come from the graphs of ADS. First, the
orientation of an ADS graph is a flag at run time, and therefore each graph
is seen as oriented: an unoriented graph has both arcs of each edge, which
the oriented algorithms read correctly, while the algorithms that require
`undirected_tag` do not take it. Second, the names of the nodes are stable,
and a removed vertex leaves the other vertices with their names (unlike a
`boost::adjacency_list` on `vecS`); `num_vertices()` is then `bound()`,
which gives the maps of Boost indexed by `vertex_index` a place for each
name. The free functions are in the namespace `ADS`, where
argument-dependent lookup finds them whatever the order of the includes.
Note that `edges( g )` of the two graphs in compressed rows is then that of
Boost.Graph, which is more specialized than the function of
[§14.9](14-graph.md#sec-14-9) (written `ADS::edges< G >( g )` in this
case).

The other direction is `FromBGL< BG , WeightMap >` of `FromBGL.h`, a view
of a graph of Boost.Graph with a vertex index as a `Graph` of ADS, the
weights read from a property map. The test
`engineered_interop.bgl-from-bgl` runs the shortest paths and the strong
components of ADS on it. On
a `boost::adjacency_list` on `vecS` the view is also a dynamic graph, and
an `ObservedGraph` on it changes the graph of Boost while it sends its
events; a deleted node keeps its vertex, isolated, since `remove_vertex()`
on `vecS` would rename the vertices after it. In `Dijkstra.h`,
`UpdatableQueue` makes any queue of the frontier of the shortest paths
([§14.8](14-graph.md#sec-14-8)) the buffer of
`boost::breadth_first_visit()`. Since `boost::dijkstra_shortest_paths()`
does not let the caller replace its 4-ary heap, `dijkstraShortestPaths()`
is the algorithm of Dijkstra of Boost.Graph on such a buffer, i.e., the
visit with a visitor that relaxes the arcs. `Heap.h` makes the mutable
heaps of Boost.Heap queues that lower their keys natively, by the free
`queue_decrease()` of [§1.14](01-introduction.md#sec-1-14). This function
calls `increase()`: a heap of Boost has on top the largest item in the
order of its comparator, and with `std::greater` the fall of a distance
moves the item toward the top. Finally, `Iterator.h` gives Boost the
traversal of the iterators of ADS that build their item (that of their
concept of C++20, instead of the single pass of their legacy category),
and Boost.Range then takes the dictionaries and the search trees. It does
so only for the types of the namespace `ADS`, and it must be included
before the headers of Boost that read the traversal.

With the standard library, `TieredVector`, `RootishArrayStack`, `Treque`,
`CubishArrayStack` and `DualArrayDeque` have the members by iterator of a
sequence container (`insert( p , x )`, `emplace( p , ... )`, `erase( p )`
and `erase( first , last )`), each one operation by rank at the rank of the
iterator. Hence `std::flat_set` and `std::flat_map` of C++23 keep their
keys in them. `StdSequence< S >` of `interop/std/FlatMap.h` adds the same
members to another sequence that inserts by rank, such as
`ContiguousDeque`, whose iterators are pointers (with them, a member
`insert( 0 , x )` would be ambiguous between a rank and a null pointer). On
`TieredVector`, `TieredFlatMap` keeps the binary search of a flat map and
inserts in $O( \sqrt{n} )$ time instead of the $O( n )$ of a
`std::vector`. Furthermore, `PmrResource` of `interop/std/Pmr.h` makes a
`PoolResource` ([chapter 5](05-memory.md#ch-5)) a
`std::pmr::memory_resource`, at the price of one virtual call per
allocation, and the sorts of [chapter 12](12-sort.md#ch-12) take any sized
random-access range beside `std::vector`, in place when the range is
contiguous (and, for the sorts of the book, also when it is not). With `ADS_WITH_GMP`,
`BigInteger::fromGmp()`, `toGmp()` and the constructors from `mpz_class`
and `mpq_class` (and the same members of `Rational`) pass the numbers to
and from GMP without a string. The tests are `engineered_interop.*`; with
`ADS_WITH_BOOST` off, they check only the part of the standard library.

## 1.9 The two levels of the library {#sec-1-9}

The library has two levels, written to be read and used side by side. At
the *textbook* level, ADS realizes each structure in the form of a
textbook. Our main one is [Montresor
2025](Z-bibliography.md#ref-montresor2025), whose exercises are tests of
the library, followed by [Morin 2013](Z-bibliography.md#ref-morin-cpp),
[Knuth 1997](Z-bibliography.md#ref-knuth1997), [Sedgewick
1998](Z-bibliography.md#ref-sedgewick1998) and [Cormen et al.
2022](Z-bibliography.md#ref-cormen2022) for what that book does not cover.
At this level the names of the operations, the pseudocode in the comments
and the code follow the text line by line, and the contracts state its
preconditions. Its code is written to be portable and readable rather than
"tuned". At the *engineered* level, ADS realizes the variants of the
literature on algorithm engineering ([Ferragina
2023](Z-bibliography.md#ref-ferragina2023); [Sanders et al.
2019](Z-bibliography.md#ref-sanders2019)). These keep the interface of a
textbook structure and change its representation to use the caches, the
instructions on several items at once and the cores of a machine. For
instance, the hash table with open addressing of [Montresor 2025,
§7.2.2](Z-bibliography.md#ref-montresor2025) is `OpenHashTable`. Its
engineered counterpart is `FlatHashTable`, after the tables of Boost, with
groups of 15 places and a byte of fingerprint per place, compared all at
once. Similarly, the quicksort of the book stands beside `ADS::sort`, which
is compared with the sorts of the standard library and of Boost. Each
engineered structure cites the work it follows, and the benchmarks of
`bench/` compare it with the best implementations available ([appendix
B](B-benchmarks.md#app-B)). Each chapter describes both levels, and says
what the second changes and why it is faster.

Also, the code follows a few conventions. We write each realization as one header, included by its path (e.g.,
`#include "ADS/stack/ArrayStack.h"`), which declares its classes and
functions with their contracts. At its end the header includes the
definitions of its templates, in the folder `impl/` next to it (e.g.,
`ADS/stack/impl/ArrayStack.tpp`). All the names are in the namespace `ADS`,
and the details of the implementation in `ADS::detail`, out of the API
reference. Each operation states its cost in its comment, and the operations
of the textbooks also give there their pseudocode, in the conventions of the
front matter of this manual, from which the manual extracts it. Members of
the classes are named `f_` followed by the name of the text, a `return( x );`
has its parentheses, and the code is formatted by the rules of the repository
`code-style`. In the code and in the text we always denote by $n$ the number
of items of the structure at hand.

We test the library with GCC and Clang on Linux, macOS and Windows, and with
MSVC on Windows. Besides `ADS/Portable.h`, the few dependencies on the
platform are optional backends, each used only when it is found, i.e., GMP
for the integers of any length, a CBLAS for the products of matrices, the
headers of Boost for the four structures that delegate to it where it is
found (`PointerTable`, `FlatHashTable`, `SmallVector` and the generators
xoshiro256, [§7.6](07-dictionary.md#sec-7-6)), OpenMP,
oneTBB and FastFlow for the parallel executors, and MPI for the distributed
ones. The library is also available as a module of C++20, `ads`, which
exports all the names that the headers declare in the namespace `ADS`, and
as one module for each of the sets that we describe next.

The two levels are also two sets of headers, which a program may take
apart; we divide them by the origin of the code, whatever its speed. In the
*base* set lies what follows a textbook or an academic source ([Montresor
2025](Z-bibliography.md#ref-montresor2025), [Knuth
1997](Z-bibliography.md#ref-knuth1997), [Morin
2013](Z-bibliography.md#ref-morin-cpp), and the book and the notes of
[Ferragina 2023](Z-bibliography.md#ref-ferragina2023)), even when the
result is fast. For instance, the Red-Black tree of `SearchTree.h`, the
B+-tree of `BPlusTree.h` and the hash tables of the book are base, and so
are `BigInteger` and `Rational` (which run on GMP where it is found), the
rank and select of `RankSelect.h` and the PGM-index. In the *advanced* set
lies what is written for speed beyond those sources (e.g., `UnionFind`,
`DaryHeap`, `StaticGraph`, `CompactRankSelect`, `FastBitwise` and
`MultiwayMerge`), with the concurrent and parallel structures, the
executors and the distributed layer. It also holds the typed modifications
and the observers (cf. [§1.13](01-introduction.md#sec-1-13)), which no
textbook treats. Hence a structure of a textbook and its engineered
counterpart lie in different sets (e.g., `MFSet` and `UnionFind`,
`HeapPriorityQueue` and `DaryHeap`, `ArrayGraph` and `StaticGraph`,
`DancingLinks` and `FastExactCover`). An advanced structure extends a base
one through the same concepts and by composition, i.e., by algorithms
written once on a concept and by members of the base types. We do not
derive one from the other, since a hierarchy of classes with virtual calls
would cost the speed for which the advanced set exists.

A third set, the *core*, holds the infrastructure that both sets include
(the contracts, the iterators, the primitives of `Portable.h`, the
capabilities and the concept of the executors). It also holds a few
helpers of both levels, i.e., the pool of the nodes with its huge pages,
the random hashing of the randomized structures, the checks of the sorts,
the bits of the coders and the files of records. A base header may also
use an advanced one inside (e.g., `SuffixArray.h` sorts the ranks by
`RadixSort.h`, and `ExternalSort.h` merges its runs on a `DaryHeap`). We
list each use of this kind, with its reason, in the file of the sets; the
advanced header is then compiled with the base one, but its names are not
part of the base set. Two headers still hold a part of each set
(`EngineeredQuickSorts.h` and `PriorityQueue.h`), and we put them in both
sets until we move their engineered parts to new headers, as we did for
`MaxFlow.h`, `Matching.h` and `Visit.h`. The push-relabel of the first is
now in `PushRelabel.h`, the blossoms of the second in `BlossomMatching.h`
and the components by one visit of the third in `StrongComponents.h`, each
included by the header of the book, so that an include of the latter
still gives the whole of it.

Each set has an *entry point*, a header that includes all the headers of
the set and the core. Thus `#include "ADS/base.h"` gives the base set,
`ADS/advanced.h` the advanced one, `ADS/core.h` the core and `ADS/ADS.h`
the three. Also, each header keeps its path, and one can still include any
of them as before. The modules follow the same division: `ads.core`,
`ads.base` and `ads.advanced` export the names of the headers of their sets
(the last two also export `ads.core`), and `ads` exports the three. A
program that writes `import ads;` gets all the names as before, whereas one
that writes `import ads.base;` compiles neither the concurrent structures
nor the executors of OpenMP, oneTBB and MPI. The headers of `ADS/interop/`
([§1.8.1](01-introduction.md#sec-1-8-1) and
[§14.14](14-graph.md#sec-14-14)) are a set of their own, *interop*, which
no entry point includes and no module exports, since each of them includes
the headers of another library and is included by the program that wants
it, next to them. The file `cmake/ads_sets.txt`
gives the set of each header, and from it the script `cmake/ads_sets.py`
writes the entry points and the table of the sets in the README. CMake, the
makefile and the continuous integration run the script, so a new header
with no set (or an include across the sets that the file does not allow)
stops the build. Finally, in [Table 1.5](01-introduction.md#tab-1-5) the
column Set gives the set of each header with classes, and the opening
paragraph of each chapter lists which of its headers are base and which are
advanced.

## 1.10 Building, testing and using the library {#sec-1-10}

The library is header-only, and `include/` is the whole of it. A program
that adds `include/` to its path of headers (`-I`) and is compiled as C++20
can include any header. Its tests and benchmarks are built with CMake
(`cmake -S . -B build`, `cmake --build build`, `ctest --test-dir build`) or
with the makefile (`make -j4 test`). The file `test/tests.txt` lists the
tests, one per line, each with its program, its name and its source, and
`ctest` runs each one as `<program>.<name>`, e.g., `ctest -R
ods_lists.ods-ex1.5.bag`. A test program also runs one test by name and
lists its tests, and the test `<program>.registry` of each program checks
that `tests.txt` and the tests of the program agree. An installation
(`cmake --install build --prefix <dir>`) gives a package of CMake, used by
`find_package(ADS)` and the target `ADS::ADS`, and a file of pkg-config,
while the library is also distributed by several package managers, and can
be taken at configure time by `FetchContent`. Full instructions, with the
options of the backends and of the module, are in the sections Build and
test and Use of the README of the repository
(<https://github.com/dmeoli/ADS#build-and-test>), and the API reference,
generated from the comments of the headers, is at
<https://dmeoli.github.io/ADS/>.

## 1.11 How this manual is organized {#sec-1-11}

In this manual we follow the order in which a course on data structures and
algorithms meets its subjects, in four parts and some appendices. Part I,
the foundations, has this chapter, the sequences (chapter
[2](02-sequence.md#ch-2)), the stacks (chapter [3](03-stack.md#ch-3)), the
queues and the deques (chapter [4](04-queue.md#ch-4)) and the management of
the memory (chapter [5](05-memory.md#ch-5)). Part II, on searching and
ordering, has the searches in sorted data and the learned indexes (chapter
[6](06-search.md#ch-6)), the hash tables, the tries and the structures on
integers (chapter [7](07-dictionary.md#ch-7)), and the trees (chapter
[8](08-tree.md#ch-8)). It goes on with the priority queues (chapter
[9](09-priority.md#ch-9)), the sets and the filters (chapter
[10](10-set.md#ch-10)), the disjoint sets (chapter [11](11-mfset.md#ch-11))
and the sorts (chapter [12](12-sort.md#ch-12)). Part III, on the
algorithms, has the design of algorithms (chapter
[13](13-algorithm.md#ch-13)), the graphs (chapter [14](14-graph.md#ch-14)),
the strings (chapter [15](15-string.md#ch-15)) and the geometry (chapter
[16](16-geometry.md#ch-16)), then the numerical algorithms (chapter
[17](17-numeric.md#ch-17)), the combinatorial generation (chapter
[18](18-combinatorial.md#ch-18)), the backtracking (chapter
[19](19-backtrack.md#ch-19)) and the intractable problems (chapter
[20](20-complexity.md#ch-20)). Part IV, on engineering, has the
compression (chapter [21](21-compression.md#ch-21)), the succinct
structures (chapter [22](22-succinct.md#ch-22)), the external memory
(chapter [23](23-external.md#ch-23)), the concurrency (chapter
[24](24-concurrent.md#ch-24)), the parallelism (chapter
[25](25-parallel.md#ch-25)) and the distributed computations (chapter
[26](26-distributed.md#ch-26)). Then two appendices describe how to write a
new structure ([appendix A](A-writing-structure.md#app-A)) and the
benchmarks ([appendix B](B-benchmarks.md#app-B)), and the bibliography
closes the manual.

Part I is needed by all the others, and Part II by most of Part III. In turn,
the chapters of Part IV need the structures that they compress, store or
share, and say so at their start. Each chapter covers the headers of one area
of `include/ADS`, named under its title (this one covers the headers at the
root of `include/ADS` and the interfaces of Morin), and has the same form.
For each structure it gives the abstract data type and its axioms, the
representation and its invariants, and figures of its state in memory before
and after the operations. Then come the pseudocode of the operations,
extracted from the headers, the analysis of the costs as lemmas with their
proofs, the declarations that a user calls with short examples of their
use, and the engineered variants with what they change. A section of
discussion and references and one of exercises, each solved by a test of the
library, close the chapter.

## 1.12 The map of the structures {#sec-1-12}

In [Table 1.4](01-introduction.md#tab-1-4) we give the costs of the
operations of the realizations of the LIST, of the USet and of the SSet
that correspond to those of [Morin 2013,
§1.7](Z-bibliography.md#ref-morin-cpp), as their headers state them and
their chapters prove them. [Table 1.5](01-introduction.md#tab-1-5) then
maps each class of the library to the chapter that describes it, one row
per header, and [Table 1.6](01-introduction.md#tab-1-6) lists by chapter
the headers that declare functions only. A script,
`manual/build/structmap.py`, writes the two tables from the headers
themselves. It lists a class when the class is defined at the scope of the
namespace `ADS`, or of a namespace nested in it other than `ADS::detail`,
outside the sections hidden from the API reference, and lists an alias of
such a class with it (e.g., `UnionFind`, the `DisjointSets` of the default
parameters). It also checks that each header and class of the tables
appears in this chapter. The tables leave out the concepts, the
enumerations and the aliases of plain types, which are described with their
areas, and the helper structures `AVLTreeNodes`, `JoinTreeNodes`,
`SearchTreeNodes` and `TreapNodes`, which the trees of chapter
[8](08-tree.md#ch-8) define in their files `impl/` to hold the operations
on their nodes.

<span id="tab-1-4"></span>**Table 1.4.** The costs of the realizations of the
LIST, of the USet and of the SSet that correspond to those of [Morin 2013,
§1.7](Z-bibliography.md#ref-morin-cpp); A marks an amortized bound, E an
expected one, $b$ is the size of a block, $w$ the length of a word and $t
\ge 2$ the minimum degree of a B-tree (with $t$ a constant, both bounds are $O(
\log n )$), and
for the USet and the SSet the realizations are those of the DICTIONARY on
the same representation.

| Realization | Read, write | Insert, remove | Chapter |
|---|---|---|---|
| `ArraySequence` | $O( 1 )$ | $O( 1 + n - i )$ A | [2](02-sequence.md#ch-2) |
| `RingDeque` | $O( 1 )$ | $O( 1 + \min\{ i , n - i \} )$ A | [4](04-queue.md#ch-4) |
| `DualArrayDeque` | $O( 1 )$ | $O( 1 + \min\{ i , n - i \} )$ A | [2](02-sequence.md#ch-2) |
| `RootishArrayStack` | $O( 1 )$ | $O( 1 + n - i )$ A | [2](02-sequence.md#ch-2) |
| `List` | $O( 1 + \min\{ i , n - i \} )$ | $O( 1 + \min\{ i , n - i \} )$ | [2](02-sequence.md#ch-2) |
| `SEList` | $O( 1 + \min\{ i , n - i \} / b )$ | $O( b + \min\{ i , n - i \} / b )$ A | [2](02-sequence.md#ch-2) |
| `SkipListSequence` | $O( \log n )$ E | $O( \log n )$ E | [2](02-sequence.md#ch-2) |
| `ChainedHashTable` | $O( 1 )$ E | $O( 1 )$ A, E | [7](07-dictionary.md#ch-7) |
| `OpenHashTable` | $O( 1 )$ E | $O( 1 )$ A, E | [7](07-dictionary.md#ch-7) |
| `SkipList` | $O( \log n )$ E | $O( \log n )$ E | [7](07-dictionary.md#ch-7) |
| `Treap` | $O( \log n )$ E | $O( \log n )$ E | [8](08-tree.md#ch-8) |
| `ScapegoatTree` | $O( \log n )$ | $O( \log n )$ A | [8](08-tree.md#ch-8) |
| `RedBlackTree` | $O( \log n )$ | $O( \log n )$ | [8](08-tree.md#ch-8) |
| `BinaryTrie` | $O( w )$ | $O( w )$ | [7](07-dictionary.md#ch-7) |
| `XFastTrie` | $O( \log w )$ E | $O( w )$ A, E | [7](07-dictionary.md#ch-7) |
| `YFastTrie` | $O( \log w )$ E | $O( \log w )$ A, E | [7](07-dictionary.md#ch-7) |
| `BTree` | $O( t \log_t n )$ | $O( t \log_t n )$ | [8](08-tree.md#ch-8) |

<span id="tab-1-5"></span>**Table 1.5.** Every class of the library, by
header (relative to `include/ADS`), and the chapter that describes it.

| Header | Set | Classes | Chapter |
|-----------------|------|-------------------------------|-----|
| `Algebra.h` | advanced | `SumOf`, `MinOf`, `MaxOf` | [1](01-introduction.md#ch-1) |
| `Capabilities.h` | core | `forwarding_query`, `stable_references_t`, `thread_safe_reads_t`, `rank_complexity_t`, `event_set_t` | [1](01-introduction.md#ch-1) |
| `Contract.h` | core | `contract_violation` | [1](01-introduction.md#ch-1) |
| `Modification.h` | advanced | `Refused`, `no_rebuild_t`, `effect`, `Value`, `Key`, `Cost`, `Upper`, `Lower`, `Supply`, `last`, `min`, `max`, `sum`, `reduction_of`, `list`, `Inserted`, `Erased`, `Changed`, `Relocated`, `Permuted`, `BulkInserted`, `BulkErased`, `Shifted`, `Replaced`, `Cleared`, `Rebuilt`, `Group`, `Counts`, `Direct`, `Via`, `coalesce_traits`, `NoObservers`, `Static`, `Dynamic`, `Both`, `Batch`, `Quiet`, `UndoLog`, `Shadow`, `Assigned`, `KeyChanged`, `Reopt`, `ReoptStats`, `RatioPolicy`, `NeumaierSum` | [1](01-introduction.md#ch-1), [14](14-graph.md#ch-14) |
| `Iterators.h` | core | `LinkedIterator`, `IndexIterator`, `CircularIterator`, `RankIterator`, `IndirectIterator`, `SlotIterator`, `KeyIterator`, `PairRef`, `PathIterator`, `WalkIterator` | [1](01-introduction.md#ch-1) |
| `interop/boost/Dijkstra.h` | interop | `UpdatableQueue` | [1](01-introduction.md#ch-1) |
| `interop/boost/FromBGL.h` | interop | `FromBGL` | [1](01-introduction.md#ch-1) |
| `interop/boost/Graph.h` | interop | `Arc`, `RowWeightMap`, `ArcIndexMap`, `Edge`, `ListWeightMap` | [1](01-introduction.md#ch-1) |
| `interop/std/FlatMap.h` | interop | `StdSequence` | [1](01-introduction.md#ch-1) |
| `interop/std/Pmr.h` | interop | `PmrResource` | [1](01-introduction.md#ch-1) |
| `set/Bag.h` | base | `Bag` | [1](01-introduction.md#ch-1) |
| `set/SetInterfaces.h` | base | `ArrayUSet`, `ArraySSet` | [1](01-introduction.md#ch-1) |
| `sequence/ArraySequence.h` | base | `ArraySequence` | [2](02-sequence.md#ch-2) |
| `sequence/ContiguousDeque.h` | base | `ContiguousDeque` | [2](02-sequence.md#ch-2) |
| `sequence/CubishArrayStack.h` | base | `CubishArrayStack` | [2](02-sequence.md#ch-2) |
| `sequence/CursorList.h` | base | `CursorSpace`, `CursorList` | [2](02-sequence.md#ch-2) |
| `sequence/DualArrayDeque.h` | base | `DualArrayDeque` | [2](02-sequence.md#ch-2) |
| `sequence/Fenwick.h` | advanced | `Fenwick`, `FenwickObserver` | [1](01-introduction.md#ch-1) |
| `sequence/List.h` | base | `List` | [2](02-sequence.md#ch-2) |
| `sequence/ObservableArray.h` | advanced | `ObservableArray` | [1](01-introduction.md#ch-1) |
| `sequence/OrderMaintenanceList.h` | base | `OrderMaintenanceList` | [2](02-sequence.md#ch-2) |
| `sequence/RootishArrayStack.h` | base | `RootishArrayStack` | [2](02-sequence.md#ch-2) |
| `sequence/SEList.h` | base | `SEList` | [2](02-sequence.md#ch-2) |
| `sequence/SequenceRange.h` | base | `SequenceRange` | [2](02-sequence.md#ch-2) |
| `sequence/SinglyList.h` | base | `SinglyList` | [2](02-sequence.md#ch-2) |
| `sequence/SkipListSequence.h` | base | `SkipListSequence` | [2](02-sequence.md#ch-2) |
| `sequence/SmallVector.h` | advanced | `SmallVector` | [2](02-sequence.md#ch-2) |
| `sequence/TieredVector.h` | advanced | `TieredVector` | [2](02-sequence.md#ch-2) |
| `sequence/TreapList.h` | base | `TreapList` | [2](02-sequence.md#ch-2) |
| `sequence/Treque.h` | base | `Treque` | [2](02-sequence.md#ch-2) |
| `sequence/XorList.h` | base | `XorList` | [2](02-sequence.md#ch-2) |
| `stack/ArrayStack.h` | base | `ArrayStack` | [3](03-stack.md#ch-3) |
| `stack/MinStack.h` | base | `MinStack` | [3](03-stack.md#ch-3) |
| `stack/MultiStack.h` | base | `MultiStack` | [3](03-stack.md#ch-3) |
| `stack/SequenceStack.h` | base | `SequenceStack` | [3](03-stack.md#ch-3) |
| `queue/ArrayQueue.h` | base | `ArrayQueue` | [4](04-queue.md#ch-4) |
| `queue/Dequeue.h` | base | `Dequeue` | [4](04-queue.md#ch-4) |
| `queue/LinkedQueue.h` | base | `LinkedQueue` | [4](04-queue.md#ch-4) |
| `queue/MinDeque.h` | base | `MinDeque` | [4](04-queue.md#ch-4) |
| `queue/MinQueue.h` | base | `MinQueue` | [4](04-queue.md#ch-4) |
| `queue/RandomQueue.h` | base | `RandomQueue` | [4](04-queue.md#ch-4) |
| `queue/RealTimeMinDeque.h` | base | `RealTimeMinDeque` | [4](04-queue.md#ch-4) |
| `queue/RealTimeMinQueue.h` | base | `RealTimeMinQueue` | [4](04-queue.md#ch-4) |
| `queue/RingDeque.h` | advanced | `RingDeque` | [4](04-queue.md#ch-4) |
| `queue/SlidingAggregate.h` | advanced | `Amortized`, `WorstCase`, `SlidingAggregate` | [1](01-introduction.md#ch-1) |
| `queue/UniqueQueue.h` | base | `UniqueQueue` | [4](04-queue.md#ch-4) |
| `memory/Allocators.h` | base | `BoundaryTagAllocator`, `BuddyAllocator` | [5](05-memory.md#ch-5) |
| `memory/Arena.h` | advanced | `Arena` | [5](05-memory.md#ch-5) |
| `memory/CellHeap.h` | base | `CellHeap` | [5](05-memory.md#ch-5) |
| `memory/ConcurrentPool.h` | advanced | `ConcurrentPoolResource` | [5](05-memory.md#ch-5) |
| `memory/HugePageAllocator.h` | core | `HugePageAllocator` | [5](05-memory.md#ch-5) |
| `memory/ObjectPool.h` | core | `ObjectPool` | [5](05-memory.md#ch-5) |
| `memory/PoolAllocator.h` | advanced | `PoolResource`, `PoolAllocator` | [5](05-memory.md#ch-5) |
| `memory/SizeClassAllocator.h` | advanced | `SizeClassAllocator` | [5](05-memory.md#ch-5) |
| `search/CompressedPGMIndex.h` | advanced | `CompressedPGMIndex` | [6](06-search.md#ch-6) |
| `search/DynamicPGMIndex.h` | advanced | `DynamicPGMIndex` | [6](06-search.md#ch-6) |
| `search/PGMIndex.h` | base | `PGMIndex` | [6](06-search.md#ch-6) |
| `search/RangeMinimum.h` | advanced | `SparseTable`, `PlusMinusOneRangeMinimum`, `RangeMinimum` | [6](06-search.md#ch-6) |
| `search/StaticSearch.h` | advanced | `SPlusTree` | [6](06-search.md#ch-6) |
| `search/WordRangeMinimum.h` | advanced | `WordRangeMinimum` | [6](06-search.md#ch-6) |
| `dictionary/AdaptiveRadixTree.h` | advanced | `AdaptiveRadixTree` | [7](07-dictionary.md#ch-7) |
| `dictionary/BinaryTrie.h` | base | `BinaryTrie`, `SimpleBinaryTrie` | [7](07-dictionary.md#ch-7) |
| `dictionary/BitmapTrie.h` | base | `BitmapTrie` | [7](07-dictionary.md#ch-7) |
| `dictionary/BucketCuckooTable.h` | advanced | `BucketCuckooTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/ChainedHashTable.h` | base | `ChainedHashTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/CuckooHashing.h` | base | `CuckooHashTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/DenseHashMap.h` | advanced | `DenseHashMap` | [7](07-dictionary.md#ch-7) |
| `dictionary/DirectTable.h` | base | `DirectTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/FastHash.h` | advanced | `Product128`, `FastHash` | [7](07-dictionary.md#ch-7) |
| `dictionary/FingerprintPerfectHash.h` | advanced | `FingerprintPerfectHash` | [7](07-dictionary.md#ch-7) |
| `dictionary/FlatHashTable.h` | advanced | `FlatHashTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/IndexedSkipList.h` | base | `IndexedSkipList` | [7](07-dictionary.md#ch-7) |
| `dictionary/KnuthHashing.h` | base | `CoalescedHashTable`, `BrentHashTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/MinimalPerfectHash.h` | advanced | `MinimalPerfectHash` | [7](07-dictionary.md#ch-7) |
| `dictionary/OpenHashTable.h` | base | `Probing`, `OpenHashTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/PatriciaTrie.h` | base | `PatriciaTrie` | [7](07-dictionary.md#ch-7) |
| `dictionary/PerfectHashing.h` | base | `PerfectHashTable`, `OrderedMinimalPerfectHash` | [7](07-dictionary.md#ch-7) |
| `dictionary/PointerTable.h` | advanced | `StateEqual`, `PointerTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/PositionIndex.h` | advanced | `PositionIndex` | [7](07-dictionary.md#ch-7) |
| `dictionary/RandomHashing.h` | core | `SplitMix64`, `ZobristTable`, `TabulationHash` | [7](07-dictionary.md#ch-7) |
| `dictionary/RecSplit.h` | advanced | `RecSplit` | [7](07-dictionary.md#ch-7) |
| `dictionary/SkipList.h` | base | `SkipList` | [7](07-dictionary.md#ch-7) |
| `dictionary/SortedArrayDictionary.h` | base | `SortedArrayDictionary` | [7](07-dictionary.md#ch-7) |
| `dictionary/SuccinctPatriciaTrie.h` | advanced | `SuccinctPatriciaTrie` | [7](07-dictionary.md#ch-7) |
| `dictionary/Trie.h` | base | `Trie`, `RadixTree` | [7](07-dictionary.md#ch-7) |
| `dictionary/TwoLevelDictionary.h` | base | `TwoLevelDictionary` | [7](07-dictionary.md#ch-7) |
| `dictionary/UniversalHashing.h` | base | `CarterWegmanHash`, `DotProductHash`, `MultiplyShiftHash`, `StringHash`, `MultiplicativeHash`, `CompoundHash`, `SequenceHash`, `DLeftHashTable` | [7](07-dictionary.md#ch-7) |
| `dictionary/XFastTrie.h` | base | `XFastTrie`, `ListXFastTrie` | [7](07-dictionary.md#ch-7) |
| `dictionary/YFastTrie.h` | base | `YFastTrie` | [7](07-dictionary.md#ch-7) |
| `tree/AVLTree.h` | base | `AVLTree` | [8](08-tree.md#ch-8) |
| `tree/ArrayBinaryTree.h` | base | `ArrayBinaryTree` | [8](08-tree.md#ch-8) |
| `tree/BPlusTree.h` | base | `BPlusTree` | [8](08-tree.md#ch-8) |
| `tree/BTree.h` | base | `BTree` | [8](08-tree.md#ch-8) |
| `tree/BTreeMap.h` | advanced | `BTreeMap` | [8](08-tree.md#ch-8) |
| `tree/BinaryTree.h` | base | `BinaryTree` | [8](08-tree.md#ch-8) |
| `tree/ChildListTree.h` | base | `ChildListTree` | [8](08-tree.md#ch-8) |
| `tree/CursorBinaryTree.h` | base | `CursorBinaryTree` | [8](08-tree.md#ch-8) |
| `tree/CursorTree.h` | base | `CursorTree` | [8](08-tree.md#ch-8) |
| `tree/FingerTreap.h` | base | `FingerTreap` | [8](08-tree.md#ch-8) |
| `tree/HashedTreap.h` | base | `HashedTreap` | [8](08-tree.md#ch-8) |
| `tree/JoinAlgorithms.h` | advanced | `AVLJoin`, `RedBlackJoin`, `WeightBalancedJoin`, `TreapJoin`, `JoinAlgorithms`, `TakeSecond`, `TakeFirst`, `Joinable` | [8](08-tree.md#ch-8) |
| `tree/JoinTree.h` | advanced | `AVLBalance`, `RedBlackBalance`, `WeightBalance`, `TreapBalance`, `NoAugmentation`, `SumAugmentation`, `MaxAugmentation`, `JoinTree`, `JoinAVLTree`, `JoinRedBlackTree`, `WeightBalancedTree`, `JoinTreap`, `AugmentedMap` | [8](08-tree.md#ch-8) |
| `tree/LeftLeaningRedBlackTree.h` | base | `LeftLeaningRedBlackTree` | [8](08-tree.md#ch-8) |
| `tree/LinkCutTree.h` | advanced | `LinkCutTree` | [8](08-tree.md#ch-8) |
| `tree/MaterializedAggregate.h` | advanced | `MaterializedAggregate` | [8](08-tree.md#ch-8) |
| `tree/MeasuredSearchTree.h` | base | `MeasuredSearchTree` | [8](08-tree.md#ch-8) |
| `tree/ParentTree.h` | base | `ParentTree` | [8](08-tree.md#ch-8) |
| `tree/PartialRebuildingTree.h` | base | `PartialRebuildingTree`, `WeightRebuildTree`, `CountdownTree`, `DynamiteTree` | [8](08-tree.md#ch-8) |
| `tree/PreorderArrayTree.h` | base | `PreorderArrayTree` | [8](08-tree.md#ch-8) |
| `tree/RandomizedSearchTree.h` | base | `RandomizedSearchTree` | [8](08-tree.md#ch-8) |
| `tree/ScapegoatTree.h` | base | `ScapegoatTree` | [8](08-tree.md#ch-8) |
| `tree/SearchTree.h` | base | `NotJoinable`, `SearchTree`, `BinarySearchTree`, `RedBlackTree` | [8](08-tree.md#ch-8) |
| `tree/SplayTree.h` | advanced | `SplayTree` | [8](08-tree.md#ch-8) |
| `tree/ThreadedTree.h` | base | `ThreadedTree` | [8](08-tree.md#ch-8) |
| `tree/Treap.h` | base | `Treap` | [8](08-tree.md#ch-8) |
| `tree/Tree.h` | base | `Tree` | [8](08-tree.md#ch-8) |
| `priority/BucketQueue.h` | advanced | `BucketQueue`, `BucketHeap`, `LargestSecondKey`, `LeastSecondKey` | [9](09-priority.md#ch-9) |
| `priority/DaryHeap.h` | advanced | `DaryHeap` | [9](09-priority.md#ch-9) |
| `priority/DialQueue.h` | advanced | `DialQueue` | [9](09-priority.md#ch-9) |
| `priority/FibonacciHeapPriorityQueue.h` | base | `FibonacciHeapPriorityQueue` | [9](09-priority.md#ch-9) |
| `priority/HeapPriorityQueue.h` | base | `HeapPriorityQueue` | [9](09-priority.md#ch-9) |
| `priority/IndexedDaryHeap.h` | advanced | `IndexedDaryHeap` | [9](09-priority.md#ch-9) |
| `priority/IndexedHeapPair.h` | advanced | `IndexedHeapPair` | [9](09-priority.md#ch-9) |
| `priority/LinkedHeapPriorityQueue.h` | advanced | `LinkedHeapPriorityQueue` | [9](09-priority.md#ch-9) |
| `priority/ListPriorityQueue.h` | base | `ListPriorityQueue` | [9](09-priority.md#ch-9) |
| `priority/LoserTree.h` | base | `LoserTree` | [9](09-priority.md#ch-9) |
| `priority/MeldableHeap.h` | base | `MeldableHeap` | [9](09-priority.md#ch-9) |
| `priority/MinMaxHeap.h` | advanced | `MinMaxHeap`, `IntervalHeap` | [9](09-priority.md#ch-9) |
| `priority/MultiwayMerge.h` | advanced | `FlatLoserTree` | [9](09-priority.md#ch-9) |
| `priority/PairingHeap.h` | advanced | `PairingHeap` | [9](09-priority.md#ch-9) |
| `priority/RadixHeap.h` | advanced | `RadixHeap` | [9](09-priority.md#ch-9) |
| `priority/SequenceHeap.h` | advanced | `SequenceHeap` | [9](09-priority.md#ch-9) |
| `priority/TreePriorityQueue.h` | base | `TreePriorityQueue` | [9](09-priority.md#ch-9) |
| `set/BinaryFuseFilter.h` | advanced | `BinaryFuseFilter` | [10](10-set.md#ch-10) |
| `set/BlockedBloomFilter.h` | advanced | `BlockedBloomFilter` | [10](10-set.md#ch-10) |
| `set/BloomFilter.h` | base | `BloomFilter` | [10](10-set.md#ch-10) |
| `set/BloomVariants.h` | base | `SpectralBloomFilter`, `PartitionedBloomFilter` | [10](10-set.md#ch-10) |
| `set/BoolSet.h` | base | `BoolSet` | [10](10-set.md#ch-10) |
| `set/CuckooFilter.h` | advanced | `CuckooFilter` | [10](10-set.md#ch-10) |
| `set/DictionarySet.h` | base | `DictionarySet`, `ChainedHashSet`, `OpenHashSet`, `TreeSet` | [10](10-set.md#ch-10) |
| `set/GrafiteFilter.h` | advanced | `GrafiteFilter`, `BucketingFilter` | [10](10-set.md#ch-10) |
| `set/HashMultiset.h` | base | `HashMultiset` | [10](10-set.md#ch-10) |
| `set/LinkedHashSet.h` | base | `LinkedHashSet` | [10](10-set.md#ch-10) |
| `set/ListSet.h` | base | `ListSet` | [10](10-set.md#ch-10) |
| `set/PrefixFilter.h` | advanced | `PrefixFilter` | [10](10-set.md#ch-10) |
| `set/RibbonFilter.h` | advanced | `RibbonFilter` | [10](10-set.md#ch-10) |
| `set/RoaringBitmap.h` | advanced | `RoaringBitmap` | [10](10-set.md#ch-10) |
| `set/SortedIntersection.h` | base | `PermutedBuckets` | [10](10-set.md#ch-10) |
| `set/SortedListSet.h` | base | `SortedListSet` | [10](10-set.md#ch-10) |
| `set/SpaceEfficientSortedSet.h` | base | `SpaceEfficientSortedSet` | [10](10-set.md#ch-10) |
| `mfset/ListMFSet.h` | base | `ListMFSet` | [11](11-mfset.md#ch-11) |
| `mfset/MFSet.h` | base | `MFSet` | [11](11-mfset.md#ch-11) |
| `mfset/RemUnionFind.h` | advanced | `RemUnionFind` | [11](11-mfset.md#ch-11) |
| `mfset/RollbackUnionFind.h` | advanced | `RollbackUnionFind`, `ConnectivityOp` | [11](11-mfset.md#ch-11) |
| `mfset/UnionFind.h` | advanced | `DisjointSets`, `UnionFind` | [11](11-mfset.md#ch-11) |
| `sort/QuicksortAdversary.h` | advanced | `QuicksortAdversary` | [12](12-sort.md#ch-12) |
| `algorithm/MasterTheorem.h` | base | `Growth` | [13](13-algorithm.md#ch-13) |
| `algorithm/MaximumSegments.h` | base | `Segment` | [13](13-algorithm.md#ch-13) |
| `optimization/GridPaths.h` | base | `GridSquare` | [13](13-algorithm.md#ch-13) |
| `optimization/OptimalSearchTree.h` | base | `OptimalSearchTree` | [13](13-algorithm.md#ch-13) |
| `optimization/WeightedIntervals.h` | base | `WeightedInterval` | [13](13-algorithm.md#ch-13) |
| `graph/AllPairsShortestPaths.h` | base | `AllPairs` | [14](14-graph.md#ch-14) |
| `graph/ArrayGraph.h` | base | `ArrayGraph` | [14](14-graph.md#ch-14) |
| `graph/Assignment.h` | advanced | `Assignment` | [14](14-graph.md#ch-14) |
| `graph/Graph.h` | base | `GraphNodes` | [14](14-graph.md#ch-14) |
| `graph/GraphBuilder.h` | advanced | `GraphBuilder` | [14](14-graph.md#ch-14) |
| `graph/GridGraph.h` | advanced | `GridGraph` | [14](14-graph.md#ch-14) |
| `graph/IncidenceGraph.h` | base | `IncidenceGraph` | [14](14-graph.md#ch-14) |
| `graph/ListGraph.h` | base | `ListGraph` | [14](14-graph.md#ch-14) |
| `graph/MatrixGraph.h` | base | `MatrixGraph` | [14](14-graph.md#ch-14) |
| `graph/MaxFlow.h` | base | `FlowNetwork`, `BoundedEdge` | [14](14-graph.md#ch-14) |
| `graph/MinimumSpanningTree.h` | base | `WeightedEdge`, `MergeCounts` | [14](14-graph.md#ch-14) |
| `graph/Observable.h` | advanced | `NodeInserted`, `NodeDeleted`, `EdgeInserted`, `EdgeDeleted`, `WeightChanged`, `GraphEvents`, `coalesce_traits`, `ObservedGraph`, `Observable` | [14](14-graph.md#ch-14) |
| `graph/PointerGraph.h` | advanced | `PointerGraph` | [14](14-graph.md#ch-14) |
| `graph/Reoptimization.h` | advanced | `EventQueue`, `ShortestPathsObserver`, `SpanningForestObserver`, `AugmentEngine`, `MaxFlowObserver`, `AssignmentObserver` | [14](14-graph.md#ch-14) |
| `graph/ShortestPaths.h` | base | `ShortestPathTree`, `FrontierCounts`, `PriorityFrontier`, `QueueFrontier`, `StackFrontier`, `DequeFrontier` | [14](14-graph.md#ch-14) |
| `graph/StaticGraph.h` | advanced | `WeightedArc`, `StaticGraph` | [14](14-graph.md#ch-14) |
| `graph/Visit.h` | base | `VisitCounts`, `DfsSchema` | [14](14-graph.md#ch-14) |
| `interop/lemon/Digraphs.h` | interop | `StaticDigraphBase`, `StaticDigraphView`, `GraphView` | [14](14-graph.md#ch-14) |
| `interop/lemon/Heaps.h` | interop | `CmpKey`, `DaryHeap`, `MonotoneHeap` | [14](14-graph.md#ch-14) |
| `interop/lemon/Maps.h` | interop | `DenseMap`, `IndexMap`, `ObservedMap` | [14](14-graph.md#ch-14) |
| `interop/lemon/Observers.h` | interop | `Ends`, `LemonTap`, `LemonMirror` | [14](14-graph.md#ch-14) |
| `interop/lemon/Warm.h` | interop | `PreflowEngine`, `NetworkSimplexAssignment`, `MinCostFlowObserver` | [14](14-graph.md#ch-14) |
| `string/AhoCorasick.h` | advanced | `AhoCorasick` | [15](15-string.md#ch-15) |
| `string/FrontCoding.h` | base | `FrontCodedDictionary` | [15](15-string.md#ch-15) |
| `string/LongestCommonExtension.h` | advanced | `LceIndex` | [15](15-string.md#ch-15) |
| `string/MultiSearch.h` | advanced | `MultiSearcher` | [15](15-string.md#ch-15) |
| `string/Palindromes.h` | base | `Eertree` | [15](15-string.md#ch-15) |
| `string/PrefixSearch.h` | base | `BinnedSearch`, `BinnedStringSearch` | [15](15-string.md#ch-15) |
| `string/RearCoding.h` | base | `RearCodedDictionary` | [15](15-string.md#ch-15) |
| `string/StringSearch.h` | advanced | `StringSearcher` | [15](15-string.md#ch-15) |
| `string/SuffixApplications.h` | base | `CopyPhrase` | [15](15-string.md#ch-15) |
| `string/SuffixArray.h` | base | `SuffixArray` | [15](15-string.md#ch-15) |
| `string/SuffixAutomaton.h` | advanced | `SuffixAutomaton` | [15](15-string.md#ch-15) |
| `string/SuffixTree.h` | base | `SuffixTree` | [15](15-string.md#ch-15) |
| `geometry/ConvexHull.h` | base | `Point` | [16](16-geometry.md#ch-16) |
| `geometry/IntervalTree.h` | advanced | `IntervalTree` | [16](16-geometry.md#ch-16) |
| `geometry/RangeTree.h` | advanced | `RangeTree` | [16](16-geometry.md#ch-16) |
| `numeric/BigInteger.h` | base | `BigInteger` | [17](17-numeric.md#ch-17) |
| `numeric/Combinatorics.h` | base | `FibonacciCache` | [17](17-numeric.md#ch-17) |
| `numeric/CsrMatrix.h` | advanced | `CsrMatrix` | [17](17-numeric.md#ch-17) |
| `numeric/DenseMatrix.h` | advanced | `DenseMatrix` | [17](17-numeric.md#ch-17) |
| `numeric/DynamicCsr.h` | advanced | `DynamicCsr`, `DynamicCsc` | [17](17-numeric.md#ch-17) |
| `numeric/NumberTheory.h` | base | `Montgomery64` | [17](17-numeric.md#ch-17) |
| `numeric/RSA.h` | base | `RsaKey` | [17](17-numeric.md#ch-17) |
| `numeric/RandomGenerators.h` | advanced | `Xoshiro256T`, `WyRand`, `PhiloxT`, `AliasTable`, `Xoshiro256`, `Xoshiro256StarStar`, `Philox4x32`, `Philox4x64` | [17](17-numeric.md#ch-17) |
| `numeric/RandomNumbers.h` | base | `LinearCongruential`, `LinearCongruentialT`, `MinStd`, `Mmix` | [17](17-numeric.md#ch-17) |
| `numeric/Rational.h` | base | `Rational` | [17](17-numeric.md#ch-17) |
| `numeric/SellMatrix.h` | advanced | `SellMatrix` | [17](17-numeric.md#ch-17) |
| `numeric/SparseAssembly.h` | advanced | `AssemblyWorkspace` | [17](17-numeric.md#ch-17) |
| `numeric/SparseMatrix.h` | base | `SparseMatrix` | [17](17-numeric.md#ch-17) |
| `backtrack/DancingLinks.h` | base | `ExactCover` | [19](19-backtrack.md#ch-19) |
| `backtrack/FastExactCover.h` | advanced | `FastExactCover` | [19](19-backtrack.md#ch-19) |
| `backtrack/SatSolvers.h` | base | `BasicCdcl`, `Cdcl` | [19](19-backtrack.md#ch-19) |
| `complexity/Domino.h` | base | `Tile` | [20](20-complexity.md#ch-20) |
| `complexity/Nondeterminism.h` | base | `Chooser` | [20](20-complexity.md#ch-20) |
| `intractable/Heuristics.h` | base | `Buckets` | [20](20-complexity.md#ch-20) |
| `intractable/Pseudopolynomial.h` | base | `Piece` | [20](20-complexity.md#ch-20) |
| `compression/ArithmeticCoding.h` | base | `RangeEncoder`, `RangeDecoder` | [21](21-compression.md#ch-21) |
| `compression/BurrowsWheeler.h` | base | `BWT` | [21](21-compression.md#ch-21) |
| `compression/CanonicalHuffman.h` | base | `CanonicalHuffman` | [21](21-compression.md#ch-21) |
| `compression/DirectAccessCodes.h` | advanced | `DirectAccessCodes` | [21](21-compression.md#ch-21) |
| `compression/FMIndex.h` | base | `FMIndex` | [21](21-compression.md#ch-21) |
| `compression/HuffmanFMIndex.h` | advanced | `HuffmanFMIndex` | [21](21-compression.md#ch-21) |
| `compression/IntegerCodes.h` | base | `PForDeltaBlock` | [21](21-compression.md#ch-21) |
| `compression/LempelZiv.h` | base | `LZ77Phrase`, `LZSSPhrase`, `LZ78Phrase` | [21](21-compression.md#ch-21) |
| `compression/MoveRIndex.h` | advanced | `MoveRIndex` | [21](21-compression.md#ch-21) |
| `compression/PPM.h` | base | `PPMModel` | [21](21-compression.md#ch-21) |
| `compression/PackedBits.h` | core | `PackedBitWriter`, `PackedBitReader` | [21](21-compression.md#ch-21) |
| `compression/RIndex.h` | advanced | `RIndex` | [21](21-compression.md#ch-21) |
| `succinct/BPTree.h` | base | `BPTree` | [22](22-succinct.md#ch-22) |
| `succinct/BVGraph.h` | base | `BVGraph` | [22](22-succinct.md#ch-22) |
| `succinct/BalancedParentheses.h` | base | `BalancedParentheses` | [22](22-succinct.md#ch-22) |
| `succinct/BitStream.h` | base | `BitWriter`, `BitReader` | [22](22-succinct.md#ch-22) |
| `succinct/BitVector.h` | base | `SubscriptIterator`, `BitVector` | [22](22-succinct.md#ch-22) |
| `succinct/CompactRankSelect.h` | advanced | `CompactRankSelect` | [22](22-succinct.md#ch-22) |
| `succinct/EliasFano.h` | base | `BasicEliasFano`, `EliasFano`, `LazyEliasFano` | [22](22-succinct.md#ch-22) |
| `succinct/EliasFanoGraph.h` | base | `EliasFanoGraph` | [22](22-succinct.md#ch-22) |
| `succinct/HuffmanWaveletTree.h` | base | `HuffmanWaveletTree` | [22](22-succinct.md#ch-22) |
| `succinct/InterleavedRankSelect.h` | advanced | `InterleavedRankSelect` | [22](22-succinct.md#ch-22) |
| `succinct/JacobsonTree.h` | base | `JacobsonTree` | [22](22-succinct.md#ch-22) |
| `succinct/K2Tree.h` | base | `K2Tree` | [22](22-succinct.md#ch-22) |
| `succinct/LAVector.h` | advanced | `LAVector` | [22](22-succinct.md#ch-22) |
| `succinct/LoudsTree.h` | base | `LoudsTree` | [22](22-succinct.md#ch-22) |
| `succinct/MoveStructure.h` | advanced | `MoveStructure` | [22](22-succinct.md#ch-22) |
| `succinct/PackedArray.h` | advanced | `PackedArray`, `TritArray` | [22](22-succinct.md#ch-22) |
| `succinct/PartitionedEliasFano.h` | advanced | `PartitionedEliasFano` | [22](22-succinct.md#ch-22) |
| `succinct/RRRBitVector.h` | advanced | `RRRBitVector` | [22](22-succinct.md#ch-22) |
| `succinct/RankSelect.h` | base | `RankSelectOptions`, `BasicRankSelect`, `RankSelect`, `LazyRankSelect` | [22](22-succinct.md#ch-22) |
| `succinct/WaveletMatrix.h` | advanced | `WaveletMatrix` | [22](22-succinct.md#ch-22) |
| `external/BlockBTree.h` | base | `BlockBTree` | [23](23-external.md#ch-23) |
| `external/BlockStore.h` | base | `BlockStore` | [23](23-external.md#ch-23) |
| `external/RecordFile.h` | core | `RecordFile`, `RecordWriter`, `TempDirectory` | [23](23-external.md#ch-23) |
| `external/TwoLevelMemory.h` | base | `TwoLevelMemory`, `MemoryLease`, `DiskFile`, `DiskReader`, `DiskWriter` | [23](23-external.md#ch-23) |
| `concurrent/Backoff.h` | advanced | `Backoff` | [24](24-concurrent.md#ch-24) |
| `concurrent/ConcurrentCuckooTable.h` | advanced | `ConcurrentCuckooTable` | [24](24-concurrent.md#ch-24) |
| `concurrent/ConcurrentFlatTable.h` | advanced | `ConcurrentFlatTable`, `ZeroingClear`, `EpochClear` | [24](24-concurrent.md#ch-24) |
| `concurrent/ConcurrentUnionFind.h` | advanced | `ConcurrentUnionFind` | [24](24-concurrent.md#ch-24) |
| `concurrent/EventCount.h` | advanced | `EventCount` | [24](24-concurrent.md#ch-24) |
| `concurrent/LockFreePriorityQueue.h` | advanced | `LockFreePriorityQueue` | [24](24-concurrent.md#ch-24) |
| `concurrent/MPMCQueue.h` | advanced | `MPMCQueue` | [24](24-concurrent.md#ch-24) |
| `concurrent/MPSCInbox.h` | advanced | `MPSCInbox` | [24](24-concurrent.md#ch-24) |
| `concurrent/MPSCQueue.h` | advanced | `MPSCQueue` | [24](24-concurrent.md#ch-24) |
| `concurrent/MinBoard.h` | advanced | `MinBoard` | [24](24-concurrent.md#ch-24) |
| `concurrent/MultiQueue.h` | advanced | `MultiQueue`, `MultiQueueHeap` | [24](24-concurrent.md#ch-24) |
| `concurrent/SPSCQueue.h` | advanced | `SPSCQueue` | [24](24-concurrent.md#ch-24) |
| `concurrent/SingleWriterFlatTable.h` | advanced | `SingleWriterFlatTable` | [24](24-concurrent.md#ch-24) |
| `concurrent/StealablePriorityQueue.h` | advanced | `StealablePriorityQueue`, `StealingFrontier` | [24](24-concurrent.md#ch-24) |
| `concurrent/WorkStealingDeque.h` | advanced | `WorkStealingDeque` | [24](24-concurrent.md#ch-24) |
| `parallel/Channels.h` | advanced | `ChannelPort`, `LockedChannels`, `InboxChannels`, `LaneChannels`, `MeshChannels`, `ChannelsOf` | [25](25-parallel.md#ch-25) |
| `parallel/Executor.h` | core | `Sequential` | [25](25-parallel.md#ch-25) |
| `parallel/FastFlowExecutor.h` | advanced | `FastFlowLock`, `FastFlowExecutor` | [25](25-parallel.md#ch-25) |
| `parallel/Mailbox.h` | advanced | `Mailbox`, `NoCoalescing`, `CoalesceBy` | [25](25-parallel.md#ch-25) |
| `parallel/OpenMPExecutor.h` | advanced | `OpenMPLock`, `OpenMPExecutor` | [25](25-parallel.md#ch-25) |
| `parallel/SpeculativeFor.h` | advanced | `Reservation`, `SpeculativeCounts` | [25](25-parallel.md#ch-25) |
| `parallel/TBBExecutor.h` | advanced | `TBBExecutor` | [25](25-parallel.md#ch-25) |
| `parallel/ThreadPool.h` | advanced | `ThreadPool` | [25](25-parallel.md#ch-25) |
| `distributed/MPIExecutor.h` | advanced | `MPIEnvironment`, `MPIExecutor`, `DistributedMin` | [26](26-distributed.md#ch-26) |
| `distributed/MPIMailbox.h` | advanced | `MPICodec`, `MPIByteMailbox`, `MPIMailbox` | [26](26-distributed.md#ch-26) |

<span id="tab-1-6"></span>**Table 1.6.** The headers that declare functions
only, by chapter.

| Chapter | Headers of functions |
|-----|-----------------------------------------------|
| [1](01-introduction.md#ch-1) | `ADT.h`, `Portable.h`, `interop/boost/Heap.h`, `interop/boost/Iterator.h` |
| [2](02-sequence.md#ch-2) | `sequence/IndexedList.h`, `sequence/ListSort.h`, `sequence/Rearrangements.h`, `sequence/SequenceAlgorithms.h` |
| [3](03-stack.md#ch-3) | `stack/Expressions.h`, `stack/StackAlgorithms.h` |
| [4](04-queue.md#ch-4) | `queue/QueueAlgorithms.h` |
| [5](05-memory.md#ch-5) | `memory/PageHeap.h` |
| [6](06-search.md#ch-6) | `search/BinarySearch.h`, `search/CountLess.h`, `search/InterpolationSearch.h`, `search/KnuthSearches.h`, `search/LocalOptima.h`, `search/Minimum.h`, `search/MonotoneSearch.h`, `search/NthElement.h`, `search/Selection.h`, `search/SortedSearches.h` |
| [7](07-dictionary.md#ch-7) | `dictionary/HashFunctions.h`, `dictionary/RowFingerprint.h`, `dictionary/TableSize.h` |
| [8](08-tree.md#ch-8) | `tree/BinaryTreeAlgorithms.h`, `tree/ExpressionTree.h`, `tree/HeapByInsertion.h` |
| [9](09-priority.md#ch-9) | `priority/OrderKey.h`, `priority/PriorityQueue.h` |
| [10](10-set.md#ch-10) | `set/SetOperations.h`, `set/VectorIntersection.h` |
| [12](12-sort.md#ch-12) | `sort/CountingSort.h`, `sort/EngineeredQuickSorts.h`, `sort/ExchangeSorts.h`, `sort/ExternalSort.h`, `sort/Heapsort.h`, `sort/InsertionSort.h`, `sort/KnuthSorts.h`, `sort/LearnedSort.h`, `sort/MergeSort.h`, `sort/MultikeyQuickSort.h`, `sort/PigeonholeSort.h`, `sort/QuickSort.h`, `sort/RadixSort.h`, `sort/RunSorts.h`, `sort/SampleSort.h`, `sort/SelectionSort.h`, `sort/ShellSort.h`, `sort/Sort.h`, `sort/Sorted.h`, `sort/SortingNetworks.h`, `sort/StrandSort.h`, `sort/StringSort.h`, `sort/TreeSort.h`, `sort/VectorQuickSort.h` |
| [13](13-algorithm.md#ch-13) | `algorithm/AkraBazzi.h`, `algorithm/BinaryCounter.h`, `algorithm/Hanoi.h`, `algorithm/MonteCarlo.h`, `algorithm/Paradigms.h`, `algorithm/SequenceQueries.h`, `algorithm/Subsequences.h`, `optimization/CoinChange.h`, `optimization/CoinGame.h`, `optimization/FractionalKnapsack.h`, `optimization/HouseProblems.h`, `optimization/IntervalCover.h`, `optimization/Intervals.h`, `optimization/Knapsack.h`, `optimization/KnapsackVariants.h`, `optimization/LinearPartition.h`, `optimization/SequenceProblems.h`, `optimization/ShortestJobFirst.h`, `optimization/StockTrading.h` |
| [14](14-graph.md#ch-14) | `graph/BlossomMatching.h`, `graph/EulerianCycle.h`, `graph/FlowApplications.h`, `graph/Grid.h`, `graph/Matching.h`, `graph/Paths.h`, `graph/Properties.h`, `graph/PushRelabel.h`, `graph/StrongComponents.h`, `interop/lemon/Parallel.h` |
| [15](15-string.md#ch-15) | `string/Alignment.h`, `string/Anagrams.h`, `string/ApproximateMatching.h`, `string/EditDistance.h`, `string/Huffman.h`, `string/LongestCommonSubsequence.h`, `string/Lyndon.h`, `string/Parentheses.h`, `string/Segmentations.h`, `string/StringBreaking.h`, `string/StringMatching.h`, `string/SubsequenceProblems.h`, `string/SuffixSorting.h`, `string/WordBreak.h` |
| [16](16-geometry.md#ch-16) | `geometry/ClosestPair.h` |
| [17](17-numeric.md#ch-17) | `numeric/BinaryInteger.h`, `numeric/Catalan.h`, `numeric/Matrix.h`, `numeric/MatrixChain.h`, `numeric/Polynomial.h`, `numeric/RandomSampling.h` |
| [18](18-combinatorial.md#ch-18) | `combinatorial/Bitwise.h`, `combinatorial/FastBitwise.h`, `combinatorial/FastGeneration.h`, `combinatorial/Generation.h` |
| [19](19-backtrack.md#ch-19) | `backtrack/Anagrams.h`, `backtrack/Enumeration.h`, `backtrack/KnightTour.h`, `backtrack/NQueens.h`, `backtrack/Sudoku.h` |
| [20](20-complexity.md#ch-20) | `complexity/EasyCases.h`, `complexity/Problems.h`, `complexity/Reductions.h`, `intractable/Approximation.h`, `intractable/BranchAndBound.h`, `intractable/FifteenPuzzle.h` |
| [21](21-compression.md#ch-21) | `compression/BinaryPacking.h`, `compression/Entropy.h`, `compression/HuffmanCoder.h`, `compression/LZ77Linear.h`, `compression/Rans.h`, `compression/StreamVByte.h`, `compression/Tans.h` |
| [23](23-external.md#ch-23) | `external/ExternalSorting.h`, `external/ListRanking.h` |
| [25](25-parallel.md#ch-25) | `parallel/Affinity.h`, `parallel/Farm.h`, `parallel/ParallelGraph.h`, `parallel/ParallelGreedy.h`, `parallel/ParallelMerge.h`, `parallel/ParallelNumeric.h`, `parallel/ParallelShortestPaths.h`, `parallel/ParallelSort.h`, `parallel/ParallelText.h`, `parallel/Scratch.h` |
| [26](26-distributed.md#ch-26) | `distributed/DistributedSort.h` |

## 1.13 Typed modifications {#sec-1-13}

A structure often has dependents, i.e., objects derived from it that must
follow its changes (e.g., an index of its items, an aggregate of their values,
or a certificate that a solution computed on it is still optimal). The pattern
Observer ([Gamma et al. 1994](Z-bibliography.md#ref-gamma1994)) lets the
structure inform them, and `Modification.h` (beyond the book) gives the form
in which a structure of ADS does it, at no cost when nobody listens. We call
*event* the value by which a change is sent, an aggregate of one of the
templates of the namespace `ADS::mod`. An event carries the state before the
change as well as the state after it; hence, an observer that reads the
events, without reading the structure, stays correct whatever happens to the
structure later. The events of the item of handle $h$ are
`Inserted< H , K , V >` and `Erased< H , K , V >`, with its key and its value
(none when `V` is `void`), and `Changed< A , H , V >`, a change of its
attribute `A` from `from` to `to`. The aliases `Assigned` and `KeyChanged`
stand for the attributes `Value` and `Key`, while `Cost`, `Upper`, `Lower`,
`Supply` or any other tag name the others. We call *global* the events of the
structure as a whole, i.e., `Relocated`, `Permuted`, `BulkInserted`,
`BulkErased`, `Shifted< A , D >` (each value of `A` moved by `delta`),
`Replaced< A >`, `Cleared` and `Rebuilt`, the last of which means that
anything may have changed. Each template has a static mask `effects`, a subset
of `effect::grows`, `shrinks`, `up` and `down` in which a bit not set is a
guarantee (e.g., an insertion makes no value decrease). Also, the changes and
the shifts have `direction()`, and `inverse( e )` gives the event that undoes
$e$. A structure declares the events it emits as a list,
`using modifications = mod::list< ... >`.

An observer of a structure is any object with an `on( e )` for at least one of
its events. Its other parts are optional, and the subject finds them at
compile time by the design by introspection of Alexandrescu. They are
`on( s )` for a `std::span` of one kind, or of the variant of all the kinds,
which a batch prefers to the events one by one, and the group hooks
`group_begin()` (possibly with the versions before and after the batch) and
`group_end()`. Furthermore, an observer may have `prepare( e )`, which may
refuse a change before it is made, `audit( s )`, a check of the observer
against the structure $s$, and `mask()`, a filter of the kinds at run time.
Each of these members is tested by a concept of the namespace `mod`:
`observes< O , E >` for `on( e )` on the event `E`, `observes_span< O , E >`
for `on( s )` on a span, `groups`, `refuses`, `auditable< O , S >`,
`masked< O >`, `lazy< O >` for an observer that queues the events and works at
its next `flush()`, and `counted< O >` for one that reports counts of its own
by `counts()`; `observer_of< O , L >` holds when $O$ observes at least one
event of the list $L$, and an event with an inverse (cf. `UndoLog` below)
satisfies `invertible< E >`. The kinds of an event, as seen by the coalescing
of a batch, are the values of the enumeration `mod::Role`, and
`mod::variant_t< L >` is the `std::variant` of the events of $L$, i.e., the
type `Event` of a subject and the type of the buffer of a batch. A subject
policy (`NoObservers`, `Static< O... >`, `Dynamic< L >` or `Both`) gives the
subject of a structure of the events $L$ as its member template `bind< L >`.
Two lists complete it, `requires_modifications`, of the events without which
the observer cannot work, and `depends_on`, of the static observers that must
precede it. An `on()` may also return `Keep::Detach` to leave with no
exception. We remark that a wrong attachment does not compile. This is the
case of an object with an `on()` for none of the events, of one that requires
an event the structure never emits, and of one whose `on()` of an erasure, a
clearing, a rebuilding or a replacement may throw, since an observer left half
updated by these could not be repaired.

The observers that listen are a parameter of the type of the structure, its
*subject policy* ([Alexandrescu
2001](Z-bibliography.md#ref-alexandrescu2001)), which takes one of four forms.
`NoObservers`, the default, is an empty member, and `Static< O... >` holds
references to the observers, which are called directly in the order of the
arguments. `Dynamic<>` holds the observers attached at run time by `attach()`,
each kept as a pointer together with the pointers to the functions that
restore its type, while `Both< Static< O... > , Dynamic<> >` combines the two
and calls the static ones first. Before a mutator builds the event, or reads
the old state that only the event needs, it tests `listened< E >`, a constant
of the subject, and then `live< E >()`, i.e., whether someone wants $E$ now.
With `NoObservers`, and with static observers that do not want $E$, the first
test is false at compile time; hence, the change is the code of the plain
structure. With `Dynamic<>` and nobody attached, the change costs a load and a
test of the union of the masks, since the path that sends the event is a cold
function out of line. We check both claims with the script
`bench/zerocost.py`, by the instructions that callgrind counts and by the code
of the changes, compared byte for byte. `GridGraph` takes the subject as a
parameter of its own, for the changes of the costs of its cells
([§14.13.3](14-graph.md#sec-14-13-3)). A structure whose code cannot change
(e.g., one of another library) is wrapped instead, as `ObservedGraph` wraps
the graphs ([§14.13.1](14-graph.md#sec-14-13-1)); `JoinTree`, instead, tells
its changes from inside its mutators ([§8.12.7](08-tree.md#sec-8-12-7)).

`batch( s )` opens a group of RAII on the structure $s$. Its events are
collected until the outermost group closes (the nested ones only count the
depth), and then they are sent coalesced into their *net effect*, as one span
per observer of the kinds that it wants, between its group hooks. We model the
structure as a partial map $\sigma$ from the handles to the states of the
items; the net effect of a batch $B$ is then a sequence that leads from the
state before $B$ to the state after it. The coalescing drops everything before
the last `Cleared` or `Rebuilt` and keeps the global events in place, adding
up consecutive shifts and merging consecutive replacements. Between two global
events, it reduces the events of each handle by pushing them on a list $N$ of
the handle, with the rules

$$
\begin{array}{rcl}
N \cdot \mathrm{Inserted}( h , s ) \cdot \mathrm{Erased}( h , s ) & \to & N , \cr
\mathrm{Inserted}( h , s ) \cdot \mathrm{Changed}_A( h , a , b ) & \to & \mathrm{Inserted}( h , s[ A := b ] ) , \cr
\mathrm{Changed}_A( h , a , b ) \cdot \mathrm{Changed}_A( h , b , c ) & \to & \mathrm{Changed}_A( h , a , c ) , \cr
\mathrm{Changed}_A( h , a , b ) \cdot \mathrm{Erased}( h , s ) & \to & \mathrm{Erased}( h , s[ A := a ] ) , \cr
\mathrm{Erased}( h , s ) \cdot \mathrm{Inserted}( h , t ) & \to & \textstyle\prod_{A : s_A \ne t_A} \mathrm{Changed}_A( h , s_A , t_A ) ,
\end{array} \tag{1.15}
$$

where the state $s$ of an item is its key and its value, the second rule
holds for the key and the value, the product of the fifth runs over these
two attributes (and has thus at most two factors), the third vanishes
when $a = c$, and a change with $a = b$ (a touch) is dropped. The changes of
different attributes of one handle commute. Note that the last rule applies
only when the structure emits the changes that it needs; otherwise, the
erasure and the insertion stay. In the third rule $c$ is the value of the
*reduction* of the attribute, which is the last value by default, and the
least, the largest or the sum of the differences for an attribute that declares
`reduce::min`, `reduce::max` or `reduce::sum`. These are the reductions of
the parallel Boost Graph Library ([Gregor and Lumsdaine
2005](Z-bibliography.md#ref-gregor2005)) and the duplicate operator of
GraphBLAS ([Davis 2019](Z-bibliography.md#ref-davis2019)). A structure with a
rule of its own specializes `coalesce_traits`, as the graphs do.

<span id="lem-1-14"></span>**Lemma 1.14.** *Let $B$ be a history of a
structure (a sequence of events each of which starts from the state that
the previous ones leave) whose attributes have the reduction `last`, and $C$
its net effect by (1.15). Then $C$ is a history that leads from the state
before $B$ to the state after it, $\lvert C \rvert \le \lvert B \rvert$,
and $C$ is built in $O( \lvert B \rvert )$ expected time when the number of
kinds of event is a constant.*

*Proof.* The events after the last `Cleared` or `Rebuilt` start from any
state; hence, the part before it does not change the state after $B$.
Between two global events, the events of different handles commute, since
each reads and writes the state of its handle only. Thus, it suffices
that each rule of (1.15) replace two steps at one handle by steps from the
same state to the same state, which one checks rule by rule. The first rule
goes from $\bot$ to $s$ and back, the second inserts the state after the
change, and the third composes two changes of one attribute; the fourth
erases from the state before the change, with the attribute restored, and
the fifth goes from $s$ to $t$ by one change per attribute that differs. No
rule leaves more events than it takes. A list $N$ holds at most one
insertion or erasure and one change per attribute, plus an erasure and an
insertion when the changes are missing (i.e., a constant number of events),
and a hash table on the handles finds the list of each event in $O( 1 )$
expected time. $\square$

The global events stay in place, instead of going last, since an item
inserted with an absolute key after a shift would otherwise be shifted
twice; for the same reason, a relocation and a permutation, which change the
meaning of the handles, bound the runs. The bulk events carry spans, which a
batch copies; hence, the caller may reuse its arrays.

The events have three further uses, i.e., (i) the safety under exceptions,
(ii) the suppression of the events, and (iii) the versions and the audits. As
for (i), outside a batch an insertion and a change are sent after they are
made, and an erasure before it. If an `on()` throws, `emit( e , undo )` sends
`inverse( e )` to the observers already informed, in the reverse order, then
calls the undo of the structure and lets the exception go on. This is the
strong guarantee of [Abrahams 2000](Z-bibliography.md#ref-abrahams2000),
provided that the undo does not throw. `prepare()` gives the same guarantee
with no undo, since it refuses (by `Refused`) before anything changes, as the
indices of Boost.MultiIndex do. A batch sends its events only at its close;
hence, a batch left by an exception is discarded. The structure then goes back
to its checkpoint if it has `checkpoint()` and `rollback()` (also called by
`Batch::rollback()`), and otherwise the observers get `Rebuilt`. Furthermore,
`UndoLog< L >` records the events, which carry the old states, and brings any
structure with `apply()` back to one of its checkpoints by the inverses in the
reverse order, in one batch (a hole in the log at an event with no inverse,
such as the deletion of a node of a graph); the graph wrapper is undoable in
this way within its batches ([§14.13.4](14-graph.md#sec-14-13-4)). As for
(ii), `quiet( s )` suspends the events while it lives and sends `Rebuilt` at
its end, while `quiet( s , no_rebuild )` is the promise that nobody needs to
know, checked by the audits of the observers at its end. Inside a batch, the
events collected before a mute are replaced by `Rebuilt`. As for (iii), the
subject keeps a revision per kind of event (the events of that kind sent to
someone) and `version()`, their sum. A lazy observer, which queues the events
and works at its next query, can therefore tell by one comparison whether
anything it depends on changed. At the level 3 of the contracts
(cf. [§1.7.1](01-introduction.md#sec-1-7-1)), the subject calls `audit()` of
each observer after each event outside a batch and after each batch, and it
counts the events, the batches and the rebuildings that each observer
receives. Finally, `Shadow< Inc , Scratch >` sends every event to an
incremental observer and to one that computes from scratch, and compares their
answers at each query, i.e., the audit by recomputation of [Frangioni and
Manca 2006](Z-bibliography.md#ref-frangioni2006). An observer that keeps a
solution decides between a repair and a computation from scratch by a
recompute policy of its own (`ReoptPolicy`, `RatioPolicy` by default), on
counts of work and never on times, and the subject sends it `Rebuilt` in place
of a batch when the policy asks so at the close; we describe it with the
observers of the graphs, in [§14.13.4](14-graph.md#sec-14-13-4).

## 1.14 Capabilities {#sec-1-14}

An algorithm that takes a structure as a parameter of its type may choose its
faster path at compile time from what the structure offers, and
`Capabilities.h` (beyond the book) gives the vocabulary of these choices,
shared by all the areas. A concept of C++20 decides a matter of syntax but not
one of meaning, since a `rank()` that scans the items satisfies the same
concept as one that reads a counter. Hence we distinguish three kinds of
capability. A *syntactic* capability is a concept on members, e.g.,
`splittable< S >` and `joinable< S >` for `split()` and `join2()` of the trees
of [§8.12](08-tree.md#sec-8-12), `undoable< S >` for the `checkpoint()` and
`rollback( t )` that the batches of [§1.13](01-introduction.md#sec-1-13) use,
and `addressable< Q >` for the handles of a priority queue with `decrease()`,
`increase()` and `erase()`. The others of this kind are the bulk operations,
`rank()` and `select()`, `prefetch()`, the operations of the node tables of a
search (`erasable< T >`, `epoch_clearable< T >`) and those of the graphs
(`in_arcs< G >`, `batch_updatable< G >`). A *semantic* capability, which the
syntax cannot show, is an explicit opt-in of the structure, as
`std::ranges::enable_borrowed_range` is in the standard library. It is a
variable template that the structure specializes, such as
`enable_stable_references< S >` (the items keep their addresses, a handle may
be a pointer and the structure never emits `Relocated`) and
`nonmonotone_values< T >`, or a static member that it declares, such as
`S::monotone`, the only direction in which its values move, by which an
observer compiles out the path of the other direction. A structure that
declares none of them gets the conservative answer. We remark that a concept
whose members no structure of ADS has yet (e.g., `shiftable_keys< Q >` or
`batch_extract< Q >`) is defined all the same, since it is the name under
which the next structure will be found.

A third kind is a property that a wrapper must pass on without knowing it,
because a wrapper written today cannot list the properties of the structures
of tomorrow. We call it a *query*, `s.query( tag )`, after the environments of
P2300 ([Dominiak et al. 2024](Z-bibliography.md#ref-dominiak2024)). Its tag
derives from `forwarding_query` (`stable_references_t`, `thread_safe_reads_t`,
`rank_complexity_t` or `event_set_t`), the structure answers it by a static
member `query( tag )` if it has one, the tag gives the default otherwise, and
`query_v< S , Tag >` reads the answer at compile time. `ObservedGraph`
forwards every query to the graph it wraps, except the events, which are those
of the wrapper. In turn, `Shadow` composes the answers of its two observers by
the meet of the tag, i.e., the conjunction for the stable references and for
the reads that may run with writes, and the worse of the two classes of time
for `rank()`. The customization point objects in the form of [Niebler
2015](Z-bibliography.md#ref-niebler2015) serve only the operations that a type
which ADS does not own may lack, each with a default (e.g., `reserve( s , n )`
does not act on a `std::list`, and `insert_bulk( s , r )` inserts the items
one by one). Each of them calls the member if there is one, then a free
function found by argument-dependent lookup, and then the default.

Accordingly, we split the PRIORITY QUEUE of [chapter 9](09-priority.md#ch-9)
into two concepts. `PriorityQueue` requires the items, `empty()`, the item of
least priority read and removed, and an insertion with the keys of the order
of the queue (a priority, one or two keys, or none for items ordered by
themselves). It reads them through the customization points `queue_min()`,
`queue_deleteMin()` and `queue_insert()`, which try the members of the book
first and the spelling of the standard library last; hence `DaryHeap`,
`BucketQueue`, `RadixHeap` and `std::priority_queue` model it as they are.
`AddressablePriorityQueue` adds the handles of the book and `decrease()`, and
it is the former concept, with the same meaning for the queues with handles.
How a queue lowers a key is then a capability in its turn, which
`decrease_key_of< Q >()` returns: `native` when `queue_decrease( q , h , p )`
takes its handles (the member `decrease()` of a queue that declares a
`Handle`, else a free function found by argument-dependent lookup, the handle
type being `queue_handle_t< Q >`), `lazy` for a `PriorityQueue` without
them, and `none` otherwise. The customization point has no step on the
spelling of another library, since Boost.Heap calls `increase()` the move
of an item toward the top of its heap, i.e., what ADS calls a decrease
([§1.8.1](01-introduction.md#sec-1-8-1)). On a lazy
queue the item is inserted again with the lower key, and its stale copies are
skipped when they come out. An opt-in `Q::decrease_key` wins over the
inference, and `MultiQueue`, a relaxed queue with no exact `min()`, declares
by it that it is lazy ([Algorithm 1.8](01-introduction.md#alg-1-8)).

<span id="alg-1-8"></span>**Algorithm 1.8.** The capability of a queue to
lower a priority, computed at compile time.

--8<-- "generated/Capabilities-decrease_key_of.pseudo.md"

Neither form is better in general. A lazy queue needs no handles and no
table of positions, at the price of one copy per decrease instead of one
per item, and on sparse graphs it is often the faster of the two ([Chen et
al. 2007](Z-bibliography.md#ref-chen2007)). Hence the frontier of the
shortest paths ([§14.8](14-graph.md#sec-14-8)) and Prim's algorithm take
either form; in the lazy one they keep, for each node, whether it is in
the frontier, and they skip a copy of a node that is not.

<span id="lem-1-15"></span>**Lemma 1.15.** *Let the priorities of the
copies be the pairs $( d , v )$ of a distance and a node, where a copy of
$v$ is inserted at each fall of its distance. Then the lazy frontier extracts
the same nodes, in the same order and with the same counts, as a native
one.*

*Proof.* The distance of a node only falls, and therefore the copy of $v$
inserted last has the least priority among the copies of $v$ in the queue;
while $v$ is in the frontier, that copy carries the priority $( d_v , v )$
which the native queue holds. When $v$ leaves the frontier, its other
copies have larger priorities and are skipped, and if $v$ enters it again
(with negative weights) its new copy is again the least of its copies.
Hence, at each extraction, the least copy of a node in the frontier is the
least entry of the native queue (the pairs are distinct), and the stale
copies before it are skipped and not counted; the additions and the
decreases are the calls of the prototype, which are the same in both.
$\square$

[Table 1.7](01-introduction.md#tab-1-7) lists all the names of
`Capabilities.h`, with what each one requires or answers; the
customization points `reserve`, `prefetch` and `insert_bulk`, and the
object `query`, are in the namespace `ADS`, as the concepts are.

<span id="tab-1-7"></span>**Table 1.7.** The names of `Capabilities.h`:
S marks a syntactic capability (a concept on members), O an opt-in, Q a
query or its tools, T a type or a customization point; $S$ is a
structure, $Q$ a queue, $T$ a table of nodes and $G$ a graph.

| Name | Kind | Requires or gives |
|---|---|---|
| `mod::list<E...>` | T | a list of event types (cf. [§1.13](01-introduction.md#sec-1-13)) |
| `key_type_t<S>` | T | `S::Key`, else `S::key_type`, else `S::value_type`, else `std::size_t` |
| `modifications_of<S>` | T | `S::modifications`, else the empty list |
| `priority_t<Q>` | T | `Q::Priority`, else the items of $Q$ |
| `enable_stable_references<S>` | O | the items keep their addresses (false by default) |
| `nonmonotone_values<T>` | O | the values of the nodes may also rise (false by default) |
| `monotone_of<S>()`, `declares_monotone<S>` | O | the bits `up` and `down` of `S::monotone`, all bits by default |
| `forwarding_query`, `forwarding_query_tag<T>` | Q | the base of the tags, and the concept of a tag |
| `stable_references_t`, `thread_safe_reads_t`, `rank_complexity_t`, `event_set_t` | Q | the four tags, and the objects `stable_references`, `thread_safe_reads`, `rank_complexity`, `event_set` |
| `complexity` | Q | `constant`, `logarithmic`, `linear`, the answer of `rank_complexity_t` |
| `query_v<S, T>`, `query` | Q | the answer of $S$ to the tag $T$, at compile time and as an object |
| `meetable_query<T>`, `query_meet_v<A, B, T>` | Q | a tag with a meet, and the answer of a composition |
| `reserve`, `prefetch`, `insert_bulk` | T | the member, else a free function found by ADL, else the default |
| `bulk_insertable<S>`, `bulk_erasable<S>` | S | `insert_bulk(r)`, `erase_bulk(r)` |
| `splittable<S>`, `joinable<S>` | S | `std::move( s ).split( k )`, `S::join2( l , r )` |
| `rankable<S>`, `selectable<S, G>` | S | `rank(k)`, `select(i)` (and `augSelect(p)` with an augmentation $G$) |
| `prefetchable<S>`, `reservable<S>` | S | `prefetch(k)`, `reserve(n)` |
| `augmented_by<S, G>`, `range_actionable<S, A>` | S | `augRange(lo, hi)` with `S::Augmented` equal to $G$, `applyRange(lo, hi, a)` |
| `decrease_key_kind`, `decrease_key_of<Q>()` | T | `none`, `lazy`, `native` ([Algorithm 1.8](01-introduction.md#alg-1-8)) |
| `addressable<Q>` | S | an `AddressablePriorityQueue` with `increase(h, p)` and `erase(h)` |
| `observes_values<Q>` | S | `update(id)`, the key of an item read again from the table |
| `batch_extract<Q>`, `shiftable_keys<Q>` | S | `extract(theta, out)`, `shift_all(delta)` |
| `monotone_extract<Q>`, `rekeyable<Q, F>` | S | a `PriorityQueue` with `rebase()`, `rekey(f)` |
| `erasable<T>`, `epoch_clearable<T>` | S | `erase(k)`, the policy `T::Clear` with `epoch()` |
| `undoable<S>`, `snapshotable<S>` | S | `rollback( s.checkpoint() )`, `snapshot()` |
| `in_arcs<G>`, `stable_edge_ids<G>`, `batch_updatable<G>` | S | `forEachIn(v, f)`, `arcId(u, v)`, `batch()` |

## 1.15 Structures that tell their changes {#sec-1-15}

We now describe the first structures of ADS that emit the events of
[§1.13](01-introduction.md#sec-1-13) from inside their mutators, namely the
indexed heap of [§9.5](09-priority.md#sec-9-5) and an array. Such events
feed aggregates, of which we give two, the prefix aggregates of an array and
the aggregate of a sliding window. Each aggregate is written once for any
associative operation, and the rule by which it absorbs a change is chosen
at compile time from the algebra of the operation, which `Algebra.h` names
by concepts in the manner of `Capabilities.h`
([§1.14](01-introduction.md#sec-1-14)).

### 1.15.1 The indexed heap and the observable array {#sec-1-15-1}

`IndexedDaryHeap< K , D , Policy >` takes a subject policy as its last
parameter, `NoObservers` by default, and declares the events
`Inserted< int , K >`, `Erased< int , K >`, `KeyChanged< int , K >`,
`Relocated< std::size_t >`, `Cleared` and `Rebuilt`. Its ids are the handles
of the events, which are stable while an id is in the heap. Hence, an observer
that needs the keys only (e.g., an index of the open nodes of a search,
usually by another order) may follow the heap through the first three events,
which `push()` sends, `erase()` and `pop()` send before the removal, and
`decrease()`, `increase()` and `update()` send. The places of the vector are
internal maintenance of the heap, which never travels on the optional channel;
`Relocated` carries them only to an observer that wants them, since otherwise
each level of a walk would build an event. We remark that a relocation is an
exchange, i.e., `Relocated( p , q )` swaps the contents of the places $p$ and
$q$, an empty place included. Also, the event of the item comes before those
of its walk: the new item is at the place $s$ after the last one, the key
changes where the item is, and an erased item leaves its place empty.

<span id="lem-1-16"></span>**Lemma 1.16.** *Let an observer keep a copy $M$ of
the vector of the ids of the heap, place an inserted id at the place $s$,
empty the place of an erased id and exchange $M[p]$ and $M[q]$ at each
`Relocated( p , q )`, dropping the empty places at the end. Then, after each
operation, $M$ is the vector of the ids of the heap, and the operation sends
at most $\lceil \log_D ( ( D - 1 ) s + 1 ) \rceil + 1$ events.*

*Proof.* A walk up from the place $i$ with the item $x$ moves the parent
$p$ of $i$ down to $i$ and goes on from $p$. If we think of $x$ as being at
$i$, this is the exchange of $x$ with its parent, and the walk down is the
same with the least child. `push()` puts $x$ at the place $s$ and walks up
from it, while a change of key walks from the place of the item. An erasure
(and `pop()`, the erasure of the root) empties the place $i$ of the item,
then moves the last item from the place $s - 1$ to $i$ and walks from
there. This move is the exchange of the last item with the empty place,
which the first `Relocated` carries. Hence the events of an operation are,
in order, the exchanges whose composition is the change of the vector, and
their number is that of the levels walked, i.e., at most the height of the
heap ([§9.4](09-priority.md#sec-9-4)), plus one. $\square$

The walk that emits is a second instance of the template of the walk, called
only on the path on which someone wants `Relocated`. As in the other
mutators, each operation of the heap tests `listened` of its events before it
tests `live`, and with `NoObservers` both tests vanish at compile time. By
the check of zero cost of `bench/zerocost.py` we compare the heap of today
with the heap of the tree before the subject policy. With `NoObservers` the
instructions that callgrind counts for one update, one `pop()` and one
`push()` are the same in the two trees (389.07 with GCC 13.3, 340.19 with
Clang 18.1), and the code of `push()`, `decrease()`, `increase()`, `erase()`
and `pop()` is the same byte for byte, the padding after the last instruction
excluded.

The simplest structure that emits is `ObservableArray< T , Policy >`, a vector
whose writes go through the subject policy, with the index as the handle and
as the key. `set(i, v)` and `add( i , d )` send `Assigned` with the value
before and the value after, `push_back()` sends `Inserted`, and `pop_back()`
sends `Erased` before the removal; `clear()` and `assign()` send `Cleared` and
`Rebuilt` ([Algorithm 1.9](01-introduction.md#alg-1-9)). Its reads are those
of the vector, and `apply( e )` makes the change of an event, by which an
`UndoLog` of the array brings it back to a checkpoint. The array inserts and
erases at its end only; hence, an index never changes meaning between two
events of a batch, and the coalescing of (1.15) applies as it does to a
structure with stable handles (cf. [Lemma 1.14](01-introduction.md#lem-1-14)).
For instance, two writes of one index become one `Assigned`, while an append
and a removal cancel.

```cpp
// declarations of <ADS/sequence/ObservableArray.h>
template< class T , class Policy = NoObservers >
class ObservableArray
{
 public:
 using Assign = mod::Assigned< std::size_t , T >;
 using Insert = mod::Inserted< std::size_t , std::size_t , T >;
 using Erase = mod::Erased< std::size_t , std::size_t , T >;
 template< class... O >
 explicit ObservableArray( std::size_t n = 0 , const T & v = T() , O &... o );
 std::size_t size( void ) const;
 const T & operator[]( std::size_t i ) const;
 const std::vector< T > & values( void ) const;
 void set( std::size_t i , const T & v );
 void add( std::size_t i , const T & d );
 void push_back( const T & v );
 void pop_back( void );
 void clear( void );
 template< std::input_iterator I >
 void assign( I first , I last );
 void apply( const Assign & e );
 Subject & subject( void );
 template< class O >
 int attach( O & o )
  requires( Subject::dynamic );
 };
```

<span id="alg-1-9"></span>**Algorithm 1.9.** A write of the observable
array, in the pattern of the mutators of [§1.13](01-introduction.md#sec-1-13).

--8<-- "generated/ObservableArray-set.pseudo.md"

With `NoObservers` a write of the array is that of the vector it holds: the
check of zero cost counts 10 instructions per write with GCC and 6 with
Clang, as for a plain `std::vector`, the code of `set()` is the same byte
for byte, and the same holds with a static observer that does not want
`Assigned`; with `Dynamic<>` and nobody attached, a write costs 14 and 15
instructions, the load and the test of the union of the masks included.

### 1.15.2 The algebra of the aggregates {#sec-1-15-2}

An aggregate is the combination $a_1 \oplus \cdots \oplus a_n$ of some values
by an associative operation with an identity $e$, and its update after a
change depends on the algebra of $\oplus$ rather than on the structure that
keeps it. `Algebra.h` (beyond the book) names the algebras by concepts on the
static members of a type `G`; they refine the augmentations of `JoinTree`
([§8.12](08-tree.md#sec-8-12)), which are monoids as they are, and they are
those by which `assign()` of the tree and `MaterializedAggregate` choose their
updates ([§8.12.7](08-tree.md#sec-8-12-7)). A `Monoid` has `identity()` and
`combine()`, and a `CommutativeMonoid` declares the flag `commutative`. A
`Group` has `inverse()`, an `AbelianGroup` is both, and a `Selective` monoid
declares `selective`, the promise that $a \oplus b$ is always $a$ or $b$
(e.g., the least of two values). An `Action< A , G >` is a monoid of maps on
the values of $G$, with the identity map, the composition and the application
of a map to the aggregate of $n$ values, i.e., the ingredients of the lazy
updates of a range. Finally, `exact_v< G >` is false for the floating point
values, whose groups accumulate the rounding errors of the inverses. These
flags are opt-ins, since no syntax shows that an operation commutes or
selects. Of course, the concepts do not prove the laws; the tests check them
on `SumOf< T >` (an abelian group), on `MinOf< T >` and `MaxOf< T >`
(selective commutative monoids), and on operations that do not commute.

```cpp
// declarations of <ADS/Algebra.h>
template< class G >
concept Monoid = requires( const typename G::Value & a )
{ ... };
template< class G >
concept CommutativeMonoid = Monoid< G > && detail::commutativeFlag< G >();
template< class G >
concept Group = Monoid< G > && requires( const typename G::Value & a )
{ ... };
template< class G >
concept AbelianGroup = Group< G > && CommutativeMonoid< G >;
template< class G >
concept Selective = Monoid< G > && detail::selectiveFlag< G >();
template< class G >
inline constexpr bool exact_v =
  ! std::is_floating_point_v< typename G::Value >;
template< class T >
struct SumOf
{
 using Value = T;
 static constexpr bool commutative = true;
 static constexpr Value identity( void );
 static constexpr Value combine( const Value & a , const Value & b );
 static constexpr Value inverse( const Value & a );
 };
template< class T >
struct MinOf
{
 using Value = T;
 static constexpr bool commutative = true;
 static constexpr bool selective = true;
 static constexpr Value identity( void );
 static constexpr Value combine( const Value & a , const Value & b );
 };
template< class T >
struct MaxOf
{
 using Value = T;
 static constexpr bool selective = true;
 static constexpr Value identity( void );
 static constexpr Value combine( const Value & a , const Value & b );
 };
```

<span id="lem-1-17"></span>**Lemma 1.17.** *Let $c = x \oplus a \oplus y$
be an aggregate whose operand $a$ becomes $b$, and
$c' = x \oplus b \oplus y$. Then (i) in an abelian group,
$c' = c \oplus a^{-1} \oplus b$; (ii) in a commutative monoid, if
$b = a \oplus d$ then $c' = c \oplus d$; (iii) in a group, if $x = e$ then
$c' = b \oplus a^{-1} \oplus c$, and if $y = e$ then
$c' = c \oplus a^{-1} \oplus b$; (iv) in a selective monoid, the aggregate
of any values is one of them, or $e$ if there is none.*

*Proof.* (i) By commutativity $c = a \oplus x \oplus y$, and therefore
$c \oplus a^{-1} \oplus b = x \oplus y \oplus b = x \oplus b \oplus y$.
(ii) Likewise, $c \oplus d = x \oplus y \oplus a \oplus d = x \oplus ( a
\oplus d ) \oplus y$. (iii) With $x = e$, $b \oplus a^{-1} \oplus c = b
\oplus a^{-1} \oplus a \oplus y = b \oplus y$, by associativity alone, and
symmetrically with $y = e$. (iv) By induction on the number of operands,
since each combination is one of its two operands. $\square$

Lemma 1.17 is the dispatch of the structures that follow. Its first part is
the update of a prefix sum by its difference, and the second the `add()` of a
commutative monoid (e.g., a maximum that is raised). The third is the removal
of the first value of a window and the insertion of the last one
(subtract-on-evict), which needs no commutativity, and the fourth is the
monotone deque of a window of minima. A monoid with none of these properties
can only recombine the parts that a change touches. For a group whose values
are not exact, the inverse does not undo the rounding of the combination, and
the aggregate may drift. In this case the structures recompute it from the
values after `period()` updates, at a cost of $O( 1 )$ amortized per update
when the period is at least the number of values, while the audits of the
tests catch a drift beyond their tolerance.

### 1.15.3 Prefix aggregates {#sec-1-15-3}

`Fenwick< G >` keeps the values $a_0 , \ldots , a_{n-1}$ of a monoid $G$ and,
for $j = 1 , \ldots , n$, the nodes

$$
t_j = a_{j - \lambda( j )} \oplus \cdots \oplus a_{j - 1} , \tag{1.16}
$$

where $\lambda( j )$ is the lowest bit of $j$, i.e., the largest power of 2
that divides it ([Fenwick 1994](Z-bibliography.md#ref-fenwick1994)). For any
monoid the tree answers `prefix( r )`, the aggregate
$\pi_r = a_0 \oplus \cdots \oplus a_{r-1}$ ([Algorithm
1.10](01-introduction.md#alg-1-10)). For a group it also answers
`range( l , r )` $= \pi_l^{-1} \oplus \pi_r$, which needs no commutativity
since $\pi_r = \pi_l \oplus ( a_l \oplus \cdots \oplus a_{r-1} )$. A value
changes by `set()`, by `add()` in a commutative monoid, and at the end by
`push_back()` and `pop_back()`.

```cpp
// declarations of <ADS/sequence/Fenwick.h>
template< Monoid G >
class Fenwick
{
 public:
 using Value = typename G::Value;
 explicit Fenwick( std::size_t n = 0 );
 template< std::input_iterator I >
 Fenwick( I first , I last );
 std::size_t size( void ) const;
 const Value & get( std::size_t i ) const;
 Value prefix( std::size_t r ) const;
 Value range( std::size_t l , std::size_t r ) const
  requires Group< G >;
 void set( std::size_t i , const Value & v );
 void add( std::size_t i , const Value & d )
  requires CommutativeMonoid< G >;
 void push_back( const Value & v );
 void pop_back( void );
 template< std::input_iterator I >
 void assign( I first , I last );
 std::size_t period( void ) const;
 void period( std::size_t p );
 };
```

<span id="alg-1-10"></span>**Algorithm 1.10.** The prefix of length $r$, by
the nodes of the binary digits of $r$.

--8<-- "generated/Fenwick-prefix.pseudo.md"

<span id="lem-1-18"></span>**Lemma 1.18.** *(i) For $0 \le r \le n$, the
sequence $j_1 = r$, $j_{k+1} = j_k - \lambda( j_k )$ reaches 0 after at most
$\lfloor \log_2 r \rfloor + 1$ steps, and
$\pi_r = t_{j_m} \oplus \cdots \oplus t_{j_1}$. (ii) For $j \ge 1$, $t_j$ is
the combination of the nodes $t_{j - 2^k}$, $2^k < \lambda( j )$, by
decreasing $k$, and of $a_{j-1}$. (iii) The nodes that contain $a_i$ are
those of the sequence $j_1 = i + 1$, $j_{k+1} = j_k + \lambda( j_k )$, while
$j_k \le n$, and they are at most $\lfloor \log_2 n \rfloor + 1$.*

*Proof.* (i) Each step clears the lowest bit set of $j$, hence the steps are
at most the bits of $r$, and by (1.16) the node $t_{j_k}$ covers the indices
$[ j_{k+1} , j_k )$; these intervals are consecutive and, from $k = m$ down
to 1, they go from 0 to $r$, so that their combination in this order is
$\pi_r$ by associativity alone. (ii) The indices
$[ j - \lambda( j ) , j - 1 )$ split into the intervals
$[ j - 2^{k+1} , j - 2^k )$ for $2^k < \lambda( j )$, since the bits of $j$
below $\lambda( j )$ are zero, and $\lambda( j - 2^k ) = 2^k$, so that each
interval is that of $t_{j - 2^k}$; by decreasing $k$ they go from left to
right, and $a_{j-1}$ comes last. (iii) The value $a_i$ is in $t_j$ if and
only if $j - \lambda( j ) \le i < j$, and therefore the least such $j$ is
$i + 1$. Let $t_j$ contain $a_i$. For $j' = j + m$ with
$0 < m < \lambda( j )$ the lowest bit of $j'$ is that of $m$, and
$j' - \lambda( j' ) \ge j' - m = j > i$, so that $t_{j'}$ does not contain
$a_i$. For $j' = j + \lambda( j )$ the addition carries, hence
$\lambda( j' ) \ge 2 \lambda( j )$ and
$j' - \lambda( j' ) \le j - \lambda( j ) \le i$, so that $t_{j'}$ contains
it; since $\lambda$ at least doubles at each step, the steps are at most
$\lfloor \log_2 n \rfloor + 1$. $\square$

Thus `prefix( r )` takes $O( \log n )$ combinations, and the nodes are built
by (ii) in increasing order in $\sum_{j \le n} \log_2 \lambda( j ) = O( n )$
combinations. A value appended gets its node in $O( \log n )$, with no other
node changed, and the removal of the last value drops its node only.

<span id="alg-1-11"></span>**Algorithm 1.11.** The update of a value, by
the difference in an abelian group and by the parts of the nodes in any
other monoid.

--8<-- "generated/Fenwick-set.pseudo.md"

<span id="lem-1-19"></span>**Lemma 1.19.** *Let $a_i$ become $v$. (i) In an
abelian group, combining each node of [Lemma 1.18](01-introduction.md#lem-1-18)
(iii) with $\delta = a_i^{-1} \oplus v$ gives the nodes of the new values,
in $O( \log n )$ combinations. (ii) In a commutative monoid, if
$v = a_i \oplus d$, the same holds with $\delta = d$. (iii) In any monoid,
recomputing those nodes by [Lemma 1.18](01-introduction.md#lem-1-18) (ii) in
increasing order gives the new nodes, in $O( \log^2 n )$ combinations.*

*Proof.* A node that does not contain $a_i$ does not change, and one that
does is of the form $x \oplus a_i \oplus y$, which (i) and (ii) update by
[Lemma 1.17](01-introduction.md#lem-1-17) (i) and (ii). For (iii), the nodes
that $t_j$ combines have indices less than $j$; hence those among them that
contain $a_i$ are recomputed before $t_j$ when the nodes are taken in
increasing order, while the others are unchanged. Each recomputation takes
$O( \log n )$ combinations, for at most $\lfloor \log_2 n \rfloor + 1$
nodes. $\square$

`FenwickObserver< G >` is the tree of [Algorithm
1.11](01-introduction.md#alg-1-11) kept by the events of an `ObservableArray`
of the values of $G$. It applies `Assigned` by `set()`, `Inserted` and
`Erased` by `push_back()` and `pop_back()`, and `Cleared` by emptying the
tree. `Rebuilt` carries no values; on it the observer marks its tree stale and
ignores the events until its next query, at which it builds the tree again
from the vector of the array, in $O( n )$. For a static policy one builds the
observer before its array, and gives it the array by `rebuild( a )`.

<span id="lem-1-20"></span>**Lemma 1.20.** *After each event sent outside a
batch, and after each batch, the tree of `FenwickObserver` is, at its next
query, the tree of the values of the array.*

*Proof.* By induction on the events received. Outside a batch each event
starts from the state of the array that the observer knows, and Lemmas 1.18
and 1.19 keep the tree equal to that of the array. A batch delivers its net
effect, which by [Lemma 1.14](01-introduction.md#lem-1-14) leads from the
state before the batch to the state after it, with `Cleared` or `Rebuilt`
first if the batch had one. After `Rebuilt` the events are ignored, and at
the next query the tree is built from the array as it is then. This is the
state after the events delivered so far, i.e., the state that the ignored
events would have reached. $\square$

We check Lemma 1.20 by the test `engineered_emitters.fenwick-observer`, i.e.,
by the audit of [§1.13](01-introduction.md#sec-1-13). A `Shadow` sends each
event of an array to the observer and to one that recomputes each prefix by a
scan of its own copy of the values, and compares their answers at each query,
on random writes, appends, removals, batches, clearings and rebuildings. In
the example below, we attach the observer of the prefix sums at run time.

```cpp
ADS::ObservableArray< long long , ADS::Dynamic<> > a( 8 , 1 );
ADS::FenwickObserver< ADS::SumOf< long long > > p( a );
a.attach( p );
a.set( 3 , 10 );                       // Assigned( 3 , 1 , 10 )
a.push_back( 5 );                     // Inserted( 8 , 8 , 5 )
const long long s = p.prefix( 9 );    // 1 + 1 + 1 + 10 + 1 + 1 + 1 + 1 + 5
const long long r = p.range( 3 , 4 );  // 10
```

### 1.15.4 The aggregate of a sliding window {#sec-1-15-4}

`SlidingAggregate< G , Policy >` (in `queue/`) is a QUEUE of values
$x_1 , \ldots , x_n$ of a monoid $G$ with `aggregate()`, the combination
$\alpha = x_1 \oplus \cdots \oplus x_n$ from the front to the back, which the
queues with the minimum of [§4.8](04-queue.md#sec-4-8) and
[§4.9](04-queue.md#sec-4-9) compute for $\oplus = \min$. With the policy
`Amortized`, the default, the algebra chooses the algorithm, i.e.,
subtract-on-evict for a group, the monotone deque for a selective monoid whose
values compare by `==` ([Algorithm 1.12](01-introduction.md#alg-1-12)), and
Two-Stacks Lite ([Tangwongsan et
al. 2021](Z-bibliography.md#ref-tangwongsan2021)) for any other monoid
([Algorithm 1.13](01-introduction.md#alg-1-13)). With `WorstCase`, any monoid
takes the real-time queue of Hood and Melville of `RealTimeMinQueue`, on
stacks whose cells hold aggregates instead of places. Its constant `kind`
names the algorithm chosen.

```cpp
// declarations of <ADS/queue/SlidingAggregate.h>
struct Amortized
{ };
struct WorstCase
{ };
enum class window_kind {
 subtract_on_evict ,
 monotone_deque ,
 two_stacks ,
 real_time
 };
template< Monoid G , class Policy = Amortized >
class SlidingAggregate
{
 public:
 using Value = typename G::Value;
 static constexpr window_kind kind = detail::windowKindOf< G , Policy >();
 bool empty( void ) const;
 std::size_t size( void ) const;
 void enqueue( const Value & x );
 Value dequeue( void );
 const Value & top( void ) const;
 Value aggregate( void ) const;
 template< class F >
 void forEach( F && f ) const;
 void clear( void );
 };
```

<span id="alg-1-12"></span>**Algorithm 1.12.** The insertion into the
monotone deque $C$ of the candidates of a selective monoid.

--8<-- "generated/SlidingAggregate-enqueue.pseudo.md"

<span id="alg-1-13"></span>**Algorithm 1.13.** The removal of Two-Stacks
Lite, whose first part carries the aggregates of its suffixes.

--8<-- "generated/SlidingAggregate-dequeue.pseudo.md"

<span id="lem-1-21"></span>**Lemma 1.21.** *(i) In a group, the aggregate
kept as $s \gets s \oplus x$ at each insertion and $s \gets x_1^{-1} \oplus
s$ at each removal of $x_1$ is $\alpha$. (ii) In a selective monoid, let the
deque $C$ of the candidates lose from its back each $c$ with $c \oplus x =
x$ before $x$ is appended, and lose its front at the removal of the item it
is; then the combination of the candidates from the $l$-th item on is
$x_l \oplus \cdots \oplus x_n$ for each $l$, and the front of $C$ is
$\alpha$. (iii) In any monoid, if the first $m$ items carry the aggregates
$S_k = x_k \oplus \cdots \oplus x_m$ and $b$ is the aggregate of the others,
and a removal with $m = 0$ first makes all the items the first part, then
$\alpha = S_1 \oplus b$. Each operation of (i) costs $O( 1 )$, and those of
(ii) and (iii) cost $O( 1 )$ amortized.*

*Proof.* (i) By induction on the operations from $s = e$: an insertion of
$x$ makes $s = x_1 \oplus \cdots \oplus x_n \oplus x$, and a removal makes
$x_1^{-1} \oplus x_1 \oplus ( x_2 \oplus \cdots \oplus x_n )$, i.e., [Lemma
1.17](01-introduction.md#lem-1-17) (iii). (ii) Let $\Gamma_l$ be the
combination of the candidates from the $l$-th item on. At an insertion of
$x$ each candidate $c$ removed is the last one, so that
$\Gamma_l = \Gamma' \oplus c$ with $\Gamma'$ the candidates before it, and
$x_l \oplus \cdots \oplus x_n \oplus x = \Gamma' \oplus ( c \oplus x ) =
\Gamma' \oplus x$; once no candidate is removed, appending $x$ gives the new
suffix. At a removal, the suffixes from the second item on do not change,
and the first item goes from $C$ if it is there. Two candidates $c , c'$
became consecutive when $c'$ was appended with $c$ last, and then
$c \oplus c' \ne c'$, i.e., $c \oplus c' = c$ by selectivity. Hence,
combining from the back, $\Gamma_1$ is the front of $C$; also, each item
enters and leaves $C$ once. (iii) $S_1$ covers the first part and $b$ the
items after it, in the order of the queue; a removal drops $x_1$ and $S_1$,
and $S_2$ is the aggregate of the first part that remains, while an
insertion extends $b$ on the right. Each item enters the first part once, at
the cost of one combination, which the potential $n - m$ pays. $\square$

<span id="lem-1-22"></span>**Lemma 1.22.** *Let the queue be kept as
`RealTimeMinQueue` keeps it ([§4.9](04-queue.md#sec-4-9)), with the fronts
$F$ and $N$ and the backs $R$ and $R'$, where each cell of a front holds
$x \oplus \sigma$ and each cell of a back holds $\sigma \oplus x$, $x$ its
item and $\sigma$ the aggregate of the cells below it. Let $\hat P$ be the
aggregate in the top cell of the part $P$, $r_0$ the size of $R'$ when the
rotation started, and $\hat N_k$ the aggregate of the cell $k - 1$ of $N$,
$k = \min( r_0 , \lvert N \rvert )$. Then
$\alpha = \hat F \oplus \hat R' \oplus \hat N_k \oplus \hat R$, the empty
parts left out, and each operation costs $O( 1 )$ in the worst case.*

*Proof.* Indeed, the cost is that of the rotation of
[§4.9](04-queue.md#sec-4-9), with one combination more per move. The top cell
of $F$ holds the aggregate of the cells from 0 to its top, in the order of
the queue, since the aggregate was computed when the cells were pushed and a
move does not change it. This includes the bottom cells already moved into
$N$, whose items are still in the queue right after those that remain, and
excludes the items removed from the top. In turn, the top of $R'$ covers the
items of the frozen back not yet moved, which precede in the queue those that
were moved. The latter are the first $k$ cells of $N$, with the last item of
the back at the bottom; hence, the aggregate of the cell $k - 1$, with the
item first, combines them in the order of the queue. Note that the cells of
$N$ above $r_0$ hold items of $F$, already counted in $\hat F$, and the new
back $R$ follows. When the rotation ends, $N$ becomes the front, and its top
cell covers all its items. $\square$

The minimum of `RealTimeMinQueue` is Lemma 1.22 for the minima, where the
parts may overlap since $\min$ is idempotent; this is why it may read the
least place of $F$ in $N$. `MinQueue` and `RealTimeMinQueue` stay as they
are, since they also take an order that is an object, not a type, and give
the least item by reference, which a monoid whose combination returns a
value cannot do. We check by the test `engineered_emitters.sliding-minqueue`
that the windows of the minima agree with them. In the example below the monoid
of the concatenations does not commute, and the window takes Two-Stacks
Lite.

```cpp
struct Concat
{
 using Value = std::string;
 static Value identity( void ) { return( Value() ); }
 static Value combine( const Value & a , const Value & b ) { return( a + b ); }
 };
ADS::SlidingAggregate< Concat > w;
w.enqueue( "ab" );
w.enqueue( "c" );
w.dequeue();
w.enqueue( "d" );
const std::string s = w.aggregate();  // "cd"
static_assert( ADS::SlidingAggregate< Concat >::kind ==
               ADS::window_kind::two_stacks );
static_assert( ADS::SlidingAggregate< ADS::SumOf< int > >::kind ==
               ADS::window_kind::subtract_on_evict );
```

## 1.16 Discussion and references {#sec-1-16}

The abstract data types go back to [Liskov and Zilles
1974](Z-bibliography.md#ref-liskov1974), after the principle
of the hiding of information of [Parnas
1972](Z-bibliography.md#ref-parnas1972). Their algebraic
specification, with the sufficient completeness of [Lemma
1.1](01-introduction.md#lem-1-1), goes back to [Guttag and
Horning 1978](Z-bibliography.md#ref-guttag1978). Our
specification by $( D , F , A )$ and the realization of the
types in C++, with the requirements of abstraction and
protection, follow [Montresor 2025,
§3.2](Z-bibliography.md#ref-montresor2025) and [Cadoli et al.
1997](Z-bibliography.md#ref-cadoli1997), while the generic
programming on which the concepts of C++20 rest is that of
[Musser and Stepanov 1989](Z-bibliography.md#ref-musser1989),
and the design of libraries of concepts that of [Sutton and
Stroustrup 2012](Z-bibliography.md#ref-sutton2012). The
interfaces LIST, USet and SSet of [Morin 2013,
§1.2](Z-bibliography.md#ref-morin-cpp) are simplified
versions of those of the Java Collections Framework, and ADS
names them as the standard library of C++ does. Our contracts
follow [Hoare 1969](Z-bibliography.md#ref-hoare1969) and
[Meyer 1992](Z-bibliography.md#ref-meyer1992), and their
levels the proposal of the contracts of C++26 ([Berne et al.
2025](Z-bibliography.md#ref-berne2025)). For the mathematics
of [§1.5](01-introduction.md#sec-1-5) one may see [Graham,
Knuth and Patashnik 1994](Z-bibliography.md#ref-graham1994)
and [Lehman, Leighton and Meyer
2018](Z-bibliography.md#ref-lehman2018), and, for the
probability in the analysis of algorithms, [Motwani and
Raghavan 1995](Z-bibliography.md#ref-motwani1995), while our
asymptotic notation is that of [Knuth
1976](Z-bibliography.md#ref-knuth1976). [Hagerup
1998](Z-bibliography.md#ref-hagerup1998) surveys the word-RAM
and its algorithms, the amortized analysis is that of [Tarjan
1985](Z-bibliography.md#ref-tarjan1985), and the models of
the memory hierarchy are those of [Aggarwal and Vitter
1988](Z-bibliography.md#ref-aggarwal1988) and [Frigo et al.
2012](Z-bibliography.md#ref-frigo2012). The
iterators and their categories are those of the standard
library ([Stroustrup 2013, ch.
33](Z-bibliography.md#ref-stroustrup2013)). Finally, the
typed modifications carry the old values in the events, as the
change propagation of [Acar et al.
2002](Z-bibliography.md#ref-acar2002) does with its traces;
their exception protocol is that of the notifier of LEMON
([Dezső et al. 2011](Z-bibliography.md#ref-dezso2011)), with the
rollback of an observer that throws; the counts of each observer
follow the measures of the kinetic data structures of [Basch et
al. 1999](Z-bibliography.md#ref-basch1999); and the dispatch on
a closed set of events by its index replaces the cascade of casts
whose cost [Solodkyy et al. 2012](Z-bibliography.md#ref-solodkyy2012)
measured. The capabilities follow the customization points of [Niebler
2015](Z-bibliography.md#ref-niebler2015) and the forwarded queries of
[Dominiak et al. 2024](Z-bibliography.md#ref-dominiak2024), and the lazy
decrease of a key is compared with the native one in [Chen et al.
2007](Z-bibliography.md#ref-chen2007).

## 1.17 Exercises {#sec-1-17}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Solve the nine problems on the lines of a text of [Morin 2013, exercise
   1.1](Z-bibliography.md#ref-morin-cpp), each with the interface it calls
   for. They are (i) the lines in the reverse order, with a stack; (ii) the
   blocks of 50 lines each reversed, keeping at most 50 lines, with a stack
   of 50 places; (iii) at each blank line, the line 42 lines before it,
   keeping at most 43 lines, with a queue; (iv) the first occurrence of each
   line and (v) the later ones, in space proportional to the distinct
   lines, with a USet (here a `Bag`); (vi) the lines sorted by length and
   then in order, each once, with an SSet, and (vii) with their
   repetitions, with a bag; (viii) the lines of even rank and then those of
   odd rank, with two queues; (ix) the lines in a random order, with a
   random queue. Test: `ods_lists.ods-ex1.1.lines`.
2. Describe the relation between the Dyck words and the operations
   `push()` and `pop()` of a stack ([Morin 2013, exercise
   1.2](Z-bibliography.md#ref-morin-cpp); [Lemma
   1.2](01-introduction.md#lem-1-2)). Test: `ods_lists.ods-ex1.2.dyck`.
3. Decide with a stack whether a string of $n$ brackets `()`, `[]` and
   `{}` is matched, in $O( n )$ time ([Morin 2013, exercise
   1.3](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_lists.ods-ex1.3.matched`.
4. Reverse the order of the items of a stack that has only `push()` and
   `pop()`, using a queue and nothing else ([Morin 2013, exercise
   1.4](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_lists.ods-ex1.4.reverse-stack`.
5. Using a USet, realize a BAG, whose `find( x )` gives an item equal to
   $x$ and whose `findAll( x )` gives all of them ([Morin 2013, exercise
   1.5](Z-bibliography.md#ref-morin-cpp)). Test: `ods_lists.ods-ex1.5.bag`.
6. Write simple realizations of the LIST, of the USet and of the SSet on a
   vector, to be used as oracles for the faster ones ([Morin 2013, exercise
   1.6](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_lists.ods-ex1.6.reference-sets`; the LIST on a vector is the
   `ArraySequence` of chapter [2](02-sequence.md#ch-2), checked by
   `ods_lists.ods-2.1.array-stack`.
7. Improve the realizations of the previous exercise, i.e., the `find()`
   of the SSet by a binary search, and the insertions and the removals by
   moving the items as one block ([Morin 2013, exercise
   1.7](Z-bibliography.md#ref-morin-cpp)). Test:
   `ods_lists.ods-ex1.6.reference-sets`, which checks `ArraySSet` against
   `std::set`. Furthermore, the faster LISTs of chapter [2](02-sequence.md#ch-2), whose
   insertions and removals take $O( 1 + \min\{ i , n - i \} )$ amortized
   time, are checked by `ods_lists.ods-2.4.array-deque` and
   `ods_lists.ods-2.5.dual-array-deque`.
8. Check that the realizations of the LIST and of the DEQUE model the
   concepts `IndexedList` and `IndexedDeque` (the `static_assert`s of the
   program `ods_lists`), and run one function template on all the
   realizations of the LIST. Test: `ods_lists.ods-1.2.interfaces`.
9. Check the axioms (1.1), (1.2) and (1.6), and those of the DICTIONARY, on
   all the realizations of the STACK, of the QUEUE, of the SET and of the
   DICTIONARY. Tests: `contract_stack.stack-axioms`,
   `contract_queue.queue-axioms`, `contract_set.set-axioms` and
   `contract_dictionary.dictionary-axioms`.
10. Run the algorithms of `std`, of `std::ranges` and of Boost on the
    containers of the library through their iterators, and check the
    category of each iterator at compile time. Tests: `interop.sequences`,
    `interop.restricted`, `interop.dictionaries`, `interop.trees` and
    `interop.nodes-and-graphs`.
11. Write a structure of items with a key, a value and a cost that tells its
    changes through a subject policy, and check that a batch delivers the
    net effect of (1.15), that an observer that throws leaves the structure
    and the observers already told as they were, and that `UndoLog` brings
    it back to a checkpoint ([Lemma 1.14](01-introduction.md#lem-1-14)).
    Tests: `engineered_modification.mod-coalesce`,
    `engineered_modification.mod-rollback` and
    `engineered_modification.mod-batch`.
12. Run the shortest paths and Prim's algorithm on a priority queue without
    handles, which lowers a key by a new insertion, and check that they
    give the distances, the trees, the order of extraction and the counts
    of a queue with handles, also with negative weights ([Lemma
    1.15](01-introduction.md#lem-1-15)); then check the capability concepts,
    the opt-ins and the queries on the structures of the library, and the
    forwarding of the queries by `ObservedGraph` and by `Shadow`. Tests:
    `engineered_capabilities.cap-lazy-frontier`,
    `engineered_capabilities.cap-queries` and
    `engineered_capabilities.cap-queue-points`.
13. Attach to an `IndexedDaryHeap` an observer that keeps a copy of its
    vector from the exchanges alone, and check it after every operation
    ([Lemma 1.16](01-introduction.md#lem-1-16)); then keep the prefix sums
    of an `ObservableArray` by `FenwickObserver` under a `Shadow` that
    recomputes them from scratch ([Lemma 1.20](01-introduction.md#lem-1-20)),
    and the aggregate of a sliding window of a monoid that does not commute
    by each algorithm of `SlidingAggregate` against a scan of the window
    (Lemmas [1.21](01-introduction.md#lem-1-21) and
    [1.22](01-introduction.md#lem-1-22)). Tests:
    `engineered_emitters.emit-heap-relocated`,
    `engineered_emitters.fenwick-observer` and
    `engineered_emitters.sliding-aggregate`.
