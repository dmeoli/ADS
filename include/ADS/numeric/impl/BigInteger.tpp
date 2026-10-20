/*--------------------------------------------------------------------------*/
/*-------------------------- File BigInteger.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of BigInteger [see BigInteger.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*------------------------- THE ARITHMETIC OF WORDS ------------------------*/
/*--------------------------------------------------------------------------*/

#if ADS_BIGINT_WIDE == 128
__extension__ typedef unsigned __int128 UInt128;
#endif

/// a b = hi 2^64 + lo, lo returned: one instruction where the compiler
/// has it, else the four products of the halves (Algorithm 4.3.1M)
inline std::uint64_t mulWide( std::uint64_t a , std::uint64_t b ,
                              std::uint64_t & hi )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 p = UInt128( a ) * b;
 hi = std::uint64_t( p >> 64 );
 return( std::uint64_t( p ) );
#elif ADS_BIGINT_WIDE == 64
 return( _umul128( a , b , &hi ) );
#else
 constexpr std::uint64_t M = 0xFFFFFFFFULL;
 const std::uint64_t a0 = a & M , a1 = a >> 32 , b0 = b & M , b1 = b >> 32 ,
                     p00 = a0 * b0 , p01 = a0 * b1 , p10 = a1 * b0 ,
                     mid = ( p00 >> 32 ) + ( p01 & M ) + ( p10 & M );
 hi = a1 * b1 + ( p01 >> 32 ) + ( p10 >> 32 ) + ( mid >> 32 );
 return( ( mid << 32 ) | ( p00 & M ) );
#endif
 }

/// a b + c + d = hi 2^64 + lo, lo returned, which never overflows
inline std::uint64_t mulAdd( std::uint64_t a , std::uint64_t b ,
                             std::uint64_t c , std::uint64_t d ,
                             std::uint64_t & hi )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 p = UInt128( a ) * b + c + d;
 hi = std::uint64_t( p >> 64 );
 return( std::uint64_t( p ) );
#else
 std::uint64_t lo = mulWide( a , b , hi );
 lo += c;
 hi += lo < c;
 lo += d;
 hi += lo < d;
 return( lo );
#endif
 }

/// a + b + c, c in { 0 , 1 }, the carry out in c
inline std::uint64_t addCarry( std::uint64_t a , std::uint64_t b ,
                               std::uint64_t & c )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 s = UInt128( a ) + b + c;
 c = std::uint64_t( s >> 64 );
 return( std::uint64_t( s ) );
#elif ADS_BIGINT_WIDE == 64
 unsigned long long s;
 c = _addcarry_u64( (unsigned char)( c ) , a , b , &s );
 return( s );
#else
 const std::uint64_t s = a + b , t = s + c;
 c = std::uint64_t( s < a ) | std::uint64_t( t < s );
 return( t );
#endif
 }

/// a - b - c, c in { 0 , 1 }, the borrow out in c
inline std::uint64_t subBorrow( std::uint64_t a , std::uint64_t b ,
                                std::uint64_t & c )
{
#if ADS_BIGINT_WIDE == 128
 const UInt128 s = UInt128( a ) - b - c;
 c = std::uint64_t( s >> 64 ) & 1;
 return( std::uint64_t( s ) );
#elif ADS_BIGINT_WIDE == 64
 unsigned long long s;
 c = _subborrow_u64( (unsigned char)( c ) , a , b , &s );
 return( s );
#else
 const std::uint64_t s = a - b , t = s - c;
 c = std::uint64_t( a < b ) | std::uint64_t( s < c );
 return( t );
#endif
 }

/// floor( ( u1 2^64 + u0 ) / d ) for d with the top bit set and u1 < d;
/// in the portable form the division of two digits of 32 bits by one
/// twice (Hacker's Delight, figure 9-3), also for clang-cl, whose division
/// of unsigned __int128 needs a library that MSVC lacks
inline std::uint64_t divWide( std::uint64_t u1 , std::uint64_t u0 ,
                              std::uint64_t d )
{
#if ( ADS_BIGINT_WIDE == 128 ) && ! defined( _MSC_VER )
 return( std::uint64_t( ( ( UInt128( u1 ) << 64 ) | u0 ) / d ) );
#elif ADS_BIGINT_WIDE == 64
 unsigned long long r;
 return( _udiv128( u1 , u0 , d , &r ) );
#else
 constexpr std::uint64_t B = std::uint64_t( 1 ) << 32 , M = B - 1;
 const std::uint64_t d1 = d >> 32 , d0 = d & M , n1 = u0 >> 32 ,
                     n0 = u0 & M;
 std::uint64_t q1 = u1 / d1 , r = u1 - q1 * d1;
 while( ( q1 >= B ) || ( q1 * d0 > ( ( r << 32 ) | n1 ) ) ) {
  --q1;
  r += d1;
  if( r >= B ) break;
  }
 const std::uint64_t u21 = ( u1 << 32 ) + n1 - q1 * d;   // modulo 2^64
 std::uint64_t q0 = u21 / d1;
 r = u21 - q0 * d1;
 while( ( q0 >= B ) || ( q0 * d0 > ( ( r << 32 ) | n0 ) ) ) {
  --q0;
  r += d1;
  if( r >= B ) break;
  }
 return( ( q1 << 32 ) + q0 );
#endif
 }

/// floor( ( 2^19 - 3 2^8 ) / d ) for the 2^8 values of d of 9 bits
inline constexpr auto kRecip9 = []() {
 std::array< std::uint16_t , 256 > t{};
 for( std::uint32_t i = 0 ; i < 256 ; ++i )
  t[ i ] = std::uint16_t( 0x7FD00u / ( 256 + i ) );
 return( t );
 }();

/// the reciprocal of d with the top bit set, floor( ( b^2 - 1 ) / d ) - b:
/// 11 bits from the 9 leading ones of d, then three steps of Newton and a
/// last correction (Moeller and Granlund, Algorithm 2), with no division
inline std::uint64_t reciprocal( std::uint64_t d )
{
 const std::uint64_t d0 = d & 1 , d40 = ( d >> 24 ) + 1 ,
                     d63 = ( d >> 1 ) + d0 ,
                     v0 = kRecip9[ ( d >> 55 ) - 256 ] ,
                     v1 = ( v0 << 11 ) - ( ( v0 * v0 * d40 ) >> 40 ) - 1 ,
                     v2 = ( v1 << 13 ) +
                          ( ( v1 * ( ( std::uint64_t( 1 ) << 60 ) -
                                     v1 * d40 ) ) >> 47 ) ,
                     e = ( ( v2 >> 1 ) & ( 0 - d0 ) ) - v2 * d63;
 std::uint64_t h;
 mulWide( v2 , e , h );
 const std::uint64_t v3 = ( v2 << 31 ) + ( h >> 1 ) ,
                     l = mulWide( v3 , d , h );
 return( v3 - h - ( l + d < l ) - d );      // v3 - ( v3 d + d ) / b - d
 }

/// the quotient of u1 b + u0 by d with the top bit set, u1 < d, by the
/// reciprocal v of d (Moeller and Granlund, Algorithm 4), the remainder in
/// r; the sum is modulo b^2
inline std::uint64_t div2by1( std::uint64_t u1 , std::uint64_t u0 ,
                              std::uint64_t d , std::uint64_t v ,
                              std::uint64_t & r )
{
 std::uint64_t q1;
 const std::uint64_t q0 = mulAdd( v , u1 , u0 , 0 , q1 );
 q1 += u1 + 1;
 r = u0 - q1 * d;
 if( r > q0 ) {
  --q1;
  r += d;
  }
 if( r >= d ) {                             // rarely
  ++q1;
  r -= d;
  }
 return( q1 );
 }

#if ADS_BIGINT_ASM
// The loops on n >= 1 digits: 1, 2 and 4 steps first, as the bits of n
// say, then the blocks of 8 steps. MULX leaves the flags alone, and MOV,
// LEA and JRCXZ too, so the chains of carries run through all of them. In
// r = a m the high half of each product is added to the low half of the
// next on the chain of ADCX (CF). In r += a m that sum t_s for the digit s
// is made on the same chain, and added to r_s on the chain of ADOX (OF)
// one step later, after the next product, so that the two chains do not
// wait for each other; in r -= a m the complement of t_s is added, the
// chain of OF starting at 1, and the borrow is the complement of its
// carry. The steps work by turns on the registers w0 , w1 and w2 , w3
// (the low and the high half), and between the groups of steps t is in w2
// and the high half in w3.

/// the step s of r += a m: the product a_s m in LO and HI, the sum t of
/// the step before, in PL with its high half PH, added to r; the even
/// steps add t to r first, the odd ones make their t first
#define ADS_ADDMUL_STEP( s , LO , HI , PL , PH )                             \
 "mulx " #s "*8(%[a]), %[" #LO "], %[" #HI "]\n\t"                           \
 "adox " #s "*8-8(%[r]), %[" #PL "]\n\t"                                     \
 "mov %[" #PL "], " #s "*8-8(%[r])\n\t"                                      \
 "adcx %[" #PH "], %[" #LO "]\n\t"

#define ADS_ADDMUL_STEP_ODD( s , LO , HI , PL , PH )                         \
 "mulx " #s "*8(%[a]), %[" #LO "], %[" #HI "]\n\t"                           \
 "adcx %[" #PH "], %[" #LO "]\n\t"                                           \
 "adox " #s "*8-8(%[r]), %[" #PL "]\n\t"                                     \
 "mov %[" #PL "], " #s "*8-8(%[r])\n\t"

/// the step s of r -= a m
#define ADS_SUBMUL_STEP( s , LO , HI , PL , PH )                             \
 "mulx " #s "*8(%[a]), %[" #LO "], %[" #HI "]\n\t"                           \
 "adox " #s "*8-8(%[r]), %[" #PL "]\n\t"                                     \
 "mov %[" #PL "], " #s "*8-8(%[r])\n\t"                                      \
 "adcx %[" #PH "], %[" #LO "]\n\t"                                           \
 "not %[" #LO "]\n\t"

#define ADS_SUBMUL_STEP_ODD( s , LO , HI , PL , PH )                         \
 "mulx " #s "*8(%[a]), %[" #LO "], %[" #HI "]\n\t"                           \
 "adcx %[" #PH "], %[" #LO "]\n\t"                                           \
 "not %[" #LO "]\n\t"                                                        \
 "adox " #s "*8-8(%[r]), %[" #PL "]\n\t"                                     \
 "mov %[" #PL "], " #s "*8-8(%[r])\n\t"

/// the step s of r = a m
#define ADS_MUL1_STEP( s , LO , HI , PL , PH )                               \
 "mulx " #s "*8(%[a]), %[" #LO "], %[" #HI "]\n\t"                           \
 "adcx %[" #PH "], %[" #LO "]\n\t"                                           \
 "mov %[" #LO "], " #s "*8(%[r])\n\t"

#define ADS_MUL1_STEP_ODD ADS_MUL1_STEP

/// the steps s and s + 1
#define ADS_STEP2( STEP , s , t )                                            \
 STEP( s , w0 , w1 , w2 , w3 ) STEP##_ODD( t , w2 , w3 , w0 , w1 )

/// the steps of a loop on the digits of a and r, after the first if any:
/// 1, 2 and 4 steps if M1, M2 and M4 are not 0, then M8 blocks of 8
/// steps, the counts in registers or in memory; the high half at the end
/// in w3
#define ADS_LADDER( STEP , M1 , M2 , M4 , M8 )                               \
 "mov " M1 ", %[c]\n\t"                                                      \
 "jrcxz 1f\n\t"                                                              \
 STEP( 0 , w0 , w1 , w2 , w3 )                                               \
 "mov %[w0], %[w2]\n\t"                                                      \
 "mov %[w1], %[w3]\n\t"                                                      \
 "lea 8(%[a]), %[a]\n\t"                                                     \
 "lea 8(%[r]), %[r]\n"                                                       \
 "1:\n\t"                                                                    \
 "mov " M2 ", %[c]\n\t"                                                      \
 "jrcxz 2f\n\t"                                                              \
 ADS_STEP2( STEP , 0 , 1 )                                                   \
 "lea 16(%[a]), %[a]\n\t"                                                    \
 "lea 16(%[r]), %[r]\n"                                                      \
 "2:\n\t"                                                                    \
 "mov " M4 ", %[c]\n\t"                                                      \
 "jrcxz 3f\n\t"                                                              \
 ADS_STEP2( STEP , 0 , 1 ) ADS_STEP2( STEP , 2 , 3 )                         \
 "lea 32(%[a]), %[a]\n\t"                                                    \
 "lea 32(%[r]), %[r]\n"                                                      \
 "3:\n\t"                                                                    \
 "mov " M8 ", %[c]\n\t"                                                      \
 "jmp 7f\n\t"                                                                \
 ".p2align 5\n"                                                              \
 "9:\n\t"                                                                    \
 ADS_STEP2( STEP , 0 , 1 ) ADS_STEP2( STEP , 2 , 3 )                         \
 ADS_STEP2( STEP , 4 , 5 ) ADS_STEP2( STEP , 6 , 7 )                         \
 "lea 64(%[a]), %[a]\n\t"                                                    \
 "lea 64(%[r]), %[r]\n\t"                                                    \
 "lea -1(%[c]), %[c]\n"                                                      \
 "7:\n\t"                                                                    \
 "jrcxz 8f\n\t"                                                              \
 "jmp 9b\n"                                                                  \
 "8:\n\t"

/// the loop of the products by one digit with the steps STEP, started by
/// START, the first digit by FIRST, which leaves t in w2 and the high half
/// in w3 and moves a and r one digit, ended by FOLD, which folds the
/// carries into w3
#define ADS_MUL_LOOP( STEP , START , FIRST , FOLD )                          \
 __asm__ __volatile__(                                                       \
          START                                                              \
          FIRST                                                              \
          ADS_LADDER( STEP , "%[n1]" , "%[n2]" , "%[n4]" , "%[n8]" )         \
          FOLD                                                               \
          : [ w0 ] "=&r" ( w0 ) , [ w1 ] "=&r" ( w1 ) , [ w2 ] "=&r" ( w2 ) , \
            [ w3 ] "=&r" ( w3 ) , [ c ] "=&c" ( c ) , [ a ] "+&r" ( a ) ,   \
            [ r ] "+&r" ( r )                                                \
          : "d" ( m ) , [ n1 ] "r" ( n1 & 1 ) , [ n2 ] "r" ( n1 & 2 ) ,      \
            [ n4 ] "r" ( n1 & 4 ) , [ n8 ] "r" ( n1 / 8 )                    \
          : "cc" , "memory" )

/// the first digit of r += a m and r -= a m, its t in w2
#define ADS_FIRST_ADD                                                        \
 "mulx (%[a]), %[w2], %[w3]\n\t"                                             \
 "lea 8(%[a]), %[a]\n\t"                                                     \
 "lea 8(%[r]), %[r]\n\t"

#define ADS_FIRST_SUB ADS_FIRST_ADD "not %[w2]\n\t"

/// the last t of r += a m and r -= a m into r, and the carries into w3
#define ADS_FOLD_ADD                                                         \
 "adox -8(%[r]), %[w2]\n\t"                                                  \
 "mov %[w2], -8(%[r])\n\t"                                                   \
 "mov $0, %k[w0]\n\t"                                                        \
 "adcx %[w0], %[w3]\n\t"

/// r = a m, n > 0 digits, the carry
inline std::uint64_t mul1Asm( std::uint64_t * r , const std::uint64_t * a ,
                              std::size_t n , std::uint64_t m )
{
 std::uint64_t c , w0 , w1 , w2 , w3;
 const std::size_t n1 = n;
 ADS_MUL_LOOP( ADS_MUL1_STEP , "xor %k[w3], %k[w3]\n\t" , "" ,
               "mov $0, %k[w0]\n\t" "adcx %[w0], %[w3]" );
 return( w3 );
 }

/// r += a m, n > 0 digits, the carry
inline std::uint64_t addMul1Adx( std::uint64_t * r , const std::uint64_t * a ,
                                 std::size_t n , std::uint64_t m )
{
 std::uint64_t c , w0 , w1 , w2 , w3;
 const std::size_t n1 = n - 1;
 ADS_MUL_LOOP( ADS_ADDMUL_STEP , "xor %k[w0], %k[w0]\n\t" , ADS_FIRST_ADD ,
               ADS_FOLD_ADD "adox %[w0], %[w3]" );
 return( w3 );
 }

/// r -= a m, n > 0 digits a multiple of 4, the borrow, c subtracted at the
/// bottom too: in each block of 4 digits the products are summed on the
/// chain of ADC, the top half absorbing the last carry, then subtracted
/// from r on the chain of SBB, whose borrow goes into the next block
inline std::uint64_t subMul1Asm( std::uint64_t * r , const std::uint64_t * a ,
                                 std::size_t n , std::uint64_t m ,
                                 std::uint64_t c = 0 )
{
 std::uint64_t i = n / 4 , l0 , l1 , l2 , l3 , h0 , h1;
 __asm__ __volatile__(
  "xor %k[h0], %k[h0]\n\t"
  ".p2align 4\n"
  "1:\n\t"
  "mulx (%[a]), %[l0], %[h0]\n\t"
  "mulx 8(%[a]), %[l1], %[h1]\n\t"
  "adc %[c], %[l0]\n\t"
  "adc %[h0], %[l1]\n\t"
  "mulx 16(%[a]), %[l2], %[h0]\n\t"
  "mulx 24(%[a]), %[l3], %[c]\n\t"
  "adc %[h1], %[l2]\n\t"
  "adc %[h0], %[l3]\n\t"
  "adc $0, %[c]\n\t"
  "mov (%[r]), %[h0]\n\t"
  "sub %[l0], %[h0]\n\t"
  "mov %[h0], (%[r])\n\t"
  "mov 8(%[r]), %[h0]\n\t"
  "sbb %[l1], %[h0]\n\t"
  "mov %[h0], 8(%[r])\n\t"
  "mov 16(%[r]), %[h0]\n\t"
  "sbb %[l2], %[h0]\n\t"
  "mov %[h0], 16(%[r])\n\t"
  "mov 24(%[r]), %[h0]\n\t"
  "sbb %[l3], %[h0]\n\t"
  "mov %[h0], 24(%[r])\n\t"
  "lea 32(%[a]), %[a]\n\t"
  "lea 32(%[r]), %[r]\n\t"
  "dec %[i]\n\t"
  "jnz 1b\n\t"
  "adc $0, %[c]"
  : [ c ] "+&r" ( c ) , [ i ] "+&r" ( i ) , [ a ] "+&r" ( a ) ,
    [ r ] "+&r" ( r ) , [ l0 ] "=&r" ( l0 ) , [ l1 ] "=&r" ( l1 ) ,
    [ l2 ] "=&r" ( l2 ) , [ l3 ] "=&r" ( l3 ) , [ h0 ] "=&r" ( h0 ) ,
    [ h1 ] "=&r" ( h1 )
  : "d" ( m )
  : "cc" , "memory" );
 return( c );
 }

/// r -= a m for n = K digits, 2 <= K <= 8, the steps of the loop in a
/// row with no count: the first digit, K - 1 steps, the last t into r
#define ADS_SUBMUL_SHORT( STEPS , FIX , K )                                  \
 __asm__ __volatile__(                                                       \
  "movabs $0x7FFFFFFFFFFFFFFF, %[w0]\n\t"                                   \
  "add $1, %[w0]\n\t"                                                        \
  ADS_FIRST_SUB                                                              \
  STEPS                                                                      \
  FIX                                                                        \
  "lea 8*" #K "-8(%[r]), %[r]\n\t"                                          \
  ADS_FOLD_ADD                                                               \
  "seto %b[w0]\n\t"                                                          \
  "lea 1(%[w3]), %[w3]\n\t"                                                  \
  "sub %[w0], %[w3]"                                                         \
  : [ w0 ] "=&r" ( w0 ) , [ w1 ] "=&r" ( w1 ) , [ w2 ] "=&r" ( w2 ) ,        \
    [ w3 ] "=&r" ( w3 ) , [ a ] "+&r" ( a ) , [ r ] "+&r" ( r )             \
  : "d" ( m )                                                                \
  : "cc" , "memory" )

/// the first k steps after the first digit, by turns on the registers
#define ADS_SM_1 ADS_SUBMUL_STEP( 0 , w0 , w1 , w2 , w3 )
#define ADS_SM_2 ADS_SM_1 ADS_SUBMUL_STEP_ODD( 1 , w2 , w3 , w0 , w1 )
#define ADS_SM_3 ADS_SM_2 ADS_SUBMUL_STEP( 2 , w0 , w1 , w2 , w3 )
#define ADS_SM_4 ADS_SM_3 ADS_SUBMUL_STEP_ODD( 3 , w2 , w3 , w0 , w1 )
#define ADS_SM_5 ADS_SM_4 ADS_SUBMUL_STEP( 4 , w0 , w1 , w2 , w3 )
#define ADS_SM_6 ADS_SM_5 ADS_SUBMUL_STEP_ODD( 5 , w2 , w3 , w0 , w1 )
#define ADS_SM_7 ADS_SM_6 ADS_SUBMUL_STEP( 6 , w0 , w1 , w2 , w3 )
#define ADS_SM_FIX "mov %[w0], %[w2]\n\t" "mov %[w1], %[w3]\n\t"

/// r -= a m, 2 <= n <= 8 digits, the borrow, with no loop
inline std::uint64_t subMul1Short( std::uint64_t * r ,
                                   const std::uint64_t * a , std::size_t n ,
                                   std::uint64_t m )
{
 std::uint64_t w0 , w1 , w2 , w3;
 switch( n ) {
  case 2: ADS_SUBMUL_SHORT( ADS_SM_1 , ADS_SM_FIX , 2 ); break;
  case 3: ADS_SUBMUL_SHORT( ADS_SM_2 , "" , 3 ); break;
  case 4: ADS_SUBMUL_SHORT( ADS_SM_3 , ADS_SM_FIX , 4 ); break;
  case 5: ADS_SUBMUL_SHORT( ADS_SM_4 , "" , 5 ); break;
  case 6: ADS_SUBMUL_SHORT( ADS_SM_5 , ADS_SM_FIX , 6 ); break;
  case 7: ADS_SUBMUL_SHORT( ADS_SM_6 , "" , 7 ); break;
  default: ADS_SUBMUL_SHORT( ADS_SM_7 , ADS_SM_FIX , 8 ); break;
  }
 return( w3 );
 }

/// the masks of a ladder on n digits, in memory, by the register T
#define ADS_MASKS( N , T )                                                   \
 "mov " N ", " T "\n\t" "and $1, " T "\n\t" "mov " T ", %[m1]\n\t"           \
 "mov " N ", " T "\n\t" "and $2, " T "\n\t" "mov " T ", %[m2]\n\t"           \
 "mov " N ", " T "\n\t" "and $4, " T "\n\t" "mov " T ", %[m4]\n\t"           \
 "mov " N ", " T "\n\t" "shr $3, " T "\n\t" "mov " T ", %[m8]\n\t"

/// the rows of a product [see mulRows()], the masks of the rows after the
/// first made again at each row by MASKS
#define ADS_ROWS( MASKS )                                                    \
 __asm__ __volatile__(                                                       \
  "mov (%[b]), %%rdx\n\t"                                                    \
  "xor %k[w3], %k[w3]\n\t"                                                   \
  ADS_LADDER( ADS_MUL1_STEP , "%[f1]" , "%[f2]" , "%[f4]" , "%[f8]" )        \
  "mov $0, %k[w0]\n\t"                                                       \
  "adcx %[w0], %[w3]\n\t"                                                    \
  "mov %[w3], (%[r])\n\t"                                                    \
  "jmp 5f\n"                                                                 \
  "4:\n\t"                                                                   \
  MASKS                                                                      \
  "mov (%[b]), %%rdx\n\t"                                                    \
  "xor %k[w0], %k[w0]\n\t"                                                   \
  ADS_FIRST_ADD                                                              \
  ADS_LADDER( ADS_ADDMUL_STEP , "%[m1]" , "%[m2]" , "%[m4]" , "%[m8]" )      \
  ADS_FOLD_ADD                                                               \
  "adox %[w0], %[w3]\n\t"                                                    \
  "mov %[w3], (%[r])\n"                                                      \
  "5:\n\t"                                                                   \
  "lea 8(%[b]), %[b]\n\t"                                                    \
  "mov %[len], %[w0]\n\t"                                                    \
  "shl $3, %[w0]\n\t"                                                        \
  "sub %[w0], %[a]\n\t"                                                      \
  "sub %[w0], %[r]\n\t"                                                      \
  "lea %c[da](%[a]), %[a]\n\t"                                               \
  "lea %c[dr](%[r]), %[r]\n\t"                                               \
  "lea %c[dl](%[len]), %[len]\n\t"                                           \
  "dec %[rows]\n\t"                                                          \
  "jnz 4b"                                                                   \
  : [ w0 ] "=&r" ( w0 ) , [ w1 ] "=&r" ( w1 ) , [ w2 ] "=&r" ( w2 ) ,        \
    [ w3 ] "=&r" ( w3 ) , [ c ] "=&c" ( c ) , [ a ] "+&r" ( a ) ,           \
    [ r ] "+&r" ( r ) , [ b ] "+&r" ( b ) , [ len ] "+&r" ( len ) ,         \
    [ rows ] "+&r" ( rows ) , [ m1 ] "+m" ( m[ 0 ] ) ,                       \
    [ m2 ] "+m" ( m[ 1 ] ) , [ m4 ] "+m" ( m[ 2 ] ) , [ m8 ] "+m" ( m[ 3 ] ) \
  : [ f1 ] "m" ( f[ 0 ] ) , [ f2 ] "m" ( f[ 1 ] ) , [ f4 ] "m" ( f[ 2 ] ) ,  \
    [ f8 ] "m" ( f[ 3 ] ) , [ da ] "i" ( 8 * da ) , [ dr ] "i" ( 8 * dr ) ,  \
    [ dl ] "i" ( -dl )                                                       \
  : "rdx" , "cc" , "memory" )

