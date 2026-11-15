# 3. Stacks {#ch-3}

[Source: `include/ADS/stack/ArrayStack.h`, `SequenceStack.h`,
`MultiStack.h`, `MinStack.h`, `Expressions.h`, `StackAlgorithms.h`]

We are concerned in this chapter with the *stack*, i.e., a sequence to
which items are added, and from which they are removed, at one end only
(its *top*): the last item added is the first one removed (LIFO, for "last
in, first out"). A computation typically needs a stack when it has to come
back to the points it left in the reverse order in which it left them, as
with the calls of a program, the open parentheses of an expression or the
choices of a backtracking search. The headers of `ADS/stack` realize it on
a vector of fixed capacity ([§3.2](03-stack.md#sec-3-2)), on a sequence
with an efficient head ([§3.3](03-stack.md#sec-3-3)), as several stacks
sharing one vector ([§3.4](03-stack.md#sec-3-4)) and as a stack that also
gives its least item ([§3.5](03-stack.md#sec-3-5)). We then use a stack
to evaluate and translate arithmetic expressions
([§3.6](03-stack.md#sec-3-6)), collect the algorithms that reach a stack
through its operations only, and hence run on any realization
([§3.7](03-stack.md#sec-3-7)), and relate the stack to the balanced
sequences, i.e., the Dyck words and the matched strings of brackets
([§3.8](03-stack.md#sec-3-8)). [Table 3.1](03-stack.md#tab-3-1)
summarizes the costs. In the sets of [§1.9](01-introduction.md#sec-1-9),
the headers of this chapter are all base.

## 3.1 The interface {#sec-3-1}

The STACK of [Montresor 2025, §4.3.1](Z-bibliography.md#ref-montresor2025) has
four operations: `push( v )` adds the item $v$ on top, `pop()` removes the
item on top and returns it, `top()` reads it, and `empty()` is true when there
is no item. Their meaning is fixed by the two axioms

$$
\mathrm{top}( \mathrm{push}( S , v ) ) = v , \qquad
\mathrm{pop}( \mathrm{push}( S , v ) ) = S , \tag{3.1}
$$

where `top()` and `pop()` are undefined on the empty stack; this is the
precondition that the realizations check in their contracts (cf. the
concept `Stack` of `ADS/ADT.h`, which requires the type `Item` and the four
operations, but no other). Throughout the chapter $n$ is the number of
items of a stack, $c$ the capacity of a stack on a vector, and $k$ the
number of stacks that share one vector. All the realizations of
[Table 3.1](03-stack.md#tab-3-1) model the concept, but for `MultiStack`,
whose operations take the index of a stack as their first argument; the
algorithms of [§3.7](03-stack.md#sec-3-7) therefore run on the other
three.

<span id="tab-3-1"></span>**Table 3.1.** The realizations of the STACK in
`ADS/stack` and the cost of their operations; $n$ is the number of items,
$c$ the capacity, $k$ the number of stacks, and $N \ge n$ the largest
number of items held at once since the stack was built (its vectors never
shrink).

| Realization | Representation | Space | `push`, `pop`, `top` | Capacity |
|---|---|---|---|---|
| `ArrayStack` | a vector, the top in the place $n - 1$ | $\Theta( c )$ | $O( 1 )$ | $c$, fixed |
| `SequenceStack` | a sequence, the top its first item | that of the sequence | $O( 1 )$ on a list | none |
| `MultiStack` | $k$ linked stacks in one vector | $\Theta( N + k )$ | $O( 1 )$ amortized | none |
| `MinStack` | two vectors, the items and the minima | $\Theta( N )$ | $O( 1 )$ amortized, `min()` too | none |

## 3.2 A stack on a vector {#sec-3-2}

`ArrayStack` keeps the $n$ items in the places $0 , \ldots , n - 1$ of a
vector $A$ of $c$ places, with the item at the bottom in $A[ 0 ]$ and the top
in $A[ n - 1 ]$. Hence `push( v )` writes the place $n$ and increments $n$,
while `pop()` decrements $n$ and returns the item of the place $n$ ([Algorithm
3.1](03-stack.md#alg-3-1)), each in constant time. Its vector is allocated
once, when the stack is built (its $c$ places are then default-constructed,
which costs $O( c )$ time and requires items with a default constructor), and
the only precondition besides those of (3.1) is that `push()` finds the stack
not full, i.e., $n < c$. [Figure 3.1](03-stack.md#fig-3-1) shows the vector
before and after the two operations. We remark that `pop()` does not clear the
place it frees: the item is moved out of it, and the place is simply no longer
counted.

![Figure 3.1. An `ArrayStack` of capacity $c = 8$ with the items `a`, `b`, `c`, `d`, after `push(e)` and after `pop()`; the place that `push(e)` fills is in orange, the one that `pop()` reads in violet, and the free places in the faintest tint.](../figures/stack-array.svg){#fig-3-1}

<span id="alg-3-1"></span>**Algorithm 3.1.** The operations of
`ArrayStack` ([Montresor 2025, §4.3.2](Z-bibliography.md#ref-montresor2025));
$A[ 0 .. c - 1 ]$ is the vector.

--8<-- "generated/ArrayStack-push.pseudo.md"

--8<-- "generated/ArrayStack-pop.pseudo.md"

<span id="lem-3-1"></span>**Lemma 3.1.** *Let an `ArrayStack` of
capacity $c$ undergo any sequence of operations whose preconditions hold.
Then its items are those of $A[ 0 .. n - 1 ]$, from the bottom to the top,
with $0 \le n \le c$, and its operations satisfy the axioms (3.1); each of
`push()`, `pop()`, `top()` and `empty()` takes $O( 1 )$ time, while the
construction takes $O( c )$ time and $\Theta( c )$ space.*

*Proof.* By induction on the operations, the new stack having $n = 0$. A
`push( v )` with $n < c$ writes $v$ in $A[ n ]$ and increments $n$, which
stays at most $c$, so that $v$ is the new top and the places below it are
untouched; hence `top()`, which reads $A[ n - 1 ]$, gives $v$ right after it.
A `pop()` with $n > 0$ decrements $n$ and returns $A[ n ]$, the old top, and
leaves $A[ 0 .. n - 1 ]$, which right after a `push()` is the stack before it,
i.e., $\mathrm{pop}( \mathrm{push}( S , v ) ) = S$. Each operation makes a
constant number of steps, and the constructor allocates and default-constructs
the $c$ places ([§1.6](01-introduction.md#sec-1-6)). $\square$

The code follows the pseudocode line by line; its contracts check the
preconditions above, the number of items after each operation, and, after a
`push( v )`, that $n \le c$ and that $v$ is on top. For instance, with the
header `ADS/stack/ArrayStack.h`,

```cpp
ADS::ArrayStack< std::string > S( 8 );  // c = 8, n = 0
S.push( "a" );
S.push( "b" );
std::string t = S.pop();  // t is "b", and n = 1
```

where `pop()` moves the item out of the place $n$ (line 3 of `pop()`)
instead of copying it. This matters when the items own memory (say,
strings or vectors), since the place left behind is
not read again before a `push()` overwrites it. Beside the four operations,
`size()` and `capacity()` give $n$ and $c$. A stack is copied item by item
into a vector of the same capacity and moved in $O( 1 )$ time (the stack
moved from is left empty, with no capacity); `swap()` exchanges the items
and the capacities of two stacks in $O( 1 )$ time, and the assignment
copies and swaps. Two stacks are equal
when they hold the same items in the same order, whatever their
capacities. Also, the class gives a read-only, contiguous view of its items
from the bottom to the top, through `begin()` and `end()` (or `cbegin()`
and `cend()`), whose iterators are plain pointers. With the aliases
`value_type`, `size_type`, `difference_type`, `reference`,
`const_reference`, `iterator` and `const_iterator` of a container of the
standard library, the algorithms of the standard library run on this view
unchanged, while the operations of the STACK remain the only way to modify
it.

<span id="rem-3-1"></span>**Remark 3.1.** We keep the capacity of
`ArrayStack` fixed, since this is the realization of [Montresor 2025,
§4.3.2](Z-bibliography.md#ref-montresor2025). In [Morin
2013, §2.1](Z-bibliography.md#ref-morin-cpp), instead, the realization resizes the vector to
$2 n$ places when it is full and when $3 n$ does not exceed its capacity,
which costs $O( 1 )$ amortized time per operation by an argument like that
of [Lemma 3.3](03-stack.md#lem-3-3); `std::vector` with `push_back` and
`pop_back` behaves in the same way, but it never shrinks. Both the stacks of
[§3.4](03-stack.md#sec-3-4) and [§3.5](03-stack.md#sec-3-5) rest on
`std::vector`, and have no capacity for this reason.

In `ADS/stack/ArrayStack.h`, the declarations of `ArrayStack` that a user
calls are

```cpp
// declarations of <ADS/stack/ArrayStack.h>
template< class T >
class ArrayStack
{
 public:
 using Item = T;
 explicit ArrayStack( std::size_t dim );
 bool empty( void ) const;
 const T & top( void ) const;
 T pop( void );
 void push( const T & v );
 std::size_t size( void ) const;
 std::size_t capacity( void ) const;
 bool operator==( const ArrayStack & o ) const;
 };
```

## 3.3 A stack on a sequence {#sec-3-3}

Any SEQUENCE realizes a stack if its first item is taken as the top:

$$
\mathrm{top}() = \mathrm{read}( \mathrm{head}() ) , \quad
\mathrm{pop}() = \mathrm{remove}( \mathrm{head}() ) , \quad
\mathrm{push}( v ) = \mathrm{insert}( \mathrm{head}() , v ) , \tag{3.2}
$$

which is the definition of `SequenceStack` ([Montresor 2025,
§4.3.1](Z-bibliography.md#ref-montresor2025)).

<span id="lem-3-2"></span>**Lemma 3.2.** *On any realization of the
SEQUENCE that satisfies the axioms (1.4), the operations (3.2) satisfy the
axioms (3.1), and `empty()` is that of the sequence. On `List` and on
`SinglyList` each of them takes $O( 1 )$ time, with no capacity.*

*Proof.* By (1.4) with $p = \mathrm{head}()$, i.e., $\mathit{pos}_0$, the
insertion puts $v$ at the index 0 and returns its position, which is then
the head, and reading it gives $v$; hence
$\mathrm{top}( \mathrm{push}( S , v ) ) = v$. Removing the head right after
the insertion gives back the sequence before it, by the second axiom of
(1.4), and the item that `pop()` returns is the one it reads before the
removal; hence $\mathrm{pop}( \mathrm{push}( S , v ) ) = S$. As for the
cost, on the two lists `head()`, `read()`, `insert()` and `remove()` at a
position take $O( 1 )$ time ([Table 2.1](02-sequence.md#tab-2-1)), and
each operation of (3.2) calls a constant number of them. $\square$

The price of the list is one node (an item and at least one pointer) per item.
On a vector, however, an insertion at the head moves all the items; there the
top had better be the last item, as in `ArrayStack`. Since the class is
parametrized on the sequence, the same code gives a stack on `List`, on
`SinglyList` and on any other model of the concept `Sequence` of `ADS/ADT.h`
(cf. [§1.3.2](01-introduction.md#sec-1-3-2)). Its `pop()` reads the head and
then removes it, and thus copies the item instead of moving it, since a
SEQUENCE has no operation that hands out the item it removes. Two stacks are
equal when their sequences are, and the contracts check that `top()` and
`pop()` find the stack not empty and that `push( v )` leaves $v$ on top. When
the sequence is a range of the standard library, `begin()` and `end()` give
its constant iterators, i.e., a view of the items from the top to the bottom.

In `ADS/stack/SequenceStack.h`, the declarations of `SequenceStack` that a
user calls are

```cpp
// declarations of <ADS/stack/SequenceStack.h>
template< Sequence S >
class SequenceStack
{
 public:
 using Item = typename S::Item;
 bool empty( void ) const;
 const Item & top( void ) const;
 Item pop( void );
 void push( const Item & v );
 bool operator==( const SequenceStack & o ) const;
 };
```

## 3.4 Several stacks in one vector {#sec-3-4}

Two stacks can share one vector of $c$ places by growing from its two ends
toward each other ([Knuth 1997, §2.2.2](Z-bibliography.md#ref-knuth1997)).
Neither of them is full while the vector has a free place, whereas two vectors
of $c / 2$ places each would refuse a `push()` as soon as one of the two
stacks reached $c / 2$ items. `MultiStack` extends the idea to $k$ stacks,
numbered from 0, without fixing a capacity in advance. Each item is a cell of
a single vector $C$, which holds the item and the index of the cell below it
in its stack; $\mathit{top}[ i ]$ is the index of the top of the stack $i$ (or
$-1$ when the stack is empty), and the free cells form one more list, whose
head is $\mathit{free}$ ([Figure 3.2](03-stack.md#fig-3-2)). A `push( i , v )`
takes the first free cell, or a new cell at the end of $C$ when there is none,
and links it on top of the stack $i$; a `pop( i )` unlinks the top of the
stack $i$ and puts its cell back at the head of the free list ([Algorithm
3.2](03-stack.md#alg-3-2)). Both cost $O( 1 )$, except when $C$ has to grow.

![Figure 3.2. A `MultiStack` with two stacks, `x1 x2 x3` (the top `x3`, in the cell 6) and `y1 y2` (the top `y2`, in the cell 4), and the free list 2, 5, 7 (in the faintest tint), the top of the stack 0 in violet; the lower row of a cell in use is the index of the cell below it in its stack, that of a free cell the index of the next free one, and $-1$ ends a list.](../figures/stack-multi.svg){#fig-3-2}

<span id="alg-3-2"></span>**Algorithm 3.2.** The operations of
`MultiStack`: the free list of [Knuth 1997,
§2.2.3](Z-bibliography.md#ref-knuth1997) on the idea of [Montresor 2025,
exercise 4.7](Z-bibliography.md#ref-montresor2025).

--8<-- "generated/MultiStack-push.pseudo.md"

--8<-- "generated/MultiStack-pop.pseudo.md"

Here $C$ is a `std::vector`. When it is full, it is replaced by a vector
$\gamma$ times as large, into which the cells are moved, for a constant
$\gamma > 1$ fixed by the standard library. This is $\gamma = 2$ in the
libraries of GCC and LLVM; that of Microsoft adds half the capacity rounded
down, a factor between $4 / 3$ and $3 / 2$, and the lemma below holds there
with $\gamma = 4 / 3$. The indices of the links do not change in the move,
and this is the reason why the cells are linked by indices instead of
pointers. An enlargement costs time linear in
the size of $C$; yet, it happens rarely enough that its cost, spread over
the operations that came before it, is constant per operation ([Tarjan
1985](Z-bibliography.md#ref-tarjan1985)).

<span id="lem-3-3"></span>**Lemma 3.3.** *After any sequence of
operations on a `MultiStack`, each cell of $C$ is either in the list that
starts at $\mathit{top}[ i ]$ for exactly one $i$, which holds the items of
the stack $i$ from its top down, or in the list that starts at
$\mathit{free}$. Moreover, let $C$ grow by the factor $\gamma > 1$.
Starting from an empty `MultiStack`, any sequence of $m$ operations moves
fewer than $\gamma m / ( \gamma - 1 )$ cells in all the enlargements of
$C$ (i.e., $2 m$ for $\gamma = 2$); hence each operation costs $O( 1 )$
amortized time. Finally, $C$ has $N$ cells, where $N$ is the largest number
of items held at once by all the stacks, and if an enlargement multiplies
the capacity of $C$ by at most $\gamma'$, then that capacity is less than
$\gamma' N$; the space is thus $\Theta( N + k )$ words.*

*Proof.* The first claim holds for the empty `MultiStack`, whose lists are all
empty. A `push( i , v )` takes the cell $c$ at the head of the free list (line
3), which it removes from that list, or a new cell (line 2), which is in no
list. Then line 4 makes $c$ the head of the list of the stack $i$, with the
old top below it; the list then holds $v$ and then the old items from the top
down. A `pop( i )` removes the head $c$ of the list of the stack $i$, whose
item is the top, and makes it the head of the free list (line 3); no other
cell changes list in either operation. As for the cost, an enlargement happens
only when all the cells of $C$ are in use, and it moves them all. Only a
`push()` adds a cell to $C$, and therefore the last enlargement of the
sequence moves $s \le m$ cells. The sizes at which $C$ is enlarged grow at
least by the factor $\gamma$ from one enlargement to the next, and hence the
cells moved in all are at most
$s ( 1 + \gamma^{-1} + \gamma^{-2} + \cdots ) < \gamma s / ( \gamma - 1 ) \le \gamma m / ( \gamma - 1 )$.
As for the space, a `push()` appends a cell to $C$ only when the free list is
empty, i.e., when all the cells of $C$ hold items, and the number of cells
thus grows by one exactly when the number of items exceeds its largest value
so far; hence $C$ has $N$ cells. The last enlargement found $s \le N - 1$
cells in use, all of them, and made the capacity at most
$\gamma' s < \gamma' N$ (or 1, for the first cell). Besides $C$, the stacks
take the $k$ tops and the $k$ counters. $\square$

Since $C$ may move during a `push()`, an item of the stacks passed to it by
reference (e.g., in `push( i , top( j ) )`) would be read, after the move,
from a place that is no longer valid; the code therefore builds the new cell,
copying $v$, before $C$ grows. Also, `stacks()` gives $k$, `size( i )` the
number of items of the stack $i$ (kept in a counter per stack, and hence in
$O( 1 )$ time), and `stack( i )` a view of the stack $i$ from the top to the
bottom, whose forward iterator `const_iterator` (an `IndexIterator`,
cf. [§1.8](01-introduction.md#sec-1-8)) follows the links. Its contracts check
that $k > 0$, that the index of a stack is in $[ 0 , k )$ and that `top()` and
`pop()` find the stack not empty. The invariant is that each cell is in one
stack or in the free list, and that the counters are the lengths of the
stacks. It is checked after each `push()` and `pop()`, at the corresponding
level of the contracts, in $O( | C | + k )$ time, where $| C |$ is the largest
number of items reached so far (since $C$ grows only when no cell is free).

In the libraries of GCC, LLVM and Microsoft one has $\gamma' = 2$ in
[Lemma 3.3](03-stack.md#lem-3-3) (the last one enlarges a vector of $s$
places to $\max\{ \lfloor 3 s / 2 \rfloor , s + 1 \}$ places). By contrast,
$k$ vectors, each one large enough for the largest stack, would take $k$
times the largest size. It is fair to remark,
however, that the links cost one index per item. Also, the items of a
stack are scattered over $C$ in the order in which the free cells were
reused. A scan of one stack is thus usually not a scan of contiguous
memory, as it is in `ArrayStack`, and it may well be slower for this
reason.

In `ADS/stack/MultiStack.h`, the declarations of `MultiStack` that a user
calls are

```cpp
// declarations of <ADS/stack/MultiStack.h>
template< class T >
class MultiStack
{
 public:
 using Item = T;
 explicit MultiStack( int k );
 int stacks( void ) const;
 bool empty( int i ) const;
 int size( int i ) const;
 const T & top( int i ) const;
 void push( int i , const T & v );
 T pop( int i );
 std::ranges::subrange< const_iterator > stack( int i ) const;
 };
```

## 3.5 A stack with the minimum {#sec-3-5}

`MinStack` ([Morin 2013, exercise 3.14](Z-bibliography.md#ref-morin-cpp))
is a stack that also gives its least item by `min()`, under an order `C`
(a strict weak order, `std::less<>` by default), with all its operations in
$O( 1 )$ amortized time. In an `ArrayStack` the least item takes a scan of
the $n$ items, and a single variable holding the minimum does not suffice
either, since after a `pop()` of the least item only a scan would find the
next one. `MinStack` keeps instead, beside the vector $A$ of its items, a
second vector $M$ of *minima*. An item is pushed on $M$ too when $M$ is
empty or the item is not larger than the top of $M$, and it leaves $M$ when
it is popped from $A$ while being the top of $M$ ([Algorithm
3.3](03-stack.md#alg-3-3) and [Figure 3.3](03-stack.md#fig-3-3)). Both
vectors are `std::vector`s with the top at the back.

![Figure 3.3. A `MinStack` with the items 5, 3, 7, 3 after `push(9)`, `push(2)` and `pop()`; what each operation adds is in orange, what it reads or removes in violet, the rest in the faint tint. The 9 does not enter $M$, while the 2 does, and leaves it with `pop()`.](../figures/stack-min.svg){#fig-3-3}

<span id="alg-3-3"></span>**Algorithm 3.3.** The operations of `MinStack`
([Morin 2013, exercise 3.14](Z-bibliography.md#ref-morin-cpp)); $A$ is the
stack of the items and $M$ that of the minima.

--8<-- "generated/MinStack-push.pseudo.md"

--8<-- "generated/MinStack-pop.pseudo.md"

Let us call an item of $A$ a *record low* if it is not larger than any
item below it in $A$ (the item at the bottom is one). Both the correctness of
`min()` and the space taken by $M$ rest on the following invariant.

<span id="lem-3-4"></span>**Lemma 3.4.** *After any sequence of
operations, $M$ holds the record lows of $A$, from the bottom up; hence $M$
does not increase from its bottom, its top is the least item of the stack,
and its size $m$ is at most $n$. If the $n$ items are distinct and pushed
in a uniformly random order, then the expected value of $m$ is the
harmonic number $H_n = \sum_{i = 1}^{n} 1 / i \le 1 + \ln n$ (with no
`pop()`). Each operation takes $O( 1 )$ amortized time, and the space is
$\Theta( N )$, with $N$ the largest number of items held at once.*

*Proof.* By induction on the operations, the empty stack being trivial. Note
that the record lows of $A$ do not increase from the bottom up, by their
definition. Also, the highest of them is a least item of $A$, since the
highest least item is a record low, and the highest record low (that item or
one above it) is not larger than it. A `push( x )` makes $x$ a record low if
and only if $x$ is not larger than any item of $A$, i.e., if $A$ is empty or
$x \le \min A = \mathrm{top}( M )$, and it does not change the status of the
items below $x$; this is the test of line 3 of `push()`. A `pop()` removes the
top $x$ of $A$, which is the top of $M$ if $x$ is a record low, and then
$x \le \mathrm{top}( M )$; otherwise some item below $x$ is smaller than $x$,
so that $\mathrm{top}( M ) < x$. This is the test of line 3 of `pop()`, and
the items below $x$ keep their status. As for the expectation, the $i$-th item
pushed is a record low with probability $1 / i$ (i.e., when it is the least of
the first $i$ items, whose order is uniformly random). The sum over $i$ gives
$H_n$, and $H_n \le 1 + \ln n$ follows by comparing the sum with
$\int_1^n d x / x$ ([Lemma 1.7](01-introduction.md#lem-1-7), where the same
count is that of the changes of a running minimum). Finally, each operation
makes at most one comparison and one `push_back` or `pop_back` on each of $A$
and $M$. Neither vector shrinks, and only a `push()` makes them grow. Hence
the argument of the second part of [Lemma 3.3](03-stack.md#lem-3-3) bounds the
items moved by their enlargements in $m'$ operations by
$2 \gamma m' / ( \gamma - 1 )$; their capacities are less than $\gamma' N$,
since $M$ never holds more items than $A$. $\square$

An item equal to the least goes to $M$ too, for the reason given by the
proof: removing it then leaves on top of $M$ the copy pushed by the equal
item below it, i.e., the least item of the others. Pairing each item with
the minimum of the items below it would also give `min()` in $O( 1 )$
time, but it would store $2 n$ items, whereas $M$ holds $n$ items only on
an input that never increases, and $O( \log n )$ items in expectation on a
random one. The code follows [Algorithm 3.3](03-stack.md#alg-3-3), with
the order reached through `C` only: the tests $\mathrm{top}( M ) < x$ of
line 3 of `push()` and `pop()` are calls of the comparison, and two items
are equal when neither is less than the other. With the header
`ADS/stack/MinStack.h`,

```cpp
ADS::MinStack< int > S;  // the order std::less<>
for( int x : { 5 , 3 , 7 , 3 } )
 S.push( x );
S.push( 9 );      // M stays 5 3 3
S.push( 2 );      // M becomes 5 3 3 2
S.pop();          // removes 2 from A and M
int m = S.min();  // m is 3
```

is the run of [Figure 3.3](03-stack.md#fig-3-3). Besides these, the class has `size()`,
`clear()`, `items()` (the vector $A$, from the bottom to the top) and
`minima()` (the size $m$ of $M$). Its contracts check that `top()`,
`pop()` and `min()` find the stack not empty, and the invariant of
[Lemma 3.4](03-stack.md#lem-3-4) in the form that $M$ does not increase
and that its top is equivalent to the least item of $A$ (an $O( n )$
check). Two `MinStack`s back to back give the queue and the deque with the
minimum of [§4.8](04-queue.md#sec-4-8). Also, [Appendix
A](A-writing-structure.md#fig-A-1) follows this class through the files
of the library that a new structure typically touches.

In `ADS/stack/MinStack.h`, the declarations of `MinStack` that a user calls
are

```cpp
// declarations of <ADS/stack/MinStack.h>
template< class T , class C = std::less<> >
class MinStack
{
 public:
 using Item = T;
 explicit MinStack( C less = C() );
 bool empty( void ) const;
 std::size_t size( void ) const;
 const T & top( void ) const;
 const T & min( void ) const;
 void push( const T & x );
 T pop( void );
 void clear( void );
 const std::vector< T > & items( void ) const;
 std::size_t minima( void ) const;
 };
```

## 3.6 Arithmetic expressions {#sec-3-6}

The functions of `ADS/stack/Expressions.h` take expressions whose operands
are nonnegative integers and whose operators are the binary `+`, `-`, `*`
and `/` (the division truncated toward zero), with tokens possibly
separated by spaces. An expression in *postfix* notation (the reverse
Polish notation of [Łukasiewicz
1929](Z-bibliography.md#ref-lukasiewicz1929)) writes each operator after
its two operands, e.g., `5 9 8 + 4 6 * * 7 + *` for
$5 \cdot ( ( 9 + 8 ) \cdot ( 4 \cdot 6 ) + 7 )$. It needs neither
parentheses nor precedences, since the order of the tokens is the order of
the evaluation. Formally, an operand is a postfix expression, and so is
$E_1 \, E_2 \, \mathrm{op}$ when $E_1$ and $E_2$ are. Then `evalPostfix()`
scans the tokens and pushes each operand; for each operator, it pops the
right operand $t$, then the left one $s$, and pushes
$s \mathbin{\mathrm{op}} t$ ([Algorithm 3.4](03-stack.md#alg-3-4)).

<span id="alg-3-4"></span>**Algorithm 3.4.** The evaluation of a postfix
expression.

--8<-- "generated/Expressions-evalPostfix.pseudo.md"

<span id="lem-3-5"></span>**Lemma 3.5.** *The scan of a postfix
expression $E$ that starts with a stack $S$ ends with $S$ and the value of
$E$ on top of it; hence `evalPostfix()` computes the value of an expression
of $n$ tokens in $O( n )$ time and $O( n )$ space.*

*Proof.* By induction on the structure of $E$: an operand is pushed; for
$E = E_1 \, E_2 \, \mathrm{op}$, the scan of $E_1$ leaves its value $s$ on
$S$, that of $E_2$ leaves its value $t$ on $S$ and $s$, and the operator
replaces the two with $s \mathbin{\mathrm{op}} t$. Each token costs one or
three operations of the stack. $\square$

A sequence of tokens that is not a postfix expression shows up as an
operator that finds fewer than two items, or as a number of items other
than one left at the end (none for the empty expression). These are the
preconditions that `evalPostfix()` checks. It also refuses a parenthesis
(which a postfix expression never has), a character other than the
digits, the four operators, the parentheses and the space (when the
expression is split into tokens), a division by 0, and an operation
whose result does not fit a `long`
(an operand that does not fit one is rejected by `std::stol`, which throws
`std::out_of_range`). Its stack is an `ArrayStack` of capacity $n + 1$,
which cannot overflow since each token pushes at most one item.

Translating an infix expression into a postfix one is the complementary
use of a stack, which now holds the operators. If the infix expression is
fully parenthesized, e.g., `( 5 * ( ( ( 9 + 8 ) * ( 4 * 6 ) ) + 7 ) )`,
one can write the operands as they come, skip the open parentheses, push
the operators, and pop one operator at each closed parenthesis
(`toPostfixParenthesized()`, after [Sedgewick 1998,
§4.3](Z-bibliography.md#ref-sedgewick1998)). Indeed, the pair of
parentheses that a closed parenthesis closes is the last one opened, and
its operator is the last one pushed and not yet popped. With the usual
precedences ($\cdot$ and $/$ before $+$ and $-$) and the associativity to
the left, the shunting-yard algorithm of [Dijkstra
1961](Z-bibliography.md#ref-dijkstra1961) keeps on the stack the operators
whose right operand is not complete yet (`toPostfix()`,
[Algorithm 3.5](03-stack.md#alg-3-5)). An operator $x$ first pops to the
output, down to the nearest open parenthesis, each operator whose
precedence $\pi$ is not lower than that of $x$ (the right operand of each
of them ends where $x$ begins). A closed parenthesis pops the operators
down to its open one. [Table 3.2](03-stack.md#tab-3-2) follows the
algorithm on an expression with both precedences and a pair of
parentheses; its open parentheses are skipped by
`toPostfixParenthesized()`, which has no line for them.

<span id="alg-3-5"></span>**Algorithm 3.5.** The translation of a fully
parenthesized expression ([Sedgewick 1998,
§4.3](Z-bibliography.md#ref-sedgewick1998)) and the shunting-yard algorithm
([Dijkstra 1961](Z-bibliography.md#ref-dijkstra1961)); $\pi( x )$ is the
precedence of the operator $x$, 2 for $\cdot$ and $/$ and 1 for $+$ and
$-$.

--8<-- "generated/Expressions-toPostfixParenthesized.pseudo.md"

--8<-- "generated/Expressions-toPostfix.pseudo.md"

<span id="tab-3-2"></span>**Table 3.2.** The shunting-yard algorithm on
`35 + 17 * ( 40 - 9 ) - 7`: the stack after each token (its top on the
right) and the output so far.

| Token | Stack | Output |
|---|---|---|
| `35` | | `35` |
| `+` | `+` | `35` |
| `17` | `+` | `35 17` |
| `*` | `+ *` | `35 17` |
| `(` | `+ * (` | `35 17` |
| `40` | `+ * (` | `35 17 40` |
| `-` | `+ * ( -` | `35 17 40` |
| `9` | `+ * ( -` | `35 17 40 9` |
| `)` | `+ *` | `35 17 40 9 -` |
| `-` | `-` | `35 17 40 9 - * +` |
| `7` | `-` | `35 17 40 9 - * + 7` |
| end | | `35 17 40 9 - * + 7 -` |

In line 10 of
`toPostfix()` in [Algorithm 3.5](03-stack.md#alg-3-5), the test
$\pi( x ) \le \pi( \mathrm{top}( S ) )$, with $\le$ in place of $<$, makes
the operators of equal precedence associate to the left (e.g.,
`8 - 3 - 2` becomes `8 3 - 2 -`, whose value is 3). To state what the two functions compute, we let the postfix form $P$ of an expression be
defined by $P( a ) = a$ for an operand $a$, $P( ( E ) ) = P( E )$ and
$P( E_1 \mathbin{\mathrm{op}} E_2 ) = P( E_1 ) \, P( E_2 ) \,
\mathrm{op}$. The infix expressions with the usual precedences are those of the grammar

$$
E \to T \mid E + T \mid E - T , \qquad
T \to F \mid T \cdot F \mid T / F , \qquad
F \to a \mid ( E ) , \tag{3.3}
$$

whose left recursion is the associativity to the left, and in which we
give the level $\ell = 1$ to the phrases derived from $E$, $\ell = 2$ to
those derived from $T$ and $\ell = 3$ to those derived from $F$; the fully
parenthesized expressions are those of the grammar $F \to a \mid ( F
\mathbin{\mathrm{op}} F )$.

<span id="lem-3-6"></span>**Lemma 3.6.** *On a fully parenthesized
expression $e$, `toPostfixParenthesized( e )` returns $P( e )$; on an
expression $e$ of the grammar (3.3), `toPostfix( e )` returns $P( e )$
with the operators of equal precedence associated to the left. Both take
$O( n )$ time and space on $n$ tokens
([Samelson and Bauer 1960](Z-bibliography.md#ref-samelson1960); [Dijkstra
1961](Z-bibliography.md#ref-dijkstra1961)). Moreover, in `toPostfix()` the
operators on the stack above its topmost open parenthesis (or above its
bottom, when there is none) have strictly increasing precedences from the
bottom up, on any sequence of tokens.*

*Proof.* For `toPostfixParenthesized()` we show, by induction on the
structure of a fully parenthesized phrase $X$, that its scan appends
$P( X )$ to the output and ends with the stack it started with. This is
line 4 for an operand; for $X = ( A \mathbin{\mathrm{op}} B )$, the open
parenthesis is skipped, the scan of $A$ appends $P( A )$, $\mathrm{op}$ is
pushed (line 5), the scan of $B$ appends $P( B )$ and leaves
$\mathrm{op}$ on top, and the closed parenthesis pops it to the output
(line 6); hence $P( A ) \, P( B ) \, \mathrm{op} = P( X )$ is appended.

For `toPostfix()`, let a stack $S$ *admit* the level $\ell$ if its top is
not an operator of precedence at least $\ell$ (an empty stack and an open
parenthesis on top admit every level). We show, by induction on the
derivation of a phrase $X$ of level $\ell$, that the scan of $X$ from a
stack $S$ that admits $\ell$ appends to the output a string $Q$ and pushes
on $S$, which it leaves untouched below, a sequence $R$ of operators of
precedence at least $\ell$, such that $Q$ followed by the items of $R$
from the top down is $P( X )$. For an operand, $Q = a$ and $R$ is empty.
For $X = ( E )$, the open parenthesis is pushed (line 5), the stack then
admits the level 1 of $E$, and by induction the scan of $E$ appends $Q_E$
and pushes $R_E$; the closed parenthesis pops $R_E$ to the output down to
the open parenthesis, which $R_E$ does not contain, and pops it (lines 7
and 8). Thus $Q$ is $Q_E$ followed by $R_E$ from the top down, i.e.,
$P( E ) = P( X )$, and $R$ is empty. For $X = E_1 + T$ (and in the same
way for $-$), $S$ admits the level 1 of $E_1$, whose scan appends $Q_1$
and pushes $R_1$; in line 10 the operator then pops every item of $R_1$,
since their precedences are at least 1, and stops at the top of $S$, which
is not an operator since $S$ admits the level 1. After the push of line
12, the stack has $+$ on top, of precedence $1 < 2$, and admits the level
2 of $T$, whose scan appends $Q_2$ and pushes $R_2$. Hence $Q$ is $Q_1$,
$R_1$ from the top down and $Q_2$, while $R$ is $+$ under $R_2$, and
$Q$ followed by $R$ from the top down is $P( E_1 ) \, P( T ) \, + =
P( X )$, with precedences at least 1 in $R$. For $X = T_1 \cdot F$ (and
for $/$) the argument is the same with the level 2: the operator pops
$R_1$, whose precedences are at least 2, stops at the top of $S$, which is not an operator of precedence 2. After its push the stack admits the
level 3 of $F$, whose scan pushes no operator; hence $R$ is the operator
only, of precedence 2. Finally, a phrase $E$ derived as $T$, or $T$
derived as $F$, is scanned from a stack that admits the lower level and
hence the higher one, and pushes operators of precedence at least the
higher level, hence at least the lower one. The whole $e$ is scanned from
the empty stack, and line 13 appends $R$ from the top down, which gives
$P( e )$; the associativity to the left is that of the derivation of
$E_1 + T$ in (3.3). As for the cost, each token is pushed at most once and
popped at most once, each iteration of the loops of lines 7, 10 and 13
pops an item, and the stack and the output hold at most $n$ tokens. The last claim holds on the empty stack. Line 5 pushes an open parenthesis,
above which no operator lies, and line 12 pushes an operator $x$ when the
loop of line 10 has stopped, i.e., on the empty stack, on an open
parenthesis or on an operator of precedence lower than $\pi( x )$, while a
pop keeps the claim on what remains. $\square$

The induction shows why line 10 tests $\le$: the operator $+$ that
follows $E_1$ must pop the $+$ and the $-$ of $E_1$ still on the stack, so
that they precede it in the output. Both translations require
balanced parentheses and no tokens but those above, and
`toPostfixParenthesized()` also requires an operator in each pair of
parentheses. Their contracts refuse a character outside the tokens and a
closed parenthesis that finds the stack empty, and an open parenthesis
(in `toPostfix()`) or an operator (in `toPostfixParenthesized()`) left on
the stack at the end. An expression that is not well formed in other ways
is translated without complaint (e.g., `1 2 +` stays as it is), and the
output separates the tokens by one space.

In `ADS/stack/Expressions.h`, the functions are declared as

```cpp
// declarations of <ADS/stack/Expressions.h>
long evalPostfix( std::string_view e );
std::string toPostfixParenthesized( std::string_view e );
std::string toPostfix( std::string_view e );
```

## 3.7 Algorithms on a stack {#sec-3-7}

The functions of `ADS/stack/StackAlgorithms.h` act on a stack through
`push()`, `pop()`, `top()` and `empty()` only, and are therefore written
once for all the models of the concept `Stack`. A first group rests on
the reversal of the order of the items that go through a stack. A string
is a palindrome if and only if its characters, pushed in order and then
popped, come out as the string read from its start; hence `isPalindrome()`,
on a `SequenceStack` of a `List`, takes $O( n )$ time and space. Removing
the items larger than a key while keeping the order of the others
(`removeGreater()`) takes an auxiliary stack, which reverses the items kept
when they are moved there and once more when they come back, in $O( n )$
time and space ([Algorithm 3.6](03-stack.md#alg-3-6)).

<span id="alg-3-6"></span>**Algorithm 3.6.** Two uses of the reversal
([Montresor 2025, §4.3.1](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/StackAlgorithms-isPalindrome.pseudo.md"

--8<-- "generated/StackAlgorithms-removeGreater.pseudo.md"

A queue, by contrast, keeps the order of the items that go through it, and
a stack with no operation but `push()` and `pop()` is thus reversed by a
queue alone ([Morin 2013, exercise 1.4](Z-bibliography.md#ref-morin-cpp)).
Here `reverseWithQueue()` pops all the items into the queue, which then holds
them from the old top down, and pushes them back in the order in which
they leave it, so that the old top, pushed first, ends at the bottom
([Algorithm 3.7](03-stack.md#alg-3-7)). Each item is popped, enqueued,
dequeued and pushed once, in $O( n )$ time, and the queue takes $O( n )$
space; it is a `LinkedQueue` ([§4.3](04-queue.md#sec-4-3)) unless another
model of the concept `Queue` is given as the second parameter of the
template. Its code is the pseudocode, line by line, and an audit checks
that the stack holds its old items in the reverse order.

<span id="alg-3-7"></span>**Algorithm 3.7.** The order of a stack reversed
by a queue ([Morin 2013, exercise 1.4](Z-bibliography.md#ref-morin-cpp)).

--8<-- "generated/StackAlgorithms-reverseWithQueue.pseudo.md"

Recursion supplies a second stack (that of the calls), and two of the
functions use no other structure. To visit the items from the bottom to the
top and leave the stack as it was, `forEachBottomUp()` pops the top, visits
the rest by a recursive call, visits the top and pushes it back
([Algorithm 3.8](03-stack.md#alg-3-8)). The stack of the calls holds the
items popped, and the visit takes $2 n$ operations of the stack and a
recursion of depth $n$. In a similar way, `sortStack()` sorts a stack by
passes, after counting its items by the same recursion. A pass pops two
items, pushes back the larger, makes a recursive pass on the rest, and
pushes the smaller on top ([Algorithm 3.9](03-stack.md#alg-3-9)).

<span id="alg-3-8"></span>**Algorithm 3.8.** The visit of a stack from the
bottom up, with the stack of the calls.

--8<-- "generated/StackAlgorithms-forEachBottomUp.pseudo.md"

<span id="lem-3-7"></span>**Lemma 3.7.** *Let $x_1 , \ldots , x_n$ be
the items of a stack from the top down. Then `isPalindrome( s )` is true if
and only if $s$ is a palindrome; `removeGreater( s , k )` leaves the items
$x_i \le k$ in their order; `reverseWithQueue( s )` leaves $x_n , \ldots ,
x_1$ from the top down; `forEachBottomUp( s , f )` calls $f$ on $x_n ,
\ldots , x_1$, in this order, and leaves the stack as it was. Each one
makes $O( n )$ operations on its structures (with $n$ the length of $s$
for `isPalindrome()`), and takes $O( n )$ space besides the stack.*

*Proof.* A sequence pushed on an empty stack and then popped comes out
reversed, by $n$ applications of (3.1); the same holds for a queue, by the
axiom FIFO of [§4.1](04-queue.md#sec-4-1), with the order kept instead of
reversed. In `isPalindrome()`, the pops of line 4 thus give $s$ from its
last character to its first, and they all match the characters of $s$ from
its first if and only if $s$ reads the same in the two directions. In
`removeGreater()`, line 4 pushes on $A$ the items $x_i \le k$ in the order
of $i$, and line 5 pops them from $A$ in the reverse order and pushes them
back on $s$, where the last one pushed, the $x_i \le k$ of least $i$, is
the top. In `reverseWithQueue()`, line 3 enqueues $x_1 , \ldots , x_n$,
and line 4 pushes them in the same order; $x_n$, pushed last, is
the top. For `forEachBottomUp()`, by induction on $n$: the empty stack is
left alone by the test of line 2, and otherwise $x_1$ is popped, the
recursive call visits $x_n , \ldots , x_2$ and leaves $x_2 , \ldots , x_n$
as they were, and line 4 visits $x_1$ and pushes it back on top. Each item is pushed and
popped (or enqueued and dequeued) a constant number of times in each
function, and the auxiliary structure, or the stack of the calls in
`forEachBottomUp()`, holds at most $n$ items. $\square$

<span id="alg-3-9"></span>**Algorithm 3.9.** The sort of a stack by
passes, i.e., the bubble sort on a stack.

--8<-- "generated/StackAlgorithms-pass.pseudo.md"

<span id="lem-3-8"></span>**Lemma 3.8.** *A pass carries a largest item
of the stack to its bottom; hence after $i$ passes the $i$ largest items
are at the bottom, in order, and $n - 1$ passes sort the stack (the least
item on top); a pass moves no item up by more than one place, and hence
$n - 1$ passes are needed when $n \ge 2$ and the only least item is at the
bottom. The $n - 1$ passes of `sortStack()` take $\Theta( n^2 )$
operations of the stack on every input, with a stack of the calls of depth
$n$.*

*Proof.* Let $x_1 , \ldots , x_k$ be the items from the top, and $c_j =
\max\{ x_1 , \ldots , x_j \}$. A pass leaves $\min( c_j , x_{j+1} )$ at
the place $j$ from the top, for $j < k$, and $c_k$ at the bottom. Indeed,
by induction on the levels of the recursion, the level $j$ finds $c_j$ on
top, pushes back $\max( c_j , x_{j+1} ) = c_{j+1}$ for the level below,
and pushes $\min( c_j , x_{j+1} )$ when that level returns. Now let $x_{p+1}
\le \cdots \le x_k$ be the $k - p$ largest items, in order. Then $c_j =
x_j$ for $j > p$, so that the pass leaves them in their places and puts
$c_p$, a largest item among the others, at the place $p$. The claim on $i$
passes follows by induction on $i$, and after $n - 1$ passes the least
item is on top of the others, which are sorted. By the same formula, the
item at the place $j < k$ after a pass is either $x_{j+1}$ or $c_j$, an
item of the places $1 , \ldots , j$; the item at the bottom is $c_k$, and
no item thus rises by more than one place. Each level of a pass makes at
most two pops and two pushes, on a recursion of at most $k = n$ levels. The count of the items takes $2 n$ more, and each of the $n - 1$ passes thus costs
$\Theta( n )$ operations, whatever the order of the items. $\square$

As in the bubble sort, a small item rises slowly, and the $n - 1$ passes
that the code makes whatever the input are as many as the worst input
needs; each pass also goes down to the bottom of the stack, even when the
stack is already sorted. Finally, the contracts of
`sortStack()`, `removeGreater()`, `reverseWithQueue()` and
`forEachBottomUp()` are audits, which read the stack by popping it and
pushing it back. They check, respectively, a sorted stack with the same
number of items, no item larger than the key, the old items in the reverse
order, and the stack as it was.

In `ADS/stack/StackAlgorithms.h`, the functions are declared as

```cpp
// declarations of <ADS/stack/StackAlgorithms.h>
bool isPalindrome( const std::string & s );
template< Stack S >
void sortStack( S & s );
template< Stack S >
void removeGreater( S & s , const typename S::Item & k );
template< Stack S , class F >
void forEachBottomUp( S & s , F && f );
bool isDyckWord( std::span< const int > w );
bool isMatched( std::string_view s );
template< Stack S , Queue Q = LinkedQueue< typename S::Item > >
void reverseWithQueue( S & s );
```

## 3.8 Dyck words and matched brackets {#sec-3-8}

A *Dyck word* is a sequence $w_1 , \ldots , w_n$ of $+1$ and $-1$ whose
prefix sums $s_j = w_1 + \cdots + w_j$ are never negative ([Morin 2013,
exercise 1.2](Z-bibliography.md#ref-morin-cpp)). Read with $+1$ as a
`push()` and $-1$ as a `pop()`, a word is a sequence of operations on a
stack, after $j$ of which the stack holds $s_j$ items as long as no `pop()`
finds it empty. Hence $w$ is a Dyck word if and only if its operations, run
on an empty stack, never pop the empty stack, and the stack is empty at
the end if and only if, in addition, $s_n = 0$ ([Lemma
1.2](01-introduction.md#lem-1-2), where the argument is given in full,
with the pseudocode in [Algorithm 1.1](01-introduction.md#alg-1-1)). A
stack whose items are all alike is described by its size, and
`isDyckWord()` keeps the stack as a counter $h$, in $O( n )$ time and
$O( 1 )$ space (line 5 of its pseudocode, in
[Algorithm 1.1](01-introduction.md#alg-1-1), is the `pop()` of the empty
stack); its precondition is that each item of $w$ is $+1$ or $-1$. We
remark that it returns true also when $s_n > 0$, since it decides the
condition on the prefix sums only.

Brackets, instead, need the items themselves. Let the *matched* strings
of the brackets `()`, `[]` and `{}` be defined by a grammar: the empty string is
matched, and if $u$ and $v$ are matched, then so are `(`$u$`)`,
`[`$u$`]`, `{`$u$`}` and $u v$ ([Morin 2013, exercise
1.3](Z-bibliography.md#ref-morin-cpp)). For instance, `{{()[]}}` is
matched, while `{{()]}` is not, since its second `{` faces a `]`. In its
scan, `isMatched()` pushes each open bracket, and at each closed bracket pops the
top, which must be the open bracket of the same kind; the string is
matched if no `pop()` fails or mismatches and the stack is empty at the end
([Algorithm 1.1](01-introduction.md#alg-1-1) and [Table
3.3](03-stack.md#tab-3-3)). Its stack is an `ArrayStack` of capacity $n$,
which cannot overflow since there are at most $n$ open brackets; a
character other than the six brackets violates its precondition.

<span id="tab-3-3"></span>**Table 3.3.** `isMatched()` on `{{()[]}}` and
on `{{()]}` (the columns 2 and 3, and 4 and 5): the stack after each
bracket, its top on the right; on the second string, the `]` pops a `{`
and the answer is false.

| Step | Bracket | Stack | Bracket | Stack |
|---|---|---|---|---|
| 1 | `{` | `{` | `{` | `{` |
| 2 | `{` | `{{` | `{` | `{{` |
| 3 | `(` | `{{(` | `(` | `{{(` |
| 4 | `)` | `{{` | `)` | `{{` |
| 5 | `[` | `{{[` | `]` | `{`, false |
| 6 | `]` | `{{` | | |
| 7 | `}` | `{` | | |
| 8 | `}` | empty, true | | |

<span id="lem-3-9"></span>**Lemma 3.9.** *`isMatched( s )` is true if and
only if $s$ is matched, and it takes $O( n )$ time and $O( n )$ space.
Moreover, if $s$ has two adjacent brackets `()`, `[]` or `{}`, then $s$ is
matched if and only if the string left by their removal is; hence the
removal of such pairs, in any order until none is left, empties $s$ if and
only if $s$ is matched, in $O( n^2 )$ time.*

*Proof.* We first show, by induction on the grammar, that the scan of a
matched $u$ that starts with a stack $S$ never pops an item of $S$ and ends
with $S$ again. This is trivial for the empty string and follows at once
for $u v$; for `(`$u$`)`, the `(` is pushed, the scan of $u$ leaves it on
top, and the `)` pops it and finds its kind (and the same holds for the
other two kinds). Hence the scan of a matched $s$ from the empty stack
ends with the empty stack and no failure. Conversely, let the scan of $s$
succeed and end with the empty stack, and let us argue by induction on
$n$, the empty string being matched. The first bracket of $s$ is an open
one, say `(`, since a closed one would pop the empty stack; let the `)` at
the place $j$ be the bracket that pops it (a bracket pops it, since the
stack is empty at the end, and it is a `)`, since the scan succeeds). Between the two, the scan
pushes and pops items above the `(` only, and hence it succeeds on the
substring $u$ strictly between them, started from the empty stack, and
ends with it empty; by induction, $u$ is matched. After the place $j$ the
stack is empty again, so that the rest $v$ of $s$ is also scanned with
success from the empty stack, and it is matched by induction; thus $s =$
`(`$u$`)`$v$ is matched. Each bracket costs $O( 1 )$ time, and the stack
holds at most $n$ items. As for the pairs, the scan pushes the open
bracket of an adjacent pair and pops it at once, finding its kind; it thus reaches the rest of $s$ with the stack it had before the pair; hence it
succeeds on $s$ if and only if it succeeds on $s$ without the pair, and the
first part gives the claim. If $s$ is matched, so is every string reached by
removals, and a nonempty matched string has an adjacent pair (in
`(`$u$`)`$v$, either $u$ is empty or $u$ has one, by induction), so that the
removals end with the empty string; if $s$ is not matched, no string
reached is, and the empty string is matched. Each removal is found by a
scan of $O( n )$ characters, and there are at most $n / 2$ of them.
$\square$

With $+1$ for an open bracket and $-1$ for a closed one, a matched string
is a Dyck word with $s_n = 0$; the converse holds when there is a single
kind of brackets, and the items of the stack are needed only to tell the
kinds apart. In the library, the test compares `isMatched()` with the
removal of the adjacent pairs `()`, `[]` and `{}` until none is left,
which is correct by [Lemma 3.9](03-stack.md#lem-3-9). It takes, however,
$O( n^2 )$ time.

## 3.9 Discussion and references {#sec-3-9}

The stack on a vector and on a list, and the two stacks in one vector, are
in most textbooks on data structures (see, e.g., [Knuth 1997, §2.2.1 and
§2.2.2](Z-bibliography.md#ref-knuth1997)). There one also finds $k$ stacks
of variable size in one area, kept contiguous by repacking the area when a
stack overflows; this keeps each stack in contiguous memory, at the price
of a cost per overflow that is not constant. As for `MultiStack`, its
free list is that of the dynamic storage allocation of [Knuth 1997,
§2.2.3](Z-bibliography.md#ref-knuth1997). Also, the amortized analysis of
the enlargements is that of [Tarjan 1985](Z-bibliography.md#ref-tarjan1985),
and it returns with the sequences that grow and shrink ([Morin 2013,
§2.1](Z-bibliography.md#ref-morin-cpp)). Our stack with the minimum is
[Morin 2013, exercise 3.14](Z-bibliography.md#ref-morin-cpp), the building
block of the queues with the minimum of his exercises 3.15 and 3.16
([§4.8](04-queue.md#sec-4-8)); keeping the record lows only, instead of a
minimum per item, is a common refinement of it. The evaluation of the
postfix expressions with a stack goes back to the compilers of the late
fifties ([Samelson and Bauer 1960](Z-bibliography.md#ref-samelson1960)),
and the shunting-yard algorithm to the first Algol 60 compiler ([Dijkstra
1961](Z-bibliography.md#ref-dijkstra1961)); [Sedgewick 1998,
§4.3](Z-bibliography.md#ref-sedgewick1998) has both, together with the
translation of the fully parenthesized expressions. The Dyck words, the
matched brackets and the reversal of a stack by a queue are the exercises
of [Morin 2013, chapter 1](Z-bibliography.md#ref-morin-cpp) on the stack;
the balanced parentheses return in [§22.8](22-succinct.md#sec-22-8), as
the succinct representation of an ordered tree.

## 3.10 Exercises {#sec-3-10}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize two stacks in one vector of $c$ places, so that a `push()` on
   either of them succeeds while the vector has a free place ([Montresor
   2025, exercise 4.7](Z-bibliography.md#ref-montresor2025)). Test:
   `ch04.ex4.7`.
2. Realize the STACK on a singly linked list, with each operation in
   $O( 1 )$ time, and explain why the top must be the head of the list
   instead of its tail. Test: `ch04.sec4.3.1.stack-on-list`.
3. Check the axioms (3.1) on `ArrayStack` and on `SequenceStack` (on a list
   and on a vector), and the contracts of the realizations of the STACK.
   Tests: `contract_stack.stack-axioms`, `contract_stack.array-stack`,
   `contract_stack.sequence-stack`, `contract_stack.multistack`.
4. Realize $k$ stacks in one vector with no capacity fixed in advance, in
   space $O( n + k )$ and with each operation in $O( 1 )$ amortized time.
   Test: `beyond.multistack`.
5. Design a stack that supports `push()`, `pop()`, `top()`, `size()` and
   `min()`, the least item, each in $O( 1 )$ amortized time ([Morin 2013,
   exercise 3.14](Z-bibliography.md#ref-morin-cpp)), and bound the space
   that it adds to that of the items. Test:
   `ods_lists.ods-ex3.14.min-stack`.
6. Evaluate an expression in postfix notation, rejecting the sequences of
   tokens that are not postfix expressions and the results that overflow,
   and translate an infix expression with the usual precedences into
   postfix notation in $O( n )$ time. Tests: `beyond.postfix`,
   `contract_stack.expressions`.
7. Give the output of the sequence `LA*STI*N*FIR*ST**OU*T*******` on a
   stack, a letter meaning `push()` and an asterisk `pop()`, and the least
   capacity of an `ArrayStack` that runs it. Test:
   `beyond.lifo-fifo-traces`.
8. Using only the operations of the STACK and recursion, decide whether a
   string is a palindrome, remove the items larger than a key keeping the
   order of the others, and sort a stack. Tests:
   `beyond.stack-algorithms`, `contract_stack.stack-algorithms`.
9. Visit the items of a stack from the bottom to the top and leave it as
   it was, using no structure other than the stack of the calls. Test:
   `beyond.stack-bottom-up`.
10. Reverse the order of the items of a stack that has only `push()` and
    `pop()`, using a queue and nothing else ([Morin 2013, exercise
    1.4](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex1.4.reverse-stack`.
11. Show that a sequence of $+1$ and $-1$ is a Dyck word if and only if
    the pushes for $+1$ and the pops for $-1$, run on an empty stack, never
    pop the empty stack ([Morin 2013, exercise
    1.2](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex1.2.dyck`.
12. Decide with a stack whether a string of $n$ brackets `()`, `[]` and
    `{}` is matched, in $O( n )$ time ([Morin 2013, exercise
    1.3](Z-bibliography.md#ref-morin-cpp)). Test:
    `ods_lists.ods-ex1.3.matched`.
13. Solve the problems 1 and 2 of [Morin 2013, exercise
    1.1](Z-bibliography.md#ref-morin-cpp) with a stack: write the lines of
    a text in the reverse order, and write them in blocks of 50, each
    block reversed, keeping at most 50 lines in memory. Test:
    `ods_lists.ods-ex1.1.lines`.
