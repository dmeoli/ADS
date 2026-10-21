/*--------------------------------------------------------------------------*/
/*------------------------- File MoveStructure.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of MoveStructure.h, with their contracts.
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

inline MoveStructure::MoveStructure( std::span< const std::uint64_t > p ,
                                     std::span< const std::uint64_t > q ,
                                     std::span< const std::uint32_t > tag ,
                                     std::uint64_t n ) : f_n( n )
{
 ADS_PRE( ( p.size() == q.size() ) && ( p.size() == tag.size() ) );
 ADS_PRE( p.empty() || ( p[ 0 ] == 0 ) );
 if( p.empty() ) return;
 // the intervals by their starts, with their lengths, images and tags,
 // and the images by their starts, with the start of their interval
 struct Item { std::uint64_t len , q; std::uint32_t tag; };
 std::map< std::uint64_t , Item > in;
 std::map< std::uint64_t , std::uint64_t > img;
 for( std::size_t i = 0 ; i < p.size() ; ++i ) {
  const std::uint64_t len = ( i + 1 < p.size() ? p[ i + 1 ] : n ) - p[ i ];
  in.emplace( p[ i ] , Item{ len , q[ i ] , tag[ i ] } );
  img.emplace( q[ i ] , p[ i ] );
  }
 // the balance: an image with 4 starts or more is cut at the third, so
 // that its first part holds 2, and the images that the new start may
 // weigh on are checked again
 std::vector< std::uint64_t > work;
 for( const auto & [ qs , ps ] : img ) work.push_back( qs );
 while( ! work.empty() ) {
  const std::uint64_t qs = work.back();
  work.pop_back();
  const auto at = img.find( qs );
  if( at == img.end() ) continue;
  const std::uint64_t ps = at->second;
  Item & it = in.at( ps );
  auto s = in.lower_bound( qs );
  int k = 0;
  std::uint64_t third = 0;
  for( ; ( s != in.end() ) && ( s->first < qs + it.len ) && ( k < 4 ) ;
       ++s , ++k )
   if( k == 2 ) third = s->first;
  if( k < 4 ) continue;
  const std::uint64_t t = third - qs;       // the cut, inside the image
  const std::uint64_t np = ps + t;
  in.emplace( np , Item{ it.len - t , qs + t , it.tag } );
  in.at( ps ).len = t;
  img.emplace( qs + t , np );
  work.push_back( qs );
  work.push_back( qs + t );
  auto h = img.upper_bound( np );           // the image holding np
  --h;
  work.push_back( h->first );
  }
 // the arrays, by start
 f_m = in.size();
 f_wp = std::max( 1 , int( std::bit_width( n ) ) );
 f_wd = std::max( 1 , int( std::bit_width( f_m ) ) );
 f_P = BitVector( f_m * std::size_t( f_wp ) );
 f_Q = BitVector( f_m * std::size_t( f_wp ) );
 f_D = BitVector( f_m * std::size_t( f_wd ) );
 std::uint32_t top = 0;
 for( const auto t : tag ) top = std::max( top , t );
 f_wt = std::max( 1 , int( std::bit_width( top ) ) );
 f_tag = BitVector( f_m * std::size_t( f_wt ) );
 std::vector< std::uint64_t > starts;
 starts.reserve( f_m );
 for( const auto & [ ps , it ] : in ) starts.push_back( ps );
 std::size_t i = 0;
 for( const auto & [ ps , it ] : in ) {
  f_P.put( i * std::size_t( f_wp ) , ps , f_wp );
  f_Q.put( i * std::size_t( f_wp ) , it.q , f_wp );
  const std::size_t d = std::size_t( std::upper_bound( starts.begin() ,
                                                       starts.end() , it.q ) -
                                     starts.begin() ) - 1;
  f_D.put( i * std::size_t( f_wd ) , d , f_wd );
  f_tag.put( i * std::size_t( f_wt ) , it.tag , f_wt );
  ++i;
  }
 ADS_AUDIT( ( [ & ]() {                     // no image holds 4 starts
  for( std::size_t j = 0 ; j < f_m ; ++j ) {
   const std::uint64_t a = f_Q.get( j * std::size_t( f_wp ) , f_wp ) ,
                       b = a + start( j + 1 ) - start( j );
   const auto lo = std::lower_bound( starts.begin() , starts.end() , a ) ,
              hi = std::lower_bound( starts.begin() , starts.end() , b );
   if( hi - lo >= 4 ) return( false );
   }
  return( true ); }() ) );
 }

inline std::pair< std::uint64_t , std::size_t > MoveStructure::move(
                                     std::uint64_t x , std::size_t i ) const
{
 ADS_PRE( ( i < f_m ) && ( start( i ) <= x ) && ( x < start( i + 1 ) ) );
 const std::uint64_t y = f_Q.get( i * std::size_t( f_wp ) , f_wp ) +
                         ( x - f_P.get( i * std::size_t( f_wp ) , f_wp ) );
 std::size_t j = std::size_t( f_D.get( i * std::size_t( f_wd ) , f_wd ) );
 while( start( j + 1 ) <= y ) ++j;          // at most 3 steps
 return( std::pair( y , j ) );
 }

inline std::size_t MoveStructure::interval( std::uint64_t x ) const
{
 ADS_PRE( x < f_n );
 std::size_t lo = 0 , hi = f_m;             // start( lo ) <= x < start( hi )
 while( lo + 1 < hi ) {
  const std::size_t mid = ( lo + hi ) / 2;
  if( start( mid ) <= x ) lo = mid;
  else hi = mid;
  }
 return( lo );
 }

inline std::size_t MoveStructure::sizeInBits( void ) const
{
 return( f_P.size() + f_Q.size() + f_D.size() + f_tag.size() );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File MoveStructure.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