/// the rows of a product, R > 0 of them: r[ 0 , L ] = a m_0, then for
/// j = 1 , ... , R - 1, with a_j = a + j da, r_j = r + j dr, m_j = b[ j ]
/// and L_j = L - j dl >= 1, r_j[ 0 , L_j ] = r_j[ 0 , L_j ) + a_j m_j: the
/// product a b of L and R digits for da = dl = 0 and dr = 1, the products
/// a_i a_j, i < j, of a square for b = a, a = a + 1, r = r + 1, L = n - 1,
/// R = n - 1 and da = dl = 1, dr = 2
template< int da , int dr , int dl >
inline void mulRows( std::uint64_t * r , const std::uint64_t * a ,
                     std::size_t L , const std::uint64_t * b , std::size_t R )
{
 std::uint64_t c , w0 , w1 , w2 , w3 , len = L , rows = R;
 const std::uint64_t f[ 4 ] = { L & 1 , L & 2 , L & 4 , L / 8 };
 std::uint64_t m[ 4 ] = { ( L - 1 ) & 1 , ( L - 1 ) & 2 , ( L - 1 ) & 4 ,
                          ( L - 1 ) / 8 };
 if constexpr( dl == 0 ) ADS_ROWS( "" );
 else ADS_ROWS( "lea -1(%[len]), %[w0]\n\t" ADS_MASKS( "%[w0]" , "%[w1]" ) );
 }

// The product of a by 4 digits of b in one pass over a, a digit a_i at a
// time: the window P0 , ... , P4 holds the sums for the places i to i + 4,
// the last one new and 0; r_i is added to P0 on the chain of ADOX (OF),
// then each product a_i b_c, c < 4, its low half to P_c on the chain of
// ADCX (CF) and its high half to P_{c+1} on that of OF, the carry of CF
// into P4 at the end; P0 then holds the digit i, and the sum of the window
// with a_i b and r_i, below b^5, leaves no carry out of P4, so both flags
// are 0 again for the next digit. Per digit: 4 products, 22 instructions
// of the front end, about 1.35 cycles per product on a Skylake against
// the 1.45 to 2 of the rows of r += a m. The window turns by one register
// per digit, so the loop is 5 digits long and is entered at the digit
// ( -n ) mod 5 by a table of jumps, the pointers moved back as many digits.

/// the digit s of the pass, the window ( P0 , ... , P4 ), and r_i added to
/// P0 by RADD
#define ADS_PASS_STEP( s , RADD , STORE , P0 , P1 , P2 , P3 , P4 )           \
 "1" #s ":\n\t"                                                              \
 "mov " #s "*8(%[a]), %%rdx\n\t"                                             \
 "mov $0, %k[" #P4 "]\n\t"                                                   \
 RADD( s , P0 )                                                              \
 "mulx (%[b]), %[lo], %[hi]\n\t"                                             \
 "adcx %[lo], %[" #P0 "]\n\t"                                                \
 "adox %[hi], %[" #P1 "]\n\t"                                                \
 "mulx 8(%[b]), %[lo], %[hi]\n\t"                                            \
 "adcx %[lo], %[" #P1 "]\n\t"                                                \
 "adox %[hi], %[" #P2 "]\n\t"                                                \
 "mulx 16(%[b]), %[lo], %[hi]\n\t"                                           \
 "adcx %[lo], %[" #P2 "]\n\t"                                                \
 "adox %[hi], %[" #P3 "]\n\t"                                                \
 "mulx 24(%[b]), %[lo], %[hi]\n\t"                                           \
 "adcx %[lo], %[" #P3 "]\n\t"                                                \
 "adox %[hi], %[" #P4 "]\n\t"                                                \
 "adcx %[z], %[" #P4 "]\n\t"                                                 \
 STORE( s , P0 )

#define ADS_PASS_RADD( s , P0 ) "adox " #s "*8(%[r]), %[" #P0 "]\n\t"
#define ADS_PASS_RNONE( s , P0 )
#define ADS_PASS_RNOT( s , P0 )                                              \
 "mov " #s "*8(%[r]), %[lo]\n\t"                                             \
 "not %[lo]\n\t"                                                             \
 "adox %[lo], %[" #P0 "]\n\t"
#define ADS_PASS_STORE( s , P0 ) "mov %[" #P0 "], " #s "*8(%[r])\n\t"
#define ADS_PASS_STNOT( s , P0 )                                             \
 "not %[" #P0 "]\n\t"                                                        \
 "mov %[" #P0 "], " #s "*8(%[r])\n\t"

/// the pass, r_i added by RADD, the digits stored by STORE
#define ADS_PASS( RADD , STORE )                                             \
 __asm__ __volatile__(                                                       \
  "xor %k[w0], %k[w0]\n\t"                                                   \
  "xor %k[w1], %k[w1]\n\t"                                                   \
  "xor %k[w2], %k[w2]\n\t"                                                   \
  "xor %k[w3], %k[w3]\n\t"                                                   \
  "xor %k[w4], %k[w4]\n\t"                                                   \
  "lea 7f(%%rip), %[lo]\n\t"                                                 \
  "movslq (%[lo],%[e],4), %[hi]\n\t"                                         \
  "add %[lo], %[hi]\n\t"                                                     \
  "xor %k[lo], %k[lo]\n\t"                                                   \
  "jmp *%[hi]\n\t"                                                           \
  ".p2align 2\n"                                                             \
  "7:\n\t"                                                                   \
  ".long 10f-7b, 11f-7b, 12f-7b, 13f-7b, 14f-7b\n\t"                         \
  ".p2align 5\n"                                                             \
  "9:\n\t"                                                                   \
  ADS_PASS_STEP( 0 , RADD , STORE , w0 , w1 , w2 , w3 , w4 )                 \
  ADS_PASS_STEP( 1 , RADD , STORE , w1 , w2 , w3 , w4 , w0 )                 \
  ADS_PASS_STEP( 2 , RADD , STORE , w2 , w3 , w4 , w0 , w1 )                 \
  ADS_PASS_STEP( 3 , RADD , STORE , w3 , w4 , w0 , w1 , w2 )                 \
  ADS_PASS_STEP( 4 , RADD , STORE , w4 , w0 , w1 , w2 , w3 )                 \
  "lea 40(%[a]), %[a]\n\t"                                                   \
  "lea 40(%[r]), %[r]\n\t"                                                   \
  "dec %[c]\n\t"                                                             \
  "jnz 9b\n\t"                                                               \
  "mov %[w0], (%[r])\n\t"                                                    \
  "mov %[w1], 8(%[r])\n\t"                                                   \
  "mov %[w2], 16(%[r])\n\t"                                                  \
  "mov %[w3], 24(%[r])"                                                      \
  : [ w0 ] "=&r" ( w0 ) , [ w1 ] "=&r" ( w1 ) , [ w2 ] "=&r" ( w2 ) ,        \
    [ w3 ] "=&r" ( w3 ) , [ w4 ] "=&r" ( w4 ) , [ lo ] "=&r" ( lo ) ,       \
    [ hi ] "=&r" ( hi ) , [ c ] "+&r" ( c ) , [ a ] "+&r" ( pa ) ,          \
    [ r ] "+&r" ( pr )                                                       \
  : [ b ] "r" ( b ) , [ z ] "r" ( std::uint64_t( 0 ) ) , [ e ] "r" ( e )     \
  : "rdx" , "cc" , "memory" )

/// r[ 0 , n + 4 ) = a[ 0 , n ) b[ 0 , 4 ) if not add, else
/// r[ 0 , n + 4 ) = r[ 0 , n ) + a b, n > 0
template< bool add >
inline void mulPass4( std::uint64_t * r , const std::uint64_t * a ,
                      std::size_t n , const std::uint64_t * b )
{
 const std::uint64_t e = ( 5 - n % 5 ) % 5;
 std::uint64_t w0 , w1 , w2 , w3 , w4 , lo , hi , c = ( n + e ) / 5 ,
               pa = std::uint64_t( reinterpret_cast< std::uintptr_t >( a ) ) -
                    8 * e ,
               pr = std::uint64_t( reinterpret_cast< std::uintptr_t >( r ) ) -
                    8 * e;
 if constexpr( add ) ADS_PASS( ADS_PASS_RADD , ADS_PASS_STORE );
 else ADS_PASS( ADS_PASS_RNONE , ADS_PASS_STORE );
 }

/// r[ 0 , n ) = ~( ~r[ 0 , n ) + a b ) and r[ n , n + 4 ) the top 4 digits
/// of ~r[ 0 , n ) + a b, n > 0: so that r - a b, r of n + 4 digits, is
/// ( r_top - s ) b^n + r[ 0 , n ), s the top digits, r_top the old ones
inline void subPass4( std::uint64_t * r , const std::uint64_t * a ,
                      std::size_t n , const std::uint64_t * b )
{
 const std::uint64_t e = ( 5 - n % 5 ) % 5;
 std::uint64_t w0 , w1 , w2 , w3 , w4 , lo , hi , c = ( n + e ) / 5 ,
               pa = std::uint64_t( reinterpret_cast< std::uintptr_t >( a ) ) -
                    8 * e ,
               pr = std::uint64_t( reinterpret_cast< std::uintptr_t >( r ) ) -
                    8 * e;
 ADS_PASS( ADS_PASS_RNOT , ADS_PASS_STNOT );
 }

/// r[ 0 , 8 ) = a[ 0 , 4 )^2: the products a_i a_j, i < j, at the places 1
/// to 6 in t1 , ... , t6, on the chains of CF and OF, then 2 t on the
/// chain of OF and the squares a_i^2 on that of CF
inline void sqr4Asm( std::uint64_t * r , const std::uint64_t * a )
{
 std::uint64_t t1 , t2 , t3 , t4 , t5 , t6 , lo , hi , z;
 __asm__ __volatile__(
  "xor %k[z], %k[z]\n\t"
  "mov (%[a]), %%rdx\n\t"
  "mulx 8(%[a]), %[t1], %[t2]\n\t"
  "mulx 16(%[a]), %[lo], %[t3]\n\t"
  "adcx %[lo], %[t2]\n\t"
  "mulx 24(%[a]), %[lo], %[t4]\n\t"
  "adcx %[lo], %[t3]\n\t"
  "mov 8(%[a]), %%rdx\n\t"
  "mulx 16(%[a]), %[lo], %[hi]\n\t"
  "adox %[lo], %[t3]\n\t"
  "adcx %[hi], %[t4]\n\t"
  "mulx 24(%[a]), %[lo], %[t5]\n\t"
  "adox %[lo], %[t4]\n\t"
  "adcx %[z], %[t5]\n\t"
  "mov 16(%[a]), %%rdx\n\t"
  "mulx 24(%[a]), %[lo], %[t6]\n\t"
  "adox %[lo], %[t5]\n\t"
  "adox %[z], %[t6]\n\t"
  "adcx %[z], %[t6]\n\t"
  "xor %k[z], %k[z]\n\t"
  "mov (%[a]), %%rdx\n\t"
  "mulx %%rdx, %[lo], %[hi]\n\t"
  "mov %[lo], (%[r])\n\t"
  "adox %[t1], %[t1]\n\t"
  "adcx %[hi], %[t1]\n\t"
  "mov %[t1], 8(%[r])\n\t"
  "mov 8(%[a]), %%rdx\n\t"
  "mulx %%rdx, %[lo], %[hi]\n\t"
  "adox %[t2], %[t2]\n\t"
  "adcx %[lo], %[t2]\n\t"
  "mov %[t2], 16(%[r])\n\t"
  "adox %[t3], %[t3]\n\t"
  "adcx %[hi], %[t3]\n\t"
  "mov %[t3], 24(%[r])\n\t"
  "mov 16(%[a]), %%rdx\n\t"
  "mulx %%rdx, %[lo], %[hi]\n\t"
  "adox %[t4], %[t4]\n\t"
  "adcx %[lo], %[t4]\n\t"
  "mov %[t4], 32(%[r])\n\t"
  "adox %[t5], %[t5]\n\t"
  "adcx %[hi], %[t5]\n\t"
  "mov %[t5], 40(%[r])\n\t"
  "mov 24(%[a]), %%rdx\n\t"
  "mulx %%rdx, %[lo], %[hi]\n\t"
  "adox %[t6], %[t6]\n\t"
  "adcx %[lo], %[t6]\n\t"
  "mov %[t6], 48(%[r])\n\t"
  "adox %[z], %[hi]\n\t"
  "adcx %[z], %[hi]\n\t"
  "mov %[hi], 56(%[r])"
  : [ t1 ] "=&r" ( t1 ) , [ t2 ] "=&r" ( t2 ) , [ t3 ] "=&r" ( t3 ) ,
    [ t4 ] "=&r" ( t4 ) , [ t5 ] "=&r" ( t5 ) , [ t6 ] "=&r" ( t6 ) ,
    [ lo ] "=&r" ( lo ) , [ hi ] "=&r" ( hi ) , [ z ] "=&r" ( z )
  : [ a ] "r" ( a ) , [ r ] "r" ( r )
  : "rdx" , "cc" , "memory" );
 }

/// the digit s of r = 2 r + d: r_s doubled with the top bit of r_{s-1} on
/// the chain of OF, d_s added on that of CF
#define ADS_LSH1ADD_STEP( s )                                                \
 "mov " #s "*8(%[r]), %[x]\n\t"                                              \
 "adox %[x], %[x]\n\t"                                                       \
 "adcx " #s "*8(%[d]), %[x]\n\t"                                             \
 "mov %[x], " #s "*8(%[r])\n\t"

/// r = 2 r + d, n > 0 digits, n even, the result below b^n
inline void lsh1AddAsm( std::uint64_t * r , const std::uint64_t * d ,
                        std::size_t n )
{
 std::uint64_t x , c;
 __asm__ __volatile__(
  "xor %k[x], %k[x]\n\t"
  "mov %[n2], %[c]\n\t"
  "jrcxz 1f\n\t"
  ADS_LSH1ADD_STEP( 0 ) ADS_LSH1ADD_STEP( 1 )
  "lea 16(%[r]), %[r]\n\t"
  "lea 16(%[d]), %[d]\n"
  "1:\n\t"
  "mov %[n4], %[c]\n\t"
  "jmp 7f\n"
  "9:\n\t"
  ADS_LSH1ADD_STEP( 0 ) ADS_LSH1ADD_STEP( 1 )
  ADS_LSH1ADD_STEP( 2 ) ADS_LSH1ADD_STEP( 3 )
  "lea 32(%[r]), %[r]\n\t"
  "lea 32(%[d]), %[d]\n\t"
  "lea -1(%[c]), %[c]\n"
  "7:\n\t"
  "jrcxz 8f\n\t"
  "jmp 9b\n"
  "8:"
  : [ x ] "=&r" ( x ) , [ c ] "=&c" ( c ) , [ r ] "+&r" ( r ) ,
    [ d ] "+&r" ( d )
  : [ n2 ] "r" ( n & 2 ) , [ n4 ] "r" ( n / 4 )
  : "cc" , "memory" );
 }

// The loops on the digits of a, b and r, one step per digit, 1 and 2 and
// 4 steps first as the bits of n say, then blocks of 8, the chains of
// carries kept across them by MOV, LEA and JRCXZ; the steps use x and y by
// turns, and the 1-step group moves x to y for the steps that keep a
// digit from the step before.

/// the steps STEP over n digits, the counts in the registers n1, n2, n4
/// and n8, the pointers a, b and r moved on
#define ADS_ELEM( STEP , FIX1 )                                              \
 "mov %[n1], %[c]\n\t"                                                       \
 "jrcxz 1f\n\t"                                                              \
 STEP( 0 , x , y )                                                           \
 FIX1                                                                        \
 "lea 8(%[a]), %[a]\n\t"                                                     \
 "lea 8(%[b]), %[b]\n\t"                                                     \
 "lea 8(%[r]), %[r]\n"                                                       \
 "1:\n\t"                                                                    \
 "mov %[n2], %[c]\n\t"                                                       \
 "jrcxz 2f\n\t"                                                              \
 STEP( 0 , x , y ) STEP( 1 , y , x )                                         \
 "lea 16(%[a]), %[a]\n\t"                                                    \
 "lea 16(%[b]), %[b]\n\t"                                                    \
 "lea 16(%[r]), %[r]\n"                                                      \
 "2:\n\t"                                                                    \
 "mov %[n4], %[c]\n\t"                                                       \
 "jrcxz 3f\n\t"                                                              \
 STEP( 0 , x , y ) STEP( 1 , y , x ) STEP( 2 , x , y ) STEP( 3 , y , x )     \
 "lea 32(%[a]), %[a]\n\t"                                                    \
 "lea 32(%[b]), %[b]\n\t"                                                    \
 "lea 32(%[r]), %[r]\n"                                                      \
 "3:\n\t"                                                                    \
 "mov %[n8], %[c]\n\t"                                                       \
 "jmp 7f\n\t"                                                                \
 ".p2align 4\n"                                                              \
 "9:\n\t"                                                                    \
 STEP( 0 , x , y ) STEP( 1 , y , x ) STEP( 2 , x , y ) STEP( 3 , y , x )     \
 STEP( 4 , x , y ) STEP( 5 , y , x ) STEP( 6 , x , y ) STEP( 7 , y , x )     \
 "lea 64(%[a]), %[a]\n\t"                                                    \
 "lea 64(%[b]), %[b]\n\t"                                                    \
 "lea 64(%[r]), %[r]\n\t"                                                    \
 "lea -1(%[c]), %[c]\n"                                                      \
 "7:\n\t"                                                                    \
 "jrcxz 8f\n\t"                                                              \
 "jmp 9b\n"                                                                  \
 "8:\n\t"

/// the operands of the loops, with more inputs in MORE
#define ADS_ELEM_OPS( MORE )                                                 \
 : [ x ] "=&r" ( x ) , [ y ] "=&r" ( y ) , [ t ] "=&r" ( t ) ,              \
   [ c ] "=&c" ( c ) , [ a ] "+&r" ( a ) , [ b ] "+&r" ( b ) ,              \
   [ r ] "+&r" ( r ) , [ h ] "+&r" ( h )                                     \
 : [ n1 ] "r" ( n & 1 ) , [ n2 ] "r" ( n & 2 ) , [ n4 ] "r" ( n & 4 ) ,     \
   [ n8 ] "r" ( n / 8 ) MORE                                                 \
 : "cc" , "memory"

#define ADS_ELEM_VARS                                                        \
 std::uint64_t x , y , t , c , h = 0;

/// r = a + b and r = a - b
#define ADS_ADC_STEP( s , X , Y )                                            \
 "mov " #s "*8(%[a]), %[" #X "]\n\t"                                         \
 "adc " #s "*8(%[b]), %[" #X "]\n\t"                                         \
 "mov %[" #X "], " #s "*8(%[r])\n\t"

#define ADS_SBB_STEP( s , X , Y )                                            \
 "mov " #s "*8(%[a]), %[" #X "]\n\t"                                         \
 "sbb " #s "*8(%[b]), %[" #X "]\n\t"                                         \
 "mov %[" #X "], " #s "*8(%[r])\n\t"

/// r = a + b and r = a - b, n > 0 digits, the carry or the borrow
inline std::uint64_t addNAsm( std::uint64_t * r , const std::uint64_t * a ,
                              const std::uint64_t * b , std::size_t n )
{
 ADS_ELEM_VARS
 __asm__ __volatile__( "xor %k[x], %k[x]\n\t"
                       ADS_ELEM( ADS_ADC_STEP , "" )
                       "setc %b[h]"
                       ADS_ELEM_OPS( ) );
 return( h );
 }

inline std::uint64_t subNAsm( std::uint64_t * r , const std::uint64_t * a ,
                              const std::uint64_t * b , std::size_t n )
{
 ADS_ELEM_VARS
 __asm__ __volatile__( "xor %k[x], %k[x]\n\t"
                       ADS_ELEM( ADS_SBB_STEP , "" )
                       "setc %b[h]"
                       ADS_ELEM_OPS( ) );
 return( h );
 }

/// the steps s of the four phases of the sum of Karatsuba [see
/// karaSum()]: p walks on r, t on the product of the differences, L8 is
/// 8 l; the subtraction adds the complement of t, CF starting at 1
#define ADS_KA_STEP( s )                                                     \
 "mov " #s "*8(%[p],%[L8]), %[x]\n\t"                                        \
 "adcx " #s "*8(%[p],%[L8],2), %[x]\n\t"                                     \
 "mov %[x], " #s "*8(%[p],%[L8],2)\n\t"                                      \
 "adox " #s "*8(%[p]), %[x]\n\t"                                             \
 "mov %[x], " #s "*8(%[p],%[L8])\n\t"
#define ADS_KB1_ADD( s )                                                     \
 "mov " #s "*8(%[p]), %[x]\n\t"                                              \
 "adcx " #s "*8(%[t]), %[x]\n\t"                                             \
 "mov %[x], " #s "*8(%[p])\n\t"
#define ADS_KB1_SUB( s )                                                     \
 "mov " #s "*8(%[t]), %[y]\n\t"                                              \
 "not %[y]\n\t"                                                              \
 "adcx " #s "*8(%[p]), %[y]\n\t"                                             \
 "mov %[y], " #s "*8(%[p])\n\t"
#define ADS_KB2_ADD( s )                                                     \
 "mov " #s "*8(%[p]), %[x]\n\t"                                              \
 "adox " #s "*8(%[p],%[L8]), %[x]\n\t"                                       \
 "adcx " #s "*8(%[t]), %[x]\n\t"                                             \
 "mov %[x], " #s "*8(%[p])\n\t"
#define ADS_KB2_SUB( s )                                                     \
 "mov " #s "*8(%[t]), %[y]\n\t"                                              \
 "not %[y]\n\t"                                                              \
 "mov " #s "*8(%[p]), %[x]\n\t"                                              \
 "adox " #s "*8(%[p],%[L8]), %[x]\n\t"                                       \
 "adcx %[y], %[x]\n\t"                                                       \
 "mov %[x], " #s "*8(%[p])\n\t"
#define ADS_KB3_ADD( s )                                                     \
 "mov " #s "*8(%[p]), %[x]\n\t"                                              \
 "adox %[z], %[x]\n\t"                                                       \
 "adcx " #s "*8(%[t]), %[x]\n\t"                                             \
 "mov %[x], " #s "*8(%[p])\n\t"
#define ADS_KB3_SUB( s )                                                     \
 "mov " #s "*8(%[t]), %[y]\n\t"                                              \
 "not %[y]\n\t"                                                              \
 "mov " #s "*8(%[p]), %[x]\n\t"                                              \
 "adox %[z], %[x]\n\t"                                                       \
 "adcx %[y], %[x]\n\t"                                                       \
 "mov %[x], " #s "*8(%[p])\n\t"

/// a phase: the steps STEP over the digits counted by the masks at
/// mk[ M ] , ... , mk[ M + 3 ], p moved on, and t if T
#define ADS_KPHASE( STEP , M , T )                                           \
 "mov 8*" #M "(%[mk]), %[c]\n\t"                                             \
 "jrcxz 1f\n\t"                                                              \
 STEP( 0 )                                                                   \
 "lea 8(%[p]), %[p]\n\t"                                                     \
 T( "lea 8(%[t]), %[t]\n\t" )                                                \
 "1:\n\t"                                                                    \
 "mov 8*" #M "+8(%[mk]), %[c]\n\t"                                           \
 "jrcxz 2f\n\t"                                                              \
 STEP( 0 ) STEP( 1 )                                                         \
 "lea 16(%[p]), %[p]\n\t"                                                    \
 T( "lea 16(%[t]), %[t]\n\t" )                                               \
 "2:\n\t"                                                                    \
 "mov 8*" #M "+16(%[mk]), %[c]\n\t"                                          \
 "jrcxz 3f\n\t"                                                              \
 STEP( 0 ) STEP( 1 ) STEP( 2 ) STEP( 3 )                                     \
 "lea 32(%[p]), %[p]\n\t"                                                    \
 T( "lea 32(%[t]), %[t]\n\t" )                                               \
 "3:\n\t"                                                                    \
 "mov 8*" #M "+24(%[mk]), %[c]\n\t"                                          \
 "jmp 7f\n"                                                                  \
 "9:\n\t"                                                                    \
 STEP( 0 ) STEP( 1 ) STEP( 2 ) STEP( 3 )                                     \
 STEP( 4 ) STEP( 5 ) STEP( 6 ) STEP( 7 )                                     \
 "lea 64(%[p]), %[p]\n\t"                                                    \
 T( "lea 64(%[t]), %[t]\n\t" )                                               \
 "lea -1(%[c]), %[c]\n"                                                      \
 "7:\n\t"                                                                    \
 "jrcxz 8f\n\t"                                                              \
 "jmp 9b\n"                                                                  \
 "8:\n\t"

#define ADS_KT_YES( x ) x
#define ADS_KT_NO( x )

/// the sum of Karatsuba, SUB for the subtraction of t
#define ADS_KARA_SUM( B1 , B2 , B3 , START )                                 \
 __asm__ __volatile__(                                                       \
  "xor %k[z], %k[z]\n\t"                                                     \
  ADS_KPHASE( ADS_KA_STEP , 0 , ADS_KT_NO )                                  \
  "setc 128(%[mk])\n\t"                                                      \
  "seto 129(%[mk])\n\t"                                                      \
  "movzbl 129(%[mk]), %k[y]\n\t"                                             \
  "xor %k[z], %k[z]\n\t"                                                     \
  "adox 136(%[mk]), %[y]\n\t"                                                \
  START                                                                      \
  ADS_KPHASE( B1 , 4 , ADS_KT_YES )                                          \
  ADS_KPHASE( B2 , 8 , ADS_KT_YES )                                          \
  ADS_KPHASE( B3 , 12 , ADS_KT_YES )                                         \
  "setc 130(%[mk])\n\t"                                                      \
  "seto 131(%[mk])"                                                          \
  : [ x ] "=&r" ( x ) , [ y ] "=&r" ( y ) , [ z ] "=&r" ( z ) ,             \
    [ c ] "=&c" ( c ) , [ p ] "+&r" ( p ) , [ t ] "+&r" ( t )               \
  : [ L8 ] "r" ( 8 * l ) , [ mk ] "r" ( mk )                                 \
  : "cc" , "memory" )

