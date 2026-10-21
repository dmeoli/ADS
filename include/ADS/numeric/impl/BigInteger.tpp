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
 const std::uint64_t m = 0 - std::uint64_t( r > q0 );   // with no branch,
 q1 += m;                                   // as it is taken half the time
 r += m & d;
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

/// the registers of the window turned by k, and a step on them
#define ADS_W0 w0 , w1 , w2 , w3 , w4
#define ADS_W1 w1 , w2 , w3 , w4 , w0
#define ADS_W2 w2 , w3 , w4 , w0 , w1
#define ADS_W3 w3 , w4 , w0 , w1 , w2
#define ADS_W4 w4 , w0 , w1 , w2 , w3
#define ADS_PSTEP_X( s , R , ... )                                           \
 ADS_PASS_STEP( s , R , ADS_PASS_STORE , __VA_ARGS__ )
#define ADS_PSTEP( s , R , W ) ADS_PSTEP_X( s , R , W )

/// the pass of N = 4 to 8 digits in a row, with no loop: the steps 0 to
/// N - 1 on the window turned by one register each, its last 4
/// registers the top
#define ADS_PASS_ROW( RADD , N , STEPS , T0 , T1 , T2 , T3 )                \
 __asm__ __volatile__(                                                       \
  "xor %k[w0], %k[w0]\n\t"                                                   \
  "xor %k[w1], %k[w1]\n\t"                                                   \
  "xor %k[w2], %k[w2]\n\t"                                                   \
  "xor %k[w3], %k[w3]\n\t"                                                   \
  "xor %k[w4], %k[w4]\n\t"                                                   \
  STEPS                                                                      \
  "mov %[" #T0 "], 8*" #N "(%[r])\n\t"                                       \
  "mov %[" #T1 "], 8*" #N "+8(%[r])\n\t"                                     \
  "mov %[" #T2 "], 8*" #N "+16(%[r])\n\t"                                    \
  "mov %[" #T3 "], 8*" #N "+24(%[r])"                                        \
  : [ w0 ] "=&r" ( w0 ) , [ w1 ] "=&r" ( w1 ) , [ w2 ] "=&r" ( w2 ) ,        \
    [ w3 ] "=&r" ( w3 ) , [ w4 ] "=&r" ( w4 ) , [ lo ] "=&r" ( lo ) ,       \
    [ hi ] "=&r" ( hi )                                                      \
  : [ a ] "r" ( a ) , [ r ] "r" ( r ) , [ b ] "r" ( b ) ,                   \
    [ z ] "r" ( std::uint64_t( 0 ) )                                         \
  : "rdx" , "cc" , "memory" )

#define ADS_ROW_RADD_1 \
 ADS_PSTEP( 0 , ADS_PASS_RADD , ADS_W0 )

#define ADS_ROW_RADD_2 ADS_ROW_RADD_1 \
 ADS_PSTEP( 1 , ADS_PASS_RADD , ADS_W1 )

#define ADS_ROW_RADD_3 ADS_ROW_RADD_2 \
 ADS_PSTEP( 2 , ADS_PASS_RADD , ADS_W2 )

#define ADS_ROW_RADD_4 ADS_ROW_RADD_3 \
 ADS_PSTEP( 3 , ADS_PASS_RADD , ADS_W3 )

#define ADS_ROW_RADD_5 ADS_ROW_RADD_4 \
 ADS_PSTEP( 4 , ADS_PASS_RADD , ADS_W4 )

#define ADS_ROW_RADD_6 ADS_ROW_RADD_5 \
 ADS_PSTEP( 5 , ADS_PASS_RADD , ADS_W0 )

#define ADS_ROW_RADD_7 ADS_ROW_RADD_6 \
 ADS_PSTEP( 6 , ADS_PASS_RADD , ADS_W1 )

#define ADS_ROW_RADD_8 ADS_ROW_RADD_7 \
 ADS_PSTEP( 7 , ADS_PASS_RADD , ADS_W2 )

#define ADS_ROW_RNONE_1 \
 ADS_PSTEP( 0 , ADS_PASS_RNONE , ADS_W0 )

#define ADS_ROW_RNONE_2 ADS_ROW_RNONE_1 \
 ADS_PSTEP( 1 , ADS_PASS_RNONE , ADS_W1 )

#define ADS_ROW_RNONE_3 ADS_ROW_RNONE_2 \
 ADS_PSTEP( 2 , ADS_PASS_RNONE , ADS_W2 )

#define ADS_ROW_RNONE_4 ADS_ROW_RNONE_3 \
 ADS_PSTEP( 3 , ADS_PASS_RNONE , ADS_W3 )

#define ADS_ROW_RNONE_5 ADS_ROW_RNONE_4 \
 ADS_PSTEP( 4 , ADS_PASS_RNONE , ADS_W4 )

#define ADS_ROW_RNONE_6 ADS_ROW_RNONE_5 \
 ADS_PSTEP( 5 , ADS_PASS_RNONE , ADS_W0 )

#define ADS_ROW_RNONE_7 ADS_ROW_RNONE_6 \
 ADS_PSTEP( 6 , ADS_PASS_RNONE , ADS_W1 )

#define ADS_ROW_RNONE_8 ADS_ROW_RNONE_7 \
 ADS_PSTEP( 7 , ADS_PASS_RNONE , ADS_W2 )

/// r[ 0 , N + 4 ) = a[ 0 , N ) b[ 0 , 4 ), added to r[ 0 , N ) if add, for
/// 4 <= N <= 8 [see mulPass4()]
template< bool add >
inline void mulPassRow( std::uint64_t * r , const std::uint64_t * a ,
                        std::size_t N , const std::uint64_t * b )
{
 std::uint64_t w0 , w1 , w2 , w3 , w4 , lo , hi;
 if constexpr( add ) {
  switch( N ) {
   case 4:
    ADS_PASS_ROW( ADS_PASS_RADD , 4 , ADS_ROW_RADD_4 ,
                  w4 , w0 , w1 , w2 );
    break;
   case 5:
    ADS_PASS_ROW( ADS_PASS_RADD , 5 , ADS_ROW_RADD_5 ,
                  w0 , w1 , w2 , w3 );
    break;
   case 6:
    ADS_PASS_ROW( ADS_PASS_RADD , 6 , ADS_ROW_RADD_6 ,
                  w1 , w2 , w3 , w4 );
    break;
   case 7:
    ADS_PASS_ROW( ADS_PASS_RADD , 7 , ADS_ROW_RADD_7 ,
                  w2 , w3 , w4 , w0 );
    break;
   default:
    ADS_PASS_ROW( ADS_PASS_RADD , 8 , ADS_ROW_RADD_8 ,
                  w3 , w4 , w0 , w1 );
    break;
   }
  }
 if constexpr( ! add ) {
  switch( N ) {
   case 4:
    ADS_PASS_ROW( ADS_PASS_RNONE , 4 , ADS_ROW_RNONE_4 ,
                  w4 , w0 , w1 , w2 );
    break;
   case 5:
    ADS_PASS_ROW( ADS_PASS_RNONE , 5 , ADS_ROW_RNONE_5 ,
                  w0 , w1 , w2 , w3 );
    break;
   case 6:
    ADS_PASS_ROW( ADS_PASS_RNONE , 6 , ADS_ROW_RNONE_6 ,
                  w1 , w2 , w3 , w4 );
    break;
   case 7:
    ADS_PASS_ROW( ADS_PASS_RNONE , 7 , ADS_ROW_RNONE_7 ,
                  w2 , w3 , w4 , w0 );
    break;
   default:
    ADS_PASS_ROW( ADS_PASS_RNONE , 8 , ADS_ROW_RNONE_8 ,
                  w3 , w4 , w0 , w1 );
    break;
   }
  }
 }

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

/// the digit s of the pass of 6 rows [see mulPass6()], the window
/// ( P0 , ... , P6 )
#define ADS_PASS6_STEP( s , RADD , STORE , P0 , P1 , P2 , P3 , P4 , P5 , P6 ) \
 "1" #s ":\n\t"                                                              \
 "mov " #s "*8(%[a]), %%rdx\n\t"                                             \
 "mov $0, %k[" #P6 "]\n\t"                                                   \
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
 "mulx 32(%[b]), %[lo], %[hi]\n\t"                                           \
 "adcx %[lo], %[" #P4 "]\n\t"                                                \
 "adox %[hi], %[" #P5 "]\n\t"                                                \
 "mulx 40(%[b]), %[lo], %[hi]\n\t"                                           \
 "adcx %[lo], %[" #P5 "]\n\t"                                                \
 "adox %[hi], %[" #P6 "]\n\t"                                                \
 "adcx %[z], %[" #P6 "]\n\t"                                                 \
 STORE( s , P0 )

/// the registers of the window of 7 turned by k, and a step on them
#define ADS_V0 w0 , w1 , w2 , w3 , w4 , w5 , w6
#define ADS_V1 w1 , w2 , w3 , w4 , w5 , w6 , w0
#define ADS_V2 w2 , w3 , w4 , w5 , w6 , w0 , w1
#define ADS_V3 w3 , w4 , w5 , w6 , w0 , w1 , w2
#define ADS_V4 w4 , w5 , w6 , w0 , w1 , w2 , w3
#define ADS_V5 w5 , w6 , w0 , w1 , w2 , w3 , w4
#define ADS_V6 w6 , w0 , w1 , w2 , w3 , w4 , w5
#define ADS_P6_X( s , R , S , ... ) ADS_PASS6_STEP( s , R , S , __VA_ARGS__ )
#define ADS_P6( s , R , S , V ) ADS_P6_X( s , R , S , V )

/// the pass of 6 rows, r_i added by RADD, the digits stored by STORE
#define ADS_PASS6( RADD , STORE )                                            \
 __asm__ __volatile__(                                                       \
  "xor %k[w0], %k[w0]\n\t"                                                   \
  "xor %k[w1], %k[w1]\n\t"                                                   \
  "xor %k[w2], %k[w2]\n\t"                                                   \
  "xor %k[w3], %k[w3]\n\t"                                                   \
  "xor %k[w4], %k[w4]\n\t"                                                   \
  "xor %k[w5], %k[w5]\n\t"                                                   \
  "lea 7f(%%rip), %[lo]\n\t"                                                 \
  "movslq (%[lo],%[c],4), %[hi]\n\t"                                         \
  "add %[lo], %[hi]\n\t"                                                     \
  "mov %[n7], %[c]\n\t"                                                      \
  "xor %k[lo], %k[lo]\n\t"                                                   \
  "xor %k[w6], %k[w6]\n\t"                                                   \
  "jmp *%[hi]\n\t"                                                           \
  ".p2align 2\n"                                                             \
  "7:\n\t"                                                                   \
  ".long 10f-7b, 11f-7b, 12f-7b, 13f-7b, 14f-7b, 15f-7b, 16f-7b\n\t"         \
  ".p2align 5\n"                                                             \
  "9:\n\t"                                                                   \
  ADS_P6( 0 , RADD , STORE , ADS_V0 )                                        \
  ADS_P6( 1 , RADD , STORE , ADS_V1 )                                        \
  ADS_P6( 2 , RADD , STORE , ADS_V2 )                                        \
  ADS_P6( 3 , RADD , STORE , ADS_V3 )                                        \
  ADS_P6( 4 , RADD , STORE , ADS_V4 )                                        \
  ADS_P6( 5 , RADD , STORE , ADS_V5 )                                        \
  ADS_P6( 6 , RADD , STORE , ADS_V6 )                                        \
  "lea 56(%[a]), %[a]\n\t"                                                   \
  "lea 56(%[r]), %[r]\n\t"                                                   \
  "dec %[c]\n\t"                                                             \
  "jnz 9b\n\t"                                                               \
  "mov %[w0], (%[r])\n\t"                                                    \
  "mov %[w1], 8(%[r])\n\t"                                                   \
  "mov %[w2], 16(%[r])\n\t"                                                  \
  "mov %[w3], 24(%[r])\n\t"                                                  \
  "mov %[w4], 32(%[r])\n\t"                                                  \
  "mov %[w5], 40(%[r])"                                                      \
  : [ w0 ] "=&r" ( w0 ) , [ w1 ] "=&r" ( w1 ) , [ w2 ] "=&r" ( w2 ) ,        \
    [ w3 ] "=&r" ( w3 ) , [ w4 ] "=&r" ( w4 ) , [ w5 ] "=&r" ( w5 ) ,       \
    [ w6 ] "=&r" ( w6 ) , [ lo ] "=&r" ( lo ) , [ hi ] "=&r" ( hi ) ,       \
    [ c ] "+&r" ( c ) , [ a ] "+&r" ( pa ) , [ r ] "+&r" ( pr )             \
  : [ b ] "r" ( b ) , [ z ] "m" ( zero ) , [ n7 ] "m" ( n7 )                 \
  : "rdx" , "cc" , "memory" )

