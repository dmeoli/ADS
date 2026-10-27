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

namespace ADS {

template< class K , unsigned D >
IndexedDaryHeap< K , D >::IndexedDaryHeap( int n )
{
 reset( n );
 }

template< class K , unsigned D >
void IndexedDaryHeap< K , D >::reset( int n )
{
 ADS_PRE( n >= 0 );
 f_A.clear();
 f_pos.assign( std::size_t( n < 0 ? 0 : n ) , -1 );
 }

template< class K , unsigned D >
inline bool IndexedDaryHeap< K , D >::contains( int id ) const
{
 ADS_PRE( ( id >= 0 ) && ( id < ids() ) );
 return( f_pos[ id ] >= 0 );
 }

template< class K , unsigned D >
inline const K & IndexedDaryHeap< K , D >::key( int id ) const
{
 ADS_PRE( contains( id ) );
 return( f_A[ f_pos[ id ] ].key );
 }

template< class K , unsigned D >
inline void IndexedDaryHeap< K , D >::push( int id , const K & k )
{
 ADS_PRE( ! contains( id ) );
 f_A.push_back( Entry{ k , id } );
 up( f_A.size() - 1 , Entry{ k , id } );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D >
inline void IndexedDaryHeap< K , D >::decrease( int id , const K & k )
{
 ADS_PRE( contains( id ) && !( f_A[ f_pos[ id ] ].key < k ) );
 up( std::size_t( f_pos[ id ] ) , Entry{ k , id } );
 ADS_INVARIANT( ordered() );
 }

template< class K , unsigned D >
inline bool IndexedDaryHeap< K , D >::pushOrDecrease( int id , const K & k )
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

template< class K , unsigned D >
inline const typename IndexedDaryHeap< K , D >::Entry &
IndexedDaryHeap< K , D >::top( void ) const
{
 ADS_PRE( ! f_A.empty() );
 return( f_A.front() );
 }

template< class K , unsigned D >
inline typename IndexedDaryHeap< K , D >::Entry
IndexedDaryHeap< K , D >::pop( void )
{
 ADS_PRE( ! f_A.empty() );
 const Entry x = f_A.front();
 f_pos[ x.id ] = -1;
 const Entry y = f_A.back();
 f_A.pop_back();
 if( ! f_A.empty() )
  down( 0 , y );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class K , unsigned D >
void IndexedDaryHeap< K , D >::clear( void )
{
 for( const Entry & e : f_A )
  f_pos[ e.id ] = -1;
 f_A.clear();
 }

template< class K , unsigned D >
inline void IndexedDaryHeap< K , D >::up( std::size_t i , Entry x )
{
 while( i > 0 ) {
  const std::size_t p = ( i - 1 ) / D;
  if( !( x.key < f_A[ p ].key ) )
   break;
  f_A[ i ] = f_A[ p ];
  f_pos[ f_A[ i ].id ] = int( i );
  i = p;
  }
 f_A[ i ] = x;
 f_pos[ x.id ] = int( i );
 }

template< class K , unsigned D >
inline void IndexedDaryHeap< K , D >::down( std::size_t i , Entry x )
{
 const std::size_t n = f_A.size();
 for( ; ; ) {
  const std::size_t c = D * i + 1;
  if( c >= n )
   break;
  std::size_t b = c;                        // the least child
  if( c + D <= n )                          // all D children: no bound
   for( std::size_t k = c + 1 ; k < c + D ; ++k )
    b = f_A[ k ].key < f_A[ b ].key ? k : b;
  else
   for( std::size_t k = c + 1 ; k < n ; ++k )
    b = f_A[ k ].key < f_A[ b ].key ? k : b;
  if( !( f_A[ b ].key < x.key ) )
   break;
  f_A[ i ] = f_A[ b ];
  f_pos[ f_A[ i ].id ] = int( i );
  i = b;
  }
 f_A[ i ] = x;
 f_pos[ x.id ] = int( i );
 }

template< class K , unsigned D >
bool IndexedDaryHeap< K , D >::ordered( void ) const
{
 const std::size_t n = f_A.size();
 if( n > 1024 ) {                           // the root and the last item
  for( std::size_t i : { std::size_t( 0 ) , n - 1 } )
   if( f_pos[ f_A[ i ].id ] != int( i ) )
    return( false );
  for( std::size_t i = 1 ; ( i <= D ) && ( i < n ) ; ++i )
   if( f_A[ i ].key < f_A[ 0 ].key )
    return( false );
  return( !( f_A[ n - 1 ].key < f_A[ ( n - 2 ) / D ].key ) );
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
