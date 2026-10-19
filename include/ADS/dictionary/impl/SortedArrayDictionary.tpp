/*--------------------------------------------------------------------------*/
/*--------------------- File SortedArrayDictionary.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SortedArrayDictionary [see SortedArrayDictionary.h],
 * with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

template< std::totally_ordered K , class V >
std::optional< V > SortedArrayDictionary< K , V >::lookup( const K & k ) const
{
 const std::size_t i = place( k );
 if( ( i < f_A.size() ) && ( f_A[ i ].first == k ) )
  return( f_A[ i ].second );
 return( std::nullopt );
 }

template< std::totally_ordered K , class V >
void SortedArrayDictionary< K , V >::insert( const K & k , const V & v )
{
 const std::size_t i = place( k );
 if( ( i < f_A.size() ) && ( f_A[ i ].first == k ) )
  f_A[ i ].second = v;                       // the new value replaces it
 else
  f_A.insert( f_A.begin() + i , { k , v } );
 ADS_POST( lookup( k ).has_value() );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered K , class V >
void SortedArrayDictionary< K , V >::remove( const K & k )
{
 const std::size_t i = place( k );
 if( ( i < f_A.size() ) && ( f_A[ i ].first == k ) )
  f_A.erase( f_A.begin() + i );
 ADS_POST( ! lookup( k ).has_value() );
 ADS_INVARIANT( sorted() );
 }

template< std::totally_ordered K , class V >
bool SortedArrayDictionary< K , V >::empty( void ) const
{
 return( f_A.empty() );
 }

template< std::totally_ordered K , class V >
std::size_t SortedArrayDictionary< K , V >::size( void ) const
{
 return( f_A.size() );
 }

template< std::totally_ordered K , class V >
const std::vector< std::pair< K , V > > &
SortedArrayDictionary< K , V >::pairs( void ) const
{
 return( f_A );
 }

template< std::totally_ordered K , class V >
std::size_t SortedArrayDictionary< K , V >::place( const K & k ) const
{
 std::size_t lo = 0 , hi = f_A.size();      // the answer is in [ lo , hi ]
 while( lo < hi ) {
  const std::size_t m = lo + ( hi - lo ) / 2;
  if( f_A[ m ].first < k )
   lo = m + 1;
  else
   hi = m;
  }
 return( lo );
 }

template< std::totally_ordered K , class V >
bool SortedArrayDictionary< K , V >::sorted( void ) const
{
 for( std::size_t i = 1 ; i < f_A.size() ; ++i )
  if( !( f_A[ i - 1 ].first < f_A[ i ].first ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------- End File SortedArrayDictionary.tpp -------------------*/
/*--------------------------------------------------------------------------*/
