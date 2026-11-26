/*--------------------------------------------------------------------------*/
/*--------------------------- File KllSketch.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of KllSketch [see KllSketch.h], with their contracts.
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
/*--------------------------------- View -----------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::semiregular T , class Compare >
double KllSketch< T , Compare >::View::rank( const T & x ) const
{
 if( f_n == 0 )
  return( 0 );
 const auto i =
   std::upper_bound( f_I.begin() , f_I.end() , x , f_cmp ) - f_I.begin();
 return( i == 0 ? 0.0
		 : double( f_W[ std::size_t( i - 1 ) ] ) / double( f_n ) );
 }

template< std::semiregular T , class Compare >
const T & KllSketch< T , Compare >::View::quantile( double q ) const
{
 ADS_PRE( f_n > 0 && q >= 0 && q <= 1 );
 // the first item whose rank, computed as rank() computes it, reaches q
 const auto i =
   std::partition_point( f_W.begin() , f_W.end() ,
			 [ & ]( std::uint64_t w ) {
			  return( double( w ) / double( f_n ) < q );
			  } ) -
   f_W.begin();
 return( f_I[ std::min( std::size_t( i ) , f_I.size() - 1 ) ] );
 }

template< std::semiregular T , class Compare >
std::size_t KllSketch< T , Compare >::View::size( void ) const
{
 return( f_I.size() );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- KllSketch --------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::semiregular T , class Compare >
KllSketch< T , Compare >::KllSketch( std::uint16_t k , std::uint64_t seed ,
				    Compare cmp )
    : f_s( seed ) , f_k( k ) , f_cmp( cmp )
{
 ADS_PRE( k >= 8 );
 f_I.resize( k );
 f_L = { std::uint32_t( k ) , std::uint32_t( k ) };
 }

template< std::semiregular T , class Compare >
std::uint32_t KllSketch< T , Compare >::capacity( unsigned H ,
						 unsigned h ) const
{
 // round( k ( 2 / 3 )^depth ) in integers, as ( 2 k 2^depth / 3^depth +
 // 1 ) / 2; past the depth 30 it is below 1 for any k of 16 bits
 const unsigned depth = H - h - 1;
 if( depth > 30 )
  return( 8 );
 std::uint64_t three = 1;
 for( unsigned i = 0 ; i < depth ; ++i )
  three *= 3;
 const std::uint64_t c =
   ( ( ( std::uint64_t( f_k ) << 1 ) << depth ) / three + 1 ) >> 1;
 return( std::uint32_t( std::max< std::uint64_t >( 8 , c ) ) );
 }

template< std::semiregular T , class Compare >
std::uint32_t KllSketch< T , Compare >::capacity( unsigned H ) const
{
 std::uint32_t c = 0;
 for( unsigned h = 0 ; h < H ; ++h )
  c += capacity( H , h );
 return( c );
 }

template< std::semiregular T , class Compare >
bool KllSketch< T , Compare >::coin( void )
{
 if( f_nbits == 0 ) {
  f_bits = splitmix64( f_s );
  f_s += 0x9e3779b97f4a7c15ULL;
  f_nbits = 64;
  }
 const bool b = ( f_bits & 1 ) != 0;
 f_bits >>= 1;
 --f_nbits;
 return( b );
 }

template< std::semiregular T , class Compare >
void KllSketch< T , Compare >::reseed( std::uint64_t salt )
{
 f_s = splitmix64( f_s ^ splitmix64( salt ) );
 f_nbits = 0;
 }

template< std::semiregular T , class Compare >
void KllSketch< T , Compare >::compact( unsigned h )
{
 auto & I = f_I;
 auto & L = f_L;
 const std::uint32_t b = L[ h ] , e = L[ h + 1 ] , top = L[ h + 2 ] ,
   pop = e - b ,
		     odd = pop & 1 , ab = b + odd , half = ( pop - odd ) / 2;
 if( h == 0 && ! f_sorted0 )
  std::sort( I.begin() + ab , I.begin() + e , f_cmp );
 const std::uint32_t c = coin() ? 1 : 0;
 if( top == e ) {  // the level above is empty: the kept items at the top
  for( std::uint32_t t = c ? 0 : 1 ; t < half ; ++t )  // t = 0 stays
   I[ e - 1 - t ] = std::move( I[ e - 1 - c - 2 * t ] );
  }
 else {  // the kept items at the bottom, then merged with the level above
  for( std::uint32_t t = c ? 0 : 1 ; t < half ; ++t )  // t = 0 stays
   I[ ab + t ] = std::move( I[ ab + c + 2 * t ] );
  // the merge in place of [ ab , ab + half ) and [ e , top ) into
  // [ ab + half , top ): a write never passes the next read of the level
  // above, since the first run has half items, nor reaches the first run,
  // which ends where the writes start
  std::uint32_t i = ab , j = e , o = ab + half;
  while( i < ab + half && j < top ) {
   if( f_cmp( I[ j ] , I[ i ] ) )
    I[ o++ ] = std::move( I[ j++ ] );
   else
    I[ o++ ] = std::move( I[ i++ ] );
   }
  while( i < ab + half )
   I[ o++ ] = std::move( I[ i++ ] );
  }
 L[ h + 1 ] -= half;
 if( odd ) {
  L[ h ] = L[ h + 1 ] - 1;
  if( L[ h ] != b )
   I[ L[ h ] ] = std::move( I[ b ] );
  }
 else
  L[ h ] = L[ h + 1 ];
 if( h == 0 )
  f_sorted0 = true;  // at most one item left
 else {              // the levels below moved up by the places freed
  std::move_backward( I.begin() + L[ 0 ] , I.begin() + b ,
		      I.begin() + b + half );
  for( unsigned l = 0 ; l < h ; ++l )
   L[ l ] += half;
  }
 }

template< std::semiregular T , class Compare >
void KllSketch< T , Compare >::insert( const T & x )
{
 if( f_L[ 0 ] == 0 ) {
  // the buffer is full, and some level holds at least its capacity
  const unsigned H = levels();
  unsigned h = 0;
  while( f_L[ h + 1 ] - f_L[ h ] < capacity( H , h ) )
   ++h;
  if( h == H - 1 ) {  // a new top level, the buffer grown at its start
   const std::uint32_t delta = capacity( H + 1 , 0 );
   f_I.insert( f_I.begin() , delta , T{} );
   for( auto & l : f_L )
    l += delta;
   f_L.push_back( f_L.back() );
   }
  compact( h );
  }
 f_I[ --f_L[ 0 ] ] = x;
 if( f_n == 0 )
  f_min = f_max = x;
 else {
  if( f_cmp( x , f_min ) )
   f_min = x;
  if( f_cmp( f_max , x ) )
   f_max = x;
  }
 ++f_n;
 f_sorted0 = f_L[ 1 ] - f_L[ 0 ] <= 1;
 ADS_INVARIANT( f_L.back() == f_I.size() &&
		f_L.back() == capacity( levels() ) );
 }

template< std::semiregular T , class Compare >
void KllSketch< T , Compare >::mergeHigher( const KllSketch & o )
{
 // the levels as vectors: the level 0 of this sketch, then the sorted
 // merges of the levels of the two
 unsigned H = std::max( levels() , o.levels() );
 std::vector< std::vector< T > > W( H );
 W[ 0 ].assign( f_I.begin() + f_L[ 0 ] , f_I.begin() + f_L[ 1 ] );
 for( unsigned h = 1 ; h < H ; ++h ) {
  const auto * s = this;
  const auto in = [ & ]( const KllSketch & x , bool first ) {
   return( h < x.levels() ? x.f_I.begin() + x.f_L[ h + ( first ? 0 : 1 ) ]
			   : x.f_I.end() );
   };
  W[ h ].reserve( ( in( *s , false ) - in( *s , true ) ) +
		  ( in( o , false ) - in( o , true ) ) );
  std::merge( in( *s , true ) , in( *s , false ) , in( o , true ) ,
	      in( o , false ) ,
	      std::back_inserter( W[ h ] ) , f_cmp );
  }
 // the general compaction from the bottom
 std::size_t count = 0;
 for( const auto & w : W )
  count += w.size();
 std::size_t target = capacity( H );
 bool sorted0 = f_sorted0;
 std::vector< T > merged;
 for( unsigned h = 0 ; h < H ; ++h ) {
  if( count < target || W[ h ].size() < capacity( H , h ) )
   continue;
  if( h == H - 1 )  // before the references, which it would invalidate
   W.emplace_back();
  auto & v = W[ h ];
  auto & up = W[ h + 1 ];
  if( h == 0 && ! sorted0 )
   std::sort( v.begin() , v.end() , f_cmp );
  const std::size_t odd = v.size() & 1 , half = ( v.size() - odd ) / 2 ,
		    c = coin() ? 1 : 0;
  merged.clear();
  merged.reserve( half + up.size() );
  std::size_t j = 0;
  for( std::size_t t = 0 ; t < half ; ++t ) {
   T & y = v[ odd + c + 2 * t ];
   while( j < up.size() && f_cmp( up[ j ] , y ) )
    merged.push_back( std::move( up[ j++ ] ) );
   merged.push_back( std::move( y ) );
   }
  while( j < up.size() )
   merged.push_back( std::move( up[ j++ ] ) );
  up.swap( merged );
  v.resize( odd );
  if( h == 0 )
   sorted0 = true;
  count -= half;
  if( h == H - 1 ) {
   ++H;
   target += capacity( H , 0 );
   }
  }
 ADS_INVARIANT( count <= target );
 // back into the buffer, its free places at the start
 f_I.assign( target , T{} );
 f_L.assign( H + 1 , 0 );
 f_L[ 0 ] = std::uint32_t( target - count );
 for( unsigned h = 0 ; h < H ; ++h ) {
  std::move( W[ h ].begin() , W[ h ].end() , f_I.begin() + f_L[ h ] );
  f_L[ h + 1 ] = f_L[ h ] + std::uint32_t( W[ h ].size() );
  }
 f_sorted0 = sorted0;
 }

template< std::semiregular T , class Compare >
void KllSketch< T , Compare >::merge( const KllSketch & o )
{
 if( o.empty() )
  return;
 if( &o == this ) {  // the items of o are read while this one changes
  const KllSketch c( o );
  merge( c );
  return;
  }
 const std::uint64_t n = f_n + o.f_n;
 const T lo = o.f_min , hi = o.f_max;
 for( std::uint32_t i = o.f_L[ 0 ] ; i < o.f_L[ 1 ] ; ++i )
  insert( o.f_I[ i ] );
 if( o.levels() >= 2 )
  mergeHigher( o );
 if( f_cmp( lo , f_min ) )
  f_min = lo;
 if( f_cmp( f_max , hi ) )
  f_max = hi;
 f_n = n;
 }

template< std::semiregular T , class Compare >
std::uint64_t KllSketch< T , Compare >::size( void ) const
{
 return( f_n );
 }

template< std::semiregular T , class Compare >
bool KllSketch< T , Compare >::empty( void ) const
{
 return( f_n == 0 );
 }

template< std::semiregular T , class Compare >
std::size_t KllSketch< T , Compare >::retained( void ) const
{
 return( f_L.back() - f_L[ 0 ] );
 }

template< std::semiregular T , class Compare >
std::uint16_t KllSketch< T , Compare >::k( void ) const
{
 return( f_k );
 }

template< std::semiregular T , class Compare >
unsigned KllSketch< T , Compare >::levels( void ) const
{
 return( unsigned( f_L.size() - 1 ) );
 }

template< std::semiregular T , class Compare >
const T & KllSketch< T , Compare >::min( void ) const
{
 ADS_PRE( f_n > 0 );
 return( f_min );
 }

template< std::semiregular T , class Compare >
const T & KllSketch< T , Compare >::max( void ) const
{
 ADS_PRE( f_n > 0 );
 return( f_max );
 }

template< std::semiregular T , class Compare >
typename KllSketch< T , Compare >::View
KllSketch< T , Compare >::view( void ) const
{
 // the items with their weights 2^h: the level 0 sorted, then each level,
 // sorted already, merged into the items before it, and the weights summed
 std::vector< std::pair< T , std::uint64_t > > P;
 P.reserve( retained() );
 const auto less = [ & ]( const auto & a , const auto & b ) {
  return( f_cmp( a.first , b.first ) );
  };
 for( unsigned h = 0 ; h < levels() ; ++h ) {
  const std::size_t mid = P.size();
  for( std::uint32_t i = f_L[ h ] ; i < f_L[ h + 1 ] ; ++i )
   P.emplace_back( f_I[ i ] , std::uint64_t( 1 ) << h );
  if( h == 0 && ! f_sorted0 )
   std::sort( P.begin() , P.end() , less );
  else if( h > 0 )
   std::inplace_merge( P.begin() , P.begin() + long( mid ) , P.end() , less );
  }
 View v;
 v.f_n = f_n;
 v.f_cmp = f_cmp;
 v.f_I.reserve( P.size() );
 v.f_W.reserve( P.size() );
 std::uint64_t w = 0;
 for( auto & [ x , c ] : P ) {
  w += c;
  v.f_I.push_back( std::move( x ) );
  v.f_W.push_back( w );
  }
 return( v );
 }

template< std::semiregular T , class Compare >
double KllSketch< T , Compare >::rank( const T & x ) const
{
 return( view().rank( x ) );
 }

template< std::semiregular T , class Compare >
T KllSketch< T , Compare >::quantile( double q ) const
{
 ADS_PRE( f_n > 0 && q >= 0 && q <= 1 );
 if( q == 0 )
  return( f_min );
 if( q == 1 )
  return( f_max );
 return( view().quantile( q ) );
 }

template< std::semiregular T , class Compare >
double KllSketch< T , Compare >::normalizedRankError( std::uint16_t k )
{
 return( 2.296 / std::pow( double( k ) , 0.9723 ) );
 }

template< std::semiregular T , class Compare >
void KllSketch< T , Compare >::clear( void )
{
 f_I.assign( f_k , T{} );
 f_L = { std::uint32_t( f_k ) , std::uint32_t( f_k ) };
 f_n = 0;
 f_min = f_max = T{};
 f_sorted0 = true;
 }

/*--------------------------------------------------------------------------*/

template< std::semiregular T , class Compare >
template< class A >
inline void KllSketch< T , Compare >::persist( A & a )
{
 a( f_I , f_L , f_n , f_min , f_max , f_s , f_bits , f_nbits , f_k ,
    f_sorted0 );
 a.require( f_k >= 8 && f_L.size() >= 2 && f_L.size() <= 62 &&
	      f_L.back() == f_I.size() && f_L.back() == capacity( levels() ) &&
	      f_nbits <= 64 ,
	    "the levels of a KllSketch" );
 // the starts increase, the weights sum to n and the levels above the 0
 // are sorted
 bool ok = true;
 std::uint64_t w = 0;
 for( unsigned h = 0 ; ok && h < levels() ; ++h ) {
  ok = f_L[ h ] <= f_L[ h + 1 ];
  if( ok ) {
   w += std::uint64_t( f_L[ h + 1 ] - f_L[ h ] ) << h;
   if( h > 0 )
    ok = std::is_sorted( f_I.begin() + f_L[ h ] , f_I.begin() + f_L[ h + 1 ] ,
			 f_cmp );
   }
  }
 a.require( ok && w == f_n , "the items of a KllSketch" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File KllSketch.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
