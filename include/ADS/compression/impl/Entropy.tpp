/*--------------------------------------------------------------------------*/
/*---------------------------- File Entropy.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of Entropy.h, with their contracts.
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

template< class C , class F >
double entropy( const std::map< C , F > & freq )
{
 double total = 0;
 for( const auto & e : freq ) {
  ADS_PRE( !( e.second < F( 0 ) ) );
  total += double( e.second );
  }
 ADS_PRE( total > 0 );
 double h = 0;
 for( const auto & e : freq )
  if( double( e.second ) > 0 ) {
   const double p = double( e.second ) / total;
   h -= p * std::log2( p );
   }
 return( h );
 }

inline double entropy( std::string_view s )
{
 if( s.empty() ) return( 0 );
 std::map< char , std::size_t > freq;
 for( char c : s ) ++freq[ c ];
 return( entropy( freq ) );
 }

inline double entropy( std::string_view s , std::size_t k )
{
 if( s.empty() ) return( 0 );
 std::map< std::string_view , std::map< char , std::size_t > > follow;
 for( std::size_t i = k ; i < s.size() ; ++i )
  ++follow[ s.substr( i - k , k ) ][ s[ i ] ];
 double h = 0;
 for( const auto & [ w , freq ] : follow ) {
  std::size_t m = 0;
  for( const auto & e : freq ) m += e.second;
  h += double( m ) * entropy( freq );
  }
 h /= double( s.size() );
 ADS_POST( h <= entropy( s ) + 1e-9 );
 return( h );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Entropy.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
