/*--------------------------------------------------------------------------*/
/*------------------------------ File RSA.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of RSA.h [see RSA.h].
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

template< class R >
U64 randomPrime( int bits , R & rng )
{
 ADS_PRE( ( bits >= 2 ) && ( bits <= 63 ) );
 std::uniform_int_distribution< U64 > d( U64( 1 ) << ( bits - 1 ) ,
                                         ( U64( 1 ) << bits ) - 1 );
 for( ; ; )
  if( const U64 c = d( rng ) | 1 ; millerRabin( c , 40 , rng ) ) {
   ADS_POST( ( c >> ( bits - 1 ) ) == 1 );
   return( c );
   }
 }

template< class R >
std::pair< RsaKey , RsaKey > rsaKeys( R & rng )
{
 for( ; ; ) {
  const U64 x = randomPrime( 31 , rng ) , y = randomPrime( 31 , rng );
  if( x == y ) continue;
  const U64 phi = ( x - 1 ) * ( y - 1 );
  const U64 s = randomPrime( 24 , rng );           // private, prime
  if( phi % s == 0 ) continue;
  const U64 p = U64( modInverse( static_cast< long long >( s ) ,
                                  static_cast< long long >( phi ) ) );
  const RsaKey pub{ x * y , p } , priv{ x * y , s };
  ADS_POST( ( rsa( rsa( 2 , pub ) , priv ) == 2 ) &&
            ( rsa( rsa( 2 , priv ) , pub ) == 2 ) );
  return( std::pair( pub , priv ) );
  }
 }

inline U64 rsa( U64 M , const RsaKey & k )
{
 ADS_PRE( M < k.N );
 return( powMod( M , k.e , k.N ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------------- End File RSA.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
