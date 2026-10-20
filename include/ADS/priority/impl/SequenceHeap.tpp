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
 *         Universita' di Pisa \n
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
 if( f_ins.size() == M ) {
  const T y = x;                            // x may be an item here
  flush();
  f_ins.push_back( y );
  }
 else
  f_ins.push_back( x );
 std::push_heap( f_ins.begin() , f_ins.end() ,
                 [ this ]( const T & a , const T & b ) {
                  return( worse( a , b ) ); } );
 ++f_n;
 ADS_INVARIANT( ordered() );
 }

template< class T , class Better >
inline const T & SequenceHeap< T , Better >::top( void )
{
 ADS_PRE( f_n > 0 );
 if( f_del.done() )
  refillDeletion();
 if( f_del.done() ||
     ( ! f_ins.empty() && f_better( f_ins.front() , f_del.a[ f_del.at ] ) ) )
  return( f_ins.front() );
 return( f_del.a[ f_del.at ] );
 }

template< class T , class Better >
inline T SequenceHeap< T , Better >::pop( void )
{
 ADS_PRE( f_n > 0 );
 if( f_del.done() )
  refillDeletion();
 const bool fromIns = f_del.done() ||
  ( ! f_ins.empty() && f_better( f_ins.front() , f_del.a[ f_del.at ] ) );
 --f_n;
 if( fromIns ) {
  std::pop_heap( f_ins.begin() , f_ins.end() ,
                 [ this ]( const T & a , const T & b ) {
                  return( worse( a , b ) ); } );
  T x = std::move( f_ins.back() );
  f_ins.pop_back();
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
 std::vector< T > key( L );
 std::vector< unsigned char > dead( L , 1 );
 for( std::size_t i = 0 ; i < k ; ++i ) {
  key[ i ] = R[ i ]->a[ R[ i ]->at ];
  dead[ i ] = 0;
  }
 const auto wins = [ & ]( std::size_t i , std::size_t j ) {
  return( ! dead[ i ] && ( dead[ j ] || ! f_better( key[ j ] , key[ i ] ) ) );
  };
 std::vector< std::size_t > W( 2 * L ) , Lose( L );
 for( std::size_t i = 0 ; i < L ; ++i )     // the matches, bottom up
  W[ L + i ] = i;
 for( std::size_t v = L - 1 ; v >= 1 ; --v ) {
  const std::size_t x = W[ 2 * v ] , y = W[ 2 * v + 1 ];
  const bool xw = wins( x , y );
  W[ v ] = xw ? x : y;
  Lose[ v ] = xw ? y : x;
  }
 std::size_t w = W[ 1 ];
 for( std::size_t taken = 0 ; ( taken < n ) && ! dead[ w ] ; ++taken ) {
  Run & r = *R[ w ];
  out.push_back( std::move( r.a[ r.at++ ] ) );
  if( r.done() )
   dead[ w ] = 1;
  else
   key[ w ] = r.a[ r.at ];
  for( std::size_t v = ( L + w ) / 2 ; v >= 1 ; v /= 2 ) {   // replay
   const std::size_t l = Lose[ v ];
   const bool up = wins( l , w );
   Lose[ v ] = up ? w : l;
   w = up ? l : w;
   }
  }
 }

template< class T , class Better >
void SequenceHeap< T , Better >::flush( void )
{
 Run S;                                     // the insertion heap, sorted
 S.a.swap( f_ins );
 f_ins.reserve( M );
 std::sort( S.a.begin() , S.a.end() , f_better );
 Run * B0 = f_groups.empty() ? nullptr : &f_groups[ 0 ].buf;
 if( ( f_del.done() || ! f_better( S.a.front() , f_del.a.back() ) ) &&
     ( ! B0 || B0->done() || ! f_better( S.a.front() , B0->a.back() ) ) ) {
  join( 0 , std::move( S ) );               // no better than the buffers
  return;
  }
 const std::size_t nd = f_del.left();
 Run * B = f_groups.empty() ? nullptr : &f_groups[ 0 ].buf;
 const std::size_t nb = B ? B->left() : 0;
 std::vector< T > U;
 U.reserve( S.a.size() + nd + nb );
 std::vector< Run * > R{ &S , &f_del };
 if( B )
  R.push_back( B );
 merge( R , U , 0 );
 auto at = [ & ]( std::size_t i ) {
  return( U.begin() + std::ptrdiff_t( i ) ); };
 f_del = Run();                             // the best nd stay best,
 f_del.a.assign( std::make_move_iterator( at( 0 ) ) ,
                 std::make_move_iterator( at( nd ) ) );
 if( B ) {                                  // the next nb in the buffer
  *B = Run();
  B->a.assign( std::make_move_iterator( at( nd ) ) ,
               std::make_move_iterator( at( nd + nb ) ) );
  }
 Run s;                                     // and the rest in group 1
 s.a.assign( std::make_move_iterator( at( nd + nb ) ) ,
             std::make_move_iterator( U.end() ) );
 f_ins.clear();
 f_ins.reserve( M );
 if( ! s.a.empty() )
  join( 0 , std::move( s ) );
 }

template< class T , class Better >
void SequenceHeap< T , Better >::join( std::size_t i , Run && s )
{
 if( i == f_groups.size() )
  f_groups.emplace_back();
 if( f_groups[ i ].seqs.size() == K ) {     // the group to the next one,
  std::vector< Run * > R;                   // with the buffer there
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

template< class T , class Better >
void SequenceHeap< T , Better >::refillDeletion( void )
{
 f_del.a.clear();
 f_del.at = 0;
 std::vector< Group * > G;                  // the groups with items, by
 for( Group & g : f_groups ) {              // the first of their buffer
  if( g.buf.done() )
   refill( g );
  if( ! g.buf.done() )
   G.push_back( &g );
  }
 const auto below = [ this ]( const Group * a , const Group * b ) {
  return( f_better( b->buf.a[ b->buf.at ] , a->buf.a[ a->buf.at ] ) ); };
 std::make_heap( G.begin() , G.end() , below );
 while( ! G.empty() && ( f_del.a.size() < M ) ) {
  std::pop_heap( G.begin() , G.end() , below );
  Group & g = *G.back();
  f_del.a.push_back( std::move( g.buf.a[ g.buf.at++ ] ) );
  if( g.buf.done() )                        // refilled at once, so that
   refill( g );                             // its sequences take part
  if( g.buf.done() )
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
                          f_better ) ); };
 // no item of the run r is better than the item x
 const auto notBetter = [ this ]( const Run & r , const T & x ) {
  for( std::size_t i = r.at ; i < r.a.size() ; ++i )
   if( f_better( r.a[ i ] , x ) )
    return( false );
  return( true ); };
 std::size_t n = f_ins.size() + f_del.left();
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
