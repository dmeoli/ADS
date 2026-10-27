/*--------------------------------------------------------------------------*/
/*---------------------- File SizeClassAllocator.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SizeClassAllocator [see SizeClassAllocator.h], with
 * their contracts. allocate() and deallocate() are inline, so that the
 * compiler expands them in the loop of the caller.
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

inline unsigned SizeClassAllocator::sizeClass( std::size_t n )
{
 if( n <= 8 )
  return( 0 );
 if( n <= 128 )                             // the multiples of 16
  return( unsigned( ( n + 15 ) >> 4 ) );
 const unsigned e = unsigned( std::bit_width( n - 1 ) ) - 1;   // 2^e < n
 const std::size_t step = std::size_t( 1 ) << ( e - 2 );     // <= 2^(e+1)
 const unsigned j = unsigned( ( n - ( std::size_t( 1 ) << e ) + step - 1 )
                              >> ( e - 2 ) );
 return( 9 + ( e - 7 ) * 4 + j - 1 );
 }

inline std::size_t SizeClassAllocator::classSize( unsigned c )
{
 if( c == 0 )
  return( 8 );
 if( c <= 8 )
  return( std::size_t( c ) << 4 );
 const unsigned e = 7 + ( c - 9 ) / 4 , j = ( c - 9 ) % 4 + 1;
 return( ( std::size_t( 1 ) << e ) + ( std::size_t( j ) << ( e - 2 ) ) );
 }

inline SizeClassAllocator::SizeClassAllocator(
                                     SizeClassAllocator && other ) noexcept
 : f_pages( std::exchange( other.f_pages , nullptr ) ) ,
   f_bytes( std::exchange( other.f_bytes , 0 ) )
{
 for( unsigned c = 0 ; c < CLASSES ; ++c ) {
  f_free[ c ] = std::exchange( other.f_free[ c ] , nullptr );
  f_next[ c ] = std::exchange( other.f_next[ c ] , nullptr );
  f_end[ c ] = std::exchange( other.f_end[ c ] , nullptr );
  }
 }

inline SizeClassAllocator &
SizeClassAllocator::operator=( SizeClassAllocator && other ) noexcept
{
 if( this != &other ) {
  release();
  f_pages = std::exchange( other.f_pages , nullptr );
  f_bytes = std::exchange( other.f_bytes , 0 );
  for( unsigned c = 0 ; c < CLASSES ; ++c ) {
   f_free[ c ] = std::exchange( other.f_free[ c ] , nullptr );
   f_next[ c ] = std::exchange( other.f_next[ c ] , nullptr );
   f_end[ c ] = std::exchange( other.f_end[ c ] , nullptr );
   }
  }
 return( *this );
 }

inline void * SizeClassAllocator::fresh( unsigned c )
{
 const std::size_t s = classSize( c );
 const std::size_t h = 16;                  // the header, aligned
 const std::size_t b = s + h > PAGE ? s + h : PAGE;
 char * m = static_cast< char * >( ::operator new( b ) );
 f_pages = ::new( static_cast< void * >( m ) ) Page{ f_pages , b };
 f_bytes += b;
 f_next[ c ] = m + h + s;                   // the first block given now
 f_end[ c ] = m + b;
 return( m + h );
 }

inline void * SizeClassAllocator::allocate( std::size_t n )
{
 ADS_PRE( n >= 1 );
 if( n > LARGEST )
  return( ::operator new( n ) );
 const unsigned c = sizeClass( n );
 if( Free * f = f_free[ c ] ) {             // the block given back last
  f_free[ c ] = f->next;
  return( f );
  }
 const std::size_t s = classSize( c );
 if( std::size_t( f_end[ c ] - f_next[ c ] ) >= s ) {   // the next one of
  void * p = f_next[ c ];                               // the page
  f_next[ c ] += s;
  return( p );
  }
 return( fresh( c ) );
 }

inline void SizeClassAllocator::deallocate( void * p , std::size_t n )
 noexcept
{
 if( n > LARGEST ) {
  ::operator delete( p );
  return;
  }
 const unsigned c = sizeClass( n );
 Free * f = static_cast< Free * >( p );
 f->next = f_free[ c ];
 f_free[ c ] = f;
 }

inline void SizeClassAllocator::release( void ) noexcept
{
 while( Page * g = f_pages ) {
  f_pages = g->prev;
  ::operator delete( static_cast< void * >( g ) );
  }
 for( unsigned c = 0 ; c < CLASSES ; ++c ) {
  f_free[ c ] = nullptr;
  f_next[ c ] = f_end[ c ] = nullptr;
  }
 f_bytes = 0;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File SizeClassAllocator.tpp --------------------*/
/*--------------------------------------------------------------------------*/