/// r[ l , 2n ) += z0 + z2 -+ t, z0 = r[ 0 , 2 l ), z2 = r[ 2 l , 2 n ),
/// of 2 h digits, l - 1 <= h <= l, t of 2 l digits, subtracted if sub:
/// with zi = zi0 + zi1 b^l, H = z01 + z20 goes to r[ l , 2 l ) with z00
/// and to r[ 2 l , 3 l ) with z21 (phase A, on the chains of CF and OF),
/// then t goes to r[ l , 3 l ) (phases B, on CF) with z21 (on OF); the
/// carry of H, which goes to b^{2 l} and b^{3 l}, in cH, and the carry at
/// b^{3 l}, -1 to 3, returned
template< bool sub >
inline std::int64_t karaSumAsm( std::uint64_t * r , const std::uint64_t * t ,
                                std::size_t l , std::size_t h ,
                                std::uint64_t & cH )
{
 const std::size_t m = 2 * h - l;           // the digits of z21
 alignas( 8 ) std::uint64_t mk[ 18 ] = {
  l & 1 , l & 2 , l & 4 , l / 8 , l & 1 , l & 2 , l & 4 , l / 8 ,
  m & 1 , m & 2 , m & 4 , m / 8 , ( l - m ) & 1 , ( l - m ) & 2 ,
  ( l - m ) & 4 , ( l - m ) / 8 , 0 , ~std::uint64_t( 0 ) };
 std::uint64_t x , y , z , c , * p = r;
 if constexpr( sub )
  ADS_KARA_SUM( ADS_KB1_SUB , ADS_KB2_SUB , ADS_KB3_SUB , "stc\n\t" );
 else ADS_KARA_SUM( ADS_KB1_ADD , ADS_KB2_ADD , ADS_KB3_ADD , "" );
 const auto * f = reinterpret_cast< const unsigned char * >( mk + 16 );
 cH = f[ 0 ];
 return( std::int64_t( f[ 0 ] ) + std::int64_t( f[ 3 ] ) +
         std::int64_t( f[ 2 ] ) - std::int64_t( sub ) );
 }

/// r = a + 2 b and r = a - 2 b: b doubled with the top bit of the digit
/// before on the chain of OF, added to a or its complement subtracted on
/// that of CF, which starts at 1 for the difference
#define ADS_ADDLSH1_STEP( s , X , Y )                                        \
 "mov " #s "*8(%[b]), %[" #X "]\n\t"                                         \
 "adox %[" #X "], %[" #X "]\n\t"                                             \
 "adcx " #s "*8(%[a]), %[" #X "]\n\t"                                        \
 "mov %[" #X "], " #s "*8(%[r])\n\t"

#define ADS_SUBLSH1_STEP( s , X , Y )                                        \
 "mov " #s "*8(%[b]), %[" #X "]\n\t"                                         \
 "adox %[" #X "], %[" #X "]\n\t"                                             \
 "not %[" #X "]\n\t"                                                         \
 "adcx " #s "*8(%[a]), %[" #X "]\n\t"                                        \
 "mov %[" #X "], " #s "*8(%[r])\n\t"

/// r = a + 2 b, n > 0 digits, the carry, 0 to 2
inline std::uint64_t addLsh1Asm( std::uint64_t * r , const std::uint64_t * a ,
                                 const std::uint64_t * b , std::size_t n )
{
 ADS_ELEM_VARS
 __asm__ __volatile__( "xor %k[x], %k[x]\n\t"
                       ADS_ELEM( ADS_ADDLSH1_STEP , "" )
                       "mov $0, %k[h]\n\t"
                       "adcx %[h], %[h]\n\t"
                       "adox %[c], %[h]"
                       ADS_ELEM_OPS( ) );
 return( h );
 }

/// r = a - 2 b, n > 0 digits, the borrow, 0 to 2
inline std::uint64_t subLsh1Asm( std::uint64_t * r , const std::uint64_t * a ,
                                 const std::uint64_t * b , std::size_t n )
{
 ADS_ELEM_VARS
 __asm__ __volatile__( "xor %k[x], %k[x]\n\t"
                       "stc\n\t"
                       ADS_ELEM( ADS_SUBLSH1_STEP , "" )
                       "mov $1, %k[h]\n\t"
                       "mov $0, %k[t]\n\t"
                       "adox %[t], %[h]\n\t"
                       "sbb %[t], %[h]"
                       ADS_ELEM_OPS( ) );
 return( h );
 }

/// r = ( a + b ) / 2 and r = ( a - b ) / 2, the bit shifted in at the top
/// the carry or the borrow (the sign bit of a negative difference): the
/// digit s of the sum is made on the chain of CF, and the digit s - 1 of
/// the result from it and the one before it by SHRX, SHLX and LEA, which
/// leave the flags alone; X holds the digit s of the sum, Y the one before
#define ADS_RSH1_STEP( s , X , Y , OP )                                      \
 "mov " #s "*8(%[a]), %[" #X "]\n\t"                                         \
 #OP " " #s "*8(%[b]), %[" #X "]\n\t"                                        \
 "shrx %[one], %[" #Y "], %[t]\n\t"                                          \
 "shlx %[s63], %[" #X "], %[h]\n\t"                                          \
 "lea (%[t],%[h]), %[t]\n\t"                                                 \
 "mov %[t], " #s "*8-8(%[r])\n\t"

#define ADS_RSH1ADD_STEP( s , X , Y ) ADS_RSH1_STEP( s , X , Y , adc )
#define ADS_RSH1SUB_STEP( s , X , Y ) ADS_RSH1_STEP( s , X , Y , sbb )

/// the first digit of the sum, in y, the pointers one digit on, and n - 1
#define ADS_RSH1_FIRST( OP )                                                 \
 "mov (%[a]), %[y]\n\t"                                                      \
 #OP " (%[b]), %[y]\n\t"                                                     \
 "lea 8(%[a]), %[a]\n\t"                                                     \
 "lea 8(%[b]), %[b]\n\t"                                                     \
 "lea 8(%[r]), %[r]\n\t"

/// the last digit: the top of the sum and the carry or the borrow
#define ADS_RSH1_LAST                                                        \
 "setc %b[h]\n\t"                                                            \
 "movzx %b[h], %k[h]\n\t"                                                    \
 "shl $63, %[h]\n\t"                                                         \
 "shr $1, %[y]\n\t"                                                          \
 "or %[h], %[y]\n\t"                                                         \
 "mov %[y], -8(%[r])"

#define ADS_RSH1_FN( name , OP , STEP )                                      \
inline void name( std::uint64_t * r , const std::uint64_t * a ,              \
                  const std::uint64_t * b , std::size_t n0 )                 \
{                                                                            \
 ADS_ELEM_VARS                                                               \
 const std::size_t n = n0 - 1;                                               \
 __asm__ __volatile__( "xor %k[x], %k[x]\n\t"                                \
                       ADS_RSH1_FIRST( OP )                                  \
                       ADS_ELEM( STEP , "mov %[x], %[y]\n\t" )               \
                       ADS_RSH1_LAST                                         \
                       ADS_ELEM_OPS( ADS_RSH1_MORE ) );                      \
 }

#define ADS_RSH1_MORE , [ one ] "r" ( std::uint64_t( 1 ) ) ,                 \
                        [ s63 ] "r" ( std::uint64_t( 63 ) )

/// r = ( a + b ) / 2 and r = ( a - b ) / 2, n > 0 digits, the carry or the
/// borrow shifted in at the top
ADS_RSH1_FN( rsh1AddAsm , adc , ADS_RSH1ADD_STEP )
ADS_RSH1_FN( rsh1SubAsm , sbb , ADS_RSH1SUB_STEP )

/// r = a / 3 for a multiple of 3 (Hensel): with d = ( b - 1 ) / 3,
/// -d is 1 / 3 modulo b and r = -a d ( 1 + b + b^2 + ... ) modulo b^n, the
/// sums of the products a_i d made by the running difference h, on the
/// chain of SUB and SBB, the products independent of it
#define ADS_DIV3_STEP( s , X , Y )                                           \
 "mulx " #s "*8(%[a]), %[" #X "], %[" #Y "]\n\t"                             \
 "sub %[" #X "], %[h]\n\t"                                                   \
 "mov %[h], " #s "*8(%[r])\n\t"                                              \
 "sbb %[" #Y "], %[h]\n\t"

#define ADS_DIV3_MORE , [ d ] "d" ( 0x5555555555555555ULL )

inline void divExact3Asm( std::uint64_t * r , const std::uint64_t * a ,
                          std::size_t n )
{
 ADS_ELEM_VARS
 const std::uint64_t * b = a;
 __asm__ __volatile__( ADS_ELEM( ADS_DIV3_STEP , "" )
                       ADS_ELEM_OPS( ADS_DIV3_MORE ) );
 }

#endif

/// the reciprocal of d1 b + d0, d1 with the top bit set,
/// floor( ( b^3 - 1 ) / ( d1 b + d0 ) ) - b, from that of d1 corrected
/// for d0 (Moeller and Granlund, Algorithm 6)
inline std::uint64_t reciprocal2( std::uint64_t d1 , std::uint64_t d0 )
{
 std::uint64_t v = reciprocal( d1 ) , p = d1 * v + d0;
 if( p < d0 ) {                             // the sum wrapped
  --v;
  if( p >= d1 ) {
   --v;
   p -= d1;
   }
  p -= d1;
  }
 std::uint64_t t1;
 const std::uint64_t t0 = mulWide( d0 , v , t1 );
 p += t1;
 if( p < t1 ) {
  --v;
  if( ( p > d1 ) || ( ( p == d1 ) && ( t0 >= d0 ) ) ) --v;
  }
 return( v );
 }

