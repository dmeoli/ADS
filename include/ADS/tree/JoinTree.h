/*--------------------------------------------------------------------------*/
/*---------------------------- File JoinTree.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * JOINTREE_PLACEHOLDER
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_JoinTree
 #define __ADS_JoinTree
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/dictionary/RandomHashing.h"
#include "ADS/tree/JoinAlgorithms.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------ SCHEMES AND AUGMENTATIONS -----------------------*/
/*--------------------------------------------------------------------------*/

struct AVLBalance {};        ///< the AVL trees [see AVLJoin]
struct RedBlackBalance {};   ///< the Red-Black trees [see RedBlackJoin]
struct WeightBalance {};     ///< the weight-balanced trees
struct TreapBalance {};      ///< the treaps [see TreapJoin]

/// an augmentation of a map from K to V: a monoid on G::Value, of identity
/// G::identity() and associative operation G::combine( a , b ), and the
/// value G::from( k , v ) of a pair; the augmented value of a subtree is
/// the combination of the values of its pairs by increasing keys
template< class G , class K , class V >
concept Augmentation = requires( const K & k , const V & v ,
                                 const typename G::Value & a ) {
 { G::identity() } -> std::convertible_to< typename G::Value >;
 { G::from( k , v ) } -> std::convertible_to< typename G::Value >;
 { G::combine( a , a ) } -> std::convertible_to< typename G::Value >;
 };

/// no augmentation: nothing is kept, nothing computed
struct NoAugmentation {
 struct Value {
  bool operator==( const Value & ) const = default;
  };
 static Value identity( void ) { return( Value() ); }
 template< class K , class V >
 static Value from( const K & , const V & ) { return( Value() ); }
 static Value combine( const Value & , const Value & ) { return( Value() ); }
 };

/// the sum of the values
template< class V >
struct SumAugmentation {
 using Value = V;
 static Value identity( void ) { return( V() ); }
 template< class K >
 static Value from( const K & , const V & v ) { return( v ); }
 static Value combine( const Value & a , const Value & b ) {
  return( a + b );
  }
 };

