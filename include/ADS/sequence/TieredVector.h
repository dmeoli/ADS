/*--------------------------------------------------------------------------*/
/*--------------------------- File TieredVector.h --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The tiered vector of Goodrich and Kloss, beyond the book: a sequence
 * with access by index in O( 1 ) time, as a vector, and insertion and
 * removal at any index in \f$ O( \sqrt{n} ) \f$ time, where a vector
 * [see ArraySequence.h] moves O( n ) items.
 *
 * The items are in blocks of b items, b a power of 2, each a circular
 * buffer with the offset of its first item, all full but the last one; the
 * item of index i is the item \f$ i \bmod b \f$ of the block
 * \f$ \lfloor i / b \rfloor \f$, a shift, a mask and two reads. An
 * insertion at i moves the items of its block after i by one place, at
 * most b - 1 of them, and then the last item of each following block to
 * the front of the next one, O( 1 ) per block since a circular buffer
 * grows at the front by moving its offset: \f$ O( b + n / b ) \f$ moves.
 * A removal is the symmetric. b follows the size, the least power of 2
 * not below \f$ \sqrt{n} \f$ and 64 at least: when n leaves
 * \f$ [ b^2 / 4 , 4 b^2 ] \f$ the items are copied into the blocks of the
 * new b, in O( n ) time once every \f$ \Omega( n ) \f$ operations, so
 * that an insertion or a removal costs \f$ O( \sqrt{n} ) \f$ amortized
 * and the space is \f$ n + O( \sqrt{n} ) \f$ items.
 *
 * The contracts [see Contract.h]: an index is below size() (at most
 * size() for an insertion), and pop_back() needs a vector that is not
 * empty; the invariant, checked on vectors of at most 1024 items, is that
 * the blocks but the last are full and that they count size() items. The
 * operations are defined in TieredVector.tpp.
 *
 * \see M. T. Goodrich, J. G. Kloss II, Tiered vectors: efficient dynamic
 *      arrays for rank-based sequences, Algorithms and Data Structures
 *      (WADS 1999), Lecture Notes in Computer Science 1663, 205-216, 1999,
 *      doi:10.1007/3-540-48447-7_21
 * \see P. Bille, A. R. Christiansen, M. B. Ettienne, I. L. Gortz, Fast
 *      dynamic arrays, 25th Annual European Symposium on Algorithms (ESA
 *      2017), LIPIcs 87, 16:1-16:13, 2017,
 *      doi:10.4230/LIPIcs.ESA.2017.16
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

#ifndef __ADS_TieredVector
 #define __ADS_TieredVector
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <bit>
#include <cstddef>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS TieredVector ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a sequence with access in O( 1 ) and insertion and removal anywhere in
/// O( sqrt n ), in circular blocks of b items
template< class T >
class TieredVector {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;
 using size_type = std::size_t;
 using difference_type = std::ptrdiff_t;
 using reference = T &;
 using const_reference = const T &;

 TieredVector( void ) = default;

 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// the items of a block
 [[nodiscard]] std::size_t block( void ) const { return( f_b ); }

 /// \pre i < size()
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < f_n );
  Block & k = f_blocks[ i >> f_lg ];
  return( k.a[ ( k.off + i ) & ( f_b - 1 ) ] );
  }
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < f_n );
  const Block & k = f_blocks[ i >> f_lg ];
  return( k.a[ ( k.off + i ) & ( f_b - 1 ) ] );
  }

 /// x at the index i, the items from i one place on
 /// \pre i <= size()
 void insert( std::size_t i , const T & x );

 /// removes the item at the index i, the items after it one place back
 /// \pre i < size()
 void erase( std::size_t i );

 void push_back( const T & x ) { insert( f_n , x ); }

 /// \pre the vector is not empty
 void pop_back( void ) { erase( f_n - 1 ); }

 void clear( void );

/*------------------------- THE STANDARD INTERFACE -------------------------*/

 /// the iterator by index, random access, on items const if Const
 template< bool Const >
 class Iterator {
  using Owner = std::conditional_t< Const , const TieredVector ,
                                    TieredVector >;

  public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using reference = std::conditional_t< Const , const T & , T & >;
  using pointer = std::conditional_t< Const , const T * , T * >;

  Iterator( void ) = default;
  Iterator( Owner * v , std::size_t i ) : f_v( v ) , f_i( i ) {}

  template< bool K > requires ( Const && ! K )
  Iterator( const Iterator< K > & o ) : f_v( o.f_v ) , f_i( o.f_i ) {}

  reference operator*( void ) const { return( ( *f_v )[ f_i ] ); }
  pointer operator->( void ) const { return( &**this ); }
  reference operator[]( difference_type k ) const {
   return( ( *f_v )[ std::size_t( difference_type( f_i ) + k ) ] );
   }
  Iterator & operator++( void ) { ++f_i; return( *this ); }
  Iterator & operator--( void ) { --f_i; return( *this ); }
  Iterator operator++( int ) { auto t = *this; ++f_i; return( t ); }
  Iterator operator--( int ) { auto t = *this; --f_i; return( t ); }
  Iterator & operator+=( difference_type k ) {
   f_i = std::size_t( difference_type( f_i ) + k );
   return( *this );
   }
  Iterator & operator-=( difference_type k ) { return( *this += -k ); }
  friend Iterator operator+( Iterator a , difference_type k ) {
   return( a += k );
   }
  friend Iterator operator+( difference_type k , Iterator a ) {
   return( a += k );
   }
  friend Iterator operator-( Iterator a , difference_type k ) {
   return( a -= k );
   }
  friend difference_type operator-( const Iterator & a ,
                                    const Iterator & b ) {
   return( difference_type( a.f_i ) - difference_type( b.f_i ) );
   }
  bool operator==( const Iterator & o ) const { return( f_i == o.f_i ); }
  auto operator<=>( const Iterator & o ) const { return( f_i <=> o.f_i ); }

  private:
  Owner * f_v = nullptr;
  std::size_t f_i = 0;

  template< bool > friend class Iterator;
  };

 using iterator = Iterator< false >;
 using const_iterator = Iterator< true >;

 [[nodiscard]] iterator begin( void ) { return( iterator( this , 0 ) ); }
 [[nodiscard]] iterator end( void ) { return( iterator( this , f_n ) ); }
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , f_n ) );
  }
 [[nodiscard]] const_iterator cbegin( void ) const { return( begin() ); }
 [[nodiscard]] const_iterator cend( void ) const { return( end() ); }

/*--------------------------------------------------------------------------*/
 private:

 /// a block: b places, the first item at off
 struct Block {
  std::unique_ptr< T[] > a;
  std::size_t off = 0;
  };

 /// the blocks for the size n, b the least power of 2 not below sqrt( n )
 void rebuild( std::size_t n );

 /// the blocks but the last full, and size() items in them
 [[nodiscard]] bool full( void ) const;

 std::vector< Block > f_blocks;   ///< the blocks
 std::size_t f_n = 0;             ///< the number of items
 std::size_t f_b = 64;            ///< the places of a block
 unsigned f_lg = 6;               ///< log2( f_b )

 };  // end( class TieredVector )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/TieredVector.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* TieredVector.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File TieredVector.h -------------------------*/
/*--------------------------------------------------------------------------*/
