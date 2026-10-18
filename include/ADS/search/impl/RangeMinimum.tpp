/*--------------------------------------------------------------------------*/
/*------------------------- File RangeMinimum.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of RangeMinimum.h, with their contracts.
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

namespace detail {

/// r is the leftmost minimum of A[ i .. j ], by a scan
template< class A , class Compare >
bool leftmostMinimum( const A & a , std::size_t i , std::size_t j ,
                      std::size_t r , Compare less )
{
 if( ( r < i ) || ( r > j ) ) return( false );
 for( std::size_t k = i ; k <= j ; ++k ) {
  if( less( a[ k ] , a[ r ] ) ) return( false );
  if( ( k < r ) && ! less( a[ r ] , a[ k ] ) ) return( false );
  }
 return( true );
 }

/// no item of A is smaller than its parent P
template< class T , class Compare >
bool heapOrdered( const std::vector< T > & A ,
                  const std::vector< std::size_t > & P , Compare less )
{
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  if( ( P[ i ] < A.size() ) && less( A[ i ] , A[ P[ i ] ] ) )
   return( false );
 return( true );
 }

/// floor( log2( x ) ) for x > 0
inline std::size_t rmqLog2( std::size_t x )
{
 return( std::size_t( std::bit_width( x ) ) - 1 );
 }

}  // end( namespace detail )

template< class T , class Compare >
std::vector< std::size_t > cartesianTree( const std::vector< T > & A ,
                                          Compare less )
{
 const std::size_t n = A.size();
 std::vector< std::size_t > P( n , n ) , S;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  std::size_t last = n;            // the last item popped
  while( ! S.empty() && less( A[ i ] , A[ S.back() ] ) ) {
   last = S.back();
   S.pop_back();
   }
  if( last < n ) P[ last ] = i;
  if( ! S.empty() ) P[ i ] = S.back();
  S.push_back( i );
  }
 ADS_AUDIT( detail::heapOrdered( A , P , less ) );
 return( P );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- SparseTable ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Compare >
SparseTable< T , Compare >::SparseTable( std::vector< T > A , Compare less )
 : f_A( std::move( A ) ) , f_less( less )
{
 const std::size_t n = f_A.size();
 if( n == 0 ) return;
 f_M.emplace_back( n );
 for( std::size_t i = 0 ; i < n ; ++i ) f_M[ 0 ][ i ] = i;
 for( std::size_t L = 1 ; ( std::size_t( 1 ) << L ) <= n ; ++L ) {
  const std::size_t h = std::size_t( 1 ) << ( L - 1 );
  std::vector< std::size_t > row( n - 2 * h + 1 );
  for( std::size_t i = 0 ; i < row.size() ; ++i )
   row[ i ] = better( f_M[ L - 1 ][ i ] , f_M[ L - 1 ][ i + h ] );
  f_M.push_back( std::move( row ) );
  }
 }

template< class T , class Compare >
std::size_t SparseTable< T , Compare >::better( std::size_t a ,
                                                std::size_t b ) const
{
 return( f_less( f_A[ b ] , f_A[ a ] ) ? b : a );
 }

template< class T , class Compare >
std::size_t SparseTable< T , Compare >::query( std::size_t i ,
                                               std::size_t j ) const
{
 ADS_PRE( ( i <= j ) && ( j < f_A.size() ) );
 const std::size_t L = detail::rmqLog2( j - i + 1 );
 const std::size_t r = better( f_M[ L ][ i ] ,
                               f_M[ L ][ j + 1 - ( std::size_t( 1 ) << L ) ] );
 ADS_AUDIT( detail::leftmostMinimum( f_A , i , j , r , f_less ) );
 return( r );
 }

template< class T , class Compare >
const T & SparseTable< T , Compare >::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_A.size() );
 return( f_A[ i ] );
 }

/*--------------------------------------------------------------------------*/
/*---------------------- PlusMinusOneRangeMinimum --------------------------*/
/*--------------------------------------------------------------------------*/

