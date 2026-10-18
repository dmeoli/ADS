/*--------------------------------------------------------------------------*/
/*--------------------------- File CellHeap.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CellHeap [see CellHeap.h], with their contracts.
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

inline CellHeap::Value CellHeap::atom( long k )
{
 ADS_PRE( k >= 0 );
 return( -2 - k );
 }

inline bool CellHeap::isCell( Value v )
{
 return( v >= 0 );
 }

inline CellHeap::CellHeap( std::size_t n ) : f_C( n )
{
 ADS_PRE( n >= 1 );
 for( std::size_t i = 0 ; i < n ; ++i ) {   // all on the free list
  f_C[ i ].car = i + 1 < n ? Value( i + 1 ) : nil;
  }
 f_free = 0;
 f_nfree = n;
 }

inline CellHeap::Value CellHeap::cons( Value a , Value d )
{
 if( f_free == nil )
  collect();
 ADS_PRE( f_free != nil );                  // the memory is exhausted
 const Value c = f_free;
 f_free = f_C[ c ].car;
 --f_nfree;
 f_C[ c ] = Cell{ a , d , false , false , false };
 return( c );
 }

inline CellHeap::Value CellHeap::car( Value c ) const
{
 ADS_PRE( isCell( c ) && ( c < Value( f_C.size() ) ) && ! f_C[ c ].free );
 return( f_C[ c ].car );
 }

inline CellHeap::Value CellHeap::cdr( Value c ) const
{
 ADS_PRE( isCell( c ) && ( c < Value( f_C.size() ) ) && ! f_C[ c ].free );
 return( f_C[ c ].cdr );
 }

inline void CellHeap::setCar( Value c , Value v )
{
 ADS_PRE( isCell( c ) && ( c < Value( f_C.size() ) ) && ! f_C[ c ].free );
 f_C[ c ].car = v;
 }

inline void CellHeap::setCdr( Value c , Value v )
{
 ADS_PRE( isCell( c ) && ( c < Value( f_C.size() ) ) && ! f_C[ c ].free );
 f_C[ c ].cdr = v;
 }

inline void CellHeap::addRoot( Value c )
{
 ADS_PRE( isCell( c ) && ( c < Value( f_C.size() ) ) );
 f_roots.insert( c );
 }

inline void CellHeap::removeRoot( Value c )
{
 const auto it = f_roots.find( c );
 if( it != f_roots.end() ) f_roots.erase( it );
 }

inline void CellHeap::markReversal( Value cur )
{
 Value prev = nil;
 for( ; ; ) {
  while( isCell( cur ) && ! f_C[ cur ].mark ) {   // down the car
   Cell & x = f_C[ cur ];
   x.mark = true;
   x.cdrSide = false;
   const Value next = x.car;
   x.car = prev;                            // reversed
   prev = cur;
   cur = next;
   }
  while( ( prev != nil ) && f_C[ prev ].cdrSide ) {   // up from a cdr
   Cell & x = f_C[ prev ];
   const Value next = x.cdr;
   x.cdr = cur;                             // restored
   cur = prev;
   prev = next;
   }
  if( prev == nil )
   return;
  Cell & x = f_C[ prev ];                   // from the car to the cdr
  x.cdrSide = true;
  const Value next = x.car;
  x.car = cur;                              // the car restored
  cur = x.cdr;
  x.cdr = next;                             // the cdr reversed
  }
 }

inline void CellHeap::markStack( Value root )
{
 std::vector< Value > S = { root };
 while( ! S.empty() ) {
  const Value c = S.back();
  S.pop_back();
  if( ! isCell( c ) || f_C[ c ].mark ) continue;
  f_C[ c ].mark = true;
  S.push_back( f_C[ c ].cdr );
  S.push_back( f_C[ c ].car );
  }
 }

inline std::vector< bool > CellHeap::reachable( void ) const
{
 std::vector< bool > r( f_C.size() , false );
 std::vector< Value > S( f_roots.begin() , f_roots.end() );
 while( ! S.empty() ) {
  const Value c = S.back();
  S.pop_back();
  if( ! isCell( c ) || r[ c ] ) continue;
  r[ c ] = true;
  S.push_back( f_C[ c ].car );
  S.push_back( f_C[ c ].cdr );
  }
 return( r );
 }

inline std::size_t CellHeap::collect( bool reversal )
{
 std::vector< bool > live;                  // for the audit
 std::vector< Cell > before;
 if constexpr( ADS_CONTRACTS >= 3 ) {
  live = reachable();
  before = f_C;
  }
 for( Value r : f_roots )
  if( reversal ) markReversal( r ); else markStack( r );
 std::size_t freed = 0;
 f_free = nil;                              // the sweep rebuilds the list
 f_nfree = 0;
 for( std::size_t i = f_C.size() ; i-- > 0 ; ) {
  Cell & x = f_C[ i ];
  if( x.mark ) {
   x.mark = false;
   continue;
   }
  if( ! x.free ) ++freed;
  x = Cell{ f_free , nil , false , false , true };
  f_free = Value( i );
  ++f_nfree;
  }
 ADS_AUDIT( [ & ]( void ) {
  for( std::size_t i = 0 ; i < f_C.size() ; ++i )
   if( live[ i ] ? ( f_C[ i ].free || ( f_C[ i ].car != before[ i ].car ) ||
                     ( f_C[ i ].cdr != before[ i ].cdr ) )
                 : ! f_C[ i ].free )
    return( false );
  return( true );
  }() );
 return( freed );
 }

inline std::size_t CellHeap::freeCells( void ) const
{
 return( f_nfree );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File CellHeap.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
