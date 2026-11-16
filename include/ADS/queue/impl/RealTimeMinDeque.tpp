/*--------------------------------------------------------------------------*/
/*----------------------- File RealTimeMinDeque.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RealTimeMinDeque [see `RealTimeMinDeque.h`], with their
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

namespace ADS
{

template< std::copy_constructible T , class C >
RealTimeMinDeque< T , C >::RealTimeMinDeque( const RealTimeMinDeque & o )
    : f_A
{ Array( 16 ) , Array( 16 ) } , f_cells( o.f_cells )
{
 o.forEach( [ & ]( const T & x ) { push_back( x ); } );
 }

template< std::copy_constructible T , class C >
auto RealTimeMinDeque< T , C >::operator=( const RealTimeMinDeque & o )
  -> RealTimeMinDeque &
{
 RealTimeMinDeque c( o );
 swap( c );
 return( *this );
 }

template< std::copy_constructible T , class C >
void RealTimeMinDeque< T , C >::swap( RealTimeMinDeque & o ) noexcept
{
 for( std::size_t i = 0 ; i < 2 ; ++i ) {
  f_A[ i ].swap( o.f_A[ i ] );
  f_new[ i ].swap( o.f_new[ i ] );
  }
 for( std::size_t i = 0 ; i < f_old.size() ; ++i )
  f_old[ i ].swap( o.f_old[ i ] );
 std::swap( f_g , o.f_g );
 std::swap( f_p , o.f_p );
 std::swap( f_cs , o.f_cs );
 std::swap( f_cg , o.f_cg );
 std::swap( f_t , o.f_t );
 std::swap( f_n , o.f_n );
 std::swap( f_rb , o.f_rb );
 std::swap( f_cells , o.f_cells );
 }

template< std::copy_constructible T , class C >
void RealTimeMinDeque< T , C >::push( int s , const T & x )
{
 [[maybe_unused]] const std::size_t n = f_n;
 f_cells.push( f_A[ s ] , x );
 ++f_n;
 afterOp();
 ADS_POST( f_n == n + 1 );
 }

template< std::copy_constructible T , class C >
T RealTimeMinDeque< T , C >::pop( int s )
{
 ADS_PRE( ! empty() );
 [[maybe_unused]] const std::size_t n = f_n;
 const int x = f_A[ s ].size() > 0 ? s : 1 - s;  // or the only item
 ADS_INVARIANT( ( x == s ) || ( ! f_rb && ( f_n == 1 ) ) );
 Array & A = f_A[ x ];
 T v = std::move( A.at( A.hi() - 1 ) );
 A.pop();
 --f_n;
 if( f_rb ) {  // the copies keep prefixes
  ADS_INVARIANT( ( x != f_g ) || ( A.hi() >= f_t ) );
  std::size_t & c = ( x == f_g ) ? f_cg : f_cs;
  if( A.hi() < c ) {
   f_new[ x ].pop();
   --c;
   }
  }
 afterOp();
 ADS_POST( f_n == n - 1 );
 return( v );
 }

template< std::copy_constructible T , class C >
const T & RealTimeMinDeque< T , C >::end( int s ) const
{
 ADS_PRE( ! empty() );
 const Array & A = f_A[ s ].size() > 0 ? f_A[ s ] : f_A[ 1 - s ];
 return( A.at( A.hi() - 1 ) );
 }

template< std::copy_constructible T , class C >
const T & RealTimeMinDeque< T , C >::min( void ) const
{
 ADS_PRE( ! empty() );
 if( f_A[ 0 ].size() == 0 )
  return( f_cells.min( f_A[ 1 ] ) );
 if( f_A[ 1 ].size() == 0 )
  return( f_cells.min( f_A[ 0 ] ) );
 const T &a = f_cells.min( f_A[ 0 ] ) , &b = f_cells.min( f_A[ 1 ] );
 return( f_cells.less( b , a ) ? b : a );
 }

template< std::copy_constructible T , class C >
bool RealTimeMinDeque< T , C >::due( void ) const
{
 const std::size_t a = f_A[ 0 ].size() , b = f_A[ 1 ].size();
 return( ( std::max( a , b ) > 3 * std::min( a , b ) + 1 ) ||
	  ( 2 * a >= f_A[ 0 ].capacity() ) ||
	  ( 2 * b >= f_A[ 1 ].capacity() ) );
 }

template< std::copy_constructible T , class C >
void RealTimeMinDeque< T , C >::afterOp( void )
{
 int budget = kSteps;  // the old arrays, a little
 for( Array & o : f_old )
  while( ( budget > 0 ) && ( o.capacity() > 0 ) ) {
   if( o.size() == 0 ) {
    o = Array();
    break;
    }
   o.pop();
   --budget;
   }
 if( f_rb )
  steps( kSteps );
 else if( due() )
  start();
 }

template< std::copy_constructible T , class C >
void RealTimeMinDeque< T , C >::start( void )
{
 f_g = f_A[ 0 ].size() >= f_A[ 1 ].size() ? 0 : 1;
 const std::size_t n0 = f_n , keep = ( n0 + 1 ) / 2;
 f_t = f_A[ f_g ].size() - keep;  // the bottom of G that passes
 f_new[ 0 ] = Array( 2 * n0 + 16 );
 f_new[ 1 ] = Array( 2 * n0 + 16 );
 f_p = f_t;
 f_cs = 0;
 f_cg = f_t;
 f_rb = true;
 if( n0 >= kSmall ) {
  steps( kSteps );
  return;
  }
 while( f_rb )
  steps( kSteps );  // small: at once, and the old
 for( Array & o : f_old )
  o = Array();  // arrays freed now
 }

template< std::copy_constructible T , class C >
void RealTimeMinDeque< T , C >::steps( int k )
{
 const int s = 1 - f_g;
 Array &G = f_A[ f_g ] , &S = f_A[ s ];
 Array &Gn = f_new[ f_g ] , &Sn = f_new[ s ];
 for( ; ; --k ) {
  if( f_p > 0 ) {  // the bottom of G, downwards
   if( k == 0 )
    return;
   f_cells.push( Sn , G.at( --f_p ) );
   }
  else if( f_cs < S.hi() ) {  // the prefix of S
   if( k == 0 )
    return;
   f_cells.push( Sn , S.at( f_cs++ ) );
   }
  else if( f_cg < G.hi() ) {  // the prefix of G above t
   if( k == 0 )
    return;
   f_cells.push( Gn , G.at( f_cg++ ) );
   }
  else
   break;  // both prefixes complete
  }
 std::size_t j = 0;  // the old arrays to destroy
 for( Array * a : { &G , &S } ) {
  while( f_old[ j ].capacity() > 0 ) {
   ++j;
   ADS_INVARIANT( j < f_old.size() );
   }
  f_old[ j ] = std::move( *a );
  }
 G = std::move( Gn );
 S = std::move( Sn );
 Gn = Array();
 Sn = Array();
 f_rb = false;
 }

template< std::copy_constructible T , class C >
template< class F >
void RealTimeMinDeque< T , C >::forEach( F && f ) const
{
 for( std::size_t i = f_A[ 0 ].hi() ; i > 0 ; --i )
  f( f_A[ 0 ].at( i - 1 ) );
 for( std::size_t i = 0 ; i < f_A[ 1 ].hi() ; ++i )
  f( f_A[ 1 ].at( i ) );
 }

template< std::copy_constructible T , class C >
void RealTimeMinDeque< T , C >::clear( void )
{
 RealTimeMinDeque e( f_cells.less );
 swap( e );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File RealTimeMinDeque.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
