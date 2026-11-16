# Appendix C. Working with other libraries {#app-C}

[Source: `include/ADS/interop/`, `include/ADS/parallel/TBBExecutor.h`,
`include/ADS/parallel/OpenMPExecutor.h`, `include/ADS/distributed/`,
`test/interop_examples.cpp`, `test/interop_examples_mpi.cpp`]

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
others need only the standard library. Every example is also a test,
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

## C.2 Boost.Graph {#sec-C-2}

We compile the adapters of Boost with `ADS_WITH_BOOST`. CMake turns it on
when it finds the headers of Boost 1.83 or later (`-DADS_WITH_BOOST=ON`
requires them, `OFF` leaves them out), the component `Boost` of
`find_package(ADS COMPONENTS Boost)` carries it, and the makefile defines it
by default (`BOOST=<prefix>` where Boost is not where the compiler looks);
no library is linked, since Boost is used by its headers. With `Graph.h`, a
`StaticGraph` is a graph of Boost.Graph with no copy, and a program of
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
where it lies, next to its head. Thus the algorithm reads about the same memory
as on `compressed_sparse_row_graph`, and the edge descriptor (the pair of its
tail and of its place in the rows) is also its `edge_index`. `ArrayGraph` is
adapted in the same way, whereas `ListGraph` and an `ObservedGraph` on it are
mutable graphs of Boost.Graph, whose weight is looked up in $O( \deg u )$ time
([§1.8.1](01-introduction.md#sec-1-8-1)).

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
`FlatHashTable` keeps its items in a `boost::unordered_flat_map`,
`PointerTable` in a `boost::unordered_flat_set` and `SmallVector` in a
`boost::container::small_vector`, and the generators xoshiro256 run on
Boost.Random ([§7.6](07-dictionary.md#sec-7-6)), while their interface is
that of ADS. Defining `ADS_FLATHASH_BOOST` (or one of the other three
macros of [§7.6](07-dictionary.md#sec-7-6)) to 0 keeps the code of ADS for
one structure.

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
order on their thread. We run the test of this example in the continuous
integration, on a Kokkos built on its backend of OpenMP. We remark that, at
the time of writing, the header does not compile with a Kokkos built on
`Serial` only, since `Kokkos::Serial::in_parallel()` takes no argument
there.

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

## C.14 Where the examples run {#sec-C-14}

[Table C.1](C-libraries.md#tab-C-1) gives the option that compiles the
examples of each library, and the names of their tests. These run on any
machine that has the libraries, and in the continuous integration, whose
job `interop` installs oneTBB, Boost, GMP, OpenBLAS, abLEMON, stdexec,
Taskflow and Kokkos; the job of MPI runs the examples of MPI and of
Boost.MPI. Also, `make check` compiles the examples of a library only where
it finds the library, and it names those that it skips. The directories of
a library that is not where the compiler looks are given to it as flags in
the environment, `DECLCHECK_LEMON`, `DECLCHECK_STDEXEC`,
`DECLCHECK_TASKFLOW` and `DECLCHECK_KOKKOS` (e.g.,
`DECLCHECK_TASKFLOW="-I<dir>"`).

<span id="tab-C-1"></span>

**Table C.1.** The libraries of the examples: the option of CMake and its
default, the component of `find_package(ADS)` (none where `ADS::ADS`
carries the library), and the tests. The makefile takes the same variable
(`auto`, `0` or `1`) for all the libraries but Kokkos and Boost.MPI, which
only CMake builds.

| Library | Option | Default | Component | Tests |
|------------------|---------------------|------|----------|-------------------|
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

Finally, the test of an example whose library is not compiled in passes
with no check, and it prints so. A run of
`interop_examples` on a machine without (say) Kokkos thus does not fail,
but it does not test that example either, which then only the continuous
integration covers.