/// the quotient of u2 b^2 + u1 b + u0 by d1 b + d0, u2 b + u1 below it,
/// by its reciprocal v (Moeller and Granlund, Algorithm 5), the remainder
/// in r1 b + r0
inline std::uint64_t div3by2( std::uint64_t u2 , std::uint64_t u1 ,
                              std::uint64_t u0 , std::uint64_t d1 ,
                              std::uint64_t d0 , std::uint64_t v ,
                              std::uint64_t & r1 , std::uint64_t & r0 )
{
 std::uint64_t q1;
 std::uint64_t q0 = mulWide( v , u2 , q1 );   // ( q1 , q0 ) = v u2 + u2 b + u1
 q0 += u1;
 q1 += u2 + ( q0 < u1 );
 r1 = u1 - q1 * d1;
 std::uint64_t t1 , c = 0;
 const std::uint64_t t0 = mulWide( d0 , q1 , t1 );
 r0 = subBorrow( u0 , d0 , c );             // ( r1 , r0 ) -= d + d0 q1
 r1 = subBorrow( r1 , d1 , c );
 c = 0;
 r0 = subBorrow( r0 , t0 , c );
 r1 = subBorrow( r1 , t1 , c );
 ++q1;
 if( r1 >= q0 ) {                           // one too large
  --q1;
  c = 0;
  r0 = addCarry( r0 , d0 , c );
  r1 = addCarry( r1 , d1 , c );
  }
 if( ( r1 > d1 ) || ( ( r1 == d1 ) && ( r0 >= d0 ) ) ) {   // rarely
  ++q1;
  c = 0;
  r0 = subBorrow( r0 , d0 , c );
  r1 = subBorrow( r1 , d1 , c );
  }
 return( q1 );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE FIGURES ---------------------------------*/
/*--------------------------------------------------------------------------*/

/// the value of the digit c in the bases up to 36, -1 if none
inline int digitValue( char c )
{
 if( ( c >= '0' ) && ( c <= '9' ) ) return( c - '0' );
 if( ( c >= 'a' ) && ( c <= 'z' ) ) return( c - 'a' + 10 );
 if( ( c >= 'A' ) && ( c <= 'Z' ) ) return( c - 'A' + 10 );
 return( -1 );
 }

/// the largest power of the base in a digit of 64 bits, and its exponent
inline std::pair< std::uint64_t , int > bigBase( int base )
{
 const std::uint64_t b = std::uint64_t( base );
 std::uint64_t p = b;
 int k = 1;
 while( p <= ~std::uint64_t( 0 ) / b ) {
  p *= b;
  ++k;
  }
 return( std::pair( p , k ) );
 }

/// the figures of the bases up to 36
inline constexpr const char * kFigures =
                                       "0123456789abcdefghijklmnopqrstuvwxyz";

/// the pairs of decimal figures "00" to "99"
inline constexpr auto kPairs = []() {
 std::array< char , 200 > t{};
 for( int i = 0 ; i < 100 ; ++i ) {
  t[ 2 * i ] = char( '0' + i / 10 );
  t[ 2 * i + 1 ] = char( '0' + i % 10 );
  }
 return( t );
 }();

/// the 8 decimal figures of y < 10^8 at p, by pairs: the halves of 4
/// figures and their pairs, all independent of each other
inline void put8( char * p , std::uint32_t y )
{
 const std::uint32_t a = y / 10000 , b = y % 10000 , a1 = a / 100 ,
                     a0 = a % 100 , b1 = b / 100 , b0 = b % 100;
 std::copy( kPairs.data() + 2 * a1 , kPairs.data() + 2 * a1 + 2 , p );
 std::copy( kPairs.data() + 2 * a0 , kPairs.data() + 2 * a0 + 2 , p + 2 );
 std::copy( kPairs.data() + 2 * b1 , kPairs.data() + 2 * b1 + 2 , p + 4 );
 std::copy( kPairs.data() + 2 * b0 , kPairs.data() + 2 * b0 + 2 , p + 6 );
 }

/// the k figures of r in the base, the last one at end[ -1 ]; the base 10
/// as 3 + 8 + 8 figures, r < 10^{19}, by put8()
inline void putFigures( char * end , std::uint64_t r , int k , int base )
{
 if( base == 10 ) {
  char t[ 19 ];
  const std::uint64_t x2 = r / 10000000000000000ULL ,
                      x = r % 10000000000000000ULL;
  const std::uint32_t x1 = std::uint32_t( x / 100000000 ) ,
                      x0 = std::uint32_t( x % 100000000 );
  t[ 0 ] = char( '0' + x2 / 100 );
  t[ 1 ] = kPairs[ 2 * ( x2 % 100 ) ];
  t[ 2 ] = kPairs[ 2 * ( x2 % 100 ) + 1 ];
  put8( t + 3 , x1 );
  put8( t + 11 , x0 );
  std::copy( t + 19 - k , t + 19 , end - k );
  }
 else
  for( int t = 0 ; t < k ; ++t , r /= std::uint64_t( base ) )
   *--end = kFigures[ r % std::uint64_t( base ) ];
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE NTT -----------------------------------*/
/*--------------------------------------------------------------------------*/

/// the arithmetic modulo a prime p < 2^31 on lanes of 32 bits, the same
/// for one lane and for the 8 and 16 lanes of AVX2 and AVX-512: the sum
/// and the difference, the product by a constant w with its quotient
/// wq = floor( w 2^32 / p ) (Shoup), and the product a b 2^{-32} of
/// Montgomery, from the high halves of a b and of m p with
/// m = a b / p modulo 2^32, whose low halves are equal; every result in
/// [ 0 , p )
struct Lanes1 {
 using V = std::uint32_t;
 static constexpr std::size_t W = 1;

 static V load( const std::uint32_t * x ) { return( *x ); }
 static void store( std::uint32_t * x , V v ) { *x = v; }
 static V set( std::uint32_t x ) { return( x ); }
 static V add( V a , V b , V p ) {
  const V s = a + b;
  return( std::min( s , s - p ) );
  }
 static V sub( V a , V b , V p ) {
  const V d = a - b;
  return( std::min( d , d + p ) );
  }
 static V mulhi( V a , V b ) {
  return( V( ( std::uint64_t( a ) * b ) >> 32 ) );
  }
 static V mulShoup( V x , V w , V wq , V p ) {
  const V r = x * w - mulhi( x , wq ) * p;
  return( std::min( r , r - p ) );
  }
 static V mulMont( V a , V b , V p , V pinv ) {
  return( sub( mulhi( a , b ) , mulhi( a * b * pinv , p ) , p ) );
  }
 };

#if ADS_BIGINT_SIMD >= 256
/// the lanes of AVX2, and of AVX-512 below, with split( a , b , h ) that
/// gathers in x the items of a and b at the first place of the pairs at
/// distance h < W and in y their partners, and join() that undoes it
struct Lanes8 {
 using V = __m256i;
 static constexpr std::size_t W = 8;

 static V load( const void * x ) {
  return( _mm256_loadu_si256( static_cast< const V * >( x ) ) );
  }
 static void store( void * x , V v ) {
  _mm256_storeu_si256( static_cast< V * >( x ) , v );
  }
 static V set( std::uint32_t x ) { return( _mm256_set1_epi32( int( x ) ) ); }
 static V add( V a , V b , V p ) {
  const V s = _mm256_add_epi32( a , b );
  return( _mm256_min_epu32( s , _mm256_sub_epi32( s , p ) ) );
  }
 static V sub( V a , V b , V p ) {
  const V d = _mm256_sub_epi32( a , b );
  return( _mm256_min_epu32( d , _mm256_add_epi32( d , p ) ) );
  }
 static V mulhi( V a , V b ) {
  const V e = _mm256_srli_epi64( _mm256_mul_epu32( a , b ) , 32 ) ,
          o = _mm256_mul_epu32( _mm256_srli_epi64( a , 32 ) ,
                                _mm256_srli_epi64( b , 32 ) );
  return( _mm256_blend_epi32( e , o , 0xAA ) );
  }
 static V mulShoup( V x , V w , V wq , V p ) {
  const V r = _mm256_sub_epi32( _mm256_mullo_epi32( x , w ) ,
                                _mm256_mullo_epi32( mulhi( x , wq ) , p ) );
  return( _mm256_min_epu32( r , _mm256_sub_epi32( r , p ) ) );
  }
 static V mulMont( V a , V b , V p , V pinv ) {
  const V m = _mm256_mullo_epi32( _mm256_mullo_epi32( a , b ) , pinv );
  return( sub( mulhi( a , b ) , mulhi( m , p ) , p ) );
  }
 static void split( V a , V b , std::size_t h , V & x , V & y ) {
  if( h == 4 ) {
   x = _mm256_permute2x128_si256( a , b , 0x20 );
   y = _mm256_permute2x128_si256( a , b , 0x31 );
   }
  else if( h == 2 ) {
   x = _mm256_unpacklo_epi64( a , b );
   y = _mm256_unpackhi_epi64( a , b );
   }
  else {
   x = _mm256_castps_si256( _mm256_shuffle_ps( _mm256_castsi256_ps( a ) ,
                                               _mm256_castsi256_ps( b ) ,
                                               0x88 ) );
   y = _mm256_castps_si256( _mm256_shuffle_ps( _mm256_castsi256_ps( a ) ,
                                               _mm256_castsi256_ps( b ) ,
                                               0xDD ) );
   }
  }
 static void join( V x , V y , std::size_t h , V & a , V & b ) {
  if( h == 4 ) {
   a = _mm256_permute2x128_si256( x , y , 0x20 );
   b = _mm256_permute2x128_si256( x , y , 0x31 );
   }
  else if( h == 2 ) {
   a = _mm256_unpacklo_epi64( x , y );
   b = _mm256_unpackhi_epi64( x , y );
   }
  else {
   a = _mm256_unpacklo_epi32( x , y );
   b = _mm256_unpackhi_epi32( x , y );
   }
  }
 };
#endif

/// the number theoretic transform modulo a prime P < 2^31 of up to 2^26
/// terms, the roots of unity from G, a quadratic nonresidue, on the lanes
/// of O
template< std::uint32_t P , std::uint32_t G >
struct NTT {

 /// 1 / P modulo 2^32, by Newton from the 3 bits right in P
 static constexpr std::uint32_t inv( void ) {
  std::uint32_t x = P;
  for( int i = 0 ; i < 4 ; ++i ) x *= 2 - P * x;
  return( x );
  }

 static constexpr std::uint32_t kInv = inv();
 static constexpr std::uint32_t kQ1 = std::uint32_t(   ///< 2^32 / P
                                       ( std::uint64_t( 1 ) << 32 ) / P );

 static constexpr std::uint32_t mul( std::uint32_t a , std::uint32_t b ) {
  return( std::uint32_t( std::uint64_t( a ) * b % P ) );
  }

 /// x^e modulo P
 static constexpr std::uint32_t power( std::uint32_t x , std::uint64_t e ) {
  std::uint32_t y = 1;
  for( ; e ; e >>= 1 ) {
   if( e & 1 ) y = mul( y , x );
   x = mul( x , x );
   }
  return( y );
  }

 /// floor( w 2^32 / P ), the quotient of Shoup of w < P
 static constexpr std::uint32_t shoup( std::uint32_t w ) {
  return( std::uint32_t( ( std::uint64_t( w ) << 32 ) / P ) );
  }

 /// the powers of the roots of unity (inverse if inverse), the round of
 /// length len at len / 2 , ... , len - 1, for the transforms up to n
 /// terms, and their quotients of Shoup in q; exact, kept per thread and
 /// grown
 static const std::uint32_t * roots( std::size_t n , bool inverse ,
                                     const std::uint32_t * & q ) {
  thread_local std::vector< std::uint32_t > T[ 2 ] , Q[ 2 ];
  auto & t = T[ inverse ];
  if( t.size() < n ) {
   t.assign( n , 0 );
   for( std::size_t len = 2 ; len <= n ; len *= 2 ) {
    std::uint32_t w = power( G , ( P - 1 ) / len );
    if( inverse ) w = power( w , P - 2 );
    std::uint32_t x = 1;
    for( std::size_t k = 0 ; k < len / 2 ; ++k ) {
     t[ len / 2 + k ] = x;
     x = mul( x , w );
     }
    }
   Q[ inverse ].resize( n );
   for( std::size_t i = 0 ; i < n ; ++i ) Q[ inverse ][ i ] = shoup( t[ i ] );
   }
  q = Q[ inverse ].data();
  return( t.data() );
  }

 /// the rounds of h < W, on pairs of vectors whose items are split by
 /// distance h, the roots, the same for every pair, split with them
 template< class O >
 static void twiddles( const std::uint32_t * w , const std::uint32_t * wq ,
                       std::size_t h , typename O::V & tw ,
                       typename O::V & tq ) {
  std::uint32_t s[ 4 * O::W ];
  for( std::size_t j = 0 ; j < 2 * O::W ; ++j ) {
   const std::size_t k = j % ( 2 * h );
   s[ j ] = k < h ? w[ k ] : 0;
   s[ 2 * O::W + j ] = k < h ? wq[ k ] : 0;
   }
  typename O::V y;
  O::split( O::load( s ) , O::load( s + O::W ) , h , tw , y );
  O::split( O::load( s + 2 * O::W ) , O::load( s + 3 * O::W ) , h , tq , y );
  }

 /// the transform of a, n >= 2 W a power of 2, from the natural order to
 /// that of the reversed bits (Gentleman and Sande)
 template< class O >
 static void forward( std::uint32_t * a , std::size_t n ,
                      const std::uint32_t * T , const std::uint32_t * Q ) {
  using V = typename O::V;
  const V p = O::set( P );
  for( std::size_t h = n / 2 ; h >= 1 ; h /= 2 ) {
   const std::uint32_t * w = T + h , * wq = Q + h;
   if( h >= O::W )
    for( std::size_t i = 0 ; i < n ; i += 2 * h )
     for( std::size_t k = 0 ; k < h ; k += O::W ) {
      const V x = O::load( a + i + k ) , y = O::load( a + i + h + k );
      O::store( a + i + k , O::add( x , y , p ) );
      O::store( a + i + h + k , O::mulShoup( O::sub( x , y , p ) ,
                                             O::load( w + k ) ,
                                             O::load( wq + k ) , p ) );
      }
   else {
    if constexpr( O::W > 1 ) {
     V tw , tq;
     twiddles< O >( w , wq , h , tw , tq );
     for( std::size_t i = 0 ; i < n ; i += 2 * O::W ) {
      V x , y;
      O::split( O::load( a + i ) , O::load( a + i + O::W ) , h , x , y );
      const V u = O::add( x , y , p ) ,
              v = O::mulShoup( O::sub( x , y , p ) , tw , tq , p );
      O::join( u , v , h , x , y );
      O::store( a + i , x );
      O::store( a + i + O::W , y );
      }
     }
    }
   }
  }

 /// the inverse transform times n, from the order of the reversed bits to
 /// the natural one (Cooley and Tukey)
 template< class O >
 static void backward( std::uint32_t * a , std::size_t n ,
                       const std::uint32_t * T , const std::uint32_t * Q ) {
  using V = typename O::V;
  const V p = O::set( P );
  for( std::size_t h = 1 ; h < n ; h *= 2 ) {
   const std::uint32_t * w = T + h , * wq = Q + h;
   if( h >= O::W )
    for( std::size_t i = 0 ; i < n ; i += 2 * h )
     for( std::size_t k = 0 ; k < h ; k += O::W ) {
      const V x = O::load( a + i + k ) ,
              y = O::mulShoup( O::load( a + i + h + k ) , O::load( w + k ) ,
                               O::load( wq + k ) , p );
      O::store( a + i + k , O::add( x , y , p ) );
      O::store( a + i + h + k , O::sub( x , y , p ) );
      }
   else {
    if constexpr( O::W > 1 ) {
     V tw , tq;
     twiddles< O >( w , wq , h , tw , tq );
     for( std::size_t i = 0 ; i < n ; i += 2 * O::W ) {
      V x , y;
      O::split( O::load( a + i ) , O::load( a + i + O::W ) , h , x , y );
      y = O::mulShoup( y , tw , tq , p );
      O::join( O::add( x , y , p ) , O::sub( x , y , p ) , h , x , y );
      O::store( a + i , x );
      O::store( a + i + O::W , y );
      }
     }
    }
   }
  }

 /// f = the pieces of 32 bits of x, n4 of them, modulo P, and zeros up to
 /// n
 template< class O >
 static void load( std::uint32_t * f , const std::uint64_t * x ,
                   std::size_t n4 , std::size_t n ) {
  using V = typename O::V;
  const V p = O::set( P ) , one = O::set( 1 ) , q = O::set( kQ1 );
  std::size_t i = 0;
  if constexpr( O::W > 1 )
   for( ; i + O::W <= n4 ; i += O::W )
    O::store( f + i , O::mulShoup( O::load( reinterpret_cast< const char * >(
                                             x ) + 4 * i ) , one , q , p ) );
  for( ; i < n4 ; ++i )
   f[ i ] = Lanes1::mulShoup( std::uint32_t( x[ i / 2 ] >>
                                             ( 32 * ( i % 2 ) ) ) ,
                              1 , kQ1 , P );
  std::fill( f + n4 , f + n , 0 );
  }

 /// fa = the cyclic convolution modulo P of the pieces of 32 bits of a and
 /// b, na and nb limbs, n terms, with fb as scratch; a square if fb is null
 template< class O >
 static void convolve( std::uint32_t * fa , std::uint32_t * fb ,
                       const std::uint64_t * a , std::size_t na ,
                       const std::uint64_t * b , std::size_t nb ,
                       std::size_t n ) {
  using V = typename O::V;
  const std::uint32_t * FQ , * BQ , * F = roots( n , false , FQ ) ,
                      * B = roots( n , true , BQ );
  load< O >( fa , a , 2 * na , n );
  forward< O >( fa , n , F , FQ );
  if( fb ) {
   load< O >( fb , b , 2 * nb , n );
   forward< O >( fb , n , F , FQ );
   }
  else fb = fa;
  // the product of Montgomery, and 2^32 / n against its 2^{-32} and the n
  // of the inverse transform
  const std::uint32_t c = mul( std::uint32_t( ( std::uint64_t( 1 ) << 32 ) %
                                              P ) ,
                               P - ( P - 1 ) / std::uint32_t( n ) );
  const V p = O::set( P ) , pinv = O::set( kInv ) , vc = O::set( c ) ,
          vq = O::set( shoup( c ) );
  for( std::size_t i = 0 ; i < n ; i += O::W )
   O::store( fa + i , O::mulShoup( O::mulMont( O::load( fa + i ) ,
                                               O::load( fb + i ) , p , pinv ) ,
                                   vc , vq , p ) );
  backward< O >( fa , n , B , BQ );
  }

 };  // end( struct NTT )

#if ADS_BIGINT_FNTT
/// the shortest length of the transforms on doubles from n terms, a power
/// of 2 or 3 times one
inline std::size_t length( std::size_t n )
{
 const std::size_t p = std::bit_ceil( n ) , t = 3 * ( p / 4 );
 return( ( t >= n ) && ( t >= 24 ) ? t : p );
 }

/// the arithmetic modulo a prime p < 2^50 on lanes of doubles with the
/// fused product and sum, of AVX2 and of AVX-512: the residues are exact
/// integers, kept in [ -p , p ]; the product a b, |a b| <= p^2, is
/// h + l with h = a b rounded and l = fma( a , b , -h ) exact, and the
/// quotient q = round( h / p ) by the product with 1 / p and the
/// addition of 1.5 2^52, which rounds to an integer; q is within 0.4 of
/// a b / p, so a b - q p = ( h - q p ) + l, both exact, is within
/// 0.9 p of 0
struct FLanes4 {
 using V = __m256d;
 static constexpr std::size_t W = 4;

 static V load( const double * x ) { return( _mm256_loadu_pd( x ) ); }
 static void store( double * x , V v ) { _mm256_storeu_pd( x , v ); }
 static V set( double x ) { return( _mm256_set1_pd( x ) ); }
 static V add( V a , V b ) { return( _mm256_add_pd( a , b ) ); }
 static V sub( V a , V b ) { return( _mm256_sub_pd( a , b ) ); }
 static V mul( V a , V b ) { return( _mm256_mul_pd( a , b ) ); }
 static V madd( V a , V b , V c ) { return( _mm256_fmadd_pd( a , b , c ) ); }
 static V msub( V a , V b , V c ) { return( _mm256_fmsub_pd( a , b , c ) ); }
 static V nmadd( V a , V b , V c ) {
  return( _mm256_fnmadd_pd( a , b , c ) );
  }
 /// x + p if x < 0, else x
 static V pos( V x , V p ) {
  return( _mm256_add_pd( x , _mm256_and_pd( _mm256_cmp_pd(
                                     x , _mm256_setzero_pd() , _CMP_LT_OQ ) ,
                                     p ) ) );
  }
 static void split( V a , V b , std::size_t h , V & x , V & y ) {
  if( h == 2 ) {
   x = _mm256_permute2f128_pd( a , b , 0x20 );
   y = _mm256_permute2f128_pd( a , b , 0x31 );
   }
  else {
   x = _mm256_unpacklo_pd( a , b );
   y = _mm256_unpackhi_pd( a , b );
   }
  }
 static void join( V x , V y , std::size_t h , V & a , V & b ) {
  split( x , y , h , a , b );
  }
 };

/// the number theoretic transform modulo a prime P < 2^50 with
/// 3 2^30 | P - 1, of up to 2^22 terms, a power of 2 or 3 times one, on
/// the lanes of doubles of O, the roots of unity from G, a generator
template< std::uint64_t P , std::uint64_t G >
struct FNTT {

 static constexpr double kP = double( P ) , kPinv = 1.0 / double( P ) ,
                         kM = 6755399441055744.0;   ///< 1.5 2^52

 static constexpr std::uint64_t mul( std::uint64_t a , std::uint64_t b ) {
  return( std::uint64_t( UInt128( a ) * b % P ) );
  }

 static constexpr std::uint64_t power( std::uint64_t x , std::uint64_t e ) {
  std::uint64_t y = 1;
  for( ; e ; e >>= 1 ) {
   if( e & 1 ) y = mul( y , x );
   x = mul( x , x );
   }
  return( y );
  }

 /// x in ( -P / 2 , P / 2 ]
 static constexpr double centered( std::uint64_t x ) {
  return( x > P / 2 ? -double( P - x ) : double( x ) );
  }

 /// x - q P, q = round( x / P ), for |x| < 2^52
 template< class O >
 static typename O::V reduce( typename O::V x ) {
  const typename O::V q = O::sub( O::madd( x , O::set( kPinv ) ,
                                           O::set( kM ) ) , O::set( kM ) );
  return( O::nmadd( q , O::set( kP ) , x ) );
  }

 /// a b - q P, q = round( a b / P ), for |a b| <= P^2
 template< class O >
 static typename O::V mulmod( typename O::V a , typename O::V b ) {
  const typename O::V h = O::mul( a , b ) , l = O::msub( a , b , h ) ,
                      q = O::sub( O::madd( h , O::set( kPinv ) ,
                                           O::set( kM ) ) , O::set( kM ) );
  return( O::add( O::nmadd( q , O::set( kP ) , h ) , l ) );
  }

 /// the powers of the roots of unity (inverse if inverse), centered, the
 /// round of length len at len / 2 , ... , len - 1, for the transforms up
 /// to n terms; exact, kept per thread and grown
 static const double * roots( std::size_t n , bool inverse ) {
  thread_local std::vector< double > T[ 2 ];
  auto & t = T[ inverse ];
  if( t.size() < n ) {                      // the round of n, and the others
   t.assign( n , 0 );                       // every other root of the next
   powers( t.data() + n / 2 , n / 2 , root( n , inverse ) );
   for( std::size_t len = n / 2 ; len >= 2 ; len /= 2 )
    for( std::size_t k = 0 ; k < len / 2 ; ++k )
     t[ len / 2 + k ] = t[ len + 2 * k ];
   }
  return( t.data() );
  }

 /// a root of unity of order len (its inverse if inverse)
 static std::uint64_t root( std::size_t len , bool inverse ) {
  const std::uint64_t w = power( G , ( P - 1 ) / len );
  return( inverse ? power( w , P - 2 ) : w );
  }

 /// x y mod P by the doubles, as mulmod(), for x , y < P
 static std::uint64_t mulFast( std::uint64_t x , std::uint64_t y ) {
  const double a = double( x ) , b = double( y ) , h = a * b ,
               l = std::fma( a , b , -h ) , q = std::nearbyint( h * kPinv );
  std::int64_t r = std::int64_t( std::fma( -q , kP , h ) + l );
  while( r < 0 ) r += std::int64_t( P );
  while( r >= std::int64_t( P ) ) r -= std::int64_t( P );
  return( std::uint64_t( r ) );
  }

 /// t[ k ] = w^k centered, k < m, on four chains of products by w^4
 static void powers( double * t , std::size_t m , std::uint64_t w ) {
  std::uint64_t x[ 4 ] = { 1 , w , mulFast( w , w ) , 0 };
  x[ 3 ] = mulFast( x[ 2 ] , w );
  const std::uint64_t w4 = mulFast( x[ 2 ] , x[ 2 ] );
  for( std::size_t k = 0 ; k < m ; k += 4 )
   for( std::size_t j = 0 ; ( j < 4 ) && ( k + j < m ) ; ++j ) {
    t[ k + j ] = centered( x[ j ] );
    x[ j ] = mulFast( x[ j ] , w4 );
    }
  }

 /// the roots of the rounds of h < W, split as the pairs of vectors
 template< class O >
 static typename O::V twiddles( const double * w , std::size_t h ) {
  double s[ 2 * O::W ];
  for( std::size_t j = 0 ; j < 2 * O::W ; ++j ) {
   const std::size_t k = j % ( 2 * h );
   s[ j ] = k < h ? w[ k ] : 0;
   }
  typename O::V x , y;
  O::split( O::load( s ) , O::load( s + O::W ) , h , x , y );
  return( x );
  }

 /// the transform of a, n >= 2 W a power of 2, from the natural order to
 /// that of the reversed bits (Gentleman and Sande)
 template< class O >
 static void forward( double * a , std::size_t n , const double * T ) {
  using V = typename O::V;
  std::size_t h = n / 2;
  for( ; h >= 2 * O::W ; h /= 4 ) {         // two rounds at a time
   const std::size_t q = h / 2;
   const double * w = T + h , * v = T + q;
   for( std::size_t i = 0 ; i < n ; i += 2 * h )
    for( std::size_t k = 0 ; k < q ; k += O::W ) {
     double * x = a + i + k;
     const V x0 = O::load( x ) , x1 = O::load( x + q ) ,
             x2 = O::load( x + 2 * q ) , x3 = O::load( x + 3 * q ) ,
             u0 = reduce< O >( O::add( x0 , x2 ) ) ,
             u1 = reduce< O >( O::add( x1 , x3 ) ) ,
             v0 = mulmod< O >( O::sub( x0 , x2 ) , O::load( w + k ) ) ,
             v1 = mulmod< O >( O::sub( x1 , x3 ) , O::load( w + q + k ) ) ,
             t = O::load( v + k );
     O::store( x , reduce< O >( O::add( u0 , u1 ) ) );
     O::store( x + q , mulmod< O >( O::sub( u0 , u1 ) , t ) );
     O::store( x + 2 * q , reduce< O >( O::add( v0 , v1 ) ) );
     O::store( x + 3 * q , mulmod< O >( O::sub( v0 , v1 ) , t ) );
     }
   }
  for( ; h >= 1 ; h /= 2 ) {
   const double * w = T + h;
   if( h >= O::W )
    for( std::size_t i = 0 ; i < n ; i += 2 * h )
     for( std::size_t k = 0 ; k < h ; k += O::W ) {
      const V x = O::load( a + i + k ) , y = O::load( a + i + h + k );
      O::store( a + i + k , reduce< O >( O::add( x , y ) ) );
      O::store( a + i + h + k , mulmod< O >( O::sub( x , y ) ,
                                             O::load( w + k ) ) );
      }
   else {
    const V t = twiddles< O >( w , h );
    for( std::size_t i = 0 ; i < n ; i += 2 * O::W ) {
     V x , y;
     O::split( O::load( a + i ) , O::load( a + i + O::W ) , h , x , y );
     O::join( reduce< O >( O::add( x , y ) ) ,
              mulmod< O >( O::sub( x , y ) , t ) , h , x , y );
     O::store( a + i , x );
     O::store( a + i + O::W , y );
     }
    }
   }
  }

 /// the inverse transform times n, from the order of the reversed bits to
 /// the natural one (Cooley and Tukey); x is reduced before x + y w and
 /// x - y w, which stay within 1.4 P, and so do the results
 template< class O >
 static void backward( double * a , std::size_t n , const double * T ) {
  using V = typename O::V;
  std::size_t h = 1;
  for( ; h < O::W ; h *= 2 ) {
   const double * w = T + h;
   const V t = twiddles< O >( w , h );
   for( std::size_t i = 0 ; i < n ; i += 2 * O::W ) {
    V x , y;
    O::split( O::load( a + i ) , O::load( a + i + O::W ) , h , x , y );
    x = reduce< O >( x );
    y = mulmod< O >( y , t );
    O::join( O::add( x , y ) , O::sub( x , y ) , h , x , y );
    O::store( a + i , x );
    O::store( a + i + O::W , y );
    }
   }
  for( ; 4 * h <= n ; h *= 4 ) {            // two rounds at a time
   const double * v = T + h , * w = T + 2 * h;
   for( std::size_t i = 0 ; i < n ; i += 4 * h )
    for( std::size_t k = 0 ; k < h ; k += O::W ) {
     double * x = a + i + k;
     const V t = O::load( v + k ) ,
             x0 = reduce< O >( O::load( x ) ) ,
             x1 = mulmod< O >( O::load( x + h ) , t ) ,
             x2 = reduce< O >( O::load( x + 2 * h ) ) ,
             x3 = mulmod< O >( O::load( x + 3 * h ) , t ) ,
             u0 = reduce< O >( O::add( x0 , x1 ) ) ,
             u1 = reduce< O >( O::sub( x0 , x1 ) ) ,
             u2 = mulmod< O >( O::add( x2 , x3 ) , O::load( w + k ) ) ,
             u3 = mulmod< O >( O::sub( x2 , x3 ) , O::load( w + h + k ) );
     O::store( x , O::add( u0 , u2 ) );
     O::store( x + 2 * h , O::sub( u0 , u2 ) );
     O::store( x + h , O::add( u1 , u3 ) );
     O::store( x + 3 * h , O::sub( u1 , u3 ) );
     }
   }
  for( ; h < n ; h *= 2 ) {
   const double * w = T + h;
   if( h >= O::W )
    for( std::size_t i = 0 ; i < n ; i += 2 * h )
     for( std::size_t k = 0 ; k < h ; k += O::W ) {
      const V x = reduce< O >( O::load( a + i + k ) ) ,
              y = mulmod< O >( O::load( a + i + h + k ) , O::load( w + k ) );
      O::store( a + i + k , O::add( x , y ) );
      O::store( a + i + h + k , O::sub( x , y ) );
      }
   else {
    const V t = twiddles< O >( w , h );
    for( std::size_t i = 0 ; i < n ; i += 2 * O::W ) {
     V x , y;
     O::split( O::load( a + i ) , O::load( a + i + O::W ) , h , x , y );
     x = reduce< O >( x );
     y = mulmod< O >( y , t );
     O::join( O::add( x , y ) , O::sub( x , y ) , h , x , y );
     O::store( a + i , x );
     O::store( a + i + O::W , y );
     }
    }
   }
  }

 /// f = the digits of x modulo P, x of nx digits as the halves of 32 bits
 /// hi and lo, and zeros up to n
 template< class O >
 static void load( double * f , const double * hi , const double * lo ,
                   std::size_t nx , std::size_t n ) {
  const typename O::V c = O::set( centered( ( std::uint64_t( 1 ) << 32 ) %
                                            P ) );
  std::size_t i = 0;
  for( ; i + O::W <= nx ; i += O::W )
   O::store( f + i , reduce< O >( O::add( mulmod< O >( O::load( hi + i ) ,
                                                       c ) ,
                                          O::load( lo + i ) ) ) );
  for( ; i < nx ; ++i )                     // the last ones, padded
   f[ i ] = double( ( UInt128( std::uint64_t( hi[ i ] ) ) << 32 |
                      std::uint64_t( lo[ i ] ) ) % P );
  std::fill( f + nx , f + n , 0.0 );
  }

 /// the powers w^k and w^{2k}, k < m, of a root w of unity of order 3 m
 /// (inverse if inverse), centered; exact, kept per thread
 static const double * roots3( std::size_t m , bool inverse ) {
  thread_local std::vector< double > T[ 2 ][ 23 ];
  auto & t = T[ inverse ][ std::bit_width( m ) - 1 ];
  if( t.empty() ) {
   t.resize( 2 * m );
   const std::uint64_t w = root( 3 * m , inverse );
   powers( t.data() , m , w );
   powers( t.data() + m , m , mulFast( w , w ) );
   }
  return( t.data() );
  }

 /// the round of radix 3 of a, n = 3 m terms: the items k, k + m and
 /// k + 2 m, with s = x1 + x2 and d = x1 - x2, turn into x0 + s and, by
 /// the cube roots of unity o and o^2, x0 + o x1 + o^2 x2 =
 /// x0 - s / 2 + d ( o - o^2 ) / 2 and its conjugate, times w^k and
 /// w^{2k} before (inverse) or after, all reduced
 template< class O >
 static void radix3( double * a , std::size_t m , bool inverse ) {
  using V = typename O::V;
  constexpr std::uint64_t o = power( G , ( P - 1 ) / 3 ) , oo = mul( o , o );
  const double * W = roots3( m , inverse );
  const V half = O::set( centered( ( P + 1 ) / 2 ) ) ,
          r = O::set( centered( mul( ( inverse ? oo + P - o : o + P - oo ) %
                                     P , ( P + 1 ) / 2 ) ) );
  for( std::size_t k = 0 ; k < m ; k += O::W ) {
   V x0 = O::load( a + k ) , x1 = O::load( a + m + k ) ,
     x2 = O::load( a + 2 * m + k );
   if( inverse ) {
    x0 = reduce< O >( x0 );
    x1 = mulmod< O >( x1 , O::load( W + k ) );
    x2 = mulmod< O >( x2 , O::load( W + m + k ) );
    }
   const V t = O::add( x1 , x2 ) , u = mulmod< O >( t , half ) ,
           v = mulmod< O >( O::sub( x1 , x2 ) , r ) ,
           z = O::sub( x0 , u );
   V y1 = reduce< O >( O::add( z , v ) ) , y2 = reduce< O >( O::sub( z , v ) );
   if( ! inverse ) {
    y1 = mulmod< O >( y1 , O::load( W + k ) );
    y2 = mulmod< O >( y2 , O::load( W + m + k ) );
    }
   O::store( a + k , reduce< O >( O::add( x0 , t ) ) );
   O::store( a + m + k , y1 );
   O::store( a + 2 * m + k , y2 );
   }
  }

 /// fa = the cyclic convolution modulo P of x and y, of nx and ny digits
 /// given by their halves, n terms, n a power of 2 or 3 times one, with
 /// fb as scratch; a square if fb is null
 template< class O >
 static void convolve( double * fa , double * fb , const double * xh ,
                       const double * xl , std::size_t nx ,
                       const double * yh , const double * yl ,
                       std::size_t ny , std::size_t n ) {
  const std::size_t m = std::has_single_bit( n ) ? n : n / 3;
  const double * F = roots( m , false ) , * B = roots( m , true );
  auto fwd = [ & ]( double * f ) {          // a round of radix 3 first
   if( m < n ) radix3< O >( f , m , false );
   for( std::size_t j = 0 ; j < n ; j += m ) forward< O >( f + j , m , F );
   };
  load< O >( fa , xh , xl , nx , n );
  fwd( fa );
  if( fb ) {
   load< O >( fb , yh , yl , ny , n );
   fwd( fb );
   }
  else fb = fa;
  const typename O::V c = O::set( centered( P - ( P - 1 ) / n ) );   // 1 / n
  for( std::size_t i = 0 ; i < n ; i += O::W )
   O::store( fa + i , mulmod< O >( mulmod< O >( O::load( fa + i ) ,
                                                O::load( fb + i ) ) , c ) );
  for( std::size_t j = 0 ; j < n ; j += m ) backward< O >( fa + j , m , B );
  if( m < n ) radix3< O >( fa , m , true );
  }

 };  // end( struct FNTT )
#endif

}  // end( namespace detail )

inline BigInteger::BigInteger( long long v ) : f_neg( v < 0 )
{
 auto m = v < 0 ? std::uint64_t( 0 ) - std::uint64_t( v ) : std::uint64_t( v );
 if( m ) f_d = { Limb( m ) };
 }

inline BigInteger::BigInteger( bool neg , Mag d ) : f_d( std::move( d ) )
{
 trim( f_d );
 f_neg = neg && ! f_d.empty();
 }

inline void BigInteger::trim( Mag & d )
{
 while( ! d.empty() && ( d.back() == 0 ) ) d.pop_back();
 }

inline bool BigInteger::normal( void ) const
{
 return( ( f_d.empty() || ( f_d.back() != 0 ) ) &&
         ! ( f_neg && f_d.empty() ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE OPERATIONS ON SPANS ------------------------*/
/*--------------------------------------------------------------------------*/

inline int BigInteger::cmpN( const Limb * a , const Limb * b , std::size_t n )
{
 for( std::size_t i = n ; i-- > 0 ; )
  if( a[ i ] != b[ i ] ) return( a[ i ] < b[ i ] ? -1 : 1 );
 return( 0 );
 }

inline BigInteger::Limb BigInteger::addN( Limb * r , const Limb * a ,
                                          std::size_t na , const Limb * b ,
                                          std::size_t nb )
{
 Limb c = 0;
 std::size_t i = 0;
#if ADS_BIGINT_ASM
 if( nb >= 4 ) c = detail::addNAsm( r , a , b , i = nb );
#endif
 for( ; i < nb ; ++i ) r[ i ] = detail::addCarry( a[ i ] , b[ i ] , c );
 for( ; c && ( i < na ) ; ++i ) c = ( ( r[ i ] = a[ i ] + 1 ) == 0 );
 if( r != a ) std::copy( a + i , a + na , r + i );
 return( c );
 }

inline BigInteger::Limb BigInteger::subN( Limb * r , const Limb * a ,
                                          std::size_t na , const Limb * b ,
                                          std::size_t nb )
{
 Limb c = 0;                                // the borrow, 0 or 1
 std::size_t i = 0;
#if ADS_BIGINT_ASM
 if( nb >= 4 ) c = detail::subNAsm( r , a , b , i = nb );
#endif
 for( ; i < nb ; ++i ) r[ i ] = detail::subBorrow( a[ i ] , b[ i ] , c );
 for( ; c && ( i < na ) ; ++i ) c = ( ( r[ i ] = a[ i ] - 1 ) == ~Limb( 0 ) );
 if( r != a ) std::copy( a + i , a + na , r + i );
 return( c );
 }

inline BigInteger::Limb BigInteger::addTo( Limb * r , std::size_t nr ,
                                           const Limb * x , std::size_t nx )
{
 return( addN( r , r , nr , x , nx ) );
 }

inline BigInteger::Limb BigInteger::subFrom( Limb * r , std::size_t nr ,
                                             const Limb * x , std::size_t nx )
{
 return( subN( r , r , nr , x , nx ) );
 }

inline BigInteger::Limb BigInteger::mul1( Limb * r , const Limb * a ,
                                          std::size_t n , Limb m )
{
 Limb c = 0;
 std::size_t i = 0;
#if ADS_BIGINT_ASM
 if( ( i = n ) ) c = detail::mul1Asm( r , a , i , m );
#endif
 for( ; i < n ; ++i ) r[ i ] = detail::mulAdd( a[ i ] , m , c , 0 , c );
 return( c );
 }

inline BigInteger::Limb BigInteger::addMul1( Limb * r , const Limb * a ,
                                             std::size_t n , Limb m )
{
 Limb c = 0;                                // a m + r + c < 2^128
 std::size_t i = 0;
#if ADS_BIGINT_ASM
 if( ( i = n ) ) c = detail::addMul1Adx( r , a , i , m );
#endif
 for( ; i < n ; ++i ) r[ i ] = detail::mulAdd( a[ i ] , m , r[ i ] , c , c );
 return( c );
 }

inline BigInteger::Limb BigInteger::subMul1( Limb * r , const Limb * a ,
                                             std::size_t n , Limb m )
{
 Limb c = 0;
 std::size_t i = 0;
#if ADS_BIGINT_ASM
 if( ( n >= 2 ) && ( n <= 8 ) )
  return( detail::subMul1Short( r , a , n , m ) );
 if( n >= 4 ) {                             // the n mod 4 low digits first
  for( ; i < n % 4 ; ++i ) {
   Limb hi;
   const Limb lo = detail::mulAdd( a[ i ] , m , c , 0 , hi ) , x = r[ i ];
   r[ i ] = x - lo;
   c = hi + ( x < lo );
   }
  return( detail::subMul1Asm( r + i , a + i , n - i , m , c ) );
  }
#endif
 for( ; i < n ; ++i ) {
  Limb hi;
  const Limb lo = detail::mulAdd( a[ i ] , m , c , 0 , hi ) , x = r[ i ];
  r[ i ] = x - lo;
  c = hi + ( x < lo );
  }
 return( c );
 }

inline void BigInteger::addMul2( Limb * r , const Limb * a , std::size_t n ,
                                  Limb m0 , Limb m1 )
{
 // the rows r += a m0 and r + 1 += a m1 together, column by column, on
 // their carries H0 and H1
 Limb H0 , H1 = 0;
 r[ 0 ] = detail::mulAdd( a[ 0 ] , m0 , r[ 0 ] , 0 , H0 );
 for( std::size_t i = 1 ; i < n ; ++i ) {
  r[ i ] = detail::mulAdd( a[ i ] , m0 , r[ i ] , H0 , H0 );
  r[ i ] = detail::mulAdd( a[ i - 1 ] , m1 , r[ i ] , H1 , H1 );
  }
 r[ n ] = detail::mulAdd( a[ n - 1 ] , m1 , H0 , H1 , r[ n + 1 ] );
 }

inline bool BigInteger::absDiff( Limb * r , const Limb * a , std::size_t na ,
                                 const Limb * b , std::size_t nb )
{
 std::size_t i = na;
 while( ( i > nb ) && ( a[ i - 1 ] == 0 ) ) --i;
 if( ( i > nb ) || ( cmpN( a , b , nb ) >= 0 ) ) {
  subN( r , a , na , b , nb );
  return( false );
  }
 subN( r , b , nb , a , nb );               // a has zeros above nb
 std::fill( r + nb , r + na , 0 );
 return( true );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE CONVERSIONS -----------------------------*/
/*--------------------------------------------------------------------------*/

inline BigInteger::Limb BigInteger::divSmall( Mag & a , Limb d )
{
 const int s = std::countl_zero( d );       // d shifted, and a with it
 return( divSmall( a , d << s , detail::reciprocal( d << s ) , s ) );
 }

inline BigInteger::Limb BigInteger::divSmall( Mag & a , Limb dn , Limb v ,
                                              int s )
{
 const std::size_t n = a.size();
 Limb r = 0 , * const x = a.data();         // the digits through a pointer
 if( n == 0 ) return( 0 );
 if( s == 0 )
  for( std::size_t i = n ; i-- > 0 ; )
   x[ i ] = detail::div2by1( r , x[ i ] , dn , v , r );
 else {
  r = x[ n - 1 ] >> ( 64 - s );
  for( std::size_t i = n ; i-- > 0 ; ) {
   const Limb u = ( x[ i ] << s ) | ( i ? x[ i - 1 ] >> ( 64 - s ) : 0 );
   x[ i ] = detail::div2by1( r , u , dn , v , r );
   }
  }
 trim( a );
 return( r >> s );
 }

inline BigInteger::Limb BigInteger::modSmall( const Limb * a , std::size_t n ,
                                              Limb d )
{
 const int s = std::countl_zero( d );
 const Limb dn = d << s , v = detail::reciprocal( dn );
 Limb r = 0;
 if( n == 0 ) return( 0 );
 if( ( n >= kModFold ) && ( s >= 3 ) && ( d > 1 ) ) {
  // R = rh b + rl, below 2^128, takes 4 digits at a time as R b^4 + ... ,
  // i.e., as the sum of their products by c[ k ] = b^k mod d < 2^61: the
  // products of the digits do not wait for R, and one division by d is
  // left at the end
  auto mod = [ & ]( Limb hi , Limb lo ) {   // ( hi b + lo ) mod d, hi < d
   Limb m;
   detail::div2by1( ( hi << s ) | ( lo >> ( 64 - s ) ) , lo << s , dn , v ,
                    m );
   return( m >> s );
   };
  Limb c[ 6 ] , rh = a[ n - 1 ] , rl = a[ n - 2 ];
  auto mul = [ & ]( Limb x , Limb y ) {     // x y mod d
   Limb H;
   const Limb L = detail::mulWide( x , y , H );
   return( mod( H , L ) );
   };
  c[ 1 ] = mod( 1 , 0 );                    // in three rounds
  c[ 2 ] = mul( c[ 1 ] , c[ 1 ] );
  c[ 3 ] = mul( c[ 1 ] , c[ 2 ] );
  c[ 4 ] = mul( c[ 2 ] , c[ 2 ] );
  c[ 5 ] = mul( c[ 2 ] , c[ 3 ] );
  auto acc = []( Limb & H , Limb & L , Limb x , Limb y ) {   // += x y
   Limb ph;
   const Limb pl = detail::mulWide( x , y , ph );
   L += pl;
   H += ph + ( L < pl );
   };
  std::size_t i = n - 2;
#if ADS_BIGINT_WIDE == 128
  using U = detail::UInt128;
  for( ; i >= 4 ; i -= 4 ) {
   const U x = U( a[ i - 1 ] ) * c[ 3 ] + U( a[ i - 2 ] ) * c[ 2 ] +
               U( a[ i - 3 ] ) * c[ 1 ] + a[ i - 4 ] ,
           y = U( rl ) * c[ 4 ] + U( rh ) * c[ 5 ] + x;
   rl = Limb( y );
   rh = Limb( y >> 64 );
   }
#else
  for( ; i >= 4 ; i -= 4 ) {
   Limb H , L = detail::mulAdd( a[ i - 1 ] , c[ 3 ] , a[ i - 4 ] , 0 , H ) ,
        G , K = detail::mulWide( rl , c[ 4 ] , G );
   acc( H , L , a[ i - 2 ] , c[ 2 ] );
   acc( G , K , rh , c[ 5 ] );
   acc( H , L , a[ i - 3 ] , c[ 1 ] );
   rl = L + K;
   rh = H + G + ( rl < K );
   }
#endif
  for( ; i > 0 ; --i ) {
   Limb H , L = detail::mulAdd( rl , c[ 1 ] , a[ i - 1 ] , 0 , H );
   acc( H , L , rh , c[ 2 ] );
   rh = H;
   rl = L;
   }
  return( mod( mod( 0 , rh ) , rl ) );      // ( rh b + rl ) mod d
  }
 if( s == 0 )
  for( std::size_t i = n ; i-- > 0 ; )
   detail::div2by1( r , a[ i ] , dn , v , r );
 else {
  r = a[ n - 1 ] >> ( 64 - s );
  for( std::size_t i = n ; i-- > 0 ; )
   detail::div2by1( r , ( a[ i ] << s ) |
                        ( i ? a[ i - 1 ] >> ( 64 - s ) : 0 ) , dn , v , r );
  }
 return( r >> s );
 }

inline void BigInteger::mulAddSmall( Mag & a , Limb m , Limb c )
{
 Limb * const x = a.data();
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  x[ i ] = detail::mulAdd( x[ i ] , m , c , 0 , c );
 if( c ) a.push_back( c );
 }

inline BigInteger BigInteger::fromStringSchool( const std::string & s ,
                                                int base )
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 std::size_t i = 0;
 bool neg = false;
 if( ( i < s.size() ) && ( ( s[ i ] == '-' ) || ( s[ i ] == '+' ) ) )
  neg = s[ i++ ] == '-';
 ADS_PRE( i < s.size() );                   // at least one digit
 const int k = detail::bigBase( base ).second;
 Mag d;
 while( i < s.size() ) {                    // k digits at a time
  Limb chunk = 0 , scale = 1;
  for( int t = 0 ; ( t < k ) && ( i < s.size() ) ; ++t , ++i ) {
   const int v = detail::digitValue( s[ i ] );
   ADS_PRE( ( v >= 0 ) && ( v < base ) );
   chunk = chunk * Limb( base ) + Limb( v );
   scale *= Limb( base );
   }
  mulAddSmall( d , scale , chunk );
  }
 return( BigInteger( neg , std::move( d ) ) );
 }

inline BigInteger::Mag BigInteger::fromChunks( const Limb * c , std::size_t n ,
                                               const std::vector< Mag > & pw ,
                                               Limb P )
{
 Mag x;
 if( n < kConvert ) {                       // Horner, a product by P each
  x.reserve( n );
  for( std::size_t j = n ; j-- > 0 ; ) mulAddSmall( x , P , c[ j ] );
  return( x );
  }
 const std::size_t h = std::bit_floor( n - 1 );   // n / 2 <= h < n
 const Mag & p = pw[ std::size_t( std::countr_zero( h ) ) ];
 const Mag hi = fromChunks( c + h , n - h , pw , P ) ,
           lo = fromChunks( c , h , pw , P );
 if( hi.empty() ) return( lo );
 x.resize( hi.size() + p.size() );          // hi P^h + lo, lo < P^h
 mulP( x.data() , hi.data() , hi.size() , p.data() , p.size() );
 addTo( x.data() , x.size() , lo.data() , lo.size() );
 trim( x );
 return( x );
 }

inline BigInteger::BigInteger( const std::string & s , int base )
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 std::size_t i = 0;
 bool neg = false;
 if( ( i < s.size() ) && ( ( s[ i ] == '-' ) || ( s[ i ] == '+' ) ) )
  neg = s[ i++ ] == '-';
 ADS_PRE( i < s.size() );                   // at least one digit
 for( std::size_t j = i ; j < s.size() ; ++j )
  ADS_PRE( ( detail::digitValue( s[ j ] ) >= 0 ) &&
           ( detail::digitValue( s[ j ] ) < base ) );
 while( ( i + 1 < s.size() ) && ( s[ i ] == '0' ) ) ++i;
 const std::size_t L = s.size() - i;        // the figures
 if( ( base & ( base - 1 ) ) == 0 ) {       // b bits per figure
  const std::size_t b = std::size_t( std::countr_zero( unsigned( base ) ) );
  f_d.assign( ( L * b + 63 ) / 64 , 0 );
  std::size_t bit = 0;
  for( std::size_t j = s.size() ; j-- > i ; bit += b ) {
   const Limb v = Limb( detail::digitValue( s[ j ] ) );
   const std::size_t w = bit / 64 , o = bit % 64;
   f_d[ w ] |= v << o;
   if( o + b > 64 ) f_d[ w + 1 ] |= v >> ( 64 - o );
   }
  }
 else {                                     // chunks of k figures
  const auto [ P , k ] = detail::bigBase( base );
  const std::size_t K = std::size_t( k ) , C = ( L + K - 1 ) / K;
#if ADS_BIGINT_GMP
  {
   // mpn_set_str() on the values of the figures, the most significant
   // first, into room for the bits of L figures and one digit more
   std::vector< unsigned char > f( L );
   for( std::size_t j = 0 ; j < L ; ++j )
    f[ j ] = static_cast< unsigned char >(
                                        detail::digitValue( s[ i + j ] ) );
   f_d.resize( ( L * std::size_t( std::bit_width( unsigned( base - 1 ) ) )
                 + 63 ) / 64 + 1 );
   f_d.resize( std::size_t( mpn_set_str(
                             reinterpret_cast< mp_limb_t * >( f_d.data() ) ,
                             f.data() , L , base ) ) );
   trim( f_d );
   f_neg = neg && ! f_d.empty();
   ADS_INVARIANT( normal() );
   ADS_AUDIT( *this == fromStringSchool( s , base ) );
   return;
   }
#endif
  Mag c( C );                               // the least significant first
  for( std::size_t j = 0 ; j < C ; ++j ) {
   const std::size_t hi = s.size() - j * K , lo = hi - std::min( K , hi - i );
   Limb x = 0;
   for( std::size_t t = lo ; t < hi ; ++t )
    x = x * Limb( base ) + Limb( detail::digitValue( s[ t ] ) );
   c[ j ] = x;
   }
  std::vector< Mag > pw( 1 , Mag{ P } );    // P^{2^i}, 2^i < C
  if( C >= kConvert )
   while( ( std::size_t( 1 ) << pw.size() ) < C )
    pw.push_back( mulMag( pw.back() , pw.back() ) );
  f_d = fromChunks( c.data() , C , pw , P );
  }
 trim( f_d );
 f_neg = neg && ! f_d.empty();
 ADS_INVARIANT( normal() );
 ADS_AUDIT( *this == fromStringSchool( s , base ) );
 }

inline void BigInteger::figures( Mag a , std::size_t pad , int base ,
                                 std::string & out , Mag * b ,
                                 std::size_t padb )
{
 const auto [ P , k ] = detail::bigBase( base );
 const int s = std::countl_zero( P );       // its reciprocal once
 const Limb dn = P << s , v = detail::reciprocal( dn );
 // the chunks of k figures of a and of b, the least significant first:
 // the digits of both divided by P in the same pass, two chains of
 // divisions that do not wait for each other
 std::vector< Limb > c[ 2 ];
 Mag * x[ 2 ] = { &a , b };
 const int m = b ? 2 : 1;
 for( int j = 0 ; j < m ; ++j )
  c[ j ].reserve( x[ j ]->size() + x[ j ]->size() / 4 + 1 );
 auto step = [ & ]( Limb * y , std::size_t i , Limb & r ) {
  const Limb u = s ? ( y[ i ] << s ) | ( i ? y[ i - 1 ] >> ( 64 - s ) : 0 )
                   : y[ i ];
  y[ i ] = detail::div2by1( r , u , dn , v , r );
  };
 while( ! a.empty() || ( b && ! b->empty() ) ) {
  std::size_t n[ 2 ] = { a.size() , b ? b->size() : 0 };
  Limb r[ 2 ] = { 0 , 0 } , * y[ 2 ] = { a.data() , b ? b->data() : nullptr };
  for( int j = 0 ; j < m ; ++j )
   if( s && n[ j ] ) r[ j ] = y[ j ][ n[ j ] - 1 ] >> ( 64 - s );
  for( ; n[ 0 ] > n[ 1 ] ; ) step( y[ 0 ] , --n[ 0 ] , r[ 0 ] );
  for( ; n[ 1 ] > n[ 0 ] ; ) step( y[ 1 ] , --n[ 1 ] , r[ 1 ] );
  for( std::size_t i = n[ 0 ] ; i-- > 0 ; ) {
   step( y[ 0 ] , i , r[ 0 ] );
   step( y[ 1 ] , i , r[ 1 ] );
   }
  for( int j = 0 ; j < m ; ++j )
   if( ! x[ j ]->empty() ) {
    trim( *x[ j ] );
    c[ j ].push_back( r[ j ] >> s );
    }
  }
 for( int j = 0 ; j < m ; ++j ) {          // then their figures
  const std::vector< Limb > & d = c[ j ];
  std::size_t top = 0;                      // the figures of the top chunk
  if( ! d.empty() )
   for( Limb y = d.back() ; y ; y /= Limb( base ) ) ++top;
  const std::size_t len = d.empty() ? 0 : ( d.size() - 1 ) *
                                          std::size_t( k ) + top ,
                    p = j ? padb : pad;
  if( p > len ) out.append( p - len , '0' );
  const std::size_t o = out.size();
  out.resize( o + len );
  char * end = out.data() + o + len;
  for( std::size_t i = 0 ; i + 1 < d.size() ; ++i , end -= k )
   detail::putFigures( end , d[ i ] , k , base );
  if( ! d.empty() ) detail::putFigures( end , d.back() , int( top ) , base );
  }
 }

inline bool BigInteger::mulBand( Limb * r , const Limb * a , std::size_t na ,
                                 const Limb * b , std::size_t nb ,
                                 std::size_t s , std::size_t e )
{
#if ADS_BIGINT_ASM
 if( ( nb < 4 ) || ( nb >= 2 * kKaratsuba ) || ( s < 6 ) ||
     ( s + 6 > na ) )
  return( false );
 // by passes of 4 rows of b, each over the digits a_i with
 // s - 5 - j <= i < e - j for its first row j: the place of a product
 // a_i b_j, i + j, is in [ s - 5 , e + 3 ), the passes write r from the
 // place s - 5 on, each its top 4 digits fresh, as in Algorithm M
 std::size_t j = 0;
 for( ; j + 4 <= nb ; j += 4 ) {
  const std::size_t i0 = s - 5 > j ? s - 5 - j : 0 ,
                    i1 = std::min( na , e - std::min( e , j ) );
  if( i0 >= i1 ) continue;
  if( j == 0 ) detail::mulPass4< false >( r + j + i0 , a + i0 , i1 - i0 , b );
  else detail::mulPass4< true >( r + j + i0 , a + i0 , i1 - i0 , b + j );
  }
 for( ; j < nb ; ++j ) {
  const std::size_t i0 = s - 5 > j ? s - 5 - j : 0 ,
                    i1 = std::min( na , e - std::min( e , j ) );
  if( i0 < i1 )
   r[ j + i1 ] = addMul1( r + j + i0 , a + i0 , i1 - i0 , b[ j ] );
  }
 return( true );
#else
 (void) r; (void) a; (void) na; (void) b; (void) nb; (void) s; (void) e;
 return( false );
#endif
 }

inline void BigInteger::fracLeaf( Mag & f , std::size_t c , Limb P , int k ,
                                  int base , bool up , char * out )
{
 // the chunks of k figures, the integer parts of f P , f P^2 , ...; with
 // c' chunks left, ceil( c' log_b P ) + 2 digits of f are enough, the
 // others dropped, f rounded up if up [see fracTree()]
 const double lg = std::log2( double( P ) ) / 64;
 Limb * x = f.data();
 std::size_t p = f.size();
 const Limb one = 1;
 for( std::size_t i = 0 ; i < c ; ++i ) {
  detail::putFigures( out + ( i + 1 ) * std::size_t( k ) ,
                      mul1( x , x , p , P ) , k , base );
  const std::size_t need = std::size_t( double( c - i - 1 ) * lg ) + 3;
  if( need < p ) {
   x += p - need;
   p = need;
   if( up ) addTo( x , p , &one , 1 );
   }
  }
 }

inline void BigInteger::fracTree( Mag & f , std::size_t j , int base , int k ,
                                  Limb P , const std::vector< Mag > & pw ,
                                  const std::vector< std::size_t > & zs ,
                                  bool up , char * out )
{
 const std::size_t c = std::size_t( 1 ) << j;   // the chunks
 if( c <= kFracLeaf ) {
  fracLeaf( f , c , P , k , base , up , out );
  return;
  }
 // f = ( X + theta ) / P^c, X the integer of the c chunks, 0 <= theta < 1,
 // theta below 1 / 2 by a little if up, else above: then the integer parts
 // of f P^i , i <= c, are those of X / P^{c-i}. With h = c / 2 chunks
 // each half and P^h = pw b^z of q digits, the low half has the fraction
 // of f P^h, the digits below the place p - z of f pw, the same theta;
 // the high half has the top digits of f, its theta the fraction r of
 // the low half: if r >= 1 / 2 it goes down, else up. The digits kept
 // are q + 2, so that each rounding moves theta by less than 3 b^{-2}
 const Mag & w = pw[ j - 1 ];
 const std::size_t z = zs[ j - 1 ] , q = w.size() + z , p = f.size() ,
                   pc = q + 2;
 const Limb one = 1;
 Mag t( p + w.size() + 4 );
 const std::size_t s = p - z - pc;          // the window [ s , p - z )
 Limb e = 1;                                // short by e at most
 if( mulBand( t.data() , f.data() , p , w.data() , w.size() , s , p - z ) )
  e = 3;                                    // 2 for the products left out
 else mulP( t.data() , f.data() , p , w.data() , w.size() );
 Mag lo( t.data() + s , t.data() + p - z );
 if( up ) addTo( lo.data() , pc , &e , 1 );
 const bool uph = ( lo.back() >> 63 ) == 0;
 Mag hi( f.data() + p - pc , f.data() + p );
 if( uph ) addTo( hi.data() , pc , &one , 1 );
 const std::size_t h = std::size_t( k ) << ( j - 1 );
 fracTree( hi , j - 1 , base , k , P , pw , zs , uph , out );
 fracTree( lo , j - 1 , base , k , P , pw , zs , up , out + h );
 }

inline void BigInteger::fracFigures( const Mag & x , std::size_t j , int base ,
                                     int k , Limb P ,
                                     const std::vector< Mag > & pw ,
                                     const std::vector< std::size_t > & zs ,
                                     std::vector< Mag > & rf ,
                                     std::string & out )
{
 // f = x / P^C, C = 2^j chunks, to p = q + 2 digits, from the reciprocal
 // R = ceil( b^{q+p} / P^C ), once per j: x R < b^{q+p} x / P^C + x, whose
 // top p digits plus 1 are no smaller than f b^p and larger by 2 at most
 const std::size_t q = pw[ j ].size() + zs[ j ] , p = q + 2;
 if( rf.size() <= j ) rf.resize( j + 1 );
 if( rf[ j ].empty() ) {
  Mag u( q + p - zs[ j ] + 1 , 0 );         // b^{q+p-z}
  u.back() = 1;
  auto [ r , rem ] = divModMag( u , pw[ j ] );
  if( ! rem.empty() ) r = addMag( r , Mag{ 1 } );
  rf[ j ] = std::move( r );
  }
 const Mag & R = rf[ j ];
 Mag t( std::max( x.size() + R.size() , q + p ) , 0 );
 if( ! x.empty() )
  mulP( t.data() , x.data() , x.size() , R.data() , R.size() );
 Mag f( t.data() + q , t.data() + q + p );
 const Limb one = 1;
 addTo( f.data() , p , &one , 1 );
 const std::size_t o = out.size();         // the C k figures
 out.resize( o + ( std::size_t( k ) << j ) );
 fracTree( f , j , base , k , P , pw , zs , true , out.data() + o );
 }

inline void BigInteger::toFigures( const Mag & x , std::size_t pad , int base ,
                                   const std::vector< Mag > & pw ,
                                   const std::vector< std::size_t > & zs ,
                                   int k , std::vector< Mag > & rc ,
                                   std::vector< Mag > & rf ,
                                   std::string & out , std::size_t j )
{
 if( ( j != npos ) && ( j >= kFracLevel ) &&
     ( pw[ j ].size() + zs[ j ] <= kConvertF ) ) {
  fracFigures( x , j , base , k , detail::bigBase( base ).first , pw , zs ,
               rf , out );
  return;
  }
 if( x.size() < kConvert ) {
  figures( x , pad , base , out );
  return;
  }
 std::size_t i = 0;                         // the power nearest sqrt( x )
 while( ( i + 1 < pw.size() ) &&
        ( pw[ i + 1 ].size() + zs[ i + 1 ] <= ( x.size() + 1 ) / 2 ) )
  ++i;
 // x = q P b^z + r b^z + x0, x0 the z low digits and P b^z the power
 const std::size_t z = std::min( zs[ i ] , x.size() );
 const auto [ q , rh ] = divModMag( Mag( x.data() + z , x.data() + x.size() ) ,
                                    pw[ i ] , false , &rc[ i ] );
 Mag r( z + rh.size() );
 std::copy( x.data() , x.data() + z , r.data() );
 std::copy( rh.data() , rh.data() + rh.size() , r.data() + z );
 trim( r );
 const std::size_t low = std::size_t( k ) << i;   // the figures of r
 if( ( q.size() < kConvert ) && ( r.size() < kConvert ) &&
     ( i < kFracLevel ) ) {                 // together
  figures( q , pad > low ? pad - low : 0 , base , out , &r , low );
  return;
  }
 toFigures( q , pad > low ? pad - low : 0 , base , pw , zs , k , rc , rf ,
            out , npos );
 toFigures( r , low , base , pw , zs , k , rc , rf , out , i );
 }

inline std::string BigInteger::toString( int base ) const
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 if( f_d.empty() ) return( "0" );
 std::string out;
 if( f_neg ) out.push_back( '-' );
 const std::size_t n = f_d.size();
 if( ( base & ( base - 1 ) ) == 0 ) {       // b bits per figure
  const std::size_t b = std::size_t( std::countr_zero( unsigned( base ) ) ) ,
                    bits = 64 * n - std::size_t( std::countl_zero(
                                                              f_d.back() ) ) ,
                    D = ( bits + b - 1 ) / b , o = out.size();
  out.resize( o + D );
  for( std::size_t j = 0 ; j < D ; ++j ) {   // the figure j from the right
   const std::size_t w = j * b / 64 , sh = j * b % 64;
   Limb x = f_d[ w ] >> sh;
   if( ( sh + b > 64 ) && ( w + 1 < n ) ) x |= f_d[ w + 1 ] << ( 64 - sh );
   out[ o + D - 1 - j ] = detail::kFigures[ x & Limb( base - 1 ) ];
   }
  }
 else if( n < kConvert ) figures( f_d , 0 , base , out );
#if ADS_BIGINT_GMP
 else if( n < kGmpToStringTop ) {
  mpz_t X;                                  // mpz_get_str(), in place
  mpz_srcptr x = gmpView( X , f_d.data() , n );
  const std::size_t o = out.size();
  out.resize( o + mpz_sizeinbase( x , base ) + 1 );
  mpz_get_str( out.data() + o , base , x );
  out.resize( o + std::strlen( out.data() + o ) );
  }
#endif
 else {
  const auto [ P , k ] = detail::bigBase( base );
  // the powers up to half the digits, pw[ i ] b^{zs[ i ]}, without the low
  // digits 0 that those of the even bases have, squared so, and their
  // reciprocals, kept per thread and base for the next conversions
  struct Powers {
   std::vector< Mag > pw , rc , rf;
   std::vector< std::size_t > zs;
   };
  thread_local Powers kept[ 37 ];
  Powers & c = kept[ base ];
  if( c.pw.empty() ) {
   c.pw.assign( 1 , Mag{ P } );
   c.zs.assign( 1 , 0 );
   }
  while( 2 * ( c.pw.back().size() + c.zs.back() ) - 1 <= ( n + 1 ) / 2 ) {
   Mag p = mulMag( c.pw.back() , c.pw.back() );
   std::size_t z = 0;
   while( p[ z ] == 0 ) ++z;
   if( p.size() + 2 * c.zs.back() > ( n + 1 ) / 2 ) break;
   c.zs.push_back( 2 * c.zs.back() + z );
   c.pw.push_back( z ? Mag( p.data() + z , p.data() + p.size() )
                     : std::move( p ) );
   }
  c.rc.resize( c.pw.size() );
  toFigures( f_d , 0 , base , c.pw , c.zs , k , c.rc , c.rf , out , npos );
  }
 ADS_AUDIT( out == toStringSchool( base ) );
 return( out );
 }

inline std::string BigInteger::toStringSchool( int base ) const
{
 ADS_PRE( ( base >= 2 ) && ( base <= 36 ) );
 if( f_d.empty() ) return( "0" );
 std::string out;
 if( f_neg ) out.push_back( '-' );
 figures( f_d , 0 , base , out );
 return( out );
 }

/*--------------------------------------------------------------------------*/
/*------------------------- THE SUM AND THE ORDER --------------------------*/
/*--------------------------------------------------------------------------*/

inline int BigInteger::sign( void ) const
{
 return( f_d.empty() ? 0 : f_neg ? -1 : 1 );
 }

inline BigInteger BigInteger::abs( void ) const
{
 return( BigInteger( false , f_d ) );
 }

inline std::size_t BigInteger::limbs( void ) const
{
 return( f_d.size() );
 }

inline BigInteger BigInteger::operator-( void ) const
{
 return( BigInteger( ! f_neg , f_d ) );
 }

inline int BigInteger::compareMag( const Mag & a , const Mag & b )
{
 if( a.size() != b.size() ) return( a.size() < b.size() ? -1 : 1 );
 return( cmpN( a.data() , b.data() , a.size() ) );
 }

inline BigInteger::Mag BigInteger::addMag( const Mag & a , const Mag & b )
{
 const Mag & x = a.size() >= b.size() ? a : b;   // Algorithm A
 const Mag & y = a.size() >= b.size() ? b : a;
 Mag r( x.size() + 1 );
 r[ x.size() ] = addN( r.data() , x.data() , x.size() , y.data() ,
                       y.size() );
 if( r.back() == 0 ) r.pop_back();
 return( r );
 }

inline BigInteger::Mag BigInteger::subMag( const Mag & a , const Mag & b )
{
 Mag r( a.size() );                         // Algorithm S
 subN( r.data() , a.data() , a.size() , b.data() , b.size() );
 trim( r );
 return( r );
 }

inline BigInteger BigInteger::addSigned( const BigInteger & a ,
                                         const BigInteger & b , bool bneg )
{
 if( a.f_neg == bneg ) return( BigInteger( a.f_neg , addMag( a.f_d ,
                                                             b.f_d ) ) );
 const int c = compareMag( a.f_d , b.f_d );
 if( c == 0 ) return( BigInteger() );
 return( c > 0 ? BigInteger( a.f_neg , subMag( a.f_d , b.f_d ) )
               : BigInteger( bneg , subMag( b.f_d , a.f_d ) ) );
 }

inline BigInteger operator+( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::addSigned( a , b , b.f_neg ) );
 }

inline BigInteger operator-( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::addSigned( a , b , ! b.f_neg ) );
 }

