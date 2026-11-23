# Appendix C. Working with other libraries {#app-C}

[Source: `include/ADS/interop/`, `include/ADS/parallel/TBBExecutor.h`,
`include/ADS/parallel/OpenMPExecutor.h`, `include/ADS/distributed/`,
`include/ADS/set/RoaringBitmap.h`, `test/interop_examples.cpp`,
`test/interop_examples_mpi.cpp`, `test/interop_graphs.cpp`,
`test/interop_columnar.cpp`, `python/`]

We are concerned in this appendix with a program that already uses another
library and wants to call ADS from it, or to run that library on the
structures of ADS. The adapters and their design are described in
[§1.8.1](01-introduction.md#sec-1-8-1), [§14.14](14-graph.md#sec-14-14),
[chapter 25](25-parallel.md#ch-25) and [chapter
26](26-distributed.md#ch-26); here we show them at work, one section per
library, by short examples that one can copy into a function of a program.
For each example we give the line of the program that changes (usually one
type), what the change costs, and the option of the build that compiles it
in. By the latter we mean the option of CMake, the component of
`find_package(ADS COMPONENTS ...)` that gives its target, and the variable
of the makefile.

An example whose first line is a comment `// with <library>` needs that
library, and it is compiled only where the library is found, whereas the
others need only the standard library. Each example is also a test,
`interop_examples.<name>` (or `interop_examples_mpi.<name>` for MPI, run on
one to four processes), whose source holds the code of the example as it is
printed here, with the checks of its result between its lines. The checker
of this manual (`declcheck.py`, run by `make check`) compiles each example
against the headers, and it also verifies that the code of the example is
still in the test. Hence a change of an interface that breaks an example
stops both the build of the manual and that of the tests.

## C.1 The standard library {#sec-C-1}

Since the containers of ADS have the interface of those of the standard
library ([§1.8](01-introduction.md#sec-1-8)), the adapters of
`ADS/interop/std/` only add what a container adaptor or an allocator of the
standard library requires of its argument; they need no option, as they
include no other library. In the first example a
`std::flat_map` of C++23 keeps its keys and its values in two
`TieredVector`s of [chapter 2](02-sequence.md#ch-2), by the alias
`TieredFlatMap` of `FlatMap.h`, and the line that changes is the type of
the map.

```cpp
// with C++23
#include "ADS/interop/std/FlatMap.h"

ADS::TieredFlatMap< std::string , int > m;  // was std::flat_map< ... >
m[ "pear" ] = 3;
m[ "apple" ] = 5;
m.erase( "pear" );
```

A key is still found by the binary search of a flat map, but each access to
a key goes through the index of its tier. A lookup may therefore
be somewhat slower than on a `std::vector` (we have not measured it),
whereas an insertion or an erasure moves $O( \sqrt{n} )$ keys instead of
$O( n )$. Of course, the alias exists only where the standard library has
`<flat_map>` (e.g., GCC 15), and the test is compiled as C++23 where the
compiler takes it.

The costs above rest on two facts: the members by iterator of the
sequences make one operation by rank each, and the single insertions of
`std::flat_map` are specified by C++23 ([flat.map.modifiers]) as a binary
search of the keys followed by one `insert( p , x )` on each container.

<span id="lem-C-1"></span>**Lemma C.1 (flat containers).** *Let $S$ be a
sequence of ADS with `insert( i , x )` and `erase( i )` by rank and with
random-access iterators. Then `insert( p , x )`, `emplace( p , ... )` and
`erase( p )` of `StdSequence< S >` make one call of `insert()` or `erase()`
of $S$ at the rank $i$ of $p$, and return an iterator on the rank $i$; the
range forms on $k$ items make $k$ such calls at consecutive ranks (at the
rank of `first`, for the erasure). In a `TieredFlatMap` of $n$ keys, an
insertion of a new key, or an erasure, takes therefore $O( \log n )$
comparisons and moves $O( 1 + \min\{ i , \sqrt{n} , n - i \} )$ keys and
as many values, plus $O( 1 )$ amortized for the blocks, where $i$ is the
rank of the key.*

*Proof.* Each member computes the rank as the difference of $p$ and
`cbegin()`, in $O( 1 )$ time since the iterators are random-access, and
calls the member of $S$ at that rank (or, in the range forms, at the ranks
$i , i + 1 , \ldots$ of the insertion and $k$ times at the rank $i$ of the
erasure); the iterator returned is `begin()` plus that rank. A
`TieredVector` has the same members by iterator in its class, written
in the same way, and its iterators are random-access in $O( 1 )$ time;
hence the binary search of the keys makes $O( \log n )$ comparisons, and
the one insertion (or erasure) by rank on each of the two `TieredVector`s
moves the items of (2.7) by [Lemma 2.14](02-sequence.md#lem-2-14).
$\square$

The lemma holds for the containers of the standard library that follow the
effects of the standard; an implementation that inserted by other means
(e.g., by appending and sorting again a batch of keys, as `insert()` of a
range may do) would pay the moves of its scheme instead.

Also, the algorithms of `std::ranges` take the dictionaries and the search
trees of ADS as they are, since their iterators model the concepts of
C++20, even those that build their item on the fly
([§1.8](01-introduction.md#sec-1-8)); the second example counts and
searches the pairs of a `BTreeMap` of [chapter 8](08-tree.md#ch-8).

```cpp
#include "ADS/tree/BTreeMap.h"

ADS::BTreeMap< int , int > b;
for( int k = 0 ; k < 100 ; ++k )
 b.insert( k , ( k * 37 ) % 101 );
const auto even = std::ranges::count_if(
  b , []( const auto & x ) { return( x.first % 2 == 0 ); } );
const auto top = std::ranges::max_element(
  b , {} , []( const auto & x ) { return( x.second ); } );
// even is 50, and ( *top ).first is 30, whose value is 100
```

Note that `*top` is a pair built by the iterator, not a reference into the
tree; this is why the example reads `( *top ).first` instead of
`top->first`, and also why a copy of the pairs (e.g., by
`std::ranges::to< std::vector >()` of C++23) holds values. In the third
example we put three adapters together: (i) the sorts of [chapter
12](12-sort.md#ch-12) on a `std::deque` and on a part of a vector, through
its `std::span`; (ii) the pool of [chapter 5](05-memory.md#ch-5) under a
`std::pmr::map`, by `PmrResource`; (iii) a `std::priority_queue` as the
frontier of the shortest paths of [§14.8](14-graph.md#sec-14-8), by
`PriorityFrontier`.

```cpp
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/std/Pmr.h"
#include "ADS/sort/Sort.h"
#include <deque>
#include <memory_resource>
#include <queue>

std::deque< int > d = { 5 , 3 , 9 , 1 , 7 };
ADS::sort( d );                                // 1 3 5 7 9
std::vector< int > v = { 9 , 8 , 7 , 6 , 5 , 4 };
ADS::sort( std::span< int >( v ).subspan( 2 , 3 ) );  // 9 8 5 6 7 4
ADS::PoolResource<> pool;
ADS::PmrResource<> mr( pool );                 // a std::pmr::memory_resource
std::pmr::map< int , int > m( &mr );           // its nodes in the pool
for( int k = 0 ; k < 1000 ; ++k )
 m[ k ] = k * k;
const ADS::StaticGraph< double > g( 4 , { { 0 , 1 , 2. } , { 1 , 2 , 1. } ,
                                         { 0 , 2 , 4. } , { 2 , 3 , 1. } } );
using P = std::pair< double , int >;
std::priority_queue< P , std::vector< P > , std::greater< P > > q;
ADS::PriorityFrontier S( q , g.n() );          // lazy: no decrease-key
const auto T = ADS::shortestPaths( g , 0 , S );  // T.d is { 0 , 2 , 3 , 4 }
```

The sort of the deque runs in place on its random-access iterators, while
`PmrResource` costs one virtual call per allocation, which is the price of
`std::pmr` (the pool is as in [chapter 5](05-memory.md#ch-5)). Since
`std::priority_queue` cannot lower a key, the frontier on it is *lazy*: a node
whose distance falls is pushed again, and its stale copies are skipped when
they reach the top. Hence the queue may hold up to $m$ pairs instead of $n$;
the heaps of ADS, and those of Boost.Heap in [§C.3](C-libraries.md#sec-C-3),
lower their keys in place.

<span id="lem-C-2"></span>**Lemma C.2 (the memory resource).** *Let $r$ be
a resource of ADS with `allocate( b , a )` and `deallocate( p , b , a )`
that outlives a `PmrResource< R >` on it and the memory that it gives. Then
each allocation and each deallocation of the adapter is one virtual call
followed by one call of $r$ with the same arguments, and two adapters
compare equal if and only if both are `PmrResource< R >` on the same object
$r$; hence a block allocated through an adapter may be freed through any
adapter equal to it, as `std::pmr` requires of equal resources.*

*Proof.* `do_allocate()` and `do_deallocate()` forward their arguments to
$r$, and `do_is_equal()` is true exactly when the other resource is a
`PmrResource< R >` (by `dynamic_cast`) holding the same pointer to $r$.
Two equal adapters therefore pass the block to the same object $r$, which
frees it as if it had been given back to the adapter that allocated it.
$\square$

The costs and the alignment of the blocks are thus those of $r$, e.g., of
`PoolResource` by [Lemma 5.9](05-memory.md#lem-5-9), and the only price
of the adapter is the virtual call.

## C.2 Boost.Graph {#sec-C-2}

We compile the adapters of Boost with `ADS_WITH_BOOST`. CMake turns it on
when it finds the headers of Boost 1.83 or later (`-DADS_WITH_BOOST=ON`
requires them, `OFF` leaves them out), the component `Boost` of
`find_package(ADS COMPONENTS Boost)` carries it, and the makefile defines it
by default (`BOOST=<prefix>` where Boost is not where the compiler looks);
no library is linked, since Boost is used by its headers. With `Graph.h`, a
`StaticGraph` of either layout of its arcs is a graph of Boost.Graph with no
copy, and a program of
Boost.Graph changes only the type of its graph (e.g., from
`compressed_sparse_row_graph` to `ADS::StaticGraph`), as in the first
example, which runs the algorithm of Dijkstra of Boost.Graph on such a
graph.

```cpp
// with Boost
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/boost/Graph.h"
#include <boost/graph/dijkstra_shortest_paths.hpp>

const ADS::StaticGraph< double > g( 4 , { { 0 , 1 , 2. } , { 1 , 2 , 1. } ,
                                         { 0 , 2 , 4. } , { 2 , 3 , 1. } } );
std::vector< double > d( num_vertices( g ) );
std::vector< int > p( num_vertices( g ) );
boost::dijkstra_shortest_paths(
  g , 0 , boost::weight_map( get( boost::edge_weight , g ) )
            .distance_map( d.data() )
            .predecessor_map( p.data() ) );  // d is { 0 , 2 , 3 , 4 }
```

A scan of the out-arcs is the scan of a row of the graph, and a weight is read
where it lies, in the array of the weights at the place of its arc. Thus the
algorithm reads the same memory as on `compressed_sparse_row_graph`, which also
keeps the properties of the edges apart from the heads, and the edge descriptor
(the pair of its tail and of its place in the rows) is also its `edge_index`.
`ArrayGraph` is adapted in the same way, whereas `ListGraph` and an
`ObservedGraph` on it are mutable graphs of Boost.Graph, whose weight is looked
up in $O( \deg u )$ time ([§1.8.1](01-introduction.md#sec-1-8-1)).

In the other direction, `FromBGL` of `FromBGL.h` is a view of a graph of
Boost.Graph as a graph of ADS, its weights read from a property map, by
which the second example runs the algorithm of Johnson of
[§14.8](14-graph.md#sec-14-8) on a `boost::adjacency_list`. Then
`dijkstraShortestPaths()` of `Dijkstra.h` runs the algorithm of Dijkstra of
Boost.Graph on a heap of ADS, which `boost::dijkstra_shortest_paths()` does
not let the caller choose.

```cpp
// with Boost
#include "ADS/graph/ShortestPaths.h"
#include "ADS/interop/boost/Dijkstra.h"
#include "ADS/interop/boost/FromBGL.h"
#include "ADS/priority/IndexedDaryHeap.h"
#include <boost/graph/adjacency_list.hpp>

using AL =
  boost::adjacency_list< boost::vecS , boost::vecS , boost::directedS ,
                         boost::no_property ,
                         boost::property< boost::edge_weight_t , double > >;
AL al( 4 );
add_edge( 0 , 1 , 2. , al );
add_edge( 1 , 2 , 1. , al );
add_edge( 0 , 2 , 4. , al );
add_edge( 2 , 3 , 1. , al );
const auto w = get( boost::edge_weight , al );
const ADS::bgl::FromBGL< AL , decltype( w ) > g( al , w );  // no copy
const auto T = ADS::johnson( g , 0 );  // T.d is { 0 , 2 , 3 , 4 }
std::vector< double > d( 4 );
std::vector< int > p( 4 );
ADS::IndexedDaryHeap< double , 4 > q( 4 );  // was the 4-ary heap of Boost
ADS::bgl::dijkstraShortestPaths( al , 0 , q , p.data() , d.data() );
```

This view keeps one mark per vertex, and it reads the out-edges and the
weights through the traits of Boost.Graph, i.e., at the cost of the
iterators of the graph of Boost. On an `adjacency_list` on `vecS` it is
also a dynamic graph; hence an `ObservedGraph` on it changes the graph of
Boost while it sends its events to the observers. In
`dijkstraShortestPaths()` the queue may be any queue of the frontier of
[§14.8](14-graph.md#sec-14-8), `std::priority_queue` and the heaps of
Boost.Heap included, and the line that changes is the declaration of the
queue.

<span id="lem-C-3"></span>**Lemma C.3 (the view of Boost.Graph).** *Let
the vertex index of a graph $g$ of Boost.Graph with $n$ vertices be a
bijection onto $[ 0 , n )$. Then `FromBGL` on $g$ is built in $O( n )$ time
and space, with no copy of the edges; `forEachAdj( u , f )` and
`forEachEdge( u , f )` call $f$ once per out-edge of the vertex of index
$u$, in the order of `out_edges()`, with $O( 1 )$ work besides the
iterators and the property maps of $g$. On a graph whose vertices are
their indices, the names of the nodes never change: `deleteNode( u )`
removes the edges of $u$ and leaves its vertex isolated and marked, and
`insertNode()` names the new vertex by its index, which is $n'$ for a graph
of $n'$ vertices on `vecS`.*

*Proof.* The constructor sizes the vector of the vertices and the marks to
$n$, and visits the vertices once to store each at its index; no other data
is copied. The visits run the iterators of `out_edges()` of the vertex
stored at $u$, and map each target to its index (and, for `forEachEdge()`,
read its weight) by one `get()`. `deleteNode()` calls `clear_vertex()` and
marks $u$, so that `remove_vertex()`, which on `vecS` would renumber the
vertices after $u$, is never called; `insertNode()` appends the vertex of
`add_vertex()`, which on `vecS` is the next index, and returns its index.
$\square$

The visit of `dijkstraShortestPaths()` is that of
`boost::breadth_first_visit()`, with the queue of ADS behind the buffer of
Boost.Graph and a visitor that relaxes the arcs; [Algorithm
C.1](C-libraries.md#alg-C-1) gives it in the form of the prototype of
[§14.8](14-graph.md#sec-14-8), with the colors of Boost.Graph.

<span id="alg-C-1"></span>**Algorithm C.1.** The algorithm of Dijkstra of
Boost.Graph on a queue $Q$ of ADS: white the vertices not reached, gray
those in $Q$.

--8<-- "generated/Dijkstra-dijkstraShortestPaths.pseudo.md"

## C.3 Boost.Heap and Boost.Unordered {#sec-C-3}

Like those of [§C.2](C-libraries.md#sec-C-2), these adapters need
`ADS_WITH_BOOST`. `Heap.h` gives the mutable
heaps of Boost.Heap the free `queue_decrease()` of
[§1.14](01-introduction.md#sec-1-14), by which the shortest paths and the
algorithm of Prim of ADS lower a key on the handle of Boost; the line that
changes is again the declaration of the queue. Conversely,
Boost.Unordered takes a hash function of ADS as its hasher, and `FastHash` of
[chapter 7](07-dictionary.md#ch-7) declares `is_avalanching`, the
convention by which `boost::unordered_flat_map` skips the mixing that it
would otherwise apply to the hash.

```cpp
// with Boost
#include "ADS/dictionary/FastHash.h"
#include "ADS/graph/ShortestPaths.h"
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/boost/Heap.h"
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

const ADS::StaticGraph< double > g( 4 , { { 0 , 1 , 2. } , { 1 , 2 , 1. } ,
                                         { 0 , 2 , 4. } , { 2 , 3 , 1. } } );
using P = std::pair< double , int >;
using H = boost::heap::fibonacci_heap<
  P , boost::heap::compare< std::greater< P > > >;
H h;
ADS::PriorityFrontier S( h , g.n() );          // native decrease-key
const auto T = ADS::shortestPaths( g , 0 , S );  // T.d is { 0 , 2 , 3 , 4 }
boost::unordered_flat_map< std::string , int , ADS::FastHash< std::string > >
  m;  // the hash of ADS, avalanching: no mixing by Boost
m[ "pear" ] = 3;
m[ "apple" ] = 5;
```

Since a heap of Boost keeps on top the largest item in the order of its
comparator, `queue_decrease()` calls `increase()`, which
with `std::greater` moves an item whose distance falls toward the top. A
heap of Boost.Heap that is not mutable (e.g., `d_ary_heap` without
`mutable_< true >`) is taken as well, but lazily, as `std::priority_queue`
in [§C.1](C-libraries.md#sec-C-1).

The other way round requires no change of the program: with
`ADS_WITH_BOOST`,
`PointerTable` keeps its items in a `boost::unordered_flat_set` and
`SmallVector` in a `boost::container::small_vector`, and the generators
xoshiro256 run on Boost.Random ([§7.6](07-dictionary.md#sec-7-6)), while
their interface is that of ADS. Defining `ADS_POINTERTABLE_BOOST` (or one
of the other two macros of [§7.6](07-dictionary.md#sec-7-6)) to 0 keeps
the code of ADS for one structure; `FlatHashTable` keeps its items in a
`boost::unordered_flat_map` only when `ADS_FLATHASH_BOOST` is defined to
1, since its own code is on a par with it.

## C.4 LEMON {#sec-C-4}

We compile the headers of `ADS/interop/lemon/` with `ADS_WITH_LEMON`,
which the target `ADS::LEMON` of CMake defines (the component `LEMON` of
`find_package(ADS COMPONENTS LEMON)`). The option is `AUTO`, i.e., it is on
where abLEMON, or another LEMON whose headers compile in C++20, is found
(`LEMON_ROOT` where it is not), and the makefile puts the flags of LEMON in
`$(ADSLEMONINC)` (`LEMON=<prefix>`). Unlike the backends of
[§C.2](C-libraries.md#sec-C-2), LEMON is never included by `ADS::ADS`, and
thus a program that does not link `ADS::LEMON` compiles none of it. A
program written on LEMON takes a heap of ADS by the named template
parameters of an algorithm, i.e., by one type. In the first example the
`Dijkstra` of LEMON runs on the `DaryHeap` of `Heaps.h`, and the shortest
paths of ADS then run on the same digraph through `GraphView` of
`Digraphs.h`.

```cpp
// with LEMON
#include "ADS/graph/ShortestPaths.h"
#include "ADS/interop/lemon/Digraphs.h"
#include "ADS/interop/lemon/Heaps.h"
#include <lemon/dijkstra.h>
#include <lemon/list_graph.h>

using GR = lemon::ListDigraph;
GR g;
GR::ArcMap< int > l( g );
const GR::Node s = g.addNode() , a = g.addNode() , t = g.addNode();
l.set( g.addArc( s , a ) , 2 );
l.set( g.addArc( a , t ) , 1 );
l.set( g.addArc( s , t ) , 4 );
using H = ADS::lemon_interop::DaryHeap< int , GR::NodeMap< int > >;
lemon::Dijkstra< GR , GR::ArcMap< int > >::SetStandardHeap< H >::Create d(
  g , l );
d.run( s );  // d.dist( t ) is 3
const ADS::lemon_interop::GraphView< GR , GR::ArcMap< int > > view( g , l );
const auto T = ADS::johnson( view , g.id( s ) );  // T.d[ g.id( t ) ] is 3
```

Each operation of `DaryHeap` makes one read of the cross-reference map of LEMON
more than the `IndexedDaryHeap` under it, i.e., one dependent load more than
the `BinHeap` of LEMON, which may or may not be visible (we have not measured
it). In exchange, with the policy `Dynamic<>` its observers receive every
change that the algorithm makes on its frontier
([§14.14.1](14-graph.md#sec-14-14-1)). On the other hand, `GraphView` makes no
copy, since its nodes are named by the ids of LEMON; where an algorithm runs
often on the same digraph, `toStatic()` copies it into a `StaticGraph` once, in
$\Theta( n + m )$ time.

Conversely, `StaticDigraphView` is a digraph of LEMON over a
`StaticGraph`, on which the second example runs the `Preflow` of LEMON.
Then a `LemonTap` of `Observers.h` passes the insertions and the erasures
of the arcs of a `ListDigraph` to an observer of ADS.

```cpp
// with LEMON
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/lemon/Digraphs.h"
#include "ADS/interop/lemon/Observers.h"
#include <lemon/list_graph.h>
#include <lemon/preflow.h>

using V = ADS::lemon_interop::StaticDigraphView< long long >;
const ADS::StaticGraph< long long > g(
  4 , { { 0 , 1 , 3 } , { 0 , 2 , 2 } , { 1 , 3 , 2 } , { 2 , 3 , 3 } } );
const V lg( g );  // no copy of the rows
const auto c = lg.weights();
lemon::Preflow< V , V::WeightMap > pf( lg , c , lg.nodeFromId( 0 ) ,
                                      lg.nodeFromId( 3 ) );
pf.run();  // pf.flowValue() is 4
using GR = lemon::ListDigraph;
using Tap = ADS::lemon_interop::LemonTap< GR , GR::Arc >;
struct Count
{
 int live = 0;  ///< the arcs in the digraph
 void on( const Tap::Insert & ) { ++live; }
 void on( const Tap::Erase & ) noexcept { --live; }
 };
GR h;
Tap tap( h );
Count k;
tap.attach( k );
const GR::Node u = h.addNode() , v = h.addNode();
const GR::Arc a = h.addArc( u , v );
h.addArc( v , u );
h.erase( a );  // k.live is 1
```

This view uses the rows of the `StaticGraph` and reads the capacities where
they lie, but it builds once the tails and the lists of the arcs into each
node, which `Preflow` needs and the rows lack, in $8 a + 4 n$ bytes for
$a$ arcs. As to the tap, it is an observer of the notifier of LEMON, and
it adds to
each insertion and erasure of an arc the calls of the observers attached to
it. Note that it does not see the changes that the notifier does not notify
(e.g., `changeTarget()`), which [§14.14.2](14-graph.md#sec-14-14-2) lists,
while the opposite direction, a `ListDigraph` kept in step with an
`ObservedGraph` of ADS, is `LemonMirror`. Finally, the warm starts that ADS
writes into private copies of six headers of LEMON are described in
[§14.14.4](14-graph.md#sec-14-14-4); they need the rewritten headers first
on the include path, as `ADS::LEMON` and `$(ADSLEMONINC)` arrange.

## C.5 GMP {#sec-C-5}

`BigInteger` and `Rational` of [chapter 17](17-numeric.md#ch-17) run on GMP
wherever it is found, by the macro `ADS_WITH_GMP`, `AUTO` with CMake (the
component `GMP`) and on by default with the makefile (`GMP=<prefix>`). With
it, the constructors from `mpz_class` and `mpq_class`, `fromGmp()` and
`toGmp()` pass a number to and from GMP without writing it as a string.
Thus a program on GMP can call an algorithm of ADS on its integers (e.g.,
one of the number theory of [chapter 17](17-numeric.md#ch-17)) and take the
result back; the line that changes is the declaration of the number.

```cpp
// with GMP
#include "ADS/numeric/BigInteger.h"
#include <gmpxx.h>

const mpz_class m( "-98765432109876543210987654321" );
const ADS::BigInteger x( m );                        // no string between
const mpz_class y = ( x * x ).toGmp< mpz_class >();  // the same, in GMP
```

A conversion copies the limbs of the number, in time linear in its length,
which is usually small against an operation on the same numbers, and the
interface of C (`mpz_t`, `mpq_t`) is taken as well, by `toGmp( z )` and
`fromGmp( z )`. Without GMP the constructors from its types are not
declared, and the numbers run on the algorithms of the book.

## C.6 BLAS {#sec-C-6}

`gemm()` of `DenseMatrix.h` ([chapter 17](17-numeric.md#ch-17)) is a call
of `cblas_dgemm()` or `cblas_sgemm()` for `double` and `float` wherever a
CBLAS is found (OpenBLAS, the reference CBLAS, or Accelerate on macOS), by
the macro `ADS_WITH_BLAS`, `AUTO` with CMake (the component `BLAS`) and on by
default with the makefile (`BLAS=<prefix>`). Since a `DenseMatrix` keeps
its entries by rows in one contiguous vector, whose leading dimension is
the number of its columns, a program can also call any routine of BLAS or
LAPACK on its `data()`, as the example does before it computes the same
product by `multiply()`.

```cpp
// with BLAS
#include "ADS/numeric/DenseMatrix.h"
#include <cblas.h>

ADS::DenseMatrix< double > A( 2 , 3 , 1. ) , B( 3 , 2 , 2. ) , C( 2 , 2 );
cblas_dgemm( CblasRowMajor , CblasNoTrans , CblasNoTrans , 2 , 2 , 3 , 1. ,
             A.data() , 3 , B.data() , 2 , 0. , C.data() , 2 );
const auto D = ADS::multiply( A , B );  // on cblas_dgemm() too
```

A CBLAS runs on the threads it is built or set for (e.g., those of
`OPENBLAS_NUM_THREADS`), and hence the parallel `gemm( ex , ... )` of [chapter
25](25-parallel.md#ch-25) makes one call from the calling thread, instead
of dividing the rows among the threads of the executor. We remark that the
result of the CBLAS may differ from that of the loops of ADS in the last
bits, since the two add the terms in different orders; the bound of the
error is the same (cf. [chapter 17](17-numeric.md#ch-17)).

## C.7 oneTBB {#sec-C-7}

Since the parallel algorithms of ADS take an executor as their first argument
([chapter 25](25-parallel.md#ch-25)), for a program that already runs on a
runtime the line that changes is the declaration of the executor (e.g.,
`ADS::ThreadPool ex( 4 )` becomes `ADS::TBBExecutor ex( arena )`), so that ADS
does not start a pool beside the threads of the program. `TBBExecutor` is
compiled with `ADS_WITH_TBB`, which CMake defines with `-DADS_WITH_TBB=ON` (off
by default, since `ADS::ADS` then links oneTBB) and the makefile with
`ADS_WITH_TBB=1`. In the example we sort in an arena of the program, and then
call a concurrent structure of [chapter 24](24-concurrent.md#ch-24) from a loop
of oneTBB, which needs no adapter.

```cpp
// with oneTBB
#include "ADS/concurrent/ConcurrentUnionFind.h"
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/TBBExecutor.h"
#include <tbb/parallel_for.h>
#include <tbb/task_arena.h>

std::vector< double > A( 1000000 );
std::mt19937 r( 1 );
for( double & x : A )
 x = double( r() % 1000003 );
tbb::task_arena arena( 4 );  // the arena of the program
ADS::TBBExecutor ex( arena );
ADS::sort( ex , A );  // on the threads of the arena
ADS::ConcurrentUnionFind uf( 1000 );
tbb::parallel_for( 0 , 999 , [ & ]( int i ) {
 if( i % 2 == 0 )
  ( void )uf.unite( i , i + 1 );
 } );  // the sets { 0 , 1 } , { 2 , 3 } , ...
```

Each call of the executor is a task of the arena; thus the constraints of
the arena (its threads, its NUMA node, its type of core) hold for ADS too,
and the calls nest as the tasks of oneTBB do. The executor adds
rather little to the cost of `tbb::parallel_for`, on which it runs. Also,
`TBBExecutor ex( ADS::TBBExecutor::current )` runs on the arena of each
caller, whichever it is.

## C.8 OpenMP {#sec-C-8}

`OpenMPExecutor` is compiled where the compiler runs OpenMP, i.e., with
`-fopenmp` (`-DADS_WITH_OPENMP=ON` with CMake, off by default, and
`ADS_WITH_OPENMP=1` with the makefile). Called inside a region `omp
parallel` of the program, it makes its calls the tasks of a `taskloop` of
the team that is there, where a nested region would usually get one
thread (this needs OpenMP 4.5). The line that changes is again the
declaration of the executor, inside the region.

```cpp
// with OpenMP
#include "ADS/parallel/OpenMPExecutor.h"
#include "ADS/parallel/ParallelSort.h"

std::vector< double > A( 1000000 );
std::mt19937 r( 1 );
for( double & x : A )
 x = double( r() % 1000003 );
#pragma omp parallel num_threads( 4 )
#pragma omp single
{
 ADS::OpenMPExecutor ex;  // the team of the region
 ADS::sort( ex , A );     // by the tasks of the team
 }
```

Outside a region, the executor opens a region at each call, of
`omp_get_max_threads()` threads unless it is built with a number, and the
concurrent structures of ADS are called from a `taskloop` or a `parallel
for` of the program as from the loop of oneTBB in
[§C.7](C-libraries.md#sec-C-7). Note also that an exception thrown by a
call is caught within the region and thrown again by `bulk()` after it,
since OpenMP does not let it leave a task.

## C.9 std::execution {#sec-C-9}

`StdParExecutor< Policy >` of `interop/parallel/StdParExecutor.h` runs the
calls of the executor by `std::for_each` of the policy on their indices
(`std::execution::par` by default). They run, therefore, on the threads of
the parallel algorithms of the standard library, which are those of oneTBB
with libstdc++. We compile it with `ADS_WITH_STDPAR`, defined by the target
`ADS::StdPar` of CMake (the component `StdPar`, the option being `AUTO`)
and by the makefile in `$(ADSINTEROPINC)`.

```cpp
// with std::execution
#include "ADS/interop/parallel/StdParExecutor.h"
#include "ADS/parallel/ParallelSort.h"

std::vector< double > A( 1000000 );
std::mt19937 r( 1 );
for( double & x : A )
 x = double( r() % 1000003 );
ADS::StdParExecutor<> ex( 4 );  // std::execution::par
ADS::sort( ex , A );
```

Note that the number given to the executor is the concurrency that it
reports to the
algorithms of ADS, by which they cut their work into blocks; it does not
bound the threads of the standard library, which chooses them. Since
`std::sort( std::execution::par , ... )` gives no control on the threads
either, the executor is a way to run the algorithms of ADS on the same
threads as the parallel algorithms of the program.

## C.10 stdexec {#sec-C-10}

`SenderExecutor< Sch >` of `interop/parallel/SenderExecutor.h` runs the
calls of the executor on a scheduler of the senders of stdexec (the
`std::execution` of C++26), by `bulk` and `sync_wait`. It is compiled with
`ADS_WITH_STDEXEC`, defined by the target `ADS::stdexec` (the component
`stdexec`, with `STDEXEC_INCLUDE_DIR` the directory of its headers) and by
the makefile in `$(ADSINTEROPINC)` (`STDEXEC_ROOT=<dir>`). In the example
we run the sort and a reduction of [chapter 25](25-parallel.md#ch-25) on the
static pool of stdexec.

```cpp
// with stdexec
#include "ADS/interop/parallel/SenderExecutor.h"
#include "ADS/parallel/Executor.h"
#include "ADS/parallel/ParallelSort.h"
#include <exec/static_thread_pool.hpp>

std::vector< double > A( 1000000 );
std::mt19937 r( 1 );
for( double & x : A )
 x = double( r() % 1000003 );
exec::static_thread_pool pool( 4 );  // the pool of the program
ADS::SenderExecutor ex( pool.get_scheduler() , 4 );
ADS::sort( ex , A );
const double s = ADS::parallelReduce(
  ex , 0 , A.size() , 0.0 ,
  [ & ]( std::size_t lo , std::size_t hi ) {
   double t = 0;
   for( std::size_t i = lo ; i < hi ; ++i )
    t += A[ i ];
   return( t );
   } ,
  std::plus<>() );
```

A `sync_wait()` holds the thread that waits, and therefore a call of
`bulk()` made inside a call of this executor runs in order on its thread.
Hence the fork and join of ADS (the recursion of `parallelInvoke()`, the
lower levels of the sorts) loses its nested parallelism here, whereas the
loops, the reductions and the top level of the sorts keep theirs. Also, the
number given with the scheduler is its concurrency, which a scheduler of
the senders does not report.

## C.11 Taskflow {#sec-C-11}

`TaskflowExecutor` of `interop/parallel/TaskflowExecutor.h` runs the calls
on the workers of a `tf::Executor` of the program, and it is compiled with
`ADS_WITH_TASKFLOW`, defined by the target `ADS::Taskflow` (the component
`Taskflow`, with `Taskflow_INCLUDE_DIR` the directory of its headers) and
by the makefile in `$(ADSINTEROPINC)` (`Taskflow_ROOT=<dir>`).

```cpp
// with Taskflow
#include "ADS/interop/parallel/TaskflowExecutor.h"
#include "ADS/parallel/ParallelSort.h"

std::vector< double > A( 1000000 );
std::mt19937 r( 1 );
for( double & x : A )
 x = double( r() % 1000003 );
tf::Executor workers( 4 );  // the executor of the program
ADS::TaskflowExecutor ex( workers );
ADS::sort( ex , A );
```

Unlike the executor on the senders, this one nests, since a nested call
co-runs its graph on the worker that waits; its price is a graph of
Taskflow built at each call, which is rather small against a sort of a million
items but possibly not against a loop of a few cheap iterations.

## C.12 Kokkos {#sec-C-12}

`KokkosExecutor< Space >` of `interop/parallel/KokkosExecutor.h` runs the
calls on a space of the host of Kokkos, by default
`Kokkos::DefaultHostExecutionSpace`, through `Kokkos::parallel_for` and
`fence()`. We compile it with `ADS_WITH_KOKKOS`, defined by the target
`ADS::Kokkos` (the component `Kokkos`), which takes the flags of Kokkos from
its package of CMake (found by `CMAKE_PREFIX_PATH`), whereas the makefile
does not take Kokkos, whose flags depend on how it was built. A program
starts Kokkos as it does for its kernels, here by a `Kokkos::ScopeGuard`.

```cpp
// with Kokkos
#include "ADS/interop/parallel/KokkosExecutor.h"
#include "ADS/parallel/ParallelSort.h"

std::vector< double > A( 1000000 );
std::mt19937 r( 1 );
for( double & x : A )
 x = double( r() % 1000003 );
Kokkos::ScopeGuard guard;  // Kokkos started, as by the program
ADS::KokkosExecutor<> ex;  // on Kokkos::DefaultHostExecutionSpace
ADS::sort( ex , A );
```

As on the senders, a dispatch of Kokkos holds the thread that waits, and
the calls made inside a call, or inside a kernel of the program, run in
order on their thread; a kernel of the program is recognized on the backend
of OpenMP by its parallel region (`omp_get_level()`, since the
`in_parallel()` of the spaces is deprecated in Kokkos 4 and removed in
Kokkos 5), while on the other backends its nested calls run in order only
when the space has one thread. We run the test of this example in the
continuous integration, on a Kokkos built on its backend of OpenMP.

## C.13 MPI {#sec-C-13}

The sort over the processes of [chapter 26](26-distributed.md#ch-26), and
the executor and the mailbox of the processes, take the communicator of
the program, and they are compiled with `ADS_WITH_MPI`, which is off by default
(`AUTO` or `ON` with CMake, `ADS_WITH_MPI=auto` or `1` with the makefile),
since `ADS::ADS` then carries MPI to every program that links it. A
program starts MPI (by itself, or by `MPIEnvironment` at the funneled level
that the mailbox needs), and the line that changes is the communicator
given to ADS, e.g., one of the program instead of `MPI_COMM_WORLD`.

```cpp
// with MPI
#include "ADS/distributed/DistributedSort.h"
#include "ADS/distributed/MPIExecutor.h"
#include "ADS/parallel/ThreadPool.h"

MPI_Comm comm;  // a communicator of the program
MPI_Comm_dup( MPI_COMM_WORLD , &comm );
const ADS::MPIExecutor ex( comm );
std::vector< int > A( 1000 );
std::mt19937 r( static_cast< unsigned >( ex.rank() ) );
for( int & x : A )
 x = int( r() % 100000 );
ADS::ThreadPool pool( 2 );  // the threads of each process
ADS::distributed::sort( pool , comm , A );
```

A program of Boost.MPI needs no adapter for this, since a
`boost::mpi::communicator` converts to `MPI_Comm`. In turn, `BoostMPI.h`
is compiled with `ADS_WITH_BOOST_MPI` (the target `ADS::BoostMPI`, the
component `BoostMPI`, which links Boost.MPI and Boost.Serialization). It
adds `BoostMPICodec`, by which a message that the program already
serializes by Boost.Serialization goes through the mailbox of [chapter
26](26-distributed.md#ch-26), by one specialization of `MPICodec`.

```cpp
// with Boost.MPI
#include "ADS/distributed/BoostMPI.h"
#include "ADS/distributed/DistributedSort.h"
#include "ADS/parallel/ThreadPool.h"
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/environment.hpp>

boost::mpi::environment env( boost::mpi::threading::funneled );
boost::mpi::communicator world;
std::vector< int > A( 1000 );
std::mt19937 r( static_cast< unsigned >( world.rank() ) );
for( int & x : A )
 x = int( r() % 100000 );
ADS::ThreadPool pool( 2 );
ADS::distributed::sort( pool , world , A );  // world is an MPI_Comm
```

In both examples `A` is sorted at the end, and no item of the process $r$
exceeds one of the process $r + 1$. The threads of each process sort its
part, and the messages between the processes are those of the regular
sampling of [chapter 26](26-distributed.md#ch-26), whichever library the
program used to start MPI. We run the two examples on one to four
processes in their tests, by `mpiexec` with CMake and by `make test-mpi`
with the makefile.

## C.14 Eigen {#sec-C-14}

A program that keeps its matrices in the structures of [chapter
17](17-numeric.md#ch-17) calls the solvers and the decompositions of Eigen
on them through `interop/eigen/Eigen.h`. We compile it with
`ADS_WITH_EIGEN` (`AUTO`, hint `EIGEN3_INCLUDE_DIR`), which the target and
the component `Eigen` define. In this header `map( A )` of a `DenseMatrix`
is an `Eigen::Map` of a dense matrix by rows over the entries of $A$, with
no copy. Also, `map( A )` of a `BasicCsrMatrix< T , I , I >`, i.e., one
whose columns and offsets have the same signed type, is an `Eigen::Map` of
the row-major `Eigen::SparseMatrix` over its three arrays. However, the
default `CsrMatrix` has columns of 32 bits without sign and offsets
`std::size_t`, whereas Eigen takes one signed type for both (its
`StorageIndex`); hence `toEigen()` copies it, and `fromEigen()` copies a
sparse matrix of Eigen back into ADS. In the example we solve by the
conjugate gradient of Eigen a system whose matrix is a `BasicCsrMatrix<
double , int >`, read in place, and the line that changes is the type of
the matrix.

```cpp
// with Eigen
#include "ADS/interop/eigen/Eigen.h"
#include "ADS/numeric/CsrMatrix.h"
#include <Eigen/IterativeLinearSolvers>

const ADS::BasicCsrMatrix< double , int > A(
  3 , 3 , { { 0 , 0 , 4. } , { 0 , 1 , -1. } , { 1 , 0 , -1. } ,
            { 1 , 1 , 4. } , { 1 , 2 , -1. } , { 2 , 1 , -1. } ,
            { 2 , 2 , 4. } } );
const auto M = ADS::eigen::map( A );  // no copy of the rows
Eigen::ConjugateGradient< ADS::eigen::RowSparse< double > ,
                          Eigen::Lower | Eigen::Upper >
  cg( M );
const Eigen::VectorXd x = cg.solve( Eigen::VectorXd::Ones( 3 ) );
const auto y = A.multiply( { x( 0 ) , x( 1 ) , x( 2 ) } );  // y is 1 1 1
const auto E = ADS::eigen::toEigen( ADS::CsrMatrix( 2 , 2 ) );  // a copy
```

The solver binds the map by a `Ref` of Eigen (cf. the documentation of
Eigen on the sparse solvers), which, as far as we can see, copies none of
the arrays of `A`. A copy by `toEigen()` costs $\Theta( m + \mathit{nnz} )$
time and the memory of a second matrix. A map holds the pointers of the
three vectors, and thus it must not outlive its matrix, nor see it change
its size.

## C.15 METIS and KaHIP {#sec-C-15}

ADS has no partitioner of graphs. METIS ([Karypis and Kumar
1998](Z-bibliography.md#ref-karypis1998)) and KaHIP ([Sanders and Schulz
2013](Z-bibliography.md#ref-sanders2013)) take as input the compressed rows
of a graph, `xadj` the offsets and `adjncy` the heads, with both arcs of an
edge. These are the rows of a `StaticGraph` of [chapter
14](14-graph.md#ch-14), and hence `partition( g , k )` of
`interop/metis/Metis.h` and of `interop/kahip/KaHIP.h` hands `offsets()` and
`heads()` to the library as they are. This requires that the index type of
the library be `int` (as the 32-bit `idx_t` of the METIS of the
distributions, and the `kahip_idx` of a KaHIP built without `KAHIP_64BIT`)
and that the graph have the separate layout; the weights of `weightData()`
are read in place too, if they are of that type. Otherwise `CsrInput` of
`interop/Csr.h` copies only the arrays that differ (e.g., the weights of a
`StaticGraph< double >`, or the heads and the weights of an interleaved
graph, which alternate in one array), and `copied()` gives them. The options
are `ADS_WITH_METIS` and `ADS_WITH_KAHIP` (`AUTO`, hints
`METIS_INCLUDE_DIR`, `METIS_LIBRARY`, `KaHIP_INCLUDE_DIR` and
`KaHIP_LIBRARY`). A partition is a `Partition`, i.e., the part of each node
and the weight of the edges cut; its `order()` numbers the nodes part by
part, and `relabel()` applies it.

```cpp
// with METIS
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/metis/Metis.h"

std::vector< std::pair< int , int > > E;  // a ring of 8 nodes
for( int u = 0 ; u < 8 ; ++u )
 E.emplace_back( u , ( u + 1 ) % 8 );
const ADS::StaticGraph<> g( 8 , E , false );
const ADS::Partition P = ADS::metis::partition( g , 2 );  // rows in place
const ADS::StaticGraph<> h = g.relabel( P.order() );  // P.cut is 2
```

In the second example `kaffpa()` of KaHIP, in its mode `ECO`, bisects a
ring whose two light edges form the best cut. Since the weights are `int`
and the layout is the separate one, KaHIP reads the three arrays of the
graph in place.

```cpp
// with KaHIP
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/kahip/KaHIP.h"

std::vector< std::tuple< int , int , int > > E;  // a ring, two light edges
for( int u = 0 ; u < 8 ; ++u )
 E.emplace_back( u , ( u + 1 ) % 8 , u % 4 == 3 ? 1 : 10 );
const ADS::StaticGraph< int > g( 8 , E , false );
ADS::kahip::Options o;
o.mode = ECO;
const ADS::Partition P = ADS::kahip::partition( g , 2 , o );  // P.cut is 2
```

Both libraries take their arrays as pointers to non-constant data, but
they write none of them (KaHIP builds a graph from them, and METIS reads
them with the numbering of C, which `partition()` sets); hence, the adapters
pass them by a `const_cast`. As both libraries require, the graph must be
unoriented and without loops, and its weights positive integers (a weight
of a floating type is truncated by the copy). A relabelling by `order()`
may then give the parallel algorithms of [chapter
25](25-parallel.md#ch-25) rows in which each part is contiguous, and the
processes of [chapter 26](26-distributed.md#ch-26) one block of
consecutive nodes each, although we have not measured what this gains.

## C.16 SuiteSparse:GraphBLAS {#sec-C-16}

GraphBLAS ([Davis 2019](Z-bibliography.md#ref-davis2019)) writes the
algorithms of graphs as products over semirings, which ADS does not have,
and `interop/graphblas/GraphBLAS.h` lets them run on the graphs and on the
matrices of ADS (`ADS_WITH_GRAPHBLAS`, `AUTO`, version 10 or later, hints
`GraphBLAS_INCLUDE_DIR` and `GraphBLAS_LIBRARY`). Since its version 10, a
`GxB_Container` holds the compressed rows of a matrix as vectors, and
`GxB_Vector_load()` with `GxB_IS_READONLY` makes a vector of an array that
its owner keeps (GraphBLAS reads it, and never writes nor frees it).
Hence, `view( g )` of a `StaticGraph` of the separate layout (or, more
precisely, of any graph of the concept `CsrArrays`) is a `GrB_Matrix` over
`offsets()`, `heads()` and `weightData()`. If the graph is unweighted, the
matrix is iso-valued, i.e., all its entries are one value `true`. In the
same way `view( A )` of a `BasicCsrMatrix` is the matrix of its three
arrays, whose integers GraphBLAS reads as `GrB_UINT32` or `GrB_UINT64` by
their size; therefore, the default `CsrMatrix` (offsets of 64 bits, columns
of 32) is read in place too. In the example we count the triangles of a
graph by $\sum ( A \circ A^2 ) / 6$, the product being masked by the
graph.

```cpp
// with GraphBLAS
#include "ADS/graph/StaticGraph.h"
#include "ADS/interop/graphblas/GraphBLAS.h"

GrB_init( GrB_NONBLOCKING );  // once, by the program
const ADS::StaticGraph<> g(
  4 , { { 0 , 1 } , { 0 , 2 } , { 1 , 2 } , { 2 , 3 } } , false );
const ADS::graphblas::Matrix A = ADS::graphblas::view( g );  // no copy
GrB_Matrix C = nullptr;
GrB_Matrix_new( &C , GrB_INT64 , 4 , 4 );
GrB_mxm( C , A , nullptr , GrB_PLUS_TIMES_SEMIRING_INT64 , A , A ,
         GrB_DESC_S );  // C = A .* A^2
std::int64_t t = 0;
GrB_Matrix_reduce_INT64( &t , nullptr , GrB_PLUS_MONOID_INT64 , C ,
                         nullptr );
GrB_Matrix_free( &C );  // t / 6 is 1 triangle
```

A view is a `graphblas::Matrix`, which frees the handle at its end and
leaves the arrays to ADS. The graph must outlive its view, and a view may be
an input of the operations of GraphBLAS but not their output (a call that
wrote it would have to copy the arrays first). An interleaved graph has no
view, and `copy( g )` builds its matrix by `GrB_Matrix_build()` in
$\Theta( n + a )$ time instead; conversely, `toCsr()` copies a matrix of
GraphBLAS into a `BasicCsrMatrix` by its triplets. We remark that the
header of GraphBLAS includes `<complex.h>`, whose macro `I` would rename
the template parameters of that name, and that the adapter undefines it.

## C.17 NetworKit {#sec-C-17}

NetworKit ([Staudt, Sazonovs and Meyerhenke
2016](Z-bibliography.md#ref-staudt2016)) has many more algorithms of network
analysis than ADS. Some of them are dynamic, i.e., they keep a result (the
components, the distances from a source, a centrality) while the graph
changes, provided that the caller changes the graph and then passes each
change to them, by `update()` or by `updateBatch()` with one `GraphEvent`
per change. The header `interop/networkit/NetworKit.h`
(`ADS_WITH_NETWORKIT`, `AUTO`, version 11.2 or later, hints
`NetworKit_INCLUDE_DIR`, `NetworKit_LIBRARY` and `tlx_INCLUDE_DIR`) has two
parts. First, `toNetworKit( g )` copies a graph of ADS, since the rows of
`Graph::fromCSR()` are of 64-bit indices and doubles. Furthermore,
`NetworKitMirror` is an observer of the `ObservedGraph` of
[§14.13](14-graph.md#sec-14-13), which keeps a graph of NetworKit in step
with that of ADS and makes both steps for each change through the wrapper.
In the example the dynamic components of NetworKit follow the insertions
and the deletions of the edges of a `ListGraph`, the last two in one batch
of ADS.

```cpp
// with NetworKit
#include "ADS/graph/ListGraph.h"
#include "ADS/graph/Observable.h"
#include "ADS/interop/networkit/NetworKit.h"
#include <networkit/components/DynConnectedComponents.hpp>

using G = ADS::ListGraph<>;
using M = ADS::networkit::NetworKitMirror< G >;
G g( 4 , false );
g.insertEdge( 0 , 1 );
M m( g );  // a copy of g
NetworKit::DynConnectedComponents cc( m.graph() );
cc.run();  // 3 components
m.attach( cc );
ADS::ObservedGraph< G , ADS::Static< M > > og( g , m );
og.insertEdge( 2 , 3 );  // cc.numberOfComponents() is 2
{
 auto b = ADS::batch( og );
 og.insertEdge( 1 , 2 );
 og.deleteEdge( 0 , 1 );
 }  // one updateBatch(), 2 components
```

The events map one to one (`EdgeInserted` to `EDGE_ADDITION`, `EdgeDeleted`
to `EDGE_REMOVAL`, `WeightChanged` to `EDGE_WEIGHT_UPDATE`, and those of
the nodes to `NODE_ADDITION`, `NODE_RESTORATION` and `NODE_REMOVAL`). A
batch reaches each algorithm as one `updateBatch()`, with the net effect
per edge that ADS has already coalesced. We attach an algorithm with the
kinds of the events it takes (by default those of the edges, since most
dynamic algorithms of NetworKit reject the others), while the graph of
NetworKit receives them all. A change costs what NetworKit spends on it,
plus $O( 1 )$, and the copy made by the constructor costs
$\Theta( n + m )$ time.

## C.18 Apache Arrow {#sec-C-18}

Within a process, pyarrow, Polars, DuckDB, pandas and nanoarrow hand the
columns of Apache Arrow to one another by its C Data Interface, i.e., by two
structures of C, `ArrowSchema` (the type of a column) and `ArrowArray` (its
buffers), whose callback `release` frees what the producer keeps alive.
Since the specification freezes the two structures as an ABI,
`ADS/interop/arrow/Arrow.h` defines them, under the guard that the
specification prescribes, and it needs no option and no library. We export a
structure of ADS by `exportArrow()`, which fills them with a column whose
values are the memory of the structure, kept alive by a `std::shared_ptr`
until the consumer releases the array. Conversely, `ArrowColumn< T >` takes
a column of the consumer without copy and shows it as a
`std::span< const T >`. Such a span may be the input of a build of ADS
(e.g., of a `WaveletMatrix` or of an `EliasFano`, cf. [chapter
22](22-succinct.md#ch-22)).

```cpp
#include "ADS/interop/arrow/Arrow.h"

ADS::BitVector b( 1000 );
b.set( 7 );
ArrowArray a;
ArrowSchema s;
ADS::exportArrow( std::move( b ) , &a , &s , "flags" );  // no copy
a.release( &a );  // as the consumer does when done
s.release( &s );
std::vector< std::int64_t > v{ 3 , 1 , 4 };
ArrowArray c;
ArrowSchema t;
ADS::exportArrow( std::move( v ) , &c , &t );
ADS::ArrowColumn< std::int64_t > col( &c , t );  // taken back, no copy
t.release( &t );
```

A `BitVector` becomes a column of booleans with no copy, since Arrow numbers
the bit $i$ as the bit $i \bmod 8$ of the byte $\lfloor i / 8 \rfloor$, which
is the layout of its words on a little-endian processor. A `PackedArray` of
8, 16 or 32 bits becomes a column of unsigned integers of that width, and a
contiguous sequence of numbers (a `std::vector`, a `std::array`, a
`std::span`) a column of their type. We copy the values only where
the layouts differ: for a `PackedArray` of 2 or 4 bits (decoded into
bytes), for a sequence that is not contiguous (e.g., a `TieredVector`), and
for the words of a `BitVector` or of a `PackedArray` on a big-endian
processor. In the other direction, `toBitVector()`, `toPackedArray< B >()`
and `toVector< T >()` copy a column into a structure of ADS, which then
holds the words, and they refuse a column with nulls, with children or of
another type by `std::invalid_argument`. Where nanoarrow is found, the test
`interop_columnar.arrow-nanoarrow` validates with it the columns that ADS
exports, and imports into ADS those that nanoarrow builds.

<span id="lem-C-4"></span>**Lemma C.4 (the columns of Arrow).** *Let
`exportArrow()` be called on a structure $x$ of $n$ items. (i) If $x$ is a
`BitVector`, or a `PackedArray` of 1, 8, 16 or 32 bits, on a little-endian
processor, or a contiguous range of numbers, the buffer of the values of
the array is the memory of $x$ (of the object into which $x$ is moved, for
the form that takes it by value), and no value is copied; otherwise the
values are copied, in $O( n )$ time. (ii) The buffers stay valid until
the consumer calls `release` of the array, which frees them once and sets
`release` to null, and the same holds for the name and `release` of the
schema. (iii) An `ArrowColumn` built on an array leaves the source
released, without calling its callback, and calls the callback exactly
once, when it is destroyed or assigned.*

*Proof.* (i) The branches of `exportArrow()` take the words, or
`std::ranges::data()` of $x$, in exactly these cases, and otherwise fill the
copy of the private data, by one pass over the items. (ii) The private data of
the array holds $x$ by a `std::shared_ptr` and the copy, if any, and the
callback of the array deletes it and clears `release`, as that of the schema
does with its name; the consumer, as the specification prescribes, calls each
callback once. (iii) The constructor moves the structure into the object and
sets `release` of the source to null, the move constructor and the move
assignment do the same with the object moved from, and `reset()` calls the
callback only if `release` is not null, and then clears it. $\square$

The structure exported must not change until the array is released, which
is a precondition of `exportArrow()`, since the consumer reads its memory
in place.

## C.19 CRoaring {#sec-C-19}

`RoaringBitmap` ([chapter 10](10-set.md#ch-10)) writes and reads the
portable format of Roaring, by `writePortable()`, `readPortable()` and
`portableSizeInBytes()`. CRoaring, the implementations in Java, Go and Rust
and the systems built on them share this format, and thus a file written by
any of them is read by ADS, and the converse. Since ADS has no containers
of runs, it reads those of a file as arrays or bitmaps; the input is
checked field by field, and a truncated or corrupted one is refused with
`PersistError`.
On this format, `ADS/interop/croaring/CRoaring.h` passes a set to CRoaring
and back by `toCRoaring()` and `fromCRoaring()` (and by `toRoaring()` for a
`roaring::Roaring`), under the option `ADS_WITH_CROARING`, `AUTO` with
CMake (the component `CRoaring`).

```cpp
// with CRoaring
#include "ADS/interop/croaring/CRoaring.h"

const std::uint32_t x[] = { 1 , 5 , 70000 };
const ADS::RoaringBitmap s( x , 3 );
const std::vector< std::byte > file = s.writePortable();  // any Roaring
roaring_bitmap_t * r = ADS::toCRoaring( s );
roaring_bitmap_add_range( r , 100 , 200 );            // in CRoaring
const ADS::RoaringBitmap t = ADS::fromCRoaring( r );  // and back
roaring_bitmap_free( r );
```

A conversion costs a buffer of `portableSizeInBytes()` bytes and two linear
passes over it, since both sides go through the format instead of the
internals of the other library. We have not measured it against a direct
copy of the containers, which would depend on those internals. We remark
that, for the sample of the specification of the format, ADS writes the
bytes of the file `bitmapwithoutruns.bin` of CRoaring. Also, the test
`interop_columnar.roaring-croaring` reads the same sample as CRoaring
writes it with runs (the file `bitmapwithruns.bin`), and it passes random
sets in both directions.

<span id="lem-C-5"></span>**Lemma C.5 (the bitmaps of CRoaring).** *Let
$s$ be a `RoaringBitmap` of $b$ bytes in the portable format, and $r$ a
bitmap of CRoaring of $b'$ bytes in it. Then `toCRoaring( s )` is a bitmap
of CRoaring of the same set as $s$, and `fromCRoaring( r )` a
`RoaringBitmap` of the same set as $r$, in $O( b )$ and $O( b' )$ time
besides the work of CRoaring on the same bytes; in particular,
`fromCRoaring( toCRoaring( s ) )` equals $s$.*

*Proof.* `toCRoaring()` writes $s$ by `writePortable()` into a buffer of
$b$ bytes and gives it to `roaring_bitmap_portable_deserialize_safe()`;
`fromCRoaring()` sizes a buffer by `roaring_bitmap_portable_size_in_bytes()`,
fills it by `roaring_bitmap_portable_serialize()` and reads it by
`readPortable()`. Both sides write and read the same format (the
specification of Roaring, which [Lemire et al.
2018](Z-bibliography.md#ref-lemire2018) describes), and each pass of ADS
is linear in the bytes. $\square$

The lemma assumes that CRoaring writes and reads the format as specified,
which the tests check on the samples of the specification and on random
sets.

## C.20 sdsl-lite {#sec-C-20}

In its version 3, header-only and under the BSD license, sdsl-lite keeps
its vectors in the words of ADS. Indeed, the bit $i$ of a `bit_vector` and of a
`BitVector` is the bit $i \bmod 64$ of the word $\lfloor i / 64 \rfloor$, and
the field $i$ of an `int_vector< B >` and of a `PackedArray< B >` ([chapter
22](22-succinct.md#ch-22)) takes the bits $i B$ to $( i + 1 ) B - 1$, which
never cross a word, as $B$ divides 64. Hence `ADS/interop/sdsl/Sdsl.h`
converts them by a copy of the words, `toSdsl()` in one direction and
`toBitVector()` and `toPackedArray< B >()` in the other. It is compiled
under the option `ADS_WITH_SDSL`, `AUTO` with CMake (the component
`sdsl`). A structure of
either library may then be built on the data of the other, as in the
example, where a rank directory of each library is built on the same bits.

```cpp
// with sdsl-lite
#include "ADS/interop/sdsl/Sdsl.h"
#include "ADS/succinct/RankSelect.h"
#include <sdsl/rank_support_v5.hpp>

ADS::BitVector b( 1000 );
b.set( 3 );
b.set( 500 );
const sdsl::bit_vector v = ADS::toSdsl( b );       // a copy of the words
const sdsl::rank_support_v5< 1 > r( &v );          // a rank of sdsl
const ADS::RankSelect q( ADS::toBitVector( v ) );  // and one of ADS
```

<span id="lem-C-6"></span>**Lemma C.6 (the vectors of sdsl-lite).** *For a
`BitVector` or a `PackedArray< B >` of $n$ items, `toSdsl()` gives a
`bit_vector` or an `int_vector< B >` with the same $n$ bits or fields, by
a copy of $\lceil n B / 64 \rceil$ words ($B = 1$ for the bits), and
`toBitVector()` and `toPackedArray< B >()` of a vector of width $B$ do the
converse in the same time. A vector of another width $W$ is read field by
field in $O( n )$ time: each field equals the one of the input when it
fits in $B$ bits, and else it is clamped to $2^B - 1$ if the array
saturates, while without saturation the conversion throws
`std::invalid_argument`.*

*Proof.* By the layouts stated above, the bit or the field $i$ lies in the
same place of the same word in both libraries, and the functions copy the
first $\lceil n B / 64 \rceil$ words by `memcpy()`, or build the array on
them. For a width $W \ne B$, `toPackedArray< B >()` first scans the fields
for one above $2^B - 1$ when the array does not saturate, and then builds
the array by one call per field. $\square$

Neither library can take the words of the other without a copy, since each
one owns them: ADS aligns them to a line of the cache, and sdsl allocates
them by its manager of the memory. An `int_vector` of a width other
than $B$ is read field by field. Both libraries also write a vector of
bits in the same form to the archives of cereal (cf. [chapter
23](23-external.md#ch-23)), as its words in a binary archive and as
integers of 64 bits, one per bit, in a JSON one. The test
`interop_columnar.sdsl-cereal` thus loads in sdsl the `BitVector` that ADS
saves, and in ADS the `bit_vector` that sdsl saves, in the binary, portable
binary and JSON archives of cereal.

## C.21 Python {#sec-C-21}

We now turn to Python, where the package `ads-python`, in the directory
`python/`, gives the structures of ADS that NumPy, SciPy and NetworkX lack
through an extension module `ads` written on nanobind. It also registers
`nx-ads`, a backend of NetworkX ([Hagberg et al.
2008](Z-bibliography.md#ref-hagberg2008)) that runs the algorithms of ADS
behind its functions. A library of templates cannot be bound generically,
and thus the module instantiates a fixed set of them, chosen where ADS has
something that the libraries of Python have not. These are (i)
`StaticGraph`, with no weights and with weights of type `double` in both
layouts of the arcs ([§14.4](14-graph.md#sec-14-4)), with its sequential and
parallel algorithms, (ii) `CsrMatrix`, (iii) `BitVector`, `RankSelect`,
`PackedArray` of 1 to 32 bits and `EliasFano` ([chapter
22](22-succinct.md#ch-22)), (iv) `RoaringBitmap`, (v) the Bloom, blocked
Bloom, cuckoo, prefix, binary fuse, ribbon and Grafite filters ([chapter
10](10-set.md#ch-10)), (vi) the indexed heaps of [chapter
9](09-priority.md#ch-9), and (vii) `DynamicGraph`. As for the graphs, the
algorithms are the visits, the shortest paths of
[§14.8](14-graph.md#sec-14-8), the components, the strong components and the
topological sort, as well as the parallel visit, the components of Afforest,
the maximal independent set and the stepping algorithms of [chapter
25](25-parallel.md#ch-25). Also, the heaps decrease the key of an item,
which `heapq` cannot do, and `DynamicGraph` is a `ListGraph` behind the
`ObservedGraph` of [§14.13](14-graph.md#sec-14-13), whose typed events are
read as tuples. We have left out what NumPy, SciPy and NetworkX already do
as well (e.g., the sorts, the hash tables and the dense matrices).

A graph or a matrix is built from arrays of NumPy by one copy of each, and
its rows are sorted only where they are not. Its arrays are then exposed as
read-only views of NumPy over the memory of the structure, which each view
keeps alive; in the interleaved layout, the heads and the weights are two
views with a stride of 16 bytes over the array of the pairs. We remark that
a graph cannot adopt an array of NumPy without the copy, since its arrays
are vectors that hold their memory. When a structure is opened by
`restore_mapped()`, i.e., by the `restoreMapped()` of [chapter
23](23-external.md#ch-23), the views lie instead in the file; hence a
structure that is stored once is opened by any later run in $O( 1 )$ time,
with no copy and no parsing. Each static structure has `store()`,
`restore()`, `restore_mapped()`, `to_bytes()` and `from_bytes()`, and pickle
writes the same format. A `RoaringBitmap` is written in the portable format
of [§C.19](C-libraries.md#sec-C-19) instead, which pyroaring reads by
`BitMap.deserialize()` and writes by `BitMap.serialize()`. Each call that
takes time releases the GIL, and the parallel ones run on a `ThreadPool`
given to them or on a pool of the module. Furthermore, the preconditions of
ADS are checked at the level 1 of the contracts (cf.
[§1.7.1](01-introduction.md#sec-1-7-1)), and thus a wrong argument raises
`ContractViolation`, or a `ValueError` or an `IndexError` of the binding,
before it can break a structure. In the example, a graph is built from its
rows, stored, opened in place and queried; then a bitmap is written in the
portable format, and the changes of a batch are read as their net effect.

```python
import ads
import numpy as np

offsets = np.array([0, 2, 3, 4, 4], dtype=np.int32)
heads = np.array([1, 2, 3, 3], dtype=np.int32)
weights = np.array([1.0, 4.0, 2.0, 1.0])
g = ads.static_graph(offsets, heads, weights, layout="interleaved")
g.store(path)                                     # once
h = ads.InterleavedStaticGraph.restore_mapped(path)  # O(1), no copy
d, parent = ads.shortest_paths(h, 0)              # [0, 1, 4, 3]
r = ads.RoaringBitmap(np.arange(0, 1 << 20, 3, dtype=np.uint32))
data = r.serialize()                              # read by pyroaring
dg = ads.DynamicGraph(3)
with dg.batch():
    dg.add_edge(0, 1, 5.0)
    dg.set_weight(0, 1, 2.0)
dg.events()                         # [("edge_inserted", 0, 1, 2.0)]
```

On the side of NetworkX, the backend follows the dispatch of NetworkX 3,
under the entry point `networkx.backends`. A call with `backend="ads"`, or
with a graph already converted by `nx_ads.from_networkx()`, converts the
graph into a `StaticGraph` whose weights are those of the attribute that the
function names; it then runs the algorithm of ADS and returns the result of
NetworkX, with the names of the nodes. It implements the shortest paths from
a source (by the arcs, by Dijkstra, and by Bellman and Ford, also on
negative weights), the components, the strong and the weak ones, the
descendants and the ancestors, the test of acyclicity and the topological
sort. Its `can_run()` leaves to NetworkX the arguments that ADS does not
take, e.g., a weight given by a function or the shortest paths between all
the pairs. Its `should_run()` declines the graphs of fewer than $10^4$ nodes
and edges, below which we expect the conversion to cost more than the
algorithm saves, although we have not yet measured this threshold. Note
that, among several shortest paths of the same length, the one of ADS may
differ from that of NetworkX, since the rows of a `StaticGraph` are sorted
by head, whereas NetworkX visits the neighbors in the order of their
insertion.

```python
import networkx as nx

G = nx.grid_2d_graph(30, 30)
for k, (u, v) in enumerate(G.edges):
    G.edges[u, v]["weight"] = 1 + k % 7
d = nx.single_source_dijkstra_path_length(G, (0, 0), backend="ads")
nx.number_connected_components(G, backend="ads")  # 1
```

With CMake, the package is built under the option `ADS_WITH_PYTHON`, which
is `AUTO` (on where a Python with its headers and nanobind are found) and
off in the packages of the distributions. It is also built by `pip install
python/`, through scikit-build-core, into one wheel for the stable ABI of
Python 3.12 and later, with the stubs of the module written by the stubgen
of nanobind; the wheel leaves out the libraries that ADS may use beside its
headers, and it depends on NumPy only. The wheels of Linux (x86_64 and
aarch64), macOS (arm64 and x86_64) and Windows, and the sdist, are attached
to each release of ADS, and `pip install` of the wheel of the platform
installs the package with no compiler. Its tests of pytest, i.e., the ctest
`python.pytest`, compare each algorithm with NetworkX and SciPy, each bitmap
with pyroaring and each heap with `heapq`. They also run the tests of
NetworkX of the shortest paths, the components, the visits and the acyclic
graphs on the backend, by the machinery of NetworkX for its backends
(`NETWORKX_TEST_BACKEND=ads`). Only one of them is marked as an expected
failure, since it changes the graph of NetworkX while it iterates over the
topological order, which the backend computes on a copy. Both examples,
`python/examples/structures.py` and `networkx_backend.py`, are run by the
ctests `python.example.*`. Finally, we have not measured the times against
NetworkX and SciPy, which `bench/python/compare.py` takes on an idle
machine.

## C.22 Where the examples run {#sec-C-22}

[Table C.1](C-libraries.md#tab-C-1) gives the option that compiles the
examples of each library, and the names of their tests. These run on any
machine that has the libraries, and in the continuous integration, whose
job `interop` installs oneTBB, Boost, GMP, OpenBLAS, abLEMON, stdexec,
Taskflow and Kokkos, as well as cereal, Eigen, METIS, KaHIP, GraphBLAS,
NetworKit, CRoaring, sdsl-lite and nanoarrow, and runs the examples on all
of them by CMake and by the makefile. The jobs `test` install those of the
latter that the package managers of Linux, macOS and MSYS2 have, or that
are quick to build from their sources, the job of MPI runs the examples of
MPI and of Boost.MPI, and the job `python` builds the wheel of the package
of [§C.21](C-libraries.md#sec-C-21) and runs its tests on Linux, macOS and
Windows. Also, `make check` compiles the examples of a library only where
it finds the library, and it names those that it skips. The directories of
a library that is not where the compiler looks are given to it as flags in
the environment, `DECLCHECK_LEMON`, `DECLCHECK_STDEXEC`,
`DECLCHECK_TASKFLOW`, `DECLCHECK_KOKKOS`, `DECLCHECK_EIGEN`,
`DECLCHECK_KAHIP`, `DECLCHECK_GRAPHBLAS`, `DECLCHECK_NETWORKIT`,
`DECLCHECK_CROARING` and `DECLCHECK_SDSLLITE` (e.g.,
`DECLCHECK_TASKFLOW="-I<dir>"`).

<span id="tab-C-1"></span>

**Table C.1.** The libraries of the examples: the option of CMake and its
default, the component of `find_package(ADS)` (none where `ADS::ADS`
carries the library), and the tests. The makefile takes the same variable
(`auto`, `0` or `1`) for all the libraries but Kokkos, Boost.MPI and
Python, which only CMake builds, and `ADS_WITH_NANOARROW` for nanoarrow,
which CMake uses where it finds it; the adapters of Eigen, METIS, KaHIP,
GraphBLAS and NetworKit are also tested by `interop_graphs.*`, and those of
Arrow, CRoaring and sdsl-lite by `interop_columnar.*`.

| Library | Option | Default | Component | Tests |
|-------------------|--------------------|------|----------|-------------------|
| standard library | none | | none | `std-*` |
| Boost.Graph, Boost.Heap, Boost.Unordered | `ADS_WITH_BOOST` | `AUTO` | `Boost` | `bgl-*`, `boost-*` |
| LEMON | `ADS_WITH_LEMON` | `AUTO` | `LEMON` | `lemon-*` |
| GMP | `ADS_WITH_GMP` | `AUTO` | `GMP` | `gmp-numbers` |
| BLAS | `ADS_WITH_BLAS` | `AUTO` | `BLAS` | `blas-dense-matrix` |
| oneTBB | `ADS_WITH_TBB` | `OFF` | none | `tbb-arena-sort` |
| OpenMP | `ADS_WITH_OPENMP` | `OFF` | none | `openmp-region-sort` |
| std::execution | `ADS_WITH_STDPAR` | `AUTO` | `StdPar` | `stdpar-sort` |
| stdexec | `ADS_WITH_STDEXEC` | `AUTO` | `stdexec` | `stdexec-sort` |
| Taskflow | `ADS_WITH_TASKFLOW` | `AUTO` | `Taskflow` | `taskflow-sort` |
| Kokkos | `ADS_WITH_KOKKOS` | `AUTO` | `Kokkos` | `kokkos-sort` |
| MPI | `ADS_WITH_MPI` | `OFF` | none | `mpi-sort` |
| Boost.MPI | `ADS_WITH_BOOST_MPI` | `AUTO` | `BoostMPI` | `boost-mpi-sort` |
| Eigen | `ADS_WITH_EIGEN` | `AUTO` | `Eigen` | `eigen-conjugate-gradient` |
| METIS | `ADS_WITH_METIS` | `AUTO` | `METIS` | `metis-partition` |
| KaHIP | `ADS_WITH_KAHIP` | `AUTO` | `KaHIP` | `kahip-partition` |
| SuiteSparse:GraphBLAS | `ADS_WITH_GRAPHBLAS` | `AUTO` | `GraphBLAS` | `graphblas-triangles` |
| NetworKit | `ADS_WITH_NETWORKIT` | `AUTO` | `NetworKit` | `networkit-dynamic-components` |
| Arrow | none | | none | `arrow-columns` |
| CRoaring | `ADS_WITH_CROARING` | `AUTO` | `CRoaring` | `croaring-bitmaps` |
| sdsl-lite | `ADS_WITH_SDSL` | `AUTO` | `sdsl` | `sdsl-vectors` |
| Python, NetworkX | `ADS_WITH_PYTHON` | `AUTO` | none | `python.pytest`, `python.example.*` |

Finally, the test of an example whose library is not compiled in passes
with no check, and it prints so. A run of
`interop_examples` on a machine without (say) Kokkos thus does not fail,
but it does not test that example either, which then only the continuous
integration covers.
