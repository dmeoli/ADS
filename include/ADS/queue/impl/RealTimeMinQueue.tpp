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
void RealTimeMinQueue< T , C >::enqueue( const T & x )
{
 [[maybe_unused]] const std::size_t n = size();
 f_q.push( x );
 ADS_POST( size() == n + 1 );
 }

template< class T , class C >
T RealTimeMinQueue< T , C >::dequeue( void )
{
 ADS_PRE( ! empty() );
 [[maybe_unused]] const std::size_t n = size();
 T x = f_q.pop();
 ADS_POST( size() == n - 1 );
 return( x );
 }

template< class T , class C >
const T & RealTimeMinQueue< T , C >::top( void ) const
{
 ADS_PRE( ! empty() );
 ADS_INVARIANT( f_q.f_F.size() > 0 );  // the front never empties first
 return( f_q.front() );
 }

template< class T , class C >
const T & RealTimeMinQueue< T , C >::min( void ) const
{
 ADS_PRE( ! empty() );
 const auto & less = f_q.f_link.less;
 const auto &F = f_q.f_F , &N = f_q.f_N;
 const T * best = nullptr;
 auto take = [ & ]( const T & x ) {
  if( ! best || less( x , *best ) )
   best = &x;
  };
 if( F.size() > 0 ) {  // its least place may be in N
  const std::size_t a = F.summary( F.hi() - 1 );
  take( a >= F.lo() ? F.at( a ) : N.at( f_q.f_r0 + a ) );
  }
 if( f_q.f_Rf.size() > 0 )
  take( f_q.f_link.min( f_q.f_Rf ) );
 if( N.size() > 0 )
  take( f_q.f_link.min( N ) );
 if( f_q.f_R.size() > 0 )
  take( f_q.f_link.min( f_q.f_R ) );
 return( *best );
 }

template< class T , class C >
template< class F >
void RealTimeMinQueue< T , C >::forEach( F && f ) const
{
 f_q.forEach( std::forward< F >( f ) );
 }

template< class T , class C >
void RealTimeMinQueue< T , C >::clear( void )
{
 f_q.clear();
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File RealTimeMinQueue.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
