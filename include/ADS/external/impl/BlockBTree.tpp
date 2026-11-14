/*--------------------------------------------------------------------------*/
/*--------------------------- File BlockBTree.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BlockBTree [see BlockBTree.h], with their contracts.
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

template< std::totally_ordered K >
BlockBTree< K >::Held::Held( const BlockBTree * tree , Node n )
 : node( std::move( n ) ) , t( tree )
{
 if( ++t->f_held > t->f_peak )
  t->f_peak = t->f_held;
 }

template< std::totally_ordered K >
BlockBTree< K >::Held::~Held()
{
 --t->f_held;
 }

template< std::totally_ordered K >
BlockBTree< K >::BlockBTree( int B , BTreePolicy p , TwoLevelMemory * mem )
 : f_bs( mem ) , f_B( B ) , f_policy( p )
{
 ADS_PRE( B >= 2 );
 f_root = f_bs.placeBlock( Node() );
 }

template< std::totally_ordered K >
std::size_t BlockBTree< K >::maxKeys( void ) const
{
 return( std::size_t( f_policy == BTreePolicy::WIDE ? 3 * f_B - 1 :
                                                      2 * f_B - 1 ) );
 }

template< std::totally_ordered K >
std::size_t BlockBTree< K >::place( const Node & u , const K & x )
{
 return( std::size_t( std::lower_bound( u.keys.begin() , u.keys.end() , x )
                      - u.keys.begin() ) );
 }

template< std::totally_ordered K >
void BlockBTree< K >::path( std::size_t len ) const
{
 if( len > f_peakPath )
  f_peakPath = len;
 }

template< std::totally_ordered K >
std::optional< K > BlockBTree< K >::lowerBound( const K & x ) const
{
//! [BlockBTree-lowerBound]
 std::optional< K > z;
 std::size_t u = f_root;
 Held cur( this , f_bs.readBlock( u ) );
 for( ; ; ) {
  const Node & a = cur.node;
  const std::size_t i = place( a , x );
  if( i < a.keys.size() ) {
   if( a.keys[ i ] == x )
    return( x );
   z = a.keys[ i ];
   }
  if( a.kids.empty() )
   return( z );
  u = a.kids[ i ];
  cur.node = f_bs.readBlock( u );
  }
//! [BlockBTree-lowerBound]
 }

template< std::totally_ordered K >
bool BlockBTree< K >::contains( const K & x ) const
{
 const auto z = lowerBound( x );
 return( z && ( *z == x ) );
 }

template< std::totally_ordered K >
template< class D >
void BlockBTree< K >::deal( Node & p , std::size_t f , std::size_t k ,
                            std::size_t c , const Node & known ,
                            D && decide )
{
//! [BlockBTree-deal]
 // the keys and the children gathered, which count as k nodes in memory
 f_held += k;
 if( f_held > f_peak )
  f_peak = f_held;
 const bool leaf = known.kids.empty();
 std::vector< K > A;
 std::vector< std::size_t > C , ids( k );
 A.reserve( k * ( maxKeys() + 2 ) );
 if( ! leaf )
  C.reserve( k * ( maxKeys() + 2 ) );
 for( std::size_t j = 0 ; j < k ; ++j ) {
  ids[ j ] = p.kids[ f + j ];
  const Node v = ( f + j == c ) ? known : f_bs.readBlock( ids[ j ] );
  A.insert( A.end() , v.keys.begin() , v.keys.end() );
  C.insert( C.end() , v.kids.begin() , v.kids.end() );
  if( j + 1 < k )
   A.push_back( p.keys[ f + j ] );
  }
 const std::size_t k2 = decide( A.size() );
 const std::size_t T = A.size() - ( k2 - 1 );
 const std::size_t base = T / k2 , extra = T % k2;
 std::vector< std::size_t > nid( k2 );
 std::vector< K > sep;
 std::size_t a = 0 , b = 0;
 for( std::size_t j = 0 ; j < k2 ; ++j ) {
  const std::size_t sz = base + ( j < extra ? 1 : 0 );
  Node w;
  w.keys.assign( A.begin() + std::ptrdiff_t( a ) ,
                 A.begin() + std::ptrdiff_t( a + sz ) );
  a += sz;
  if( ! leaf ) {
   w.kids.assign( C.begin() + std::ptrdiff_t( b ) ,
                  C.begin() + std::ptrdiff_t( b + sz + 1 ) );
   b += sz + 1;
   }
  if( j + 1 < k2 )
   sep.push_back( A[ a++ ] );
  if( j < k ) {
   nid[ j ] = ids[ j ];
   f_bs.writeBlock( nid[ j ] , w );
   }
  else
   nid[ j ] = f_bs.placeBlock( w );
  }
 for( std::size_t j = k2 ; j < k ; ++j )
  f_bs.freeBlock( ids[ j ] );
 p.keys.erase( p.keys.begin() + std::ptrdiff_t( f ) ,
               p.keys.begin() + std::ptrdiff_t( f + k - 1 ) );
 p.keys.insert( p.keys.begin() + std::ptrdiff_t( f ) , sep.begin() ,
                sep.end() );
 p.kids.erase( p.kids.begin() + std::ptrdiff_t( f ) ,
               p.kids.begin() + std::ptrdiff_t( f + k ) );
 p.kids.insert( p.kids.begin() + std::ptrdiff_t( f ) , nid.begin() ,
                nid.end() );
 if( k2 > k )
  ++f_splits;
 else if( k2 < k )
  ++f_merges;
 else
  ++f_borrows;
 f_held -= k;
//! [BlockBTree-deal]
 }

template< std::totally_ordered K >
void BlockBTree< K >::fixOverflow( Node & p , std::size_t i , const Node & u )
{
//! [BlockBTree-fixOverflow]
 const std::size_t M = maxKeys() , slack = std::size_t( f_B / 4 );
 if( ( f_policy == BTreePolicy::THREE ) && ( p.kids.size() >= 2 ) ) {
  const std::size_t s = ( i + 1 < p.kids.size() ) ? i + 1 : i - 1;
  // two nodes if each keeps B / 4 keys of room, three otherwise
  deal( p , std::min( i , s ) , 2 , i , u , [ & ]( std::size_t t ) {
   return( ( t - 1 + 1 ) / 2 <= M - slack ? std::size_t( 2 ) :
                                            std::size_t( 3 ) ); } );
  }
 else
  deal( p , i , 1 , i , u , []( std::size_t ) {
   return( std::size_t( 2 ) ); } );
//! [BlockBTree-fixOverflow]
 }

template< std::totally_ordered K >
std::size_t BlockBTree< K >::lowKeys( bool twoLeaves ) const
{
 return( ( f_policy == BTreePolicy::THREE ) && twoLeaves ?
         std::max< std::size_t >( 1 , std::size_t( f_B / 2 ) ) : minKeys() );
 }

template< std::totally_ordered K >
void BlockBTree< K >::fixUnderflow( Node & p , std::size_t i ,
                                    const Node & u , bool root )
{
//! [BlockBTree-fixUnderflow]
 const std::size_t M = maxKeys() , m = minKeys();
 const std::size_t slack = std::size_t( f_B / 4 );
 const std::size_t B = std::size_t( f_B );
 if( ( f_policy == BTreePolicy::THREE ) && root && ( p.kids.size() == 2 ) &&
     u.kids.empty() ) {   // two leaves: merged while B / 4 below the top
  deal( p , 0 , 2 , i , u , [ & ]( std::size_t t ) {
   return( t <= M - slack ? std::size_t( 1 ) : std::size_t( 2 ) ); } );
  return;
  }
 if( ( f_policy == BTreePolicy::THREE ) && ( p.kids.size() >= 3 ) ) {
  const std::size_t f = std::min( i > 0 ? i - 1 : 0 , p.kids.size() - 3 );
  deal( p , f , 3 , i , u , [ & ]( std::size_t t ) {
   return( ( t - 1 + 1 ) / 2 <= M - slack ? std::size_t( 2 ) :
                                            std::size_t( 3 ) ); } );
  return;
  }
 // the sibling on the left, on the right for the first child
 const std::size_t f = i > 0 ? i - 1 : i;
 if( f_policy == BTreePolicy::WIDE )   // merged while they fit in 2.5 B
  deal( p , f , 2 , i , u , [ & ]( std::size_t t ) {
   return( 2 * t <= 5 * B ? std::size_t( 1 ) : std::size_t( 2 ) ); } );
 else                                  // borrow while both keep B - 1
  deal( p , f , 2 , i , u , [ & ]( std::size_t t ) {
   return( t - 1 >= 2 * m ? std::size_t( 2 ) : std::size_t( 1 ) ); } );
//! [BlockBTree-fixUnderflow]
 }

template< std::totally_ordered K >
bool BlockBTree< K >::insert( const K & x )
{
 std::vector< std::pair< std::size_t , std::size_t > > P;
 std::size_t u = f_root , i;
 Held cur( this , f_bs.readBlock( u ) );
//! [BlockBTree-insert]
 for( ; ; ) {   // down to the leaf, the path kept as indices
  i = place( cur.node , x );
  if( ( i < cur.node.keys.size() ) && ( cur.node.keys[ i ] == x ) )
   return( false );
  if( cur.node.kids.empty() )
   break;
  P.emplace_back( u , i );
  u = cur.node.kids[ i ];
  cur.node = f_bs.readBlock( u );
  }
 path( P.size() );
 cur.node.keys.insert( cur.node.keys.begin() + std::ptrdiff_t( i ) , x );
 f_bs.writeBlock( u , cur.node );
 ++f_n;
 while( cur.node.keys.size() > maxKeys() ) {   // up, while too full
  Held p( this );
  std::size_t q , j;
  if( P.empty() ) {   // a new root
   p.node.kids.push_back( u );
   q = f_bs.placeBlock( p.node );
   f_root = q;
   j = 0;
   }
  else {
   std::tie( q , j ) = P.back();
   P.pop_back();
   p.node = f_bs.readBlock( q );
   }
  fixOverflow( p.node , j , cur.node );
  f_bs.writeBlock( q , p.node );
  cur.node = std::move( p.node );
  u = q;
  }
//! [BlockBTree-insert]
 ADS_POST( contains( x ) );
 ADS_INVARIANT( consistent() );
 return( true );
 }

template< std::totally_ordered K >
bool BlockBTree< K >::remove( const K & x )
{
 constexpr std::size_t NONE = std::numeric_limits< std::size_t >::max();
 std::vector< std::pair< std::size_t , std::size_t > > P;
 std::size_t u = f_root , target = NONE , tIdx = 0 , leafIdx = 0;
 Held cur( this , f_bs.readBlock( u ) );
 const bool twoKids = cur.node.kids.size() == 2;   // the root's
//! [BlockBTree-remove]
 for( ; ; ) {
  if( target != NONE ) {   // down the leftmost path to the successor
   if( cur.node.kids.empty() )
    break;
   P.emplace_back( u , 0 );
   u = cur.node.kids[ 0 ];
   }
  else {
   const std::size_t i = place( cur.node , x );
   const bool found = ( i < cur.node.keys.size() ) &&
                      ( cur.node.keys[ i ] == x );
   if( cur.node.kids.empty() ) {
    if( ! found )
     return( false );
    leafIdx = i;
    break;
    }
   if( found ) {   // x in an internal node: its successor replaces it
    target = u;
    tIdx = i;
    P.emplace_back( u , i + 1 );
    u = cur.node.kids[ i + 1 ];
    }
   else {
    P.emplace_back( u , i );
    u = cur.node.kids[ i ];
    }
   }
  cur.node = f_bs.readBlock( u );
  }
 path( P.size() );
 K y = x;
 if( target == NONE )
  cur.node.keys.erase( cur.node.keys.begin() + std::ptrdiff_t( leafIdx ) );
 else {
  y = cur.node.keys.front();
  cur.node.keys.erase( cur.node.keys.begin() );
  }
 f_bs.writeBlock( u , cur.node );
 --f_n;
 bool replaced = target == NONE;
 // a leaf under a root of two children may hold fewer keys with THREE
 auto low = [ & ]( void ) {
  return( lowKeys( twoKids && ( P.size() == 1 ) && cur.node.kids.empty() ) );
  };
 while( ! P.empty() && ( cur.node.keys.size() < low() ) ) {
  const auto [ q , j ] = P.back();
  P.pop_back();
  Held p( this , f_bs.readBlock( q ) );
  if( ! replaced && ( q == target ) ) {
   p.node.keys[ tIdx ] = y;
   replaced = true;
   }
  fixUnderflow( p.node , j , cur.node , P.empty() );
  f_bs.writeBlock( q , p.node );
  cur.node = std::move( p.node );
  u = q;
  }
 if( ! replaced ) {   // the underflows stopped below the node of x
  Held t( this , f_bs.readBlock( target ) );
  t.node.keys[ tIdx ] = y;
  f_bs.writeBlock( target , t.node );
  }
 if( ( u == f_root ) && cur.node.keys.empty() && ! cur.node.kids.empty() ) {
  f_root = cur.node.kids.front();   // the root left with one child
  f_bs.freeBlock( u );
  }
//! [BlockBTree-remove]
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( consistent() );
 return( true );
 }

template< std::totally_ordered K >
int BlockBTree< K >::height( void ) const
{
 int h = 1;
 for( std::size_t u = f_root ; ! f_bs.peek( u ).kids.empty() ; ++h )
  u = f_bs.peek( u ).kids.front();
 return( h );
 }

template< std::totally_ordered K >
std::size_t BlockBTree< K >::internalNodes( void ) const
{
 std::size_t I = 0;
 std::vector< std::size_t > S{ f_root };
 while( ! S.empty() ) {
  const Node & v = f_bs.peek( S.back() );
  S.pop_back();
  if( ! v.kids.empty() ) {
   ++I;
   S.insert( S.end() , v.kids.begin() , v.kids.end() );
   }
  }
 return( I );
 }

template< std::totally_ordered K >
auto BlockBTree< K >::levels( void ) const
 -> std::vector< std::vector< std::vector< K > > >
{
 std::vector< std::vector< std::vector< K > > > L;
 std::vector< std::size_t > level{ f_root };
 while( ! level.empty() ) {
  std::vector< std::vector< K > > keys;
  std::vector< std::size_t > next;
  for( const std::size_t u : level ) {
   const Node & v = f_bs.peek( u );
   keys.push_back( v.keys );
   next.insert( next.end() , v.kids.begin() , v.kids.end() );
   }
  L.push_back( std::move( keys ) );
  level = std::move( next );
  }
 return( L );
 }

template< std::totally_ordered K >
void BlockBTree< K >::assign(
                   const std::vector< std::vector< std::vector< K > > > & L )
{
 // the old blocks given back
 std::vector< std::size_t > S{ f_root };
 while( ! S.empty() ) {
  const std::size_t u = S.back();
  S.pop_back();
  const Node & v = f_bs.peek( u );
  S.insert( S.end() , v.kids.begin() , v.kids.end() );
  f_bs.freeBlock( u );
  }
 f_n = 0;
 std::vector< std::size_t > below;   // the blocks of the level below
 for( std::size_t d = L.size() ; d-- > 0 ; ) {
  std::vector< std::size_t > here;
  std::size_t c = 0;
  for( const auto & keys : L[ d ] ) {
   Node w;
   w.keys = keys;
   f_n += keys.size();
   if( d + 1 < L.size() ) {
    ADS_PRE( c + keys.size() + 1 <= below.size() );
    w.kids.assign( below.begin() + std::ptrdiff_t( c ) ,
                   below.begin() + std::ptrdiff_t( c + keys.size() + 1 ) );
    c += keys.size() + 1;
    }
   here.push_back( f_bs.placeBlock( w ) );
   }
  ADS_PRE( ( d + 1 == L.size() ) || ( c == below.size() ) );
  below = std::move( here );
  }
 ADS_PRE( below.size() == 1 );
 f_root = below.front();
 ADS_PRE( consistent() );
 }

template< std::totally_ordered K >
template< class F >
void BlockBTree< K >::forEach( F && f ) const
{
 // an explicit stack of ( block , next child )
 std::vector< std::pair< std::size_t , std::size_t > > S{ { f_root , 0 } };
 while( ! S.empty() ) {
  auto & [ u , j ] = S.back();
  const Node & v = f_bs.peek( u );
  if( v.kids.empty() ) {
   for( const K & k : v.keys )
    f( k );
   S.pop_back();
   continue;
   }
  if( j > 0 && j <= v.keys.size() )
   f( v.keys[ j - 1 ] );
  if( j > v.keys.size() ) {
   S.pop_back();
   continue;
   }
  const std::size_t c = v.kids[ j++ ];
  S.emplace_back( c , 0 );
  }
 }

template< std::totally_ordered K >
bool BlockBTree< K >::valid( std::size_t u , const K * lo , const K * hi ,
                             int d , int & h , std::size_t & n ) const
{
 const Node & v = f_bs.peek( u );
 const std::size_t s = v.keys.size();
 const bool two = ( d == 1 ) && v.kids.empty() &&
                  ( f_bs.peek( f_root ).kids.size() == 2 );
 if( ( s > maxKeys() ) || ( ( u != f_root ) && ( s < lowKeys( two ) ) ) )
  return( false );
 for( std::size_t i = 0 ; i < s ; ++i )
  if( ( i > 0 && !( v.keys[ i - 1 ] < v.keys[ i ] ) ) ||
      ( lo && !( *lo < v.keys[ i ] ) ) || ( hi && !( v.keys[ i ] < *hi ) ) )
   return( false );
 n += s;
 if( v.kids.empty() ) {
  if( h < 0 )
   h = d;
  return( h == d );
  }
 if( v.kids.size() != s + 1 )
  return( false );
 for( std::size_t i = 0 ; i <= s ; ++i )
  if( ! valid( v.kids[ i ] , i > 0 ? &v.keys[ i - 1 ] : lo ,
               i < s ? &v.keys[ i ] : hi , d + 1 , h , n ) )
   return( false );
 return( true );
 }

template< std::totally_ordered K >
bool BlockBTree< K >::consistent( void ) const
{
 if( f_n > 1024 )
  return( true );
 int h = -1;
 std::size_t n = 0;
 return( valid( f_root , nullptr , nullptr , 0 , h , n ) && ( n == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BlockBTree.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
