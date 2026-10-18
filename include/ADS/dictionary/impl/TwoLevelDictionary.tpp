/*--------------------------------------------------------------------------*/
/*---------------------- File TwoLevelDictionary.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of TwoLevelDictionary [see TwoLevelDictionary.h], with
 * their contracts.
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

inline std::vector< std::string > TwoLevelDictionary::anchorsOf(
                                               const FrontCodedDictionary & F )
{
 std::vector< std::string > A;
 A.reserve( F.anchors() );
 for( std::size_t k = 0 ; k < F.anchors() ; ++k )
  A.emplace_back( F.anchorString( k ) );
 return( A );
 }

inline TwoLevelDictionary::TwoLevelDictionary( FrontCodedDictionary F )
 : f_F( std::move( F ) ) , f_T( anchorsOf( f_F ) )
{
 }

inline std::size_t TwoLevelDictionary::lexicographicPosition(
                                                  std::string_view Q ) const
{
 const std::size_t r = f_F.rankInBucket( f_T.lexicographicPosition( Q ) ,
                                         Q , false );
 ADS_POST( ( ( r == 0 ) || ( f_F.access( r - 1 ) < Q ) ) &&
           ( ( r == size() ) || ( Q <= f_F.access( r ) ) ) );
 return( r );
 }

inline std::size_t TwoLevelDictionary::prefixEnd( std::string_view P ) const
{
 const std::size_t r = f_F.rankInBucket( f_T.prefixEnd( P ) , P , true );
 ADS_POST( ( ( r == 0 ) ||
             detail::precedes( f_F.access( r - 1 ) , P , true ) ) &&
           ( ( r == size() ) ||
             ! detail::precedes( f_F.access( r ) , P , true ) ) );
 return( r );
 }

inline std::pair< std::size_t , std::size_t > TwoLevelDictionary::prefixRange(
                                                  std::string_view P ) const
{
 return( std::pair( lexicographicPosition( P ) , prefixEnd( P ) ) );
 }

inline std::string TwoLevelDictionary::access( std::size_t i ) const
{
 return( f_F.access( i ) );
 }

inline std::size_t TwoLevelDictionary::size( void ) const
{
 return( f_F.size() );
 }

inline const PatriciaTrie & TwoLevelDictionary::upper( void ) const
{
 return( f_T );
 }

inline const FrontCodedDictionary & TwoLevelDictionary::lower( void ) const
{
 return( f_F );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File TwoLevelDictionary.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