inline std::strong_ordering operator<=>( const BigInteger & a ,
                                         const BigInteger & b )
{
 if( a.sign() != b.sign() ) return( a.sign() <=> b.sign() );
 const int c = BigInteger::compareMag( a.f_d , b.f_d );
 return( a.f_neg ? 0 <=> c : c <=> 0 );
 }

inline BigInteger & BigInteger::operator+=( const BigInteger & b )
{
 if( f_neg != b.f_neg ) return( *this = *this + b );
 if( f_d.size() < b.f_d.size() ) f_d.resize( b.f_d.size() , 0 );
 const Limb c = addTo( f_d.data() , f_d.size() , b.f_d.data() ,
                       b.f_d.size() );
 if( c ) f_d.push_back( c );
 return( *this );
 }

inline BigInteger & BigInteger::operator-=( const BigInteger & b )
{
 if( f_neg == b.f_neg ) return( *this = *this - b );
 if( f_d.size() < b.f_d.size() ) f_d.resize( b.f_d.size() , 0 );
 const Limb c = addTo( f_d.data() , f_d.size() , b.f_d.data() ,
                       b.f_d.size() );
 if( c ) f_d.push_back( c );
 return( *this );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE PRODUCT -------------------------------*/
/*--------------------------------------------------------------------------*/

inline void BigInteger::mulSchool( Limb * r , const Limb * a , std::size_t na ,
                                   const Limb * b , std::size_t nb )
{
 if( na < nb ) {                            // the rows on the longer one
  std::swap( a , b );
  std::swap( na , nb );
  }
 if( nb == 0 ) {
  std::fill( r , r + na , 0 );
  return;
  }
#if ADS_BIGINT_ASM
 if( nb < 4 ) {                             // Algorithm M, by rows
  detail::mulRows< 0 , 1 , 0 >( r , a , na , b , nb );
  return;
  }
 // by passes of 4 rows, the rows left one at a time
 detail::mulPass4< false >( r , a , na , b );
 std::size_t j = 4;
 for( ; j + 4 <= nb ; j += 4 )
  detail::mulPass4< true >( r + j , a , na , b + j );
 for( ; j < nb ; ++j ) r[ na + j ] = addMul1( r + j , a , na , b[ j ] );
#else
 r[ na ] = mul1( r , a , na , b[ 0 ] );     // Algorithm M
 std::size_t j = 1;
 // two rows at once, which keeps the digits of r in registers
 for( ; j + 1 < nb ; j += 2 ) addMul2( r + j , a , na , b[ j ] , b[ j + 1 ] );
 for( ; j < nb ; ++j ) r[ na + j ] = addMul1( r + j , a , na , b[ j ] );
#endif
 }

inline void BigInteger::sqrTile( Limb * r , const Limb * a , std::size_t n )
{
 // the products a_i a_j, i < j, in t, then 2 t plus the squares
 Limb t[ 8 ] = {};
 for( std::size_t i = 0 ; i + 1 < n ; ++i ) {
  Limb c = 0;
  for( std::size_t j = i + 1 ; j < n ; ++j )
   t[ i + j ] = detail::mulAdd( a[ i ] , a[ j ] , t[ i + j ] , c , c );
  t[ i + n ] = c;
  }
 Limb s = 0 , c = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb lo = t[ 2 * i ] , hi = t[ 2 * i + 1 ];
  const Limb l2 = ( lo << 1 ) | s , h2 = ( hi << 1 ) | ( lo >> 63 );
  s = hi >> 63;
  Limb ph;
  const Limb pl = detail::mulWide( a[ i ] , a[ i ] , ph );
  r[ 2 * i ] = detail::addCarry( l2 , pl , c );
  r[ 2 * i + 1 ] = detail::addCarry( h2 , ph , c );
  }
 }

