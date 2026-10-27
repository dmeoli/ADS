/*--------------------------------------------------------------------------*/
/*-------------------------- File BucketQueue.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BucketQueue [see BucketQueue.h], with their contracts.
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

template< class T , std::integral K >
inline void BucketQueue< T , K >::push( const T & x , K f , K g )
{
 ADS_PRE( ( f >= 0 ) && ( g >= 0 ) );
 const auto F = std::size_t( f ) , G = std::size_t( g );
 if( F >= f_layers.size() )
  f_layers.resize( F + 1 );
 Layer & l = f_layers[ F ];
 bool sparse = ( G >= D ) && ! l.dense();
 if( sparse && ( l.runs.size() == R ) ) {   // one run too many: dense
  densify( l );
  sparse = false;
  }
 if( sparse ) {                             // run() will not throw
  if( l.runs.size() == l.runs.capacity() )
   l.runs.reserve( std::max< std::size_t >( 4 , 2 * l.runs.size() ) );
  }
 else
  if( ( G >= S ) && ( G - S >= l.big.size() ) )
   l.big.resize( G - S + 1 , NIL );
 Index c;
 if( f_free != NIL ) {                      // the cell freed last
  c = f_free;
  f_cells[ c ].item = x;                    // first, in case it throws
  f_free = f_cells[ c ].next;
  }
 else {
  c = Index( f_cells.size() );
  f_cells.push_back( Cell{ x , NIL } );
  }
 Index & h = sparse ? run( l , g ) : l.head( G );
 f_cells[ c ].next = h;
 h = c;
 if( ( ! sparse ) && ( ( l.count == 0 ) || ( G > l.topG ) ) )
  l.topG = Index( G );
 ++l.count;
 if( ( f_size++ == 0 ) || ( F < f_minF ) )
  f_minF = F;
 ADS_INVARIANT( counted() && ordered( l ) );
 }

template< class T , std::integral K >
auto BucketQueue< T , K >::run( Layer & l , K g ) -> Index &
{
 auto & v = l.runs;
 if( v.empty() || ( g > v.back().g ) ) {    // the largest g: a new last run
  v.push_back( Run{ g , NIL } );
  return( v.back().head );
  }
 if( g == v.back().g )
  return( v.back().head );
 const auto it = std::lower_bound( v.begin() , v.end() , g ,
                  []( const Run & r , K k ) { return( r.g < k ); } );
 if( it->g == g )
  return( it->head );
 return( v.insert( it , Run{ g , NIL } )->head );
 }

template< class T , std::integral K >
void BucketQueue< T , K >::densify( Layer & l )
{
 const std::size_t G = std::size_t( l.runs.back().g );
 l.big.resize( G - S + 1 , NIL );           // more than D - S: dense
 for( const Run & r : l.runs )
  l.big[ std::size_t( r.g ) - S ] = r.head;
 l.topG = Index( G );                       // above every dense g < D
 std::vector< Run >().swap( l.runs );
 }

template< class T , std::integral K >
inline auto BucketQueue< T , K >::front( void ) -> Layer &
{
 ADS_PRE( f_size > 0 );
 while( f_layers[ f_minF ].count == 0 )     // forward to the least f
  ++f_minF;
 Layer & l = f_layers[ f_minF ];
 if( l.runs.empty() )
  while( l.head( l.topG ) == NIL )          // backward to the largest g
   --l.topG;
 return( l );
 }

template< class T , std::integral K >
inline const T & BucketQueue< T , K >::top( void )
{
 return( f_cells[ front().top() ].item );
 }

template< class T , std::integral K >
inline K BucketQueue< T , K >::topF( void )
{
 front();
 return( K( f_minF ) );
 }

template< class T , std::integral K >
typename BucketQueue< T , K >::Entry BucketQueue< T , K >::popEntry( void )
{
 const K g = front().topKey() , f = K( f_minF );
 return( Entry{ pop() , f , g } );          // front() is now O( 1 )
 }

template< class T , std::integral K >
inline K BucketQueue< T , K >::topG( void )
{
 return( front().topKey() );
 }

template< class T , std::integral K >
inline T BucketQueue< T , K >::pop( void )
{
 Layer & l = front();
 Index & h = l.top();
 const Index c = h;
 h = f_cells[ c ].next;
 T x = std::move( f_cells[ c ].item );
 f_cells[ c ].next = f_free;                // the cell is free
 f_free = c;
 --f_size;
 if( ( h == NIL ) && ! l.runs.empty() )     // the last run is empty
  l.runs.pop_back();
 if( --l.count == 0 ) {                     // free the heads of large g
  if( ! l.big.empty() )
   std::vector< Index >().swap( l.big );
  if( l.runs.capacity() )
   std::vector< Run >().swap( l.runs );
  l.topG = 0;
  }
 ADS_INVARIANT( counted() && ordered( l ) );
 return( x );
 }

template< class T , std::integral K >
bool BucketQueue< T , K >::empty( void ) const
{
 return( f_size == 0 );
 }

template< class T , std::integral K >
std::size_t BucketQueue< T , K >::size( void ) const
{
 return( f_size );
 }

template< class T , std::integral K >
void BucketQueue< T , K >::clear( void )
{
 std::vector< Layer >().swap( f_layers );
 std::vector< Cell >().swap( f_cells );
 f_free = NIL;
 f_minF = 0;
 f_size = 0;
 }

template< class T , std::integral K >
bool BucketQueue< T , K >::counted( void ) const
{
 std::size_t n = 0;
 for( const Layer & l : f_layers )
  n += l.count;
 return( n == f_size );
 }

template< class T , std::integral K >
bool BucketQueue< T , K >::ordered( const Layer & l )
{
 if( l.runs.size() > ( l.dense() ? 0 : R ) )
  return( false );
 for( std::size_t i = 0 ; i < l.runs.size() ; ++i )
  if( ( std::size_t( l.runs[ i ].g ) < D ) || ( l.runs[ i ].head == NIL ) ||
      ( i && ( l.runs[ i - 1 ].g >= l.runs[ i ].g ) ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BucketQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
