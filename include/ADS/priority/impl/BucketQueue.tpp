/*--------------------------------------------------------------------------*/
/*-------------------------- File BucketQueue.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BucketQueue [see BucketQueue.h], with their contracts.
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

template< class T , std::integral K >
inline void BucketQueue< T , K >::push( const T & x , K f , K g )
{
 ADS_PRE( ( f >= 0 ) && ( g >= 0 ) );
 const auto F = std::size_t( f ) , G = std::size_t( g );
 if( F >= f_layers.size() )
  f_layers.resize( F + 1 );
 Layer & l = f_layers[ F ];
 if( ( G < D ) || l.dense() || ! pushRun( l , x , g ) ) {  // a dense list
  if( ( G >= S ) && ( G - S >= l.big.size() ) )
   l.big.resize( G - S + 1 , NIL );
  Index & h = l.head( G );
  const Index c = cell( x );
  f_cells[ c ].next = h;
  h = c;
  if( ( l.count == 0 ) || ( G > l.topG ) )
   l.topG = Index( G );
  }
 ++l.count;
 if( ( f_size++ == 0 ) || ( F < f_minF ) )
  f_minF = F;
 ADS_INVARIANT( counted() && ordered( l ) );
 }

template< class T , std::integral K >
inline auto BucketQueue< T , K >::cell( const T & x ) -> Index
{
 if( f_free != NIL ) {                      // the cell freed last
  const Index c = f_free;
  f_cells[ c ].item = x;                    // first, in case it throws
  f_free = f_cells[ c ].next;
  return( c );
  }
 f_cells.push_back( Cell{ x , NIL } );
 return( Index( f_cells.size() - 1 ) );
 }

template< class T , std::integral K >
bool BucketQueue< T , K >::pushRun( Layer & l , const T & x , K g )
{
 if( ( l.runs != NIL ) && ( f_runs[ l.runs ].size() == R ) ) {
  densify( l );                             // one run too many: dense
  return( false );
  }
 Index k = l.runs;                          // the runs of the layer
 if( k == NIL ) {
  if( f_spare.empty() ) {
   f_spare.reserve( f_runs.size() + 1 );    // release() will not throw
   f_runs.emplace_back();
   f_spare.push_back( Index( f_runs.size() - 1 ) );
   }
  k = f_spare.back();
  }
 Runs & v = f_runs[ k ];
 if( v.size() == v.capacity() )             // run() will not throw
  v.reserve( std::max< std::size_t >( 4 , 2 * v.size() ) );
 const Index c = cell( x );
 if( l.runs == NIL ) {                      // the spare runs taken
  l.runs = k;
  f_spare.pop_back();
  }
 Index & h = run( v , g );
 f_cells[ c ].next = h;
 h = c;
 return( true );
 }

template< class T , std::integral K >
auto BucketQueue< T , K >::run( Runs & v , K g ) -> Index &
{
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
 const Runs & v = f_runs[ l.runs ];
 const std::size_t G = std::size_t( v.back().g );
 l.big.resize( G - S + 1 , NIL );           // more than D - S: dense
 for( const Run & r : v )
  l.big[ std::size_t( r.g ) - S ] = r.head;
 l.topG = Index( G );                       // above every dense g < D
 release( l );
 }

template< class T , std::integral K >
void BucketQueue< T , K >::release( Layer & l )
{
 f_runs[ l.runs ].clear();                  // kept for the next layer
 f_spare.push_back( l.runs );               // within its capacity
 l.runs = NIL;
 }

template< class T , std::integral K >
inline auto BucketQueue< T , K >::top( Layer & l ) -> Index &
{
 return( l.runs == NIL ? l.head( l.topG ) : f_runs[ l.runs ].back().head );
 }

template< class T , std::integral K >
inline K BucketQueue< T , K >::topKey( const Layer & l ) const
{
 return( l.runs == NIL ? K( l.topG ) : f_runs[ l.runs ].back().g );
 }

template< class T , std::integral K >
inline auto BucketQueue< T , K >::front( void ) -> Layer &
{
 ADS_PRE( f_size > 0 );
 while( f_layers[ f_minF ].count == 0 )     // forward to the least f
  ++f_minF;
 Layer & l = f_layers[ f_minF ];
 if( l.runs == NIL )
  while( l.head( l.topG ) == NIL )          // backward to the largest g
   --l.topG;
 return( l );
 }

template< class T , std::integral K >
inline const T & BucketQueue< T , K >::top( void )
{
 return( f_cells[ top( front() ) ].item );
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
 const K g = topKey( front() ) , f = K( f_minF );
 return( Entry{ pop() , f , g } );          // front() is now O( 1 )
 }

template< class T , std::integral K >
inline K BucketQueue< T , K >::topG( void )
{
 return( topKey( front() ) );
 }

template< class T , std::integral K >
inline T BucketQueue< T , K >::pop( void )
{
 Layer & l = front();
 if( l.runs != NIL )                        // from the last run
  return( popRun( l ) );
 T x = take( l.head( l.topG ) );
 taken( l );
 ADS_INVARIANT( counted() && ordered( l ) );
 return( x );
 }

template< class T , std::integral K >
T BucketQueue< T , K >::popRun( Layer & l )
{
 Runs & v = f_runs[ l.runs ];
 T x = take( v.back().head );
 if( v.back().head == NIL ) {               // the last run is empty
  v.pop_back();
  if( v.empty() )
   release( l );
  }
 taken( l );
 ADS_INVARIANT( counted() && ordered( l ) );
 return( x );
 }

template< class T , std::integral K >
inline T BucketQueue< T , K >::take( Index & h )
{
 const Index c = h;
 h = f_cells[ c ].next;
 T x = std::move( f_cells[ c ].item );
 f_cells[ c ].next = f_free;                // the cell is free
 f_free = c;
 --f_size;
 return( x );
 }

template< class T , std::integral K >
inline void BucketQueue< T , K >::taken( Layer & l )
{
 if( --l.count == 0 ) {                     // free the heads of large g
  if( ! l.big.empty() )
   std::vector< Index >().swap( l.big );
  l.topG = 0;
  }
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
 std::vector< Runs >().swap( f_runs );
 std::vector< Index >().swap( f_spare );
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
bool BucketQueue< T , K >::ordered( const Layer & l ) const
{
 if( l.runs == NIL )
  return( true );
 const Runs & v = f_runs[ l.runs ];
 if( v.empty() || l.dense() || ( v.size() > R ) )
  return( false );
 for( std::size_t i = 0 ; i < v.size() ; ++i )
  if( ( std::size_t( v[ i ].g ) < D ) || ( v[ i ].head == NIL ) ||
      ( i && ( v[ i - 1 ].g >= v[ i ].g ) ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BucketQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