inline void BigInteger::sqrSchool( Limb * r , const Limb * a , std::size_t n )
{
 if( n == 0 ) return;
 r[ 0 ] = 0;                                // the products a_i a_j, i < j
 r[ 2 * n - 1 ] = 0;
#if ADS_BIGINT_ASM
 if( n >= 8 ) {
  // a = sum of the tiles A_k b^{4k} of 4 digits (the last one shorter),
  // a^2 = 2 T + D: T the products a_i a_j, i < j, of digits of different
  // tiles, by passes of 4 rows over the digits after each tile, and D the
  // squares A_k^2, of 8 digits each and side by side, at b^{8k}
  std::fill( r , r + 2 * n , 0 );
  for( std::size_t i = 0 ; i + 4 < n ; i += 4 )
   detail::mulPass4< true >( r + 2 * i + 4 , a + i + 4 , n - i - 4 , a + i );
  constexpr std::size_t kStack = 64;
  Limb in[ kStack ];
  std::unique_ptr< Limb[] > heap( 2 * n > kStack ? new Limb[ 2 * n ]
                                                 : nullptr );
  Limb * const d = heap ? heap.get() : in;
  std::size_t i = 0;
  for( ; i + 4 <= n ; i += 4 ) detail::sqr4Asm( d + 2 * i , a + i );
  if( i < n ) sqrTile( d + 2 * i , a + i , n - i );
  detail::lsh1AddAsm( r , d , 2 * n );
  return;
  }
 if( n > 1 ) detail::mulRows< 1 , 2 , 1 >( r + 1 , a + 1 , n - 1 , a , n - 1 );
#else
 r[ n ] = mul1( r + 1 , a + 1 , n - 1 , a[ 0 ] );
 for( std::size_t i = 1 ; i + 1 < n ; ++i )
  r[ n + i ] = addMul1( r + 2 * i + 1 , a + i + 1 , n - i - 1 , a[ i ] );
#endif
 Limb s = 0 , c = 0;                        // twice them, and the squares
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb lo = r[ 2 * i ] , hi = r[ 2 * i + 1 ];
  const Limb l2 = ( lo << 1 ) | s , h2 = ( hi << 1 ) | ( lo >> 63 );
  s = hi >> 63;
  Limb ph;
  const Limb pl = detail::mulWide( a[ i ] , a[ i ] , ph );
  r[ 2 * i ] = detail::addCarry( l2 , pl , c );
  r[ 2 * i + 1 ] = detail::addCarry( h2 , ph , c );
  }
 }

inline std::size_t BigInteger::mulScratch( std::size_t n )
{
 // a step on n digits takes at most 4 n + 24 and leaves at most
 // n / 2 + 2 digits to the next one
 return( 8 * n + 64 * std::size_t( std::bit_width( n ) ) + 64 );
 }

inline void BigInteger::mulN( Limb * r , const Limb * a , const Limb * b ,
                              std::size_t n , Limb * w , std::size_t fft ,
                              std::size_t toom )
{
 if( n < kKaratsuba ) {
  if( a == b ) sqrSchool( r , a , n );
  else mulSchool( r , a , n , b , n );
  }
 else if( ( n >= fft ) && ( 2 * n <= kNTTMax ) ) mulNTT( r , a , n , b , n );
 else if( n >= toom ) toom3( r , a , b , n , w , fft , toom );
 else karatsuba( r , a , b , n , w , fft , toom );
 }

inline void BigInteger::karatsuba( Limb * r , const Limb * a , const Limb * b ,
                                   std::size_t n , Limb * w , std::size_t fft ,
                                   std::size_t toom )
{
 const bool sq = a == b;
 const std::size_t h = n / 2 , l = n - h;   // a = a1 b^l + a0, the same b
 mulN( r , a , b , l , w , fft , toom );    // z0 = a0 b0
 mulN( r + 2 * l , a + l , b + l , h , w , fft , toom );   // z2 = a1 b1
 Limb * da = w , * db = w + l , * t = w + 2 * l;
 bool neg = absDiff( da , a , l , a + l , h );   // t = |a0 - a1| |b0 - b1|
 if( sq ) neg = false;
 else neg = neg != absDiff( db , b , l , b + l , h );
 mulN( t , da , sq ? da : db , l , w + 4 * l , fft , toom );
 // r += b^l ( z0 + z2 - ( a0 - a1 ) ( b0 - b1 ) ): with zi = zi0 + b^l zi1,
 // the sum S = z01 + z20 goes to b^l with z00 and to b^{2l} with z21
 Limb * x = r + l , * y = r + 2 * l;
#if ADS_BIGINT_ASM
 if( 2 * h > l ) {                          // z21 not empty
  Limb cH;
  const std::int64_t e =
   neg ? detail::karaSumAsm< false >( r , t , l , h , cH )
       : detail::karaSumAsm< true >( r , t , l , h , cH );
  if( cH ) addTo( r + 2 * l , 2 * n - 2 * l , &cH , 1 );
  if( e > 0 ) {
   const Limb u = Limb( e );
   addTo( r + 3 * l , 2 * n - 3 * l , &u , 1 );
   }
  else if( e < 0 ) {
   const Limb u = 1;
   subFrom( r + 3 * l , 2 * n - 3 * l , &u , 1 );
   }
  return;
  }
#endif
 const Limb c = addN( y , x , l , y , l );  // S
 const Limb cx = addN( x , y , l , r , l ) + c ,
            cy = addTo( y , l , r + 3 * l , 2 * h - l ) + c;
 addTo( r + 2 * l , 2 * n - 2 * l , &cx , 1 );
 addTo( r + 3 * l , 2 * n - 3 * l , &cy , 1 );
 if( neg ) addTo( x , 2 * n - l , t , 2 * l );
 else subFrom( x , 2 * n - l , t , 2 * l );
 }

inline BigInteger::Limb BigInteger::addLsh1( Limb * r , const Limb * a ,
                                             const Limb * b , std::size_t n )
{
#if ADS_BIGINT_ASM
 return( detail::addLsh1Asm( r , a , b , n ) );
#else
 Limb c = 0 , s = 0;                        // the carry, the bit of b out
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb x = b[ i ];
  r[ i ] = detail::addCarry( a[ i ] , ( x << 1 ) | s , c );
  s = x >> 63;
  }
 return( c + s );
#endif
 }

inline BigInteger::Limb BigInteger::subLsh1( Limb * r , const Limb * a ,
                                             const Limb * b , std::size_t n )
{
#if ADS_BIGINT_ASM
 return( detail::subLsh1Asm( r , a , b , n ) );
#else
 Limb c = 0 , s = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb x = b[ i ];
  r[ i ] = detail::subBorrow( a[ i ] , ( x << 1 ) | s , c );
  s = x >> 63;
  }
 return( c + s );
#endif
 }

inline void BigInteger::rsh1Add( Limb * r , const Limb * a , const Limb * b ,
                                 std::size_t n )
{
#if ADS_BIGINT_ASM
 detail::rsh1AddAsm( r , a , b , n );
#else
 Limb c = 0 , y = detail::addCarry( a[ 0 ] , b[ 0 ] , c );
 for( std::size_t i = 1 ; i < n ; ++i ) {
  const Limb x = detail::addCarry( a[ i ] , b[ i ] , c );
  r[ i - 1 ] = ( y >> 1 ) | ( x << 63 );
  y = x;
  }
 r[ n - 1 ] = ( y >> 1 ) | ( c << 63 );
#endif
 }

inline void BigInteger::rsh1Sub( Limb * r , const Limb * a , const Limb * b ,
                                 std::size_t n )
{
#if ADS_BIGINT_ASM
 detail::rsh1SubAsm( r , a , b , n );
#else
 Limb c = 0 , y = detail::subBorrow( a[ 0 ] , b[ 0 ] , c );
 for( std::size_t i = 1 ; i < n ; ++i ) {
  const Limb x = detail::subBorrow( a[ i ] , b[ i ] , c );
  r[ i - 1 ] = ( y >> 1 ) | ( x << 63 );
  y = x;
  }
 r[ n - 1 ] = ( y >> 1 ) | ( c << 63 );
#endif
 }

inline void BigInteger::divExact3( Limb * a , std::size_t n )
{
#if ADS_BIGINT_ASM
 detail::divExact3Asm( a , a , n );
#else
 // with d = ( b - 1 ) / 3, -d = 1 / 3 modulo b, and a / 3 is
 // -a d ( 1 + b + ... + b^{n-1} ) modulo b^n, a running difference
 constexpr Limb d = 0x5555555555555555ULL;
 Limb h = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  Limb p1;
  const Limb p0 = detail::mulWide( a[ i ] , d , p1 ) , c = h < p0;
  h -= p0;
  a[ i ] = h;
  h -= p1 + c;
  }
#endif
 }

inline void BigInteger::toom3( Limb * r , const Limb * a , const Limb * b ,
                               std::size_t n , Limb * w , std::size_t fft ,
                               std::size_t toom )
{
 // a = a2 x^2 + a1 x + a0, x = b^k, a0 and a1 of k digits, a2 of s; the
 // same for b; the product r( x ) = r4 x^4 + ... + r0 from its values at
 // 0, 1, -1, 2 and infinity (Toom; Bodrato's sequence of interpolation)
 const bool sq = a == b;
 const std::size_t k = ( n + 2 ) / 3 , s = n - 2 * k , K = k + 1 , L = 2 * K;
 Limb * const ap = w , * const am = w + K , * const bp = w + 2 * K ,
      * const bm = w + 3 * K , * const a2 = w + 4 * K ,
      * const b2 = w + 5 * K ,
      * const vm1 = w + 6 * K , * const v2 = vm1 + L , * const v1 = v2 + L ,
      * const ws = v1 + L;
 // the values of a and b at 1, -1 and 2, of K digits: a0 + a2 +- a1 and
 // a0 + 2 ( a1 + 2 a2 ), the sign of the value at -1
 auto eval = [ & ]( const Limb * x , Limb * p , Limb * m , Limb * t ) {
  p[ k ] = addN( p , x , k , x + 2 * k , s );
  const bool neg = absDiff( m , p , K , x + k , k );
  addTo( p , K , x + k , k );
  Limb c = addLsh1( t , x + k , x + 2 * k , s );
  t[ k ] = s < k ? addN( t + s , x + k + s , k - s , &c , 1 ) : c;
  c = addLsh1( t , x , t , k );
  t[ k ] = 2 * t[ k ] + c;
  return( neg );
  };
 bool neg = eval( a , ap , am , a2 );
 if( ! sq ) neg = neg != eval( b , bp , bm , b2 );
 else neg = false;
 mulN( vm1 , am , sq ? am : bm , K , ws , fft , toom );
 mulN( v2 , a2 , sq ? a2 : b2 , K , ws , fft , toom );
 mulN( v1 , ap , sq ? ap : bp , K , ws , fft , toom );
 mulN( r , a , b , k , ws , fft , toom );   // v0
 mulN( r + 4 * k , a + 2 * k , b + 2 * k , s , ws , fft , toom );   // vinf
 const Limb * const v0 = r , * const vi = r + 4 * k;
 // r3 = ( ( v2 - vm1 ) / 3 - ( v1 - v0 ) ) / 2 - 2 vinf,
 // r1 = ( v1 - vm1 ) / 2 - r3, r2 = v1 - v0 - r1 - r3 - vinf
 if( neg ) addTo( v2 , L , vm1 , L );
 else subFrom( v2 , L , vm1 , L );
 divExact3( v2 , L );
 if( neg ) rsh1Add( vm1 , v1 , vm1 , L );
 else rsh1Sub( vm1 , v1 , vm1 , L );
 subFrom( v1 , L , v0 , 2 * k );
 rsh1Sub( v2 , v2 , v1 , L );
 subFrom( v1 , L , vm1 , L );
 subFrom( v1 , L , vi , 2 * s );
 const Limb c = subLsh1( v2 , v2 , vi , 2 * s );
 subFrom( v2 + 2 * s , L - 2 * s , &c , 1 );
 subFrom( vm1 , L , v2 , L );
 // r = vinf x^4 + r3 x^3 + r2 x^2 + r1 x + v0, each r_i below b^{2n - ik}
 std::copy( v1 , v1 + 2 * k , r + 2 * k );
 addTo( r + 4 * k , 2 * s , v1 + 2 * k , std::min( L - 2 * k , 2 * s ) );
 addTo( r + k , 2 * n - k , vm1 , std::min( L , 2 * n - k ) );
 addTo( r + 3 * k , 2 * n - 3 * k , v2 , std::min( L , 2 * n - 3 * k ) );
 }

template< class O >
inline void BigInteger::mulNTT3( Limb * r , const Limb * a , std::size_t na ,
                                 const Limb * b , std::size_t nb ,
                                 std::size_t n )
{
 using N1 = detail::NTT< 2013265921u , 31 >;   // 15 2^27 + 1
 using N2 = detail::NTT< 1811939329u , 13 >;   // 27 2^26 + 1
 using N3 = detail::NTT< 469762049u , 3 >;     // 7 2^26 + 1
 using V = typename O::V;
 constexpr std::uint64_t p1 = 2013265921u , p2 = 1811939329u ,
                         p3 = 469762049u;
 // a coefficient, at most 2^25 products of two pieces below 2^32, stays
 // below p1 p2 p3 > 2^90: p1 p2 > 2^26 ( 2^63 / p3 ) says so in 64 bits
 static_assert( ( p1 * p2 ) >> 26 > ( std::uint64_t( 1 ) << 63 ) / p3 );
 const std::size_t m = 2 * ( na + nb ) - 1;
 const bool sq = ( a == b ) && ( na == nb );
 std::vector< std::uint32_t , detail::NoInit< std::uint32_t > > r1( n ) ,
                                     r2( n ) , r3( n ) , w( sq ? 0 : n );
 std::uint32_t * fb = sq ? nullptr : w.data();
 N1::template convolve< O >( r1.data() , fb , a , na , b , nb , n );
 N2::template convolve< O >( r2.data() , fb , a , na , b , nb , n );
 N3::template convolve< O >( r3.data() , fb , a , na , b , nb , n );
 // Garner: x = y1 + p1 y2 + p1 p2 y3 < p1 p2 p3, y2 and y3 by the
 // products of Shoup with the constants, in place of the residues
 constexpr std::uint32_t i12 = N2::power( std::uint32_t( p1 - p2 ) ,
                                          p2 - 2 ) ,
                         p1m3 = std::uint32_t( p1 % p3 ) ,
                         i123 = N3::power( std::uint32_t( p1 * p2 % p3 ) ,
                                           p3 - 2 );
 const V P2 = O::set( std::uint32_t( p2 ) ) ,
         P3 = O::set( std::uint32_t( p3 ) ) ,
         I12 = O::set( i12 ) , Q12 = O::set( N2::shoup( i12 ) ) ,
         M3 = O::set( p1m3 ) , QM3 = O::set( N3::shoup( p1m3 ) ) ,
         I123 = O::set( i123 ) , Q123 = O::set( N3::shoup( i123 ) ) ,
         one = O::set( 1 ) , Q3 = O::set( N3::kQ1 ) , zero = O::set( 0 );
 for( std::size_t i = 0 ; i < n ; i += O::W ) {
  const V y1 = O::load( r1.data() + i ) ,
          y2 = O::mulShoup( O::sub( O::load( r2.data() + i ) ,
                                    O::add( y1 , zero , P2 ) , P2 ) ,
                            I12 , Q12 , P2 ) ,
          s = O::add( O::mulShoup( y1 , one , Q3 , P3 ) ,
                      O::mulShoup( y2 , M3 , QM3 , P3 ) , P3 );
  O::store( r2.data() + i , y2 );
  O::store( r3.data() + i , O::mulShoup( O::sub( O::load( r3.data() + i ) ,
                                                 s , P3 ) ,
                                         I123 , Q123 , P3 ) );
  }
 // the pieces of 32 bits of the product, two per limb
 const std::uint64_t p12 = p1 * p2;
#if ADS_BIGINT_WIDE == 128
 // the coefficients 2 j and 2 j + 1, below 2^91, at the bits 64 j and
 // 64 j + 32, and the carry, below 2^60
 using U = detail::UInt128;
 auto x = [ & ]( std::size_t i ) {
  return( i < m ? U( p12 ) * r3[ i ] + ( r1[ i ] + p1 * r2[ i ] ) : U( 0 ) );
  };
 U c = 0;
 for( std::size_t j = 0 ; j < na + nb ; ++j ) {
  c += x( 2 * j ) + ( x( 2 * j + 1 ) << 32 );
  r[ j ] = Limb( c );
  c >>= 64;
  }
#else
 const std::uint64_t p12lo = p12 & 0xFFFFFFFFULL , p12hi = p12 >> 32;
 std::uint64_t c0 = 0 , c1 = 0 , c2 = 0;    // the carry, three pieces
 for( std::size_t i = 0 ; i < 2 * ( na + nb ) ; ++i ) {
  std::uint64_t x0 = 0 , x1 = 0 , x2 = 0;
  if( i < m ) {
   const std::uint64_t y1 = r1[ i ] , y2 = r2[ i ] , y3 = r3[ i ] ,
                       lo = y1 + p1 * y2 , tl = p12lo * y3 ,
                       th = p12hi * y3 ,
                       s0 = ( lo & 0xFFFFFFFFULL ) + ( tl & 0xFFFFFFFFULL ) ,
                       s1 = ( lo >> 32 ) + ( tl >> 32 ) +
                            ( th & 0xFFFFFFFFULL ) + ( s0 >> 32 );
   x0 = s0 & 0xFFFFFFFFULL;
   x1 = s1 & 0xFFFFFFFFULL;
   x2 = ( th >> 32 ) + ( s1 >> 32 );
   }
  const std::uint64_t v0 = x0 + c0 , v1 = x1 + c1 + ( v0 >> 32 ) ,
                      v2 = x2 + c2 + ( v1 >> 32 );
  if( i & 1 ) r[ i / 2 ] |= ( v0 & 0xFFFFFFFFULL ) << 32;
  else r[ i / 2 ] = v0 & 0xFFFFFFFFULL;
  c0 = v1 & 0xFFFFFFFFULL;
  c1 = v2 & 0xFFFFFFFFULL;
  c2 = v2 >> 32;
  }
#endif
 }

