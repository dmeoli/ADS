/*--------------------------------------------------------------------------*/
/*---------------------------- File BPlusTree.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The B+-tree of order m of the reality check of chapter 6, the search tree
 * of the file systems and of the databases, whose nodes are pages of the
 * secondary memory.
 *
 * An internal node has f children, \f$ \lceil m / 2 \rceil \leq f \leq m \f$
 * (the root at least 2), and f - 1 keys k[ 0 ] , ... , k[ f - 2 ]: the
 * child p[ i ] holds the keys in ( k[ i - 1 ] , k[ i ] ], p[ 0 ] those up to
 * k[ 0 ] and p[ f - 1 ] those larger than k[ f - 2 ]. The values are only in
 * the leaves, which hold up to m keys each, are all on the same level and
 * are linked in a list in the order of the keys, so that the keys in an
 * interval are read in O( log n + k ) time [see range()]. The height is
 * \f$ O( \log_{\lceil m / 2 \rceil} n ) \f$: with pages of 4 KB and m = 256
 * a few levels index billions of keys.
 *
 * The search goes down to the first child whose key is not smaller than the
 * one looked for. The insertion goes into the leaf of the key; a leaf with m
 * keys already splits its m + 1 keys between a new leaf on its left, with
 * the first \f$ \lceil ( m + 1 ) / 2 \rceil \f$, and itself, and the largest
 * key of the new leaf goes into the parent next to it; a parent with m + 1
 * children splits in the same way, the middle key going one level up, up to
 * the root, which may split and give a new root with two children. The book
 * describes the removal only as "a series of similar cases" and so it is
 * not here.
 *
 * \see A. Montresor, Algoritmi e Strutture Dati, 2025, section 6.3,
 *      https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf
 * \see R. Bayer, E. McCreight, Organization and maintenance of large
 *      ordered indexes, Acta Informatica 1(3), 173-189, 1972,
 *      doi:10.1007/BF00288683
 * \see D. Comer, The ubiquitous B-tree, ACM Computing Surveys 11(2),
 *      121-137, 1979, doi:10.1145/356770.356776
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_BPlusTree
 #define __ADS_BPlusTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS BPlusTree -------------------------------*/
/*--------------------------------------------------------------------------*/

/// a B+-tree of order m, without the removal
template< std::totally_ordered K , class V >
class BPlusTree {

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values

 /// an empty B+-tree of order m > 2, the root a leaf
 explicit BPlusTree( int m ) : f_m( m ) {
  if( m <= 2 )
   throw( std::invalid_argument( "ADS::BPlusTree: the order must be > 2" ) );
  f_root = new Node( true );
  }

 BPlusTree( const BPlusTree & ) = delete;
 BPlusTree & operator=( const BPlusTree & ) = delete;

 ~BPlusTree() { destroy( f_root ); }

/*--------------------------------------------------------------------------*/

 /// the value of the key x, or nothing
 [[nodiscard]] std::optional< V > lookup( const K & x ) const {
  const Node * f = leaf( x );
  const auto i = std::lower_bound( f->k.begin() , f->k.end() , x ) -
                 f->k.begin();
  if( ( i < long( f->k.size() ) ) && ( f->k[ i ] == x ) )
   return( f->v[ i ] );
  return( std::nullopt );
  }

 /// associates the value v to the key x
 void insert( const K & x , const V & v ) {
  Node * f = leaf( x );
  const auto i = std::lower_bound( f->k.begin() , f->k.end() , x ) -
                 f->k.begin();
  if( ( i < long( f->k.size() ) ) && ( f->k[ i ] == x ) ) {
   f->v[ i ] = v;                         // the key is there
   return;
   }
  f->k.insert( f->k.begin() + i , x );
  f->v.insert( f->v.begin() + i , v );
  ++f_size;
  if( int( f->k.size() ) <= f_m )         // the leaf was not full
   return;

  // the leaf f splits: the first a keys to a new leaf g on its left
  const int a = ( f_m + 2 ) / 2;          // ceil( ( m + 1 ) / 2 )
  Node * g = new Node( true );
  g->k.assign( f->k.begin() , f->k.begin() + a );
  g->v.assign( f->v.begin() , f->v.begin() + a );
  f->k.erase( f->k.begin() , f->k.begin() + a );
  f->v.erase( f->v.begin() , f->v.begin() + a );
  g->prev = f->prev;
  g->next = f;
  if( f->prev ) f->prev->next = g;
  f->prev = g;
  up( f , g , g->k.back() );
  }

 [[nodiscard]] std::size_t size( void ) const { return( f_size ); }

 /// the order m
 [[nodiscard]] int order( void ) const { return( f_m ); }

 /// the pairs with keys in [ A , B ], read along the list of the leaves
 [[nodiscard]] std::vector< std::pair< K , V > > range( const K & A ,
                                                       const K & B ) const {
  std::vector< std::pair< K , V > > out;
  for( const Node * f = leaf( A ) ; f ; f = f->next )
   for( std::size_t i = 0 ; i < f->k.size() ; ++i ) {
    if( B < f->k[ i ] )
     return( out );
    if( ! ( f->k[ i ] < A ) )
     out.emplace_back( f->k[ i ] , f->v[ i ] );
    }
  return( out );
  }

