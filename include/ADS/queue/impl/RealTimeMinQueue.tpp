/*--------------------------------------------------------------------------*/
/*----------------------- File RealTimeMinQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RealTimeMinQueue [see `RealTimeMinQueue.h`], with their
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

template< class T , class C >
RealTimeMinQueue< T , C >::RealTimeMinQueue( const RealTimeMinQueue & o )
    : f_R( 1 ) , f_less( o.f_less )
{
 o.forEach( [ & ]( const T & x ) { enqueue( x ); } );
 }

template< class T , class C >
auto RealTimeMinQueue< T , C >::operator=( const RealTimeMinQueue & o )
  -> RealTimeMinQueue &
{
 RealTimeMinQueue c( o );
 swap( c );
 return( *this );
 }

template< class T , class C >
void RealTimeMinQueue< T , C >::swap( RealTimeMinQueue & o ) noexcept
{
 f_F.swap( o.f_F );
 f_R.swap( o.f_R );
 f_Rf.swap( o.f_Rf );
 f_N.swap( o.f_N );
 std::swap( f_r0 , o.f_r0 );
 std::swap( f_i , o.f_i );
 std::swap( f_n , o.f_n );
 std::swap( f_rot , o.f_rot );
 std::swap( f_less , o.f_less );
 }

template< class T , class C >
void RealTimeMinQueue< T , C >::enqueue( const T & x )
{
 [[maybe_unused]] const std::size_t n = f_n;
 f_R.push( x , f_less );
 ++f_n;
 if( f_rot )
  steps( 3 );
 startIfDue();  // also at the end of one
 ADS_POST( f_n == n + 1 );
 }

template< class T , class C >
T RealTimeMinQueue< T , C >::dequeue( void )
{
 ADS_PRE( ! empty() );
 [[maybe_unused]] const std::size_t n = f_n;
 T x = std::move( f_F.at( f_F.hi() - 1 ) );
 f_F.pop();
 --f_n;
 if( f_rot )
  steps( 3 );
 startIfDue();  // also at the end of one
 ADS_POST( f_n == n - 1 );
 return( x );
 }

template< class T , class C >
void RealTimeMinQueue< T , C >::startIfDue( void )
{
 while( ! f_rot && ( f_R.size() > f_F.size() ) ) {  // |R| = |F| + 1
  f_r0 = f_R.size();
  f_Rf = std::move( f_R );
  f_R = Array( f_F.size() + f_r0 + 2 );  // room until the next one
  f_N = Array( f_F.size() + f_r0 );
  f_i = 0;
  f_rot = true;
  steps( 3 );
  }
 }

template< class T , class C >
void RealTimeMinQueue< T , C >::steps( int k )
{
 for( ; ; --k ) {
  if( f_Rf.size() > 0 ) {  // the frozen back, from its top
   if( k == 0 )
    return;
   f_N.push( std::move( f_Rf.at( f_Rf.hi() - 1 ) ) , f_less );
   f_Rf.pop();
   }
  else if( f_i < f_F.hi() ) {  // F from its bottom
   if( k == 0 )
    return;
   f_N.push( std::move( f_F.at( f_i ) ) , f_less );
   f_F.popBottom();
   ++f_i;
   }
  else {  // the moves met the top of F
   f_F = std::move( f_N );
   f_N = Array();
   f_Rf = Array();
   f_rot = false;
   return;
   }
  }
 }

template< class T , class C >
const T & RealTimeMinQueue< T , C >::top( void ) const
{
 ADS_PRE( ! empty() );
 ADS_INVARIANT( f_F.size() > 0 );  // the front never empties first
 return( f_F.at( f_F.hi() - 1 ) );
 }

template< class T , class C >
const T & RealTimeMinQueue< T , C >::min( void ) const
{
 ADS_PRE( ! empty() );
 const T * best = nullptr;
 auto take = [ & ]( const T & x ) {
  if( ! best || f_less( x , *best ) )
   best = &x;
  };
 if( f_F.size() > 0 ) {  // its least place may be in N
  const std::size_t a = f_F.least( f_F.hi() - 1 );
  take( a >= f_F.lo() ? f_F.at( a ) : f_N.at( f_r0 + a ) );
  }
 if( f_Rf.size() > 0 )
  take( f_Rf.min() );
 if( f_N.size() > 0 )
  take( f_N.min() );
 if( f_R.size() > 0 )
  take( f_R.min() );
 return( *best );
 }

template< class T , class C >
template< class F >
void RealTimeMinQueue< T , C >::forEach( F && f ) const
{
 // F from its top down to the first place not moved, then the moves of F
 // in N, the frozen back from its bottom, its moves in N, and R
 for( std::size_t i = f_F.hi() ; i > f_F.lo() ; --i )
  f( f_F.at( i - 1 ) );
 const std::size_t k = f_rot ? std::min( f_r0 , f_N.hi() ) : 0;
 for( std::size_t i = f_N.hi() ; i > k ; --i )
  f( f_N.at( i - 1 ) );
 for( std::size_t i = f_Rf.lo() ; i < f_Rf.hi() ; ++i )
  f( f_Rf.at( i ) );
 for( std::size_t i = k ; i > 0 ; --i )
  f( f_N.at( i - 1 ) );
 for( std::size_t i = f_R.lo() ; i < f_R.hi() ; ++i )
  f( f_R.at( i ) );
 }

template< class T , class C >
void RealTimeMinQueue< T , C >::clear( void )
{
 RealTimeMinQueue e( f_less );
 swap( e );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File RealTimeMinQueue.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
