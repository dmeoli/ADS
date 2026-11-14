/*--------------------------------------------------------------------------*/
/*----------------------------- File Treque.h ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The treque (triple ended queue) of exercise 2.4 of Morin's book, beyond
 * the book: a LIST by rank whose `operator[]` takes \f$ O( 1 ) \f$ time and
 * whose insertions and removals at the rank \f$ i \f$ take
 * \f$ O( 1 + \min\{ i , n - i , | n / 2 - i | \} ) \f$ amortized time, fast
 * near either end and near the middle. The items are in two circular
 * deques [see `RingDeque.h`], \f$ L \f$ with the ranks
 * \f$ 0 , \ldots , |L| - 1 \f$ and \f$ R \f$ with the others, with
 * \f$ |L| = \lfloor n / 2 \rfloor \f$ or \f$ |L| = \lceil n / 2 \rceil \f$:
 * an operation at the rank \f$ i < |L| \f$ is one at the rank \f$ i \f$ of
 * \f$ L \f$, which moves the items of its shorter side, the
 * \f$ \min\{ i , |L| - i \} \f$ ones, and the same holds for \f$ R \f$;
 * then one item at most passes between the back of \f$ L \f$ and the front
 * of \f$ R \f$ to restore the balance, in \f$ O( 1 ) \f$ time.
 *
 * The contracts [see `Contract.h`]: a rank is below `size()` (at most
 * `size()` for an insertion), and the removals at the ends need a treque
 * that is not empty; the invariant is
 * \f$ \lfloor n / 2 \rfloor \le |L| \le \lceil n / 2 \rceil \f$. The
 * operations are defined in `Treque.tpp`.
 *
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, doi:10.15215/aupress/9781927356388.01, exercise 2.4
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_Treque
 #define __ADS_Treque
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"
#include "ADS/Iterators.h"
#include "ADS/queue/RingDeque.h"

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Treque ------------------------------*/
/*--------------------------------------------------------------------------*/

/// a LIST by rank fast at both ends and in the middle, on two circular
/// deques
template< class T >
class Treque {

/*--------------------------------------------------------------------------*/
 public:

 using value_type = T;                     ///< the type of the items
 using size_type = std::size_t;            ///< the type of the sizes
 using difference_type = std::ptrdiff_t;   ///< the type of the distances
 using reference = T &;                    ///< a reference to an item
 using const_reference = const T &;        ///< a constant reference
 /// the iterator on the items
 using iterator = RankIterator< Treque , T , false >;
 /// the iterator on the constant items
 using const_iterator = RankIterator< Treque , T , true >;

 /// the empty treque
 Treque( void ) = default;

 /// the items of `l`, in order
 Treque( std::initializer_list< T > l );

 /// true if there is no item
 [[nodiscard]] bool empty( void ) const { return( size() == 0 ); }

 /// the number \f$ n \f$ of items
 [[nodiscard]] std::size_t size( void ) const {
  return( f_L.size() + f_R.size() );
  }

 /// the item of rank \f$ i \f$, in \f$ L \f$ or in \f$ R \f$
 /// \pre \f$ i < n \f$
 [[nodiscard]] T & operator[]( std::size_t i ) {
  ADS_PRE( i < size() );
  return( i < f_L.size() ? f_L[ i ] : f_R[ i - f_L.size() ] );
  }

 /// the item of rank \f$ i \f$, constant
 /// \pre \f$ i < n \f$
 [[nodiscard]] const T & operator[]( std::size_t i ) const {
  ADS_PRE( i < size() );
  return( i < f_L.size() ? f_L[ i ] : f_R[ i - f_L.size() ] );
  }

 /// the first item
 /// \pre the treque is not empty
 [[nodiscard]] const T & front( void ) const { return( ( *this )[ 0 ] ); }

 /// the last item
 /// \pre the treque is not empty
 [[nodiscard]] const T & back( void ) const {
  return( ( *this )[ size() - 1 ] );
  }

 /// puts \f$ x \f$ at the rank \f$ i \f$, in
 /// \f$ O( 1 + \min\{ i , n - i , | n / 2 - i | \} ) \f$ amortized time
 ///
 /// In pseudocode (Morin, exercise 2.4), \f$ L \f$ and \f$ R \f$ the two
 /// deques:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{insert}( i , x ) \cr
 /// 2 & \quad \textbf{if}\ i < |L|\ \textbf{then}\ L.\mathrm{insert}( i , x
 ///     )\ \textbf{else}\ R.\mathrm{insert}( i - |L| , x ) \cr
 /// 3 & \quad \mathrm{balance}()
 /// \end{array}}
 /// \f]
 /// \pre \f$ i \le n \f$
 /// \post one item more, \f$ x \f$ at the rank \f$ i \f$
 void insert( std::size_t i , const T & x );

 /// removes the item of rank \f$ i \f$, in
 /// \f$ O( 1 + \min\{ i , n - i , | n / 2 - i | \} ) \f$ time
 ///
 /// In pseudocode (Morin, exercise 2.4):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{erase}( i ) \cr
 /// 2 & \quad \textbf{if}\ i < |L|\ \textbf{then}\ L.\mathrm{erase}( i )
 ///     \ \textbf{else}\ R.\mathrm{erase}( i - |L| ) \cr
 /// 3 & \quad \mathrm{balance}()
 /// \end{array}}
 /// \f]
 /// \pre \f$ i < n \f$
 /// \post one item less
 void erase( std::size_t i );

 /// \f$ x \f$ at the front
 void push_front( const T & x ) { insert( 0 , x ); }

 /// \f$ x \f$ at the back
 void push_back( const T & x ) { insert( size() , x ); }

 /// removes the first item
 /// \pre the treque is not empty
 void pop_front( void ) { erase( 0 ); }

 /// removes the last item
 /// \pre the treque is not empty
 void pop_back( void ) { erase( size() - 1 ); }

 /// removes all the items
 void clear( void ) { f_L.clear(); f_R.clear(); }

 /// the same items in the same order
 [[nodiscard]] bool operator==( const Treque & o ) const;

 /// the iterator on the first item
 [[nodiscard]] iterator begin( void ) { return( iterator( this , 0 ) ); }
 /// the iterator past the last item
 [[nodiscard]] iterator end( void ) { return( iterator( this , size() ) ); }
 /// the constant iterator on the first item
 [[nodiscard]] const_iterator begin( void ) const {
  return( const_iterator( this , 0 ) );
  }
 /// the constant iterator past the last item
 [[nodiscard]] const_iterator end( void ) const {
  return( const_iterator( this , size() ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// one item from the longer deque to the shorter one, if they differ by
 /// more than one item
 ///
 /// In pseudocode (Morin, exercise 2.4):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{balance}() \cr
 /// 2 & \quad \textbf{if}\ |L| > |R| + 1\ \textbf{then}\ R.\mathrm{pushFront}(
 ///     L.\mathrm{popBack}() ) \cr
 /// 3 & \quad \textbf{else if}\ |R| > |L| + 1\ \textbf{then}
 ///     \ L.\mathrm{pushBack}( R.\mathrm{popFront}() )
 /// \end{array}}
 /// \f]
 void balance( void );

 /// the two deques differ by one item at most
 [[nodiscard]] bool balanced( void ) const;

 RingDeque< T > f_L;   ///< the ranks \f$ 0 , \ldots , |L| - 1 \f$
 RingDeque< T > f_R;   ///< the ranks \f$ |L| , \ldots , n - 1 \f$

 };  // end( class Treque )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/Treque.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* Treque.h included */

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Treque.h ----------------------------*/
/*--------------------------------------------------------------------------*/
