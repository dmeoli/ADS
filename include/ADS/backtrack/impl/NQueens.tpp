/*--------------------------------------------------------------------------*/
/*---------------------------- File NQueens.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of NQueens [see NQueens.h], with their contracts.
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

template< class F >
long nQueens( int n , F && f )
{
 ADS_PRE( ( n >= 1 ) && ( n <= 32 ) );
 std::vector< int > Q( n );
 long count = 0;
 bool stop = false;
 const std::uint64_t all = ( std::uint64_t( 1 ) << n ) - 1;
 auto rec = [ & ]( auto & self , int r , std::uint64_t cols ,
                   std::uint64_t d1 , std::uint64_t d2 ) -> void {
  if( stop ) return;
  if( r == n ) {
   ++count;
   ADS_AUDIT( admissible( Q ) );
   stop = f( Q );
   return;
   }
  for( std::uint64_t free = all & ~( cols | d1 | d2 ) ; free && ! stop ;
       free &= free - 1 ) {
   const std::uint64_t bit = free & ( ~free + 1 );
   Q[ r ] = std::countr_zero( bit );
   self( self , r + 1 , cols | bit , ( ( d1 | bit ) << 1 ) & all ,
         ( d2 | bit ) >> 1 );
   }
  };
 rec( rec , 0 , 0 , 0 , 0 );
 return( count );
 }

inline bool admissible( const std::vector< int > & Q )
{
 for( std::size_t i = 0 ; i < Q.size() ; ++i )
  for( std::size_t j = i + 1 ; j < Q.size() ; ++j )
   if( ( Q[ i ] == Q[ j ] ) ||
       ( std::abs( Q[ i ] - Q[ j ] ) == int( j - i ) ) )
    return( false );
 return( true );
 }

inline std::string board( const std::vector< int > & Q )
{
 ADS_PRE( std::all_of( Q.begin() , Q.end() , [ & ]( int c ) {
  return( ( c >= 0 ) && ( c < int( Q.size() ) ) ); } ) );
 std::string s;
 for( int c : Q ) {
  for( std::size_t j = 0 ; j < Q.size() ; ++j )
   s += int( j ) == c ? 'Q' : '.';
  s += '\n';
  }
 return( s );
 }

template< class F >
long nQueensIterative( int n , F && f )
{
 ADS_PRE( n >= 1 );
 std::vector< int > V( n , -1 );
 auto free = [ & ]( int k ) {               // V[ k ] not attacked above
  for( int h = 0 ; h < k ; ++h )
   if( ( V[ h ] == V[ k ] ) || ( std::abs( V[ h ] - V[ k ] ) == k - h ) )
    return( false );
  return( true );
  };
 long count = 0;
 for( int k = 0 ; k >= 0 ; ) {
  do
   ++V[ k ];
  while( ( V[ k ] < n ) && ! free( k ) );
  if( V[ k ] == n ) {                       // row k is over: back
   V[ k ] = -1;
   --k;
   }
  else if( k == n - 1 ) {                   // a solution
   ADS_POST( admissible( V ) );
   ++count;
   if( f( V ) )
    return( count );
   }
  else
   ++k;
  }
 return( count );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File NQueens.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
