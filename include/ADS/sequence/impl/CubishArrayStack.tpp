/*--------------------------------------------------------------------------*/
/*----------------------- File CubishArrayStack.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CubishArrayStack [see CubishArrayStack.h], with their
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

template< class T >
CubishArrayStack< T >::CubishArrayStack( std::initializer_list< T > l )
{
 for( const T & x : l )
  push_back( x );
 }

template< class T >
CubishArrayStack< T >::CubishArrayStack( const CubishArrayStack & o )
{
 rebuild( o.f_n );
 for( const T & x : o )
  push_back( x );
 }

template< class T >
CubishArrayStack< T >::CubishArrayStack( CubishArrayStack && o ) noexcept
    : f_mids( std::move( o.f_mids ) ) , f_spare( std::move( o.f_spare ) ) ,
      f_n( std::exchange( o.f_n , 0 ) ) , f_b( std::exchange( o.f_b , 16 ) ) ,
      f_lg( std::exchange( o.f_lg , 4 ) )
{
 o.f_mids.clear();
 o.f_spare.clear();
 }

template< class T >
CubishArrayStack< T > &
CubishArrayStack< T >::operator=( CubishArrayStack o ) noexcept
{
 swap( o );
 return( *this );
 }

template< class T >
void CubishArrayStack< T >::swap( CubishArrayStack & o ) noexcept
{
 f_mids.swap( o.f_mids );
 f_spare.swap( o.f_spare );
 std::swap( f_n , o.f_n );
 std::swap( f_b , o.f_b );
 std::swap( f_lg , o.f_lg );
 }

template< class T >
std::size_t CubishArrayStack< T >::capacity( void ) const
{
 std::size_t c = f_spare.size();
 for( const Mid & M : f_mids )
  c += M.nl;
 return( c * f_b );
 }

template< class T >
typename CubishArrayStack< T >::Mid
CubishArrayStack< T >::newMid( void ) const
{
 Mid M;
 M.L = std::make_unique< Leaf[] >( 2 * f_b );
 return( M );
 }

template< class T >
std::unique_ptr< T[] > CubishArrayStack< T >::newLeaf( void )
{
 if( f_spare.empty() )
  return( std::make_unique< T[] >( f_b ) );
 std::unique_ptr< T[] > a = std::move( f_spare.back() );
 f_spare.pop_back();
 return( a );
 }

template< class T >
void CubishArrayStack< T >::freeLeaf( std::unique_ptr< T[] > a )
{
 if( f_spare.size() < 4 )  // enough for the cascades of one operation
  f_spare.push_back( std::move( a ) );
 }

template< class T >
void CubishArrayStack< T >::reset( Mid & M )
{
 for( std::size_t j = 0 ; j < M.nl ; ++j )
  freeLeaf( std::move( leaf( M , j ).a ) );
 M.head = M.nl = M.g = M.n = 0;
 }

template< class T >
void CubishArrayStack< T >::pushFront( Mid & M , T && x )
{
 if( M.g == 0 ) {  // a new leaf at the front
  M.head = ( M.head - 1 ) & ( 2 * f_b - 1 );
  M.L[ M.head ] = Leaf{ newLeaf() , 0 };
  ++M.nl;
  M.g = f_b;
  }
 --M.g;
 Leaf & f = M.L[ M.head ];
 f.a[ ( f.off + M.g ) & ( f_b - 1 ) ] = std::move( x );
 ++M.n;
 }

template< class T >
void CubishArrayStack< T >::pushBack( Mid & M , T && x )
{
 const std::size_t v = M.g + M.n;
 if( v == M.nl * f_b ) {  // a new leaf at the back
  leaf( M , M.nl ) = Leaf{ newLeaf() , 0 };
  ++M.nl;
  }
 Leaf & f = leaf( M , v >> f_lg );
 f.a[ ( f.off + v ) & ( f_b - 1 ) ] = std::move( x );
 ++M.n;
 }

template< class T >
T CubishArrayStack< T >::popFront( Mid & M )
{
 T x = std::move( at( M , 0 ) );
 ++M.g;
 --M.n;
 if( M.n == 0 )
  reset( M );
 else if( M.g == f_b ) {  // the first leaf empty
  freeLeaf( std::move( M.L[ M.head ].a ) );
  M.head = ( M.head + 1 ) & ( 2 * f_b - 1 );
  --M.nl;
  M.g = 0;
  }
 return( x );
 }

template< class T >
T CubishArrayStack< T >::popBack( Mid & M )
{
 const std::size_t v = M.g + M.n - 1;
 T x = std::move( at( M , M.n - 1 ) );
 --M.n;
 if( M.n == 0 )
  reset( M );
 else if( ( v & ( f_b - 1 ) ) == 0 ) {  // the last leaf empty
  freeLeaf( std::move( leaf( M , M.nl - 1 ).a ) );
  --M.nl;
  }
 return( x );
 }

template< class T >
void CubishArrayStack< T >::insert( Mid & M , std::size_t r , T && x )
{
 const std::size_t m = f_b - 1;
 if( M.g + M.n == M.nl * f_b ) {  // a new leaf at the back
  leaf( M , M.nl ) = Leaf{ newLeaf() , 0 };
  ++M.nl;
  }
 const std::size_t v = M.g + r;
 const std::size_t k = v >> f_lg , L = ( M.g + M.n ) >> f_lg;
 for( std::size_t j = L ; j > k ; --j ) {  // the last item of each leaf
  Leaf & p = leaf( M , j - 1 );           // to the front of the next
  Leaf & q = leaf( M , j );
  q.off = ( q.off - 1 ) & m;
  q.a[ q.off ] = std::move( p.a[ ( p.off + m ) & m ] );
  }
 Leaf & c = leaf( M , k );  // and the items of the leaf
 const std::size_t end = k < L ? m : M.g + M.n - ( k << f_lg );  // after
 for( std::size_t q = end ; q > ( v & m ) ; --q )  // r one place on
  c.a[ ( c.off + q ) & m ] = std::move( c.a[ ( c.off + q - 1 ) & m ] );
 c.a[ ( c.off + v ) & m ] = std::move( x );
 ++M.n;
 }

template< class T >
void CubishArrayStack< T >::erase( Mid & M , std::size_t r )
{
 const std::size_t m = f_b - 1;
 const std::size_t v = M.g + r;
 const std::size_t k = v >> f_lg , L = ( M.g + M.n - 1 ) >> f_lg;
 Leaf & c = leaf( M , k );  // the items of the leaf after
 const std::size_t end = k < L ? f_b : M.g + M.n - ( k << f_lg );  // r
 for( std::size_t q = ( v & m ) + 1 ; q < end ; ++q )  // one place back
  c.a[ ( c.off + q - 1 ) & m ] = std::move( c.a[ ( c.off + q ) & m ] );
 for( std::size_t j = k + 1 ; j <= L ; ++j ) {  // the first item of each
  Leaf & p = leaf( M , j - 1 );                // leaf to the back of the
  Leaf & q = leaf( M , j );                    // one before
  p.a[ ( p.off + m ) & m ] = std::move( q.a[ q.off ] );
  q.off = ( q.off + 1 ) & m;
  }
 --M.n;
 if( M.n == 0 )
  reset( M );
 else if( M.g + M.n <= ( M.nl - 1 ) * f_b ) {  // the last leaf empty
  freeLeaf( std::move( leaf( M , M.nl - 1 ).a ) );
  --M.nl;
  }
 }

template< class T >
void CubishArrayStack< T >::rebuild( std::size_t n )
{
 std::size_t b = 16;  // the least power of 2, 16 at
 while( b * b * b < n )
  b *= 2;        // least, whose cube is not
 if( b == f_b )  // below n
  return;
 CubishArrayStack o;
 o.f_b = b;
 o.f_lg = unsigned( std::countr_zero( b ) );
 const std::size_t C = b * b;
 for( std::size_t i = 0 ; i < f_n ; ++i ) {
  if( o.f_mids.empty() || ( o.f_mids.back().n == C ) )
   o.f_mids.push_back( o.newMid() );
  o.pushBack( o.f_mids.back() , std::move( ( *this )[ i ] ) );
  }
 o.f_n = f_n;
 swap( o );
 }

template< class T >
void CubishArrayStack< T >::insert( std::size_t i , const T & x )
{
 ADS_PRE( i <= f_n );
 [[maybe_unused]] const std::size_t n = f_n;
 T y = x;  // x may be an item
 if( f_n + 1 > 8 * f_b * f_b * f_b )
  rebuild( f_n + 1 );
 const std::size_t C = f_b * f_b;
 if( f_mids.empty() || ( f_mids.back().n == C ) )
  f_mids.push_back( newMid() );
 const std::size_t k = i >> ( 2 * f_lg );
 for( std::size_t j = f_mids.size() - 1 ; j > k ; --j )
  pushFront( f_mids[ j ] , popBack( f_mids[ j - 1 ] ) );
 insert( f_mids[ k ] , i & ( C - 1 ) , std::move( y ) );
 ++f_n;
 ADS_POST( f_n == n + 1 );
 ADS_INVARIANT( full() );
 }

template< class T >
void CubishArrayStack< T >::erase( std::size_t i )
{
 ADS_PRE( i < f_n );
 [[maybe_unused]] const std::size_t n = f_n;
 const std::size_t C = f_b * f_b;
 const std::size_t k = i >> ( 2 * f_lg );
 erase( f_mids[ k ] , i & ( C - 1 ) );
 for( std::size_t j = k + 1 ; j < f_mids.size() ; ++j )
  pushBack( f_mids[ j - 1 ] , popFront( f_mids[ j ] ) );
 if( f_mids.back().n == 0 )
  f_mids.pop_back();
 --f_n;
 if( ( f_b > 16 ) && ( 8 * f_n < f_b * f_b * f_b ) )
  rebuild( f_n );
 ADS_POST( f_n == n - 1 );
 ADS_INVARIANT( full() );
 }

template< class T >
void CubishArrayStack< T >::clear( void )
{
 f_mids.clear();
 f_spare.clear();
 f_n = 0;
 f_b = 16;
 f_lg = 4;
 }

template< class T >
bool CubishArrayStack< T >::operator==( const CubishArrayStack & o ) const
{
 if( f_n != o.f_n )
  return( false );
 for( std::size_t i = 0 ; i < f_n ; ++i )
  if( ! ( ( *this )[ i ] == o[ i ] ) )
   return( false );
 return( true );
 }

template< class T >
bool CubishArrayStack< T >::full( void ) const
{
 const std::size_t C = f_b * f_b;
 std::size_t s = 0;
 for( std::size_t j = 0 ; j < f_mids.size() ; ++j ) {
  const Mid & M = f_mids[ j ];
  if( ( M.n == 0 ) || ( M.n > C ) ||
      ( ( j + 1 < f_mids.size() ) && ( M.n < C ) ) )
   return( false );
  if( ( M.g >= f_b ) || ( M.nl != ( M.g + M.n + f_b - 1 ) / f_b ) )
   return( false );
  s += M.n;
  }
 return( s == f_n );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File CubishArrayStack.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
