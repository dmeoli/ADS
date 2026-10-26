/*--------------------------------------------------------------------------*/
/*------------------------- File PositionIndex.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PositionIndex [see PositionIndex.h], with their
 * contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

template< class T , class H >
inline typename PositionIndex< T , H >::Position
PositionIndex< T , H >::find( const T * p ) const
{
 ADS_PRE( p != nullptr );
 const Position * const i = f_table.find( p );
 return( i ? *i : NONE );
 }

template< class T , class H >
inline bool PositionIndex< T , H >::contains( const T * p ) const
{
 return( find( p ) != NONE );
 }

template< class T , class H >
inline bool PositionIndex< T , H >::insert( const T * p , Position i )
{
 ADS_PRE( ( p != nullptr ) && ( i != NONE ) );
 return( f_table.insert( p , i ) );
 }

template< class T , class H >
inline bool PositionIndex< T , H >::erase( const T * p )
{
 ADS_PRE( p != nullptr );
 return( f_table.remove( p ) );
 }

template< class T , class H >
template< std::ranges::random_access_range R >
void PositionIndex< T , H >::assign( const R & v )
{
 const std::size_t n = std::size_t( std::ranges::size( v ) );
 ADS_PRE( n < NONE );
 f_table.clear();
 f_table.reserve( n );
 for( std::size_t i = 0 ; i < n ; ++i )
  insert( std::ranges::begin( v )[ i ] , Position( i ) );
 }

template< class T , class H >
void PositionIndex< T , H >::renumber( std::span< const Position > E )
{
 ADS_PRE( std::is_sorted( E.begin() , E.end() ) &&
          ( std::adjacent_find( E.begin() , E.end() ) == E.end() ) );
 if( E.empty() || f_table.empty() )
  return;

 // the bits of the positions erased, with the count before each word
 const Position m = Position( E.size() );
 const Position N = E.back() + 1;
 const std::size_t W = ( std::size_t( N ) + 63 ) / 64;
 f_words.assign( W , Word{ 0 , 0 } );
 for( const Position e : E )
  f_words[ e >> 6 ].bits |= std::uint64_t( 1 ) << ( e & 63 );
 Position r = 0;
 for( Word & w : f_words ) {
  w.rank = r;
  r += Position( std::popcount( w.bits ) );
  }
 const Word * const B = f_words.data();

 // the new positions, in the order of the places, with no branch but on
 // the keys of the positions erased; a position p >= N moves by m, as
 // N - 1 is erased
 f_drop.clear();
 f_table.forEach( [ & ]( const T * const & k , Position & p ) {
  const bool high = p >= N;
  const Position q = high ? N - 1 : p;
  const Word w = B[ q >> 6 ];
  const std::uint64_t bit = std::uint64_t( 1 ) << ( q & 63 );
  const bool erased = ( ! high ) && ( w.bits & bit );
  p -= high ? m : w.rank + Position( std::popcount( w.bits & ( bit - 1 ) ) );
  if( erased ) [[unlikely]]
   f_drop.push_back( k );
  } );
 for( const T * k : f_drop )
  f_table.remove( k );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File PositionIndex.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