 /// the keys of the nodes, level by level from the root, each level from
 /// left to right: the shape of the tree, as in Figura 6.7
 [[nodiscard]] std::vector< std::vector< std::vector< K > > > levels( void )
  const {
  std::vector< std::vector< std::vector< K > > > out;
  std::vector< const Node * > L = { f_root };
  while( ! L.empty() ) {
   out.emplace_back();
   std::vector< const Node * > N;
   for( auto u : L ) {
    out.back().push_back( u->k );
    N.insert( N.end() , u->p.begin() , u->p.end() );
    }
   L = std::move( N );
   }
  return( out );
  }

 /// checks the shape: the keys in order and in their intervals, the number
 /// of children, the leaves on one level and linked in order; it throws
 /// std::logic_error if one fails
 void check( void ) const {
  int depth = -1;
  const Node * last = nullptr;
  check( f_root , nullptr , nullptr , 0 , depth , last );
  if( last && last->next )
   throw( std::logic_error( "ADS::BPlusTree::check: the list goes on" ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 struct Node {
  explicit Node( bool l ) : is_leaf( l ) {}
  bool is_leaf;                      ///< true if a leaf
  std::vector< K > k;                ///< the keys
  std::vector< Node * > p;           ///< the children, if internal
  std::vector< V > v;                ///< the values, if a leaf
  Node * parent = nullptr;           ///< the parent
  Node * prev = nullptr;             ///< the previous leaf
  Node * next = nullptr;             ///< the next leaf
  };

 /// the leaf where the key x is, or would be
 Node * leaf( const K & x ) const {
  Node * u = f_root;
  while( ! u->is_leaf ) {
   const auto i = std::lower_bound( u->k.begin() , u->k.end() , x ) -
                  u->k.begin();            // the first k[ i ] >= x
   u = u->p[ i ];
   }
  return( u );
  }

 /// inserts the new node g, with largest key s, just before its sibling f
 /// in their parent, splitting the parents that get m + 1 children
 void up( Node * f , Node * g , const K & s ) {
  Node * b = f->parent;
  if( ! b ) {                              // f was the root
   b = f_root = new Node( false );
   b->k = { s };
   b->p = { g , f };
   f->parent = g->parent = b;
   return;
   }
  const auto i = std::find( b->p.begin() , b->p.end() , f ) - b->p.begin();
  b->p.insert( b->p.begin() + i , g );
  b->k.insert( b->k.begin() + i , s );
  g->parent = b;
  if( int( b->p.size() ) <= f_m )
   return;

  // b has m + 1 children: the first a go to a new node c on its left, the
  // key between the two groups goes up
  const int a = ( f_m + 2 ) / 2;
  Node * c = new Node( false );
  c->p.assign( b->p.begin() , b->p.begin() + a );
  c->k.assign( b->k.begin() , b->k.begin() + ( a - 1 ) );
  const K mid = b->k[ a - 1 ];
  b->p.erase( b->p.begin() , b->p.begin() + a );
  b->k.erase( b->k.begin() , b->k.begin() + a );
  for( auto u : c->p )
   u->parent = c;
  up( b , c , mid );
  }

 void check( const Node * u , const K * lo , const K * hi , int d ,
             int & depth , const Node * & last ) const {
  auto fail = []( const char * w ) {
   throw( std::logic_error( std::string( "ADS::BPlusTree::check: " ) + w ) );
   };
  if( ! std::is_sorted( u->k.begin() , u->k.end() ) ||
      ( std::adjacent_find( u->k.begin() , u->k.end() ) != u->k.end() ) )
   fail( "the keys of a node are not increasing" );
  for( const K & x : u->k )
   if( ( lo && ! ( *lo < x ) ) || ( hi && ( *hi < x ) ) )
    fail( "a key is out of its interval" );
  if( u->is_leaf ) {
   if( ( u != f_root ) && u->k.empty() )
    fail( "an empty leaf" );
   if( ( int( u->k.size() ) > f_m ) || ( u->k.size() != u->v.size() ) )
    fail( "a leaf has more than m keys" );
   if( depth < 0 )
    depth = d;
   else if( depth != d )
    fail( "the leaves are not on one level" );
   if( u->prev != last )
    fail( "the list of the leaves is broken" );
   if( last && ( last->next != u ) )
    fail( "the list of the leaves is broken" );
   last = u;
   return;
   }
  const int f = int( u->p.size() );
  if( ( f > f_m ) || ( f < ( u == f_root ? 2 : ( f_m + 1 ) / 2 ) ) ||
      ( int( u->k.size() ) != f - 1 ) )
   fail( "an internal node has a wrong number of children" );
  for( int i = 0 ; i < f ; ++i ) {
   if( u->p[ i ]->parent != u )
    fail( "a wrong parent" );
   check( u->p[ i ] , i > 0 ? &u->k[ i - 1 ] : lo ,
          i < f - 1 ? &u->k[ i ] : hi , d + 1 , depth , last );
   }
  }

 static void destroy( Node * u ) {
  for( auto c : u->p )
   destroy( c );
  delete u;
  }

 int f_m;                     ///< the order
 Node * f_root;               ///< the root
 std::size_t f_size = 0;      ///< the number of keys

 };  // end( class BPlusTree )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/

#endif  /* BPlusTree.h included */

/*--------------------------------------------------------------------------*/
/*-------------------------- End File BPlusTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
