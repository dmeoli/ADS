/*--------------------------------------------------------------------------*/
/*------------------------ File SlidingAggregate.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SlidingAggregate [see `SlidingAggregate.h`], one
 * window per algorithm, with their contracts.
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

namespace ADS::detail
{

/*--------------------------------------------------------------------------*/
/*---------------------------- SUBTRACT-ON-EVICT ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class G >
void SubtractWindow< G >::push( const Value & x )
{
 f_q.push_back( x );
 f_s = G::combine( f_s , x );
 }

template< class G >
auto SubtractWindow< G >::pop( void ) -> Value
{
 Value x = std::move( f_q.front() );
 f_q.pop_front();
 f_s = G::combine( G::inverse( x ) , f_s );
 if constexpr( ! exact_v< G > )
  if( ++f_since >= f_period ) {  // the drift dropped
   f_s = G::identity();
   for( const auto & y : f_q )
    f_s = G::combine( f_s , y );
   f_since = 0;
   }
 return( x );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- MONOTONE DEQUE -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class G >
void DequeWindow< G >::push( const Value & x )
{
 while( ! f_c.empty() && ( G::combine( f_c.back().second , x ) == x ) )
  f_c.pop_back();  // absorbed by x
 f_c.emplace_back( f_head + f_q.size() , x );
 f_q.push_back( x );
 }

template< class G >
auto DequeWindow< G >::pop( void ) -> Value
{
 if( f_c.front().first == f_head )
  f_c.pop_front();
 ++f_head;
 Value x = std::move( f_q.front() );
 f_q.pop_front();
 return( x );
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- TWO-STACKS LITE ----------------------------*/
/*--------------------------------------------------------------------------*/

template< class G >
auto StacksWindow< G >::pop( void ) -> Value
{
 if( f_f.empty() ) {  // all the items the first part
  f_f.reserve( f_q.size() );
  Value s = G::identity();
  for( std::size_t k = f_q.size() ; k-- > 0 ; ) {
   s = G::combine( f_q[ k ] , s );
   f_f.push_back( s );
   }
  f_b = G::identity();
  }
 f_f.pop_back();
 Value x = std::move( f_q.front() );
 f_q.pop_front();
 return( x );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------------- REAL TIME -------------------------------*/
/*--------------------------------------------------------------------------*/

template< class Link >
void RealTimeRotation< Link >::swap( RealTimeRotation & o ) noexcept
{
 f_F.swap( o.f_F );
 f_R.swap( o.f_R );
 f_Rf.swap( o.f_Rf );
 f_N.swap( o.f_N );
 std::swap( f_r0 , o.f_r0 );
 std::swap( f_i , o.f_i );
 std::swap( f_n , o.f_n );
 std::swap( f_rot , o.f_rot );
 std::swap( f_link , o.f_link );
 }

template< class G >
template< class Q >
auto AggLink< G >::aggregate( const Q & q ) -> Item
{
 // the front with its moves, the frozen back, its moves at the bottom of
 // the new front, the back: in the order of the window
 Item x = G::identity();
 if( q.f_F.size() > 0 )
  x = q.f_F.summary( q.f_F.hi() - 1 );
 if( q.f_Rf.size() > 0 )
  x = G::combine( x , q.f_Rf.summary( q.f_Rf.hi() - 1 ) );
 if( q.f_rot ) {
  const std::size_t k = std::min( q.f_r0 , q.f_N.hi() );
  if( k > 0 )
   x = G::combine( x , q.f_N.summary( k - 1 ) );
  }
 if( q.f_R.size() > 0 )
  x = G::combine( x , q.f_R.summary( q.f_R.hi() - 1 ) );
 return( x );
 }

template< class Link >
void RealTimeRotation< Link >::push( const Item & x )
{
 f_link.pushBack( f_R , x );
 ++f_n;
 if( f_rot )
  steps( 3 );
 startIfDue();  // also at the end of one
 }

template< class Link >
auto RealTimeRotation< Link >::pop( void ) -> Item
{
 ADS_INVARIANT( f_F.size() > 0 );  // the front never empties first
 Item x = std::move( f_F.at( f_F.hi() - 1 ) );
 f_F.pop();
 --f_n;
 if( f_rot )
  steps( 3 );
 startIfDue();  // also at the end of one
 return( x );
 }

template< class Link >
void RealTimeRotation< Link >::startIfDue( void )
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

template< class Link >
void RealTimeRotation< Link >::steps( int k )
{
 for( ; ; --k ) {
  if( f_Rf.size() > 0 ) {  // the frozen back, from its top
   if( k == 0 )
    return;
   f_link.pushFront( f_N , std::move( f_Rf.at( f_Rf.hi() - 1 ) ) );
   f_Rf.pop();
   }
  else if( f_i < f_F.hi() ) {  // the front from its bottom
   if( k == 0 )
    return;
   f_link.pushFront( f_N , std::move( f_F.at( f_i ) ) );
   f_F.popBottom();
   ++f_i;
   }
  else {  // the moves met the top
   f_F = std::move( f_N );
   f_N = Array();
   f_Rf = Array();
   f_rot = false;
   return;
   }
  }
 }

template< class Link >
template< class F >
void RealTimeRotation< Link >::forEach( F && f ) const
{
 // the front from its top down to the first place not moved, then the
 // moves of the front in the new one, the frozen back from its bottom,
 // its moves in the new front, and the back
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

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS::detail )

/*--------------------------------------------------------------------------*/
/*---------------------- End File SlidingAggregate.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