/// r[ 0 , n + 6 ) = a[ 0 , n ) b[ 0 , 6 ) if not add, else
/// r[ 0 , n + 6 ) = r[ 0 , n ) + a b, n > 0: the pass of 4 rows [see
/// mulPass4()] on 6, the window of 7 places, the loop 7 digits long, 31
/// instructions for 6 products instead of 22 for 4
template< bool add >
inline void mulPass6( std::uint64_t * r , const std::uint64_t * a ,
                      std::size_t n , const std::uint64_t * b )
{
 const std::uint64_t e = ( 7 - n % 7 ) % 7 , zero = 0 , n7 = ( n + e ) / 7;
 std::uint64_t w0 , w1 , w2 , w3 , w4 , w5 , w6 , lo , hi , c = e ,
               pa = std::uint64_t( reinterpret_cast< std::uintptr_t >( a ) ) -
                    8 * e ,
               pr = std::uint64_t( reinterpret_cast< std::uintptr_t >( r ) ) -
                    8 * e;
 if constexpr( add ) ADS_PASS6( ADS_PASS_RADD , ADS_PASS_STORE );
 else ADS_PASS6( ADS_PASS_RNONE , ADS_PASS_STORE );
 }

/// as subPass4() on 6 digits of b: r[ 0 , n ) = ~( ~r[ 0 , n ) + a b ) and
/// r[ n , n + 6 ) the top 6 digits of ~r[ 0 , n ) + a b
inline void subPass6( std::uint64_t * r , const std::uint64_t * a ,
                      std::size_t n , const std::uint64_t * b )
{
 const std::uint64_t e = ( 7 - n % 7 ) % 7 , zero = 0 , n7 = ( n + e ) / 7;
 std::uint64_t w0 , w1 , w2 , w3 , w4 , w5 , w6 , lo , hi , c = e ,
               pa = std::uint64_t( reinterpret_cast< std::uintptr_t >( a ) ) -
                    8 * e ,
               pr = std::uint64_t( reinterpret_cast< std::uintptr_t >( r ) ) -
                    8 * e;
 ADS_PASS6( ADS_PASS_RNOT , ADS_PASS_STNOT );
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

/// the products a_i a_j, 0 <= i < j < 6, at the places i + j = 1 .. 10,
/// in t[ 0 .. 10 ): the rows of a0 .. a4 on the digits after them, each
/// row's high halves on the chain of CF and its sums into the places on
/// that of OF, the places final after each row stored to free registers
inline void tri6Asm( std::uint64_t * t , const std::uint64_t * a )
{
 std::uint64_t x1 , x2 , x3 , x4 , x5 , x6 , lo , hi , z;
 __asm__ __volatile__(
  "xor %k[z], %k[z]\n\t"
  "mov (%[a]), %%rdx\n\t"                  // a0: places 1 .. 6
  "mulx 8(%[a]), %[x1], %[x2]\n\t"
  "mulx 16(%[a]), %[lo], %[x3]\n\t"
  "adcx %[lo], %[x2]\n\t"
  "mulx 24(%[a]), %[lo], %[x4]\n\t"
  "adcx %[lo], %[x3]\n\t"
  "mulx 32(%[a]), %[lo], %[x5]\n\t"
  "adcx %[lo], %[x4]\n\t"
  "mulx 40(%[a]), %[lo], %[x6]\n\t"
  "adcx %[lo], %[x5]\n\t"
  "adcx %[z], %[x6]\n\t"
  "mov %[x1], (%[t])\n\t"
  "mov %[x2], 8(%[t])\n\t"
  "mov 8(%[a]), %%rdx\n\t"                 // a1: places 3 .. 7, in x1
  "mulx 16(%[a]), %[lo], %[hi]\n\t"
  "adox %[lo], %[x3]\n\t"
  "adcx %[hi], %[x4]\n\t"
  "mulx 24(%[a]), %[lo], %[hi]\n\t"
  "adox %[lo], %[x4]\n\t"
  "adcx %[hi], %[x5]\n\t"
  "mulx 32(%[a]), %[lo], %[hi]\n\t"
  "adox %[lo], %[x5]\n\t"
  "adcx %[hi], %[x6]\n\t"
  "mulx 40(%[a]), %[lo], %[x1]\n\t"
  "adox %[lo], %[x6]\n\t"
  "adcx %[z], %[x1]\n\t"
  "adox %[z], %[x1]\n\t"
  "mov %[x3], 16(%[t])\n\t"
  "mov %[x4], 24(%[t])\n\t"
  "mov 16(%[a]), %%rdx\n\t"                // a2: places 5 .. 8, in x2
  "mulx 24(%[a]), %[lo], %[hi]\n\t"
  "adox %[lo], %[x5]\n\t"
  "adcx %[hi], %[x6]\n\t"
  "mulx 32(%[a]), %[lo], %[hi]\n\t"
  "adox %[lo], %[x6]\n\t"
  "adcx %[hi], %[x1]\n\t"
  "mulx 40(%[a]), %[lo], %[x2]\n\t"
  "adox %[lo], %[x1]\n\t"
  "adcx %[z], %[x2]\n\t"
  "adox %[z], %[x2]\n\t"
  "mov %[x5], 32(%[t])\n\t"
  "mov %[x6], 40(%[t])\n\t"
  "mov 24(%[a]), %%rdx\n\t"                // a3: places 7 .. 9, in x3
  "mulx 32(%[a]), %[lo], %[hi]\n\t"
  "adox %[lo], %[x1]\n\t"
  "adcx %[hi], %[x2]\n\t"
  "mulx 40(%[a]), %[lo], %[x3]\n\t"
  "adox %[lo], %[x2]\n\t"
  "adcx %[z], %[x3]\n\t"
  "adox %[z], %[x3]\n\t"
  "mov %[x1], 48(%[t])\n\t"
  "mov %[x2], 56(%[t])\n\t"
  "mov 32(%[a]), %%rdx\n\t"                // a4: places 9 .. 10, in x4
  "mulx 40(%[a]), %[lo], %[x4]\n\t"
  "adox %[lo], %[x3]\n\t"
  "adox %[z], %[x4]\n\t"
  "mov %[x3], 64(%[t])\n\t"
  "mov %[x4], 72(%[t])"
  : [ x1 ] "=&r" ( x1 ) , [ x2 ] "=&r" ( x2 ) , [ x3 ] "=&r" ( x3 ) ,
    [ x4 ] "=&r" ( x4 ) , [ x5 ] "=&r" ( x5 ) , [ x6 ] "=&r" ( x6 ) ,
    [ lo ] "=&r" ( lo ) , [ hi ] "=&r" ( hi ) , [ z ] "=&r" ( z )
  : [ a ] "r" ( a ) , [ t ] "r" ( t )
  : "rdx" , "cc" , "memory" );
 }

/// the digit i of r = 2 r + the squares a_i^2 at b^{2i}: the places 2i
/// and 2i + 1 doubled with the top bit before on the chain of OF, the
/// halves of a_i^2 added on that of CF
#define ADS_SQRDIAG_STEP( s )                                                \
 "mov " #s "*8(%[a]), %%rdx\n\t"                                             \
 "mulx %%rdx, %[lo], %[hi]\n\t"                                              \
 "mov " #s "*16(%[r]), %[x]\n\t"                                             \
 "adox %[x], %[x]\n\t"                                                       \
 "adcx %[lo], %[x]\n\t"                                                      \
 "mov %[x], " #s "*16(%[r])\n\t"                                             \
 "mov " #s "*16+8(%[r]), %[x]\n\t"                                           \
 "adox %[x], %[x]\n\t"                                                       \
 "adcx %[hi], %[x]\n\t"                                                      \
 "mov %[x], " #s "*16+8(%[r])\n\t"

/// r = 2 r + sum of a_i^2 b^{2i}, r of 2 n digits, n > 0, the result
/// below b^{2n}
inline void sqrDiagAsm( std::uint64_t * r , const std::uint64_t * a ,
                        std::size_t n )
{
 std::uint64_t x , lo , hi , c;
 __asm__ __volatile__(
  "xor %k[x], %k[x]\n\t"
  "mov %[n1], %[c]\n\t"
  "jrcxz 1f\n\t"
  ADS_SQRDIAG_STEP( 0 )
  "lea 8(%[a]), %[a]\n\t"
  "lea 16(%[r]), %[r]\n"
  "1:\n\t"
  "mov %[n2], %[c]\n\t"
  "jmp 7f\n"
  "9:\n\t"
  ADS_SQRDIAG_STEP( 0 ) ADS_SQRDIAG_STEP( 1 )
  "lea 16(%[a]), %[a]\n\t"
  "lea 32(%[r]), %[r]\n\t"
  "lea -1(%[c]), %[c]\n"
  "7:\n\t"
  "jrcxz 8f\n\t"
  "jmp 9b\n"
  "8:"
  : [ x ] "=&r" ( x ) , [ lo ] "=&r" ( lo ) , [ hi ] "=&r" ( hi ) ,
    [ c ] "=&c" ( c ) , [ r ] "+&r" ( r ) , [ a ] "+&r" ( a )
  : [ n1 ] "r" ( n & 1 ) , [ n2 ] "r" ( n / 2 )
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

#define ADS_DIV3_MORE , [ d ] "d" ( f )

/// r = a / d for a multiple of d, d a divisor of b - 1, f = ( b - 1 ) / d,
/// as above with f for ( b - 1 ) / 3
inline void divExactAsm( std::uint64_t * r , const std::uint64_t * a ,
                         std::size_t n , std::uint64_t f )
{
 ADS_ELEM_VARS
 const std::uint64_t * b = a;
 __asm__ __volatile__( ADS_ELEM( ADS_DIV3_STEP , "" )
                       ADS_ELEM_OPS( ADS_DIV3_MORE ) );
 }

/// ( a ; b ) = ( w3 a + w1 b' + c1 ; w2 a' + w0 b + c2 ) in place, n > 0
/// digits, w = { w0 , w1 , w2 , w3 } each below 2^63, x' = ~x if inv,
/// else x, the carries out in c1 and c2 [see BigInteger::hgcdInv1() and
/// BigInteger::hgcdMul1()]: the two products of a digit of a row summed
/// on ADD and ADC, as their high halves and the carries stay below 2^64,
/// then the carry of the row before; two digits a turn, the carry of
/// each row in the registers of the high halves of the other digit
#define ADS_ROWS_DIGIT( s , L1 , H1 , L2 , H2 , C1 , C2 , TL , TH , NOT )     \
 "mov " #s "*8(%[a],%[i],8), %%rdx\n\t"                                      \
 "mulx 24(%[w]), %[" #L1 "], %[" #H1 "]\n\t"                                 \
 NOT                                                                         \
 "mulx 16(%[w]), %[" #L2 "], %[" #H2 "]\n\t"                                 \
 "mov " #s "*8(%[b],%[i],8), %%rdx\n\t"                                      \
 "mulx (%[w]), %[" #TL "], %[" #TH "]\n\t"                                   \
 "add %[" #TL "], %[" #L2 "]\n\t"                                            \
 "adc %[" #TH "], %[" #H2 "]\n\t"                                            \
 NOT                                                                         \
 "mulx 8(%[w]), %[" #TL "], %[" #TH "]\n\t"                                  \
 "add %[" #TL "], %[" #L1 "]\n\t"                                            \
 "adc %[" #TH "], %[" #H1 "]\n\t"                                            \
 "add %[" #C1 "], %[" #L1 "]\n\t"                                            \
 "adc $0, %[" #H1 "]\n\t"                                                    \
 "add %[" #C2 "], %[" #L2 "]\n\t"                                            \
 "adc $0, %[" #H2 "]\n\t"                                                    \
 "mov %[" #L1 "], " #s "*8(%[a],%[i],8)\n\t"                                 \
 "mov %[" #L2 "], " #s "*8(%[b],%[i],8)\n\t"

#define ADS_ROWS_LOOP( NOT )                                                 \
 __asm__ __volatile__(                                                       \
  "test $1, %[i]\n\t"                                                        \
  "jz 1f\n\t"                                                                \
  ADS_ROWS_DIGIT( 0 , m1 , g1 , m2 , g2 , h1 , h2 , l1 , l2 , NOT )          \
  "mov %[g1], %[h1]\n\t"                                                     \
  "mov %[g2], %[h2]\n\t"                                                     \
  "add $1, %[i]\n\t"                                                         \
  "1:\n\t"                                                                   \
  "test %[i], %[i]\n\t"                                                      \
  "jz 3f\n\t"                                                                \
  ".p2align 4\n"                                                             \
  "2:\n\t"                                                                   \
  ADS_ROWS_DIGIT( 0 , m1 , g1 , m2 , g2 , h1 , h2 , l1 , l2 , NOT )          \
  ADS_ROWS_DIGIT( 1 , l1 , h1 , l2 , h2 , g1 , g2 , m1 , m2 , NOT )          \
  "add $2, %[i]\n\t"                                                         \
  "jnz 2b\n"                                                                 \
  "3:\n\t"                                                                   \
  : [ l1 ] "=&r" ( l1 ) , [ h1 ] "+&r" ( c1 ) , [ l2 ] "=&r" ( l2 ) ,        \
    [ h2 ] "+&r" ( c2 ) , [ m1 ] "=&r" ( m1 ) , [ g1 ] "=&r" ( g1 ) ,        \
    [ m2 ] "=&r" ( m2 ) , [ g2 ] "=&r" ( g2 ) , [ i ] "+&r" ( i )            \
  : [ a ] "r" ( a ) , [ b ] "r" ( b ) , [ w ] "r" ( w )                      \
  : "rdx" , "cc" , "memory" )

template< bool inv >
inline void rowsAsm( std::uint64_t * a , std::uint64_t * b , std::size_t n ,
                     const std::uint64_t * w , std::uint64_t & c1 ,
                     std::uint64_t & c2 )
{
 std::uint64_t l1 , l2 , m1 , g1 , m2 , g2;
 std::int64_t i = -std::int64_t( n );
 a += n;
 b += n;
 if constexpr( inv ) ADS_ROWS_LOOP( "not %%rdx\n\t" );
 else ADS_ROWS_LOOP( "" );
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
 const std::size_t s = p - z - pc;          // the window [ s , p - z )
 Limb e = 1;                                // short by e at most
 Mag lo;
 // past kFracCyclic digits of w the product modulo b^N - 1, N >= p and
 // N >= p + |w| - s + 2, so that its top digits W wrap onto the bottom,
 // below the window, which the sum raises by 1 at most; but for the low
 // N digits L of the product at least b^N - 1 - W, rare, when the
 // residue is L + W - b^N + 1 < b^{s-1}: then the digits from s - 1 up are
 // 0 and the product is made in full
 bool done = false;
#if ADS_BIGINT_FNTT
 const std::size_t ws = w.size() ,
                   N = detail::length( std::max( p , p + ws - s + 2 ) );
 if( ( ws >= kFracCyclic ) && ( N < p + ws ) && ( s >= 2 ) &&
     ( N <= ( std::size_t( 1 ) << 22 ) ) ) {
  Mag t( N );
  mulCyclic( t.data() , f.data() , p , w.data() , ws , N );
  bool zero = true;
  for( std::size_t i = s - 1 ; i < N ; ++i ) zero = zero && ( t[ i ] == 0 );
  if( ! zero ) {
   lo = Mag( t.data() + s , t.data() + p - z );
   bool lz = true;                          // over by 1 at most: rounded
   for( const Limb d : lo ) lz = lz && ( d == 0 );   // down, one less
   if( ! up && ! lz ) subFrom( lo.data() , pc , &one , 1 );
   done = true;
   }
  }
#endif
 if( ! done ) {
  Mag t( p + w.size() + 4 );
  if( mulBand( t.data() , f.data() , p , w.data() , w.size() , s , p - z ) )
   e = 3;                                   // 2 for the products left out
  else mulP( t.data() , f.data() , p , w.data() , w.size() );
  lo = Mag( t.data() + s , t.data() + p - z );
  }
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
 // by passes of 4 rows, the rows left one at a time; the passes of up to
 // 8 digits with no loop
 std::size_t j = 4;
 if( na <= 8 ) {
  detail::mulPassRow< false >( r , a , na , b );
  for( ; j + 4 <= nb ; j += 4 )
   detail::mulPassRow< true >( r + j , a , na , b + j );
  }
 else if( nb >= 6 ) {                      // by 6 rows, 4 for the rest
  detail::mulPass6< false >( r , a , na , b );
  for( j = 6 ; j + 6 <= nb ; j += 6 )
   detail::mulPass6< true >( r + j , a , na , b + j );
  for( ; j + 4 <= nb ; j += 4 )
   detail::mulPass4< true >( r + j , a , na , b + j );
  }
 else detail::mulPass4< false >( r , a , na , b );
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
  // a^2 = 2 T + D, T the products a_i a_j, i < j, and D the squares a_i^2
  // at b^{2i}: T by blocks of 6 digits, the passes of 6 rows over the
  // digits after each block and the products inside each block, then D
  // with the doubling in one pass
  std::fill( r , r + 2 * n , 0 );
  std::size_t i = 0;
  for( ; i + 6 <= n ; i += 6 ) {
   if( i + 6 < n )
    detail::mulPass6< true >( r + 2 * i + 6 , a + i + 6 , n - i - 6 , a + i );
   Limb t[ 10 ];
   detail::tri6Asm( t , a + i );
   addTo( r + 2 * i + 1 , 2 * n - 2 * i - 1 , t , 10 );
   }
  for( ; i + 1 < n ; ++i ) {                // the last block, short
   const Limb c = addMul1( r + 2 * i + 1 , a + i + 1 , n - i - 1 , a[ i ] );
   addTo( r + n + i , n - i , &c , 1 );
   }
  detail::sqrDiagAsm( r , a , n );
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
 // a step on n digits takes at most 7 n + 40 and leaves at most
 // n / 2 + 2 digits to the next one
 return( 16 * n + 64 * std::size_t( std::bit_width( n ) ) + 64 );
 }

inline void BigInteger::mulN( Limb * r , const Limb * a , const Limb * b ,
                              std::size_t n , Limb * w , std::size_t fft ,
                              std::size_t toom )
{
 // the squares by thresholds of their own, unless a method was asked
 const bool sq = a == b;
 if( sq ) {
  if( fft == kFFT ) fft = kSqrFFT;
  if( toom == kToom3 ) toom = kSqrToom3;
  }
 if( n < ( sq ? kSqrKaratsuba : kKaratsuba ) ) {
  if( sq ) sqrSchool( r , a , n );
  else mulSchool( r , a , n , b , n );
  }
 else if( ( n >= fft ) && ( 2 * n <= kNTTMax ) ) mulNTT( r , a , n , b , n );
 else if( ( n >= ( sq ? kSqrToom4 : kToom4 ) ) && ( n >= toom ) )
  toom4( r , a , b , n , w , fft , toom );
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
 divExactB1( a , n , 0x5555555555555555ULL );
 }

inline void BigInteger::divExactB1( Limb * a , std::size_t n , Limb f )
{
#if ADS_BIGINT_ASM
 detail::divExactAsm( a , a , n , f );
#else
 // with f = ( b - 1 ) / d, -f = 1 / d modulo b, and a / d is
 // -a f ( 1 + b + ... + b^{n-1} ) modulo b^n, a running difference
 Limb h = 0;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  Limb p1;
  const Limb p0 = detail::mulWide( a[ i ] , f , p1 ) , c = h < p0;
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

inline void BigInteger::rshN( Limb * a , std::size_t n , int s )
{
 for( std::size_t i = 0 ; i + 1 < n ; ++i )
  a[ i ] = ( a[ i ] >> s ) | ( a[ i + 1 ] << ( 64 - s ) );
 a[ n - 1 ] >>= s;
 }

inline void BigInteger::toom4( Limb * r , const Limb * a , const Limb * b ,
                               std::size_t n , Limb * w , std::size_t fft ,
                               std::size_t toom )
{
 // a = a3 x^3 + a2 x^2 + a1 x + a0, x = b^k, a3 of s digits, the same
 // for b; the product r( x ) = r6 x^6 + ... + r0 from its values at 0,
 // 1, -1, 2, -2, 1/2 (times 2^6) and infinity, all the steps of the
 // interpolation on nonnegative numbers: with E1 = ( v1 + v-1 ) / 2,
 // O1 = ( v1 - v-1 ) / 2, E2 = ( v2 + v-2 ) / 2, O2 = ( v2 - v-2 ) / 4,
 // r4 = ( ( E2 - r0 - 64 r6 ) / 4 - ( E1 - r0 - r6 ) ) / 3,
 // r2 = E1 - r0 - r6 - r4, H = ( vh - 64 r0 - 16 r2 - 4 r4 - r6 ) / 2 =
 // 16 r1 + 4 r3 + r5, r3 = ( 17 O1 - O2 - H ) / 9,
 // r5 = ( O2 - O1 - 3 r3 ) / 15, r1 = O1 - r3 - r5
 const bool sq = a == b;
 const std::size_t k = ( n + 3 ) / 4 , s = n - 3 * k , K = k + 1 , L = 2 * K;
 Limb * const e = w;                        // the 10 values, K digits each
 Limb * const v1 = e + 10 * K , * const vm1 = v1 + L , * const v2 = vm1 + L ,
      * const vm2 = v2 + L , * const vh = vm2 + L , * const t = vh + L ,
      * const ws = t + L;
 // x( 1 ), |x( -1 )|, x( 2 ), |x( -2 )| and 8 x( 1 / 2 ) in p[ 0 .. 4 ],
 // the signs of x( -1 ) and x( -2 )
 auto eval = [ & ]( const Limb * x , Limb * p , bool & n1 , bool & n2 ) {
  Limb * const P1 = p , * const M1 = p + K , * const P2 = p + 2 * K ,
       * const M2 = p + 3 * K , * const H = p + 4 * K , * const u = t ,
       * const z = t + K;
  u[ k ] = addN( u , x , k , x + 2 * k , k );       // x0 + x2
  z[ k ] = addN( z , x + k , k , x + 3 * k , s );   // x1 + x3
  addN( P1 , u , K , z , K );
  n1 = absDiff( M1 , u , K , z , K );
  std::copy( x , x + k , u );               // x0 + 4 x2
  u[ k ] = addMul1( u , x + 2 * k , k , 4 );
  z[ k ] = mul1( z , x + k , k , 2 );       // 2 x1 + 8 x3
  const Limb c = addMul1( z , x + 3 * k , s , 8 );
  addTo( z + s , K - s , &c , 1 );
  addN( P2 , u , K , z , K );
  n2 = absDiff( M2 , u , K , z , K );
  std::copy( x + 3 * k , x + 3 * k + s , H );   // x3 + 2 x2 + 4 x1 + 8 x0
  std::fill( H + s , H + K , 0 );
  H[ k ] = addMul1( H , x + 2 * k , k , 2 );
  H[ k ] += addMul1( H , x + k , k , 4 );
  H[ k ] += addMul1( H , x , k , 8 );
  };
 bool na1 , na2 , nb1 = false , nb2 = false;
 eval( a , e , na1 , na2 );
 if( ! sq ) eval( b , e + 5 * K , nb1 , nb2 );
 else {                                   // the values of a square are >= 0
  nb1 = na1;
  nb2 = na2;
  }
 const Limb * const f = sq ? e : e + 5 * K;
 mulN( v1 , e , f , K , ws , fft , toom );
 mulN( vm1 , e + K , f + K , K , ws , fft , toom );
 mulN( v2 , e + 2 * K , f + 2 * K , K , ws , fft , toom );
 mulN( vm2 , e + 3 * K , f + 3 * K , K , ws , fft , toom );
 mulN( vh , e + 4 * K , f + 4 * K , K , ws , fft , toom );
 mulN( r , a , b , k , ws , fft , toom );   // r0
 mulN( r + 6 * k , a + 3 * k , b + 3 * k , s , ws , fft , toom );   // r6
 const Limb * const r0 = r , * const r6 = r + 6 * k;
 // O1 in vm1, E1 - r0 - r6 in v1; 2 O2 in vm2, E2 in v2
 if( na1 != nb1 ) rsh1Add( vm1 , v1 , vm1 , L );
 else rsh1Sub( vm1 , v1 , vm1 , L );
 subFrom( v1 , L , vm1 , L );
 subFrom( v1 , L , r0 , 2 * k );
 subFrom( v1 , L , r6 , 2 * s );
 if( na2 != nb2 ) rsh1Add( vm2 , v2 , vm2 , L );
 else rsh1Sub( vm2 , v2 , vm2 , L );
 subFrom( v2 , L , vm2 , L );
 rshN( vm2 , L , 1 );
 // r4 = ( ( E2 - r0 - 64 r6 ) / 4 - v1 ) / 3 in v2, r2 = v1 - r4 in v1
 subFrom( v2 , L , r0 , 2 * k );
 const Limb c6 = subMul1( v2 , r6 , 2 * s , 64 );
 subFrom( v2 + 2 * s , L - 2 * s , &c6 , 1 );
 rshN( v2 , L , 2 );
 subFrom( v2 , L , v1 , L );
 divExact3( v2 , L );
 subFrom( v1 , L , v2 , L );
 // H = ( vh - 64 r0 - 16 r2 - 4 r4 - r6 ) / 2 in vh
 const Limb c0 = subMul1( vh , r0 , 2 * k , 64 );
 subFrom( vh + 2 * k , L - 2 * k , &c0 , 1 );
 subMul1( vh , v1 , L , 16 );
 subMul1( vh , v2 , L , 4 );
 subFrom( vh , L , r6 , 2 * s );
 rshN( vh , L , 1 );
 // r3 = ( 17 O1 - O2 - H ) / 9 in t, r5 = ( O2 - O1 - 3 r3 ) / 15 in vm2,
 // r1 = O1 - r3 - r5 in vm1
 mul1( t , vm1 , L , 17 );
 subFrom( t , L , vm2 , L );
 subFrom( t , L , vh , L );
 divExact3( t , L );                       // by 9
 divExact3( t , L );
 subFrom( vm2 , L , vm1 , L );
 subMul1( vm2 , t , L , 3 );
 divExactB1( vm2 , L , 0x1111111111111111ULL );   // by 15
 subFrom( vm1 , L , t , L );
 subFrom( vm1 , L , vm2 , L );
 // r = r6 x^6 + r5 x^5 + ... + r0, each r_i below b^{2n - ik}
 std::copy( v1 , v1 + 2 * k , r + 2 * k );
 std::copy( v2 , v2 + 2 * k , r + 4 * k );
 const std::size_t N = 2 * n;
 addTo( r + 4 * k , N - 4 * k , v1 + 2 * k , L - 2 * k );
 addTo( r + 6 * k , N - 6 * k , v2 + 2 * k , std::min( L - 2 * k , 2 * s ) );
 addTo( r + k , N - k , vm1 , L );
 addTo( r + 3 * k , N - 3 * k , t , L );
 addTo( r + 5 * k , N - 5 * k , vm2 , std::min( L , N - 5 * k ) );
 }

inline bool BigInteger::toom32( Limb * r , const Limb * a , std::size_t na ,
                                const Limb * b , std::size_t nb , Limb * w ,
                                std::size_t fft , std::size_t toom )
{
 // a = a2 x^2 + a1 x + a0 and b = b1 x + b0, x = b^k: the product
 // r3 x^3 + ... + r0 from v0 = a0 b0, v1 = a( 1 ) b( 1 ),
 // v-1 = a( -1 ) b( -1 ) and r3 = a2 b1: r2 = ( v1 + v-1 ) / 2 - v0,
 // r1 = ( v1 - v-1 ) / 2 - r3
 const std::size_t k = std::max( ( na + 2 ) / 3 , ( nb + 1 ) / 2 );
 if( ( na <= 2 * k ) || ( nb <= k ) ) return( false );
 const std::size_t s = na - 2 * k , t = nb - k , K = k + 1 , L = 2 * K ,
                   N = na + nb;
 Limb * const ap = w , * const am = ap + K , * const bp = am + K ,
      * const bm = bp + K , * const v1 = bm + K , * const vm = v1 + L ,
      * const ws = vm + L;
 ap[ k ] = addN( ap , a , k , a + 2 * k , s );   // a0 + a2
 bool neg = absDiff( am , ap , K , a + k , k );
 addTo( ap , K , a + k , k );
 bp[ k ] = addN( bp , b , k , b + k , t );
 bm[ k ] = 0;
 neg = neg != absDiff( bm , b , k , b + k , t );
 mulN( v1 , ap , bp , K , ws , fft , toom );
 mulN( vm , am , bm , K , ws , fft , toom );
 mulN( r , a , b , k , ws , fft , toom );   // v0
 std::fill( r + 2 * k , r + 3 * k , 0 );
 mulP( r + 3 * k , a + 2 * k , s , b + k , t , fft , toom );   // r3
 // ( v1 - v-1 ) / 2 in vm, ( v1 + v-1 ) / 2 in v1, with the sign of v-1
 if( neg ) rsh1Add( vm , v1 , vm , L );
 else rsh1Sub( vm , v1 , vm , L );
 subFrom( v1 , L , vm , L );
 subFrom( v1 , L , r , 2 * k );             // r2
 subFrom( vm , L , r + 3 * k , s + t );     // r1
 addTo( r + k , N - k , vm , std::min( L , N - k ) );
 addTo( r + 2 * k , N - 2 * k , v1 , std::min( L , N - 2 * k ) );
 return( true );
 }

inline bool BigInteger::toom42( Limb * r , const Limb * a , std::size_t na ,
                                const Limb * b , std::size_t nb , Limb * w ,
                                std::size_t fft , std::size_t toom )
{
 // a = a3 x^3 + ... + a0 and b = b1 x + b0, x = b^k: the product
 // r4 x^4 + ... + r0 from the values at 0, 1, -1, 2 and infinity: with
 // E = ( v1 + v-1 ) / 2 and O = ( v1 - v-1 ) / 2, r2 = E - r0 - r4,
 // r3 = ( ( v2 - r0 - 4 r2 - 16 r4 ) / 2 - O ) / 3, r1 = O - r3
 const std::size_t k = std::max( ( na + 3 ) / 4 , ( nb + 1 ) / 2 );
 if( ( na <= 3 * k ) || ( nb <= k ) ) return( false );
 const std::size_t s = na - 3 * k , t = nb - k , K = k + 1 , L = 2 * K ,
                   N = na + nb;
 Limb * const ap = w , * const am = ap + K , * const a2 = am + K ,
      * const bp = a2 + K , * const bm = bp + K , * const b2 = bm + K ,
      * const v1 = b2 + K , * const vm = v1 + L , * const v2 = vm + L ,
      * const u = v2 + L , * const ws = u + L;
 // a( 1 ) , |a( -1 )| , a( 2 ) from x0 = a0 + a2 and x1 = a1 + a3
 u[ k ] = addN( u , a , k , a + 2 * k , k );
 Limb * const z = u + K;
 z[ k ] = addN( z , a + k , k , a + 3 * k , s );
 addN( ap , u , K , z , K );
 bool neg = absDiff( am , u , K , z , K );
 std::copy( a , a + k , a2 );               // a0 + 2 a1 + 4 a2 + 8 a3
 a2[ k ] = addMul1( a2 , a + k , k , 2 );
 a2[ k ] += addMul1( a2 , a + 2 * k , k , 4 );
 const Limb c3 = addMul1( a2 , a + 3 * k , s , 8 );
 addTo( a2 + s , K - s , &c3 , 1 );
 bp[ k ] = addN( bp , b , k , b + k , t );
 bm[ k ] = 0;
 neg = neg != absDiff( bm , b , k , b + k , t );
 std::copy( b , b + k , b2 );               // b0 + 2 b1
 b2[ k ] = 0;
 {
  const Limb c = addMul1( b2 , b + k , t , 2 );
  addTo( b2 + t , K - t , &c , 1 );
 }
 mulN( v1 , ap , bp , K , ws , fft , toom );
 mulN( vm , am , bm , K , ws , fft , toom );
 mulN( v2 , a2 , b2 , K , ws , fft , toom );
 mulN( r , a , b , k , ws , fft , toom );   // r0
 std::fill( r + 2 * k , r + 4 * k , 0 );
 mulP( r + 4 * k , a + 3 * k , s , b + k , t , fft , toom );   // r4
 const Limb * const r0 = r , * const r4 = r + 4 * k;
 // O in vm, E then r2 in v1
 if( neg ) rsh1Add( vm , v1 , vm , L );
 else rsh1Sub( vm , v1 , vm , L );
 subFrom( v1 , L , vm , L );
 subFrom( v1 , L , r0 , 2 * k );
 subFrom( v1 , L , r4 , s + t );
 // r3 in v2, r1 in vm
 subFrom( v2 , L , r0 , 2 * k );
 subMul1( v2 , v1 , L , 4 );
 const Limb c = subMul1( v2 , r4 , s + t , 16 );
 subFrom( v2 + s + t , L - s - t , &c , 1 );
 rshN( v2 , L , 1 );
 subFrom( v2 , L , vm , L );
 divExact3( v2 , L );
 subFrom( vm , L , v2 , L );
 addTo( r + k , N - k , vm , std::min( L , N - k ) );
 addTo( r + 2 * k , N - 2 * k , v1 , std::min( L , N - 2 * k ) );
 addTo( r + 3 * k , N - 3 * k , v2 , std::min( L , N - 3 * k ) );
 return( true );
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
 // digits of the longer factor apart when the transform of the rest is
 // shorter by more than they cost: about 12 L log2( L ) cycles for a
 // transform of L terms, 1.6 per product of the rows below kKaratsuba
 // digits, and from there nb / t products of t digits by Karatsuba, each
 // 1.6 K^2 ( t / K )^{log2 3} for K = kKaratsuba
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
 const bool sq = ( a == b ) && ( na == nb );   // no rows, one transform
 // the lengths 2^k and 3 2^{k-1}
 for( std::size_t k = 4 ; k < ( sq ? 4 : 23 ) ; ++k )
  for( std::size_t m : { std::size_t( 1 ) << k ,
                         std::size_t( 3 ) << ( k - 1 ) } )
   if( ( m < nf0 ) && ( m + na / 2 >= L ) && ( m + 1 <= L ) ) {
    const double tt = double( L - m ) , K = double( kKaratsuba ) ,
                 top = tt < K ? 1.6 * tt * double( nb )
                              : double( nb ) / tt * 1.6 * K * K *
                                std::pow( tt / K , 1.585 );
    const double c = cost( m ) + top;
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
  if( t < kKaratsuba )
   for( std::size_t j = na - t ; j < na ; ++j )
    r[ j + nb ] = addMul1( r + j , b , nb , a[ j ] );
  else {                                    // the top digits by mulP()
   Mag x( t + nb );
   mulP( x.data() , a + na - t , t , b , nb );
   addTo( r + na - t , t + nb , x.data() , t + nb );
   }
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
 const bool sq = ( a == b ) && ( na == nb );
 if( nb < ( sq ? kSqrKaratsuba : kKaratsuba ) ) {
  if( sq ) sqrSchool( r , a , na );
  else mulSchool( r , a , na , b , nb );
  return;
  }
 if( ( nb >= ( sq && ( fft == kFFT ) ? kSqrFFT : fft ) ) &&
     ( na + nb <= kNTTMax ) ) {
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
 // a up to 7 / 4 as long as b by Toom-3/2, up to 5 / 2 by Toom-4/2, as
 // long as a method was not asked by name
 if( ( nb >= kToom32 ) && ( toom != npos ) ) {
  if( 4 * na <= 7 * nb ) {
   if( toom32( r , a , na , b , nb , w , fft , toom ) ) return;
   }
  else if( 2 * na <= 5 * nb ) {
   if( toom42( r , a , na , b , nb , w , fft , toom ) ) return;
   }
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
 BigInteger r;                              // in place, with no move
 const std::size_t na = a.f_d.size() , nb = b.f_d.size();
 if( ( na == 0 ) || ( nb == 0 ) ) return( r );
 r.f_d.resize( na + nb );
 BigInteger::mulP( r.f_d.data() , a.f_d.data() , na , b.f_d.data() , nb );
 if( r.f_d.back() == 0 ) r.f_d.pop_back();  // na + nb - 1 digits or more
 r.f_neg = a.f_neg != b.f_neg;
 return( r );
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

template< std::size_t K >
inline bool BigInteger::divBlocks( Limb * a , std::size_t & m ,
                                   const Limb * b , std::size_t n , Limb * q ,
                                   bool cut ,
                                   Limb d1 , Limb d0 , Limb v ,
                                   std::size_t least )
{
#if ADS_BIGINT_ASM
 // the digits of the quotient K at a time: the K digits Q of the window
 // a[ j - K , j + n ), below b b^K, from its top t + K digits by the top
 // t = K + 2 digits of b: no smaller than Q, and larger by 1 at most (by
 // 1 + Q / b^{t-1} < 2), then the window less Q b by a pass of K rows
 // [see subPass4()], and b added back while it is negative; if cut, the
 // window and b cut to the L = j + 1 leading digits that the digit j - 1
 // takes [see divBase()], Q of b^K or more for the cut b sending back true
 constexpr std::size_t t = K + 2;
 for( ; m >= K ; m -= K ) {
  const std::size_t L = cut ? std::min( n , m + 1 ) : n , o = n - L;
  if( L < least ) break;
  Limb * const x = a + m - K + o;           // the window
  const Limb * const y = b + o;
  Limb w[ t + K ] , Q[ K ] , T[ K ] , S[ K ];
  const Limb * const D = b + n - t;
  std::copy( x + L - t , x + L + K , w );
  bool full = cmpN( w + K , D , t ) >= 0;   // then Q = b^K - 1
  for( std::size_t k = K ; ! full && k-- > 0 ; ) {
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
   if( cut ) return( true );
   std::fill( Q , Q + K , ~Limb( 0 ) );
   }
  std::copy( x + L , x + L + K , T );
  if constexpr( K == 6 ) detail::subPass6( x , y , L , Q );
  else detail::subPass4( x , y , L , Q );
  std::copy( x + L , x + L + K , S );
  // the top D = T - S of the window less Q b, -1 or 0 since the window
  // is in [ -b , b ): S - T is 0 or 1
  const Limb br = subN( S , S , K , T , K ) , one = 1;
  ADS_INVARIANT( ! br && ( S[ 0 ] < 2 ) &&
                 std::all_of( S + 1 , S + K ,
                              []( Limb y0 ) { return( y0 == 0 ); } ) );
  for( Limb d = S[ 0 ] ; d ; ) {            // b back, rarely
   subFrom( Q , K , &one , 1 );
   d -= addTo( x , L , y , L );
   }
  std::fill( x + L , x + L + K , 0 );
  std::copy( Q , Q + K , q + m - K );
  }
#else
 (void) a; (void) m; (void) b; (void) n; (void) q; (void) cut; (void) d1;
 (void) d0; (void) v; (void) least;
#endif
 return( false );
 }

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
 if( divBlocks< 6 >( a , m , b , n , q , cut , d1 , d0 , v , kDivBlock6 ) ||
     divBlocks< 4 >( a , m , b , n , q , cut , d1 , d0 , v , kDivBlock ) )
  return( 2 );
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
 if( s ) {                                  // forward, in lanes
  const int t = 64 - s;
  vn[ 0 ] = v[ 0 ] << s;
  for( std::size_t i = 1 ; i < n ; ++i )
   vn[ i ] = ( v[ i ] << s ) | ( v[ i - 1 ] >> t );
  un[ 0 ] = u[ 0 ] << s;
  for( std::size_t i = 1 ; i < nu ; ++i )
   un[ i ] = ( u[ i ] << s ) | ( u[ i - 1 ] >> t );
  un[ nu ] = u[ nu - 1 ] >> t;
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
 else if( ( n >= kNewton ) ||
          ( keep && ( n >= kNewtonKept ) ) )
  top = divNewton( un , k , vn , n , q.data() , keep );
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
 std::size_t m = n;                         // the digits by Hensel
 if( n >= kDivExact2 ) {
  // both ways at once (Krandick and Jebelean): the low m = n - n / 3 of the
  // quotient by Hensel from the bottom, the others by the division of
  // a[ m , n ) from the top, on two chains that do not wait for each
  // other; with q = qh b^m + ql, ql d = a mod b^m + c b^m for the last
  // borrow c < d, so a / b^m = qh d + c and its quotient is qh
  const std::size_t top = n / 3;           // the digits from the top
  m = n - top;
  const int t = std::countl_zero( d );
  const Limb dn = d << t , w = detail::reciprocal( dn ) , below = y[ m - 1 ];
  Limb r = t ? y[ n - 1 ] >> ( 64 - t ) : 0;
  auto hensel = [ & ]( std::size_t i ) {
   const Limb x = y[ i ] , q = ( x - c ) * v;
   Limb hi;
   detail::mulWide( q , d , hi );
   c = hi + ( x < c );
   y[ i ] = q;
   };
  // two digits from the bottom for each one from the top, whose chain is
  // about twice as long
  for( std::size_t k = 0 ; k < top ; ++k ) {
   hensel( 2 * k );
   hensel( 2 * k + 1 );
   const std::size_t i = n - 1 - k;
   const Limb lower = i > m ? y[ i - 1 ] : below ,
              u = t ? ( y[ i ] << t ) | ( lower >> ( 64 - t ) ) : y[ i ];
   y[ i ] = detail::div2by1( r , u , dn , w , r );
   }
  for( std::size_t i = 2 * top ; i < m ; ++i ) hensel( i );
  trim( a );
  return;
  }
 for( std::size_t i = 0 ; i < m ; ++i ) {
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
/*---------------------------- THE SQUARE ROOT -----------------------------*/
/*--------------------------------------------------------------------------*/

inline void BigInteger::sqrt2( Limb h , Limb l , Mag & s , Mag & r )
{
 // the root x in doubles, about 2^11 off at most, then x + d with d the
 // nearest integer to ( a - x^2 ) / 2 x, in doubles too, off by 1 at
 // most, then a step of 1 at a time, the remainder kept exact on 128 bits
 // (a - x^2 >= 0 for x <= root, and below 2 x + 1 at the end)
 const double e = std::sqrt( double( h ) * 0x1p64 + double( l ) );
 Limb x = e >= 0x1p64 ? ~Limb( 0 ) : Limb( e );
 auto rem = [ & ]( Limb y , Limb & r1 ) {   // a - y^2 modulo 2^128
  Limb ph , c = 0;
  const Limb pl = detail::mulWide( y , y , ph ) ,
             r0 = detail::subBorrow( l , pl , c );
  r1 = h - ph - c;
  return( r0 );
  };
 Limb r1 , r0 = rem( x , r1 );
 const double d = ( double( std::int64_t( r1 ) ) * 0x1p64 + double( r0 ) ) /
                  ( 2 * double( x ) );
 x += Limb( std::int64_t( std::nearbyint( d ) ) );
 r0 = rem( x , r1 );
 while( std::int64_t( r1 ) < 0 ) {          // x above the root
  --x;
  r0 = rem( x , r1 );
  }
 // a - x^2 > 2 x: x below the root
 while( ( r1 > ( x >> 63 ) ) ||
        ( ( r1 == ( x >> 63 ) ) && ( r0 > ( x << 1 ) ) ) ) {
  ++x;
  r0 = rem( x , r1 );
  }
 s = Mag{ x };
 r.resize( 0 );
 if( r1 ) r = Mag{ r0 , r1 };
 else if( r0 ) r = Mag{ r0 };
 }

inline std::size_t BigInteger::sqrtN( const Limb * m , std::size_t k ,
                                      Limb * s , Limb * r , bool root )
{
 // as sqrtRec() on the stack, the quotient by 2 s' as that of the half of
 // r' beta + a1 by s', whose top bit is set, and its remainder doubled
 // with the bit out; for the root alone as in sqrtRec(), the remainder of
 // the division at hand
 const Limb one = 1;
 if( k == 1 ) {
  Mag x , y;
  sqrt2( m[ 1 ] , m[ 0 ] , x , y );
  s[ 0 ] = x[ 0 ];
  r[ 0 ] = y.size() > 0 ? y[ 0 ] : 0;
  r[ 1 ] = y.size() > 1 ? y[ 1 ] : 0;
  return( y.size() );
  }
 root = root && ( k >= 4 );
 const std::size_t l = k / 2 - ( root && ( k % 2 == 0 ) ) , h = k - l;
 Limb rr[ kSqrtSmall + 2 ] , t[ kSqrtSmall + 2 ] , q[ kSqrtSmall + 2 ] ,
      x[ kSqrtSmall + 2 ] , q2[ kSqrtSmall + 2 ];
 const std::size_t rn = sqrtN( m + 2 * l , h , s + l , rr );
 std::fill( rr + rn , rr + h + 1 , 0 );
 std::copy( m + l , m + 2 * l , t );        // ( r' beta + a1 ) / 2
 std::copy( rr , rr + h + 1 , t + l );
 const Limb e = t[ 0 ] & 1;
 rshN( t , k + 1 , 1 );
 Limb qt;                                   // the digit l of q, 0 or 1
 if( h == 1 ) {
  const Limb d = s[ l ];
  qt = t[ 1 ] >= d;
  const Limb hi = t[ 1 ] - ( qt ? d : 0 );
  q[ 0 ] = detail::divWide( hi , t[ 0 ] , d );
  t[ 0 ] -= q[ 0 ] * d;
  }
 else qt = divBase( t , l , s + l , h , q );
 // x = u beta + a0, u = 2 u' + e of h + 1 digits
 std::copy( m , m + l , x );
 x[ l + h ] = t[ h - 1 ] >> 63;
 for( std::size_t i = h ; i-- > 1 ; )
  x[ l + i ] = ( t[ i ] << 1 ) | ( t[ i - 1 ] >> 63 );
 x[ l ] = ( t[ 0 ] << 1 ) | e;
 // s = s' beta + q, its digit k in sk; q^2 of 2 l + 1 digits
 std::copy( q , q + l , s );
 const Limb sk = qt ? addTo( s + l , h , &one , 1 ) : 0;
 if( root )                                 // u >= beta: r >= 0
  for( std::size_t i = 2 * l ; i <= k ; ++i )
   if( x[ i ] ) return( 0 );
 if( qt ) {                                 // q = beta
  std::fill( q2 , q2 + 2 * l , 0 );
  q2[ 2 * l ] = 1;
  }
 else {
  mulP( q2 , q , l , q , l );
  q2[ 2 * l ] = 0;
  }
 std::fill( x + k + 1 , x + kSqrtSmall + 2 , 0 );
 std::fill( q2 + 2 * l + 1 , q2 + k + 1 , 0 );
 if( cmpN( x , q2 , k + 1 ) >= 0 ) subN( r , x , k + 1 , q2 , k + 1 );
 else {                                     // x + 2 s - 1 - q^2, s - 1
  Limb c = addLsh1( r , x , s , k );
  r[ k ] = x[ k ] + 2 * sk + c;
  subFrom( r , k + 1 , q2 , k + 1 );
  subFrom( r , k + 1 , &one , 1 );
  subFrom( s , k , &one , 1 );
  }
 std::size_t n = k + 1;
 while( ( n > 0 ) && ( r[ n - 1 ] == 0 ) ) --n;
 return( n );
 }

inline void BigInteger::sqrtRec( const Limb * m , std::size_t k , Mag & s ,
                                 Mag & r , bool root )
{
 // m of 2 k digits, its top digit at least b / 4: with beta = b^l,
 // m = A beta^2 + a1 beta + a0, ( s' , r' ) the root and the remainder
 // of A, ( q , u ) the quotient and the remainder of r' beta + a1 by
 // 2 s', s = s' beta + q and r = u beta + a0 - q^2, corrected once if
 // negative (Zimmermann's Karatsuba square root)
 if( k <= kSqrtSmall ) {                    // on the stack
  s.resize( k + 1 );
  r.resize( k + 1 );
  r.resize( sqrtN( m , k , s.data() , r.data() , root ) );
  s.resize( k );
  trim( s );
  return;
  }
 // for the root alone, l < h so that u >= beta and r >= 0 but with
 // probability about b^{l-h} / 2: no remainder and no q^2 then
 const std::size_t l = k / 2 - ( root && ( k % 2 == 0 ) ) , h = k - l;
 Mag s1 , r1;
 sqrtRec( m + 2 * l , h , s1 , r1 );
 Mag num( r1.size() + l );                  // r' beta + a1
 std::copy( m + l , m + 2 * l , num.data() );
 std::copy( r1.begin() , r1.end() , num.data() + l );
 trim( num );
 const Mag d2 = addMag( s1 , s1 );
 if( root ) {
  // the quotient of num b alone, q b + d: d >= 1 makes u >= 2 s' / b >=
  // b^{h-1} >= beta, so r >= 0, with no remainder; else the remainder
  Mag nb( num.size() + 1 );
  nb[ 0 ] = 0;
  std::copy( num.begin() , num.end() , nb.data() + 1 );
  Mag qe = divModMag( nb , d2 , false , nullptr , false ).first;
  if( ! qe.empty() && ( qe[ 0 ] != 0 ) ) {
   s = Mag( s1.size() + l + 1 , 0 );        // s' beta + q, q <= beta
   std::copy( s1.begin() , s1.end() , s.data() + l );
   addTo( s.data() , s.size() , qe.data() + 1 , qe.size() - 1 );
   trim( s );
   r.resize( 0 );
   return;
   }
  }
 auto [ q , u ] = divModMag( num , d2 );
 s = Mag( s1.size() + l + 1 , 0 );          // s' beta + q, q <= beta
 std::copy( s1.begin() , s1.end() , s.data() + l );
 if( ! q.empty() ) addTo( s.data() , s.size() , q.data() , q.size() );
 trim( s );
 Mag x( u.size() + l );                     // u beta + a0
 std::copy( m , m + l , x.data() );
 std::copy( u.begin() , u.end() , x.data() + l );
 trim( x );
 const Mag q2 = mulMag( q , q );
 if( compareMag( x , q2 ) >= 0 ) r = subMag( x , q2 );
 else {                                     // r + 2 s - 1, s - 1
  r = subMag( addMag( x , addMag( s , s ) ) , addMag( q2 , Mag{ 1 } ) );
  s = subMag( s , Mag{ 1 } );
  }
 }

inline std::pair< BigInteger::Mag , BigInteger::Mag > BigInteger::sqrtMag(
                                                   const Mag & a , bool rem )
{
 std::pair< Mag , Mag > out;
 const std::size_t n = a.size();
 if( n == 0 ) return( out );
 // m = a 4^t with an even number of digits, its top one at least b / 4;
 // the root of a is that of m shifted by t
 const unsigned c = unsigned( std::countl_zero( a.back() ) ) / 2 ,
                z = n % 2;                  // a digit 0 below
 const std::size_t t = c + 32 * z , N = n + z;
 Limb in[ 2 * kSqrtSmall ];                 // m on the stack when short
 Mag heap( N <= 2 * kSqrtSmall ? 0 : N );
 Limb * const m = N <= 2 * kSqrtSmall ? in : heap.data();
 std::fill( m , m + N , 0 );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  m[ i + z ] |= c ? a[ i ] << ( 2 * c ) : a[ i ];
  if( c && ( i + 1 < n ) ) m[ i + 1 + z ] = a[ i ] >> ( 64 - 2 * c );
  }
 Mag & s = out.first , & r = out.second;
 sqrtRec( m , N / 2 , s , r , ! rem );
 if( t ) {
  // with the root s_m = s 2^t + s0 of m = a 4^t and its remainder r_m,
  // a - s^2 = ( r_m + 2 s0 s_m - s0^2 ) / 4^t, s0 < 2^t <= 2^63
  const Limb s0 = s[ 0 ] & ( ( Limb( 1 ) << t ) - 1 );
  if( rem && s0 ) {
   const std::size_t n2 = std::max( r.size() , s.size() ) + 2;
   r.resize( n2 , 0 );
   const Limb e = addMul1( r.data() , s.data() , s.size() , 2 * s0 );
   addTo( r.data() + s.size() , n2 - s.size() , &e , 1 );
   Limb h;
   const Limb l2[ 2 ] = { detail::mulWide( s0 , s0 , h ) , h };
   subFrom( r.data() , n2 , l2 , 2 );
   }
  auto shr = []( Mag & x , std::size_t sh ) {   // r >> 2 t , s >> t
   const std::size_t w = sh / 64 , b = sh % 64;
   if( w >= x.size() ) {
    x.resize( 0 );
    return;
    }
   for( std::size_t i = 0 ; i + w < x.size() ; ++i )
    x[ i ] = b ? ( x[ i + w ] >> b ) |
                 ( i + w + 1 < x.size() ? x[ i + w + 1 ] << ( 64 - b ) : 0 )
               : x[ i + w ];
   x.resize( x.size() - w );
   trim( x );
   };
  if( rem ) shr( r , 2 * t );
  shr( s , t );
  }
 if( ! rem ) r.resize( 0 );
 return( out );
 }

inline std::pair< BigInteger , BigInteger > BigInteger::sqrtRem(
                                                       const BigInteger & a )
{
 ADS_PRE( a.sign() >= 0 );
 auto [ s , r ] = sqrtMag( a.f_d , true );
 std::pair< BigInteger , BigInteger > out(
                                       BigInteger( false , std::move( s ) ) ,
                                       BigInteger( false , std::move( r ) ) );
 ADS_AUDIT( ( out.first * out.first + out.second == a ) &&
            ( out.second <= 2 * out.first ) );
 ADS_AUDIT( out.first == sqrtNewton( a ) );
 return( out );
 }

inline BigInteger BigInteger::sqrt( const BigInteger & a )
{
 ADS_PRE( a.sign() >= 0 );
 BigInteger s( false , sqrtMag( a.f_d , false ).first );
 ADS_AUDIT( s == sqrtNewton( a ) );
 return( s );
 }

inline BigInteger BigInteger::sqrtNewton( const BigInteger & a )
{
 ADS_PRE( a.sign() >= 0 );
 if( a.sign() == 0 ) return( a );
 // from 2^ceil( bits / 2 ), above the root, x = ( x + a / x ) / 2 while
 // it decreases
 const std::size_t bits = 64 * a.f_d.size() -
                          std::size_t( std::countl_zero( a.f_d.back() ) );
 BigInteger x = pow( 2 , ( bits + 1 ) / 2 );
 for( ; ; ) {
  BigInteger y = x + a / x;
  Mag d = y.f_d;
  divSmall( d , 2 );
  y = BigInteger( false , std::move( d ) );
  if( y >= x ) return( x );
  x = std::move( y );
  }
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

inline BigInteger::HMat::HMat( std::size_t len )
 : d( 4 * ( ( len + 1 ) / 2 + 2 ) , 0 ) , n( 1 ) ,
   alloc( ( len + 1 ) / 2 + 2 )
{
 for( std::size_t i = 0 ; i < 4 ; ++i ) p[ i ] = d.data() + i * alloc;
 p[ 0 ][ 0 ] = p[ 3 ][ 0 ] = 1;             // the identity
 }

namespace detail {

/// ( a ; b ) = ( w3 a + w1 b' + c1 ; w2 a' + w0 b + c2 ) in place, n > 0
/// digits, w0 , w1 , w2 , w3 below 2^63, x' = ~x if inv, else x, the
/// carries out in c1 and c2: each row the sum of two products by one digit
/// and its carry, below 2^128
template< bool inv >
inline void rows( std::uint64_t * a , std::uint64_t * b , std::size_t n ,
                  const std::uint64_t * w , std::uint64_t & c1 ,
                  std::uint64_t & c2 )
{
#if ADS_BIGINT_ASM
 rowsAsm< inv >( a , b , n , w , c1 , c2 );
#else
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::uint64_t x = a[ i ] , y = b[ i ];
  std::uint64_t h1 , h2 , h3 , h4;
  const std::uint64_t l1 = mulAdd( x , w[ 3 ] , c1 , 0 , h1 ) ,
                      l2 = mulAdd( y , w[ 0 ] , c2 , 0 , h3 );
  a[ i ] = mulAdd( inv ? ~y : y , w[ 1 ] , l1 , 0 , h2 );
  b[ i ] = mulAdd( inv ? ~x : x , w[ 2 ] , l2 , 0 , h4 );
  c1 = h1 + h2;
  c2 = h3 + h4;
  }
#endif
 }

/// the quotient of n0 by d0, n0 >= d0 > 0, the remainder left in n0: up
/// to 7 by three trial subtractions with no branch, else by a division
inline std::uint64_t div11( std::uint64_t & n0 , std::uint64_t d0 )
{
 if( ( ( d0 >> 61 ) == 0 ) && ( n0 < ( d0 << 3 ) ) ) {
  std::uint64_t d = d0 << 2 , m = 0 - std::uint64_t( n0 >= d ) , q = 4 & m;
  n0 -= d & m;
  d >>= 1;
  m = 0 - std::uint64_t( n0 >= d );
  n0 -= d & m;
  q += 2 & m;
  d >>= 1;
  m = 0 - std::uint64_t( n0 >= d );
  n0 -= d & m;
  return( q - m );
  }
 const std::uint64_t q = n0 / d0;
 n0 -= q * d0;
 return( q );
 }

/// the quotient of ( n1 , n0 ) by ( d1 , d0 ), n >= d, d1 > 0, the
/// remainder left in ( n1 , n0 ): a bit at a time from the difference
/// of the leading bits, with no branch but the loop
inline std::uint64_t div22( std::uint64_t & n1 , std::uint64_t & n0 ,
                            std::uint64_t d1 , std::uint64_t d0 )
{
 int k = std::countl_zero( d1 ) - std::countl_zero( n1 );
 if( k > 0 ) {
  d1 = ( d1 << k ) | ( d0 >> ( 64 - k ) );
  d0 <<= k;
  }
 std::uint64_t q = 0;
 for( ; ; ) {
  const bool ge = ( n1 > d1 ) || ( ( n1 == d1 ) && ( n0 >= d0 ) );
  const std::uint64_t m = 0 - std::uint64_t( ge );
  std::uint64_t c = 0;
  n0 = subBorrow( n0 , d0 & m , c );
  n1 -= ( d1 & m ) + c;
  q = ( q << 1 ) - m;
  if( k-- <= 0 ) break;
  d0 = ( d0 >> 1 ) | ( d1 << 63 );
  d1 >>= 1;
  }
 return( q );
 }

}  // end( namespace detail )

inline bool BigInteger::hgcd2( Limb ah , Limb al , Limb bh , Limb bl ,
                               HMat1 & M )
{
 // Euclid on ( a , b ), each remainder by one subtraction and, if it is
 // still the larger, a division, the quotient that would leave less than
 // 2^65 (the high digit below 2) taken one less and the steps stopped
 // there. With the remainders at least 2^65 the quotients are those of
 // the numbers whose leading digits these are, and so on two digits for
 // the leading digit alone with the remainders at least 2^33 (Moeller):
 // the steps go on ah and bh alone, down to 2^33, then their matrix
 // reduces ( a , b ), whose leading 64 bits go down to 2^33 again; when
 // ah and bh allow no step, on two digits down to a high digit of 2^32,
 // then on the top 96 bits in one digit. The steps on a and on b are
 // written apart, so that all stays in registers
 constexpr Limb kHalf = Limb( 1 ) << 32 , kStop = Limb( 1 ) << 33;
 if( ( ah < 2 ) || ( bh < 2 ) ) return( false );
 // x -= q y on one digit, the quotient into the column ( c0 , c1 ) of M
 // from the other ( o0 , o1 ), as x -= q y is M times ( 1 q ; 0 1 ) for
 // x = a and ( 1 0 ; q 1 ) for x = b; true to stop
 auto one = []( Limb & x , Limb y , Limb & c0 , Limb & c1 , Limb o0 ,
                Limb o1 ) {
  x -= y;
  if( x < kStop ) return( true );
  Limb q = 1;
  bool r = false;
  if( x > y ) {
   q = detail::div11( x , y );
   if( x < kStop ) r = true;                // q right, the remainder small
   else ++q;
   }
  c0 += q * o0;
  c1 += q * o1;
  return( r );
  };
 // Euclid on x and y of one digit into u, from the larger; false if no
 // step was taken
 auto euclid = [ & one ]( Limb x , Limb y , Limb * u ) {
  u[ 0 ] = u[ 3 ] = 1;
  u[ 1 ] = u[ 2 ] = 0;
  bool onB = x < y;
  for( bool first = true ; ; first = false ) {
   if( ! onB ) {
    if( one( x , y , u[ 1 ] , u[ 3 ] , u[ 0 ] , u[ 2 ] ) ) return( ! first );
    first = false;
    }
   if( one( y , x , u[ 0 ] , u[ 2 ] , u[ 1 ] , u[ 3 ] ) ) return( ! first );
   onB = false;
   }
  };
 Limb u[ 4 ];
 if( ( ah >= kStop ) && ( bh >= kStop ) && euclid( ah , bh , u ) ) {
  // ( a ; b ) = M1^{-1} ( a ; b ) modulo 2^128, exact as the results are
  // nonnegative and below 2^128; then the leading 64 bits of both
  Limb h1 , h2 , h3 , h4;
  const Limb l1 = detail::mulWide( al , u[ 3 ] , h1 ) ,
             l2 = detail::mulWide( bl , u[ 1 ] , h2 ) ,
             l3 = detail::mulWide( bl , u[ 0 ] , h3 ) ,
             l4 = detail::mulWide( al , u[ 2 ] , h4 );
  Limb c = 0 , d = 0;
  const Limb xl = detail::subBorrow( l1 , l2 , c ) ,
             yl = detail::subBorrow( l3 , l4 , d ) ,
             xh = h1 + ah * u[ 3 ] - h2 - bh * u[ 1 ] - c ,
             yh = h3 + bh * u[ 0 ] - h4 - ah * u[ 2 ] - d;
  const int k = std::countl_zero( xh | yh );
  Limb v[ 4 ];
  if( ( k < 64 - 33 ) &&
      euclid( k ? ( xh << k ) | ( xl >> ( 64 - k ) ) : xh ,
              k ? ( yh << k ) | ( yl >> ( 64 - k ) ) : yh , v ) ) {
   M.u[ 0 ] = u[ 0 ] * v[ 0 ] + u[ 1 ] * v[ 2 ];   // M1 M2
   M.u[ 1 ] = u[ 0 ] * v[ 1 ] + u[ 1 ] * v[ 3 ];
   M.u[ 2 ] = u[ 2 ] * v[ 0 ] + u[ 3 ] * v[ 2 ];
   M.u[ 3 ] = u[ 2 ] * v[ 1 ] + u[ 3 ] * v[ 3 ];
   }
  else std::copy( u , u + 4 , M.u );
  return( true );
  }
 Limb u00 = 1 , u01 = 0 , u10 = 0 , u11 = 1;
 // x -= q y on two digits: 0 to go on, 1 to stop, 2 to go on in one digit
 // with the same x
 auto two = []( Limb & xh , Limb & xl , Limb yh , Limb yl , Limb & c0 ,
                Limb & c1 , Limb o0 , Limb o1 ) {
  if( xh == yh ) return( 1 );
  if( xh < kHalf ) return( 2 );
  Limb c = 0 , q = 1;
  xl = detail::subBorrow( xl , yl , c );
  xh -= yh + c;
  if( xh < 2 ) return( 1 );
  int r = 0;
  if( xh > yh ) {
   q = detail::div22( xh , xl , yh , yl );
   if( xh < 2 ) r = 1;
   else ++q;
   }
  c0 += q * o0;
  c1 += q * o1;
  return( r );
  };
 bool onB;                                  // the next step reduces b
 {
  Limb c = 0;
  if( ( ah > bh ) || ( ( ah == bh ) && ( al > bl ) ) ) {
   al = detail::subBorrow( al , bl , c );
   ah -= bh + c;
   if( ah < 2 ) return( false );
   u01 = 1;
   }
  else {
   bl = detail::subBorrow( bl , al , c );
   bh -= ah + c;
   if( bh < 2 ) return( false );
   u10 = 1;
   }
  onB = ah < bh;
  }
 int r = 0;
 for( ; ; ) {
  if( ! onB ) {
   if( ( r = two( ah , al , bh , bl , u01 , u11 , u00 , u10 ) ) ) break;
   }
  if( ( r = two( bh , bl , ah , al , u00 , u10 , u01 , u11 ) ) ) {
   onB = true;
   break;
   }
  onB = false;
  }
 if( r == 2 ) {                             // to the top 96 bits
  ah = ( ah << 32 ) | ( al >> 32 );
  bh = ( bh << 32 ) | ( bl >> 32 );
  for( ; ; ) {
   if( ! onB && one( ah , bh , u01 , u11 , u00 , u10 ) ) break;
   if( one( bh , ah , u00 , u10 , u01 , u11 ) ) break;
   onB = false;
   }
  }
 M.u[ 0 ] = u00;
 M.u[ 1 ] = u01;
 M.u[ 2 ] = u10;
 M.u[ 3 ] = u11;
 return( true );
 }

inline std::size_t BigInteger::hgcdInv1( const HMat1 & M , Limb * a ,
                                         Limb * b , std::size_t n )
{
 // a = u11 a - u01 b and b = u00 b - u10 a, as M^{-1} = ( u11 -u01 ;
 // -u10 u00 ), both nonnegative and below b^n, in one pass with no
 // subtraction: -b = ~b + 1 - b^n, so a = u11 a + u01 ~b + u01 modulo
 // b^n, a sum below 2^128 per digit as u11 + u01 < 2^64, the carry u01
 // out of the top; the same for b
 Limb c1 = M.u[ 1 ] , c2 = M.u[ 2 ];
 detail::rows< true >( a , b , n , M.u , c1 , c2 );
 ADS_AUDIT( ( c1 == M.u[ 1 ] ) && ( c2 == M.u[ 2 ] ) );
 return( n - ( ( a[ n - 1 ] | b[ n - 1 ] ) == 0 ) );
 }

inline void BigInteger::hgcdMul1( HMat & M , const HMat1 & M1 )
{
 // each row ( x , y ) of M to ( u00 x + u10 y , u01 x + u11 y )
 const std::size_t n = M.n;
 const Limb w[ 4 ] = { M1.u[ 3 ] , M1.u[ 2 ] , M1.u[ 1 ] , M1.u[ 0 ] };
 bool grow = false;
 for( std::size_t r = 0 ; r < 2 ; ++r ) {
  Limb * const x = M.p[ 2 * r ] , * const y = M.p[ 2 * r + 1 ];
  Limb c1 = 0 , c2 = 0;
  detail::rows< false >( x , y , n , w , c1 , c2 );
  x[ n ] = c1;
  y[ n ] = c2;
  grow = grow || ( c1 | c2 );
  }
 M.n = n + grow;
 ADS_AUDIT( M.n < M.alloc );
 }

inline void BigInteger::hgcdUpdate( HMat & M , const Limb * q ,
                                    std::size_t qn , unsigned c )
{
 while( ( qn > 0 ) && ( q[ qn - 1 ] == 0 ) ) --qn;
 if( qn == 0 ) return;
 Limb * const t0 = M.p[ c ] , * const t1 = M.p[ 2 + c ];
 const Limb * const o0 = M.p[ 1 - c ] , * const o1 = M.p[ 3 - c ];
 if( qn == 1 ) {
  const Limb e0 = addMul1( t0 , o0 , M.n , q[ 0 ] ) ,
             e1 = addMul1( t1 , o1 , M.n , q[ 0 ] );
  t0[ M.n ] = e0;
  t1[ M.n ] = e1;
  M.n += ( e0 | e1 ) != 0;
  return;
  }
 // the other column cut to its digits, as the product need not grow by qn
 std::size_t n = M.n;
 for( ; n + qn > M.n ; --n )
  if( o0[ n - 1 ] | o1[ n - 1 ] ) break;
 ADS_AUDIT( n + qn < M.alloc );
 Mag t( n + qn );
 Limb e[ 2 ];
 for( std::size_t r = 0 ; r < 2 ; ++r ) {
  mulP( t.data() , r ? o1 : o0 , n , q , qn );
  Limb * const x = r ? t1 : t0;
  e[ r ] = addN( x , t.data() , n + qn , x , M.n );
  }
 n += qn;
 if( e[ 0 ] | e[ 1 ] ) {
  t0[ n ] = e[ 0 ];
  t1[ n ] = e[ 1 ];
  ++n;
  }
 else n -= ( t0[ n - 1 ] | t1[ n - 1 ] ) == 0;
 M.n = n;
 ADS_AUDIT( M.n < M.alloc );
 }

inline void BigInteger::hgcdMul( HMat & M , const HMat & M1 )
{
 // ( x y ) of each row of M times M1, x u00 + y u10 and x u01 + y u11
 const std::size_t n = M.n , m = M1.n , k = n + m + 1;
 ADS_AUDIT( k <= M.alloc );
 if( std::min( n , m ) >= kHgcdMul7 ) {
  hgcdMul7( M , M1 );
  return;
  }
 Mag t( 3 * k );
 Limb * const s0 = t.data() , * const s1 = s0 + k , * const w = s1 + k;
 for( std::size_t r = 0 ; r < 2 ; ++r ) {
  Limb * const x = M.p[ 2 * r ] , * const y = M.p[ 2 * r + 1 ];
  for( std::size_t j = 0 ; j < 2 ; ++j ) {
   Limb * const z = j ? s1 : s0;
   mulP( z , x , n , M1.p[ j ] , m );
   mulP( w , y , n , M1.p[ 2 + j ] , m );
   z[ n + m ] = addTo( z , n + m , w , n + m );
   }
  std::copy( s0 , s0 + k , x );
  std::copy( s1 , s1 + k , y );
  }
 std::size_t e = k;                         // up to 3 digits 0 on top
 while( ( e > 1 ) && ( ( M.p[ 0 ][ e - 1 ] | M.p[ 1 ][ e - 1 ] |
                         M.p[ 2 ][ e - 1 ] | M.p[ 3 ][ e - 1 ] ) == 0 ) )
  --e;
 M.n = e;
 }

inline void BigInteger::hgcdMul7( HMat & M , const HMat & M1 )
{
 // Winograd's form of Strassen's product, 7 products instead of 8: with
 // M = ( a b ; c d ) and M1 = ( e f ; g h ), s1 = c + d, s2 = s1 - a,
 // s3 = a - c, s4 = b - s2, t1 = f - e, t2 = h - t1, t3 = h - f,
 // t4 = t2 - g, p1 = a e, p2 = b g, p3 = s4 h, p4 = d t4, p5 = s1 t1,
 // p6 = s2 t2, p7 = s3 t3, u2 = p1 + p6, u3 = u2 + p7, the result
 // ( p1 + p2 , u2 + p5 + p3 ; u3 - p4 , u3 + p5 ); the numbers between
 // signed, as magnitudes of K digits and their signs
 const std::size_t n = M.n , m = M1.n , K = n + m + 3;
 Mag buf( 16 * K , 0 );
 struct S { Limb * d; bool neg; };
 std::size_t used = 0;
 auto fresh = [ & ]( void ) {
  S x{ buf.data() + used , false };
  used += K;
  return( x );
  };
 auto load = [ & ]( const Limb * x , std::size_t len ) {
  S y = fresh();
  std::copy( x , x + len , y.d );
  return( y );
  };
 // r = x + y or x - y (sub), all K digits
 auto add = [ & ]( S & r , const S & x , const S & y , bool sub ) {
  const bool yn = y.neg != sub;
  if( x.neg == yn ) {
   addN( r.d , x.d , K , y.d , K );
   r.neg = x.neg;
   }
  else {                                   // the signs differ
   const bool less = absDiff( r.d , x.d , K , y.d , K );
   r.neg = less ? yn : x.neg;
   }
  };
 const S a = load( M.p[ 0 ] , n ) , b = load( M.p[ 1 ] , n ) ,
         c = load( M.p[ 2 ] , n ) , d = load( M.p[ 3 ] , n ) ,
         e = load( M1.p[ 0 ] , m ) , f = load( M1.p[ 1 ] , m ) ,
         g = load( M1.p[ 2 ] , m ) , h = load( M1.p[ 3 ] , m );
 S s1 = fresh() , s2 = fresh() , s3 = fresh() , s4 = fresh() ,
   t1 = fresh() , t2 = fresh() , t3 = fresh() , t4 = fresh();
 add( s1 , c , d , false );
 add( s2 , s1 , a , true );
 add( s3 , a , c , true );
 add( s4 , b , s2 , true );
 add( t1 , f , e , true );
 add( t2 , h , t1 , true );
 add( t3 , h , f , true );
 add( t4 , t2 , g , true );
 Mag pb( 7 * K , 0 );                      // the products
 auto into = [ & ]( std::size_t i , const S & x , std::size_t lx ,
                    const S & y , std::size_t ly ) {
  S r{ pb.data() + i * K , x.neg != y.neg };
  mulP( r.d , x.d , lx , y.d , ly );
  return( r );
  };
 const std::size_t n1 = n + 1 , m1 = m + 1;
 S p1 = into( 0 , a , n , e , m ) , p2 = into( 1 , b , n , g , m ) ,
   p3 = into( 2 , s4 , n1 , h , m ) , p4 = into( 3 , d , n , t4 , m1 ) ,
   p5 = into( 4 , s1 , n1 , t1 , m ) , p6 = into( 5 , s2 , n1 , t2 , m1 ) ,
   p7 = into( 6 , s3 , n , t3 , m );
 S u2 = s1 , u3 = s2;                       // the room of s1 and s2 again
 add( u2 , p1 , p6 , false );
 add( u3 , u2 , p7 , false );
 S c00 = s3 , c01 = s4 , c10 = t1 , c11 = t2;
 add( c00 , p1 , p2 , false );
 add( c11 , u3 , p5 , false );
 add( u2 , u2 , p5 , false );
 add( c01 , u2 , p3 , false );
 add( c10 , u3 , p4 , true );
 ADS_AUDIT( ! c00.neg && ! c01.neg && ! c10.neg && ! c11.neg );
 const std::size_t k = n + m + 1;
 std::copy( c00.d , c00.d + k , M.p[ 0 ] );
 std::copy( c01.d , c01.d + k , M.p[ 1 ] );
 std::copy( c10.d , c10.d + k , M.p[ 2 ] );
 std::copy( c11.d , c11.d + k , M.p[ 3 ] );
 std::size_t z = k;                         // up to 3 digits 0 on top
 while( ( z > 1 ) && ( ( M.p[ 0 ][ z - 1 ] | M.p[ 1 ][ z - 1 ] |
                         M.p[ 2 ][ z - 1 ] | M.p[ 3 ][ z - 1 ] ) == 0 ) )
  --z;
 M.n = z;
 }

inline std::size_t BigInteger::hgcdAdjust( const HMat & M , std::size_t n ,
                                           Limb * a , Limb * b ,
                                           std::size_t p )
{
 // with a = A b^p + a_l, b = B b^p + b_l and ( A ; B ) already reduced:
 // a = A b^p + u11 a_l - u01 b_l and b = B b^p + u00 b_l - u10 a_l
 const std::size_t m = M.n , l = p + m;
 ADS_AUDIT( l < n );
 Mag t( 2 * l );
 Limb * const t0 = t.data() , * const t1 = t0 + l;
 mulP( t0 , a , p , M.p[ 3 ] , m );
 mulP( t1 , a , p , M.p[ 2 ] , m );
 std::copy( t0 , t0 + p , a );
 Limb ah = addTo( a + p , n - p , t0 + p , m );
 mulP( t0 , b , p , M.p[ 1 ] , m );
 ah -= subFrom( a , n , t0 , l );
 mulP( t0 , b , p , M.p[ 0 ] , m );
 std::copy( t0 , t0 + p , b );
 Limb bh = addTo( b + p , n - p , t0 + p , m );
 bh -= subFrom( b , n , t1 , l );
 if( ah | bh ) {
  a[ n ] = ah;
  b[ n ] = bh;
  return( n + 1 );
  }
 return( n - ( ( a[ n - 1 ] | b[ n - 1 ] ) == 0 ) );
 }

inline std::size_t BigInteger::gcdSubdiv( Limb * a , Limb * b , std::size_t n ,
                                          std::size_t s , HMat * M , Mag * g )
{
 // a and b swapped as needed so that a < b; the quotients go to the
 // column of M of the one reduced, 0 for b and 1 for a [see hgcdUpdate()]
 auto len = []( const Limb * x , std::size_t k ) {
  while( ( k > 0 ) && ( x[ k - 1 ] == 0 ) ) --k;
  return( k );
  };
 std::size_t an = len( a , n ) , bn = len( b , n );
 unsigned sw = 0;
 auto cmp = [ & ]( void ) {
  return( an != bn ? ( an < bn ? -1 : 1 ) : cmpN( a , b , an ) );
  };
 auto swap = [ & ]( void ) {
  std::swap( a , b );
  std::swap( an , bn );
  sw ^= 1;
  };
 int c = cmp();
 if( c == 0 ) {
  if( s == 0 ) *g = Mag( a , a + an );
  return( 0 );
  }
 if( c > 0 ) swap();
 if( an <= s ) {
  if( s == 0 ) *g = Mag( b , b + bn );
  return( 0 );
  }
 subFrom( b , bn , a , an );                // b -= a
 bn = len( b , bn );
 if( bn <= s ) {                            // undone
  const Limb e = addN( b , a , an , b , bn );
  if( e ) b[ an ] = e;
  return( 0 );
  }
 const Limb one = 1;
 c = cmp();
 if( c == 0 ) {
  if( s == 0 ) *g = Mag( b , b + bn );
  else hgcdUpdate( *M , &one , 1 , sw );
  return( 0 );
  }
 if( M ) hgcdUpdate( *M , &one , 1 , sw );
 if( c > 0 ) swap();
 auto [ q , r ] = divModMag( Mag( b , b + bn ) , Mag( a , a + an ) );
 std::copy( r.begin() , r.end() , b );      // b = b mod a
 std::fill( b + r.size() , b + bn , 0 );
 bn = r.size();
 if( bn <= s ) {
  if( s == 0 ) {
   *g = Mag( a , a + an );
   return( 0 );
   }
  if( bn > 0 ) {                            // the quotient one less
   const Limb e = addN( b , a , an , b , bn );
   if( e ) b[ an++ ] = e;
   }
  else std::copy( a , a + an , b );
  q = subMag( q , Mag{ 1 } );
  }
 if( M ) hgcdUpdate( *M , q.data() , q.size() , sw );
 return( an );
 }

inline std::size_t BigInteger::hgcdStep( std::size_t n , Limb * a , Limb * b ,
                                         std::size_t s , HMat & M )
{
 const Limb mask = a[ n - 1 ] | b[ n - 1 ];
 Limb ah , al , bh , bl;
 bool top = true;
 if( ( n == s + 1 ) || ( mask >> 63 ) ) {
  top = ( n > s + 1 ) || ( mask >= 4 );
  ah = a[ n - 1 ];
  al = a[ n - 2 ];
  bh = b[ n - 1 ];
  bl = b[ n - 2 ];
  }
 else {                                     // the leading two digits
  const int c = std::countl_zero( mask );
  ah = ( a[ n - 1 ] << c ) | ( a[ n - 2 ] >> ( 64 - c ) );
  al = ( a[ n - 2 ] << c ) | ( a[ n - 3 ] >> ( 64 - c ) );
  bh = ( b[ n - 1 ] << c ) | ( b[ n - 2 ] >> ( 64 - c ) );
  bl = ( b[ n - 2 ] << c ) | ( b[ n - 3 ] >> ( 64 - c ) );
  }
 HMat1 M1;
 if( top && hgcd2( ah , al , bh , bl , M1 ) ) {
  hgcdMul1( M , M1 );
  return( hgcdInv1( M1 , a , b , n ) );
  }
 return( gcdSubdiv( a , b , n , s , &M , nullptr ) );
 }

inline std::size_t BigInteger::hgcd( Limb * a , Limb * b , std::size_t n ,
                                     HMat & M )
{
 // Moeller's half gcd: the top half reduced by a recursive call and the
 // low digits by its matrix, a few steps, then the top of what is left
 // by a second recursive call, whose matrix M multiplies, and the steps
 // that keep |a - b| above s digits
 const std::size_t s = n / 2 + 1;
 if( n <= s ) return( 0 );
 bool ok = false;
 if( n >= kHgcd ) {
  const std::size_t n2 = 3 * n / 4 + 1 , p = n / 2;
  std::size_t nn = hgcd( a + p , b + p , n - p , M );
  if( nn ) {
   n = hgcdAdjust( M , p + nn , a , b , p );
   ok = true;
   }
  while( n > n2 ) {
   nn = hgcdStep( n , a , b , s , M );
   if( ! nn ) return( ok ? n : 0 );
   n = nn;
   ok = true;
   }
  if( n > s + 2 ) {
   const std::size_t p2 = 2 * s - n + 1;
   HMat M1( n - p2 );
   nn = hgcd( a + p2 , b + p2 , n - p2 , M1 );
   if( nn ) {
    n = hgcdAdjust( M1 , p2 + nn , a , b , p2 );
    hgcdMul( M , M1 );
    ok = true;
    }
   }
  }
 for( ; ; ) {
  const std::size_t nn = hgcdStep( n , a , b , s , M );
  if( ! nn ) return( ok ? n : 0 );
  n = nn;
  ok = true;
  }
 }

inline BigInteger::Mag BigInteger::gcd22( Limb u1 , Limb u0 , Limb v1 ,
                                          Limb v0 )
{
 // the binary gcd on two digits down to gcdWord() when both fit in one
 auto make = []( Limb h , Limb l ) {
  Mag m;
  if( h ) m = Mag{ l , h };
  else if( l ) m = Mag{ l };
  return( m );
  };
 if( ( u1 | u0 ) == 0 ) return( make( v1 , v0 ) );
 if( ( v1 | v0 ) == 0 ) return( make( u1 , u0 ) );
 auto tz = []( Limb h , Limb l ) {
  return( l ? std::countr_zero( l ) : 64 + std::countr_zero( h ) );
  };
 auto shr = []( Limb & h , Limb & l , int k ) {
  if( k >= 64 ) {
   l = h >> ( k - 64 );
   h = 0;
   }
  else if( k > 0 ) {
   l = ( l >> k ) | ( h << ( 64 - k ) );
   h >>= k;
   }
  };
 const int z = std::min( tz( u1 , u0 ) , tz( v1 , v0 ) );
 shr( u1 , u0 , tz( u1 , u0 ) );            // u odd from here on
 while( ( u1 | v1 ) && ( v1 | v0 ) ) {
  shr( v1 , v0 , tz( v1 , v0 ) );
  // ( u , v ) = ( min , |v - u| ) with no branch, by the mask of the
  // borrow of v - u
  Limb c = 0;
  const Limb d0 = detail::subBorrow( v0 , u0 , c ) ,
             d1 = detail::subBorrow( v1 , u1 , c ) , m = 0 - c ,
             e = Limb( d0 != 0 );
  u0 ^= ( u0 ^ v0 ) & m;                    // min( u , v )
  u1 ^= ( u1 ^ v1 ) & m;
  v0 = ( d0 ^ m ) - m;                      // |v - u|
  v1 = ( d1 ^ m ) + ( m & ( 1 - e ) );
  }
 Limb h = u1 , l = u0;
 if( ( u1 | v1 ) == 0 ) l = gcdWord( u0 , v0 );   // v = 0 too
 if( z >= 64 ) {
  h = l << ( z - 64 );
  l = 0;
  }
 else if( z > 0 ) {
  h = ( h << z ) | ( l >> ( 64 - z ) );
  l <<= z;
  }
 return( make( h , l ) );
 }

inline BigInteger::Mag BigInteger::gcdMag( const Mag & x , const Mag & y )
{
 const bool sw = compareMag( x , y ) < 0;
 const Mag & u0 = sw ? y : x , & v = sw ? x : y;
 if( v.empty() ) return( u0 );
 if( v.size() == 1 )
  return( Mag{ gcdLimb( u0.data() , u0.size() , v[ 0 ] ) } );
 if( ( u0.size() <= 2 ) )                   // both of two digits
  return( gcd22( u0[ 1 ] , u0[ 0 ] , v[ 1 ] , v[ 0 ] ) );
 Mag r;                                     // u = u mod v first
 if( u0.size() > v.size() ) {
  r = divModMag( u0 , v ).second;
  if( r.empty() ) return( v );
  }
 const Mag & u = u0.size() > v.size() ? r : u0;
 std::size_t n = v.size();
 Mag A( n + 1 , 0 ) , B( n + 1 , 0 );
 std::copy( u.begin() , u.end() , A.data() );
 std::copy( v.begin() , v.end() , B.data() );
 Limb * const a = A.data() , * const b = B.data();
 Mag g;
 while( n >= kGcdDC ) {                     // the half gcd of the top 1/3
  const std::size_t p = 2 * n / 3;
  HMat M( n - p );
  const std::size_t nn = hgcd( a + p , b + p , n - p , M );
  if( nn ) n = hgcdAdjust( M , p + nn , a , b , p );
  else if( ! ( n = gcdSubdiv( a , b , n , 0 , nullptr , &g ) ) ) return( g );
  }
 while( n > 2 ) {                           // Lehmer, two digits a step
  const Limb mask = a[ n - 1 ] | b[ n - 1 ];
  Limb ah = a[ n - 1 ] , al = a[ n - 2 ] , bh = b[ n - 1 ] , bl = b[ n - 2 ];
  if( ! ( mask >> 63 ) ) {
   const int c = std::countl_zero( mask );
   ah = ( ah << c ) | ( al >> ( 64 - c ) );
   al = ( al << c ) | ( a[ n - 3 ] >> ( 64 - c ) );
   bh = ( bh << c ) | ( bl >> ( 64 - c ) );
   bl = ( bl << c ) | ( b[ n - 3 ] >> ( 64 - c ) );
   }
  HMat1 M;
  if( hgcd2( ah , al , bh , bl , M ) ) n = hgcdInv1( M , a , b , n );
  else if( ! ( n = gcdSubdiv( a , b , n , 0 , nullptr , &g ) ) ) return( g );
  }
 return( gcd22( n > 1 ? a[ 1 ] : 0 , a[ 0 ] , n > 1 ? b[ 1 ] : 0 , b[ 0 ] ) );
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
