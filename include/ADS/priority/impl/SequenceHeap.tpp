/*--------------------------------------------------------------------------*/
/*-------------------------- File SequenceHeap.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SequenceHeap [see SequenceHeap.h], with their
 * contracts. push(), top() and pop() are inline, so that the compiler
 * expands their fast paths in the loop of the caller; the merges are not.
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
/*---------------------------- THE OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Better >
inline void SequenceHeap< T , Better >::push( const T & x )
{
 if( f_n == 0 ) {                           // a bulk may begin
  f_open = true;
  f_scans = 0;
  }
 ++f_n;
 if( f_open ) {                             // pushed in bulk, as they come
  f_bulk.push_back( x );
  f_minLast = false;
  ADS_INVARIANT( ordered() );
  return;
  }
 if( f_ins.size() == M ) {
  const T y = x;                            // x may be an item here
  flush();
  f_ins.push_back( y );
  }
 else
  f_ins.push_back( x );
 if( f_heaped )
  up( f_ins.size() - 1 );
 ADS_INVARIANT( ordered() );
 }

template< class T , class Better >
inline void SequenceHeap< T , Better >::up( std::size_t i )
{
 T x = std::move( f_ins[ i ] );
 while( i > 0 ) {
  const std::size_t p = ( i - 1 ) / D;
  if( ! f_better( x , f_ins[ p ] ) )
   break;
  f_ins[ i ] = std::move( f_ins[ p ] );
  i = p;
  }
 f_ins[ i ] = std::move( x );
 }

template< class T , class Better >
inline void SequenceHeap< T , Better >::down( std::size_t i , T x )
{
 const std::size_t n = f_ins.size();
 for( ; ; ) {
  const std::size_t c = D * i + 1;
  if( c >= n )
   break;
  std::size_t b = c;                        // the best child
  const std::size_t last = c + D < n ? c + D : n;
  for( std::size_t k = c + 1 ; k < last ; ++k )
   if( f_better( f_ins[ k ] , f_ins[ b ] ) )
    b = k;
  if( ! f_better( f_ins[ b ] , x ) )
   break;
  f_ins[ i ] = std::move( f_ins[ b ] );
  i = b;
  }
 f_ins[ i ] = std::move( x );
 }

template< class T , class Better >
inline void SequenceHeap< T , Better >::ready( void )
{
 ADS_PRE( f_n > 0 );
 if( f_open ) {
  if( ( f_scans == 0 ) && ( f_bulk.size() > M ) ) {   // the first look at
   if( ! f_minLast ) {                      // a large bulk: its best item
    ++f_scans;                              // found by a scan, put last
    std::size_t b = 0;
    for( std::size_t i = 1 ; i < f_bulk.size() ; ++i )
     b = bt( f_bulk[ i ] , f_bulk[ b ] ) ? i : b;
    std::swap( f_bulk[ b ] , f_bulk.back() );
    f_minLast = true;
    }
   return;
   }
  if( ! f_minLast )
   close();
  else
   return;
  }
 if( ! f_heaped ) {                         // the heap made when needed
  for( std::size_t i = f_ins.size() / D + 1 ; i-- > 0 ; )
   if( i < f_ins.size() )
    down( i , T( f_ins[ i ] ) );
  f_heaped = true;
  }
 if( f_del.done() )
  refillDeletion();
 }

template< class T , class Better >
inline const T & SequenceHeap< T , Better >::top( void )
{
 ready();
 if( f_open )                               // the best of the bulk, last
  return( f_bulk.back() );
 if( f_del.done() ||
     ( ! f_ins.empty() && bt( f_ins.front() , f_del.a[ f_del.at ] ) ) )
  return( f_ins.front() );
 return( f_del.a[ f_del.at ] );
 }

template< class T , class Better >
inline T SequenceHeap< T , Better >::pop( void )
{
 ready();
 if( f_open ) {                             // the best of the bulk, last
  T x = std::move( f_bulk.back() );
  f_bulk.pop_back();
  f_minLast = false;
  --f_n;
  ADS_INVARIANT( ordered() );
  return( x );
  }
 const bool fromIns = f_del.done() ||
  ( ! f_ins.empty() && bt( f_ins.front() , f_del.a[ f_del.at ] ) );
 --f_n;
 if( fromIns ) {
  T x = std::move( f_ins.front() );        // the last one down from the
  T y = std::move( f_ins.back() );          // root
  f_ins.pop_back();
  if( ! f_ins.empty() )
   down( 0 , std::move( y ) );
  ADS_INVARIANT( ordered() );
  return( x );
  }
 T x = std::move( f_del.a[ f_del.at++ ] );
 ADS_INVARIANT( ordered() );
 return( x );
 }

template< class T , class Better >
void SequenceHeap< T , Better >::clear( void )
{
 f_ins.clear();
 f_heaped = true;
 f_open = false;
 f_scans = 0;
 f_minLast = false;
 f_bulk.clear();
 f_lo = 0;
 f_piv.clear();
 f_bbuf = Run();
 f_del = Run();
 f_groups.clear();
 f_n = 0;
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE MERGES --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Better >
void SequenceHeap< T , Better >::merge( std::vector< Run * > & R ,
                                        std::vector< T > & out ,
                                        std::size_t n )
{
 std::erase_if( R , []( const Run * r ) { return( r->done() ); } );
 if( n == 0 ) {                             // all of them
  std::size_t total = 0;
  for( const Run * r : R ) total += r->left();
  out.reserve( out.size() + total );
  n = total;
  }
 const std::size_t k = R.size();
 if( k == 0 )
  return;
 if( k == 1 ) {                             // the rest of the only run
  Run & r = *R.front();
  const std::size_t c = std::min( r.left() , n );
  std::move( r.a.begin() + std::ptrdiff_t( r.at ) ,
             r.a.begin() + std::ptrdiff_t( r.at + c ) ,
             std::back_inserter( out ) );
  r.at += c;
  return;
  }
 // the tree of losers on L >= k leaves, the leaf i the run i, with the
 // first key of each run at hand; a run that is done (or a leaf past k)
 // loses every match, and on equal keys the run that challenges wins; the
 // replay chooses by a conditional move, not a jump
 std::size_t L = 1;
 while( L < k ) L *= 2;
 f_key.resize( L );
 f_dead.assign( L , 1 );
 f_lose.resize( 2 * L );
 f_cur.resize( L );
 f_end.resize( L );
 T ** cur = f_cur.data();
 T ** end = f_end.data();
 T * key = f_key.data();
 unsigned char * dead = f_dead.data();
 for( std::size_t i = 0 ; i < k ; ++i ) {
  cur[ i ] = R[ i ]->a.data() + R[ i ]->at;
  end[ i ] = R[ i ]->a.data() + R[ i ]->a.size();
  key[ i ] = *cur[ i ];
  dead[ i ] = 0;
  }
 const auto wins = [ & ]( std::size_t i , std::size_t j ) {
  const bool b = bt( key[ j ] , key[ i ] );
  const bool li = dead[ i ] , lj = dead[ j ];
  return( bool( ( ! li ) & ( lj | ( ! b ) ) ) );
  };
 std::size_t * W = f_lose.data();           // the winners, bottom up,
 for( std::size_t i = 0 ; i < L ; ++i )     // then the losers in place
  W[ L + i ] = i;
 f_lose2.resize( L );
 std::size_t * Lose = f_lose2.data();
 for( std::size_t v = L - 1 ; v >= 1 ; --v ) {
  const std::size_t x = W[ 2 * v ] , y = W[ 2 * v + 1 ];
  const bool xw = wins( x , y );
  W[ v ] = xw ? x : y;
  Lose[ v ] = xw ? y : x;
  }
 std::size_t w = W[ 1 ];
 for( std::size_t v = 1 ; v < L ; ++v )
  W[ v ] = Lose[ v ];
 for( std::size_t taken = 0 ; ( taken < n ) && ! dead[ w ] ; ++taken ) {
  out.push_back( std::move( *cur[ w ] ) );
  if( ++cur[ w ] == end[ w ] )
   dead[ w ] = 1;
  else
   key[ w ] = *cur[ w ];
  for( std::size_t v = ( L + w ) / 2 ; v >= 1 ; v /= 2 ) {   // replay
   const std::size_t l = W[ v ];
   const bool up = wins( l , w );
   W[ v ] = up ? w : l;
   w = up ? l : w;
   }
  }
 for( std::size_t i = 0 ; i < k ; ++i )     // what the runs keep
  R[ i ]->at = std::size_t( cur[ i ] - R[ i ]->a.data() );
 }

template< class T , class Better >
void SequenceHeap< T , Better >::merge2( const T * a , std::size_t na ,
                                         const T * b , std::size_t nb ,
                                         std::vector< T > & out ) const
{
 std::size_t i = 0 , j = 0;
 while( ( i < na ) && ( j < nb ) ) {        // the choice by a conditional
  const bool fromB = bt( b[ j ] , a[ i ] );   // move, not a jump
  out.push_back( fromB ? b[ j ] : a[ i ] );
  j += fromB;
  i += ! fromB;
  }
 out.insert( out.end() , a + i , a + na );
 out.insert( out.end() , b + j , b + nb );
 }

template< class T , class Better >
void SequenceHeap< T , Better >::flush( void )
{
 Run S;                                     // the insertion heap, sorted
 S.a.swap( f_ins );
 f_ins.reserve( M );
 if constexpr( detail::kKeyed< T , Better > )   // by the keys, or by
  radixSortBy( S.a , []( const T & x ) {         // comparisons
   return( detail::orderKeyBy< Better >( x ) ); } );
 else
  ADS::sort( S.a , [ this ]( const T & a , const T & b ) {
   return( bt( a , b ) ); } );
 Run * B0 = f_groups.empty() ? nullptr : &f_groups[ 0 ].buf;
 if( ( f_del.done() || ! bt( S.a.front() , f_del.a.back() ) ) &&
     ( ! B0 || B0->done() || ! bt( S.a.front() , B0->a.back() ) ) ) {
  join( 0 , std::move( S ) );               // no better than the buffers
  return;
  }
 const std::size_t nd = f_del.left();
 const std::size_t nb = B0 ? B0->left() : 0;
 // the deletion buffer and then the buffer of the group 1, one sorted
 // sequence since no item of the first is worse than one of the second,
 // merged with S by a merge of two with no jump
 std::vector< T > & DB = f_scratch;
 DB.clear();
 DB.insert( DB.end() , f_del.a.begin() + std::ptrdiff_t( f_del.at ) ,
            f_del.a.end() );
 if( B0 )
  DB.insert( DB.end() , B0->a.begin() + std::ptrdiff_t( B0->at ) ,
             B0->a.end() );
 Run s;
 s.a.reserve( S.a.size() + nd + nb );
 merge2( S.a.data() , S.a.size() , DB.data() , DB.size() , s.a );
 const auto at = [ & ]( std::size_t i ) {
  return( s.a.begin() + std::ptrdiff_t( i ) ); };
 f_del.a.assign( at( 0 ) , at( nd ) );      // the best nd stay best,
 f_del.at = 0;
 if( B0 ) {                                 // the next nb in the buffer,
  B0->a.assign( at( nd ) , at( nd + nb ) );
  B0->at = 0;
  }
 s.at = nd + nb;                            // the rest in the group 1
 f_ins.clear();
 f_ins.reserve( M );
 if( ! s.done() )
  join( 0 , std::move( s ) );
 }

template< class T , class Better >
void SequenceHeap< T , Better >::join( std::size_t i , Run && s )
{
 if( i == f_groups.size() )
  f_groups.emplace_back();
 if( f_groups[ i ].seqs.size() == K ) {
  std::size_t cap = M , total = 0;          // the room of a sequence here
  for( std::size_t j = 0 ; ( j < i ) && ( cap <= f_n ) ; ++j ) cap *= K;
  for( const Run & q : f_groups[ i ].seqs ) total += q.left();
  if( total <= cap * ( K / 2 ) ) {          // its sequences, mostly read,
   std::vector< Run * > R;                  // made fewer and full, in it
   for( Run & q : f_groups[ i ].seqs )
    R.push_back( &q );
   Run all;
   merge( R , all.a , 0 );
   f_groups[ i ].seqs.clear();
   for( std::size_t a = 0 ; a < all.a.size() ; a += cap ) {
    Run q;
    const std::size_t e = std::min( all.a.size() , a + cap );
    q.a.assign( std::make_move_iterator( all.a.begin() +
                                         std::ptrdiff_t( a ) ) ,
                std::make_move_iterator( all.a.begin() +
                                         std::ptrdiff_t( e ) ) );
    f_groups[ i ].seqs.push_back( std::move( q ) );
    }
   f_groups[ i ].seqs.push_back( std::move( s ) );
   return;
   }
  std::vector< Run * > R;                   // the group to the next one,
                                            // with the buffer there
  for( Run & q : f_groups[ i ].seqs )
   R.push_back( &q );
  if( i + 1 < f_groups.size() )
   R.push_back( &f_groups[ i + 1 ].buf );
  Run big;
  merge( R , big.a , 0 );
  f_groups[ i ].seqs.clear();
  if( i + 1 < f_groups.size() )
   f_groups[ i + 1 ].buf = Run();
  join( i + 1 , std::move( big ) );
  }
 f_groups[ i ].seqs.push_back( std::move( s ) );
 }

template< class T , class Better >
void SequenceHeap< T , Better >::refill( Group & g )
{
 if( g.seqs.empty() )
  return;
 std::vector< Run * > R;
 for( Run & q : g.seqs )
  R.push_back( &q );
 g.buf = Run();
 g.buf.a.reserve( M );
 merge( R , g.buf.a , M );
 std::erase_if( g.seqs , []( const Run & q ) { return( q.done() ); } );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- THE BULK ---------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Better >
void SequenceHeap< T , Better >::close( void )
{
 f_open = false;
 if( f_bulk.size() <= M ) {                 // a few: the insertion heap
  f_ins.insert( f_ins.end() , f_bulk.begin() , f_bulk.end() );
  f_heaped = false;
  f_bulk.clear();
  }
 else {                                     // many: sorted when needed
  f_lo = 0;
  f_piv.assign( 1 , { f_bulk.size() , f_bulk.size() } );
  }
 }

template< class T , class Better >
void SequenceHeap< T , Better >::sortRange( T * a , T * b )
{
 if constexpr( detail::kKeyed< T , Better > ) {
  if( b - a > 256 ) {
   f_scratch.assign( a , b );
   radixSortBy( f_scratch , []( const T & x ) {
    return( detail::orderKeyBy< Better >( x ) ); } );
   std::copy( f_scratch.begin() , f_scratch.end() , a );
   return;
   }
  }
 std::sort( a , b , [ this ]( const T & x , const T & y ) {
  return( bt( x , y ) ); } );
 }

template< class T , class Better >
T * SequenceHeap< T , Better >::partition( T * a , T * b , const T & pv )
{
 if constexpr( std::is_trivially_copyable_v< T > ) {
  T * s = a;                                // the better ones before s,
  for( T * p = a ; p < b ; ++p ) {          // by a swap at every step and
   const bool c = bt( *p , pv );            // a move of s, with no jump
   const T x = *p;
   *p = *s;
   *s = x;
   s += c;
   }
  return( s );
  }
 else
  return( std::partition( a , b , [ & ]( const T & v ) {
   return( bt( v , pv ) ); } ) );
 }

template< class T , class Better >
void SequenceHeap< T , Better >::refillBulk( void )
{
 f_bbuf.a.clear();
 f_bbuf.at = 0;
 T * A = f_bulk.data();
 while( ( f_bbuf.a.size() < M ) && ( f_lo < f_bulk.size() ) ) {
  const auto [ q , r ] = f_piv.back();      // A[ lo .. q ) not worse than
  const std::size_t need = M - f_bbuf.a.size();   // A[ q .. r ), equal,
  if( q - f_lo <= need ) {                  // not worse than the rest
   sortRange( A + f_lo , A + q );
   f_bbuf.a.insert( f_bbuf.a.end() , A + f_lo , A + r );
   f_lo = r;
   f_piv.pop_back();
   if( f_piv.empty() )                      // the bulk is done
    break;
   }
  else {                                    // A[ lo .. q ) split by the
   T * lo = A + f_lo , * hi = A + q;        // median of three: those
   T * mid = lo + ( hi - lo ) / 2 , * last = hi - 1;   // better first,
   if( bt( *mid , *lo ) ) std::swap( *mid , *lo );    // then the pivot
   if( bt( *last , *mid ) ) {
    std::swap( *last , *mid );
    if( bt( *mid , *lo ) ) std::swap( *mid , *lo );
    }
   std::swap( *mid , *last );               // the pivot aside, at the end
   const T pv = *last;
   T * m1 = partition( lo , last , pv );
   std::swap( *m1 , *last );                // the pivot in its place
   T * m2 = m1 + 1;
   if( m1 == lo )                           // none better: the equal ones
    m2 = std::partition( m2 , hi , [ & ]( const T & v ) {
     return( ! bt( pv , v ) ); } );
   f_piv.push_back( { std::size_t( m1 - A ) , std::size_t( m2 - A ) } );
   }
  }
 if( f_lo == f_bulk.size() ) {              // its memory freed
  decltype( f_bulk )().swap( f_bulk );
  f_lo = 0;
  f_piv.clear();
  }
 }

template< class T , class Better >
void SequenceHeap< T , Better >::refillDeletion( void )
{
 f_del.a.clear();
 f_del.at = 0;
 // the sources with items, by their first item: the buffers of the groups
 // and the buffer of the bulk (no group)
 struct Source {
  Run * buf;
  Group * g;
  };
 std::vector< Source > G;
 for( Group & g : f_groups ) {
  if( g.buf.done() )
   refill( g );
  if( ! g.buf.done() )
   G.push_back( { &g.buf , &g } );
  }
 if( f_bbuf.done() && ! f_bulk.empty() )
  refillBulk();
 if( ! f_bbuf.done() )
  G.push_back( { &f_bbuf , nullptr } );
 const auto below = [ this ]( const Source & a , const Source & b ) {
  return( bt( b.buf->a[ b.buf->at ] , a.buf->a[ a.buf->at ] ) ); };
 std::make_heap( G.begin() , G.end() , below );
 while( ! G.empty() && ( f_del.a.size() < M ) ) {
  std::pop_heap( G.begin() , G.end() , below );
  Source & e = G.back();
  f_del.a.push_back( std::move( e.buf->a[ e.buf->at++ ] ) );
  if( e.buf->done() ) {                     // refilled at once, so that
   if( e.g )                                // its source takes part
    refill( *e.g );
   else if( ! f_bulk.empty() )
    refillBulk();
   }
  if( e.buf->done() )
   G.pop_back();
  else
   std::push_heap( G.begin() , G.end() , below );
  }
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class T , class Better >
bool SequenceHeap< T , Better >::ordered( void ) const
{
 if( f_n > 4096 )
  return( true );
 const auto sorted = [ this ]( const Run & r ) {
  return( std::is_sorted( r.a.begin() + std::ptrdiff_t( r.at ) , r.a.end() ,
                          [ this ]( const T & a , const T & b ) {
                           return( bt( a , b ) ); } ) ); };
 // no item of the run r is better than the item x
 const auto notBetter = [ this ]( const Run & r , const T & x ) {
  for( std::size_t i = r.at ; i < r.a.size() ; ++i )
   if( bt( r.a[ i ] , x ) )
    return( false );
  return( true ); };
 std::size_t n = f_ins.size() + f_del.left() + f_bbuf.left() +
                 ( f_bulk.size() - f_lo );
 if( ! sorted( f_bbuf ) )
  return( false );
 if( ! f_open && ! f_bulk.empty() ) {       // the bulk not better than its
  Run rest;                                 // buffer and the deletion one
  rest.a.assign( f_bulk.begin() + std::ptrdiff_t( f_lo ) , f_bulk.end() );
  if( ( ! f_bbuf.done() && ! notBetter( rest , f_bbuf.a.back() ) ) ||
      ( ! f_del.done() && ! notBetter( rest , f_del.a.back() ) ) )
   return( false );
  }
 if( ! f_del.done() && ! f_bbuf.done() &&
     ! notBetter( f_bbuf , f_del.a.back() ) )
  return( false );
 if( ! sorted( f_del ) || ( f_ins.size() > M ) )
  return( false );
 for( const Group & g : f_groups ) {
  if( ! sorted( g.buf ) )
   return( false );
  n += g.buf.left();
  if( ! f_del.done() && ! notBetter( g.buf , f_del.a.back() ) )
   return( false );
  for( const Run & q : g.seqs ) {
   if( ! sorted( q ) )
    return( false );
   n += q.left();
   if( ! f_del.done() && ! notBetter( q , f_del.a.back() ) )
    return( false );
   if( ! g.buf.done() && ! notBetter( q , g.buf.a.back() ) )
    return( false );
   }
  }
 return( n == f_n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File SequenceHeap.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
