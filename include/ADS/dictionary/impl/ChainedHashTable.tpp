/*--------------------------------------------------------------------------*/
/*----------------------- File ChainedHashTable.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of ChainedHashTable [see ChainedHashTable.h], with their
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

namespace ADS {

/*--------------------------------------------------------------------------*/
/*---------------------------- THE DICTIONARY ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
ChainedHashTable< K , V , C , H >::ChainedHashTable( std::size_t m ,
                                                     bool restructure , H h )
 : f_T( m ) , f_m0( m ) , f_restructure( restructure ) , f_h( h )
{
 ADS_PRE( m > 0 );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::optional< V >
ChainedHashTable< K , V , C , H >::lookup( const K & k ) const
{
 auto & L = f_T[ place( k ) ];
 auto p = find( L , k );
 if( L.finished( p ) || ! ( L.read( p ).first == k ) )
  return( std::nullopt );
 const V v = L.read( p ).second;
 if constexpr( C == Chain::MOVE_TO_FRONT )
  if( p != L.head() ) {
   auto kv = L.read( p );
   L.remove( p );
   L.insert( L.head() , kv );
   }
 ADS_INVARIANT( consistent( k ) );
 return( v );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
void ChainedHashTable< K , V , C , H >::insert( const K & k , const V & v )
{
 auto & L = f_T[ place( k ) ];
 auto p = find( L , k );
 if( ! L.finished( p ) && ( L.read( p ).first == k ) ) {
  L.write( p , { k , v } );
  return;
  }
 if constexpr( C == Chain::SORTED )
  L.insert( p , { k , v } );           // before the first larger key
 else
  L.insert( L.head() , { k , v } );
 ++f_size;
 if( f_restructure && ( f_size > 2 * f_T.size() ) )
  rebuild( resized( f_T.size() , true , f_m0 ) );
 ADS_POST( present( k ) );
 ADS_INVARIANT( consistent( k ) );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
void ChainedHashTable< K , V , C , H >::remove( const K & k )
{
 auto & L = f_T[ place( k ) ];
 auto p = find( L , k );
 if( L.finished( p ) || ! ( L.read( p ).first == k ) )
  return;
 L.remove( p );
 --f_size;
 if( f_restructure && ( f_T.size() > f_m0 ) && ( 2 * f_size < f_T.size() ) )
  rebuild( resized( f_T.size() , false , f_m0 ) );
 ADS_POST( ! present( k ) );
 ADS_INVARIANT( consistent( k ) );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
bool ChainedHashTable< K , V , C , H >::contains( const K & k ) const
{
 auto & L = f_T[ place( k ) ];
 auto p = find( L , k );
 return( ! L.finished( p ) && ( L.read( p ).first == k ) );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::size_t ChainedHashTable< K , V , C , H >::size( void ) const
{
 return( f_size );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
bool ChainedHashTable< K , V , C , H >::empty( void ) const
{
 return( f_size == 0 );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::size_t ChainedHashTable< K , V , C , H >::capacity( void ) const
{
 return( f_T.size() );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
double ChainedHashTable< K , V , C , H >::load( void ) const
{
 return( double( f_size ) / f_T.size() );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
template< class F >
void ChainedHashTable< K , V , C , H >::forEach( F && f ) const
{
 for( const auto & L : f_T )
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
   f( L.read( p ).first , L.read( p ).second );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::vector< K >
ChainedHashTable< K , V , C , H >::chain( std::size_t i ) const
{
 ADS_PRE( i < f_T.size() );
 std::vector< K > out;
 const auto & L = f_T[ i ];
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  out.push_back( L.read( p ).first );
 return( out );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::size_t ChainedHashTable< K , V , C , H >::probes( void ) const
{
 return( f_probes );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::size_t ChainedHashTable< K , V , C , H >::restructures( void ) const
{
 return( f_rebuilds );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::size_t ChainedHashTable< K , V , C , H >::moved( void ) const
{
 return( f_moved );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE LISTS ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
std::size_t ChainedHashTable< K , V , C , H >::place( const K & k ) const
{
 return( f_h( k ) % f_T.size() );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
typename ChainedHashTable< K , V , C , H >::Pos
ChainedHashTable< K , V , C , H >::find( const Bucket & L ,
                                         const K & k ) const
{
 f_probes = 1;
 auto p = L.head();
 for( ; ! L.finished( p ) ; p = L.next( p ) , ++f_probes ) {
  if( L.read( p ).first == k )
   break;
  if constexpr( C == Chain::SORTED )
   if( k < L.read( p ).first )
    break;
  }
 return( p );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
bool ChainedHashTable< K , V , C , H >::present( const K & k ) const
{
 const auto & L = f_T[ place( k ) ];
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
  if( L.read( p ).first == k )
   return( true );
 return( false );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
bool
ChainedHashTable< K , V , C , H >::listOk( std::size_t i , const K & k ,
                                           std::size_t & n ) const
{
 const auto & L = f_T[ i ];
 int copies = 0;
 for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) , ++n ) {
  const K & x = L.read( p ).first;
  if( place( x ) != i )
   return( false );
  copies += x == k;
  if constexpr( C == Chain::SORTED ) {
   const auto q = L.next( p );
   if( ! L.finished( q ) && !( x < L.read( q ).first ) )
    return( false );
   }
  }
 return( copies <= 1 );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
bool ChainedHashTable< K , V , C , H >::consistent( const K & k ) const
{
 std::size_t n = 0;
 if( f_T.size() > 1024 )
  return( listOk( place( k ) , k , n ) );
 for( std::size_t i = 0 ; i < f_T.size() ; ++i )
  if( ! listOk( i , k , n ) )
   return( false );
 return( n == f_size );
 }

template< class K , class V , Chain C , class H >
 requires( C != Chain::SORTED || std::totally_ordered< K > )
void ChainedHashTable< K , V , C , H >::rebuild( std::size_t m )
{
 std::vector< Bucket > T( m );
 std::swap( f_T , T );
 for( auto & L : T )
  for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) ) {
   const auto & kv = L.read( p );
   auto & M = f_T[ place( kv.first ) ];
   if constexpr( C == Chain::SORTED )
    M.insert( find( M , kv.first ) , kv );
   else
    M.insert( M.head() , kv );
   }
 ++f_rebuilds;
 f_moved += f_size;
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File ChainedHashTable.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