#if ADS_BIGINT_FNTT
template< class O >
inline void BigInteger::mulFNTT( Limb * r , const Limb * a , std::size_t na ,
                                 const Limb * b , std::size_t nb ,
                                 std::size_t n , bool cyclic )
{
 constexpr std::uint64_t p1 = 1125844072267777ULL ,   // 1048524 2^30 + 1
                         p2 = 1125818302464001ULL ,   // 1048500 2^30 + 1
                         p3 = 1125769984081921ULL;    // 1048455 2^30 + 1
 using N1 = detail::FNTT< p1 , 5 >;
 using N2 = detail::FNTT< p2 , 7 >;
 using N3 = detail::FNTT< p3 , 7 >;
 using V = typename O::V;
 using U = detail::UInt128;
 // a coefficient, at most 2^21 products of two digits, stays below
 // 2^149 < p1 p2 p3
 static_assert( ( U( p1 ) * p2 >> 98 ) * p3 > ( std::uint64_t( 1 ) << 51 ) );
 const bool sq = ( a == b ) && ( na == nb );
 // the scratch, kept per thread up to transforms of 2^17 terms, so that
 // its pages are not mapped again at every product
 using Buffer = std::vector< double , detail::NoInit< double > >;
 thread_local Buffer kept;
 Buffer own;
 const std::size_t need = 4 * n + 2 * ( na + nb );
 Buffer & B = n <= ( std::size_t( 1 ) << 17 ) ? kept : own;
 if( B.size() < need ) B.resize( need );
 double * const r1 = B.data() , * const r2 = r1 + n , * const r3 = r2 + n ,
        * const ah = r3 + n + n , * const al = ah + na , * const bh = al + na ,
        * const bl = bh + nb;
 for( std::size_t i = 0 ; i < na ; ++i ) {
  ah[ i ] = double( a[ i ] >> 32 );
  al[ i ] = double( a[ i ] & 0xFFFFFFFFULL );
  }
 for( std::size_t i = 0 ; i < nb ; ++i ) {
  bh[ i ] = double( b[ i ] >> 32 );
  bl[ i ] = double( b[ i ] & 0xFFFFFFFFULL );
  }
 double * fb = sq ? nullptr : r3 + n;
 N1::template convolve< O >( r1 , fb , ah , al , na , bh , bl , nb ,
                             n );
 N2::template convolve< O >( r2 , fb , ah , al , na , bh , bl , nb ,
                             n );
 N3::template convolve< O >( r3 , fb , ah , al , na , bh , bl , nb ,
                             n );
 // Garner: x = y1 + p1 t2 + p1 p2 t3 < p1 p2 p3, with
 // t2 = ( y2 - y1 ) / p1 modulo p2 and
 // t3 = ( y3 - y1 - p1 t2 ) / ( p1 p2 ) modulo p3, in place of y2 and y3
 constexpr double i12 = N2::centered( N2::power( p1 % p2 , p2 - 2 ) ) ,
                  c13 = N3::centered( p1 % p3 ) ,
                  i3 = N3::centered( N3::power( N3::mul( p1 % p3 , p2 % p3 ) ,
                                                p3 - 2 ) );
 const V P1 = O::set( double( p1 ) ) , P2 = O::set( double( p2 ) ) ,
         P3 = O::set( double( p3 ) ) , I12 = O::set( i12 ) ,
         C13 = O::set( c13 ) , I3 = O::set( i3 );
 for( std::size_t i = 0 ; i < n ; i += O::W ) {
  const V y1 = O::pos( N1::template reduce< O >( O::load( r1 + i ) ) ,
                      P1 ) ,
          t2 = O::pos( N2::template mulmod< O >( O::sub(
                         N2::template reduce< O >( O::load( r2 + i ) ) ,
                         y1 ) , I12 ) , P2 ) ,
          s = N3::template reduce< O >( O::sub( O::load( r3 + i ) ,
                                     N3::template mulmod< O >( t2 , C13 ) ) );
  O::store( r1 + i , y1 );
  O::store( r2 + i , t2 );
  O::store( r3 + i , O::pos( N3::template mulmod< O >(
                                       O::sub( s , y1 ) , I3 ) , P3 ) );
  }
 // the coefficient i, at the bit 64 i, is y1 + p1 t2 + p1 p2 t3, each of
 // the three below 2^50 and so one digit: the product is the sum of the
 // numbers Y1, p1 T2 and p1 p2 T3 of those digits, by the products by one
 // digit, the second digit of p1 p2 one place up; the m coefficients give
 // m + 2 digits, the top two 0 unless cyclic, where they wrap to the bottom
 const U p12 = U( p1 ) * p2;
 const std::uint64_t p12lo = std::uint64_t( p12 ) ,
                     p12hi = std::uint64_t( p12 >> 64 );
 const std::size_t m = cyclic ? n : na + nb - 1;
 thread_local std::vector< Limb , detail::NoInit< Limb > > keptW;
 std::vector< Limb , detail::NoInit< Limb > > ownW;
 auto & W = m <= ( std::size_t( 1 ) << 17 ) ? keptW : ownW;
 if( W.size() < 2 * m + 2 ) W.resize( 2 * m + 2 );
 Limb * const x = W.data() , * const t = x + m + 2;
 for( std::size_t i = 0 ; i < m ; ++i ) x[ i ] = std::uint64_t( r2[ i ] );
 x[ m ] = mul1( x , x , m , p1 );
 for( std::size_t i = 0 ; i < m ; ++i ) t[ i ] = std::uint64_t( r3[ i ] );
 const Limb c = addMul1( x , t , m , p12lo );
 x[ m ] += c;
 x[ m + 1 ] = Limb( x[ m ] < c ) + addMul1( x + 1 , t , m , p12hi );
 for( std::size_t i = 0 ; i < m ; ++i ) t[ i ] = std::uint64_t( r1[ i ] );
 addTo( x , m + 2 , t , m );
 if( ! cyclic ) {
  std::copy( x , x + m + 1 , r );
  return;
  }
 std::copy( x , x + n , r );                // x[ n , n + 2 ) b^n wraps
 for( Limb k = addTo( r , n , x + n , 2 ) ; k ; )
  k = addTo( r , n , &k , 1 );
 }

inline void BigInteger::mulCyclic( Limb * r , const Limb * a , std::size_t na ,
                                   const Limb * b , std::size_t nb ,
                                   std::size_t n )
{
 mulFNTT< detail::FLanes4 >( r , a , na , b , nb , n , true );
 }
#endif

inline void BigInteger::mulNTT( Limb * r , const Limb * a , std::size_t na ,
                                const Limb * b , std::size_t nb )
{
 if( ( na == 0 ) || ( nb == 0 ) ) {
  std::fill( r , r + na + nb , 0 );
  return;
  }
 ADS_PRE( na + nb <= kNTTMax );
 const std::size_t n = std::bit_ceil( 2 * ( na + nb ) - 1 );
#if ADS_BIGINT_FNTT
 // one coefficient per digit, the transforms half as long; the top t
 // digits of the longer factor by rows when the transform of the rest is
 // shorter by more than they cost: about 12 L log2( L ) cycles for a
 // transform of L terms, 1.6 per product of the rows
 if( na < nb ) {
  std::swap( a , b );
  std::swap( na , nb );
  }
 const std::size_t L = na + nb - 1 , nf0 = detail::length( L );
 auto cost = []( std::size_t m ) {
  return( 12.0 * double( m ) * double( std::bit_width( m ) ) );
  };
 std::size_t t = 0 , nf = nf0;
 double best = cost( nf0 );
 for( std::size_t k = 4 ; k < 23 ; ++k )    // the lengths 2^k and 3 2^{k-1}
  for( std::size_t m : { std::size_t( 1 ) << k ,
                         std::size_t( 3 ) << ( k - 1 ) } )
   if( ( m < nf0 ) && ( m + na / 8 >= L ) && ( m + 1 <= L ) ) {
    const double c = cost( m ) + 1.6 * double( L - m ) * double( nb );
    if( c < best ) {
     best = c;
     t = L - m;
     nf = m;
     }
    }
 if( ( nf >= 16 ) && ( nf <= ( std::size_t( 1 ) << 22 ) ) &&
     ( nb <= ( std::size_t( 1 ) << 21 ) ) ) {
  mulFNTT< detail::FLanes4 >( r , a , na - t , b , nb , nf );
  std::fill( r + na - t + nb , r + na + nb , 0 );
  for( std::size_t j = na - t ; j < na ; ++j )
   r[ j + nb ] = addMul1( r + j , b , nb , a[ j ] );
  return;
  }
#endif
#if ADS_BIGINT_SIMD >= 256
 if( n >= 2 * detail::Lanes8::W ) {
  mulNTT3< detail::Lanes8 >( r , a , na , b , nb , n );
  return;
  }
#endif
 mulNTT3< detail::Lanes1 >( r , a , na , b , nb , n );
 }

inline void BigInteger::mulP( Limb * r , const Limb * a , std::size_t na ,
                              const Limb * b , std::size_t nb ,
                              std::size_t fft , std::size_t toom )
{
 if( na < nb ) {
  std::swap( a , b );
  std::swap( na , nb );
  }
#if ADS_BIGINT_GMP
 // the lengths where GMP is faster, unless a method was asked by name
 // [see multiplyKaratsuba() and multiplyFFT()]
 if( ( fft == kFFT ) && ( nb > 0 ) ) {
  auto g = []( const Limb * x ) {
   return( reinterpret_cast< const mp_limb_t * >( x ) ); };
  mp_limb_t * const gr = reinterpret_cast< mp_limb_t * >( r );
  if( ( a == b ) && ( na == nb ) ) {
   if( nb < kGmpSqrTop ) {
    mpn_sqr( gr , g( a ) , mp_size_t( na ) );
    return;
    }
   }
  else if( ( nb >= kGmpMul ) &&
           ( nb < ( na < 2 * nb ? kGmpMulTop : kGmpMulLongTop ) ) ) {
   mpn_mul( gr , g( a ) , mp_size_t( na ) , g( b ) , mp_size_t( nb ) );
   return;
   }
  }
#endif
 if( nb < kKaratsuba ) {
  if( ( a == b ) && ( na == nb ) ) sqrSchool( r , a , na );
  else mulSchool( r , a , na , b , nb );
  return;
  }
 if( ( nb >= fft ) && ( na + nb <= kNTTMax ) ) {
  mulNTT( r , a , na , b , nb );
  return;
  }
 mulRec( r , a , na , b , nb , fft , toom );
 }

#if defined( __GNUC__ )
__attribute__(( noinline ))
#endif
inline void BigInteger::mulRec( Limb * r , const Limb * a , std::size_t na ,
                                const Limb * b , std::size_t nb ,
                                std::size_t fft , std::size_t toom )
{
 // the scratch on the stack up to 16 KiB
 constexpr std::size_t kStack = 2048;
 const std::size_t ns = mulScratch( nb );
 Limb in[ kStack ];
 Mag heap( ns > kStack ? ns : 0 );
 Limb * const w = ns > kStack ? heap.data() : in;
 if( na == nb ) {
  mulN( r , a , b , nb , w , fft , toom );
  return;
  }
 Mag t( 2 * nb );                           // a in pieces of nb digits
 std::fill( r , r + na + nb , 0 );
 for( std::size_t off = 0 ; off < na ; off += nb ) {
  const std::size_t len = std::min( nb , na - off );
  if( len == nb )
   mulN( t.data() , a + off , b , nb , w , fft , toom );
  else mulP( t.data() , b , nb , a + off , len , fft , toom );
  addTo( r + off , na + nb - off , t.data() , len + nb );
  }
 }

inline BigInteger::Mag BigInteger::mulMag( const Mag & a , const Mag & b ,
                                           std::size_t fft , std::size_t toom )
{
 if( a.empty() || b.empty() ) return( Mag() );
 Mag r( a.size() + b.size() );
 mulP( r.data() , a.data() , a.size() , b.data() , b.size() , fft , toom );
 trim( r );
 return( r );
 }

inline BigInteger BigInteger::multiplySchool( const BigInteger & a ,
                                              const BigInteger & b )
{
 Mag r( a.f_d.size() + b.f_d.size() );
 mulSchool( r.data() , a.f_d.data() , a.f_d.size() , b.f_d.data() ,
            b.f_d.size() );
 return( BigInteger( a.f_neg != b.f_neg , std::move( r ) ) );
 }

inline BigInteger BigInteger::multiplyKaratsuba( const BigInteger & a ,
                                                 const BigInteger & b )
{
 BigInteger r( a.f_neg != b.f_neg , mulMag( a.f_d , b.f_d ,
                                            std::size_t( -1 ) ,
                                            std::size_t( -1 ) ) );
 ADS_AUDIT( r == multiplySchool( a , b ) );
 return( r );
 }

inline BigInteger BigInteger::multiplyFFT( const BigInteger & a ,
                                           const BigInteger & b )
{
 Mag d( a.f_d.size() + b.f_d.size() );
 if( d.size() <= kNTTMax )
  mulNTT( d.data() , a.f_d.data() , a.f_d.size() , b.f_d.data() ,
          b.f_d.size() );
 else
  mulP( d.data() , a.f_d.data() , a.f_d.size() , b.f_d.data() ,
        b.f_d.size() , 0 );
 BigInteger r( a.f_neg != b.f_neg , std::move( d ) );
 ADS_AUDIT( r == multiplySchool( a , b ) );
 return( r );
 }

inline BigInteger operator*( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger( a.f_neg != b.f_neg , BigInteger::mulMag( a.f_d ,
                                                              b.f_d ) ) );
 }

inline void BigInteger::addMul( BigInteger & r , const BigInteger & a ,
                                 const BigInteger & b )
{
 const BigInteger & x = a.f_d.size() >= b.f_d.size() ? a : b ,
                  & y = a.f_d.size() >= b.f_d.size() ? b : a;
 if( ( y.f_d.size() != 1 ) || ( r.f_neg != ( a.f_neg != b.f_neg ) ) ||
     ( r.f_d.size() < x.f_d.size() ) ) {
  r += a * b;
  return;
  }
 const std::size_t n = x.f_d.size();       // r += x y[ 0 ], in place
 Limb c = addMul1( r.f_d.data() , x.f_d.data() , n , y.f_d[ 0 ] );
 if( c && ( r.f_d.size() > n ) )
  c = addTo( r.f_d.data() + n , r.f_d.size() - n , &c , 1 );
 if( c ) r.f_d.push_back( c );
 }

inline BigInteger & BigInteger::operator*=( const BigInteger & b )
{
 if( ( b.f_d.size() != 1 ) || f_d.empty() ) return( *this = *this * b );
 const Limb c = mul1( f_d.data() , f_d.data() , f_d.size() , b.f_d[ 0 ] );
 if( c ) f_d.push_back( c );                // by one digit, in place
 f_neg = f_neg != b.f_neg;
 return( *this );
 }

