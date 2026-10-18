/*--------------------------------------------------------------------------*/
/*----------------------------- File Trie.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Trie and RadixTree [see Trie.h], with their contracts.
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

/*--------------------------------------------------------------------------*/
/*--------------------------------- TRIE -----------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
auto Trie< V >::find( std::string_view p ) const -> const Node *
{
 const Node * x = &f_root;
 for( char c : p ) {
  const auto it = x->next.find( c );
  if( it == x->next.end() )
   return( nullptr );
  x = it->second.get();
  }
 return( x );
 }

template< class V >
std::optional< V > Trie< V >::lookup( std::string_view k ) const
{
 const Node * x = find( k );
 return( x ? x->value : std::nullopt );
 }

template< class V >
void Trie< V >::insert( std::string_view k , const V & v )
{
 Node * x = &f_root;
 for( char c : k ) {
  auto & n = x->next[ c ];
  if( ! n )
   n = std::make_unique< Node >();
  x = n.get();
  }
 if( ! x->value )
  ++f_size;
 x->value = v;
 ADS_POST( contains( k ) );
 }

template< class V >
bool Trie< V >::remove( Node * x , std::string_view k , std::size_t d )
{
 if( d == k.size() ) {
  if( x->value ) {
   x->value.reset();
   --f_size;
   }
  }
 else {
  const auto it = x->next.find( k[ d ] );
  if( ( it != x->next.end() ) && remove( it->second.get() , k , d + 1 ) )
   x->next.erase( it );                     // the child became useless
  }
 return( ! x->value && x->next.empty() );
 }

template< class V >
void Trie< V >::remove( std::string_view k )
{
 remove( &f_root , k , 0 );
 ADS_POST( ! contains( k ) );
 }

template< class V >
bool Trie< V >::contains( std::string_view k ) const
{
 return( lookup( k ).has_value() );
 }

template< class V >
std::size_t Trie< V >::size( void ) const
{
 return( f_size );
 }

template< class V >
void Trie< V >::collect( const Node * x , std::string & s ,
                         std::vector< std::string > & out )
{
 if( x->value )
  out.push_back( s );
 for( const auto & [ c , n ] : x->next ) {
  s.push_back( c );
  collect( n.get() , s , out );
  s.pop_back();
  }
 }

template< class V >
std::vector< std::string > Trie< V >::keysWithPrefix(
                                                   std::string_view p ) const
{
 std::vector< std::string > out;
 if( const Node * x = find( p ) ) {
  std::string s( p );
  collect( x , s , out );
  }
 return( out );
 }

template< class V >
std::size_t Trie< V >::count( const Node * x )
{
 std::size_t n = 1;
 for( const auto & e : x->next ) n += count( e.second.get() );
 return( n );
 }

template< class V >
std::size_t Trie< V >::nodes( void ) const
{
 return( count( &f_root ) );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ RADIX TREE --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class V >
std::optional< V > RadixTree< V >::lookup( std::string_view k ) const
{
 const Node * x = &f_root;
 while( ! k.empty() ) {
  const auto it = x->next.find( k[ 0 ] );
  if( it == x->next.end() )
   return( std::nullopt );
  const std::string & l = it->second->label;
  if( k.substr( 0 , l.size() ) != l )
   return( std::nullopt );
  k.remove_prefix( l.size() );
  x = it->second.get();
  }
 return( x->value );
 }

template< class V >
void RadixTree< V >::insert( std::string_view k , const V & v )
{
 const std::string key( k );
 Node * x = &f_root;
 while( ! k.empty() ) {
  auto it = x->next.find( k[ 0 ] );
  if( it == x->next.end() ) {               // a new leaf with the rest
   auto n = std::make_unique< Node >();
   n->label = std::string( k );
   n->value = v;
   x->next.emplace( k[ 0 ] , std::move( n ) );
   ++f_size;
   ADS_INVARIANT( compact( &f_root , true ) );
   return;
   }
  Node * y = it->second.get();
  std::size_t j = 0;                        // the common prefix of label
  while( ( j < y->label.size() ) && ( j < k.size() ) &&
         ( y->label[ j ] == k[ j ] ) )
   ++j;
  if( j < y->label.size() ) {               // split the edge at j
   auto mid = std::make_unique< Node >();
   mid->label = y->label.substr( 0 , j );
   std::unique_ptr< Node > old = std::move( it->second );
   old->label.erase( 0 , j );
   mid->next.emplace( old->label[ 0 ] , std::move( old ) );
   it->second = std::move( mid );
   y = it->second.get();
   }
  k.remove_prefix( j );
  x = y;
  }
 if( ! x->value )
  ++f_size;
 x->value = v;
 ADS_POST( contains( key ) );
 ADS_INVARIANT( compact( &f_root , true ) );
 }

template< class V >
void RadixTree< V >::compress( Node * x )
{
 if( x->value || ( x->next.size() != 1 ) )
  return;
 std::unique_ptr< Node > c = std::move( x->next.begin()->second );
 x->next.clear();
 x->label += c->label;
 x->value = std::move( c->value );
 x->next = std::move( c->next );
 }

template< class V >
bool RadixTree< V >::remove( Node * x , std::string_view k )
{
 if( k.empty() ) {
  if( x->value ) {
   x->value.reset();
   --f_size;
   }
  }
 else {
  const auto it = x->next.find( k[ 0 ] );
  if( it != x->next.end() ) {
   Node * y = it->second.get();
   if( k.substr( 0 , y->label.size() ) == y->label ) {
    if( remove( y , k.substr( y->label.size() ) ) )
     x->next.erase( it );                   // useless: away
    else
     compress( y );                         // one child left: merged
    }
   }
  }
 return( ! x->value && x->next.empty() );
 }

template< class V >
void RadixTree< V >::remove( std::string_view k )
{
 remove( &f_root , k );
 ADS_POST( ! contains( k ) );
 ADS_INVARIANT( compact( &f_root , true ) );
 }

template< class V >
bool RadixTree< V >::contains( std::string_view k ) const
{
 return( lookup( k ).has_value() );
 }

template< class V >
std::size_t RadixTree< V >::size( void ) const
{
 return( f_size );
 }

template< class V >
void RadixTree< V >::collect( const Node * x , std::string & s ,
                              std::vector< std::string > & out )
{
 if( x->value )
  out.push_back( s );
 for( const auto & [ c , n ] : x->next ) {
  s += n->label;
  collect( n.get() , s , out );
  s.resize( s.size() - n->label.size() );
  }
 }

template< class V >
std::vector< std::string > RadixTree< V >::keysWithPrefix(
                                                   std::string_view p ) const
{
 std::vector< std::string > out;
 const Node * x = &f_root;
 std::string s;
 while( ! p.empty() ) {                     // down to the node below p
  const auto it = x->next.find( p[ 0 ] );
  if( it == x->next.end() )
   return( out );
  const std::string & l = it->second->label;
  const std::size_t m = std::min( l.size() , p.size() );
  if( l.compare( 0 , m , p.substr( 0 , m ) ) != 0 )
   return( out );
  s += l;
  p.remove_prefix( m );
  x = it->second.get();
  }
 collect( x , s , out );
 return( out );
 }

template< class V >
std::size_t RadixTree< V >::count( const Node * x )
{
 std::size_t n = 1;
 for( const auto & e : x->next ) n += count( e.second.get() );
 return( n );
 }

template< class V >
std::size_t RadixTree< V >::nodes( void ) const
{
 return( count( &f_root ) );
 }

template< class V >
bool RadixTree< V >::compact( const Node * x , bool root )
{
 if( ! root && ( x->label.empty() ||
                 ( ! x->value && ( x->next.size() < 2 ) ) ) )
  return( false );
 for( const auto & [ c , n ] : x->next )
  if( ( n->label[ 0 ] != c ) || ! compact( n.get() , false ) )
   return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Trie.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
