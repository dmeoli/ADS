/*--------------------------------------------------------------------------*/
/*-------------------------- File MappedFile.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MappedFile [see MappedFile.h], with their contracts.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <cerrno>
#include <fstream>
#include <new>
#include <string>
#include <system_error>
#include <utility>

#if __has_include( <sys/mman.h> ) && __has_include( <unistd.h> )
 #include <fcntl.h>
 #include <sys/mman.h>
 #include <sys/stat.h>
 #include <unistd.h>
 #define ADS_MAPPED_FILE_POSIX 1
#endif

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MappedFile ----------------------------*/
/*--------------------------------------------------------------------------*/

inline MappedFile::MappedFile( const std::filesystem::path & p )
{
 const auto fail = [ & ]( const char * what , int e ) {
  throw( PersistError( "ADS: " + p.string() + ": " + what + ": " +
		       std::generic_category().message( e ) ) );
  };
 auto * r = new Region;
#ifdef ADS_MAPPED_FILE_POSIX
 const int fd = ::open( p.c_str() , O_RDONLY );
 if( fd < 0 ) {
  delete r;
  fail( "cannot open the file" , errno );
  }
 struct stat st;
 if( ::fstat( fd , &st ) != 0 ) {
  const int e = errno;
  ::close( fd );
  delete r;
  fail( "cannot read the size of the file" , e );
  }
 r->size = std::size_t( st.st_size );
 if( r->size > 0 ) {
  void * m =
    ::mmap( nullptr , r->size , PROT_READ | PROT_WRITE , MAP_PRIVATE , fd ,
	    0 );
  if( m == MAP_FAILED ) {
   const int e = errno;
   ::close( fd );
   delete r;
   fail( "cannot map the file" , e );
   }
  r->base = static_cast< std::byte * >( m );
  r->system = true;
  }
 ::close( fd );  // the mapping outlives the descriptor
#else
 std::ifstream in( p , std::ios::binary | std::ios::ate );
 if( ! in ) {
  delete r;
  fail( "cannot open the file" , ENOENT );
  }
 r->size = std::size_t( in.tellg() );
 in.seekg( 0 );
 if( r->size > 0 ) {
  r->base = static_cast< std::byte * >(
    ::operator new( r->size , std::align_val_t( 64 ) ) );
  if( ! in.read( reinterpret_cast< char * >( r->base ) ,
		 std::streamsize( r->size ) ) ) {
   ::operator delete( r->base , std::align_val_t( 64 ) );
   delete r;
   fail( "cannot read the file" , EIO );
   }
  }
#endif
 f_r = r;
 }

/*--------------------------------------------------------------------------*/

inline MappedFile::MappedFile( const MappedFile & o ) noexcept : f_r( o.f_r )
{
 if( f_r )
  f_r->refs.fetch_add( 1 , std::memory_order_relaxed );
 }

/*--------------------------------------------------------------------------*/

inline MappedFile::MappedFile( MappedFile && o ) noexcept
    : f_r( std::exchange( o.f_r , nullptr ) )
{ }

/*--------------------------------------------------------------------------*/

inline MappedFile & MappedFile::operator=( const MappedFile & o ) noexcept
{
 if( f_r != o.f_r ) {
  if( o.f_r )
   o.f_r->refs.fetch_add( 1 , std::memory_order_relaxed );
  release();
  f_r = o.f_r;
  }
 return( *this );
 }

/*--------------------------------------------------------------------------*/

inline MappedFile & MappedFile::operator=( MappedFile && o ) noexcept
{
 if( this != &o ) {
  release();
  f_r = std::exchange( o.f_r , nullptr );
  }
 return( *this );
 }

/*--------------------------------------------------------------------------*/

inline MappedFile::~MappedFile( void )
{
 release();
 }

/*--------------------------------------------------------------------------*/

inline void MappedFile::release( void ) noexcept
{
 if( ( ! f_r ) ||
     ( f_r->refs.fetch_sub( 1 , std::memory_order_acq_rel ) != 1 ) ) {
  f_r = nullptr;
  return;
  }
 if( f_r->base ) {
#ifdef ADS_MAPPED_FILE_POSIX
  ::munmap( f_r->base , f_r->size );
#else
  ::operator delete( f_r->base , std::align_val_t( 64 ) );
#endif
  }
 delete f_r;
 f_r = nullptr;
 }

/*--------------------------------------------------------------------------*/

inline std::span< std::byte > MappedFile::bytes( void ) const
{
 if( ! f_r )
  return( std::span< std::byte >() );
 return( std::span< std::byte >( f_r->base , f_r->size ) );
 }

/*--------------------------------------------------------------------------*/

inline bool MappedFile::mapped( void ) const
{
 return( f_r && f_r->system );
 }

/*--------------------------------------------------------------------------*/

inline bool MappedFile::holds( const void * p ) const
{
 if( ( ! f_r ) || ( ! f_r->base ) )
  return( false );
 const auto * b = static_cast< const std::byte * >( p );
 return( ( b >= f_r->base ) && ( b < f_r->base + f_r->size ) );
 }

/*--------------------------------------------------------------------------*/

inline void MappedFile::offer( void * p , std::size_t m ) const
{
 ADS_PRE( f_r && holds( p ) );
 f_r->offered = p;
 f_r->offeredSize = m;
 }

/*--------------------------------------------------------------------------*/

inline void * MappedFile::take( std::size_t m ) const
{
 if( ( ! f_r ) || ( ! f_r->offered ) || ( f_r->offeredSize != m ) )
  return( nullptr );
 return( std::exchange( f_r->offered , nullptr ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File MappedFile.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
