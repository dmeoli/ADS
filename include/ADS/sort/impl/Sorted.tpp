/*--------------------------------------------------------------------------*/
/*---------------------------- File Sorted.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The checks of the contracts of the sorts [see Sorted.h].
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

namespace detail
{

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

namespace detail
{

template< class F >
bool contract( F && f )
{
 const bool was = checking();
 checking() = true;
 const bool ok = f();
 checking() = was;
 return( ok );
 }

template< std::ranges::random_access_range S >
std::vector< std::ranges::range_value_t< S > >
auditCopy( const S & A , std::size_t lo , std::size_t hi )
{
 using U = std::ranges::range_value_t< S >;
 if constexpr( ADS_CONTRACTS >= 3 ) {
  hi = std::min( hi , A.size() );
  return( std::vector< U >( A.begin() + lo , A.begin() + hi ) );
  }
 else
  return( std::vector< U >() );
 }

template< std::ranges::random_access_range S , class C >
bool samePermutation( const S & A ,
		      std::vector< std::ranges::range_value_t< S > > before ,
		      C less , std::size_t lo , std::size_t hi )
{
 using U = std::ranges::range_value_t< S >;
 return( contract( [ & ]( void ) {
  hi = std::min( hi , A.size() );
  if( before.size() != hi - lo )
   return( false );
  // an empty range holds the items of an empty copy, and no empty copy is
  // sorted: g++, inlining std::sort(), took on the path first == last the
  // end of the copy for its start and warned of a deletion at an offset
  if( hi == lo )
   return( true );
  std::vector< U > after( A.begin() + lo , A.begin() + hi );
  std::sort( before.begin() , before.end() , less );
  std::sort( after.begin() , after.end() , less );
  for( std::size_t i = 0 ; i < after.size() ; ++i )
   if( less( before[ i ] , after[ i ] ) || less( after[ i ] , before[ i ] ) )
    return( false );
  return( true );
  } ) );
 }

template< std::ranges::random_access_range S , class C >
bool sortedPermutation( const S & A ,
			std::vector< std::ranges::range_value_t< S > > before ,
			C less , std::size_t lo , std::size_t hi )
{
 hi = std::min( hi , A.size() );
 return( contract( [ & ]( void ) {
	   return( std::is_sorted( A.begin() + lo , A.begin() + hi , less ) );
	   } ) &&
	  samePermutation( A , std::move( before ) , less , lo , hi ) );
 }

template< class S , class C >
bool sequenceSorted( const S & L , C less )
{
 return( contract( [ & ]( void ) {
  if( L.empty() )
   return( true );
  for( auto p = L.head() , q = L.next( p ) ; ! L.finished( q ) ;
       p = q , q = L.next( q ) )
   if( less( L.read( q ) , L.read( p ) ) )
    return( false );
  return( true );
  } ) );
 }

template< class R , class F , class G >
auto sortRange( R & r , F && f , G && g )
{
 using T = std::ranges::range_value_t< R >;
 if constexpr( std::ranges::contiguous_range< R > ) {
  std::span< T > s( std::ranges::data( r ) , std::ranges::size( r ) );
  return( f( s ) );
  }
 else if constexpr( ! std::is_same_v< std::remove_cvref_t< G > , void * > ) {
  auto w = std::views::all( r );
  return( g( w ) );
  }
 else {
  std::vector< T > v;
  v.reserve( std::size_t( std::ranges::size( r ) ) );
  for( auto it = std::ranges::begin( r ) ; it != std::ranges::end( r ) ; ++it )
   v.push_back( std::ranges::iter_move( it ) );
  if constexpr( std::is_void_v< decltype( f( v ) ) > ) {
   f( v );
   std::ranges::move( v , std::ranges::begin( r ) );
   }
  else {
   auto x = f( v );
   std::ranges::move( v , std::ranges::begin( r ) );
   return( x );
   }
  }
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Sorted.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
