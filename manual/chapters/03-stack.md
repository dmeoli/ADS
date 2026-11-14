# 3. Stacks {#ch-3}

[Source: `include/ADS/stack/ArrayStack.h`, `SequenceStack.h`,
`MultiStack.h`, `Expressions.h`, `StackAlgorithms.h`]

We are concerned in this chapter with the *stack*, i.e., a sequence to
which items are added, and from which they are removed, at one end only
(its *top*): the last item added is the first one removed (LIFO, for "last
in, first out"). A computation typically needs a stack when it has to come
back to the points it left in the reverse order in which it left them, as
with the calls of a program, the open parentheses of an expression or the
choices of a backtracking search. The headers of `ADS/stack` realize it on
a vector of fixed capacity ([§3.2](03-stack.md#sec-3-2)), on a sequence
with an efficient head ([§3.3](03-stack.md#sec-3-3)) and as several stacks
sharing one vector ([§3.4](03-stack.md#sec-3-4)). Then
[§3.5](03-stack.md#sec-3-5) uses a stack to evaluate and translate
arithmetic expressions, and [§3.6](03-stack.md#sec-3-6) collects the
algorithms that reach a stack through its operations only, and hence run on
any realization; [Table 3.1](03-stack.md#tab-3-1) summarizes the costs.

## 3.1 The interface {#sec-3-1}

The STACK of [Montresor 2025, §4.3.1](Z-bibliography.md#ref-montresor2025)
has four operations: `push(v)` adds the item $v$ on top, `pop()` removes
the item on top and returns it, `top()` reads it, and `empty()` is true
when there is no item. Their meaning is fixed by the two axioms

$$
\mathrm{top}( \mathrm{push}( S , v ) ) = v , \qquad
\mathrm{pop}( \mathrm{push}( S , v ) ) = S , \tag{3.1}
$$

where `top()` and `pop()` are undefined on the empty stack; this is the
precondition that the realizations check in their contracts (cf. the
concept `Stack` of `ADS/ADT.h`, which requires the four operations and
nothing else). Throughout the chapter $n$ is the number of items of a
stack, $c$ the capacity of a stack on a vector, and $k$ the number of
stacks that share one vector.

<span id="tab-3-1"></span>**Table 3.1.** The realizations of the STACK in
`ADS/stack` and the cost of their operations; $n$ is the number of items,
$c$ the capacity.

| Realization | Representation | Space | `push`, `pop`, `top` |
|---|---|---|---|
| `ArrayStack` | a vector, the top in the place $n - 1$ | $\Theta( c )$ | $O( 1 )$ |
| `SequenceStack` | a sequence, the top its first item | that of the sequence | $O( 1 )$ on a list |
| `MultiStack` | $k$ linked stacks in one vector | $\Theta( n + k )$ | $O( 1 )$ amortized |

## 3.2 A stack on a vector {#sec-3-2}

`ArrayStack` keeps the $n$ items in the places $0 , \ldots , n - 1$ of a
vector $A$ of $c$ places, with the item at the bottom in $A[ 0 ]$ and the
top in $A[ n - 1 ]$. Hence `push(v)` writes the place $n$ and increments
$n$, while `pop()` decrements $n$ and returns the item of the place $n$
([Algorithm 3.1](03-stack.md#alg-3-1)), each in constant time. The vector
is allocated once, when the stack is built, and the only precondition
besides those of (3.1) is that `push()` finds the stack not full, i.e.,
$n < c$. [Figure 3.1](03-stack.md#fig-3-1) shows the vector before and
after the two operations. We remark that `pop()` does not clear the place
it frees: the item is moved out of it, and the place is simply no longer
counted.

![Figure 3.1. An `ArrayStack` of capacity $c = 8$ with the items `a`, `b`, `c`, `d`, after `push(e)` and after `pop()`; the place that `push(e)` fills is in orange, the one that `pop()` reads in violet, and the free places in the faintest tint.](../figures/stack-array.svg){#fig-3-1}

<span id="alg-3-1"></span>**Algorithm 3.1.** The operations of
`ArrayStack` ([Montresor 2025, §4.3.2](Z-bibliography.md#ref-montresor2025));
$A[ 0 .. c - 1 ]$ is the vector.

--8<-- "generated/ArrayStack-push.pseudo.md"

--8<-- "generated/ArrayStack-pop.pseudo.md"

The code follows the pseudocode line by line, but for the contracts (which
check the preconditions above, and that `push()` leaves $v$ on top):

```cpp
--8<-- "generated/ArrayStack-push.cpp"
```

```cpp
--8<-- "generated/ArrayStack-pop.cpp"
```

Moving the item out in `pop()`, instead of copying it, matters when the
items own memory (say, strings or vectors), since the place left behind is
not read again before a `push()` overwrites it. The class also gives a
read-only, contiguous view of its items from the bottom to the top
(through `begin()` and `end()`), on which the algorithms of the standard
library run unchanged; the operations of the STACK remain the only way to
modify it.

<span id="rem-3-1"></span>**Remark 3.1.** The capacity of `ArrayStack` is
fixed since this is the realization of [Montresor 2025,
§4.3.2](Z-bibliography.md#ref-montresor2025). The realization of [Morin
2013, §2.1](Z-bibliography.md#ref-morin-cpp) instead resizes the vector to
$2 n$ places when it is full and when $3 n$ does not exceed its capacity,
which costs $O( 1 )$ amortized time per operation by an argument like that
of [Lemma 3.1](03-stack.md#lem-3-1); `std::vector` with `push_back` and
`pop_back` behaves in the same way, but it never shrinks.

## 3.3 A stack on a sequence {#sec-3-3}

Any SEQUENCE realizes a stack if its first item is taken as the top:

$$
\mathrm{top}() = \mathrm{read}( \mathrm{head}() ) , \quad
\mathrm{pop}() = \mathrm{remove}( \mathrm{head}() ) , \quad
\mathrm{push}( v ) = \mathrm{insert}( \mathrm{head}() , v ) , \tag{3.2}
$$

which is the definition of `SequenceStack` ([Montresor 2025,
§4.3.1](Z-bibliography.md#ref-montresor2025)). On a list with pointers,
even a singly linked one, the three operations of (3.2) cost $O( 1 )$ and
the stack has no capacity, at the price of one node (an item and at least
one pointer) per item. On a vector, however, an insertion at the head moves
all the items; there the top had better be the last item, as in
`ArrayStack`. The class is parametrized on the sequence, and the same code
therefore gives a stack on `List`, on `SinglyList` and on any other model of
the concept `Sequence` of `ADS/ADT.h`.

## 3.4 Several stacks in one vector {#sec-3-4}

Two stacks can share one vector of $c$ places by growing from its two ends
toward each other ([Knuth 1997, §2.2.2](Z-bibliography.md#ref-knuth1997)).
Neither of them is full while the vector has a free place, whereas two
vectors of $c / 2$ places each would refuse a `push()` as soon as one of
the two stacks reached $c / 2$ items. `MultiStack` extends the idea to $k$
stacks, without fixing a capacity in advance. Each item is a cell of a
single vector $C$, which holds the item and the index of the cell below it
in its stack; $\mathit{top}[ i ]$ is the index of the top of the stack $i$
(or $-1$ when the stack is empty), and the free cells form one more list,
whose head is $\mathit{free}$ ([Figure 3.2](03-stack.md#fig-3-2)). A
`push(i,v)` takes the first free cell, or a new cell at the end of $C$ when
there is none, and links it on top of the stack $i$; a `pop(i)` unlinks the
top of the stack $i$ and puts its cell back at the head of the free list
([Algorithm 3.2](03-stack.md#alg-3-2)). Both cost $O( 1 )$, except when
$C$ has to grow.

![Figure 3.2. A `MultiStack` with two stacks, `x1 x2 x3` (the top `x3`, in the cell 6) and `y1 y2` (the top `y2`, in the cell 4), and the free list 2, 5, 7 (in the faintest tint), the top of the stack 0 in violet; the lower row of a cell in use is the index of the cell below it in its stack, that of a free cell the index of the next free one, and $-1$ ends a list.](../figures/stack-multi.svg){#fig-3-2}

<span id="alg-3-2"></span>**Algorithm 3.2.** The operations of
`MultiStack`: the free list of [Knuth 1997,
§2.2.3](Z-bibliography.md#ref-knuth1997) on the idea of [Montresor 2025,
exercise 4.7](Z-bibliography.md#ref-montresor2025).

--8<-- "generated/MultiStack-push.pseudo.md"

--8<-- "generated/MultiStack-pop.pseudo.md"

The vector $C$ is a `std::vector`. When it is full, it is replaced by a
vector $\gamma$ times as large, into which the cells are moved, for a
constant $\gamma > 1$ fixed by the standard library ($\gamma = 2$ in those
of GCC and LLVM, $\gamma = 3 / 2$ in that of Microsoft). The indices of the
links do not change in the move, which is the reason why the cells are
linked by indices instead of pointers. An enlargement costs time linear in
the size of $C$; yet, it happens rarely enough that its cost, spread over
the operations that came before it, is constant per operation ([Tarjan
1985](Z-bibliography.md#ref-tarjan1985)).

<span id="lem-3-1"></span>**Lemma 3.1.** *Let $C$ grow by the factor
$\gamma > 1$. Starting from an empty `MultiStack`, any sequence of $m$
operations moves fewer than $\gamma m / ( \gamma - 1 )$ cells in all the
enlargements of $C$ (i.e., $2 m$ for $\gamma = 2$); hence each operation
costs $O( 1 )$ amortized time.*

*Proof.* An enlargement happens only when all the cells of $C$ are in use,
and it moves them all. Only a `push()` adds a cell to $C$, and therefore
the last enlargement of the sequence moves $s \le m$ cells. The sizes at
which $C$ is enlarged grow at least by the factor $\gamma$ from one
enlargement to the next, and hence the cells moved in all are at most
$s ( 1 + \gamma^{-1} + \gamma^{-2} + \cdots ) < \gamma s / ( \gamma - 1 )
\le \gamma m / ( \gamma - 1 )$. $\square$

The space is that of the $n$ items of all the stacks plus their $k$ tops,
i.e., $\Theta( n + k )$, but for the free cells and the unused tail of $C$
(fewer than $\gamma$ times the largest number of items reached). By
contrast, $k$ vectors, each one large enough for the largest stack, would
take $k$ times the largest size. It is fair to remark, however, that the
links cost one index per item. Also, the items of a stack are scattered
over $C$ in the order in which the free cells were reused, and a scan of
one stack is thus usually not a scan of contiguous memory as it is in
`ArrayStack`.

## 3.5 Arithmetic expressions {#sec-3-5}

An expression in *postfix* notation (the reverse Polish notation of
[Łukasiewicz 1929](Z-bibliography.md#ref-lukasiewicz1929)) writes each
operator after its two operands, e.g., `5 9 8 + 4 6 * * 7 + *` for
$5 \cdot ( ( 9 + 8 ) \cdot ( 4 \cdot 6 ) + 7 )$. It needs neither
parentheses nor precedences, since the order of the tokens is the order of
the evaluation. Formally, an operand is a postfix expression, and so is
$E_1 \, E_2 \, \mathrm{op}$ when $E_1$ and $E_2$ are. In
`ADS/stack/Expressions.h`, `evalPostfix()` scans the tokens and pushes each
operand; for each operator, it pops the right operand $t$, then the left
one $s$, and pushes $s \mathbin{\mathrm{op}} t$
([Algorithm 3.3](03-stack.md#alg-3-3)).

<span id="alg-3-3"></span>**Algorithm 3.3.** The evaluation of a postfix
expression.

--8<-- "generated/Expressions-evalPostfix.pseudo.md"

<span id="lem-3-2"></span>**Lemma 3.2.** *The scan of a postfix
expression $E$ that starts with a stack $S$ ends with $S$ and the value of
$E$ on top of it; hence `evalPostfix()` computes the value of an expression
of $n$ tokens in $O( n )$ time and $O( n )$ space.*

*Proof.* By induction on the structure of $E$: an operand is pushed; for
$E = E_1 \, E_2 \, \mathrm{op}$, the scan of $E_1$ leaves its value $s$ on
$S$, that of $E_2$ leaves its value $t$ on $S$ and $s$, and the operator
replaces the two with $s \mathbin{\mathrm{op}} t$. Each token costs one or
three operations of the stack. $\square$

A sequence of tokens that is not a postfix expression shows up as an
operator that finds fewer than two items, or as more than one item left at
the end. These are the preconditions that `evalPostfix()` checks, together
with the overflow of the arithmetic.

Translating an infix expression into a postfix one is the complementary
use of a stack, which now holds the operators. If the infix expression is
fully parenthesized, e.g., `( 5 * ( ( ( 9 + 8 ) * ( 4 * 6 ) ) + 7 ) )`,
one can write the operands as they come, push the operators, and pop one
operator at each closed parenthesis (`toPostfixParenthesized()`, after
[Sedgewick 1998, §4.3](Z-bibliography.md#ref-sedgewick1998)). Indeed, the
pair of parentheses that a closed parenthesis closes is the last one
opened. With the usual precedences ($\cdot$ and $/$ before $+$ and $-$) and
the associativity to the left, the shunting-yard algorithm of [Dijkstra
1961](Z-bibliography.md#ref-dijkstra1961) keeps on the stack the operators
whose right operand is not complete yet (`toPostfix()`,
[Algorithm 3.4](03-stack.md#alg-3-4)). An operator $x$ first pops to the
output, down to the nearest open parenthesis, each operator whose
precedence $\pi$ is not lower than that of $x$ (the right operand of each
of them ends where $x$ begins). A closed parenthesis pops the operators
down to its open one. [Table 3.2](03-stack.md#tab-3-2) follows the
algorithm on an expression with both precedences and a pair of
parentheses.

<span id="alg-3-4"></span>**Algorithm 3.4.** The shunting-yard algorithm
([Dijkstra 1961](Z-bibliography.md#ref-dijkstra1961)); $\pi( x )$ is the
precedence of the operator $x$.

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

Between two parentheses the precedences of the operators on the stack
increase strictly from the bottom up, since an operator pops those of
precedence not lower than its own before it is pushed. The test
$\pi( x ) \le \pi( \mathrm{top}( S ) )$ (line 10 of
[Algorithm 3.4](03-stack.md#alg-3-4)), with $\le$ in place of $<$, makes
the operators of equal precedence associate to the left (e.g.,
`8 - 3 - 2` becomes `8 3 - 2 -`, whose value is 3). Each token is pushed
and popped at most once, and the translation thus costs $O( n )$ time
([Samelson and Bauer 1960](Z-bibliography.md#ref-samelson1960); [Dijkstra
1961](Z-bibliography.md#ref-dijkstra1961)).

## 3.6 Algorithms on a stack {#sec-3-6}

The functions of `ADS/stack/StackAlgorithms.h` act on a stack through
`push()`, `pop()`, `top()` and `empty()` only, and are therefore written
once for all the realizations. A string is a palindrome if and only if its
characters, pushed in order and then popped, come out as the string read
from its start (the stack reverses their order); hence `isPalindrome()`
takes $O( n )$ time. Removing the items larger than a key while keeping the
order of the others (`removeGreater()`) takes an auxiliary stack, which
reverses the items kept when they are moved there and once more when they
come back, in $O( n )$ time and space.

--8<-- "generated/StackAlgorithms-removeGreater.pseudo.md"

The recursion supplies a second stack (that of the calls), and two of the
functions use no other structure. To visit the items from the bottom to the
top and leave the stack as it was, `forEachBottomUp()` pops the top, visits
the rest by a recursive call, visits the top and pushes it back; the stack
of the calls holds the items popped. In a similar way, `sortStack()` sorts
a stack by passes. A pass pops two items, pushes back the larger, makes a
recursive pass on the rest, and pushes the smaller on top
([Algorithm 3.5](03-stack.md#alg-3-5)).

<span id="alg-3-5"></span>**Algorithm 3.5.** The sort of a stack by
passes, i.e., the bubble sort on a stack.

--8<-- "generated/StackAlgorithms-pass.pseudo.md"

<span id="lem-3-3"></span>**Lemma 3.3.** *A pass carries the largest item
of the stack to its bottom; hence after $i$ passes the $i$ largest items
are at the bottom, in order, and $n$ passes sort the stack (the least item
on top) with $O( n^2 )$ operations of the stack and a stack of the calls
of depth $O( n )$.*

*Proof.* By induction on the size: the larger of the two items popped is
pushed back, and the recursive pass carries the largest item of the rest
(which is at least as large) to the bottom, while the smaller of the two is
pushed last. An item among the $i - 1$ at the bottom, which are not smaller
than any other, is never moved above them, and a pass costs $O( n )$
operations. $\square$

The small items rise by at most one place per pass, as in the bubble sort;
the bound of [Lemma 3.3](03-stack.md#lem-3-3) is therefore attained (up to
a constant) by a stack whose least item is at the bottom.

## 3.7 Discussion and references {#sec-3-7}

The stack on a vector and on a list, and the two stacks in one vector, are
in most textbooks on data structures (see, e.g., [Knuth 1997, §2.2.1 and
§2.2.2](Z-bibliography.md#ref-knuth1997)). There one also finds $k$ stacks
of variable size in one area, kept contiguous by repacking the area when a
stack overflows; this keeps each stack in contiguous memory, at the price
of a cost per overflow that is not constant. The free list of `MultiStack`
is that of the dynamic storage allocation of [Knuth 1997,
§2.2.3](Z-bibliography.md#ref-knuth1997). The amortized analysis of the
enlargements is that of [Tarjan 1985](Z-bibliography.md#ref-tarjan1985),
and it returns with the sequences that grow and shrink ([Morin 2013,
§2.1](Z-bibliography.md#ref-morin-cpp)). The evaluation of the postfix
expressions with a stack goes back to the compilers of the late fifties
([Samelson and Bauer 1960](Z-bibliography.md#ref-samelson1960)), and the
shunting-yard algorithm to the first Algol 60 compiler ([Dijkstra
1961](Z-bibliography.md#ref-dijkstra1961)); [Sedgewick 1998,
§4.3](Z-bibliography.md#ref-sedgewick1998) has both, together with the
translation of the fully parenthesized expressions.

## 3.8 Exercises {#sec-3-8}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize two stacks in one vector of $c$ places, so that a `push()` on
   either of them succeeds while the vector has a free place ([Montresor
   2025, exercise 4.7](Z-bibliography.md#ref-montresor2025)). Test:
   `ch04.ex4.7`.
2. Realize the STACK on a singly linked list, with each operation in
   $O( 1 )$ time, and explain why the top must be the head of the list
   instead of its tail. Test: `ch04.sec4.3.1.stack-on-list`.
3. Realize $k$ stacks in one vector with no capacity fixed in advance, in
   space $O( n + k )$ and with each operation in $O( 1 )$ amortized time.
   Test: `beyond.multistack`.
4. Evaluate an expression in postfix notation, rejecting the sequences of
   tokens that are not postfix expressions and the results that overflow,
   and translate an infix expression with the usual precedences into
   postfix notation in $O( n )$ time. Test: `beyond.postfix`.
5. Using only the operations of the STACK and recursion, decide whether a
   string is a palindrome, remove the items larger than a key keeping the
   order of the others, and sort a stack. Test: `beyond.stack-algorithms`.
6. Visit the items of a stack from the bottom to the top and leave it as
   it was, using no structure other than the stack of the calls. Test:
   `beyond.stack-bottom-up`.
