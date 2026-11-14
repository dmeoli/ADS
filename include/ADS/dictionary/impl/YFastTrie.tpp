/*--------------------------------------------------------------------------*/
/*--------------------------- File YFastTrie.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of YFastTrie [see YFastTrie.h], with their contracts.
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

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
YFastTrie< U , V , W >::YFastTrie( std::uint64_t seed ) : f_g( seed )
{
 f_x.insert( TOP , new Group( f_g() ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
YFastTrie< U , V , W >::YFastTrie( YFastTrie && o ) : YFastTrie( 1 )
{
 *this = std::move( o );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
YFastTrie< U , V , W > & YFastTrie< U , V , W >::operator=( YFastTrie && o )
 noexcept
{
 std::swap( f_x , o.f_x );
 std::swap( f_g , o.f_g );
 std::swap( f_n , o.f_n );
 return( *this );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
YFastTrie< U , V , W >::~YFastTrie()
{
 for( const auto & [ r , T ] : f_x )
  delete T;
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
const U * YFastTrie< U , V , W >::ceiling( const Group & T , U x )
{
 using S = typename Group::Shape;
 const U * best = nullptr;
 for( auto u = S::root( T ) ; u ; ) {
  const U & k = S::read( u ).first;
  if( k < x )
   u = S::right( u );
  else {
   best = &k;
   u = S::left( u );
   }
  }
 return( best );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
const U * YFastTrie< U , V , W >::lowerBound( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 return( ceiling( *( *f_x.seek( x ) ).second , x ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
const U * YFastTrie< U , V , W >::find( U x ) const
{
 const U * k = lowerBound( x );
 return( k && ( *k == x ) ? k : nullptr );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::optional< V > YFastTrie< U , V , W >::lookup( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 return( ( *f_x.seek( x ) ).second->lookup( x ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool YFastTrie< U , V , W >::contains( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 return( ( *f_x.seek( x ) ).second->contains( x ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool YFastTrie< U , V , W >::insert( U x , const V & v )
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 const auto [ r , T ] = *f_x.seek( x );
 const bool fresh = ! T->contains( x );
 T->insert( x , v );
 if( fresh ) {
  ++f_n;
  if( ( x != r ) && ( f_g() % std::uint64_t( W ) == 0 ) ) {
   // x becomes the representative of the keys of T up to it
   auto [ L , y , R ] = std::move( *T ).split( x );
   Group * G = new Group( Group::join( std::move( L ) , x , *y ,
                                       Group( f_g() ) ) );
   *T = std::move( R );
   f_x.insert( x , G );
   }
  }
 ADS_POST( contains( x ) );
 ADS_INVARIANT( consistent() );
 return( fresh );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool YFastTrie< U , V , W >::remove( U x )
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 auto it = f_x.seek( x );
 const auto [ r , T ] = *it;
 if( ! T->contains( x ) )
  return( false );
 T->remove( x );
 --f_n;
 if( ( x == r ) && ( x != TOP ) ) {   // the group joins the next one
  Group * N = ( *std::next( it ) ).second;
  *N = Group::join2( std::move( *T ) , std::move( *N ) );
  delete T;
  f_x.remove( x );
  }
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( consistent() );
 return( true );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
template< class F >
void YFastTrie< U , V , W >::forEach( F && f ) const
{
 for( const auto & [ r , T ] : f_x )
  T->forEach( f );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool YFastTrie< U , V , W >::consistent( void ) const
{
 if( f_n > 256 )
  return( true );
 std::size_t n = 0;
 std::optional< U > prev;   // the previous representative
 for( const auto & e : f_x ) {
  const U & r = e.first;   // a lambda may not capture a structured binding
  const auto & T = e.second;   // under OpenMP in clang
  bool ok = true;
  T->forEach( [ & ]( const U & k , const V & ) {
   ok = ok && ( k <= r ) && ( ! prev || ( k > *prev ) );
   } );
  if( ! ok || ( ( r != TOP ) && ! T->contains( r ) ) )
   return( false );
  n += T->size();
  prev = r;
  }
 return( ( n == f_n ) && ( prev == TOP ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File YFastTrie.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
