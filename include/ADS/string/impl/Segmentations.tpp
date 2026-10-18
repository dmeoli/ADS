/*--------------------------------------------------------------------------*/
/*------------------------- File Segmentations.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Segmentations [see Segmentations.h], with their
 * contracts.
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

template< class S > requires std::is_class_v< S >
std::uint64_t countSegmentations( const S & X ,
                                  const std::vector< S > & words )
{
 for( const auto & w : words )
  ADS_PRE( w.size() > 0 );
 const std::size_t n = X.size();
 std::vector< std::uint64_t > D( n + 1 , 0 );
 D[ n ] = 1;
 for( std::size_t i = n ; i-- > 0 ; )
  for( const auto & w : words ) {
   if( w.size() > n - i )
    continue;
   std::size_t k = 0;
   while( ( k < w.size() ) && ( X[ i + k ] == w[ k ] ) )
    ++k;
   if( k == w.size() )
    D[ i ] += D[ i + k ];
   }
 return( D[ 0 ] );
 }

/*--------------------------------------------------------------------------*/

inline std::uint64_t countSegmentations(
             const std::string & X , const std::vector< std::string > & words )
{
 return( countSegmentations< std::string >( X , words ) );
 }

/*--------------------------------------------------------------------------*/

inline std::uint64_t countDecodings( const std::vector< int > & digits )
{
 for( int d : digits )
  ADS_PRE( ( d >= 0 ) && ( d <= 9 ) );
 std::uint64_t next = 1 , after = 0;           // D[ i + 1 ] , D[ i + 2 ]
 for( std::size_t i = digits.size() ; i-- > 0 ; ) {
  std::uint64_t here = 0;
  if( digits[ i ] != 0 ) {
   here = next;
   if( ( i + 1 < digits.size() ) &&
       ( 10 * digits[ i ] + digits[ i + 1 ] <= 26 ) )
    here += after;
   }
  after = next;
  next = here;
  }
 return( next );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File Segmentations.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
