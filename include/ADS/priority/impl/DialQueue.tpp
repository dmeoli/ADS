/*--------------------------------------------------------------------------*/
/*--------------------------- File DialQueue.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DialQueue [see DialQueue.h], with their contracts.
 * The hot ones are inline, so that the compiler expands them in the loop
 * of the caller.
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

template< class T , std::integral K , class P >
DialQueue< T , K , P >::DialQueue( std::size_t c )
{
 const std::size_t w = std::max< std::size_t >( std::bit_ceil( c + 1 ) , 64 );
 f_B.resize( w );
 f_bits.assign( w / 64 , 0 );
 }

template< class T , std::integral K , class P >
template< class... O >
 requires( sizeof...( O ) > 0 )
DialQueue< T , K , P >::DialQueue( std::size_t c , O &... o ) : f_sub( o... )
{
 const std::size_t w = std::max< std::size_t >( std::bit_ceil( c + 1 ) , 64 );
 f_B.resize( w );
 f_bits.assign( w / 64 , 0 );
 }

template< class T , std::integral K , class P >
inline void DialQueue< T , K , P >::put( const T & x , U u )
{
 const std::size_t b = std::size_t( u ) & ( f_B.size() - 1 );
 f_B[ b ].push_back( x );
 f_bits[ b >> 6 ] |= std::uint64_t( 1 ) << ( b & 63 );
 ++f_in;
 }

template< class T , std::integral K , class P >
inline void DialQueue< T , K , P >::push( const T & x , K k )
{
 if constexpr( heard< Ins > )
  if( f_sub.template live< Ins >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    pushCold( x , k );
   else {
    pushPlain( x , k );
    f_sub.emit( Ins{ x , k } );
    f_sub.audit( *this );
    }
   return;
   }
 pushPlain( x , k );
 }

template< class T , std::integral K , class P >
inline void DialQueue< T , K , P >::pushPlain( const T & x , K k )
{
 const U u = encode( k );
 ADS_PRE( u >= f_cur );
 if( U( u - f_cur ) < U( f_B.size() ) )
  put( x , u );
 else
  f_over.push( x , u );
 ++f_n;
 ADS_INVARIANT( counted() );
 }

template< class T , std::integral K , class P >
inline std::size_t DialQueue< T , K , P >::next( void ) const
{
 const std::size_t W = f_B.size() , s = std::size_t( f_cur ) & ( W - 1 );
 const std::size_t nw = f_bits.size();
 std::size_t w = s >> 6;
 std::uint64_t m = f_bits[ w ] & ( ~std::uint64_t( 0 ) << ( s & 63 ) );
 for( std::size_t k = 0 ; ; ) {
  if( m ) {
   const std::size_t b = w * 64 + std::size_t( std::countr_zero( m ) );
   return( ( b - s ) & ( W - 1 ) );
   }
  if( ++k > nw )  // around, and back to the
   return( W );  // low bits of the first word
  w = w + 1 == nw ? 0 : w + 1;
  m = f_bits[ w ];
  if( k == nw )
   m &= ( std::uint64_t( 1 ) << ( s & 63 ) ) - 1;
  }
 }

template< class T , std::integral K , class P >
inline void DialQueue< T , K , P >::pull( void )
{
 ADS_PRE( f_n > 0 );
 if( ! f_B[ std::size_t( f_cur ) & ( f_B.size() - 1 ) ].empty() )
  return;  // still the least key
 std::size_t d = f_in ? next() : f_B.size();
 if( ! f_over.empty() ) {        // peeked, so that the keys
  const U o = f_over.peekKey();  // pushed next may be smaller
  if( ( d == f_B.size() ) || ( o < U( f_cur + U( d ) ) ) ) {
   f_cur = o;                    // the overflow in the window,
   const U W = U( f_B.size() );  // a bucket of it at a time
   if( f_cur <= U( std::numeric_limits< U >::max() - W ) )
    f_over.extractBelow( U( f_cur + W ) ,
			 [ this ]( T && x , U u ) { put( x , u ); } );
   else
    while( ! f_over.empty() && ( U( f_over.peekKey() - f_cur ) < W ) ) {
     const U u = f_over.topKey();
     put( f_over.pop() , u );
     }
   d = next();
   }
  }
 f_cur = U( f_cur + U( d ) );
 }

template< class T , std::integral K , class P >
inline const T & DialQueue< T , K , P >::top( void )
{
 pull();
 return( f_B[ std::size_t( f_cur ) & ( f_B.size() - 1 ) ].back() );
 }

template< class T , std::integral K , class P >
inline K DialQueue< T , K , P >::topKey( void )
{
 pull();
 return( decode( f_cur ) );
 }

template< class T , std::integral K , class P >
inline T DialQueue< T , K , P >::pop( void )
{
 if constexpr( heard< Era > )
  if( f_sub.template live< Era >() ) [[unlikely]] {
   if constexpr( Subject::dynamic )
    return( popCold() );
   else {
    const K k = topKey();
    f_sub.emit( Era{ top() , k } );  // before the removal
    T x = popPlain();
    f_sub.audit( *this );
    return( x );
    }
   }
 return( popPlain() );
 }

template< class T , std::integral K , class P >
inline T DialQueue< T , K , P >::popPlain( void )
{
 pull();
 const std::size_t b = std::size_t( f_cur ) & ( f_B.size() - 1 );
 std::vector< T > & v = f_B[ b ];
 T x = std::move( v.back() );
 v.pop_back();
 if( v.empty() )
  f_bits[ b >> 6 ] &= ~( std::uint64_t( 1 ) << ( b & 63 ) );
 --f_in;
 --f_n;
 ADS_INVARIANT( counted() );
 return( x );
 }

template< class T , std::integral K , class P >
void DialQueue< T , K , P >::clear( void )
{
 for( auto & v : f_B )
  v.clear();
 f_bits.assign( f_bits.size() , 0 );
 f_cur = 0;
 f_in = f_n = 0;
 f_over.clear();
 if constexpr( heard< mod::Cleared > )
  if( f_sub.template live< mod::Cleared >() ) {
   f_sub.emit( mod::Cleared{} );
   f_sub.audit( *this );
   }
 }

template< class T , std::integral K , class P >
K DialQueue< T , K , P >::peekKey( void ) const
{
 ADS_PRE( f_n > 0 );
 const std::size_t W = f_B.size();
 const std::size_t d = f_in ? next() : W;
 const U b = U( f_cur + U( d ) );
 if( f_over.empty() )
  return( decode( b ) );
 const U o = f_over.peekKey();
 return( decode( ( d == W ) || ( o < b ) ? o : b ) );
 }

template< class T , std::integral K , class P >
void DialQueue< T , K , P >::rebase( K k )
{
 const U u = encode( k );
 const std::size_t W = f_B.size() , s = std::size_t( f_cur ) & ( W - 1 );
 std::vector< std::pair< T , U > > E;
 E.reserve( f_n );
 for( std::size_t b = 0 ; b < W ; ++b ) {
  const U key = U( f_cur + U( ( b - s ) & ( W - 1 ) ) );  // the key of b
  for( T & x : f_B[ b ] )
   E.emplace_back( std::move( x ) , key );
  f_B[ b ].clear();
  }
 while( ! f_over.empty() ) {
  const U o = f_over.topKey();
  E.emplace_back( f_over.pop() , o );
  }
 f_over.clear();
 f_bits.assign( f_bits.size() , 0 );
 f_cur = u;
 f_in = 0;
 for( auto & [ x , key ] : E ) {
  ADS_PRE( key >= u );
  if( U( key - u ) < U( W ) )
   put( x , key );
  else
   f_over.push( x , key );
  }
 ADS_INVARIANT( counted() );
 }

template< class T , std::integral K , class P >
void DialQueue< T , K , P >::extract( K theta , std::vector< T > & out )
{
 const U t = encode( theta );
 while( ( f_n > 0 ) && ( encode( peekKey() ) <= t ) )
  out.push_back( pop() );
 }

template< class T , std::integral K , class P >
void DialQueue< T , K , P >::pushCold( const T & x , K k )
{
 pushPlain( x , k );
 f_sub.emit( Ins{ x , k } );
 f_sub.audit( *this );
 }

template< class T , std::integral K , class P >
T DialQueue< T , K , P >::popCold( void )
{
 const K k = topKey();
 f_sub.emit( Era{ top() , k } );  // before the removal
 T x = popPlain();
 f_sub.audit( *this );
 return( x );
 }

template< class T , std::integral K , class P >
bool DialQueue< T , K , P >::counted( void ) const
{
 if( f_n > 1024 )
  return( f_in + f_over.size() == f_n );
 std::size_t n = 0;
 for( std::size_t b = 0 ; b < f_B.size() ; ++b ) {
  const bool used = ( f_bits[ b >> 6 ] >> ( b & 63 ) ) & 1;
  if( used == f_B[ b ].empty() )
   return( false );
  n += f_B[ b ].size();
  }
 return( ( n == f_in ) && ( f_in + f_over.size() == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File DialQueue.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
