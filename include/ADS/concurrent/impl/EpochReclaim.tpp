/*--------------------------------------------------------------------------*/
/*-------------------------- File EpochReclaim.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of EpochDomain [see EpochReclaim.h], with their
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

/*------------------------------- THE DOMAIN -------------------------------*/

template< unsigned R >
EpochDomain< R >::~EpochDomain()
{
 for( Record * r = f_records.load( std::memory_order_acquire ) ; r ; ) {
  Record * q = r->next;
  ADS_PRE( ! r->used.load( std::memory_order_acquire ) );
  for( Retirable * l : r->limbo )
   detail::freeRetired( l );
  delete r;
  r = q;
  }
 }

template< unsigned R >
std::size_t EpochDomain< R >::records( void ) const
{
 std::size_t n = 0;
 for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
      r = r->next )
  ++n;
 return( n );
 }

template< unsigned R >
std::size_t EpochDomain< R >::pending( void ) const
{
 std::uint64_t n = 0;
 for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
      r = r->next )
  n += r->retired.load( std::memory_order_relaxed ) -
       r->freed.load( std::memory_order_relaxed );
 return( std::size_t( n ) );
 }

template< unsigned R >
std::uint64_t EpochDomain< R >::freed( void ) const
{
 std::uint64_t n = 0;
 for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
      r = r->next )
  n += r->freed.load( std::memory_order_relaxed );
 return( n );
 }

template< unsigned R >
auto EpochDomain< R >::acquire( void ) -> Record *
{
 for( Record * r = f_records.load( std::memory_order_acquire ) ; r ;
      r = r->next )
  if( ( ! r->used.load( std::memory_order_relaxed ) ) &&
      ( ! r->used.exchange( true , std::memory_order_acquire ) ) )
   return( r );
 // pushed by a sequentially consistent compare-and-swap, which a scan
 // that reads the first record with sequential consistency after it in S
 // sees [see the theory page]
 Record * r = new Record;
 r->next = f_records.load( std::memory_order_relaxed );
 while( ! f_records.compare_exchange_weak(
   r->next , r , std::memory_order_seq_cst , std::memory_order_relaxed ) ) {
  }
 return( r );
 }

template< unsigned R >
void EpochDomain< R >::collect( Record & r , std::uint64_t e ) noexcept
{
 for( int j = 0 ; j < 3 ; ++j )
  if( r.limbo[ j ] && ( r.label[ j ] + kLag <= e ) ) {
   detail::ownerAdd( r.freed , detail::freeRetired( r.limbo[ j ] ) );
   r.limbo[ j ] = nullptr;
   }
 }

template< unsigned R >
void EpochDomain< R >::advance( void ) noexcept
{
 std::uint64_t e = f_epoch.load( std::memory_order_seq_cst );
 for( Record * r = f_records.load( std::memory_order_seq_cst ) ; r ;
      r = r->next ) {
  const std::uint64_t v = r->local.load( std::memory_order_seq_cst );
  if( ( v & 1 ) && ( v != 2 * e + 1 ) )
   return;  // a thread still in an older epoch
  }
 f_epoch.compare_exchange_strong( e , e + 1 , std::memory_order_seq_cst );
 }

/*------------------------------ THE RECORDS -------------------------------*/

template< unsigned R >
void EpochDomain< R >::Local::retire( Retirable * p ) noexcept
{
 Record & r = *f_r;
 // the epoch after the unlink of p
 const std::uint64_t e = f_d->f_epoch.load( std::memory_order_seq_cst );
 const int j = int( e % 3 );
 if( r.label[ j ] != e ) {
  // the limbo of the same residue holds nodes of a label kLag behind or
  // more, which no guard may still hold
  ADS_INVARIANT( ( ! r.limbo[ j ] ) || ( r.label[ j ] + kLag <= e ) );
  detail::ownerAdd( r.freed , detail::freeRetired( r.limbo[ j ] ) );
  r.limbo[ j ] = nullptr;
  r.label[ j ] = e;
  }
 p->retired_next = r.limbo[ j ];
 r.limbo[ j ] = p;
 detail::ownerAdd( r.retired , 1 );
 ++r.since;
 }

/*------------------------------- THE GUARDS -------------------------------*/

template< unsigned R >
EpochDomain< R >::Local::Guard::Guard( Local & l ) noexcept : f_l( l )
{
 // the announce, by a sequentially consistent exchange, before any read of
 // the structure, whose reads are sequentially consistent too [see
 // protect()]: a read that sees a word older than the unlink of a node is
 // coherence-ordered before that unlink, and precedes it in the total order
 // S of the sequentially consistent operations, as the exchange, sequenced
 // before it, does; a scan that follows the unlink in S then reads the
 // announce or a later value [see the theory page]
 Record & r = *l.f_r;
 EpochDomain & d = *l.f_d;
 ADS_PRE( ! ( r.local.load( std::memory_order_relaxed ) & 1 ) );
 const std::uint64_t e = d.f_epoch.load( std::memory_order_seq_cst );
 r.local.exchange( 2 * e + 1 , std::memory_order_seq_cst );
 r.now = e;
 collect( r , e );
 }

template< unsigned R >
EpochDomain< R >::Local::Guard::~Guard()
{
 Record & r = *f_l.f_r;
 if( r.since >= R ) {
  r.since = 0;
  f_l.f_d->advance();
  }
 r.local.store( 0 , std::memory_order_release );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File EpochReclaim.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
