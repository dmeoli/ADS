/*--------------------------------------------------------------------------*/
/*------------------------ File SortingNetworks.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The networks of SortingNetworks.h, with their contracts.
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

namespace detail {

/// each comparator joins two distinct places below n
inline bool within( const std::vector< Comparator > & N , std::size_t n )
{
 return( std::all_of( N.begin() , N.end() , [ n ]( const Comparator & c ) {
  return( ( c.first != c.second ) && ( c.first < n ) && ( c.second < n ) );
  } ) );
 }

}  // end( namespace detail )

inline std::vector< Comparator > mergeExchangeNetwork( std::size_t n )
{
 std::vector< Comparator > N;
 if( n < 2 )
  return( N );
 std::size_t t = 0;                         // t = ceil( log2 n )
 while( ( std::size_t( 1 ) << t ) < n ) ++t;
 for( std::size_t p = std::size_t( 1 ) << ( t - 1 ) ; p > 0 ; p >>= 1 ) {
  std::size_t q = std::size_t( 1 ) << ( t - 1 ) , r = 0 , d = p;
  for( ; ; ) {                              // Algorithm M, steps M3-M5
   for( std::size_t i = 0 ; i + d < n ; ++i )
    if( ( i & p ) == r )
     N.emplace_back( i , i + d );
   if( q == p )
    break;
   d = q - p;
   q >>= 1;
   r = p;
   }
  }
 ADS_POST( detail::within( N , n ) && std::all_of( N.begin() , N.end() ,
           []( const Comparator & c ) { return( c.first < c.second ); } ) );
 return( N );
 }

inline std::vector< Comparator > bitonicNetwork( std::size_t n )
{
 ADS_PRE( ( n == 0 ) || std::has_single_bit( n ) );
 std::vector< Comparator > N;
 for( std::size_t k = 2 ; k <= n ; k <<= 1 )        // the size of the runs
  for( std::size_t j = k >> 1 ; j > 0 ; j >>= 1 )   // the half-cleaners
   for( std::size_t i = 0 ; i < n ; ++i ) {
    const std::size_t l = i ^ j;
    if( l > i )                             // ascending if ( i & k ) == 0
     N.push_back( ( i & k ) == 0 ? Comparator( i , l ) : Comparator( l , i ) );
    }
 ADS_POST( detail::within( N , n ) );
 return( N );
 }

template< class T , class C >
void applyNetwork( std::vector< T > & A , const std::vector< Comparator > & N ,
                   C less )
{
 ADS_PRE( detail::within( N , A.size() ) );
 for( const auto & [ i , j ] : N )
  if( less( A[ j ] , A[ i ] ) )
   std::swap( A[ i ] , A[ j ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File SortingNetworks.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
