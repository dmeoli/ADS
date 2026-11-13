/*--------------------------------------------------------------------------*/
/*--------------------------- File XFastTrie.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of XFastTrie and ListXFastTrie [see XFastTrie.h], with
 * their contracts.
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

/*--------------------------------------------------------------------------*/
/*----------------------------- XFastTrie ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::optional< U > XFastTrie< U , V , W >::findNear( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 if( this->f_n == 0 )
  return( std::nullopt );
 if( this->node( x , W ) )
  return( x );
//! [XFastTrie-findNear]
 int h = W;   // a depth whose prefix of x is not there
 for( int K = 1 ; ; K = std::min( 2 * K , W ) ) {
  const int d = W - K;
  const U p = detail::triePrefix< U , W >( x , d );
  if( auto * u = this->node( p , d ) ) {   // the root, at d = 0, is there
   const auto [ v , i ] = this->deepest( x , d , h , u );
   const auto * l = this->successor( x , v , i );
   if( l == this->f_dummy )
    return( std::nullopt );
   return( l->key );
   }
  // no key in the block of x: the successor is the smallest of the next
  // block, if that is there
  if( p < U( ( U( 1 ) << d ) - 1 ) )
   if( auto * u = this->node( U( p + 1 ) , d ) )
    return( this->leftmost( u , d )->key );
  h = d;
  }
//! [XFastTrie-findNear]
 }

/*--------------------------------------------------------------------------*/
/*--------------------------- ListXFastTrie --------------------------------*/
/*--------------------------------------------------------------------------*/

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
ListXFastTrie< U , V , W >::ListXFastTrie( void )
{
 f_dummy = f_pool.make();
 f_dummy->prev = f_dummy->next = f_dummy;
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
ListXFastTrie< U , V , W >::ListXFastTrie( ListXFastTrie && o )
 : ListXFastTrie()
{
 *this = std::move( o );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
ListXFastTrie< U , V , W > & ListXFastTrie< U , V , W >::operator=(
                                             ListXFastTrie && o ) noexcept
{
 f_pool.swap( o.f_pool );
 std::swap( f_dummy , o.f_dummy );
 std::swap( f_n , o.f_n );
 for( int i = 0 ; i <= W ; ++i )
  f_t[ i ].swap( o.f_t[ i ] );
 return( *this );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
ListXFastTrie< U , V , W >::~ListXFastTrie()
{
 if constexpr( ! std::is_trivially_destructible_v< V > ) {
  for( Leaf * l = f_dummy->next ; l != f_dummy ; ) {
   Leaf * n = l->next;
   f_pool.free( l );
   l = n;
   }
  f_pool.free( f_dummy );
  }
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool ListXFastTrie< U , V , W >::entry( U p , int i , Entry & e ) const
{
 if( i == 0 ) {
  if( f_n == 0 )
   return( false );
  e = Entry{ f_dummy->next , f_dummy->prev };
  return( true );
  }
 const Entry * f = f_t[ i ].find( p );
 if( ! f )
  return( false );
 e = *f;
 return( true );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
int ListXFastTrie< U , V , W >::deepest( U x , int lo , int hi ,
                                         Entry & e ) const
{
//! [ListXFastTrie-deepest]
 while( hi - lo > 1 ) {
  const int m = ( lo + hi ) / 2;
  if( entry( detail::triePrefix< U , W >( x , m ) , m , e ) )
   lo = m;
  else
   hi = m;
  }
 // e holds the entry of lo only if a probe found it: read it again
 ( void ) entry( detail::triePrefix< U , W >( x , lo ) , lo , e );
 return( lo );
//! [ListXFastTrie-deepest]
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
auto ListXFastTrie< U , V , W >::follow( U x , const Entry & e , int l ) const
 -> Leaf *
{
//! [ListXFastTrie-follow]
 if( l == W )
  return( e.min );
 return( detail::trieBit< U , W >( x , l ) == 0 ? e.min : e.max->next );
//! [ListXFastTrie-follow]
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
auto ListXFastTrie< U , V , W >::successor( U x ) const -> Leaf *
{
 if( f_n == 0 )
  return( f_dummy );
 Entry e;
 const int l = deepest( x , 0 , W + 1 , e );
 return( follow( x , e , l ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::optional< U > ListXFastTrie< U , V , W >::find( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 const Leaf * l = successor( x );
 if( l == f_dummy )
  return( std::nullopt );
 return( l->key );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
auto ListXFastTrie< U , V , W >::lowerBound( U x ) const -> const_iterator
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 return( const_iterator( successor( x ) ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::optional< U > ListXFastTrie< U , V , W >::findNear( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 if( f_n == 0 )
  return( std::nullopt );
 if( f_t[ W ].contains( x ) )
  return( x );
//! [ListXFastTrie-findNear]
 int h = W;
 for( int K = 1 ; ; K = std::min( 2 * K , W ) ) {
  const int d = W - K;
  const U p = detail::triePrefix< U , W >( x , d );
  Entry e;
  if( entry( p , d , e ) ) {
   const int l = deepest( x , d , h , e );
   const Leaf * s = follow( x , e , l );
   if( s == f_dummy )
    return( std::nullopt );
   return( s->key );
   }
  if( p < U( ( U( 1 ) << d ) - 1 ) && entry( U( p + 1 ) , d , e ) )
   return( e.min->key );
  h = d;
  }
//! [ListXFastTrie-findNear]
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::optional< V > ListXFastTrie< U , V , W >::lookup( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 const Entry * e = f_t[ W ].find( x );
 if( ! e )
  return( std::nullopt );
 return( e->min->value );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool ListXFastTrie< U , V , W >::contains( U x ) const
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 return( f_t[ W ].contains( x ) );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
std::size_t ListXFastTrie< U , V , W >::entries( void ) const
{
 std::size_t s = 0;
 for( int i = 1 ; i <= W ; ++i )
  s += f_t[ i ].size();
 return( s );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool ListXFastTrie< U , V , W >::insert( U x , const V & v )
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 if( Entry * e = f_t[ W ].find( x ) ) {
  e->min->value = v;
  return( false );
  }
//! [ListXFastTrie-insert]
 Leaf * s = successor( x );
 Leaf * l = f_pool.make();
 l->key = x;
 l->value = v;
 l->next = s;
 l->prev = s->prev;
 s->prev->next = l;
 s->prev = l;
 ++f_n;
 for( int i = 1 ; i <= W ; ++i ) {
  const U p = detail::triePrefix< U , W >( x , i );
  if( Entry * e = f_t[ i ].find( p ) ) {
   if( x < e->min->key )
    e->min = l;
   if( x > e->max->key )
    e->max = l;
   }
  else
   f_t[ i ].insert( p , Entry{ l , l } );
  }
//! [ListXFastTrie-insert]
 ADS_POST( contains( x ) );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool ListXFastTrie< U , V , W >::remove( U x )
{
 ADS_PRE( ( detail::trieKey< U , W >( x ) ) );
 const Entry * f = f_t[ W ].find( x );
 if( ! f )
  return( false );
//! [ListXFastTrie-remove]
 Leaf * l = f->min;
 for( int i = 1 ; i <= W ; ++i ) {
  const U p = detail::triePrefix< U , W >( x , i );
  Entry * e = f_t[ i ].find( p );
  if( ( e->min == l ) && ( e->max == l ) )
   f_t[ i ].remove( p );
  else if( e->min == l )
   e->min = l->next;   // the keys of a prefix are consecutive in the list
  else if( e->max == l )
   e->max = l->prev;
  }
 l->prev->next = l->next;
 l->next->prev = l->prev;
 f_pool.free( l );
 --f_n;
//! [ListXFastTrie-remove]
 ADS_POST( ! contains( x ) );
 ADS_INVARIANT( consistent( x ) );
 return( true );
 }

template< std::unsigned_integral U , class V , int W >
 requires TrieWidth< U , W >
bool ListXFastTrie< U , V , W >::consistent( U x ) const
{
 if( f_n > 256 ) {   // around x only
  const Entry * e = f_t[ W ].find( x );
  if( ! e )
   return( true );
  const Leaf * l = e->min;
  return( ( l->key == x ) &&
          ( l->prev == f_dummy || l->prev->key < x ) &&
          ( l->next == f_dummy || l->next->key > x ) );
  }
 std::size_t k = 0;
 for( const Leaf * l = f_dummy->next ; l != f_dummy ; l = l->next , ++k )
  if( ( l->next->prev != l ) ||
      ( ( l->next != f_dummy ) && ! ( l->key < l->next->key ) ) )
   return( false );
 if( k != f_n )
  return( false );
 for( int i = 1 ; i <= W ; ++i ) {
  // one entry per run of keys with the same prefix, from its first key to
  // its last
  std::size_t runs = 0;
  for( const Leaf * l = f_dummy->next ; l != f_dummy ; l = l->next ) {
   const U p = detail::triePrefix< U , W >( l->key , i );
   if( ( l->prev != f_dummy ) &&
       ( detail::triePrefix< U , W >( l->prev->key , i ) == p ) )
    continue;
   ++runs;
   const Entry * e = f_t[ i ].find( p );
   if( ! e || ( e->min != l ) )
    return( false );
   const Leaf * m = l;
   while( ( m->next != f_dummy ) &&
          ( detail::triePrefix< U , W >( m->next->key , i ) == p ) )
    m = m->next;
   if( e->max != m )
    return( false );
   }
  if( runs != f_t[ i ].size() )
   return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File XFastTrie.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