inline BigInteger BigInteger::pow( const BigInteger & x , unsigned long n )
{
 if( n == 0 ) return( 1 );
 BigInteger y = x;                          // x^( the leading bits of n )
 for( int i = int( std::bit_width( n ) ) - 1 ; i-- > 0 ; ) {
  y *= y;
  if( ( n >> i ) & 1 ) y *= x;
  }
 return( y );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ THE QUOTIENT ------------------------------*/
/*--------------------------------------------------------------------------*/

inline BigInteger::Limb BigInteger::divBase( Limb * a , std::size_t m ,
                                             const Limb * b , std::size_t n ,
                                             Limb * q , bool cut )
{
 Limb top = 0;
 if( cmpN( a + m , b , n ) >= 0 ) {
  subFrom( a + m , n , b , n );
  top = 1;
  }
 const Limb d1 = b[ n - 1 ] , d0 = b[ n - 2 ] ,
            v = detail::reciprocal2( d1 , d0 );
#if ADS_BIGINT_ASM
 // the digits of the quotient 4 at a time: the 4 digits Q of the window
 // a[ j - 4 , j + n ), below b b^4, from its top t + 4 digits by the top
 // t digits of b, t = 6: no smaller than Q, and larger by 1 at most
 // (by 1 + Q / b^{t-1} < 2), then the window less Q b by a pass of 4 rows
 // [see subPass4()], and b added back while it is negative; if cut, the
 // window and b cut to the L = j + 1 leading digits that the digit j - 1
 // takes [see below], Q of b^4 or more for the cut b sending back 2
 for( ; m >= 4 ; m -= 4 ) {
  constexpr std::size_t t = 6;
  const std::size_t L = cut ? std::min( n , m + 1 ) : n , o = n - L;
  if( L < kDivBlock ) break;
  Limb * const x = a + m - 4 + o;           // the window
  const Limb * const y = b + o;
  Limb w[ t + 4 ] , Q[ 4 ] , T[ 4 ] , S[ 4 ];
  const Limb * const D = b + n - t;
  std::copy( x + L - t , x + L + 4 , w );
  bool full = cmpN( w + 4 , D , t ) >= 0;   // then Q = b^4 - 1
  for( std::size_t k = 4 ; ! full && k-- > 0 ; ) {
   Limb * const z = w + k;                  // Algorithm D on t digits
   const Limb u2 = z[ t ] , u1 = z[ t - 1 ] , u0 = z[ t - 2 ];
   Limb qh;
   if( ( u2 == d1 ) && ( u1 == d0 ) ) {     // b - 1, rarely
    qh = ~Limb( 0 );
    if( subMul1( z , D , t , qh ) > u2 ) {
     --qh;
     addN( z , z , t , D , t );
     }
    }
   else {
    Limb r1 , r0;
    qh = detail::div3by2( u2 , u1 , u0 , d1 , d0 , v , r1 , r0 );
    const Limb c = subMul1( z , D , t - 2 , qh ) , c1 = r0 < c ,
               c2 = r1 < c1;
    z[ t - 2 ] = r0 - c;
    z[ t - 1 ] = r1 - c1;
    if( c2 ) {
     --qh;
     addN( z , z , t , D , t );
     }
    }
   z[ t ] = 0;
   Q[ k ] = qh;
   }
  if( full ) {
   if( cut ) return( 2 );
   std::fill( Q , Q + 4 , ~Limb( 0 ) );
   }
  std::copy( x + L , x + L + 4 , T );
  detail::subPass4( x , y , L , Q );
  std::copy( x + L , x + L + 4 , S );
  // the top D = T - S of the window less Q b, -1 or 0 since the window
  // is in [ -b , b ): S - T is 0 or 1
  const Limb br = subN( S , S , 4 , T , 4 ) , one = 1;
  ADS_INVARIANT( ! br && ! S[ 1 ] && ! S[ 2 ] && ! S[ 3 ] && ( S[ 0 ] < 2 ) );
  for( Limb d = S[ 0 ] ; d ; ) {            // b back, rarely
   subFrom( Q , 4 , &one , 1 );
   d -= addTo( x , L , y , L );
   }
  std::fill( x + L , x + L + 4 , 0 );
  std::copy( Q , Q + 4 , q + m - 4 );
  }
#endif
 for( std::size_t j = m ; j-- > 0 ; ) {     // D2-D7
  // the digits of b at work, all, or the j + 2 leading ones if cut
  const std::size_t L = cut ? std::min( n , j + 2 ) : n , o = n - L;
  Limb * const x = a + j + o;
  const Limb * const y = b + o;
  const Limb u2 = a[ j + n ] , u1 = a[ j + n - 1 ] , u0 = a[ j + n - 2 ];
  Limb qhat;
  if( ( u2 == d1 ) && ( u1 == d0 ) ) {      // qhat = b - 1, rarely
   qhat = ~Limb( 0 );
   const Limb c = subMul1( x , y , L , qhat );
   if( u2 < c ) {                           // D6: add back
    --qhat;
    addN( x , x , L , y , L );
    }
   else if( cut && ( ( u2 > c ) || ( cmpN( x , y , L ) >= 0 ) ) )
    return( 2 );                            // the cut digit is b or more
   }
  else {
   // D3: qhat from the three leading digits by the two of b, exact
   // for them and at most 1 too large for all, then D4 on the L - 2
   // other digits
   Limb r1 , r0;
   qhat = detail::div3by2( u2 , u1 , u0 , d1 , d0 , v , r1 , r0 );
   const Limb c = subMul1( x , y , L - 2 , qhat ) ,
              c1 = r0 < c , c2 = r1 < c1;
   a[ j + n - 2 ] = r0 - c;
   a[ j + n - 1 ] = r1 - c1;
   if( c2 ) {                               // D6: add back, rarely
    --qhat;
    addN( x , x , L , y , L );
    }
   }
  a[ j + n ] = 0;
  q[ j ] = qhat;
  }
 return( top );
 }

inline BigInteger::Limb BigInteger::divRec( Limb * a , std::size_t m ,
                                            const Limb * b , std::size_t n ,
                                            Limb * q , Limb * t )
{
 if( m < kDivide ) return( divBase( a , m , b , n , q ) );
 const Limb one = 1;
 const std::size_t k = m / 2;
 // the upper half Q1 of a / b^{2k} by b / b^k, then A' = R1 b^{2k} +
 // ( a mod b^{2k} ) - Q1 B0 b^k in a[ 0 , n + k ), its borrow in hi
 int qh = int( divRec( a + 2 * k , m - k , b + k , n - k , q + k , t ) );
 mulP( t , q + k , m - k , b , k );
 std::int64_t hi = -std::int64_t( subFrom( a + k , n , t , m ) );
 if( qh ) hi -= std::int64_t( subFrom( a + m , n + k - m , b , k ) );
 while( hi < 0 ) {                          // at most twice
  qh -= int( subFrom( q + k , m - k , &one , 1 ) );
  hi += std::int64_t( addTo( a + k , n , b , n ) );
  }
 // the lower half Q0 of A' / b^k, then A'' = R0 b^k + ( A' mod b^k ) -
 // Q0 B0 in a[ 0 , n )
 int ql = int( divRec( a + k , k , b + k , n - k , q , t ) );
 mulP( t , q , k , b , k );
 hi = -std::int64_t( subFrom( a , n , t , 2 * k ) );
 if( ql ) hi -= std::int64_t( subFrom( a + k , n - k , b , k ) );
 while( hi < 0 ) {
  ql -= int( subFrom( q , k , &one , 1 ) );
  hi += std::int64_t( addTo( a , n , b , n ) );
  }
 if( ql ) qh += int( addTo( q + k , m - k , &one , 1 ) );
 return( Limb( qh ) );
 }

inline void BigInteger::recip( Limb * x , const Limb * a , std::size_t n )
{
 if( n <= kRecip ) {                        // floor( ( b^{2n} - 1 ) / a )
  const Mag u( 2 * n , ~Limb( 0 ) ) , v( a , a + n );
  const Mag q = divModMag( u , v ).first;   // b^n + x
  std::copy( q.begin() , q.begin() + std::ptrdiff_t( n ) , x );
  return;
  }
 // the reciprocal Xh = b^h + xh of the top h digits, then
 // X = Xh b^l + Xh ( b^{n+h} - a Xh ) / b^{2h}, Newton's step
 const std::size_t l = ( n - 1 ) / 2 , h = n - l;
 Mag xh( h + 1 ) , U( 2 * h + 2 );
 recip( xh.data() , a + l , h );
 xh[ h ] = 1;
 // t = b^{n+h} - a Xh, below 2 a < b^{n+1} once Xh is not too large: of T
 // = a Xh only the low n + 1 digits matter, or T modulo b^N - 1 for any
 // N > n + 1, by a cyclic convolution of N terms where the NTT gives it
 const Limb one = 1;
 Mag t;
#if ADS_BIGINT_FNTT
 const std::size_t N = std::max( detail::length( n ) , std::size_t( 16 ) );
 if( ( n >= kFFT ) && ( N <= ( std::size_t( 1 ) << 21 ) ) ) {
  t.resize( N + 1 );
  mulCyclic( t.data() , a , n , xh.data() , h + 1 , N );
  // t' = b^e - T modulo b^N - 1, e = n + h modulo N: the complement of
  // T, b^N - 1 - T, plus b^e, the carry out of the top back to the bottom
  for( std::size_t i = 0 ; i < N ; ++i ) t[ i ] = ~t[ i ];
  const std::size_t e = ( n + h ) % N;
  for( Limb k = addTo( t.data() + e , N - e , &one , 1 ) ; k ; )
   k = addTo( t.data() , N , &k , 1 );
  // t is t', t' + b^N - 1 or, Xh too large, t' - b^N + 1 < 0; the three
  // differ in the lowest digit, that of t known as -a_0 xh_0 modulo b
  Limb t0 = Limb( 0 ) - a[ 0 ] * xh[ 0 ];
  for( ; ; ) {
   if( t[ 0 ] == t0 ) {
    t[ N ] = 0;
    break;
    }
   if( t[ 0 ] - 1 == t0 ) {                 // t' + b^N - 1
    subFrom( t.data() , N , &one , 1 );
    t[ N ] = 1;
    break;
    }
   subFrom( xh.data() , h + 1 , &one , 1 );   // Xh - 1, T - a, t + a
   for( Limb k = addTo( t.data() , N , a , n ) ; k ; )
    k = addTo( t.data() , N , &k , 1 );
   t0 += a[ 0 ];
   }
  }
 else
#endif
 {
  Mag T( n + h + 1 );
  mulP( T.data() , a , n , xh.data() , h + 1 );
  while( T[ n + h ] ) {                     // at most twice
   subFrom( xh.data() , h + 1 , &one , 1 );
   subFrom( T.data() , n + h + 1 , a , n );
   }
  t.assign( n + 1 , 0 );                    // b^{n+h} - T modulo b^{n+1}
  subN( t.data() , t.data() , n + 1 , T.data() , n + 1 );
  }
 // X = Xh b^l + Xh t / b^{2h}, of t its h + 1 digits from the place l
 mulP( U.data() , t.data() + l , h + 1 , xh.data() , h + 1 );
 Mag X( n + 1 , 0 );                        // b^n <= X < 2 b^n
 std::copy( xh.begin() , xh.end() , X.begin() + std::ptrdiff_t( l ) );
 addTo( X.data() , n + 1 , U.data() + 2 * h - l , l + 2 );
 std::copy( X.begin() , X.begin() + std::ptrdiff_t( n ) , x );
 }

inline BigInteger::Limb BigInteger::divNewton( Limb * a , std::size_t m ,
                                               const Limb * b , std::size_t n ,
                                               Limb * q , Mag * keep ,
                                               bool quot )
{
 const Limb one = 1;
 Limb top = 0;
 if( cmpN( a + m , b , n ) >= 0 ) {
  subFrom( a + m , n , b , n );
  top = 1;
  }
 // blocks of k digits of quotient, k about m / 2 for m near n
 std::size_t k = m;
 if( m > n ) k = ( m - 1 ) / ( ( m - 1 ) / n + 1 ) + 1;
 else if( 3 * m > n ) k = ( m + 1 ) / 2;
 // X = b^k + I below b^{2k} / ( D + 1 ), D the top k digits of b, so that
 // the quotients by it are never too large
 Mag J;
 if( ! keep || ( keep->size() < k ) ) {
  Mag w( b + n - k , b + n );
  J.resize( k );
  if( addTo( w.data() , k , &one , 1 ) ) std::fill( J.begin() , J.end() , 0 );
  else recip( J.data() , w.data() , k );
  if( keep ) *keep = std::move( J );
  }
 const Limb * I = keep ? keep->data() + keep->size() - k : J.data();
#if ADS_BIGINT_FNTT
 const std::size_t N = std::max( detail::length( n ) , std::size_t( 16 ) );
 const bool cyc = N <= ( std::size_t( 1 ) << 21 );
#else
 const std::size_t N = 0;
 const bool cyc = false;
#endif
 Mag p( 2 * k ) , c( cyc ? N : n + k );
 for( std::size_t j = m ; j > 0 ; ) {
  // the window a[ j - kk , j + n ) is the remainder times b^kk plus the
  // next kk digits; its top kk digits times X give the kk digits of
  // the quotient in q[ j - kk , j ), short by a few units at most
  const std::size_t kk = std::min( k , j );
  j -= kk;
  Limb * W = a + j , * qb = q + j;
  const Limb * R = W + n;
  mulP( p.data() , R , kk , I + k - kk , kk );
  addN( qb , p.data() + kk , kk , R , kk );
  // short by 7 at most: R / b exceeds the top digits of R times b^n / b
  // by 2, which exceeds them times b^k / ( D + 1 ) by 2, X misses it by 2
  // over b^k, and the quotient is rounded
  if( quot && ( j == 0 ) && ( qb[ 0 ] < ~Limb( 0 ) - 15 ) ) break;
  // W -= qb b, the result below a few b, so n + 1 digits
  if( cyc ) {                               // modulo b^N - 1
#if ADS_BIGINT_FNTT
   mulCyclic( c.data() , b , n , qb , kk , N );
#endif
   const std::size_t L = n + kk;
   Mag x( N , 0 );                          // W modulo b^N - 1
   std::copy( W , W + std::min( L , N ) , x.begin() );
   if( L > N )
    for( Limb e = addTo( x.data() , N , W + N , L - N ) ; e ;
         e = addTo( x.data() , N , &e , 1 ) ) {}
   if( subFrom( x.data() , N , c.data() , N ) )
    subFrom( x.data() , N , &one , 1 );
   if( std::all_of( x.begin() , x.end() ,
                    []( Limb y ) { return( ~y == 0 ); } ) )
    std::fill( x.begin() , x.end() , 0 );
   // W - qb b = x + e ( b^N - 1 ), e from the lowest digit
   const Limb e = x[ 0 ] - ( W[ 0 ] - qb[ 0 ] * b[ 0 ] );
   std::fill( W , W + L , 0 );
   std::copy( x.begin() , x.begin() + std::ptrdiff_t( std::min( N , L ) ) ,
              W );
   if( e ) {
    W[ N ] += e;
    subFrom( W , N + 1 , &e , 1 );
    }
   }
  else {
   mulP( c.data() , b , n , qb , kk );
   subFrom( W , n + kk , c.data() , n + kk );
   }
  while( W[ n ] || ( cmpN( W , b , n ) >= 0 ) ) {   // a few times
   addTo( qb , kk , &one , 1 );
   W[ n ] -= subFrom( W , n , b , n );
   }
  }
 return( top );
 }

#if defined( __GNUC__ )
__attribute__(( noinline ))
#endif
inline void BigInteger::divShort( const Mag & u , const Mag & v , Mag & q ,
                                  Mag * r )
{
 // Algorithm D on the stack: v and u shifted so that the top bit of v is
 // set, u with one digit more, the trial quotient of each digit by the
 // three leading digits of the remainder and the two of v (div3by2())
 const std::size_t n = v.size() , nu = u.size() , m = nu - n;
 const int s = std::countl_zero( v.back() );
 Limb vn[ kDivShort ] , un[ 2 * kDivShort + 1 ];
 if( s ) {
  for( std::size_t i = n ; --i > 0 ; )
   vn[ i ] = ( v[ i ] << s ) | ( v[ i - 1 ] >> ( 64 - s ) );
  vn[ 0 ] = v[ 0 ] << s;
  un[ nu ] = u[ nu - 1 ] >> ( 64 - s );
  for( std::size_t i = nu ; --i > 0 ; )
   un[ i ] = ( u[ i ] << s ) | ( u[ i - 1 ] >> ( 64 - s ) );
  un[ 0 ] = u[ 0 ] << s;
  }
 else {
  std::copy( v.begin() , v.end() , vn );
  std::copy( u.begin() , u.end() , un );
  un[ nu ] = 0;
  }
 const Limb d1 = vn[ n - 1 ] , d0 = vn[ n - 2 ] ,
            iv = detail::reciprocal2( d1 , d0 );
 q.resize( m + 1 );
 for( std::size_t j = m + 1 ; j-- > 0 ; ) { // D2-D7
  Limb * const x = un + j;
  const Limb u2 = x[ n ] , u1 = x[ n - 1 ] , u0 = x[ n - 2 ];
  Limb qhat;
  if( ( u2 == d1 ) && ( u1 == d0 ) ) {      // qhat = b - 1, rarely
   qhat = ~Limb( 0 );
   if( subMul1( x , vn , n , qhat ) > u2 ) {   // D6: add back
    --qhat;
    addN( x , x , n , vn , n );
    }
   }
  else {
   Limb r1 , r0;
   qhat = detail::div3by2( u2 , u1 , u0 , d1 , d0 , iv , r1 , r0 );
   Limb c = 0;
#if ADS_BIGINT_ASM
   if( n > 3 ) c = subMul1( x , vn , n - 2 , qhat );
   else
#endif
   for( std::size_t i = 0 ; i + 2 < n ; ++i ) {   // D4, inline
    Limb hi;
    const Limb lo = detail::mulAdd( vn[ i ] , qhat , c , 0 , hi ) ,
               y = x[ i ];
    x[ i ] = y - lo;
    c = hi + ( y < lo );
    }
   const Limb c1 = r0 < c , c2 = r1 < c1;
   x[ n - 2 ] = r0 - c;
   x[ n - 1 ] = r1 - c1;
   if( c2 ) {                               // D6: add back, rarely
    --qhat;
    addN( x , x , n , vn , n );
    }
   }
  x[ n ] = 0;
  q[ j ] = qhat;
  }
 trim( q );
 if( r ) {                                  // D8: unnormalize
  r->resize( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   ( *r )[ i ] = s ? ( un[ i ] >> s ) |
                     ( i + 1 < n ? un[ i + 1 ] << ( 64 - s ) : 0 ) : un[ i ];
  trim( *r );
  }
 }

inline std::pair< BigInteger::Mag , BigInteger::Mag > BigInteger::divModMag(
                                const Mag & u , const Mag & v , bool school ,
                                Mag * keep , bool rem )
{
 if( compareMag( u , v ) < 0 ) return( std::pair( Mag() , rem ? u : Mag() ) );
 if( v.size() == 1 ) {                      // the short division
  Mag q = u;
  const Limb r = divSmall( q , v[ 0 ] );
  return( std::pair( std::move( q ) , r ? Mag{ r } : Mag() ) );
  }
 if( ! rem && ! school && ( u.size() + 4 < 2 * v.size() ) ) {
  // the quotient alone, of m digits, from the top k = m + 2 digits of v
  // and the top m + k of u: with v = vt b^s + vl and u = ut b^s + ul, the
  // quotient qt of ut by vt is the quotient q of u by v or q + 1, since
  // ut / vt - ut / ( vt + 1 ) < 2 / b; it is q unless
  // rt b^s + ul < qt vl, below b^{|qt| + s}, so that rt of more digits than
  // qt says so with no product
  const std::size_t m = u.size() - v.size() + 1 , s = v.size() - m - 2;
  const Mag ut( u.data() + s , u.data() + u.size() ) ,
            vt( v.data() + s , v.data() + v.size() );
  auto [ q , rt ] = divModMag( ut , vt , false , nullptr , true );
  if( ( rt.size() <= q.size() ) && ! q.empty() ) {
   Mag x( s + rt.size() );                  // rt b^s + ul
   std::copy( u.data() , u.data() + s , x.data() );
   std::copy( rt.begin() , rt.end() , x.data() + s );
   trim( x );
   Mag vl( v.data() , v.data() + s );
   trim( vl );
   if( compareMag( x , mulMag( q , vl ) ) < 0 ) q = subMag( q , Mag{ 1 } );
   }
  return( std::pair( std::move( q ) , Mag() ) );
  }
 if( ! rem && ! school && ( v.size() >= kDivQuot ) &&
     ( v.size() < kNewton ) && ( u.size() + 1 >= 2 * v.size() ) ) {
  // the quotient alone in two halves: the high one, of m - k digits, with
  // its remainder r1, then the low one, of k digits, alone, the quotient
  // of r1 b^k + ( u mod b^k ) < v b^k by the top digits of v [see above]
  const std::size_t m = u.size() - v.size() + 1 , k = m / 2;
  auto [ q1 , r1 ] = divModMag( Mag( u.data() + k , u.data() + u.size() ) ,
                                v , false , nullptr , true );
  Mag w( k + r1.size() );
  std::copy( u.data() , u.data() + k , w.data() );
  std::copy( r1.begin() , r1.end() , w.data() + k );
  trim( w );
  const Mag q0 = divModMag( w , v , false , nullptr , false ).first;
  Mag q( k + q1.size() , 0 );
  std::copy( q0.begin() , q0.end() , q.data() );
  std::copy( q1.begin() , q1.end() , q.data() + k );
  trim( q );
  return( std::pair( std::move( q ) , Mag() ) );
  }
 if( ( v.size() < kDivShort ) && ( u.size() < 2 * kDivShort ) ) {
  std::pair< Mag , Mag > out;
  divShort( u , v , out.first , rem ? &out.second : nullptr );
  return( out );
  }
 const std::size_t n = v.size();
#if ADS_BIGINT_GMP
 // the lengths where GMP is faster, unless Algorithm D was asked or the
 // reciprocal is to be kept: mpn_tdiv_qr() with the remainder, else
 // mpz_tdiv_q(), which skips most of it
 if( ! school && ! keep && gmpDivides( u.size() , n , rem ) ) {
  if( ! rem ) {
   mpz_t U , V , Q;
   mpz_init2( Q , mp_bitcnt_t( 64 * ( u.size() - n + 1 ) ) );
   mpz_tdiv_q( Q , gmpView( U , u.data() , u.size() ) ,
               gmpView( V , v.data() , n ) );
   std::pair< Mag , Mag > out( gmpDigits( Q ) , Mag() );
   mpz_clear( Q );
   return( out );
   }
  Mag q( u.size() - n + 1 ) , r( n );
  mpn_tdiv_qr( reinterpret_cast< mp_limb_t * >( q.data() ) ,
               reinterpret_cast< mp_limb_t * >( r.data() ) , 0 ,
               reinterpret_cast< const mp_limb_t * >( u.data() ) ,
               mp_size_t( u.size() ) ,
               reinterpret_cast< const mp_limb_t * >( v.data() ) ,
               mp_size_t( n ) );
  trim( q );
  trim( r );
  return( std::pair( std::move( q ) , std::move( r ) ) );
  }
#endif
 std::size_t m = u.size() + 1 - n;
 const int s = std::countl_zero( v.back() );   // D1: normalize
 auto shift = [ s ]( Limb * r , const Limb * x , std::size_t nx ) {
  if( s == 0 ) {                            // the digit shifted out
   std::copy( x , x + nx , r );
   return( Limb( 0 ) );
   }
  const Limb out = x[ nx - 1 ] >> ( 64 - s );
  for( std::size_t i = nx ; i-- > 1 ; )
   r[ i ] = ( x[ i ] << s ) | ( x[ i - 1 ] >> ( 64 - s ) );
  r[ 0 ] = x[ 0 ] << s;
  return( out );
  };
 // the normalized operands on the stack when short, as the scratch of
 // the remainder, un with a digit 0 below for the quotient alone
 constexpr std::size_t kStack = 64;
 Limb in[ kStack ];
 const std::size_t nu = u.size() + 1;
 std::unique_ptr< Limb[] > heap( n + nu + 1 > kStack
                                 ? new Limb[ n + nu + 1 ] : nullptr );
 Limb * const vn = heap ? heap.get() : in , * const un = vn + n + 1;
 Mag q;
 q.reserve( m + 1 );
 q.resize( m );
 shift( vn , v.data() , n );
 un[ u.size() ] = shift( un , u.data() , u.size() );
 if( ! rem && ! school && ( n < kDivCut ) ) {
  // the quotient alone, by Algorithm D on un b with one digit more, the
  // divisor cut to the j + 2 leading digits for the digit j: the digits
  // cut make un b larger by less than ( m + 1 ) b^{n-1} < b^n / 2 <= v,
  // and the remainder at the end is below v + b^{n-2}, so the quotient
  // of un b is one off at most, and the last digit, the one more, says
  // when that can change the quotient of un; a digit of b or more, which
  // the cut divisor may give, sends to Algorithm D in full
  un[ -1 ] = 0;                             // the top of un is below b
  q.resize( m + 1 );
  if( divBase( un - 1 , m + 1 , vn , n , q.data() , true ) == 2 )
   return( divModMag( u , v , true , nullptr , false ) );
  const Limb e = q[ 0 ];                    // the digit more
  std::copy( q.data() + 1 , q.data() + m + 1 , q.data() );
  q.pop_back();
  trim( q );
  if( ( e > 1 ) && ( e < ~Limb( 0 ) - 1 ) )
   return( std::pair( std::move( q ) , Mag() ) );
  const Mag p = mulMag( q , v );           // q v against u, rarely
  if( compareMag( p , u ) > 0 ) q = subMag( q , Mag{ 1 } );
  else if( compareMag( subMag( u , p ) , v ) >= 0 ) q = addMag( q , Mag{ 1 } );
  return( std::pair( std::move( q ) , Mag() ) );
  }
 // with the top digit of un 0, the quotient is one digit shorter and its
 // top, 0 or 1, comes from the method
 const std::size_t k = m - ( ( un[ nu - 1 ] == 0 ) && ( m > 1 ) );
 Limb top = 0;
 if( school || ( k < kDivide ) ) top = divBase( un , k , vn , n , q.data() );
 else if( ( n >= kNewton ) && ! rem ) {
  // the quotient alone of un b, one digit more, whose last block keeps
  // the quotient by the reciprocal when its last digit is far from b - 1
  un[ -1 ] = 0;
  q.resize( k + 1 );
  top = divNewton( un - 1 , k + 1 , vn , n , q.data() , keep , true );
  std::copy( q.data() + 1 , q.data() + k + 1 , q.data() );
  q.resize( m );
  }
 else if( n >= kNewton ) top = divNewton( un , k , vn , n , q.data() , keep );
 else {                                     // by blocks of n digits
  Mag t( n );
  std::size_t j = k;
  for( ; j > n ; j -= n ) {
   const Limb c = divRec( un + j - n , n , vn , n , q.data() + j - n ,
                          t.data() );
   if( j == k ) top = c;
   }
  const Limb c = divRec( un , j , vn , n , q.data() , t.data() );
  if( j == k ) top = c;
  }
 if( k < m ) q[ k ] = top;
 trim( q );
 Mag r;                                     // D8: unnormalize
 if( rem ) {
  r.resize( n );
  for( std::size_t i = 0 ; i < n ; ++i )
   r[ i ] = s ? ( un[ i ] >> s ) |
                ( i + 1 < n ? un[ i + 1 ] << ( 64 - s ) : 0 ) : un[ i ];
  trim( r );
  }
 return( std::pair( std::move( q ) , std::move( r ) ) );
 }

inline std::pair< BigInteger , BigInteger > BigInteger::divMod(
                                const BigInteger & a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
 auto [ q , r ] = divModMag( a.f_d , b.f_d );
 std::pair< BigInteger , BigInteger > out( BigInteger( a.f_neg != b.f_neg ,
                                                       std::move( q ) ) ,
                                           BigInteger( a.f_neg ,
                                                       std::move( r ) ) );
 ADS_AUDIT( ( out.first * b + out.second == a ) &&
            ( compareMag( out.second.f_d , b.f_d ) < 0 ) &&
            ( ( out.second.sign() == 0 ) ||
              ( out.second.sign() == a.sign() ) ) );
 ADS_AUDIT( out == divModSchool( a , b ) );
 return( out );
 }

inline std::pair< BigInteger , BigInteger > BigInteger::divModSchool(
                                const BigInteger & a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
 auto [ q , r ] = divModMag( a.f_d , b.f_d , true );
 return( std::pair( BigInteger( a.f_neg != b.f_neg , std::move( q ) ) ,
                    BigInteger( a.f_neg , std::move( r ) ) ) );
 }

inline void BigInteger::divExactSmall( Mag & a , Limb d )
{
 const int s = std::countr_zero( d );      // a and d shifted, d odd
 const std::size_t n = a.size();
 Limb * const y = a.data();
 d >>= s;
 if( s )
  for( std::size_t i = 0 ; i < n ; ++i )
   y[ i ] = ( y[ i ] >> s ) | ( i + 1 < n ? y[ i + 1 ] << ( 64 - s ) : 0 );
 Limb v = d;                                // 1 / d modulo 2^64, by Newton
 for( int i = 0 ; i < 5 ; ++i ) v *= 2 - d * v;
 Limb c = 0;                                // the borrow of a - q d
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb x = y[ i ] , q = ( x - c ) * v;
  Limb hi;
  detail::mulWide( q , d , hi );
  c = hi + ( x < c );
  y[ i ] = q;
  }
 trim( a );
 }

inline BigInteger BigInteger::divExact( BigInteger a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
 if( b.f_d.size() > 1 ) {
#if ADS_BIGINT_GMP
  if( b.f_d.size() < kGmpExactTop ) {      // mpz_divexact(), by Jebelean
   mpz_t A , B , Q;
   mpz_init2( Q , mp_bitcnt_t( 64 * a.f_d.size() ) );
   mpz_divexact( Q , gmpView( A , a.f_d.data() , a.f_d.size() ) ,
                 gmpView( B , b.f_d.data() , b.f_d.size() ) );
   BigInteger q( a.f_neg != b.f_neg , gmpDigits( Q ) );
   mpz_clear( Q );
   ADS_AUDIT( q * b == a );
   return( q );
   }
#endif
  BigInteger q = a / b;
  ADS_AUDIT( q * b == a );
  return( q );
  }
 BigInteger a0;                             // a , for the audit
 if constexpr( ADS_CONTRACTS >= 3 ) a0 = a;
 const bool neg = a.f_neg != b.f_neg;       // in place
 divExactSmall( a.f_d , b.f_d[ 0 ] );
 a.f_neg = neg && ! a.f_d.empty();
 ADS_AUDIT( a * b == a0 );
 return( a );
 }

inline BigInteger operator/( const BigInteger & a , const BigInteger & b )
{
 ADS_PRE( b.sign() != 0 );
 BigInteger q( a.f_neg != b.f_neg , BigInteger::divModMag( a.f_d , b.f_d ,
                                                           false , nullptr ,
                                                           false ).first );
 ADS_AUDIT( q == BigInteger::divModSchool( a , b ).first );
 return( q );
 }

inline BigInteger operator%( const BigInteger & a , const BigInteger & b )
{
 return( BigInteger::divMod( a , b ).second );
 }

/*--------------------------------------------------------------------------*/
/*----------------------- THE GREATEST COMMON DIVISOR ----------------------*/
/*--------------------------------------------------------------------------*/

inline std::uint64_t BigInteger::gcdWord( std::uint64_t a , std::uint64_t b )
{
 if( a == 0 ) return( b );
 if( ( b == 0 ) || ( a == 1 ) ) return( a );
 if( b == 1 ) return( 1 );
 const int s = std::countr_zero( a | b );   // the binary gcd, 4.5.2B
 a >>= std::countr_zero( a );
 do {                                       // with no branch but the loop
  b >>= std::countr_zero( b );
  const std::uint64_t m = std::min( a , b );
  b = std::max( a , b ) - m;
  a = m;
  } while( b );
 return( a << s );
 }

inline void BigInteger::linComb( Mag & u , Mag & v , std::int64_t A ,
                                 std::int64_t B , std::int64_t C ,
                                 std::int64_t D )
{
 // each row x X - y Y with x , y >= 0: the two products by one digit and
 // their difference, which never borrows past the top, as X is u or v
 const bool s1 = B > 0 , s2 = D > 0;
 const Limb x1 = Limb( s1 ? B : A ) , y1 = Limb( s1 ? -A : -B ) ,
            x2 = Limb( s2 ? D : C ) , y2 = Limb( s2 ? -C : -D );
 const std::size_t n = u.size();
 v.resize( n , 0 );
 Limb cx1 = 0 , cy1 = 0 , b1 = 0 , cx2 = 0 , cy2 = 0 , b2 = 0 ,
      * const U = u.data() , * const W = v.data();
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const Limb ui = U[ i ] , vi = W[ i ] ,
             p1 = detail::mulAdd( s1 ? vi : ui , x1 , cx1 , 0 , cx1 ) ,
             q1 = detail::mulAdd( s1 ? ui : vi , y1 , cy1 , 0 , cy1 ) ,
             p2 = detail::mulAdd( s2 ? vi : ui , x2 , cx2 , 0 , cx2 ) ,
             q2 = detail::mulAdd( s2 ? ui : vi , y2 , cy2 , 0 , cy2 );
  U[ i ] = detail::subBorrow( p1 , q1 , b1 );
  W[ i ] = detail::subBorrow( p2 , q2 , b2 );
  }
 trim( u );
 trim( v );
 }

inline BigInteger::Mag BigInteger::gcdMag( Mag u , Mag v )
{
 if( compareMag( u , v ) < 0 ) std::swap( u , v );
 constexpr std::int64_t kCap = std::int64_t( 1 ) << 30;
 while( v.size() > 1 ) {                    // L1: the leading 62 bits
  const std::size_t sh = 64 * u.size() -
                         std::size_t( std::countl_zero( u.back() ) ) - 62;
  auto top = [ sh ]( const Mag & x ) {
   auto at = [ & ]( std::size_t i ) {
    return( i < x.size() ? x[ i ] : Limb( 0 ) ); };
   const std::size_t w = sh / 64 , o = sh % 64;
   return( std::int64_t( o ? ( at( w ) >> o ) | ( at( w + 1 ) << ( 64 - o ) )
                           : at( w ) ) );
   };
  std::int64_t uh = top( u ) , vh = top( v ) , A = 1 , B = 0 , C = 0 , D = 1;
  for( ; ; ) {                              // L2, L3: Euclid on 62 bits
   const std::int64_t x = uh + A , y = vh + C , z = vh + D;
   if( ( y <= 0 ) || ( z <= 0 ) ) break;
   const std::int64_t q = x / y;
   if( q >= kCap ) break;
   // q = ( uh + B ) / z, q z = q y + q ( D - C ) < 2^62 + 2^61
   const std::int64_t e = uh + B - q * z;
   if( ( e < 0 ) || ( e >= z ) ) break;
   const std::int64_t T = A - q * C , S = B - q * D;
   if( ( T <= -kCap ) || ( T >= kCap ) || ( S <= -kCap ) || ( S >= kCap ) )
    break;
   A = C;
   C = T;
   B = D;
   D = S;
   const std::int64_t W = uh - q * vh;
   uh = vh;
   vh = W;
   }
  if( B == 0 ) {                            // L4: one long division
   Mag r = divModMag( u , v ).second;
   u = std::move( v );
   v = std::move( r );
   }
  else linComb( u , v , A , B , C , D );    // L4: the cofactors
  }
 if( v.empty() ) return( u );
 return( Mag{ gcdLimb( u.data() , u.size() , v[ 0 ] ) } );
 }

inline BigInteger::Limb BigInteger::gcdLimb( const Limb * a , std::size_t n ,
                                             Limb d )
{
 // d = 2^t e, e odd: the gcd is that of a and e, times the 2^t that a
 // has; below kHensel digits, a b^{-n} modulo e by Hensel's division, which
 // has the same gcd with e and no division
 if( n == 1 ) return( gcdWord( a[ 0 ] , d ) );
 const int t = std::countr_zero( d );
 const Limb e = d >> t;
 Limb r;
 if( n >= kHensel ) r = modSmall( a , n , e );
 else {
  Limb v = e;                               // 1 / e modulo 2^64, by Newton
  for( int i = 0 ; i < 5 ; ++i ) v *= 2 - e * v;
  r = 0;                                    // a - q e = -r b^n, r <= e
  for( std::size_t i = 0 ; i < n ; ++i ) {
   Limb h;
   detail::mulWide( ( a[ i ] - r ) * v , e , h );
   r = h + ( a[ i ] < r );
   }
  }
 const int z = a[ 0 ] ? std::min( t , std::countr_zero( a[ 0 ] ) ) : t;
 return( gcdWord( r , e ) << z );           // the 2^t that a has
 }

inline BigInteger BigInteger::gcd( const BigInteger & a ,
                                   const BigInteger & b )
{
 const Mag & x = a.f_d.size() >= b.f_d.size() ? a.f_d : b.f_d;
 const Mag & y = a.f_d.size() >= b.f_d.size() ? b.f_d : a.f_d;
 BigInteger g;
#if ADS_BIGINT_GMP
 if( y.size() >= kGmpGcd ) {                // mpz_gcd(), subquadratic
  mpz_t X , Y , G;
  mpz_init2( G , mp_bitcnt_t( 64 * y.size() ) );
  mpz_gcd( G , gmpView( X , x.data() , x.size() ) ,
           gmpView( Y , y.data() , y.size() ) );
  g.f_d = gmpDigits( G );
  mpz_clear( G );
  ADS_AUDIT( g == gcdEuclid( a , b ) );
  return( g );
  }
#endif
 if( y.size() == 1 )                        // no copy of the longer one
  g.f_d = { gcdLimb( x.data() , x.size() , y[ 0 ] ) };
 else g.f_d = gcdMag( x , y );
 ADS_AUDIT( g == gcdEuclid( a , b ) );
 return( g );
 }

inline BigInteger BigInteger::gcdEuclid( BigInteger a , BigInteger b )
{
 a = a.abs();
 b = b.abs();
 while( b.sign() != 0 ) {
  BigInteger r = a % b;
  a = std::move( b );
  b = std::move( r );
  }
 return( a );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE BACKEND OF GMP --------------------------*/
/*--------------------------------------------------------------------------*/

#if ADS_BIGINT_GMP

static_assert( sizeof( mp_limb_t ) == sizeof( BigInteger::Limb ) ,
               "the limbs of GMP are the digits of BigInteger" );

inline mpz_srcptr BigInteger::gmpView( mpz_t z , const Limb * d ,
                                       std::size_t n )
{
 return( mpz_roinit_n( z , reinterpret_cast< const mp_limb_t * >( d ) ,
                       mp_size_t( n ) ) );
 }

inline BigInteger::Mag BigInteger::gmpDigits( mpz_srcptr z )
{
 const Limb * d = reinterpret_cast< const Limb * >( mpz_limbs_read( z ) );
 return( Mag( d , d + mpz_size( z ) ) );
 }

inline bool BigInteger::gmpDivides( std::size_t u , std::size_t v , bool rem )
{
 const std::size_t m = u - v;               // the quotient has m + 1 at most
 if( rem ) return( ( m < kGmpShort ) || ( v < kGmpDivTop ) );
 if( m > v ) return( v < kGmpDivTop );
 return( ( v >= kGmpQuot ) && ( ( v < kGmpQuotTop ) || ( 2 * m <= v ) ) );
 }

#endif

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File BigInteger.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
