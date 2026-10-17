/*--------------------------------------------------------------------------*/
/*---------------------------- File Sorted.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The checks of the contracts of the sorts [see Sorted.h].
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

namespace detail {

inline bool & checking( void )
{
 thread_local bool on = false;
 return( on );
 }

}  // end( namespace detail )

inline bool checkingContract( void )
{
 return( detail::checking() );
 }

namespace detail {

template< class F >
bool contract( F && f )
{
 const bool was = checking();
 checking() = true;
 const bool ok = f();
 checking() = was;
 return( ok );
 }

template< class T >
std::vector< T > auditCopy( const std::vector< T > & A , std::size_t lo ,
                            std::size_t hi )
{
 if constexpr( ADS_CONTRACTS >= 3 ) {
  hi = std::min( hi , A.size() );
  return( std::vector< T >( A.begin() + lo , A.begin() + hi ) );
  }
 else
  return( std::vector< T >() );
 }

template< class T , class C >
bool samePermutation( const std::vector< T > & A ,
                      std::vector< T > before , C less ,
                      std::size_t lo , std::size_t hi )
{
 return( contract( [ & ]( void ) {
  hi = std::min( hi , A.size() );
  if( before.size() != hi - lo )
   return( false );
  std::vector< T > after( A.begin() + lo , A.begin() + hi );
  std::sort( before.begin() , before.end() , less );
  std::sort( after.begin() , after.end() , less );
  for( std::size_t i = 0 ; i < after.size() ; ++i )
   if( less( before[ i ] , after[ i ] ) || less( after[ i ] , before[ i ] ) )
    return( false );
  return( true ); } ) );
 }

template< class T , class C >
bool sortedPermutation( const std::vector< T > & A ,
                        std::vector< T > before , C less ,
                        std::size_t lo , std::size_t hi )
{
 hi = std::min( hi , A.size() );
 return( contract( [ & ]( void ) {
  return( std::is_sorted( A.begin() + lo , A.begin() + hi , less ) ); } ) &&
         samePermutation( A , std::move( before ) , less , lo , hi ) );
 }

template< class S , class C >
bool sequenceSorted( const S & L , C less )
{
 return( contract( [ & ]( void ) {
  if( L.isEmpty() )
   return( true );
  for( auto p = L.head() , q = L.next( p ) ; ! L.finished( q ) ;
       p = q , q = L.next( q ) )
   if( less( L.read( q ) , L.read( p ) ) )
    return( false );
  return( true ); } ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Sorted.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
