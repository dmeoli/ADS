/*--------------------------------------------------------------------------*/
/*------------------------ File IndexedDaryHeap.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of IndexedDaryHeap [see IndexedDaryHeap.h], with their
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

template< class K , unsigned D , class P >
template< class... O >
IndexedDaryHeap< K , D , P >::IndexedDaryHeap( int n , O &... o )
    : f_sub( o... )
{
 ADS_PRE( n >= 0 );
 f_pos.assign( std::size_t( n ) , -1 );
 }

template< class K , unsigned D , class P >
void IndexedDaryHeap< K , D , P >::reset( int n )
{
 ADS_PRE( n >= 0 );
 f_A.clear();
 f_pos.assign( std::size_t( n ) , -1 );
 if constexpr( heard< mod::Cleared > )
  if( alive< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class K , unsigned D , class P >
void IndexedDaryHeap< K , D , P >::grow( int n )
{
 ADS_PRE( n >= ids() );
 const auto m = std::size_t( n );
 if( m > f_pos.capacity() )  // doubled, whatever the library
  f_pos.reserve( std::max( m , 2 * f_pos.capacity() ) );
 f_pos.resize( m , -1 );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
inline bool IndexedDaryHeap< K , D , P >::contains( int id ) const
{
 ADS_PRE( ( id >= 0 ) && ( id < ids() ) );
 return( f_pos[ id ] >= 0 );
 }

template< class K , unsigned D , class P >
inline const K & IndexedDaryHeap< K , D , P >::key( int id ) const
{
 ADS_PRE( contains( id ) );
 return( f_A[ f_pos[ id ] ].key );
 }

template< class K , unsigned D , class P >
inline void IndexedDaryHeap< K , D , P >::push( int id , const K & k )
{
 ADS_PRE( ! contains( id ) );
 if constexpr( heard< Ins , Rel > )
  if( alive< Ins , Rel >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    pushCold( id , k );
   else
    pushTold( id , k );
   return;
   }
 f_A.push_back( Entry{ k , id } );
 up( f_A.size() - 1 , Entry{ k , id } );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
inline void IndexedDaryHeap< K , D , P >::decrease( int id , const K & k )
{
 ADS_PRE( contains( id ) && ! ( f_A[ f_pos[ id ] ].key < k ) );
 if constexpr( heard< Chg , Rel > )
  if( alive< Chg , Rel >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    decreaseCold( id , k );
   else
    decreaseTold( id , k );
   return;
   }
 up( std::size_t( f_pos[ id ] ) , Entry{ k , id } );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
inline void IndexedDaryHeap< K , D , P >::increase( int id , const K & k )
{
 ADS_PRE( contains( id ) && ! ( k < f_A[ f_pos[ id ] ].key ) );
 if constexpr( heard< Chg , Rel > )
  if( alive< Chg , Rel >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    increaseCold( id , k );
   else
    increaseTold( id , k );
   return;
   }
 down( std::size_t( f_pos[ id ] ) , Entry{ k , id } );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
inline void IndexedDaryHeap< K , D , P >::update( int id , const K & k )
{
 ADS_PRE( contains( id ) );
 if( k < f_A[ f_pos[ id ] ].key )
  decrease( id , k );
 else
  increase( id , k );
 }

template< class K , unsigned D , class P >
inline void IndexedDaryHeap< K , D , P >::erase( int id )
{
 ADS_PRE( contains( id ) );
 if constexpr( heard< Era , Rel > )
  if( alive< Era , Rel >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    eraseCold( id );
   else
    eraseTold( id );
   return;
   }
 const std::size_t i = std::size_t( f_pos[ id ] );
 f_pos[ id ] = -1;
 const Entry x = f_A.back();
 f_A.pop_back();
 if( i < f_A.size() ) {  // the last item into the hole
  if( ( i > 0 ) && ( x.key < f_A[ ( i - 1 ) / D ].key ) )
   up( i , x );
  else
   down( i , x );
  }
 ADS_POST( ! contains( id ) );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
inline bool IndexedDaryHeap< K , D , P >::pushOrDecrease( int id ,
							  const K & k )
{
 ADS_PRE( ( id >= 0 ) && ( id < ids() ) );
 const int i = f_pos[ id ];
 if( i < 0 )
  push( id , k );
 else if( k < f_A[ i ].key )
  decrease( id , k );
 else
  return( false );
 return( true );
 }

template< class K , unsigned D , class P >
inline const typename IndexedDaryHeap< K , D , P >::Entry &
IndexedDaryHeap< K , D , P >::top( void ) const
{
 ADS_PRE( ! f_A.empty() );
 return( f_A.front() );
 }

template< class K , unsigned D , class P >
inline typename IndexedDaryHeap< K , D , P >::Entry
IndexedDaryHeap< K , D , P >::pop( void )
{
 ADS_PRE( ! f_A.empty() );
 if constexpr( heard< Era , Rel > )
  if( alive< Era , Rel >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( popCold() );
   else
    return( popTold() );
   }
 const Entry x = f_A.front();
 f_pos[ x.id ] = -1;
 const Entry y = f_A.back();
 f_A.pop_back();
 if( ! f_A.empty() )
  down( 0 , y );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class K , unsigned D , class P >
void IndexedDaryHeap< K , D , P >::clear( void )
{
 for( const Entry & e : f_A )
  f_pos[ e.id ] = -1;
 f_A.clear();
 if constexpr( heard< mod::Cleared > )
  if( alive< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class K , unsigned D , class P >
template< bool T >
inline void
IndexedDaryHeap< K , D , P >::up( std::size_t i , Entry x ,
				[[maybe_unused]] std::size_t from )
{
 if constexpr( T )
  if( from != i )
   f_sub.emit( Rel{ from , i } );  // x into the hole
 while( i > 0 ) {
  const std::size_t p = ( i - 1 ) / D;
  if( ! ( x.key < f_A[ p ].key ) )
   break;
  f_A[ i ] = f_A[ p ];
  f_pos[ f_A[ i ].id ] = int( i );
  if constexpr( T )
   f_sub.emit( Rel{ i , p } );  // x and its parent exchanged
  i = p;
  }
 f_A[ i ] = x;
 f_pos[ x.id ] = int( i );
 }

template< class K , unsigned D , class P >
template< bool T >
inline void
IndexedDaryHeap< K , D , P >::down( std::size_t i , Entry x ,
				  [[maybe_unused]] std::size_t from )
{
 if constexpr( T )
  if( from != i )
   f_sub.emit( Rel{ from , i } );  // x into the hole
 const std::size_t n = f_A.size();
 for( ; ; ) {
  const std::size_t c = D * i + 1;
  if( c >= n )
   break;
  std::size_t b = c;  // the least child
  if( c + D <= n )    // all D children: no bound
   for( std::size_t k = c + 1 ; k < c + D ; ++k )
    b = f_A[ k ].key < f_A[ b ].key ? k : b;
  else
   for( std::size_t k = c + 1 ; k < n ; ++k )
    b = f_A[ k ].key < f_A[ b ].key ? k : b;
  if( ! ( f_A[ b ].key < x.key ) )
   break;
  f_A[ i ] = f_A[ b ];
  f_pos[ f_A[ i ].id ] = int( i );
  if constexpr( T )
   f_sub.emit( Rel{ i , b } );  // x and its child exchanged
  i = b;
  }
 f_A[ i ] = x;
 f_pos[ x.id ] = int( i );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- WHEN SOMEONE LISTENS ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , unsigned D , class P >
void IndexedDaryHeap< K , D , P >::pushTold( int id , const K & k )
{
 const std::size_t s = f_A.size();  // the place of the new item
 f_A.push_back( Entry{ k , id } );
 f_pos[ id ] = int( s );
 if( alive< Ins >() )
  f_sub.emit( Ins{ id , k } );
 if( alive< Rel >() )
  up< true >( s , Entry{ k , id } , s );
 else
  up( s , Entry{ k , id } );
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
void IndexedDaryHeap< K , D , P >::decreaseTold( int id , const K & k )
{
 const std::size_t i = std::size_t( f_pos[ id ] );
 if( alive< Chg >() )
  f_sub.emit( Chg{ id , f_A[ i ].key , k } );
 if( alive< Rel >() )
  up< true >( i , Entry{ k , id } , i );
 else
  up( i , Entry{ k , id } );
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
void IndexedDaryHeap< K , D , P >::increaseTold( int id , const K & k )
{
 const std::size_t i = std::size_t( f_pos[ id ] );
 if( alive< Chg >() )
  f_sub.emit( Chg{ id , f_A[ i ].key , k } );
 if( alive< Rel >() )
  down< true >( i , Entry{ k , id } , i );
 else
  down( i , Entry{ k , id } );
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
void IndexedDaryHeap< K , D , P >::eraseTold( int id )
{
 const std::size_t i = std::size_t( f_pos[ id ] );
 if( alive< Era >() )
  f_sub.emit( Era{ id , f_A[ i ].key } );  // before the removal
 f_pos[ id ] = -1;
 const Entry x = f_A.back();
 f_A.pop_back();
 const std::size_t s = f_A.size();  // the place of x before
 if( i < s ) {
  const bool rel = alive< Rel >();
  if( ( i > 0 ) && ( x.key < f_A[ ( i - 1 ) / D ].key ) ) {
   if( rel )
    up< true >( i , x , s );
   else
    up( i , x );
   }
  else if( rel )
   down< true >( i , x , s );
  else
   down( i , x );
  }
 f_sub.audit( *this );
 ADS_POST( ! contains( id ) );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D , class P >
typename IndexedDaryHeap< K , D , P >::Entry
IndexedDaryHeap< K , D , P >::popTold( void )
{
 const Entry x = f_A.front();
 if( alive< Era >() )
  f_sub.emit( Era{ x.id , x.key } );  // before the removal
 f_pos[ x.id ] = -1;
 const Entry y = f_A.back();
 f_A.pop_back();
 if( ! f_A.empty() ) {
  if( alive< Rel >() )
   down< true >( 0 , y , f_A.size() );  // y from the last place
  else
   down( 0 , y );
  }
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class K , unsigned D , class P >
bool IndexedDaryHeap< K , D , P >::ordered( void ) const
{
 const std::size_t n = f_A.size();
 if( n > 1024 ) {  // the root and the last item
  for( std::size_t i : { std::size_t( 0 ) , n - 1 } )
   if( f_pos[ f_A[ i ].id ] != int( i ) )
    return( false );
  for( std::size_t i = 1 ; ( i <= D ) && ( i < n ) ; ++i )
   if( f_A[ i ].key < f_A[ 0 ].key )
    return( false );
  return( ! ( f_A[ n - 1 ].key < f_A[ ( n - 2 ) / D ].key ) );
  }
 for( std::size_t i = 0 ; i < f_A.size() ; ++i )
  if( ( f_pos[ f_A[ i ].id ] != int( i ) ) ||
      ( ( i > 0 ) && ( f_A[ i ].key < f_A[ ( i - 1 ) / D ].key ) ) )
   return( false );
 std::size_t in = 0;
 for( int p : f_pos )
  in += p >= 0;
 return( in == f_A.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File IndexedDaryHeap.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
