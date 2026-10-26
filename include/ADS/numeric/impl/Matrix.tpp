/*--------------------------------------------------------------------------*/
/*---------------------------- File Matrix.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Matrix.h [see Matrix.h].
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

template< class T >
Matrix< T > multiplyNaive( const Matrix< T > & A , const Matrix< T > & B ,
                           long * mults )
{
 ADS_PRE( detail::hasShape( A , A.size() , A.size() ) &&
          detail::hasShape( B , A.size() , A.size() ) );
 const std::size_t n = A.size();
 Matrix< T > C( n , std::vector< T >( n , T( 0 ) ) );
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t k = 0 ; k < n ; ++k )
   for( std::size_t j = 0 ; j < n ; ++j )
    C[ i ][ j ] += A[ i ][ k ] * B[ k ][ j ];
 if( mults )
  *mults += long( n * n * n );
 return( C );
 }

template< class T >
Matrix< T > multiply( const Matrix< T > & A , const Matrix< T > & B )
{
 ADS_PRE( detail::hasShape( A , A.size() , B.size() ) &&
          detail::hasShape( B , B.size() , B.empty() ? 0 : B[ 0 ].size() ) );
 const std::size_t p = A.size() , q = B.size() ,
                   r = B.empty() ? 0 : B[ 0 ].size();
 Matrix< T > C( p , std::vector< T >( r , T( 0 ) ) );
 for( std::size_t i = 0 ; i < p ; ++i )
  for( std::size_t k = 0 ; k < q ; ++k )
   for( std::size_t j = 0 ; j < r ; ++j )
    C[ i ][ j ] += A[ i ][ k ] * B[ k ][ j ];
 return( C );
 }

template< class T >
Matrix< T > transpose( const Matrix< T > & A )
{
 ADS_PRE( detail::hasShape( A , A.size() , A.empty() ? 0 : A[ 0 ].size() ) );
 const std::size_t p = A.size() , q = A.empty() ? 0 : A[ 0 ].size();
 Matrix< T > B( q , std::vector< T >( p ) );
 for( std::size_t i = 0 ; i < p ; ++i )
  for( std::size_t j = 0 ; j < q ; ++j ) B[ j ][ i ] = A[ i ][ j ];
 return( B );
 }

template< class T >
Matrix< T > scale( Matrix< T > A , const T & s )
{
 for( auto & row : A )
  for( auto & x : row ) x *= s;
 return( A );
 }

namespace detail {

template< class T >
bool hasShape( const Matrix< T > & A , std::size_t r , std::size_t c )
{
 if( A.size() != r )
  return( false );
 for( const auto & row : A )
  if( row.size() != c )
   return( false );
 return( true );
 }

template< class T >
Matrix< T > block( const Matrix< T > & A , std::size_t r , std::size_t c ,
                   std::size_t h )
{
 Matrix< T > X( h , std::vector< T >( h ) );
 for( std::size_t i = 0 ; i < h ; ++i )
  for( std::size_t j = 0 ; j < h ; ++j )
   X[ i ][ j ] = A[ r + i ][ c + j ];
 return( X );
 }

template< class T >
Matrix< T > plus( const Matrix< T > & A , const Matrix< T > & B , T s )
{
 Matrix< T > C = A;
 for( std::size_t i = 0 ; i < A.size() ; ++i )
  for( std::size_t j = 0 ; j < A.size() ; ++j )
   C[ i ][ j ] += s * B[ i ][ j ];
 return( C );
 }

template< class T >
Matrix< T > strassenRec( const Matrix< T > & A , const Matrix< T > & B ,
                         std::size_t cutoff , long * mults )
{
 const std::size_t n = A.size();
 if( n <= cutoff )
  return( multiplyNaive( A , B , mults ) );
 const std::size_t h = n / 2;
 const auto A11 = block( A , 0 , 0 , h ) , A12 = block( A , 0 , h , h ) ,
            A21 = block( A , h , 0 , h ) , A22 = block( A , h , h , h ) ,
            B11 = block( B , 0 , 0 , h ) , B12 = block( B , 0 , h , h ) ,
            B21 = block( B , h , 0 , h ) , B22 = block( B , h , h , h );
 const T m = T( -1 );
 auto S = [ & ]( const Matrix< T > & X , const Matrix< T > & Y ) {
  return( strassenRec( X , Y , cutoff , mults ) ); };
 const auto X1 = S( plus( A11 , A22 ) , plus( B11 , B22 ) );
 const auto X2 = S( plus( A21 , A22 ) , B11 );
 const auto X3 = S( A11 , plus( B12 , B22 , m ) );
 const auto X4 = S( A22 , plus( B21 , B11 , m ) );
 const auto X5 = S( plus( A11 , A12 ) , B22 );
 const auto X6 = S( plus( A21 , A11 , m ) , plus( B11 , B12 ) );
 const auto X7 = S( plus( A12 , A22 , m ) , plus( B21 , B22 ) );
 const auto C11 = plus( plus( plus( X1 , X4 ) , X5 , m ) , X7 );
 const auto C12 = plus( X3 , X5 );
 const auto C21 = plus( X2 , X4 );
 const auto C22 = plus( plus( plus( X1 , X3 ) , X2 , m ) , X6 );
 Matrix< T > C( n , std::vector< T >( n ) );
 for( std::size_t i = 0 ; i < h ; ++i )
  for( std::size_t j = 0 ; j < h ; ++j ) {
   C[ i ][ j ] = C11[ i ][ j ];
   C[ i ][ j + h ] = C12[ i ][ j ];
   C[ i + h ][ j ] = C21[ i ][ j ];
   C[ i + h ][ j + h ] = C22[ i ][ j ];
   }
 return( C );
 }

}  // end( namespace detail )

template< class T >
Matrix< T > strassen( const Matrix< T > & A , const Matrix< T > & B ,
                      std::size_t cutoff , long * mults )
{
 const std::size_t n = A.size();
 ADS_PRE( detail::hasShape( A , n , n ) && detail::hasShape( B , n , n ) );
 std::size_t N = 1;
 while( N < n )
  N *= 2;
 Matrix< T > P( N , std::vector< T >( N , T( 0 ) ) ) , Q = P;
 for( std::size_t i = 0 ; i < n ; ++i )
  for( std::size_t j = 0 ; j < n ; ++j ) {
   P[ i ][ j ] = A[ i ][ j ];
   Q[ i ][ j ] = B[ i ][ j ];
   }
 auto R = detail::strassenRec( P , Q , cutoff < 1 ? 1 : cutoff , mults );
 R.resize( n );
 for( auto & r : R )
  r.resize( n );
 ADS_AUDIT( R == multiplyNaive( A , B ) );
 return( R );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Matrix.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