/// the largest of the values (from the smallest value of the type)
template< class V >
struct MaxAugmentation {
 using Value = V;
 static Value identity( void ) {
  return( std::numeric_limits< V >::lowest() );
  }
 template< class K >
 static Value from( const K & , const V & v ) { return( v ); }
 static Value combine( const Value & a , const Value & b ) {
  return( a < b ? b : a );
  }
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS JoinTree ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a DICTIONARY on a binary search tree balanced by the join of B alone,
/// augmented by G
template< std::totally_ordered K , class V , class B = AVLBalance ,
          class G = NoAugmentation >
 requires Augmentation< G , K , V >
class JoinTree;

/// the adaptor of the nodes of a JoinTree [see JoinAdaptor]
template< std::totally_ordered K , class V , class B , class G >
struct JoinTreeNodes;

template< std::totally_ordered K , class V , class B , class G >
 requires Augmentation< G , K , V >
class JoinTree
 : public Joinable< JoinTree< K , V , B , G > ,
                    JoinTreeNodes< K , V , B , G > , K , V > {

 friend struct JoinTreeNodes< K , V , B , G >;
 using Nodes = JoinTreeNodes< K , V , B , G >;

/*--------------------------------------------------------------------------*/
 public:

 using Key = K;     ///< the type of the keys
 using Value = V;   ///< the type of the values
 using AugValue = typename G::Value;   ///< the type of the augmentation

 /// a node: what the navigation below reads
 struct Node {
  Node * left = nullptr;        ///< the left child
  Node * right = nullptr;       ///< the right child
  std::size_t size = 1;         ///< the nodes of the subtree
  /// the height plus 1 (AVL), the black height times 2 plus 1 if red
  /// (Red-Black), the priority (treap)
  std::uint64_t meta = 0;
  [[no_unique_address]] AugValue aug{};   ///< the augmented value
  K key;                        ///< the key
  V value;                      ///< the value
  };

 /// an empty tree; the seed of the priorities, for a treap
 explicit JoinTree( std::uint64_t seed = 1 );

 /// a copy, node by node with the same shape, in \f$ O( n ) \f$ time
 JoinTree( const JoinTree & o );

 JoinTree( JoinTree && o ) noexcept;

 JoinTree & operator=( JoinTree o ) noexcept;

 ~JoinTree();

 void swap( JoinTree & o ) noexcept;

/*--------------------------------------------------------------------------*/
/*------------------------- DICTIONARY OPERATIONS --------------------------*/
/*--------------------------------------------------------------------------*/

 /// the value of k, or nothing, in \f$ O( \log n ) \f$ time
 [[nodiscard]] std::optional< V > lookup( const K & k ) const;

 /// associates v to k, in \f$ O( \log n ) \f$ time
 /// \post k has the value v
 void insert( const K & k , const V & v );

 /// removes k, if it is there, in \f$ O( \log n ) \f$ time
 /// \post k is not there
 void remove( const K & k );

 [[nodiscard]] bool contains( const K & k ) const;

 [[nodiscard]] std::size_t size( void ) const;

 [[nodiscard]] bool empty( void ) const;

 /// removes all the pairs
 void clear( void );

 /// the height of the tree, -1 if empty, in \f$ O( n ) \f$ time
 [[nodiscard]] int height( void ) const;

 /// f( k , v ) on the pairs by increasing keys
 template< class F >
 void forEach( F && f ) const;

/*--------------------------------------------------------------------------*/
/*------------------------- AUGMENTED QUERIES ------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the augmented value of the whole map, in \f$ O( 1 ) \f$ time
 [[nodiscard]] AugValue augValue( void ) const;

 /// the augmented value of the pairs with keys in \f$ [ lo , hi ] \f$, in
 /// \f$ O( \log n ) \f$ calls of combine()
 [[nodiscard]] AugValue augRange( const K & lo , const K & hi ) const;

 /// the augmented value of the pairs with keys \f$ \le k \f$
 [[nodiscard]] AugValue augPrefix( const K & k ) const;

 /// f( a ) on the augmented values a of the \f$ O( \log n ) \f$ pieces
 /// (whole subtrees and single pairs) whose union is the set of the keys
 /// in \f$ [ lo , hi ] \f$, by increasing keys
 template< class F >
 void augVisit( const K & lo , const K & hi , F && f ) const;

/*--------------------------------------------------------------------------*/
/*---------------------------- NAVIGATION ----------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the root, nullptr if empty; the nodes must not be changed
 [[nodiscard]] const Node * root( void ) const { return( f_root ); }

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the shape of the tree, for PathIterator
 struct Shape {
  static auto root( const auto & o ) { return( o.f_root ); }
  static auto left( const auto * n ) { return( n->left ); }
  static auto right( const auto * n ) { return( n->right ); }
  static auto read( const auto * n ) {
   return( std::pair< const K & , const V & >( n->key , n->value ) );
   }
  };

 using value_type = std::pair< const K & , const V & >;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = value_type;
 using const_reference = value_type;
 using const_iterator = PathIterator< JoinTree , Shape >;
 using iterator = const_iterator;

 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , true ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , false ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// the order, the sizes, the augmented values and the balance of the
 /// scheme, \f$ O( n ) \f$
 [[nodiscard]] bool valid( void ) const;

 Node * f_root = nullptr;   ///< the root
 SplitMix64 f_g;            ///< the priorities of a treap

 };  // end( class JoinTree )

/*--------------------------------------------------------------------------*/

/// the AVL tree whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using JoinAVLTree = JoinTree< K , V , AVLBalance , G >;

/// the Red-Black tree whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using JoinRedBlackTree = JoinTree< K , V , RedBlackBalance , G >;

/// the tree balanced by weight, in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using WeightBalancedTree = JoinTree< K , V , WeightBalance , G >;

/// the treap whose balance is in join alone
template< std::totally_ordered K , class V , class G = NoAugmentation >
using JoinTreap = JoinTree< K , V , TreapBalance , G >;

/// the map augmented by G, balanced by weight as in PAM
template< std::totally_ordered K , class V , class G >
using AugmentedMap = JoinTree< K , V , WeightBalance , G >;

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/tree/impl/JoinTree.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* JoinTree.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File JoinTree.h --------------------------*/
/*--------------------------------------------------------------------------*/
