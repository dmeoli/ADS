/*--------------------------------------------------------------------------*/
/*----------------------- File ArithmeticCoding.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions and the classes of ArithmeticCoding.h, with their
 * contracts.
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

/// the cumulative probabilities of prob, after checking that they are
/// positive with sum 1
template< class C >
std::map< C , Rational > cumulative( const std::map< C , Rational > & prob )
{
 std::map< C , Rational > f;
 Rational sum( 0 );
 for( const auto & [ c , p ] : prob ) {
  ADS_PRE( p > Rational( 0 ) );
  f[ c ] = sum;
  sum = sum + p;
  }
 ADS_PRE( sum == Rational( 1 ) );
 return( f );
 }

/// the value of the bits, a string of '0' and '1', as the dyadic fraction
/// 0.b_1 b_2 ... b_k
inline Rational dyadic( const std::string & bits )
{
 if( bits.empty() ) return( Rational( 0 ) );
 return( Rational( BigInteger( bits , 2 ) ,
                   BigInteger::pow( BigInteger( 2 ) , bits.size() ) ) );
 }

/// M = 2^w, after checking w
inline std::uint64_t rangeSize( int w )
{
 ADS_PRE( ( w >= 2 ) && ( w <= 32 ) );
 return( std::uint64_t( 1 ) << w );
 }

/// the counts of the symbols in a tree of Fenwick: f_t[ i ] is the sum of
/// the counts of the ( i & -i ) symbols up to the i-th, from 1
class FrequencyTree {

 public:

 /// sigma symbols, each with count 1
 explicit FrequencyTree( std::size_t sigma )
  : f_t( sigma + 1 , 0 ) , f_count( sigma , 1 ) { rebuild(); }

 [[nodiscard]] std::uint64_t total( void ) const { return( f_total ); }

 [[nodiscard]] std::uint64_t count( std::size_t i ) const {
  return( f_count[ i ] );
  }

 /// the sum of the counts of the symbols below i
 [[nodiscard]] std::uint64_t prefix( std::size_t i ) const {
  std::uint64_t s = 0;
  for( ; i > 0 ; i &= i - 1 ) s += f_t[ i ];
  return( s );
  }

 /// adds d to the count of i
 void add( std::size_t i , std::uint64_t d ) {
  f_count[ i ] += d;
  f_total += d;
  for( std::size_t j = i + 1 ; j < f_t.size() ; j += j & ( ~j + 1 ) )
   f_t[ j ] += d;
  }

 /// the symbol i with prefix( i ) <= t < prefix( i + 1 )
 [[nodiscard]] std::size_t find( std::uint64_t t ) const {
  std::size_t pos = 0 , step = 1;
  while( 2 * step < f_t.size() ) step *= 2;
  for( ; step ; step /= 2 )
   if( ( pos + step < f_t.size() ) && ( f_t[ pos + step ] <= t ) ) {
    pos += step;
    t -= f_t[ pos ];
    }
  return( pos );
  }

 /// halves the counts, rounding up
 void halve( void ) {
  for( auto & c : f_count ) c = c / 2 + c % 2;
  rebuild();
  }

 private:

 void rebuild( void ) {
  f_total = 0;
  std::fill( f_t.begin() , f_t.end() , 0 );
  for( std::size_t i = 0 ; i < f_count.size() ; ++i ) {
   f_total += f_count[ i ];
   f_t[ i + 1 ] += f_count[ i ];
   const std::size_t j = ( i + 1 ) + ( ( i + 1 ) & ( ~( i + 1 ) + 1 ) );
   if( j < f_t.size() ) f_t[ j ] += f_t[ i + 1 ];
   }
  }

 std::vector< std::uint64_t > f_t;       ///< the tree, from 1
 std::vector< std::uint64_t > f_count;   ///< the counts
 std::uint64_t f_total = 0;

 };  // end( class FrequencyTree )

/// the total of the counts beyond which the adaptive model halves them
inline std::uint64_t adaptiveLimit( std::uint32_t sigma )
{
 return( std::max< std::uint64_t >( std::uint64_t( 1 ) << 16 ,
                                    2 * std::uint64_t( sigma ) ) );
 }

}  // end( namespace detail )

template< class C >
std::pair< Rational , Rational > arithmeticInterval(
                                    std::span< const C > text ,
                                    const std::map< C , Rational > & prob )
{
 const auto f = detail::cumulative( prob );
 Rational l( 0 ) , s( 1 );
 for( const C & c : text ) {
  const auto it = prob.find( c );
  ADS_PRE( it != prob.end() );
  l = l + s * f.at( c );
  s = s * it->second;
  }
 return( std::pair( l , s ) );
 }

template< class C >
std::string arithmeticCodeExact( std::span< const C > text ,
                                 const std::map< C , Rational > & prob )
{
 const auto [ l , s ] = arithmeticInterval( text , prob );
 std::size_t d = 0;                        // ceil( log2( 2 / s ) )
 for( Rational t = s ; t < Rational( 2 ) ; t = t * Rational( 2 ) ) ++d;
 Rational x = l + s / Rational( 2 );
 std::string bits;
 for( std::size_t i = 0 ; i < d ; ++i ) {  // the converter of the notes
  x = x * Rational( 2 );
  if( x < Rational( 1 ) ) bits += '0';
  else {
   bits += '1';
   x = x - Rational( 1 );
   }
  }
 ADS_POST( ( l <= detail::dyadic( bits ) ) &&
           ( detail::dyadic( bits ) < l + s ) );
 return( bits );
 }

template< class C >
std::vector< C > arithmeticDecodeExact( const std::string & bits ,
                                        std::size_t n ,
                                        const std::map< C , Rational > & prob )
{
 for( char b : bits ) ADS_PRE( ( b == '0' ) || ( b == '1' ) );
 const auto f = detail::cumulative( prob );
 const Rational x = detail::dyadic( bits );
 Rational l( 0 ) , s( 1 );
 std::vector< C > text;
 for( std::size_t i = 0 ; i < n ; ++i ) {
  bool found = false;
  for( const auto & [ c , p ] : prob ) {
   const Rational lo = l + s * f.at( c );
   if( ( lo <= x ) && ( x < lo + s * p ) ) {
    text.push_back( c );
    l = lo;
    s = s * p;
    found = true;
    break;
    }
   }
  ADS_PRE( found );
  }
 return( text );
 }

inline RangeEncoder::RangeEncoder( int w )
 : f_M( detail::rangeSize( w ) ) , f_high( f_M ) {}

inline void RangeEncoder::emit( bool b )
{
 f_out.put( b );
 for( ; f_pending > 0 ; --f_pending ) f_out.put( ! b );
 }

inline void RangeEncoder::encode( std::uint64_t low , std::uint64_t high ,
                                  std::uint64_t total )
{
 ADS_PRE( ( low < high ) && ( high <= total ) && ( total <= maxTotal() ) );
 const std::uint64_t r = f_high - f_low;
 f_high = f_low + r * high / total;
 f_low = f_low + r * low / total;
 const std::uint64_t half = f_M / 2 , quarter = f_M / 4;
 for( ; ; ) {                             // the expansions
  if( f_high <= half ) emit( false );
  else if( f_low >= half ) {
   emit( true );
   f_low -= half;
   f_high -= half;
   }
  else if( ( f_low >= quarter ) && ( f_high <= half + quarter ) ) {
   ++f_pending;                           // an underflow
   f_low -= quarter;
   f_high -= quarter;
   }
  else break;
  f_low *= 2;
  f_high *= 2;
  }
 ADS_INVARIANT( f_high - f_low > quarter );
 }

inline std::vector< std::uint8_t > RangeEncoder::finish( void )
{
 ++f_pending;                    // 0 1^( m + 1 ) or 1 0^( m + 1 )
 emit( f_low >= f_M / 4 );
 return( f_out.bytes() );
 }

inline RangeDecoder::RangeDecoder( std::span< const std::uint8_t > bytes ,
                                   int w )
 : f_M( detail::rangeSize( w ) ) , f_high( f_M ) , f_in( bytes )
{
 f_v = f_in.get( w );
 }

inline std::uint64_t RangeDecoder::target( std::uint64_t total ) const
{
 ADS_PRE( ( total > 0 ) && ( total <= maxTotal() ) );
 const std::uint64_t r = f_high - f_low;
 const std::uint64_t t = ( ( f_v - f_low + 1 ) * total - 1 ) / r;
 ADS_POST( t < total );
 return( t );
 }

inline void RangeDecoder::consume( std::uint64_t low , std::uint64_t high ,
                                   std::uint64_t total )
{
 ADS_PRE( ( low < high ) && ( high <= total ) && ( total <= maxTotal() ) );
 const std::uint64_t r = f_high - f_low;
 f_high = f_low + r * high / total;
 f_low = f_low + r * low / total;
 ADS_PRE( ( f_low <= f_v ) && ( f_v < f_high ) );   // the right symbol
 const std::uint64_t half = f_M / 2 , quarter = f_M / 4;
 for( ; ; ) {
  if( f_high <= half ) {}
  else if( f_low >= half ) {
   f_low -= half;
   f_high -= half;
   f_v -= half;
   }
  else if( ( f_low >= quarter ) && ( f_high <= half + quarter ) ) {
   f_low -= quarter;
   f_high -= quarter;
   f_v -= quarter;
   }
  else break;
  f_low *= 2;
  f_high *= 2;
  f_v = 2 * f_v + std::uint64_t( f_in.get() );
  }
 ADS_INVARIANT( ( f_low <= f_v ) && ( f_v < f_high ) );
 }

template< class C >
std::vector< std::uint8_t > arithmeticEncode(
                         std::span< const C > text ,
                         const std::map< C , std::uint64_t > & counts , int w )
{
 RangeEncoder E( w );
 std::map< C , std::pair< std::uint64_t , std::uint64_t > > range;
 std::uint64_t total = 0;
 for( const auto & [ c , k ] : counts ) {
  ADS_PRE( k > 0 );
  range[ c ] = std::pair( total , total + k );
  total += k;
  }
 ADS_PRE( ( total > 0 ) && ( total <= E.maxTotal() ) );
 for( const C & c : text ) {
  const auto it = range.find( c );
  ADS_PRE( it != range.end() );
  E.encode( it->second.first , it->second.second , total );
  }
 auto code = E.finish();
 ADS_AUDIT( std::ranges::equal( arithmeticDecode( std::span<
            const std::uint8_t >( code ) , text.size() , counts , w ) ,
            text ) );
 return( code );
 }

template< class C >
std::vector< C > arithmeticDecode(
                        std::span< const std::uint8_t > code , std::size_t n ,
                        const std::map< C , std::uint64_t > & counts , int w )
{
 RangeDecoder D( code , w );
 std::vector< C > sym;
 std::vector< std::uint64_t > cum( 1 , 0 );
 for( const auto & [ c , k ] : counts ) {
  ADS_PRE( k > 0 );
  sym.push_back( c );
  cum.push_back( cum.back() + k );
  }
 const std::uint64_t total = cum.back();
 ADS_PRE( ( total > 0 ) && ( total <= D.maxTotal() ) );
 std::vector< C > text;
 text.reserve( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::uint64_t t = D.target( total );
  const std::size_t j = std::size_t( std::upper_bound( cum.begin() ,
                                     cum.end() , t ) - cum.begin() ) - 1;
  D.consume( cum[ j ] , cum[ j + 1 ] , total );
  text.push_back( sym[ j ] );
  }
 return( text );
 }

inline std::vector< std::uint8_t > adaptiveArithmeticEncode(
                     std::span< const std::uint32_t > x , std::uint32_t sigma )
{
 ADS_PRE( ( sigma >= 1 ) && ( sigma <= ( std::uint32_t( 1 ) << 20 ) ) );
 RangeEncoder E;
 detail::FrequencyTree T( sigma );
 const std::uint64_t limit = detail::adaptiveLimit( sigma );
 for( std::uint32_t c : x ) {
  ADS_PRE( c < sigma );
  const std::uint64_t low = T.prefix( c );
  E.encode( low , low + T.count( c ) , T.total() );
  T.add( c , 1 );
  if( T.total() > limit ) T.halve();
  }
 auto code = E.finish();
 ADS_AUDIT( std::ranges::equal( adaptiveArithmeticDecode( std::span<
            const std::uint8_t >( code ) , x.size() , sigma ) , x ) );
 return( code );
 }

inline std::vector< std::uint32_t > adaptiveArithmeticDecode(
                     std::span< const std::uint8_t > code , std::size_t n ,
                     std::uint32_t sigma )
{
 ADS_PRE( ( sigma >= 1 ) && ( sigma <= ( std::uint32_t( 1 ) << 20 ) ) );
 RangeDecoder D( code );
 detail::FrequencyTree T( sigma );
 const std::uint64_t limit = detail::adaptiveLimit( sigma );
 std::vector< std::uint32_t > x;
 x.reserve( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::size_t c = T.find( D.target( T.total() ) );
  const std::uint64_t low = T.prefix( c );
  D.consume( low , low + T.count( c ) , T.total() );
  x.push_back( std::uint32_t( c ) );
  T.add( c , 1 );
  if( T.total() > limit ) T.halve();
  }
 return( x );
 }

inline std::vector< std::uint8_t > adaptiveArithmeticEncode(
                                                          std::string_view s )
{
 std::vector< std::uint32_t > x( s.size() );
 for( std::size_t i = 0 ; i < s.size() ; ++i )
  x[ i ] = static_cast< unsigned char >( s[ i ] );
 return( adaptiveArithmeticEncode( std::span< const std::uint32_t >( x ) ,
                                   256 ) );
 }

inline std::string adaptiveArithmeticDecode(
                     std::span< const std::uint8_t > code , std::size_t n )
{
 const auto x = adaptiveArithmeticDecode( code , n , 256 );
 std::string s( n , '\0' );
 for( std::size_t i = 0 ; i < n ; ++i )
  s[ i ] = static_cast< char >( static_cast< unsigned char >( x[ i ] ) );
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File ArithmeticCoding.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
