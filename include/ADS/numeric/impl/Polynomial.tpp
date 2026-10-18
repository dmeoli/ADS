/*--------------------------------------------------------------------------*/
/*-------------------------- File Polynomial.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Polynomial.h [see Polynomial.h].
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

template< class T >
std::vector< T > multiplyNaive( const std::vector< T > & p ,
                                const std::vector< T > & q ,
                                long * mults )
{
 if( p.empty() || q.empty() )
  return( std::vector< T >() );
 std::vector< T > r( p.size() + q.size() - 1 , T( 0 ) );
 for( std::size_t h = 0 ; h < p.size() ; ++h )
  for( std::size_t k = 0 ; k < q.size() ; ++k )
   r[ h + k ] += p[ h ] * q[ k ];
 if( mults )
  *mults += long( p.size() * q.size() );
 return( r );
 }

namespace detail {

template< class T >
void addShifted( std::vector< T > & r , const std::vector< T > & a ,
                 std::size_t s , T sign )
{
 if( r.size() < a.size() + s )
  r.resize( a.size() + s , T( 0 ) );
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  r[ i + s ] += sign * a[ i ];
 }

template< class T >
std::vector< T > multiplyRec( const std::vector< T > & p ,
                              const std::vector< T > & q , bool three ,
                              long * mults )
{
 ADS_PRE( ( p.size() == q.size() ) && ! p.empty() );
 const std::size_t n = p.size();
 if( n == 1 ) {
  if( mults ) ++*mults;
  return( std::vector< T >{ p[ 0 ] * q[ 0 ] } );
  }
 const std::size_t h = n / 2;                    // x^h splits them
 const std::vector< T > pD( p.begin() , p.begin() + h ) ,
                        pS( p.begin() + h , p.end() ) ,
                        qD( q.begin() , q.begin() + h ) ,
                        qS( q.begin() + h , q.end() );
 auto pad = []( std::vector< T > a , std::size_t m ) {
  a.resize( m , T( 0 ) );
  return( a );
  };
 const std::size_t m = n - h;                    // pS and qS, m >= h
 std::vector< T > r( 2 * n - 1 , T( 0 ) );
 const auto rS = multiplyRec( pS , qS , three , mults );
 const auto rD = multiplyRec( pad( pD , m ) , pad( qD , m ) , three , mults );
 addShifted( r , rS , 2 * h );
 addShifted( r , rD , 0 );
 if( three ) {
  std::vector< T > sp = pS , sq = qS;
  for( std::size_t i = 0 ; i < h ; ++i ) {
   sp[ i ] += pD[ i ];
   sq[ i ] += qD[ i ];
   }
  auto rM = multiplyRec( sp , sq , three , mults );
  addShifted( rM , rS , 0 , T( -1 ) );
  addShifted( rM , rD , 0 , T( -1 ) );
  addShifted( r , rM , h );
  }
 else {
  addShifted( r , multiplyRec( pad( pD , m ) , qS , three , mults ) , h );
  addShifted( r , multiplyRec( pS , pad( qD , m ) , three , mults ) , h );
  }
 r.resize( 2 * n - 1 );
 return( r );
 }

template< class T >
std::vector< T > multiplySame( std::vector< T > p , std::vector< T > q ,
                               bool three , long * mults )
{
 if( p.empty() || q.empty() )
  return( std::vector< T >() );
 const std::size_t np = p.size() , nq = q.size();
 const std::size_t n = std::max( np , nq );
 p.resize( n , T( 0 ) );
 q.resize( n , T( 0 ) );
 auto r = multiplyRec( p , q , three , mults );
 r.resize( np + nq - 1 );
 return( r );
 }

}  // end( namespace detail )

template< class T >
std::vector< T > multiplyFour( const std::vector< T > & p ,
                               const std::vector< T > & q ,
                               long * mults )
{
 auto r = detail::multiplySame( p , q , false , mults );
 ADS_AUDIT( r == multiplyNaive( p , q ) );
 return( r );
 }

template< class T >
std::vector< T > multiplyKaratsuba( const std::vector< T > & p ,
                                    const std::vector< T > & q ,
                                    long * mults )
{
 auto r = detail::multiplySame( p , q , true , mults );
 ADS_AUDIT( r == multiplyNaive( p , q ) );
 return( r );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Polynomial.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
