/*--------------------------------------------------------------------------*/
/*------------------------ File SparseAssembly.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SparseAssembly [see SparseAssembly.h], with their
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

/// true if the z indices of x are below n
template< class J >
bool indicesBelow( const J * x , std::size_t z , std::size_t n )
{
 for( std::size_t k = 0 ; k < z ; ++k )
  if constexpr( std::is_signed_v< J > ) {
   if( ( x[ k ] < 0 ) || ( std::size_t( x[ k ] ) >= n ) )
    return( false );
   }
  else if( std::size_t( x[ k ] ) >= n )
   return( false );
 return( true );
 }

}  // end( namespace detail )

template< class I , class V , class J >
std::size_t compressTriplets( std::size_t nmajor , std::size_t nminor ,
                              std::size_t z , const J * major ,
                              const J * minor , const V * value , I * beg ,
                              I * ind , V * val ,
                              AssemblyWorkspace< I , V > & work )
{
 static_assert( std::is_integral_v< I > && std::is_integral_v< J > );
 ADS_PRE( ( beg != nullptr ) && ( ( z == 0 ) ||
          ( ( major != nullptr ) && ( minor != nullptr ) &&
            ( value != nullptr ) && ( ind != nullptr ) &&
            ( val != nullptr ) ) ) );
 ADS_PRE( ( z <= std::size_t( std::numeric_limits< I >::max() ) ) &&
          ( nminor <= std::size_t( std::numeric_limits< I >::max() ) ) );
 ADS_PRE( detail::indicesBelow( major , z , nmajor ) &&
          detail::indicesBelow( minor , z , nminor ) );

 // the distribution by the major index, into the arrays of the caller
 std::fill( beg , beg + nmajor + 1 , I( 0 ) );
 for( std::size_t k = 0 ; k < z ; ++k )
  ++beg[ std::size_t( major[ k ] ) + 1 ];
 for( std::size_t i = 0 ; i < nmajor ; ++i )
  beg[ i + 1 ] += beg[ i ];
 work.cursor.assign( beg , beg + nmajor );
 I * const at = work.cursor.data();
 for( std::size_t k = 0 ; k < z ; ++k ) {
  const std::size_t q = std::size_t( at[ std::size_t( major[ k ] ) ]++ );
  ind[ q ] = I( minor[ k ] );
  val[ q ] = value[ k ];
  }

 // each major index: its repetitions summed into the first of them, in
 // the order of the input, by the last place written for each minor index
 // plus 1 (a place of an earlier major index is not above the first of
 // this one), then its distinct entries sorted; o is the next entry
 // written, never past the one read
 work.mark.assign( nminor , I( 0 ) );
 I * const mark = work.mark.data();
 std::size_t o = 0;
 std::size_t s = 0;
 for( std::size_t i = 0 ; i < nmajor ; ++i ) {
  const std::size_t e = std::size_t( beg[ i + 1 ] ) , f = o;
  for( std::size_t k = s ; k < e ; ++k ) {
   const I j = ind[ k ];
   I & w = mark[ std::size_t( j ) ];
   if( std::size_t( w ) > f )               // a repetition of the place
    val[ std::size_t( w ) - 1 ] += val[ k ];
   else {
    w = I( o + 1 );
    ind[ o ] = j;
    val[ o ] = val[ k ];
    ++o;
    }
   }
  if( o - f <= 32 )                         // by insertion
   for( std::size_t k = f + 1 ; k < o ; ++k ) {
    const I x = ind[ k ];
    if( ! ( x < ind[ k - 1 ] ) )
     continue;
    const V y = val[ k ];
    std::size_t h = k;
    do {
     ind[ h ] = ind[ h - 1 ];
     val[ h ] = val[ h - 1 ];
     --h;
     } while( ( h > f ) && ( x < ind[ h - 1 ] ) );
    ind[ h ] = x;
    val[ h ] = y;
    }
  else {                                    // the pairs, distinct
   auto & P = work.pairs;
   P.resize( o - f );
   for( std::size_t k = f ; k < o ; ++k )
    P[ k - f ] = { ind[ k ] , val[ k ] };
   std::sort( P.begin() , P.end() , []( const auto & a , const auto & b ) {
               return( a.first < b.first ); } );
   for( std::size_t k = f ; k < o ; ++k ) {
    ind[ k ] = P[ k - f ].first;
    val[ k ] = P[ k - f ].second;
    }
   }
  beg[ i ] = I( f );
  s = e;
  }
 beg[ nmajor ] = I( o );
 return( o );
 }

template< class I , class V , class J >
std::size_t compressTriplets( std::size_t nmajor , std::size_t nminor ,
                              std::size_t z , const J * major ,
                              const J * minor , const V * value , I * beg ,
                              I * ind , V * val )
{
 AssemblyWorkspace< I , V > work;
 return( compressTriplets( nmajor , nminor , z , major , minor , value ,
                           beg , ind , val , work ) );
 }

template< class I , class V , class J >
std::size_t assembleCsc( std::size_t m , std::size_t n , std::size_t z ,
                         const J * row , const J * col , const V * value ,
                         I * beg , I * ind , V * val ,
                         AssemblyWorkspace< I , V > & work )
{
 return( compressTriplets( n , m , z , col , row , value , beg , ind , val ,
                           work ) );
 }

template< class I , class V , class J >
std::size_t assembleCsr( std::size_t m , std::size_t n , std::size_t z ,
                         const J * row , const J * col , const V * value ,
                         I * beg , I * ind , V * val ,
                         AssemblyWorkspace< I , V > & work )
{
 return( compressTriplets( m , n , z , row , col , value , beg , ind , val ,
                           work ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SparseAssembly.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
