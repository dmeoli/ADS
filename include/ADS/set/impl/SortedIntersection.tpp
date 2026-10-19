/*--------------------------------------------------------------------------*/
/*----------------------- File SortedIntersection.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SortedIntersection.h and the operations of
 * PermutedBuckets, with their contracts.
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

namespace detail {

/// A strictly increasing by less
template< class T , class C >
bool strictlyIncreasing( const std::vector< T > & A , C less )
{
 for( std::size_t i = 1 ; i < A.size() ; ++i )
  if( ! less( A[ i - 1 ] , A[ i ] ) ) return( false );
 return( true );
 }

/// a and b hold the same items, by less
template< class T , class C >
bool sameSet( const std::vector< T > & a , const std::vector< T > & b ,
              C less )
{
 if( a.size() != b.size() ) return( false );
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  if( less( a[ i ] , b[ i ] ) || less( b[ i ] , a[ i ] ) ) return( false );
 return( true );
 }

/// Algorithm 6.1 on X[ xlo , xhi ) and Y[ ylo , yhi ), the shorter one
/// split by its median
template< class T , class C >
void mutualPartition( const std::vector< T > & X , std::size_t xlo ,
                      std::size_t xhi , const std::vector< T > & Y ,
                      std::size_t ylo , std::size_t yhi ,
                      std::vector< T > & out , C less )
{
 if( ( xlo >= xhi ) || ( ylo >= yhi ) ) return;
 if( yhi - ylo > xhi - xlo ) {              // Y must be the shorter
  mutualPartition( Y , ylo , yhi , X , xlo , xhi , out , less );
  return;
  }
 const std::size_t m = ylo + ( yhi - ylo ) / 2;
 const std::size_t j = lowerBound( X , Y[ m ] , xlo , xhi , less );
 const bool found = ( j < xhi ) && ! less( Y[ m ] , X[ j ] );
 mutualPartition( X , xlo , j , Y , ylo , m , out , less );
 if( found ) out.push_back( Y[ m ] );
 mutualPartition( X , found ? j + 1 : j , xhi , Y , m + 1 , yhi , out ,
                  less );
 }

}  // end( namespace detail )

template< class T , class C >
std::vector< T > intersectMerge( const std::vector< T > & A ,
                                 const std::vector< T > & B , C less )
{
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::strictlyIncreasing( A , less ) &&
          detail::strictlyIncreasing( B , less ) ); } ) );
 std::vector< T > out;
 for( std::size_t i = 0 , j = 0 ; ( i < A.size() ) && ( j < B.size() ) ; )
  if( less( A[ i ] , B[ j ] ) ) ++i;
  else if( less( B[ j ] , A[ i ] ) ) ++j;
  else {
   out.push_back( A[ i ] );
   ++i;
   ++j;
   }
 return( out );
 }

template< class T , class C >
std::vector< T > intersectBinarySearch( const std::vector< T > & A ,
                                        const std::vector< T > & B ,
                                        C less )
{
 const auto & X = A.size() >= B.size() ? A : B;   // the longer
 const auto & Y = A.size() >= B.size() ? B : A;
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::strictlyIncreasing( X , less ) &&
          detail::strictlyIncreasing( Y , less ) ); } ) );
 std::vector< T > out;
 for( const T & y : Y ) {
  const std::size_t j = lowerBound( X , y , less );
  if( ( j < X.size() ) && ! less( y , X[ j ] ) ) out.push_back( y );
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sameSet( out , intersectMerge( A , B , less ) , less ) );
  } ) );
 return( out );
 }

template< class T , class C >
std::vector< T > intersectMutualPartition( const std::vector< T > & A ,
                                           const std::vector< T > & B ,
                                           C less )
{
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::strictlyIncreasing( A , less ) &&
          detail::strictlyIncreasing( B , less ) ); } ) );
 std::vector< T > out;
 detail::mutualPartition( A , 0 , A.size() , B , 0 , B.size() , out ,
                          less );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sameSet( out , intersectMerge( A , B , less ) , less ) );
  } ) );
 return( out );
 }

template< class T , class C >
std::vector< T > intersectDoubling( const std::vector< T > & A ,
                                    const std::vector< T > & B , C less )
{
 const auto & X = A.size() >= B.size() ? A : B;   // the longer
 const auto & Y = A.size() >= B.size() ? B : A;
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::strictlyIncreasing( X , less ) &&
          detail::strictlyIncreasing( Y , less ) ); } ) );
 const std::size_t n = X.size();
 std::vector< T > out;
 std::size_t i = 0;                         // X[ 0 , i ) left behind
 for( const T & y : Y ) {
  std::size_t lo = i , off = 1;             // probe i , i + 1 , i + 3 , ...
  while( ( i + off - 1 < n ) && less( X[ i + off - 1 ] , y ) ) {
   lo = i + off;
   off *= 2;
   }
  const std::size_t j = lowerBound( X , y , lo , std::min( i + off , n ) ,
                                    less );
  const bool found = ( j < n ) && ! less( y , X[ j ] );
  if( found ) out.push_back( y );
  i = found ? j + 1 : j;
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sameSet( out , intersectMerge( A , B , less ) , less ) );
  } ) );
 return( out );
 }

template< class T , class C >
std::vector< T > intersectTwoLevel( const std::vector< T > & A ,
                                    const std::vector< T > & B ,
                                    std::size_t L , C less )
{
 ADS_PRE( L >= 1 );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::strictlyIncreasing( A , less ) &&
          detail::strictlyIncreasing( B , less ) ); } ) );
 const std::size_t n = A.size();
 std::vector< T > H , out;                  // A' = A[ 0 ] , A[ L ] , ...
 for( std::size_t k = 0 ; k < n ; k += L ) H.push_back( A[ k ] );
 std::size_t h = 0 , pa = 0;
 for( const T & b : B ) {
  if( H.empty() || less( b , H[ 0 ] ) ) continue;   // before all blocks
  while( ( h + 1 < H.size() ) && ! less( b , H[ h + 1 ] ) ) ++h;
  pa = std::max( pa , h * L );              // the block of b, merged
  const std::size_t end = std::min( ( h + 1 ) * L , n );
  while( ( pa < end ) && less( A[ pa ] , b ) ) ++pa;
  if( ( pa < end ) && ! less( b , A[ pa ] ) ) out.push_back( b );
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sameSet( out , intersectMerge( A , B , less ) , less ) );
  } ) );
 return( out );
 }

template< class T >
void twoLevelHeaders( const DiskFile< T > & A , std::size_t L ,
                      DiskFile< T > & H )
{
 ADS_PRE( ( L >= 1 ) && ( &H.memory() == &A.memory() ) && H.empty() );
 DiskReader< T > rd( A );
 DiskWriter< T > w( H );
 for( std::size_t k = 0 ; rd.hasNext() ; ++k ) {
  const T x = rd.next();
  if( k % L == 0 ) w.push( x );
  }
 }

template< class T , class C >
void intersectMergeExternal( const DiskFile< T > & A ,
                             const DiskFile< T > & B , DiskFile< T > & out ,
                             C less )
{
 TwoLevelMemory & mem = A.memory();
 ADS_PRE( mem.memorySize() >= 3 * mem.blockSize() );
 ADS_PRE( ( &B.memory() == &mem ) && ( &out.memory() == &mem ) &&
          out.empty() );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::strictlyIncreasing( A.contents() , less ) &&
          detail::strictlyIncreasing( B.contents() , less ) ); } ) );
 DiskReader< T > ra( A ) , rb( B );
 DiskWriter< T > w( out );
 while( ra.hasNext() && rb.hasNext() )
  if( less( ra.peek() , rb.peek() ) ) ra.next();
  else if( less( rb.peek() , ra.peek() ) ) rb.next();
  else {
   w.push( ra.next() );
   rb.next();
   }
 }

template< class T , class C >
void intersectTwoLevelExternal( const DiskFile< T > & A ,
                                const DiskFile< T > & H , std::size_t L ,
                                const DiskFile< T > & B ,
                                DiskFile< T > & out , C less )
{
 TwoLevelMemory & mem = A.memory();
 ADS_PRE( ( L >= 1 ) && ( mem.memorySize() >= 4 * mem.blockSize() ) );
 ADS_PRE( ( &H.memory() == &mem ) && ( &B.memory() == &mem ) &&
          ( &out.memory() == &mem ) && out.empty() );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::strictlyIncreasing( A.contents() , less ) &&
          detail::strictlyIncreasing( B.contents() , less ) ); } ) );
 const std::size_t n = A.size();
 {
  DiskReader< T > rh( H ) , rb( B ) , ra( A );
  DiskWriter< T > w( out );
  std::size_t h = 0 , pa = 0;
  bool started = false;                     // h is a block of A
  while( rb.hasNext() ) {
   const T b = rb.next();
   if( ! rh.hasNext() && ! started ) break; // A empty
   if( ! started ) {
    if( less( b , rh.peek() ) ) continue;  // before all blocks
    rh.next();
    started = true;
    }
   while( rh.hasNext() && ! less( b , rh.peek() ) ) {
    rh.next();
    ++h;
    }
   pa = std::max( pa , h * L );
   const std::size_t end = std::min( ( h + 1 ) * L , n );
   while( ( pa < end ) && less( ra.at( pa ) , b ) ) ++pa;
   if( ( pa < end ) && ! less( b , ra.at( pa ) ) ) w.push( b );
   }
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sameSet( out.contents() ,
                           intersectMerge( A.contents() , B.contents() ,
                                           less ) , less ) ); } ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- CLASS PermutedBuckets --------------------------*/
