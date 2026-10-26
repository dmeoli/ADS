/*--------------------------------------------------------------------------*/
/*------------------------- File JoinAlgorithms.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * THEORY_PLACEHOLDER
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_JoinAlgorithms
 #define __ADS_JoinAlgorithms
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/parallel/Executor.h"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*--------------------------- CONCEPT JoinAdaptor --------------------------*/
/*--------------------------------------------------------------------------*/

/// the sum of the sizes of two trees below which the algorithms run on the
/// calling thread alone
inline constexpr std::size_t kJoinGrain = 2048;

/// the nodes of a binary search tree as the algorithms based on join see
/// them: nil(), the empty tree; left(), right(), key(), value() and
/// size(), the reading of a node (size() of nil is 0); expose( t , l , r ),
/// which takes off the children of t into l and r, so that t is a node
/// alone; connect( m , l , r ), which hangs l and r under the node alone m
/// and updates what m keeps of its subtree (its size, its height, its
/// augmented value, the parents of l and r); dispose( t ), which frees
/// the subtree t (nothing for nil); rank( t ), the rank of a tree, and
/// childRank( t , r , c ), that of the child c of t of rank r, in
/// \f$ O( 1 ) \f$; join( l , rl , m , r , rr ), the join of the balancing
/// scheme, which returns the root and its rank
template< class A >
concept JoinAdaptor = requires( typename A::Node * t ,
                                typename A::Node * & l ,
                                const typename A::Key & k ,
                                typename A::Rank r ) {
 { A::nil() } -> std::same_as< typename A::Node * >;
 { A::left( t ) } -> std::same_as< typename A::Node * >;
 { A::right( t ) } -> std::same_as< typename A::Node * >;
 { A::key( t ) } -> std::same_as< const typename A::Key & >;
 { A::value( t ) } -> std::same_as< typename A::Value & >;
 { A::size( t ) } -> std::convertible_to< std::size_t >;
 A::expose( t , l , l );
 { A::connect( t , t , t ) } -> std::same_as< typename A::Node * >;
 A::dispose( t );
 { A::rank( t ) } -> std::same_as< typename A::Rank >;
 { A::childRank( t , r , t ) } -> std::same_as< typename A::Rank >;
 { A::join( t , r , t , t , r ) }
  -> std::same_as< std::pair< typename A::Node * , typename A::Rank > >;
 { k < k } -> std::convertible_to< bool >;
 };

/*--------------------------------------------------------------------------*/
/*----------------------- THE JOINS OF THE SCHEMES -------------------------*/
/*--------------------------------------------------------------------------*/

/// the join of the AVL trees; A gives also height( t ), -1 for nil, which
/// connect() keeps
template< class A >
struct AVLJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// JOIN_AVL_DOC
 static Node * join( Node * l , Node * m , Node * r );

 /// the join when l is higher than r by more than 1
 static Node * joinRight( Node * l , Node * m , Node * r );

 /// the join when r is higher than l by more than 1
 static Node * joinLeft( Node * l , Node * m , Node * r );
 };

/// the join of the Red-Black trees, whose ranks are the black heights; A
/// gives also red( t ), false for nil, and paint( t , red )
template< class A >
struct RedBlackJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// JOIN_RB_DOC
 static std::pair< Node * , int > join( Node * l , int bl , Node * m ,
                                        Node * r , int br );

 /// the join when the black height bl of l is larger than br
 static Node * joinRight( Node * l , int bl , Node * m , Node * r ,
                          int br );

 /// the join when the black height br of r is larger than bl
 static Node * joinLeft( Node * l , int bl , Node * m , Node * r , int br );
 };

/// the join of the trees balanced by weight, with \f$ \alpha = 0.29 \f$
template< class A >
struct WeightBalancedJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 static constexpr std::size_t kAlphaNum = 29;    ///< \f$ \alpha \f$, up
 static constexpr std::size_t kAlphaDen = 100;   ///< and down

 /// JOIN_WB_DOC
 static Node * join( Node * l , Node * m , Node * r );

 /// the join when l is heavier than r beyond the balance
 static Node * joinRight( Node * l , Node * m , Node * r );

 /// the join when r is heavier than l beyond the balance
 static Node * joinLeft( Node * l , Node * m , Node * r );

 /// whether the weights a and b may be the two sides of a node:
 /// \f$ \alpha ( a + b ) \le \min( a , b ) \f$
 static bool like( std::size_t a , std::size_t b );

 /// the weight of t, its size plus 1
 static std::size_t weight( const Node * t );
 };

/// the join of the treaps; A gives also priority( t ), of a node
template< class A >
struct TreapJoin {
 using Node = typename A::Node;   ///< the type of the nodes

 /// JOIN_TREAP_DOC
 static Node * join( Node * l , Node * m , Node * r );
 };

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS JoinAlgorithms ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the algorithms on the trees whose only balancing is the join of A; each
/// of them consumes the trees it is given and returns the trees it makes
template< JoinAdaptor A >
struct JoinAlgorithms {

