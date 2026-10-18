/*--------------------------------------------------------------------------*/
/*------------------------------ File PPM.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of PPMModel [see PPM.h], with their contracts.
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

inline PPMModel::PPMModel( int order , std::uint32_t sigma )
 : f_order( order ) , f_sigma( sigma )
{
 ADS_PRE( order >= 0 );
 ADS_PRE( ( sigma >= 1 ) && ( sigma <= ( std::uint32_t( 1 ) << 16 ) ) );
 }

inline std::map< std::uint32_t , std::uint64_t > PPMModel::counts(
                               const std::vector< std::uint32_t > & ctx ) const
{
 const auto it = f_ctx.find( ctx );
 return( it == f_ctx.end() ? Counts() : it->second );
 }

template< class Choose >
std::uint32_t PPMModel::walk( Choose choose ) const
{
 std::vector< bool > excluded( f_sigma , false );
 std::vector< std::uint32_t > sym;
 std::vector< std::uint64_t > cum;
 for( std::size_t o = f_hist.size() + 1 ; o-- > 0 ; ) {
  const auto first = f_hist.end() - std::ptrdiff_t( o );
  const std::vector< std::uint32_t > key( first , f_hist.end() );
  const auto it = f_ctx.find( key );
  if( it == f_ctx.end() ) continue;        // a context never seen
  sym.clear();
  cum.assign( 1 , 0 );
  for( const auto & [ s , k ] : it->second )
   if( ! excluded[ s ] ) {
    sym.push_back( s );
    cum.push_back( cum.back() + k );
    }
  if( sym.empty() ) continue;              // all excluded: no code
  const std::uint64_t total = cum.back() + it->second.size();
  const std::size_t j = choose( sym , cum , total );
  if( j < sym.size() ) return( sym[ j ] );
  for( std::uint32_t s : sym ) excluded[ s ] = true;   // an escape
  }
 sym.clear();                              // the context of order -1
 cum.assign( 1 , 0 );
 for( std::uint32_t s = 0 ; s < f_sigma ; ++s )
  if( ! excluded[ s ] ) {
   sym.push_back( s );
   cum.push_back( cum.back() + 1 );
   }
 const std::size_t j = choose( sym , cum , cum.back() );
 ADS_PRE( j < sym.size() );
 return( sym[ j ] );
 }

inline std::vector< PPMModel::Step > PPMModel::steps( std::uint32_t c ) const
{
 ADS_PRE( c < f_sigma );
 std::vector< Step > S;
 walk( [ & ]( const std::vector< std::uint32_t > & sym ,
              const std::vector< std::uint64_t > & cum ,
              std::uint64_t total ) {
  const std::size_t j = std::size_t( std::find( sym.begin() , sym.end() ,
                                                c ) - sym.begin() );
  S.push_back( j < sym.size() ? Step{ cum[ j ] , cum[ j + 1 ] , total }
                              : Step{ cum.back() , total , total } );
  return( j );
  } );
 return( S );
 }

inline void PPMModel::encode( std::uint32_t c , RangeEncoder & e ) const
{
 for( const Step & s : steps( c ) )
  e.encode( s.low , s.high , s.total );
 }

inline std::uint32_t PPMModel::decode( RangeDecoder & d ) const
{
 return( walk( [ & ]( const std::vector< std::uint32_t > & sym ,
                      const std::vector< std::uint64_t > & cum ,
                      std::uint64_t total ) {
  const std::uint64_t t = d.target( total );
  if( t >= cum.back() ) {                 // the escape
   d.consume( cum.back() , total , total );
   return( sym.size() );
   }
  const std::size_t j = std::size_t( std::upper_bound( cum.begin() ,
                                     cum.end() , t ) - cum.begin() ) - 1;
  d.consume( cum[ j ] , cum[ j + 1 ] , total );
  return( j );
  } ) );
 }

inline void PPMModel::update( std::uint32_t c )
{
 ADS_PRE( c < f_sigma );
 for( std::size_t o = 0 ; o <= f_hist.size() ; ++o ) {
  const auto first = f_hist.end() - std::ptrdiff_t( o );
  Counts & C = f_ctx[ std::vector< std::uint32_t >( first , f_hist.end() ) ];
  ++C[ c ];
  std::uint64_t sum = 0;
  for( const auto & e : C ) sum += e.second;
  if( sum > kLimit )
   for( auto & e : C ) e.second = e.second / 2 + e.second % 2;
  }
 f_hist.push_back( c );
 if( f_hist.size() > std::size_t( f_order ) ) f_hist.erase( f_hist.begin() );
 }

inline std::vector< std::uint8_t > ppmEncode( std::string_view s , int K )
{
 PPMModel M( K );
 RangeEncoder E;
 for( char ch : s ) {
  const std::uint32_t c = static_cast< unsigned char >( ch );
  M.encode( c , E );
  M.update( c );
  }
 auto code = E.finish();
 ADS_AUDIT( ppmDecode( code , s.size() , K ) == s );
 return( code );
 }

inline std::string ppmDecode( std::span< const std::uint8_t > code ,
                              std::size_t n , int K )
{
 PPMModel M( K );
 RangeDecoder D( code );
 std::string s;
 s.reserve( n );
 for( std::size_t i = 0 ; i < n ; ++i ) {
  const std::uint32_t c = M.decode( D );
  M.update( c );
  s += static_cast< char >( static_cast< unsigned char >( c ) );
  }
 return( s );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------------- End File PPM.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
