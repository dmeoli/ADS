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

#ifdef ADS_WITH_BOOST
 #include <boost/interprocess/file_mapping.hpp>
 #include <boost/interprocess/mapped_region.hpp>
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
 std::error_code ec;
 const auto n = std::filesystem::file_size( p , ec );
 if( ec )
  fail( "cannot open the file" , ec.value() );
 auto * r = new Region;
 r->size = std::size_t( n );
 if( r->size > 0 ) {
#ifdef ADS_WITH_BOOST
  namespace bi = boost::interprocess;
  try {  // the mapping outlives the handle of the file
   const bi::file_mapping f( p.string().c_str() , bi::read_only );
   auto * m = new bi::mapped_region( f , bi::copy_on_write );
   r->base = static_cast< std::byte * >( m->get_address() );
   r->system = true;
   r->handle = m;
   r->drop = []( Region & q ) {
    delete static_cast< bi::mapped_region * >( q.handle );
    };
   }
  catch( const bi::interprocess_exception & e ) {
   delete r;
   throw( PersistError( "ADS: " + p.string() +
			": cannot map the file: " + e.what() ) );
   }
#else
  std::ifstream in( p , std::ios::binary );
  r->base = static_cast< std::byte * >(
    ::operator new( r->size , std::align_val_t( kPage ) ) );
  r->drop = []( Region & q ) {
   ::operator delete( q.base , std::align_val_t( kPage ) );
   };
  if( ! in.read( reinterpret_cast< char * >( r->base ) ,
		 std::streamsize( r->size ) ) ) {
   r->drop( *r );
   delete r;
   fail( "cannot read the file" , EIO );
   }
#endif
  }
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
 if( f_r->drop )
  f_r->drop( *f_r );
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
