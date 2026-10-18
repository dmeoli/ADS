/*--------------------------------------------------------------------------*/
/*------------------------ File TwoLevelMemory.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of TwoLevelMemory, MemoryLease, DiskFile, DiskReader and
 * DiskWriter [see TwoLevelMemory.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS TwoLevelMemory ---------------------------*/
/*--------------------------------------------------------------------------*/

inline TwoLevelMemory::TwoLevelMemory( std::size_t B , std::size_t M )
 : f_B( B ) , f_M( M )
{
 ADS_PRE( ( B >= 1 ) && ( M >= 2 * B ) );
 }

inline std::size_t TwoLevelMemory::blockSize( void ) const
{
 return( f_B );
 }

inline std::size_t TwoLevelMemory::memorySize( void ) const
{
 return( f_M );
 }

inline std::size_t TwoLevelMemory::blocks( std::size_t n ) const
{
 return( ( n + f_B - 1 ) / f_B );
 }

inline std::uint64_t TwoLevelMemory::reads( void ) const
{
 return( f_reads );
 }

inline std::uint64_t TwoLevelMemory::writes( void ) const
{
 return( f_writes );
 }

inline std::uint64_t TwoLevelMemory::ios( void ) const
{
 return( f_reads + f_writes );
 }

inline void TwoLevelMemory::resetCounters( void )
{
 f_reads = f_writes = 0;
 f_peak = f_used;
 }

inline void TwoLevelMemory::allocate( std::size_t k )
{
 ADS_PRE( f_used + k <= f_M );
 f_used += k;
 f_peak = std::max( f_peak , f_used );
 }

inline void TwoLevelMemory::release( std::size_t k )
{
 ADS_PRE( k <= f_used );
 f_used -= k;
 }

inline std::size_t TwoLevelMemory::used( void ) const
{
 return( f_used );
 }

inline std::size_t TwoLevelMemory::peak( void ) const
{
 return( f_peak );
 }

inline void TwoLevelMemory::countRead( void )
{
 ++f_reads;
 }

inline void TwoLevelMemory::countWrite( void )
{
 ++f_writes;
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS MemoryLease ----------------------------*/
/*--------------------------------------------------------------------------*/

inline MemoryLease::MemoryLease( TwoLevelMemory & mem , std::size_t k )
 : f_mem( &mem ) , f_k( k )
{
 mem.allocate( k );
 }

inline MemoryLease::~MemoryLease()
{
 f_mem->release( f_k );
 }

inline std::size_t MemoryLease::size( void ) const
{
 return( f_k );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS DiskFile ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
DiskFile< T >::DiskFile( TwoLevelMemory & mem ) : f_mem( &mem )
{
 }

template< class T >
DiskFile< T >::DiskFile( TwoLevelMemory & mem , std::vector< T > items )
 : f_mem( &mem ) , f_items( std::move( items ) )
{
 }

template< class T >
DiskFile< T > & DiskFile< T >::operator=( DiskFile && other )
{
 ADS_PRE( f_mem == other.f_mem );
 f_items = std::move( other.f_items );
 other.f_items.clear();
 return( *this );
 }

template< class T >
TwoLevelMemory & DiskFile< T >::memory( void ) const
{
 return( *f_mem );
 }

template< class T >
std::size_t DiskFile< T >::size( void ) const
{
 return( f_items.size() );
 }

template< class T >
bool DiskFile< T >::isEmpty( void ) const
{
 return( f_items.empty() );
 }

template< class T >
std::size_t DiskFile< T >::blocks( void ) const
{
 return( f_mem->blocks( f_items.size() ) );
 }

template< class T >
std::size_t DiskFile< T >::readBlock( std::size_t b , T * dest ) const
{
 ADS_PRE( b < blocks() );
 const std::size_t B = f_mem->blockSize() , lo = b * B ,
                   hi = std::min( lo + B , f_items.size() );
 std::copy( f_items.begin() + std::ptrdiff_t( lo ) ,
            f_items.begin() + std::ptrdiff_t( hi ) , dest );
 f_mem->countRead();
 return( hi - lo );
 }

template< class T >
void DiskFile< T >::appendBlock( const T * src , std::size_t n )
{
 const std::size_t B = f_mem->blockSize();
 ADS_PRE( ( n >= 1 ) && ( n <= B ) && ( f_items.size() % B == 0 ) );
 f_items.insert( f_items.end() , src , src + n );
 f_mem->countWrite();
 }

template< class T >
void DiskFile< T >::clear( void )
{
 f_items.clear();
 }

template< class T >
const std::vector< T > & DiskFile< T >::contents( void ) const
{
 return( f_items );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DiskReader -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
DiskReader< T >::DiskReader( const DiskFile< T > & f )
 : f_file( &f ) , f_lease( f.memory() , f.memory().blockSize() ) ,
   f_buf( f.memory().blockSize() ) , f_block( std::size_t( -1 ) )
{
 }

template< class T >
void DiskReader< T >::load( std::size_t b )
{
 if( b != f_block ) {
  f_file->readBlock( b , f_buf.data() );
  f_block = b;
  }
 }

template< class T >
bool DiskReader< T >::hasNext( void ) const
{
 return( f_pos < f_file->size() );
 }

template< class T >
const T & DiskReader< T >::peek( void )
{
 ADS_PRE( hasNext() );
 const std::size_t B = f_buf.size();
 load( f_pos / B );
 return( f_buf[ f_pos % B ] );
 }

template< class T >
T DiskReader< T >::next( void )
{
 T x = peek();
 ++f_pos;
 return( x );
 }

template< class T >
std::size_t DiskReader< T >::position( void ) const
{
 return( f_pos );
 }

template< class T >
const T & DiskReader< T >::at( std::size_t i )
{
 ADS_PRE( i < f_file->size() );
 const std::size_t B = f_buf.size();
 load( i / B );
 f_pos = i + 1;
 return( f_buf[ i % B ] );
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS DiskWriter -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class T >
DiskWriter< T >::DiskWriter( DiskFile< T > & f )
 : f_file( &f ) , f_lease( f.memory() , f.memory().blockSize() )
{
 ADS_PRE( f.size() % f.memory().blockSize() == 0 );
 f_buf.reserve( f.memory().blockSize() );
 }

template< class T >
DiskWriter< T >::~DiskWriter()
{
 close();
 }

template< class T >
void DiskWriter< T >::push( const T & x )
{
 ADS_PRE( ! f_closed );
 f_buf.push_back( x );
 if( f_buf.size() == f_file->memory().blockSize() ) {
  f_file->appendBlock( f_buf.data() , f_buf.size() );
  f_buf.clear();
  }
 }

template< class T >
void DiskWriter< T >::close( void )
{
 if( ! f_closed && ! f_buf.empty() )
  f_file->appendBlock( f_buf.data() , f_buf.size() );
 f_buf.clear();
 f_closed = true;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File TwoLevelMemory.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
