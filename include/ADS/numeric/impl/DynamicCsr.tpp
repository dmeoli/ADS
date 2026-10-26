/*--------------------------------------------------------------------------*/
/*-------------------------- File DynamicCsr.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DynamicCsr [see DynamicCsr.h], with their contracts.
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

template< class I , class V >
void DynamicCsr< I , V >::reserve( std::size_t r , std::size_t z )
{
 f_beg.reserve( r + 1 );
 f_ind.reserve( z );
 f_val.reserve( z );
 }

template< class I , class V >
void DynamicCsr< I , V >::clear( void )
{
 f_beg.resize( 1 );
 f_ind.clear();
 f_val.clear();
 }

template< class I , class V >
std::size_t DynamicCsr< I , V >::append( std::span< const I > ind ,
                                         std::span< const V > val )
{
 ADS_PRE( ( ind.size() == val.size() ) &&
          ( f_ind.size() + ind.size() <=
            std::size_t( std::numeric_limits< I >::max() ) ) );
 f_ind.insert( f_ind.end() , ind.begin() , ind.end() );
 f_val.insert( f_val.end() , val.begin() , val.end() );
 f_beg.push_back( I( f_ind.size() ) );
 ADS_INVARIANT( consistent() );
 return( rows() - 1 );
 }

template< class I , class V >
std::size_t DynamicCsr< I , V >::append( std::size_t r , const I * beg ,
                                         const I * ind , const V * val )
{
 const std::size_t first = rows();
 if( r == 0 )
  return( first );
 ADS_PRE( ( beg != nullptr ) &&
          std::is_sorted( beg , beg + r + 1 ) &&
          ( f_ind.size() + std::size_t( beg[ r ] - beg[ 0 ] ) <=
            std::size_t( std::numeric_limits< I >::max() ) ) );
 const std::size_t s = std::size_t( beg[ 0 ] ) ,
                   e = std::size_t( beg[ r ] );
 const I shift = I( f_ind.size() ) - beg[ 0 ];
 f_ind.insert( f_ind.end() , ind + s , ind + e );
 f_val.insert( f_val.end() , val + s , val + e );
 const std::size_t b = f_beg.size();
 f_beg.resize( b + r );
 for( std::size_t k = 1 ; k <= r ; ++k )
  f_beg[ b + k - 1 ] = beg[ k ] + shift;
 ADS_INVARIANT( consistent() );
 return( first );
 }

template< class I , class V >
template< std::ranges::contiguous_range Range >
void DynamicCsr< I , V >::remove( const Range & E )
{
 using K = std::ranges::range_value_t< Range >;
 static_assert( std::is_integral_v< K > );
 const auto R = std::span< const K >( std::ranges::data( E ) ,
                                      std::size_t( std::ranges::size( E ) ) );
 ADS_PRE( std::adjacent_find( R.begin() , R.end() ,
                              []( K a , K b ) { return( ! ( a < b ) ); } )
          == R.end() );
 ADS_PRE( R.empty() || ( std::cmp_greater_equal( R.front() , 0 ) &&
                         std::cmp_less( R.back() , rows() ) ) );
 if( R.empty() )
  return;
 const std::size_t r = rows();
 std::size_t out = std::size_t( R[ 0 ] );      // the next row written
 std::size_t o = std::size_t( f_beg[ out ] );  // the next entry written
 std::size_t k = 0;
 I * const B = f_beg.data();
 I * const X = f_ind.data();
 V * const Y = f_val.data();
 for( std::size_t i = out ; i < r ; ++i ) {
  if( ( k < R.size() ) && ( std::size_t( R[ k ] ) == i ) ) {
   ++k;
   continue;
   }
  const std::size_t s = std::size_t( B[ i ] ) ,
                    e = std::size_t( B[ i + 1 ] );
  B[ out++ ] = I( o );                         // out < i: B[ i ] is read
  std::copy( X + s , X + e , X + o );
  std::copy( Y + s , Y + e , Y + o );
  o += e - s;
  }
 B[ out ] = I( o );
 f_beg.resize( out + 1 );
 f_ind.resize( o );
 f_val.resize( o );
 ADS_INVARIANT( consistent() );
 }

template< class I , class V >
void DynamicCsr< I , V >::remove( std::size_t first , std::size_t last )
{
 ADS_PRE( ( first <= last ) && ( last <= rows() ) );
 if( first == last )
  return;
 const std::size_t s = std::size_t( f_beg[ first ] ) ,
                   e = std::size_t( f_beg[ last ] );
 f_ind.erase( f_ind.begin() + s , f_ind.begin() + e );
 f_val.erase( f_val.begin() + s , f_val.begin() + e );
 const I d = I( e - s );
 for( std::size_t i = last ; i < f_beg.size() ; ++i )
  f_beg[ i - ( last - first ) ] = f_beg[ i ] - d;
 f_beg.resize( f_beg.size() - ( last - first ) );
 ADS_INVARIANT( consistent() );
 }

template< class I , class V >
std::span< const I > DynamicCsr< I , V >::indices( std::size_t i ) const
{
 ADS_PRE( i < rows() );
 return( std::span< const I >( f_ind.data() + f_beg[ i ] ,
                               std::size_t( f_beg[ i + 1 ] - f_beg[ i ] ) ) );
 }

template< class I , class V >
std::span< const V > DynamicCsr< I , V >::values( std::size_t i ) const
{
 ADS_PRE( i < rows() );
 return( std::span< const V >( f_val.data() + f_beg[ i ] ,
                               std::size_t( f_beg[ i + 1 ] - f_beg[ i ] ) ) );
 }

template< class I , class V >
std::span< V > DynamicCsr< I , V >::values( std::size_t i )
{
 ADS_PRE( i < rows() );
 return( std::span< V >( f_val.data() + f_beg[ i ] ,
                         std::size_t( f_beg[ i + 1 ] - f_beg[ i ] ) ) );
 }

template< class I , class V >
bool DynamicCsr< I , V >::consistent( void ) const
{
 return( ( ! f_beg.empty() ) && ( f_beg.front() == I( 0 ) ) &&
         std::is_sorted( f_beg.begin() , f_beg.end() ) &&
         ( std::size_t( f_beg.back() ) == f_ind.size() ) &&
         ( f_ind.size() == f_val.size() ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File DynamicCsr.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
