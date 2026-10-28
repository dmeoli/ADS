/*--------------------------------------------------------------------------*/
/*--------------------------- File Rational.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Rational [see Rational.h], with their contracts, and
 * the backend of GMP.
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

namespace detail {

/// x == 1, with no number built for the comparison
inline bool isOne( const BigInteger & x )
{
 static const BigInteger one( 1 );
 return( ( x.limbs() == 1 ) && ( x == one ) );
 }

#if ADS_RATIONAL_GMP
/// the fraction of GMP that receives the results of `Rational`, kept by
/// the thread, so that its digits are allocated once
struct RationalScratch {
 mpq_t q;
 RationalScratch( void ) { mpq_init( q ); }
 RationalScratch( const RationalScratch & ) = delete;
 RationalScratch & operator=( const RationalScratch & ) = delete;
 ~RationalScratch( void ) { mpq_clear( q ); }
 };

/// the fraction of `RationalScratch` of the calling thread
inline mpq_ptr rationalScratch( void )
{
 thread_local RationalScratch s;
 return( s.q );
 }
#endif

}  // end( namespace detail )

#if ADS_RATIONAL_GMP

inline bool Rational::gmp( const Rational & a , const Rational & b )
{
 const std::size_t n = std::max( std::max( a.f_num.limbs() ,
                                           a.f_den.limbs() ) ,
                                 std::max( b.f_num.limbs() ,
                                           b.f_den.limbs() ) );
 return( ( n >= kGmp ) && ( n < kGmpTop ) );
 }

inline mpq_srcptr Rational::gmpView( mpq_t q , const Rational & a )
{
 BigInteger::gmpView( mpq_numref( q ) , a.f_num );
 BigInteger::gmpView( mpq_denref( q ) , a.f_den );
 return( q );
 }

inline Rational Rational::gmpApply( const Rational & a , const Rational & b ,
                                    void ( *op )( mpq_ptr , mpq_srcptr ,
                                                  mpq_srcptr ) )
{
 mpq_t A , B;
 const mpq_ptr r = detail::rationalScratch();
 op( r , gmpView( A , a ) , gmpView( B , b ) );
 return( Rational( BigInteger::gmpNumber( mpq_numref( r ) ) ,
                   BigInteger::gmpNumber( mpq_denref( r ) ) , Reduced() ) );
 }

inline int Rational::gmpCompare( const Rational & a , const Rational & b )
{
 mpq_t A , B;
 return( mpq_cmp( gmpView( A , a ) , gmpView( B , b ) ) );
 }

#endif

inline Rational::Rational( BigInteger num , BigInteger den )
{
 ADS_PRE( den.sign() != 0 );
 if( den.sign() < 0 ) {
  num = -num;
  den = -den;
  }
 const BigInteger g = BigInteger::gcd( num , den );
 f_num = detail::isOne( g ) ? std::move( num )
                            : BigInteger::divExact( std::move( num ) , g );
 f_den = detail::isOne( g ) ? std::move( den )
                            : BigInteger::divExact( std::move( den ) , g );
 ADS_AUDIT( lowest() );
 }

inline Rational::Rational( BigInteger num , BigInteger den , Reduced )
 : f_num( std::move( num ) ) , f_den( std::move( den ) )
{
 ADS_AUDIT( lowest() );
 }

inline bool Rational::lowest( void ) const
{
 return( ( f_den.sign() > 0 ) &&
         detail::isOne( BigInteger::gcd( f_num , f_den ) ) );
 }

inline std::string Rational::toString( void ) const
{
 if( detail::isOne( f_den ) ) return( f_num.toString() );
 return( f_num.toString() + "/" + f_den.toString() );
 }

inline Rational operator+( const Rational & a , const Rational & b )
{
#if ADS_RATIONAL_GMP
 if( Rational::gmp( a , b ) ) return( Rational::gmpApply( a , b , mpq_add ) );
#endif
 const BigInteger d1 = BigInteger::gcd( a.f_den , b.f_den );
 if( detail::isOne( d1 ) ) {
  BigInteger t = a.f_num * b.f_den;         // in place
  BigInteger::addMul( t , b.f_num , a.f_den );
  return( Rational( std::move( t ) , a.f_den * b.f_den ,
                    Rational::Reduced() ) );
  }
 BigInteger ad = BigInteger::divExact( a.f_den , d1 );   // once
 BigInteger t = a.f_num * BigInteger::divExact( b.f_den , d1 );
 BigInteger::addMul( t , b.f_num , ad );
 if( t.sign() == 0 ) return( Rational() );
 const BigInteger d2 = BigInteger::gcd( t , d1 );
 if( detail::isOne( d2 ) )
  return( Rational( std::move( t ) , std::move( ad *= b.f_den ) ,
                    Rational::Reduced() ) );
 return( Rational( BigInteger::divExact( std::move( t ) , d2 ) ,
                   std::move( ad *= BigInteger::divExact( b.f_den , d2 ) ) ,
                   Rational::Reduced() ) );
 }

inline Rational Rational::operator-( void ) const
{
 return( Rational( -f_num , f_den , Reduced() ) );
 }

inline Rational operator-( const Rational & a , const Rational & b )
{
#if ADS_RATIONAL_GMP
 if( Rational::gmp( a , b ) ) return( Rational::gmpApply( a , b , mpq_sub ) );
#endif
 return( a + ( -b ) );
 }

inline Rational operator*( const Rational & a , const Rational & b )
{
 if( ( a.f_num.sign() == 0 ) || ( b.f_num.sign() == 0 ) ) return( Rational() );
#if ADS_RATIONAL_GMP
 if( Rational::gmp( a , b ) ) return( Rational::gmpApply( a , b , mpq_mul ) );
#endif
 const BigInteger d1 = BigInteger::gcd( a.f_num , b.f_den ) ,
                  d2 = BigInteger::gcd( a.f_den , b.f_num );
 const bool o1 = detail::isOne( d1 ) , o2 = detail::isOne( d2 );
 auto div = []( const BigInteger & x , bool one , const BigInteger & d ) {
  return( one ? x : BigInteger::divExact( x , d ) );
  };
 return( Rational( div( a.f_num , o1 , d1 ) * div( b.f_num , o2 , d2 ) ,
                   div( a.f_den , o2 , d2 ) * div( b.f_den , o1 , d1 ) ,
                   Rational::Reduced() ) );
 }

inline Rational operator/( const Rational & a , const Rational & b )
{
 ADS_PRE( b.f_num.sign() != 0 );
#if ADS_RATIONAL_GMP
 if( ( a.f_num.sign() != 0 ) && Rational::gmp( a , b ) )
  return( Rational::gmpApply( a , b , mpq_div ) );
#endif
 const Rational inv = b.f_num.sign() > 0
                      ? Rational( b.f_den , b.f_num , Rational::Reduced() )
                      : Rational( -b.f_den , -b.f_num , Rational::Reduced() );
 return( a * inv );
 }

inline std::strong_ordering operator<=>( const Rational & a ,
                                         const Rational & b )
{
 if( a.f_num.sign() != b.f_num.sign() )
  return( a.f_num.sign() <=> b.f_num.sign() );
#if ADS_RATIONAL_GMP
 if( Rational::gmp( a , b ) ) return( Rational::gmpCompare( a , b ) <=> 0 );
#endif
 return( a.f_num * b.f_den <=> b.f_num * a.f_den );   // denominators > 0
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Rational.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
