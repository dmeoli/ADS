/*--------------------------------------------------------------------------*/
/*------------------------ File WorkingSetHeap.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of WorkingSetHeap [see WorkingSetHeap.h], with their
 * contracts, and the universal Dijkstra.
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

/*--------------------------------------------------------------------------*/
/*----------------------------- THE CONSTRUCTION ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class P , class A >
template< class... O >
WorkingSetHeap< K , P , A >::WorkingSetHeap( int n , O &... o ) : f_sub( o... )
{
 ADS_PRE( n >= 0 );
 f_N.resize( std::size_t( n ) );
 f_first.fill( -1 );
 f_B.fill( -1 );
 }

template< class K , class P , class A >
template< class... O >
WorkingSetHeap< K , P , A >::WorkingSetHeap( std::allocator_arg_t ,
					   const A & alloc , int n , O &... o )
    : f_N( alloc ) , f_sub( o... )
{
 ADS_PRE( n >= 0 );
 f_N.resize( std::size_t( n ) );
 f_first.fill( -1 );
 f_B.fill( -1 );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::reset( int n )
{
 ADS_PRE( n >= 0 );
 f_N.assign( std::size_t( n ) , Node{} );
 f_first.fill( -1 );
 f_size.fill( 0 );
 f_start.fill( 0 );
 f_k = 0;
 f_bits = 0;
 f_s = 0;
 if constexpr( heard< mod::Cleared > )
  if( alive< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::grow( int n )
{
 ADS_PRE( n >= ids() );
 const auto m = std::size_t( n );
 if( m > f_N.capacity() )  // doubled, whatever the library
  f_N.reserve( std::max( m , 2 * f_N.capacity() ) );
 f_N.resize( m );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
inline bool WorkingSetHeap< K , P , A >::contains( int id ) const
{
 ADS_PRE( ( id >= 0 ) && ( id < ids() ) );
 return( f_N[ std::size_t( id ) ].parent != OUT );
 }

template< class K , class P , class A >
inline const K & WorkingSetHeap< K , P , A >::key( int id ) const
{
 ADS_PRE( contains( id ) );
 return( f_N[ std::size_t( id ) ].key );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE OPERATIONS ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::push( int id , const K & k )
{
 ADS_PRE( ! contains( id ) );
 if constexpr( heard< Ins > )
  if( alive< Ins >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    pushCold( id , k );
   else {
    pushPlain( id , k );
    f_sub.emit( Ins{ id , k } );
    f_sub.audit( *this );
    }
   return;
   }
 pushPlain( id , k );
 ADS_POST( contains( id ) && ( f_k < L - 1 ) );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::decrease( int id , const K & k )
{
 ADS_PRE( contains( id ) && ! ( f_N[ std::size_t( id ) ].key < k ) );
 if constexpr( heard< Chg > )
  if( alive< Chg >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    decreaseCold( id , k );
   else {
    const K old = f_N[ std::size_t( id ) ].key;
    decreasePlain( id , k );
    f_sub.emit( Chg{ id , old , k } );
    f_sub.audit( *this );
    }
   return;
   }
 decreasePlain( id , k );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::increase( int id , const K & k )
{
 ADS_PRE( contains( id ) && ! ( k < f_N[ std::size_t( id ) ].key ) );
 if constexpr( heard< Chg > )
  if( alive< Chg >() ) [[unlikely]] {
   increaseCold( id , k );
   return;
   }
 erasePlain( id );
 pushPlain( id , k );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::update( int id , const K & k )
{
 ADS_PRE( contains( id ) );
 if( k < f_N[ std::size_t( id ) ].key )
  decrease( id , k );
 else
  increase( id , k );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::erase( int id )
{
 ADS_PRE( contains( id ) );
 if constexpr( heard< Era > )
  if( alive< Era >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    eraseCold( id );
   else {
    f_sub.emit( Era{ id , f_N[ std::size_t( id ) ].key } );  // before
    erasePlain( id );
    f_sub.audit( *this );
    }
   return;
   }
 erasePlain( id );
 ADS_POST( ! contains( id ) );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
inline bool WorkingSetHeap< K , P , A >::pushOrDecrease( int id , const K & k )
{
 ADS_PRE( ( id >= 0 ) && ( id < ids() ) );
 const Node & x = f_N[ std::size_t( id ) ];
 if( x.parent == OUT )
  push( id , k );
 else if( k < x.key )
  decrease( id , k );
 else
  return( false );
 return( true );
 }

template< class K , class P , class A >
inline typename WorkingSetHeap< K , P , A >::Entry
WorkingSetHeap< K , P , A >::top( void ) const
{
 ADS_PRE( ! empty() );
 const int x = f_first[ std::size_t( next( 1 ) ) ];
 return( Entry{ f_N[ std::size_t( x ) ].key , x } );
 }

template< class K , class P , class A >
inline typename WorkingSetHeap< K , P , A >::Entry
WorkingSetHeap< K , P , A >::pop( void )
{
 ADS_PRE( ! empty() );
 if constexpr( heard< Era > )
  if( alive< Era >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( popCold() );
   else {
    const Entry e = top();
    f_sub.emit( Era{ e.id , e.key } );  // before the removal
    popPlain();
    f_sub.audit( *this );
    return( e );
    }
   }
 const Entry e = popPlain();
 ADS_POST( empty() || ! ( top().key < e.key ) );
 ADS_INVARIANT( ordered() );
 return( e );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::clear( void )
{
 std::vector< int > S;  // the children still to clear
 for( int i = 1 ; i <= f_k ; ++i ) {
  const int f = f_first[ std::size_t( i ) ];
  if( f < 0 )
   continue;
  int r = f;
  do {
   Node & R = f_N[ std::size_t( r ) ];
   const int nx = R.right;
   if( R.left >= 0 )
    S.push_back( R.left );
   R = Node{};
   r = nx;
   } while( r != f );
  }
 while( ! S.empty() ) {
  Node & V = f_N[ std::size_t( S.back() ) ];
  S.pop_back();
  for( int c : { V.left , V.right } )
   if( c >= 0 )
    S.push_back( c );
  V = Node{};
  }
 f_first.fill( -1 );
 f_size.fill( 0 );
 f_start.fill( 0 );
 f_k = 0;
 f_bits = 0;
 f_s = 0;
 if constexpr( heard< mod::Cleared > )
  if( alive< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::extract( const K & theta ,
					 std::vector< int > & out )
{
 while( ! empty() && ! ( theta < top().key ) )
  out.push_back( pop().id );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE OPERATIONS, PLAIN --------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::pushPlain( int id , const K & k )
{
 Node & z = f_N[ std::size_t( id ) ];  // H_0, of one item
 z.key = k;
 z.left = -1;
 z.right = id;
 z.parent = ROOT;
 z.rank = 0;
 z.stamp = ++f_t;
 f_first[ 0 ] = id;
 f_size[ 0 ] = 1;
 f_start[ 0 ] = z.stamp;
 int j = 0;  // the least j whose meld fits
 while( ( j < f_k ) &&
	( f_size[ std::size_t( j ) ] + f_size[ std::size_t( j + 1 ) ] >
	  cap( j + 1 ) ) )
  ++j;
 const auto J = std::size_t( j );
 if( j < f_k ) {  // H_j melded into H_{j+1}
  const int a = f_first[ J ] , b = f_first[ J + 1 ];
  if( a >= 0 ) {
   if( b < 0 )
    f_first[ J + 1 ] = a;
   else {  // the two lists of roots made one
    std::swap( f_N[ std::size_t( a ) ].right , f_N[ std::size_t( b ) ].right );
    if( less( a , b ) )
     f_first[ J + 1 ] = a;
    }
   }
  f_size[ J + 1 ] += f_size[ J ];
  }
 else {  // H_k moved to H_{k+1}
  f_first[ J + 1 ] = f_first[ J ];
  f_size[ J + 1 ] = f_size[ J ];
  f_start[ J + 1 ] = f_start[ J ];
  ++f_k;
  }
 for( std::size_t i = J ; i-- > 0 ; ) {  // one place up
  f_first[ i + 1 ] = f_first[ i ];
  f_size[ i + 1 ] = f_size[ i ];
  f_start[ i + 1 ] = f_start[ i ];
  }
 f_first[ 0 ] = -1;
 f_size[ 0 ] = 0;
 f_start[ 0 ] = 0;
 ++f_s;
 if( j > 0 )
  suffix( j + 1 );
 else if( f_first[ 1 ] == id )  // H_0 into H_1: the bits change only if
  lowered( 1 , id );             // the new item is the least of H_1
 }

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::decreasePlain( int id , const K & k )
{
 Node & x = f_N[ std::size_t( id ) ];
 const int i = heapOf( x.stamp );
 const auto I = std::size_t( i );
 x.key = k;
 if( x.parent != ROOT )
  cut( i , id );  // a root now, first if least
 else if( less( id , f_first[ I ] ) )
  f_first[ I ] = id;
 if( f_first[ I ] == id )  // else the least of H_i unchanged
  lowered( i , id );
 }

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::lowered( int i , int x )
{
 const int nx = next( i + 1 );
 if( ( nx >= 0 ) && ! less( x , f_first[ std::size_t( nx ) ] ) ) {
  f_bits &= ~( 1u << unsigned( i ) );
  return;
  }
 f_bits |= 1u << unsigned( i );  // the suffix minima before H_i that
 for( int j = prev( i - 1 ) ; ( j >= 1 ) &&  // the key beats
			     ! less( f_first[ std::size_t( j ) ] , x ) ;
      j = prev( j - 1 ) )
  f_bits &= ~( 1u << unsigned( j ) );
 }

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::erasePlain( int id )
{
 const int i = heapOf( f_N[ std::size_t( id ) ].stamp );
 if( f_N[ std::size_t( id ) ].parent != ROOT )
  cut( i , id );
 removeRoot( i , id );
 --f_s;
 suffix( i );
 }

template< class K , class P , class A >
inline typename WorkingSetHeap< K , P , A >::Entry
WorkingSetHeap< K , P , A >::popPlain( void )
{
 const int j = next( 1 );
 const int x = f_first[ std::size_t( j ) ];
 const Entry e{ f_N[ std::size_t( x ) ].key , x };
 removeRoot( j , x );
 --f_s;
 suffix( j );
 return( e );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- WHEN SOMEONE LISTENS ---------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::pushCold( int id , const K & k )
{
 pushPlain( id , k );
 f_sub.emit( Ins{ id , k } );
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::decreaseCold( int id , const K & k )
{
 const K old = f_N[ std::size_t( id ) ].key;
 decreasePlain( id , k );
 f_sub.emit( Chg{ id , old , k } );
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::increaseCold( int id , const K & k )
{
 const K old = f_N[ std::size_t( id ) ].key;
 erasePlain( id );
 pushPlain( id , k );
 f_sub.emit( Chg{ id , old , k } );
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::eraseCold( int id )
{
 f_sub.emit( Era{ id , f_N[ std::size_t( id ) ].key } );  // before
 erasePlain( id );
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 }

template< class K , class P , class A >
typename WorkingSetHeap< K , P , A >::Entry
WorkingSetHeap< K , P , A >::popCold( void )
{
 const Entry e = top();
 f_sub.emit( Era{ e.id , e.key } );  // before the removal
 popPlain();
 f_sub.audit( *this );
 ADS_INVARIANT( ordered() );
 return( e );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE INNER HEAPS -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::suffix( int h )
{
 const int nx = next( h + 1 );
 int cur = nx >= 0 ? f_first[ std::size_t( nx ) ] : -1;
 for( int i = h ; i >= 1 ; --i ) {
  const int r = f_first[ std::size_t( i ) ];
  const unsigned bit = 1u << unsigned( i );
  if( ( r >= 0 ) && ( ( cur < 0 ) || less( r , cur ) ) ) {
   f_bits |= bit;
   cur = r;
   }
  else
   f_bits &= ~bit;
  }
 }

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::addRoot( int i , int x )
{
 const auto I = std::size_t( i );
 const int f = f_first[ I ];
 Node & X = f_N[ std::size_t( x ) ];
 if( f < 0 ) {
  X.right = x;
  f_first[ I ] = x;
  return;
  }
 Node & F = f_N[ std::size_t( f ) ];  // x after the first root
 X.right = F.right;
 F.right = x;
 if( less( x , f ) )
  f_first[ I ] = x;
 }

template< class K , class P , class A >
inline int WorkingSetHeap< K , P , A >::link( int u , int v )
{
 if( less( v , u ) )
  std::swap( u , v );
 Node & U = f_N[ std::size_t( u ) ];
 Node & V = f_N[ std::size_t( v ) ];
 V.right = U.left;  // the old left subtree of u under v
 if( V.right >= 0 )
  f_N[ std::size_t( V.right ) ].parent = v;
 U.left = v;
 V.parent = u;
 ++U.rank;
 return( u );
 }

template< class K , class P , class A >
inline void WorkingSetHeap< K , P , A >::cut( int i , int x )
{
 Node & X = f_N[ std::size_t( x ) ];
 const int y = X.right , u = X.parent;
 Node & U = f_N[ std::size_t( u ) ];
 if( U.left == x )  // y in the place of x
  U.left = y;
 else
  U.right = y;
 if( y >= 0 )
  f_N[ std::size_t( y ) ].parent = u;
 X.parent = ROOT;
 X.rank = rank( X.left ) + 1;
 addRoot( i , x );
 for( int v = u ; ; ) {  // the ranks of type 1 from u up
  Node & V = f_N[ std::size_t( v ) ];
  if( V.parent == ROOT ) {
   V.rank = rank( V.left ) + 1;
   break;
   }
  const int a = rank( V.left ) , c = rank( V.right );
  const int r = ( a != c ) ? std::max( a , c ) : a + 1;
  if( r >= V.rank )
   break;
  V.rank = r;
  v = V.parent;
  }
 }

template< class K , class P , class A >
void WorkingSetHeap< K , P , A >::removeRoot( int i , int x )
{
 int out = -1;                // the least root of the new list
 int high = -1;               // the largest rank in f_B
 auto add = [ & ]( int t ) {  // t among the roots
  Node & T = f_N[ std::size_t( t ) ];
  T.parent = ROOT;
  if( out < 0 ) {
   T.right = t;
   out = t;
   return;
   }
  Node & O = f_N[ std::size_t( out ) ];
  T.right = O.right;
  O.right = t;
  if( less( t , out ) )
   out = t;
  };
 auto put = [ & ]( int t ) {  // t to its bucket, or linked
  const int r = f_N[ std::size_t( t ) ].rank;
  ADS_INVARIANT( ( r >= 0 ) && ( r < 64 ) );  // r <= log2 s
  const int u = f_B[ std::size_t( r ) ];
  if( u < 0 ) {
   f_B[ std::size_t( r ) ] = t;
   high = std::max( high , r );
   }
  else {
   f_B[ std::size_t( r ) ] = -1;
   add( link( u , t ) );
   }
  };
 for( int r = f_N[ std::size_t( x ) ].right ; r != x ; ) {  // the other
  const int nx = f_N[ std::size_t( r ) ].right;           // roots
  put( r );
  r = nx;
  }
 for( int y = f_N[ std::size_t( x ) ].left ; y >= 0 ; ) {  // the spine
  Node & Y = f_N[ std::size_t( y ) ];
  const int nx = Y.right;
  Y.right = -1;
  Y.parent = ROOT;
  Y.rank = rank( Y.left ) + 1;
  put( y );
  y = nx;
  }
 for( int r = 0 ; r <= high ; ++r )
  if( f_B[ std::size_t( r ) ] >= 0 ) {
   add( f_B[ std::size_t( r ) ] );
   f_B[ std::size_t( r ) ] = -1;
   }
 f_first[ std::size_t( i ) ] = out;
 --f_size[ std::size_t( i ) ];
 f_N[ std::size_t( x ) ] = Node{};
 }

/*--------------------------------------------------------------------------*/
/*----------------------------- THE INVARIANT ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class K , class P , class A >
bool WorkingSetHeap< K , P , A >::tree( int v , int p , int beta ,
					std::uint64_t lo ,
				      std::uint64_t hi ,
				      std::size_t & c ) const
{
 struct Visit
 {
  int v , p , beta;
  };
 std::vector< Visit > S{ Visit{ v , p , beta } };
 while( ! S.empty() ) {
  const Visit t = S.back();
  S.pop_back();
  const Node & V = f_N[ std::size_t( t.v ) ];
  if( ( V.parent != t.p ) || less( t.v , t.beta ) || ( V.stamp < lo ) ||
      ( V.stamp >= hi ) || ( ++c > f_s ) )
   return( false );
  const int a = V.rank - rank( V.left ) , b = V.rank - rank( V.right );
  if( ! ( ( ( a == 1 ) && ( b == 1 ) ) || ( ( a == 0 ) && ( b > 0 ) ) ||
	  ( ( b == 0 ) && ( a > 0 ) ) ) )
   return( false );  // the rule of type 1
  if( V.left >= 0 )
   S.push_back( Visit{ V.left , t.v , t.v } );
  if( V.right >= 0 )
   S.push_back( Visit{ V.right , t.v , t.beta } );
  }
 return( true );
 }

template< class K , class P , class A >
bool WorkingSetHeap< K , P , A >::ordered( void ) const
{
 if( f_s > 1024 )
  return( true );
 if( ( f_first[ 0 ] != -1 ) || ( f_size[ 0 ] != 0 ) || ( f_bits & 1u ) ||
     ( f_k >= L - 1 ) )
  return( false );
 std::size_t total = 0;
 for( int i = 1 ; i < L ; ++i ) {
  const auto I = std::size_t( i );
  const int f = f_first[ I ];
  if( i > f_k ) {  // the places not in use
   if( ( f != -1 ) || ( f_size[ I ] != 0 ) || ( f_start[ I ] != 0 ) )
    return( false );
   continue;
   }
  const std::uint64_t lo = f_start[ I ] ,
		      hi = ( i == 1 )
			     ? std::numeric_limits< std::uint64_t >::max()
			     : f_start[ I - 1 ];
  if( ( lo == 0 ) || ! ( lo < hi ) || ( f_size[ I ] > cap( i ) ) )
   return( false );
  std::size_t c = 0;
  if( f >= 0 ) {
   int r = f;
   do {
    const Node & R = f_N[ std::size_t( r ) ];
    if( ( R.parent != ROOT ) || less( r , f ) ||
	( R.rank != rank( R.left ) + 1 ) || ( R.stamp < lo ) ||
	( R.stamp >= hi ) || ( ++c > f_s ) )
     return( false );
    if( ( R.left >= 0 ) && ! tree( R.left , r , r , lo , hi , c ) )
     return( false );
    r = R.right;
    } while( r != f );
   }
  if( c != f_size[ I ] )
   return( false );
  total += c;
  }
 std::size_t in = 0;
 for( const Node & v : f_N )
  in += ( v.parent != OUT ) ? 1 : 0;
 if( ( total != f_s ) || ( in != f_s ) )
  return( false );
 unsigned b = 0;  // the bits of the suffix minima
 for( int i = f_k , cur = -1 ; i >= 1 ; --i ) {
  const int r = f_first[ std::size_t( i ) ];
  if( ( r >= 0 ) && ( ( cur < 0 ) || less( r , cur ) ) ) {
   b |= 1u << unsigned( i );
   cur = r;
   }
  }
 return( b == f_bits );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE UNIVERSAL DIJKSTRA -------------------------*/
/*--------------------------------------------------------------------------*/

template< Graph G >
auto universalDijkstra( const G & g , int r )
{
 ADS_PRE( g.hasNode( r ) );
 const int n = g.bound();
 WorkingSetHeap< typename G::Weight > Q( std::max( n , 1 ) );
 PriorityFrontier S( Q , n );
 return( shortestPaths( g , r , S ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File WorkingSetHeap.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