/*--------------------------------------------------------------------------*/

inline PermutedBuckets::PermutedBuckets( const std::vector< std::uint64_t > &
                                                                          A ,
                                         unsigned w , std::size_t L ,
                                         std::uint64_t a , std::uint64_t c )
 : f_w( w ) , f_ell( 0 ) , f_a( a ) , f_c( c ) , f_ainv( a )
{
 ADS_PRE( ( w >= 1 ) && ( w <= 63 ) && ( a % 2 == 1 ) && ( L >= 1 ) );
 ADS_PRE( std::all_of( A.begin() , A.end() , [ & ]( std::uint64_t x ) {
  return( x <= mask() ); } ) );
 for( int k = 0 ; k < 6 ; ++k )             // Newton: a^{-1} mod 2^64
  f_ainv *= 2 - a * f_ainv;
 f_a &= mask();
 f_c &= mask();
 while( ( f_ell < f_w ) && ( ( std::size_t( 1 ) << f_ell ) * L < A.size() ) )
  ++f_ell;                                  // 2^ell buckets of L items
 f_img.reserve( A.size() );
 for( const std::uint64_t x : A ) f_img.push_back( permute( x ) );
 std::sort( f_img.begin() , f_img.end() );
 ADS_PRE( std::adjacent_find( f_img.begin() , f_img.end() ) ==
          f_img.end() );                    // distinct values
 const std::size_t nb = std::size_t( 1 ) << f_ell;
 f_off.assign( nb + 1 , 0 );
 for( const std::uint64_t y : f_img ) ++f_off[ ( y >> ( f_w - f_ell ) ) + 1 ];
 for( std::size_t b = 1 ; b <= nb ; ++b ) f_off[ b ] += f_off[ b - 1 ];
 }