inline PlusMinusOneRangeMinimum::PlusMinusOneRangeMinimum(
                                                       std::vector< long > D )
 : f_D( std::move( D ) )
{
 const std::size_t n = f_D.size();
 for( std::size_t i = 1 ; i < n ; ++i )
  ADS_PRE( ( f_D[ i ] - f_D[ i - 1 ] == 1 ) ||
           ( f_D[ i - 1 ] - f_D[ i ] == 1 ) );
 if( n == 0 ) return;
 f_d = std::max< std::size_t >( 1 , detail::rmqLog2( n ) / 2 );
 const std::size_t d = f_d , blocks = ( n + d - 1 ) / d;

 // the signature of a block: bit t - 1 set if it rises at t; the missing
 // items of the last block rise, so that they are never a minimum
 f_sig.assign( blocks , 0 );
 f_blockMin.assign( blocks , 0 );
 std::vector< long > mins( blocks );
 for( std::size_t k = 0 ; k < blocks ; ++k ) {
  const std::size_t s = k * d;
  std::size_t m = s;
  for( std::size_t t = 1 ; t < d ; ++t ) {
   if( ( s + t >= n ) || ( f_D[ s + t ] > f_D[ s + t - 1 ] ) )
    f_sig[ k ] |= std::uint32_t( 1 ) << ( t - 1 );
   if( ( s + t < n ) && ( f_D[ s + t ] < f_D[ m ] ) ) m = s + t;
   }
  f_blockMin[ k ] = m;
  mins[ k ] = f_D[ m ];
  }
 f_blocks = SparseTable< long >( std::move( mins ) );

 // the leftmost minimum of the offsets a .. b of every signature
 const std::size_t sigs = std::size_t( 1 ) << ( d - 1 );
 f_table.assign( sigs * d * d , 0 );
 for( std::size_t sig = 0 ; sig < sigs ; ++sig )
  for( std::size_t a = 0 ; a < d ; ++a ) {
   long h = 0 , best = 0;
   std::size_t arg = a;
   f_table[ ( sig * d + a ) * d + a ] = std::uint8_t( a );
   for( std::size_t b = a + 1 ; b < d ; ++b ) {
    h += ( ( sig >> ( b - 1 ) ) & 1 ) ? 1 : -1;
    if( h < best ) {
     best = h;
     arg = b;
     }
    f_table[ ( sig * d + a ) * d + b ] = std::uint8_t( arg );
    }
   }
 }

inline std::size_t PlusMinusOneRangeMinimum::inBlock( std::size_t k ,
                                                      std::size_t a ,
                                                      std::size_t b ) const
{
 return( k * f_d + f_table[ ( f_sig[ k ] * f_d + a ) * f_d + b ] );
 }

inline std::size_t PlusMinusOneRangeMinimum::query( std::size_t i ,
                                                    std::size_t j ) const
{
 ADS_PRE( ( i <= j ) && ( j < f_D.size() ) );
 const std::size_t d = f_d , bi = i / d , bj = j / d;
 std::size_t r;
 if( bi == bj )
  r = inBlock( bi , i % d , j % d );
 else {
  r = inBlock( bi , i % d , d - 1 );
  if( bj > bi + 1 ) {
   const std::size_t m = f_blockMin[ f_blocks.query( bi + 1 , bj - 1 ) ];
   if( f_D[ m ] < f_D[ r ] ) r = m;
   }
  const std::size_t c = inBlock( bj , 0 , j % d );
  if( f_D[ c ] < f_D[ r ] ) r = c;
  }
 ADS_AUDIT( detail::leftmostMinimum( f_D , i , j , r , std::less<>() ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- RangeMinimum --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Compare >
RangeMinimum< T , Compare >::RangeMinimum( std::vector< T > A ,
                                           Compare less )
 : f_A( std::move( A ) ) , f_less( less )
{
 const std::size_t n = f_A.size();
 if( n == 0 ) return;
 const std::vector< std::size_t > P = cartesianTree( f_A , f_less );
 std::vector< std::size_t > left( n , n ) , right( n , n );
 std::size_t root = n;
 for( std::size_t i = 0 ; i < n ; ++i )
  if( P[ i ] == n ) root = i;
  else if( i < P[ i ] ) left[ P[ i ] ] = i;
  else right[ P[ i ] ] = i;

 // the Euler tour, a node written on arrival and after each child
 std::vector< long > D;
 f_E.reserve( 2 * n - 1 );
 D.reserve( 2 * n - 1 );
 f_first.assign( n , 0 );
 std::vector< std::pair< std::size_t , int > > S;   // node, children done
 S.emplace_back( root , 0 );
 f_first[ root ] = 0;
 f_E.push_back( root );
 D.push_back( 0 );
 while( ! S.empty() ) {
  auto & [ v , done ] = S.back();
  const std::size_t c = done == 0 ? left[ v ] : done == 1 ? right[ v ] : n;
  if( done == 2 ) {
   S.pop_back();
   if( ! S.empty() ) {
    f_E.push_back( S.back().first );
    D.push_back( long( S.size() ) - 1 );
    }
   continue;
   }
  ++done;
  if( c < n ) {
   f_first[ c ] = f_E.size();
   f_E.push_back( c );
   D.push_back( long( S.size() ) );
   S.emplace_back( c , 0 );
   }
  }
 f_tour = PlusMinusOneRangeMinimum( std::move( D ) );
 ADS_POST( f_E.size() == 2 * n - 1 );
 }

template< class T , class Compare >
std::size_t RangeMinimum< T , Compare >::query( std::size_t i ,
                                                std::size_t j ) const
{
 ADS_PRE( ( i <= j ) && ( j < f_A.size() ) );
 std::size_t a = f_first[ i ] , b = f_first[ j ];
 if( a > b ) std::swap( a , b );
 const std::size_t r = f_E[ f_tour.query( a , b ) ];
 ADS_AUDIT( detail::leftmostMinimum( f_A , i , j , r , f_less ) );
 return( r );
 }

template< class T , class Compare >
const T & RangeMinimum< T , Compare >::operator[]( std::size_t i ) const
{
 ADS_PRE( i < f_A.size() );
 return( f_A[ i ] );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File RangeMinimum.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
