# 11. Disjoint sets {#ch-11}

[Source: `include/ADS/mfset/ListMFSet.h`, `MFSet.h`, `UnionFind.h`,
`RemUnionFind.h`, `RollbackUnionFind.h`]

We are concerned in this chapter with the MFSET (merge-find set), i.e.,
with a partition of a set of elements into disjoint components that only
merge, and with the question of whether two elements are in the same
component. It is the structure that follows the connected components of a
graph whose edges arrive one at a time (the cycles that the algorithm of
Kruskal must refuse, cf. [Chapter 14](14-graph.md#ch-14)), and the classes of an equivalence
relation given by its pairs. The headers of `ADS/mfset` realize it on lists
([§11.2](11-mfset.md#sec-11-2)) and on a forest of parents with the
heuristics of [Montresor 2025, §10.2](Z-bibliography.md#ref-montresor2025)
([§11.3](11-mfset.md#sec-11-3)); then come two engineered forests
([§11.4](11-mfset.md#sec-11-4)) and a forest whose merges can be undone,
on which the connectivity of a graph whose edges come and go is answered
offline ([§11.5](11-mfset.md#sec-11-5)). [Table 11.1](11-mfset.md#tab-11-1)
summarizes the costs, and [§11.6](11-mfset.md#sec-11-6) compares the
realizations on what each one is meant for.

## 11.1 The interface {#sec-11-1}

An MFSET is a partition of the elements $\{ 0 , \ldots , n - 1 \}$ into
disjoint *components*. It starts with the $n$ singletons
$\{ 0 \} , \ldots , \{ n - 1 \}$ (the `Mfset(n)` of [Montresor 2025,
§10.2](Z-bibliography.md#ref-montresor2025)), and changes by two
operations. $\mathrm{find}( x )$ gives the *representative* of the
component of $x$, an element of it that stays the same until the
component changes; hence, $x$ and $y$ are in the same component if and
only if

$$
\mathrm{find}( x ) = \mathrm{find}( y ) . \tag{11.1}
$$

$\mathrm{merge}( x , y )$ replaces the components of $x$ and of $y$ with
their union, and components are never split. Some specifications require
$x$ and $y$ to be in distinct components; all the realizations of the
chapter relax this precondition, since a `merge()` of two elements already
together returns false and leaves the partition as it was. Each realization
also has `same(x, y)`, i.e., the test (11.1), `n()`, the number of the
elements, and (but for `ListMFSet`) `count()`, the number of the
components. Throughout the chapter $m$ is
the number of the operations of a sequence of finds and merges, $p[ x ]$
the parent of $x$ in a forest, $\mathit{rank}[ r ]$ and
$\mathit{size}[ r ]$ the rank and the number of nodes of the tree of the
root $r$, and $\alpha( m , n )$ the inverse of the function of Ackermann,
which is at most 4 for any $m$ and $n$ that fit in a computer ([Tarjan
1975](Z-bibliography.md#ref-tarjan1975)).

<span id="tab-11-1"></span>**Table 11.1.** The realizations of the MFSET
in `ADS/mfset` and the cost of their operations, for $m \ge n$ operations.

| Realization | Representation | `find()` | `merge()` | $m$ operations |
|---|---|---|---|---|
| `ListMFSet` | a list per component, each element with its head | $O( 1 )$ | $O( n )$, the shorter list relabeled | $O( m + n \log n )$ |
| `MFSet`, no heuristic | a forest of parents | $O( n )$ | that of two finds | $O( m n )$ |
| `MFSet`, union by rank or size | as above | $O( \log n )$ | that of two finds | $O( m \log n )$ |
| `MFSet` with compression, `UnionFind` | as above, or one vector with the sizes at the roots | $O( \log n )$ | that of two finds | $O( m \, \alpha( m , n ) )$ |
| `RemUnionFind` | a forest with $p[ x ] \ge x$ | $O( n )$ | one interleaved climb | $O( m \log_{1 + m / n} n )$ |
| `RollbackUnionFind` | a forest by rank, no compression, a stack of the merges | $O( \log n )$ | that of two finds; `undo()` in $O( 1 )$ | $O( m \log n )$ |

## 11.2 Lists {#sec-11-2}

`ListMFSet` keeps a list per component, whose first element (its *head*)
is the representative, and each element keeps the head of its list in the
vector $\mathit{head}$; `find(x)` thus reads $\mathit{head}[ x ]$ in
$O( 1 )$ time. `merge(x, y)` appends the shorter of the two lists to the
longer one and relabels its elements, i.e., writes the new head into their
places of $\mathit{head}$ ([Algorithm 11.1](11-mfset.md#alg-11-1) and
[Figure 11.1](11-mfset.md#fig-11-1)). In the code the lists are vectors,
the list of a head being empty once the head has been merged into another
list.

![Figure 11.1. A `ListMFSet` of 6 elements with the components $\{ 0 , 3 \}$, $\{ 1 , 4 , 5 \}$ and $\{ 2 \}$, before and after `merge(3, 4)`, which appends the shorter list to the longer and relabels its elements (violet).](../figures/mfset-lists.svg){#fig-11-1}

<span id="alg-11-1"></span>**Algorithm 11.1.** The merge of `ListMFSet`
([Montresor 2025, §10.2.1](Z-bibliography.md#ref-montresor2025));
$\mathit{head}[ x ]$ is the head of the list of $x$ and $L[ a ]$ the list
of the head $a$.

--8<-- "generated/ListMFSet-merge.pseudo.md"

<span id="lem-11-1"></span>**Lemma 11.1.** *If `merge()` relabels the
elements of the shorter list, an element is relabeled at most
$\lfloor \log_2 n \rfloor$ times, and any sequence of $m$ operations costs
$O( m + n \log n )$ time.*

*Proof.* An element is relabeled only when its list is the shorter of the
two, and the list in which it lands is then at least twice as long as its
old one ([Hopcroft and Ullman 1973](Z-bibliography.md#ref-hopcroft1973)).
Since a list has at most $n$ elements, this happens at most
$\lfloor \log_2 n \rfloor$ times per element. Apart from the relabeling,
each operation costs $O( 1 )$ time. $\square$

The bound is tight, as the merges of the pairs, then of the quadruples,
and so on, show: each round relabels $n / 2$ elements, and there are
$\log_2 n$ rounds. The counter `relabels()` lets one check
[Lemma 11.1](11-mfset.md#lem-11-1) on any sequence. Also, `members(x)` gives
the elements of the component of $x$, which a forest cannot give without
a visit of all the elements.

## 11.3 A forest of parents {#sec-11-3}

`MFSet` keeps the components as a forest, a vector $p$ of parents in
which the roots, with $p[ x ] = x$, are the representatives ([Montresor
2025, §10.2.2](Z-bibliography.md#ref-montresor2025); [Galler and Fischer
1964](Z-bibliography.md#ref-galler1964)). `find(x)` climbs from $x$ to
the root of its tree, and `merge(x, y)` makes one of the two roots a child
of the other, after the two finds ([Algorithm 11.2](11-mfset.md#alg-11-2)
and [Figure 11.2](11-mfset.md#fig-11-2)). The two realizations thus trade
one cost for the other: on lists the finds are cheap and the merges
expensive, on a forest the converse. With no heuristic, the root of $y$
goes under that of $x$, and $n - 1$ merges may build a path of $n$ nodes,
whose deepest node costs $n - 1$ steps to each find. Two heuristics bound
the height of the trees, and may be switched off to study them (the
argument `Union` of the constructor, `Union::NONE`, `RANK` or `SIZE`, and
its argument `compress`).

* The *union by rank* ([Montresor 2025,
  §10.2.3](Z-bibliography.md#ref-montresor2025)) keeps for each root $r$ a
  rank, 0 for a singleton. It puts the root of lower rank under the other
  one, and the rank grows by 1 only when the two ranks are equal.
* The *union by size* puts the tree with fewer nodes under the other one,
  whose size grows by that of the smaller tree (on a tie, the root of $y$
  goes under that of $x$).

![Figure 11.2. Two trees of an `MFSet` of 7 elements with the union by rank and no compression (the element 1 is a tree of its own), before and after `merge(0, 4)`: the root 2, of rank 1, goes under the root 5, of rank 2, whose rank does not change; below, the vector of the parents after the merge, the violet place being the one written.](../figures/mfset-forest.svg){#fig-11-2}

<span id="alg-11-2"></span>**Algorithm 11.2.** The operations of `MFSet`
([Montresor 2025, §10.2.2, §10.2.3 and
§10.2.4](Z-bibliography.md#ref-montresor2025)), `find()` in the iterative
version of exercise 10.8, the root first and then the compression.

--8<-- "generated/MFSet-find.pseudo.md"

--8<-- "generated/MFSet-merge.pseudo.md"

<span id="lem-11-2"></span>**Lemma 11.2.** *With the union by rank, a tree
whose root has rank $r$ has at least $2^r$ nodes and height at most $r$;
hence $r \le \log_2 n$, and `find()` and `merge()` take $O( \log n )$
time ([Montresor 2025, Theorem 10.1](Z-bibliography.md#ref-montresor2025)).
The same bound holds with the union by size, a tree of the root $r$ having
height at most $\log_2 \mathit{size}[ r ]$.*

*Proof.* By induction on the merges. A singleton has rank 0 and one node.
A merge of two roots of different ranks keeps the rank and the height of
the higher tree, and only adds nodes to it. A merge of two roots of equal
rank $r$ gives the rank $r + 1$ and a height at most $r + 1$ to a tree of
at least $2^r + 2^r = 2^{r+1}$ nodes. With the union by size, a node goes
one level down only when its tree joins one at least as large. The tree
that holds it then at least doubles, as in the proof of
[Lemma 11.1](11-mfset.md#lem-11-1); its depth is thus at most
$\log_2 \mathit{size}[ r ]$. $\square$

A find that has climbed from $x$ to its root $r$ may also make each node
of the path a child of $r$, the *compression* of [Montresor 2025,
§10.2.4](Z-bibliography.md#ref-montresor2025), so that the next finds from
those nodes cost one step ([Figure 11.3](11-mfset.md#fig-11-3)). In
`MFSet` the compression is a second pass over the path, with no recursion
and no stack:

```cpp
--8<-- "generated/MFSet-find.cpp"
```

![Figure 11.3. A path from 0 to its root 5, and the same nodes after `find(0)` with the compression (each node of the path a child of the root) and with the halving of [§11.4](11-mfset.md#sec-11-4) (every other node of the way a child of its grandparent); the violet nodes are those whose parent changed.](../figures/mfset-paths.svg){#fig-11-3}

With the compression a rank is no longer the height of its tree, but only
an upper bound to it (exercise 10.7 of [Montresor
2025](Z-bibliography.md#ref-montresor2025)), since the compression lowers
the trees and never changes a rank. The two heuristics together give an
amortized cost per operation that is constant for any practical purpose.

<span id="thm-11-3"></span>**Theorem 11.3.** *With the union by rank or by
size, and the compression or the halving of the paths, a sequence of
$m \ge n$ operations on $n$ elements costs*

$$
O( m \, \alpha( m , n ) ) \tag{11.2}
$$

*time, and the bound is tight for each of these combinations ([Tarjan
1975](Z-bibliography.md#ref-tarjan1975); [Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984)).*

We do not repeat the proof of (11.2), which [Montresor 2025,
§10.2.4](Z-bibliography.md#ref-montresor2025) sketches and [Tarjan and van
Leeuwen 1984](Z-bibliography.md#ref-tarjan1984) give in full. We prove
instead the weaker bound of [Hopcroft and Ullman
1973](Z-bibliography.md#ref-hopcroft1973), whose argument (the charge of
each step of a find either to the operation or to the node, whose parent
then becomes one of larger rank) is the one that the proof of (11.2)
refines over the levels of the function of Ackermann. Let
$T( 0 ) = 1$ and $T( g + 1 ) = 2^{T( g )}$, so that $T( 1 ) = 2$,
$T( 2 ) = 4$, $T( 3 ) = 16$ and $T( 4 ) = 65536$, and let $\log^* n$ be
the least $g$ with $T( g ) \ge n$.

<span id="lem-11-4"></span>**Lemma 11.4.** *With the union by rank and
the compression of the paths, a sequence of $m$ finds and merges on $n$
elements costs $O( ( m + n ) \log^* n )$ time.*

*Proof.* A merge costs two finds and $O( 1 )$, and hence it suffices to
count the steps of the finds, i.e., the nodes of the paths that they
climb. Three facts hold throughout. (i) The rank of a node no longer
changes once the node is not a root. (ii) The ranks increase strictly from
a child to its parent, since a merge hangs a root under one of larger rank
or raises the rank of the new parent, and a compression gives a node a
parent among its ancestors. (iii) At most $n / 2^r$ nodes have rank $r$.
Indeed, the ranks are those of the same sequence with no compression,
which has the same roots, and in its forest a node of rank $r$ has at least $2^r$ descendants by
[Lemma 11.2](11-mfset.md#lem-11-2), and two nodes of equal rank have
disjoint descendants, since a common one would make one of them an
ancestor of the other, of larger rank by (ii). We put the rank $r$ in the *group* 0 if
$r \le 1$, and in the group $g \ge 1$ if $T( g - 1 ) < r \le T( g )$; since
$r \le \log_2 n$, there are at most $\log^* n + 1$ groups. We charge a step of a find
from a node $x$ to the find if $x$ is the root or a child of the
root, or if the parent of $x$ has a rank of a higher group than that of
$x$; by (ii), the last case happens at most $\log^* n + 1$ times along a
path, and a find takes at most $\log^* n + 3$ such charges. Any other step is charged to $x$,
whose parent $p$ is not the root, and the compression then gives $x$ the
root as its parent, of rank larger than that of $p$ by (ii). Hence each
charge to $x$ raises the rank of its parent by at least 1, while that of
$x$ stays the same by (i), and once the rank of the parent is in a higher
group $x$ is never charged again. A node of rank in the group $g \ge 1$ is
thus charged fewer than $T( g )$ times, and one of the group 0 at most
once. By (iii) the group $g \ge 1$ has at most
$\sum_{r > T( g - 1 )} n / 2^r \le n / 2^{T( g - 1 )} = n / T( g )$ nodes,
which take fewer than $n$ charges in all, and the group 0 takes at most
$n$. Summing over the at most $\log^* n + 1$ groups, the charges to the
nodes are $O( n \log^* n )$, and those to the finds $O( m \log^* n )$.
$\square$

The same proof holds with the union by size, once the rank of a node is
taken as $\lfloor \log_2 s \rfloor$, $s$ the size of its tree when it was
hung (the current size for a root): a node is hung only under a root of at
least twice its size, which gives (ii) and (iii). Since $\log^* n \le 5$
for $n \le 2^{65536}$, [Lemma 11.4](11-mfset.md#lem-11-4) already makes
the amortized cost constant for any practical purpose, and (11.2) improves
it only in theory. We remark that both bounds are amortized: a single find
may still cost $\Theta( \log n )$ time.

Finally, `components(G)` builds the MFSET of the connected components of
an unoriented graph by merging the ends of each edge ([Montresor 2025,
§10.2.5](Z-bibliography.md#ref-montresor2025)), after which it can follow
new edges by more merges ([Algorithm 11.3](11-mfset.md#alg-11-3)). Its
elements are the names $[ 0 , \mathrm{bound}() )$ of the graph, so that a
name that is not a node (e.g., a deleted one) is a component of its own,
counted by `count()`.

<span id="alg-11-3"></span>**Algorithm 11.3.** The connected components of
an unoriented graph $G$ by an MFSET ([Montresor 2025,
§10.2.5](Z-bibliography.md#ref-montresor2025)).

--8<-- "generated/MFSet-components.pseudo.md"

The contracts of `MFSet` check that the parents form a forest of
`count()` trees, that the ranks grow strictly towards the roots with the
union by rank, and that the sizes count the nodes of their trees with the
union by size. The accessors `parent(x)`, `rank(x)` and `size(x)` read the
three vectors (the size being meaningful at a root with the union by size
only). The counter `steps()` gives the steps of the climbs, on which the
bounds above can be measured. With the compression and no union by rank or
by size, a sequence of $m \ge n$ operations costs
$\Theta( m \log_{1 + m / n} n )$ time in the worst case ([Tarjan and van
Leeuwen 1984](Z-bibliography.md#ref-tarjan1984)), between the two bounds
of [Table 11.1](11-mfset.md#tab-11-1).

## 11.4 Engineered forests {#sec-11-4}

`DisjointSets<L, C>` (and its instance `UnionFind`, i.e.,
`DisjointSets<>`, with the union by size and the halving) keeps the forest
in one vector of integers. A root $r$ holds $-\mathit{size}[ r ]$ (or
$-1 - \mathit{rank}[ r ]$, with `L` equal to `Union::RANK`) in place of a
parent; hence the union needs no second vector, and a find touches one
array only, of $4 n$ bytes. In either encoding the more negative root is
the larger tree, which takes the other one under it, and on a tie the root
of $x$ goes under that of $y$ (the converse of `MFSet`). Its `find()`
shortens the path by *halving* it: every other node on the way is made a
child of its grandparent, in the same pass that climbs, with no second pass
and no stack ([Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984);
[Figure 11.3](11-mfset.md#fig-11-3)). The halving keeps the bound (11.2)
of the compression, and a find still costs $O( \log n )$ in the worst
case, by [Lemma 11.2](11-mfset.md#lem-11-2), since a tree of height $h$
has at least $2^h$ nodes ([Algorithm 11.4](11-mfset.md#alg-11-4)). The
full compression, in two passes, is the alternative `C` equal to
`Compression::FULL` (the two enumerations `Union` and `Compression` of the
namespace are distinct from the `MFSet::Union` of
[§11.3](11-mfset.md#sec-11-3)). Besides `find()`, `merge()` and `same()`,
the class has `reset(n)`, which returns to the $n$ singletons in $O( n )$
time. It also has `n()`, `count()` and, with the union by size only (a constraint
`requires` of the member), `size(x)`, the number of the elements of the
component of $x$; its contracts check, at the highest level, that the
parents make a forest of `count()` trees whose roots hold their sizes, or
ranks that bound their heights.

<span id="alg-11-4"></span>**Algorithm 11.4.** The operations of
`UnionFind` (the halving of [Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984) and the union by size); a root $r$
holds $p[ r ] = -\mathit{size}[ r ]$.

--8<-- "generated/UnionFind-DisjointSets-find.pseudo.md"

--8<-- "generated/UnionFind-DisjointSets-merge.pseudo.md"

The two shortenings of the path are chosen at compile time:

```cpp
--8<-- "generated/UnionFind-find.cpp"
```

`RemUnionFind` is the algorithm of Rem with *splicing* ([Dijkstra
1976](Z-bibliography.md#ref-dijkstra1976)), which the experiments of
[Patwary et al. 2010](Z-bibliography.md#ref-patwary2010) found among the
fastest sequential algorithms of union and find. Its parents are 32-bit
integers, and a parent is never smaller than its child,

$$
p[ x ] \ge x \quad \text{for each } x , \tag{11.3}
$$

the roots having $p[ x ] = x$; hence, a path increases strictly to its
root. This *link by index* takes the place of the union by rank, with no
array of ranks and no sizes. `merge(x, y)` climbs the two paths from $x$
and $y$ together, always from the node $a$ whose parent is smaller. If $a$
is a root, it is linked under the parent of the other node $b$, and the
two sets are one. Otherwise, the parent of $a$ is replaced by the larger
parent of $b$ (the splicing), and the climb goes on from the old parent of
$a$. When the two climbs reach the same parent, $x$ and $y$ were already
in the same set ([Algorithm 11.5](11-mfset.md#alg-11-5) and
[Table 11.2](11-mfset.md#tab-11-2)). Its `find()` halves the path, and the
class also has `same()`, `n()`, `count()` and `parent(x)`; the elements
are of type `std::uint32_t`, and the constructor requires $n < 2^{32}$.

<span id="alg-11-5"></span>**Algorithm 11.5.** The operations of
`RemUnionFind` (the algorithm of Rem with splicing as [Dijkstra
1976](Z-bibliography.md#ref-dijkstra1976) and [Patwary et al.
2010](Z-bibliography.md#ref-patwary2010) describe it).

--8<-- "generated/RemUnionFind-find.pseudo.md"

--8<-- "generated/RemUnionFind-merge.pseudo.md"

<span id="tab-11-2"></span>**Table 11.2.** The algorithm of Rem on the
parents $p = ( 3 , 4 , 3 , 3 , 5 , 5 )$, i.e., the sets $\{ 0 , 2 , 3 \}$
(root 3) and $\{ 1 , 4 , 5 \}$ (root 5): `merge(0, 1)` joins them after one
splicing, and `merge(2, 0)` then finds them together after one more.

| Step | $a$, $b$ | $p[ a ]$, $p[ b ]$ | Action | $p$ after |
|---|---|---|---|---|
| `merge(0, 1)` | 0, 1 | 3, 4 | splice: $p[ 0 ] \gets 4$, $a \gets 3$ | $( 4 , 4 , 3 , 3 , 5 , 5 )$ |
| | 3, 1 | 3, 4 | $a$ is a root: $p[ 3 ] \gets 4$, true | $( 4 , 4 , 3 , 4 , 5 , 5 )$ |
| `merge(2, 0)` | 2, 0 | 3, 4 | splice: $p[ 2 ] \gets 4$, $a \gets 3$ | $( 4 , 4 , 4 , 4 , 5 , 5 )$ |
| | 3, 0 | 4, 4 | the same parent: false | $( 4 , 4 , 4 , 4 , 5 , 5 )$ |

<span id="lem-11-5"></span>**Lemma 11.5.** *Each operation of
`RemUnionFind` keeps (11.3), and `merge(x, y)` joins the sets of $x$ and
$y$ and returns true if they were distinct, and returns false otherwise,
after at most $2 n$ steps.*

*Proof.* At each step, after the swap, $p[ a ] < p[ b ]$. A root $a$ has
$p[ a ] = a$, and its new parent $p[ b ]$ is thus larger than $a$; a
splicing replaces $p[ a ]$ with the larger $p[ b ]$. Both keep (11.3).
The halving of `find()` replaces a parent with a grandparent, which is
larger by (11.3). Let the sets of $x$ and $y$ be $X$ and $Y$, and let $X \ne Y$. While
the loop runs, $a$ and $b$ stay in two different trees: a splicing moves
the subtree of $a$ into the tree of $b$, but $a$ then moves to its old
parent, which is still in its tree. The parents of $a$ and $b$ are thus
never equal, and the loop ends only by the link of a root, which joins the
two trees; since the splicings only move elements of $X \cup Y$ from one
tree to the other, the two trees then hold $X \cup Y$. If $X = Y$, no root is ever linked (the root of the set is
the largest element, and the node of smaller parent is a root only if both
parents are that root), and the splicings only shorten the paths. Each
step moves $a$ to a strictly larger index; hence there are at most $2 n$
steps for the two nodes. $\square$

The climb stops as soon as the sets are joined, often well below the
roots, and each step both compresses and moves on; the two finds and the
link of the union by rank become one interleaved walk:

```cpp
--8<-- "generated/RemUnionFind-merge.cpp"
```

Yet, the link by index does not give the bound (11.2) in the worst case:
with the splicing or the halving it gives $O( m \log_{1 + m / n} n )$
time for $m$ operations ([Tarjan and van Leeuwen
1984](Z-bibliography.md#ref-tarjan1984)). When the elements are numbered
in a random order, however, the expected time is $O( m \, \alpha( m , n ) )$,
the bound of the union by rank ([Goel et al.
2014](Z-bibliography.md#ref-goel2014)). The contracts check that $x$ and
$y$ have the same root after a merge, by climbs that change no parent (a
`find()` would halve the paths, and the contracts would change the forest
that they check), and (11.3) and the number of the roots after each merge,
at their highest level and on the sets of up to 4096 elements.

## 11.5 Undo and offline connectivity {#sec-11-5}

`RollbackUnionFind` undoes its merges in the reverse of their order
([Mannila and Ukkonen 1986](Z-bibliography.md#ref-mannila1986)). Each
`merge()` changes at most three words, i.e., the parent of the root that it
hangs and the size and the rank of the other root. It pushes on a stack
$R$ a record with the root hung, the other root and whether the rank
grew. `undo()` pops a record and puts the words back, in
$O( 1 )$ time, and `rollback(t)` undoes the merges back to the checkpoint
$t$, the number of merges done when `checkpoint()` gave it
([Algorithm 11.6](11-mfset.md#alg-11-6) and
[Figure 11.4](11-mfset.md#fig-11-4)). A merge of two elements already
together pushes an empty record, and the checkpoints thus count the calls.
The space is $12 n$ bytes (the parents, the ranks and the sizes), and 12
bytes per record. The class also has `same()`, `size(x)` (the number of
the elements of the component of $x$), `n()`, `count()` and `reset(n)`,
which returns to the $n$ singletons and empties the stack; `undo(k)`
undoes the last $k$ merges, one by default.

![Figure 11.4. A `RollbackUnionFind` with two roots of rank 0 and the stack $R$ of the records, before `merge(0, 1)`, after it (0 under 1, whose rank grew, and the record pushed) and after `undo()`, which pops the record and restores the forest.](../figures/mfset-rollback.svg){#fig-11-4}

<span id="alg-11-6"></span>**Algorithm 11.6.** The operations of
`RollbackUnionFind` (the union by rank with no compression, and the stack
$R$ of the merges).

--8<-- "generated/RollbackUnionFind-find.pseudo.md"

--8<-- "generated/RollbackUnionFind-merge.pseudo.md"

--8<-- "generated/RollbackUnionFind-undo.pseudo.md"

The forest keeps the union by rank and gives up any change of the paths.
A compression would change $\Theta( \log n )$ parents per find, all to be
recorded, and its amortized bound (11.2) does not survive the undo: a
sequence that rolls back the merges and does them again can make the same
long find pay each time.

<span id="lem-11-6"></span>**Lemma 11.6.** *In `RollbackUnionFind`,
`undo()` restores the forest, the ranks and the sizes as they were before
the merge that it undoes, and each `find()` costs $O( \log n )$ time in
the worst case.*

*Proof.* With no compression, a merge changes only the parent of the root
it hangs, the size of the other root and possibly its rank; the record
holds the two roots and whether the rank grew, which is all that `undo()`
needs to put them back, the parent of a root being the root itself. The
merges are undone in the reverse of their order; hence, the forest
before each one is the forest after the undo of the later ones. The
forest is thus always one that the union by rank could build with no
undo, and [Lemma 11.2](11-mfset.md#lem-11-2) bounds the height of its
trees. $\square$

It is fair to remark that this is not the best amortized bound with
backtracking: [Westbrook and Tarjan
1989](Z-bibliography.md#ref-westbrook1989) show that some forms of
compression, adapted to the undo, give $O( \log n / \log \log n )$
amortized time per operation, and that no separable algorithm on pointers
does better. The header keeps the simpler forest, whose bound
$O( \log n )$ holds for each single operation. The code of the undo follows
[Algorithm 11.6](11-mfset.md#alg-11-6):

```cpp
--8<-- "generated/RollbackUnionFind-undo.cpp"
```

The undo makes it possible to answer offline the questions of
connectivity on a graph whose edges come and go
(`offlineConnectivity(n, ops)`), when the sequence of $k$ insertions,
deletions and questions (whether two nodes are connected, or how many
components there are) is known in advance. Each operation is a
`ConnectivityOp`, whose `kind` is `INSERT`, `DELETE`, `CONNECTED` or
`COMPONENTS` and whose `u` and `v` are the two nodes (ignored by
`COMPONENTS`). The edges are unoriented, so that $[ v , u ]$ deletes the
edge inserted as $[ u , v ]$, and the result holds the answers in the order
of the questions, 1 or 0 for `CONNECTED` and the number of the components
for `COMPONENTS`. Each edge lives on an interval of times
$[ s , e )$, from its insertion to its deletion (or to the end); the times
are counted in questions (an edge inserted and deleted with no question in
between lives on an empty interval and is dropped), and the $q$ questions are the leaves of a
segment tree. The node of the range $[ a , b )$ holds the list $L$ of the
edges that live on all of $[ a , b )$ but not on all of the range of its
parent ([Figure 11.5](11-mfset.md#fig-11-5)). A visit in depth of the
segment tree merges the edges of $L$ when it enters a node, answers the
question when it is at a leaf, and rolls back to its checkpoint when it
leaves the node ([Algorithm 11.7](11-mfset.md#alg-11-7)). Of parallel
edges, the last one inserted dies first, and a loop $[ u , u ]$ is
allowed and joins no component.

![Figure 11.5. The segment tree over $q = 4$ questions, and the nodes (violet) whose list $L$ holds an edge $e$ alive from the question 1 to the end, i.e., on the range $[ 1 , 4 )$ of the questions 1, 2 and 3.](../figures/mfset-segtree.svg){#fig-11-5}

<span id="alg-11-7"></span>**Algorithm 11.7.** The offline connectivity
by the divide and conquer of [Eppstein
1994](Z-bibliography.md#ref-eppstein1994) on the times; $q( t )$ is the
number of the questions before the time $t$ and $L( x )$ the list of the
edges of the node $x$ of the segment tree.

--8<-- "generated/RollbackUnionFind-offlineConnectivity.pseudo.md"

<span id="lem-11-7"></span>**Lemma 11.7.** *An interval of times is held
by at most $\max( 1 , 2 \lceil \log_2 q \rceil )$ nodes of the segment
tree, and when the visit reaches the leaf of a question, the forest holds
exactly the edges alive at its time. Hence the answers are those of a
union-find built again at each question, and the time is*

$$
O( k \log q \log n ) = O( k \log k \log n ) . \tag{11.4}
$$

*Proof.* Since the middle $c$ of a range $[ a , b )$ halves it rounding
up, the segment tree has depth at most $\lceil \log_2 q \rceil$, and the
ranges of the nodes of one depth are disjoint. Let $[ s , e )$ be the
interval; a node that `add()` reaches and that neither holds its range in
$[ s , e )$ nor is disjoint from it (a *partial* node) has $s$ or $e$
strictly inside its range, and hence there are at most two partial nodes
per depth, one for $s$ and one for $e$. A node that takes the interval is
a child of a partial node, and a partial node has at most one such child:
if both its children were inside $[ s , e )$, so would be its range. If the
root takes the interval, that is the only node; otherwise the nodes taken
lie at the depths 1 to $\lceil \log_2 q \rceil$, at most two per depth,
since each is the child of one of the at most two partial nodes of the
depth above. As for the leaves, the ranges of the
nodes taken partition $[ s , e )$. Hence exactly one of the ancestors
of the leaf $a$ (the leaf included) holds the edge if $a \in [ s , e )$,
and none otherwise; an edge is alive at the question $a$ exactly when
$s \le a < e$, since $s$ and $e$ are the numbers of the questions before
its insertion and before its deletion. The visit merges at a node the edges
of its list and rolls back to its checkpoint when it leaves it, which by
[Lemma 11.6](11-mfset.md#lem-11-6) restores the forest of the parent;
hence at the leaf $a$ the forest holds the edges of the lists of its
ancestors, i.e., those alive at $a$, each merged once. Each edge is thus
merged and undone at most $\max( 1 , 2 \lceil \log_2 q \rceil )$
times, in $O( \log n )$ and $O( 1 )$ time respectively (by
[Lemma 11.2](11-mfset.md#lem-11-2) for the merge). The visit enters
$2 q - 1$ nodes, and the $q$ answers cost $O( \log n )$ each, which gives
(11.4) with $q \le k$; the lists take $O( k \log k )$ space. $\square$

With the contraction of the graph at each level of the divide and conquer
([Eppstein 1994](Z-bibliography.md#ref-eppstein1994); [Peng et al.
2019](Z-bibliography.md#ref-peng2019)) the bound falls to $O( k \log n )$,
at the price of a far longer code; the rollback avoids it.

## 11.6 The realizations compared {#sec-11-6}

`ListMFSet` and `MFSet` are the realizations of the textbook, with the
heuristics that can be switched off, and they are meant to study the
bounds of [Lemma 11.1](11-mfset.md#lem-11-1),
[Lemma 11.2](11-mfset.md#lem-11-2) and
[Theorem 11.3](11-mfset.md#thm-11-3) on the counters `relabels()` and
`steps()`; `ListMFSet` is also the one that lists the members of a
component. Among the engineered forests, `UnionFind` keeps the worst-case
bound (11.2) in the least memory ($4 n$ bytes), and is the union-find of
the algorithms of Kruskal and of the connected components of the library.
`RemUnionFind` has no worst-case guarantee of the same strength, but stops
its merges early and takes no sizes. [Patwary et al.
2010](Z-bibliography.md#ref-patwary2010) found the two close to each
other, and the ranking may well depend on the order of the merges and on
the size of the sets; the comparison on the workloads of the library
belongs to [Appendix B](B-benchmarks.md#app-B), where it will be made on
an idle machine.
`RollbackUnionFind` is slower per operation by design, since it gives up
the compression, and is used only when the merges must be undone.

## 11.7 Discussion and references {#sec-11-7}

The forest of parents is that of [Galler and Fischer
1964](Z-bibliography.md#ref-galler1964), the relabeling of the shorter
list that of [Hopcroft and Ullman 1973](Z-bibliography.md#ref-hopcroft1973),
and the bound (11.2), with its lower bound on the forests of pointers, is
in [Tarjan 1975](Z-bibliography.md#ref-tarjan1975); [Tarjan and van
Leeuwen 1984](Z-bibliography.md#ref-tarjan1984) analyze all the
combinations of linking and of shortening of the paths, the halving and
the splitting included. The algorithm of Rem is in [Dijkstra
1976](Z-bibliography.md#ref-dijkstra1976), its experimental comparison
with the other variants in [Patwary et al.
2010](Z-bibliography.md#ref-patwary2010), and the analysis of the
randomized linking by index in [Goel et al.
2014](Z-bibliography.md#ref-goel2014). The set union with backtracking is
due to [Mannila and Ukkonen 1986](Z-bibliography.md#ref-mannila1986), with
the analysis of [Westbrook and Tarjan
1989](Z-bibliography.md#ref-westbrook1989) and the unlimited backtracking
of [Apostolico et al. 1994](Z-bibliography.md#ref-apostolico1994); the
offline dynamic connectivity is the divide and conquer of [Eppstein
1994](Z-bibliography.md#ref-eppstein1994). A union-find in which several
threads merge at once, by compare-and-swap on the parents, is in
`ADS/concurrent/ConcurrentUnionFind.h` and is treated with the concurrent
structures ([Chapter 24](24-concurrent.md#ch-24)).

## 11.8 Exercises {#sec-11-8}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize the MFSET on lists, with `find()` in $O( 1 )$ time and the
   relabeling of the shorter list, and check that an element is relabeled
   at most $\log_2 n$ times ([Montresor 2025,
   §10.2.1](Z-bibliography.md#ref-montresor2025)). Test:
   `ch10.sec10.2.1.lists`.
2. Realize the MFSET on a forest of parents, and build the forest of
   Figure 10.5 of [Montresor 2025](Z-bibliography.md#ref-montresor2025).
   Test: `ch10.sec10.2.2.forest`.
3. Add the union by rank, and check that a tree of rank $r$ has at least
   $2^r$ nodes (Theorem 10.1); add the union by size, and check the same
   bound on the heights. Test: `ch10.sec10.2.3.rank`.
4. Add the compression of the paths, and check on Figure 10.6 that a find
   flattens its path. Test: `ch10.sec10.2.4.compression`.
5. Show that with the compression the rank of a root is only an upper
   bound to the height of its tree (exercise 10.7), and write the
   compression with no recursion and no stack (exercise 10.8). Tests:
   `ch10.ex10.7`, `ch10.ex10.8`.
6. Follow the connected components of a graph whose edges arrive one at a
   time, and find the edge of least weight of each component (exercise
   10.9). Tests: `ch10.sec10.2.5.incremental`, `ch10.ex10.9`.
7. Check the contracts of the MFSET: the elements out of range, the
   forest of `count()` trees, the merge of two elements already together.
   Test: `contract_mfset.mfset`.
8. Keep the forest in one vector of integers, with the sizes (or the
   ranks) at the roots and the halving of the paths. Test:
   `engineered_graph.union-find`.
9. Realize the algorithm of Rem with splicing, and check it against
   `MFSet` on random sequences of merges and questions. Test:
   `ordered.rem-union-find`.
10. Realize a union-find whose merges can be undone back to a checkpoint
    in $O( 1 )$ time each, and use it to answer offline the questions of
    connectivity of a graph whose edges come and go. Tests:
    `engineered_dynamic.rollback-union-find`,
    `engineered_dynamic.offline-connectivity`.
