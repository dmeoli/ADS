/*--------------------------------------------------------------------------*/
/*-------------------------- File RecordFile.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of RecordFile, RecordWriter and TempDirectory [see
 * RecordFile.h], with their contracts.
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

#include <atomic>
#include <chrono>
#include <fstream>
#include <random>
#include <stdexcept>
#include <system_error>
#include <utility>

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS RecordFile ----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline RecordFile< T >::RecordFile( std::filesystem::path path )
 : f_path( std::move( path ) ) {}

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline const std::filesystem::path & RecordFile< T >::path( void ) const
{
 return( f_path );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline std::size_t RecordFile< T >::size( void ) const
{
 std::error_code ec;
 const auto bytes = std::filesystem::file_size( f_path , ec );
 return( ec ? 0 : std::size_t( bytes / sizeof( T ) ) );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline bool RecordFile< T >::empty( void ) const
{
 return( size() == 0 );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline void RecordFile< T >::append( const T * src , std::size_t n )
{
 ADS_PRE( src || ( n == 0 ) );
 // the size before, read only when the postconditions are checked
 [[maybe_unused]] const std::size_t before = ADS_CONTRACTS >= 2 ? size()
                                                                : 0;
 std::ofstream out( f_path , std::ios::binary | std::ios::app );
 if( n )
  out.write( reinterpret_cast< const char * >( src ) ,
             std::streamsize( n * sizeof( T ) ) );
 out.close();
 if( ! out )
  throw( std::runtime_error( "ADS::RecordFile::append: cannot write " +
                             f_path.string() ) );
 ADS_POST( size() == before + n );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
template< class F >
inline void RecordFile< T >::forEach( F && f , std::size_t buffer ) const
{
 ADS_PRE( buffer >= 1 );
 std::error_code ec;
 if( ! std::filesystem::exists( f_path , ec ) )
  return;                                   // no file, no record
 std::ifstream in( f_path , std::ios::binary );
 if( ! in )
  throw( std::runtime_error( "ADS::RecordFile::forEach: cannot read " +
                             f_path.string() ) );
 std::vector< T > buf( buffer );
 for( std::size_t left = size() ; left ; ) {
  const std::size_t k = left < buffer ? left : buffer;
  in.read( reinterpret_cast< char * >( buf.data() ) ,
           std::streamsize( k * sizeof( T ) ) );
  if( std::size_t( in.gcount() ) != k * sizeof( T ) )
   throw( std::runtime_error( "ADS::RecordFile::forEach: short read of " +
                              f_path.string() ) );
  for( std::size_t i = 0 ; i < k ; ++i )
   f( std::as_const( buf[ i ] ) );
  left -= k;
  }
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline std::vector< T > RecordFile< T >::readAll( void ) const
{
 std::vector< T > v;
 v.reserve( size() );
 forEach( [ & ]( const T & x ) { v.push_back( x ); } );
 return( v );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline void RecordFile< T >::clear( void )
{
 std::ofstream out( f_path , std::ios::binary | std::ios::trunc );
 if( ! out )
  throw( std::runtime_error( "ADS::RecordFile::clear: cannot write " +
                             f_path.string() ) );
 out.close();
 ADS_POST( empty() );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline void RecordFile< T >::remove( void )
{
 std::error_code ec;
 std::filesystem::remove( f_path , ec );
 if( ec )
  throw( std::runtime_error( "ADS::RecordFile::remove: cannot remove " +
                             f_path.string() ) );
 ADS_POST( empty() );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS RecordWriter ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline RecordWriter< T >::RecordWriter( RecordFile< T > & file ,
                                        std::size_t buffer )
 : f_file( &file ) , f_cap( buffer )
{
 ADS_PRE( buffer >= 1 );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline RecordWriter< T >::~RecordWriter()
{
 try {
  if( f_file )
   flush();
  }
 catch( ... ) {}            // a destructor does not throw
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline void RecordWriter< T >::push( const T & x )
{
 if( f_buf.capacity() < f_cap )
  f_buf.reserve( f_cap );                  // the buffer at the first push
 f_buf.push_back( x );
 if( f_buf.size() >= f_cap )
  flush();
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline void RecordWriter< T >::flush( void )
{
 if( ! f_buf.empty() ) {
  f_file->append( f_buf.data() , f_buf.size() );
  f_buf.clear();
  }
 ADS_POST( pending() == 0 );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline std::size_t RecordWriter< T >::pending( void ) const
{
 return( f_buf.size() );
 }

/*--------------------------------------------------------------------------*/

template< class T >
 requires std::is_trivially_copyable_v< T >
inline RecordFile< T > & RecordWriter< T >::file( void ) const
{
 return( *f_file );
 }

/*--------------------------------------------------------------------------*/
/*-------------------------- CLASS TempDirectory ---------------------------*/
/*--------------------------------------------------------------------------*/

inline TempDirectory::TempDirectory( const std::filesystem::path & parent ,
                                     const std::string & prefix )
{
 static std::atomic< std::uint64_t > counter{ 0 };
 const auto base = parent.empty() ? std::filesystem::temp_directory_path()
                                  : parent;
 std::filesystem::create_directories( base );
 std::mt19937_64 rng( std::random_device{}() ^
                      std::uint64_t( std::chrono::steady_clock::now()
                                      .time_since_epoch().count() ) );
 for( int attempt = 0 ; attempt < 100 ; ++attempt ) {
  const auto name = prefix + std::to_string( rng() ^ counter++ );
  std::error_code ec;
  if( std::filesystem::create_directory( base / name , ec ) ) {
   f_path = base / name;
   return;
   }
  }
 throw( std::runtime_error( "ADS::TempDirectory: cannot make a directory"
                            " in " + base.string() ) );
 }

/*--------------------------------------------------------------------------*/

inline TempDirectory::TempDirectory( TempDirectory && other ) noexcept
 : f_path( std::exchange( other.f_path , {} ) ) {}

/*--------------------------------------------------------------------------*/

inline TempDirectory::~TempDirectory()
{
 if( ! f_path.empty() ) {
  std::error_code ec;
  std::filesystem::remove_all( f_path , ec );
  }
 }

/*--------------------------------------------------------------------------*/

inline const std::filesystem::path & TempDirectory::path( void ) const
{
 return( f_path );
 }

/*--------------------------------------------------------------------------*/

inline std::filesystem::path TempDirectory::operator/(
 const std::string & name ) const
{
 return( f_path / name );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File RecordFile.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
