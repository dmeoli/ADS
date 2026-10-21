/*--------------------------------------------------------------------------*/
/*----------------------- File WorkStealingDeque.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of WorkStealingDeque [see WorkStealingDeque.h], with their
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

template< class T >
WorkStealingDeque< T >::WorkStealingDeque( std::size_t capacity )
{
 ADS_PRE( ( capacity >= 2 ) && std::has_single_bit( capacity ) );
 f_rings.push_back( std::make_unique< Ring >( std::int64_t( capacity ) ) );
 f_ring.store( f_rings.back().get() , std::memory_order_relaxed );
 }

template< class T >
void WorkStealingDeque< T >::push( T x )
{
 const std::int64_t b = f_bottom.load( std::memory_order_relaxed ) ,
                    t = f_top.load( std::memory_order_acquire );
 Ring * a = f_ring.load( std::memory_order_relaxed );
 if( b - t > a->mask ) [[unlikely]] {       // full: twice the places, the
  auto n = std::make_unique< Ring >( 2 * ( a->mask + 1 ) );   // items of
  for( std::int64_t i = t ; i < b ; ++i )   // [ t , b ) at their indices
   n->put( i , a->get( i ) );
  a = n.get();
  f_rings.push_back( std::move( n ) );
  f_ring.store( a , std::memory_order_release );
  }
 a->put( b , x );
 f_bottom.store( b + 1 , std::memory_order_release );
 }

template< class T >
std::optional< T > WorkStealingDeque< T >::take( void )
{
 const std::int64_t b = f_bottom.load( std::memory_order_relaxed ) - 1;
 Ring * a = f_ring.load( std::memory_order_relaxed );
 f_bottom.store( b , std::memory_order_seq_cst );   // then the top
 std::int64_t t = f_top.load( std::memory_order_seq_cst );
 if( t > b ) {                              // empty
  f_bottom.store( b + 1 , std::memory_order_relaxed );
  return( std::nullopt );
  }
 const T x = a->get( b );
 if( t < b )                                // more than one: no thief
  return( x );                              // reaches b
 // the last item: the owner and the thieves race for it on the top
 const bool won = f_top.compare_exchange_strong( t , t + 1 ,
                                                 std::memory_order_seq_cst ,
                                                 std::memory_order_relaxed );
 f_bottom.store( b + 1 , std::memory_order_relaxed );
 if( ! won )
  return( std::nullopt );
 return( x );
 }

template< class T >
bool WorkStealingDeque< T >::steal( T & x )
{
 std::int64_t t = f_top.load( std::memory_order_seq_cst );   // then the
 const std::int64_t b = f_bottom.load( std::memory_order_seq_cst );   // bottom
 if( t >= b )
  return( false );
 const Ring * a = f_ring.load( std::memory_order_acquire );
 const T y = a->get( t );
 if( ! f_top.compare_exchange_strong( t , t + 1 , std::memory_order_seq_cst ,
                                      std::memory_order_relaxed ) )
  return( false );                          // another one took it
 x = y;
 return( true );
 }

template< class T >
std::size_t WorkStealingDeque< T >::size( void ) const
{
 const std::int64_t b = f_bottom.load( std::memory_order_acquire ) ,
                    t = f_top.load( std::memory_order_acquire );
 return( b > t ? std::size_t( b - t ) : 0 );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File WorkStealingDeque.tpp --------------------*/
/*--------------------------------------------------------------------------*/
