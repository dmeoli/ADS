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
void BucketQueue< T , K >::push( const T & x , K f , K g )
{
 ADS_PRE( ( f >= 0 ) && ( g >= 0 ) );
 const auto F = std::size_t( f ) , G = std::size_t( g );
 if( F >= f_layers.size() )
  f_layers.resize( F + 1 );
 Layer & l = f_layers[ F ];
 if( G >= l.byG.size() )
  l.byG.resize( G + 1 );
 l.byG[ G ].push_back( x );
 if( ( l.count++ == 0 ) || ( G > l.topG ) )
  l.topG = G;
 if( ( f_size++ == 0 ) || ( F < f_minF ) )
  f_minF = F;
 ADS_INVARIANT( counted() );
 }

template< class T , std::integral K >
auto BucketQueue< T , K >::front( void ) -> Layer &
{
 ADS_PRE( f_size > 0 );
 while( f_layers[ f_minF ].count == 0 )     // forward to the least f
  ++f_minF;
 Layer & l = f_layers[ f_minF ];
 while( l.byG[ l.topG ].empty() )           // backward to the largest g
  --l.topG;
 return( l );
 }

template< class T , std::integral K >
const T & BucketQueue< T , K >::top( void )
{
 Layer & l = front();
 return( l.byG[ l.topG ].back() );
 }

template< class T , std::integral K >
K BucketQueue< T , K >::topF( void )
{
 front();
 return( K( f_minF ) );
 }

template< class T , std::integral K >
typename BucketQueue< T , K >::Entry BucketQueue< T , K >::popEntry( void )
{
 Layer & l = front();
 const K f = K( f_minF ) , g = K( l.topG );
 return( Entry{ pop() , f , g } );          // front() is now O( 1 )
 }

template< class T , std::integral K >
K BucketQueue< T , K >::topG( void )
{
 return( K( front().topG ) );
 }

template< class T , std::integral K >
T BucketQueue< T , K >::pop( void )
{
 Layer & l = front();
 T x = std::move( l.byG[ l.topG ].back() );
 l.byG[ l.topG ].pop_back();
 --f_size;
 if( --l.count == 0 ) {                     // free the empty layer
  std::vector< std::vector< T > >().swap( l.byG );
  l.topG = 0;
  }
 ADS_INVARIANT( counted() );
 return( x );
 }

template< class T , std::integral K >
bool BucketQueue< T , K >::isEmpty( void ) const
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

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BucketQueue.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