 using Node = typename A::Node;    ///< the type of the nodes
 using Key = typename A::Key;      ///< the type of the keys
 using Value = typename A::Value;  ///< the type of the values
 using Rank = typename A::Rank;    ///< the type of the ranks

 /// a tree: its root and its rank
 struct Tree {
  Node * t = nullptr;   ///< the root, nil if empty
  Rank r{};             ///< the rank
  };

 /// a tree taken apart at its root
 struct Exposed {
  Tree l;               ///< the left subtree
  Node * m = nullptr;   ///< the root, alone
  Tree r;               ///< the right subtree
  };

 /// the empty tree
 [[nodiscard]] static Tree empty( void );

 /// the tree of root t, its rank computed
 [[nodiscard]] static Tree whole( Node * t );

 /// the subtrees and the root of x
 /// \pre x is not empty
 [[nodiscard]] static Exposed expose( Tree x );

 /// the join of the scheme
 [[nodiscard]] static Tree join( Tree l , Node * m , Tree r );

 /// SPLIT_DOC
 static Node * split( Tree x , const Key & k , Tree & l , Tree & r );

 /// SPLITLAST_DOC
 static Node * splitLast( Tree x , Tree & rest );

 /// JOIN2_DOC
 [[nodiscard]] static Tree join2( Tree l , Tree r );

 /// INSERT_DOC
 [[nodiscard]] static Tree insert( Tree x , Node * n , bool & fresh );

 /// REMOVE_DOC
 [[nodiscard]] static Tree remove( Tree x , const Key & k , bool & gone );

 /// UNION_DOC
 template< Executor E , class F >
 [[nodiscard]] static Tree unite( E & ex , Tree a , Tree b , const F & f );

 /// INTERSECTION_DOC
 template< Executor E , class F >
 [[nodiscard]] static Tree intersect( E & ex , Tree a , Tree b ,
                                      const F & f );

 /// DIFFERENCE_DOC
 template< Executor E >
 [[nodiscard]] static Tree subtract( E & ex , Tree a , Tree b );

 /// BUILD_DOC
 template< Executor E >
 [[nodiscard]] static Tree build( E & ex , Node * const * N ,
                                  std::size_t n );

 /// MULTIINSERT_DOC
 template< Executor E , class F >
 [[nodiscard]] static Tree multiInsert( E & ex , Tree x , Node * const * N ,
                                        std::size_t n , const F & f );

 /// MULTIREMOVE_DOC
 template< Executor E >
 [[nodiscard]] static Tree multiRemove( E & ex , Tree x , const Key * K ,
                                        std::size_t n , std::size_t & gone );

 /// FILTER_DOC
 template< Executor E , class P >
 [[nodiscard]] static Tree filter( E & ex , Tree x , const P & p );

 /// MAPREDUCE_DOC
 template< Executor E , class M , class R , class U >
 [[nodiscard]] static U mapReduce( E & ex , Node * t , const M & map ,
                                   const R & reduce , const U & id );

 /// EXTRACT_DOC
 [[nodiscard]] static Tree extract( Tree & x , const Key & lo ,
                                    const Key & hi );

 /// the smallest key of t
 /// \pre t is not nil
 [[nodiscard]] static const Key & firstKey( Node * t );

 /// the largest key of t
 /// \pre t is not nil
 [[nodiscard]] static const Key & lastKey( Node * t );

 private:

 /// g() and h(), in parallel on ex
 template< Executor E , class G , class H >
 static void fork( E & ex , G && g , H && h );

 /// g() and h(), one after the other
 template< class G , class H >
 static void fork( Sequential & ex , G && g , H && h );

 /// whether n items are too few for more than the calling thread
 template< Executor E >
 [[nodiscard]] static bool small( E & ex , std::size_t n );

 };  // end( struct JoinAlgorithms )

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS Joinable ------------------------------*/
/*--------------------------------------------------------------------------*/

/// the value of the second tree, or of the batch, on a key in both
struct TakeSecond {
 template< class V >
 V operator()( const V & , const V & b ) const { return( b ); }
 };

/// the value of the first tree on a key in both
struct TakeFirst {
 template< class V >
 V operator()( const V & a , const V & ) const { return( a ); }
 };

