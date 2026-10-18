/*--------------------------------------------------------------------------*/
/*--------------------------- File Rational.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Rational [see Rational.h], with their contracts.
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

inline Rational::Rational( BigInteger num , BigInteger den )
{
 ADS_PRE( den.sign() != 0 );
 if( den.sign() < 0 ) {
  num = -num;
  den = -den;
  }
 const BigInteger g = BigInteger::gcd( num , den );
 f_num = num / g;
 f_den = den / g;
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
         ( BigInteger::gcd( f_num , f_den ) == BigInteger( 1 ) ) );
 }

inline std::string Rational::toString( void ) const
{
 if( f_den == BigInteger( 1 ) ) return( f_num.toString() );
 return( f_num.toString() + "/" + f_den.toString() );
 }

inline Rational operator+( const Rational & a , const Rational & b )
{
 const BigInteger d1 = BigInteger::gcd( a.f_den , b.f_den );
 if( d1 == BigInteger( 1 ) )
  return( Rational( a.f_num * b.f_den + b.f_num * a.f_den ,
                    a.f_den * b.f_den , Rational::Reduced() ) );
 const BigInteger t = a.f_num * ( b.f_den / d1 ) + b.f_num * ( a.f_den / d1 );
 const BigInteger d2 = BigInteger::gcd( t , d1 );
 if( t.sign() == 0 ) return( Rational() );
 return( Rational( t / d2 , ( a.f_den / d1 ) * ( b.f_den / d2 ) ,
                   Rational::Reduced() ) );
 }

inline Rational Rational::operator-( void ) const
{
 return( Rational( -f_num , f_den , Reduced() ) );
 }

inline Rational operator-( const Rational & a , const Rational & b )
{
 return( a + ( -b ) );
 }

inline Rational operator*( const Rational & a , const Rational & b )
{
 if( ( a.f_num.sign() == 0 ) || ( b.f_num.sign() == 0 ) ) return( Rational() );
 const BigInteger d1 = BigInteger::gcd( a.f_num , b.f_den ) ,
                  d2 = BigInteger::gcd( a.f_den , b.f_num );
 return( Rational( ( a.f_num / d1 ) * ( b.f_num / d2 ) ,
                   ( a.f_den / d2 ) * ( b.f_den / d1 ) ,
                   Rational::Reduced() ) );
 }

inline Rational operator/( const Rational & a , const Rational & b )
{
 ADS_PRE( b.f_num.sign() != 0 );
 const Rational inv = b.f_num.sign() > 0
                      ? Rational( b.f_den , b.f_num , Rational::Reduced() )
                      : Rational( -b.f_den , -b.f_num , Rational::Reduced() );
 return( a * inv );
 }

inline std::strong_ordering operator<=>( const Rational & a ,
                                         const Rational & b )
{
 return( a.f_num * b.f_den <=> b.f_num * a.f_den );   // denominators > 0
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Rational.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