inline std::uint64_t PermutedBuckets::mask( void ) const
{
 return( ( std::uint64_t( 1 ) << f_w ) - 1 );
 }

inline std::uint64_t PermutedBuckets::permute( std::uint64_t x ) const
{
 return( ( f_a * x + f_c ) & mask() );
 }

inline std::uint64_t PermutedBuckets::invert( std::uint64_t y ) const
{
 return( ( f_ainv * ( y - f_c ) ) & mask() );
 }

inline std::size_t PermutedBuckets::size( void ) const
{
 return( f_img.size() );
 }

inline std::size_t PermutedBuckets::buckets( void ) const
{
 return( f_off.size() - 1 );
 }

inline std::vector< std::uint64_t > PermutedBuckets::values( void ) const
{
 std::vector< std::uint64_t > v;
 v.reserve( f_img.size() );
 for( const std::uint64_t y : f_img ) v.push_back( invert( y ) );
 std::sort( v.begin() , v.end() );
 return( v );
 }

inline std::vector< std::uint64_t > PermutedBuckets::intersect(
                                         const PermutedBuckets & other ,
                                         std::size_t * scanned ) const
{
 ADS_PRE( ( f_w == other.f_w ) && ( f_a == other.f_a ) &&
          ( f_c == other.f_c ) );
 const PermutedBuckets & S = size() <= other.size() ? *this : other;
 const PermutedBuckets & G = size() <= other.size() ? other : *this;
 std::vector< std::uint64_t > out;
 std::size_t seen = S.f_img.size() , j = 0;
 for( const std::uint64_t y : S.f_img ) {   // y looked up in its bucket of G
  const std::size_t g = std::size_t( y >> ( G.f_w - G.f_ell ) );
  j = std::max( j , G.f_off[ g ] );
  const std::size_t je = G.f_off[ g + 1 ];
  for( ; ( j < je ) && ( G.f_img[ j ] < y ) ; ++j ) ++seen;
  if( ( j < je ) && ( G.f_img[ j ] == y ) ) {
   out.push_back( invert( y ) );
   ++seen;
   ++j;
   }
  }
 std::sort( out.begin() , out.end() );
 if( scanned ) *scanned = seen;
 ADS_AUDIT( out == intersectMerge( values() , other.values() ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File SortedIntersection.tpp --------------------*/
/*--------------------------------------------------------------------------*/