/// the operations based on join of the tree T of keys Key and values
/// Value, whose nodes A adapts [see JoinAdaptor]: a base of T (which makes
/// A its friend); A is read only in the bodies, as T is incomplete where
/// its bases are named, and gives take( t ), the root of t
/// taken out of it, which is left empty; put( t , r ), r the new root of
/// the empty t, which updates its size; peek( t ), its root; fresh( t ),
/// an empty tree like t; valid( t ), the invariant of t; salt( t ) and
/// make( k , v , s , i ), a node alone of the pair ( k , v ), the i-th of
/// a batch of salt s, which may be called in parallel
template< class T , class A , class Key , class Value >
class Joinable {

/*--------------------------------------------------------------------------*/
 public:

/*--------------------------------------------------------------------------*/
/*------------------------- SPLITTING AND JOINING --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the keys before k, the value of k (if there), the keys after k; the
 /// tree is consumed, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::tuple< T , std::optional< Value > , T > split(
                                                        const Key & k ) &&;

 /// the tree of the keys of l, the pair ( k , v ) and the keys of r; l and
 /// r are consumed, in \f$ O( | \log n_l - \log n_r | + 1 ) \f$ time
 /// \pre the keys of l are smaller than k, those of r larger
 [[nodiscard]] static T join( T && l , const Key & k , const Value & v ,
                              T && r );

 /// the tree of the keys of l and of r, which are consumed, in
 /// \f$ O( \log n ) \f$ time
 /// \pre the keys of l are smaller than those of r
 [[nodiscard]] static T join2( T && l , T && r );

 /// the pairs of the keys in \f$ [ lo , hi ] \f$, taken out of the tree,
 /// in \f$ O( \log n ) \f$ time
 [[nodiscard]] T extractRange( const Key & lo , const Key & hi );

/*--------------------------------------------------------------------------*/
/*--------------------------- BULK OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the union with o, which is consumed; on a key of both, the value
 /// f( mine , its ), by default its; \f$ O( m \log( n / m + 1 ) ) \f$
 /// work for sizes \f$ m \le n \f$
 template< class F = TakeSecond >
 void unionWith( T && o , F f = F() ) {
  Sequential seq;
  unionWith( seq , std::move( o ) , f );
  }

 /// unionWith( o , f ) on the threads of ex
 template< Executor E , class F = TakeSecond >
 void unionWith( E & ex , T && o , F f = F() );

 /// the intersection with o, which is consumed; the value f( mine , its ),
 /// by default mine; the work of unionWith(), plus the freeing of the
 /// nodes that leave
 template< class F = TakeFirst >
 void intersectWith( T && o , F f = F() ) {
  Sequential seq;
  intersectWith( seq , std::move( o ) , f );
  }

 /// intersectWith( o , f ) on the threads of ex
 template< Executor E , class F = TakeFirst >
 void intersectWith( E & ex , T && o , F f = F() );

 /// the keys not in o, which is consumed; the work of unionWith(), plus
 /// the freeing of the nodes of o
 void differenceWith( T && o ) {
  Sequential seq;
  differenceWith( seq , std::move( o ) );
  }

 /// differenceWith( o ) on the threads of ex
 template< Executor E >
 void differenceWith( E & ex , T && o );

 /// the pairs of B inserted; on a key already there, the value f( mine ,
 /// new ), by default new; \f$ O( m \log( n / m + 1 ) ) \f$ work for m
 /// pairs
 /// \pre the keys of B increasing (an audit)
 template< class F = TakeSecond >
 void multiInsert( std::span< const std::pair< Key , Value > > B ,
                   F f = F() ) {
  Sequential seq;
  multiInsert( seq , B , f );
  }

 /// multiInsert( B , f ) on the threads of ex
 template< Executor E , class F = TakeSecond >
 void multiInsert( E & ex , std::span< const std::pair< Key , Value > > B ,
                   F f = F() );

 /// the keys of B removed, \f$ O( m \log( n / m + 1 ) ) \f$ work
 /// \pre the keys of B increasing (an audit)
 void multiRemove( std::span< const Key > B ) {
  Sequential seq;
  multiRemove( seq , B );
  }

 /// multiRemove( B ) on the threads of ex
 template< Executor E >
 void multiRemove( E & ex , std::span< const Key > B );

 /// only the pairs with p( k , v ) kept, \f$ O( n ) \f$ work
 template< class P >
 void filter( P p ) {
  Sequential seq;
  filter( seq , p );
  }

 /// filter( p ) on the threads of ex
 template< Executor E , class P >
 void filter( E & ex , P p );

 /// reduce( ... reduce( id , map( k_1 , v_1 ) ) ... , map( k_n , v_n ) )
 /// by an associative reduce of identity id, \f$ O( n ) \f$ work
 template< class M , class R , class U >
 [[nodiscard]] U mapReduce( M map , R reduce , U id ) const {
  Sequential seq;
  return( mapReduce( seq , map , reduce , std::move( id ) ) );
  }

 /// mapReduce( map , reduce , id ) on the threads of ex
 template< Executor E , class M , class R , class U >
 [[nodiscard]] U mapReduce( E & ex , M map , R reduce , U id ) const;

 };  // end( class Joinable )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/JoinAlgorithms.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* JoinAlgorithms.h included */

/*--------------------------------------------------------------------------*/
/*----------------------- End File JoinAlgorithms.h ------------------------*/
/*--------------------------------------------------------------------------*/
