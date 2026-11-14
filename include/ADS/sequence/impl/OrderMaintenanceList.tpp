/*--------------------------------------------------------------------------*/
/*---------------------- File OrderMaintenanceList.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of OrderMaintenanceList [see `OrderMaintenanceList.h`],
 * with their contracts.
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

template< class T >
OrderMaintenanceList< T >::~OrderMaintenanceList()
{
 for( Node * u = f_first ; u ; ) {
  Node * v = u->next;
  delete u;
  u = v;
  }
 for( Bucket * b = f_head.next ; b ; ) {
  Bucket * c = b->next;
  delete b;
  b = c;
  }
 }

template< class T >
const std::array< double , 64 > & OrderMaintenanceList< T >::caps( void )
{
 static const std::array< double , 64 > c = [] {
  std::array< double , 64 > a{};
  for( std::size_t i = 0 ; i < a.size() ; ++i )
   a[ i ] = std::pow( 2.0 / 1.4 , double( i ) );
  return( a );
  }();
 return( c );
 }

template< class T >
auto OrderMaintenanceList< T >::addAfter( Element e , const T & v )
 -> Element
{
 ADS_PRE( ! e || e->bucket );
 Node * u = new Node{ v };
 Bucket * b;
 if( e ) {                                  // after e, in its bucket
  Node * p = const_cast< Node * >( e );
  b = p->bucket;
  u->prev = p;
  u->next = p->next;
  if( p->next ) p->next->prev = u;
  p->next = u;
  }
 else {                                     // at the front of the first
  u->next = f_first;
  if( f_first ) f_first->prev = u;
  f_first = u;
  if( u->next ) b = u->next->bucket;
  else {
   b = new Bucket;
   addBucketAfter( &f_head , b );
   }
  b->head = u;
  }
 u->bucket = b;
 ++b->size;
 ++f_n;
 const std::uint64_t lo = ( u != b->head ) ? u->prev->label : 0 ,
                     hi = ( u->next && ( u->next->bucket == b ) )
                          ? u->next->label : kU;
 if( hi - lo >= 2 )
  u->label = lo + ( hi - lo ) / 2;
 else
  relabelBucket( b );
 if( b->size > B )
  split( b );
 ADS_INVARIANT( valid( u->bucket ) );
 return( u );
 }

template< class T >
void OrderMaintenanceList< T >::remove( Element e )
{
 ADS_PRE( e && e->bucket );
 Node * u = const_cast< Node * >( e );
 Bucket * b = u->bucket;
 if( u->prev ) u->prev->next = u->next;
 else f_first = u->next;
 if( u->next ) u->next->prev = u->prev;
 if( b->head == u ) b->head = ( b->size > 1 ) ? u->next : nullptr;
 if( --b->size == 0 ) {                     // the bucket goes too
  b->prev->next = b->next;
  if( b->next ) b->next->prev = b->prev;
  delete b;
  }
 else
  ADS_INVARIANT( valid( b ) );
 u->bucket = nullptr;
 delete u;
 --f_n;
 }

template< class T >
void OrderMaintenanceList< T >::relabelBucket( Bucket * b )
{
 const std::uint64_t gap = kU / ( b->size + 1 );
 Node * x = b->head;
 for( std::size_t k = 1 ; k <= b->size ; ++k , x = x->next )
  x->label = k * gap;
 f_work += b->size;
 }

template< class T >
void OrderMaintenanceList< T >::split( Bucket * b )
{
 Bucket * c = new Bucket;
 const std::size_t keep = b->size / 2;
 Node * x = b->head;
 for( std::size_t k = 0 ; k < keep ; ++k ) x = x->next;
 c->head = x;
 c->size = b->size - keep;
 b->size = keep;
 for( std::size_t k = 0 ; k < c->size ; ++k , x = x->next )
  x->bucket = c;
 addBucketAfter( b , c );
 relabelBucket( b );
 relabelBucket( c );
 ADS_INVARIANT( valid( b ) );
 }

template< class T >
void OrderMaintenanceList< T >::addBucketAfter( Bucket * b , Bucket * c )
{
 const std::uint64_t L = b->label , hi = b->next ? b->next->label : kU;
 auto link = [ & ]( void ) {
  c->prev = b;
  c->next = b->next;
  if( b->next ) b->next->prev = c;
  b->next = c;
  };
 if( hi - L >= 2 ) {                        // room in the gap
  link();
  c->label = L + ( hi - L ) / 2;
  return;
  }
 Bucket * first = b , * last = b;
 std::size_t m = 2;                         // b and c
 int i = 1;
 std::uint64_t lo = L;
 for( ; ; ++i ) {                           // the smallest range of 2^i
  ADS_PRE( i <= 63 );                       // labels not too dense
  const std::uint64_t w = std::uint64_t( 1 ) << i;
  lo = L & ~( w - 1 );
  while( first->prev && ( first->prev->label >= lo ) ) {
   first = first->prev;
   ++m;
   }
  while( last->next && ( last->next->label - lo < w ) ) {
   last = last->next;
   ++m;
   }
  if( double( m ) <= caps()[ std::size_t( i ) ] )
   break;
  }
 link();
 const std::uint64_t gap = ( std::uint64_t( 1 ) << i ) / m;
 Bucket * x = first;
 for( std::size_t k = 0 ; k < m ; ++k , x = x->next )
  x->label = lo + k * gap;
 ++f_relabels;
 f_work += m;
 }

template< class T >
bool OrderMaintenanceList< T >::valid( const Bucket * b ) const
{
 if( ( b->size < 1 ) || ( b->size > B ) || ! b->prev ||
     ( b->prev->next != b ) || ( b->prev->label >= b->label ) ||
     ( b->next && ( ( b->next->prev != b ) ||
                    ( b->next->label <= b->label ) ) ) ||
     ( b->label >= kU ) )
  return( false );
 const Node * x = b->head;
 if( ( ( x->prev == nullptr ) != ( b->prev == &f_head ) ) ||
     ( x->prev ? ( x->prev->bucket != b->prev ) : ( x != f_first ) ) )
  return( false );
 std::uint64_t last = 0;
 for( std::size_t k = 0 ; k < b->size ; ++k , x = x->next ) {
  if( ! x || ( x->bucket != b ) || ( x->label <= last ) ||
      ( x->label >= kU ) || ( x->next && ( x->next->prev != x ) ) )
   return( false );
  last = x->label;
  }
 return( x ? ( x->bucket == b->next ) : ( b->next == nullptr ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File OrderMaintenanceList.tpp -------------------*/
/*--------------------------------------------------------------------------*/
