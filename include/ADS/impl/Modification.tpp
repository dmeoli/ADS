/*--------------------------------------------------------------------------*/
/*------------------------- File Modification.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of the subjects [see Modification.h], the coalescing of
 * the batches and the rollback of UndoLog, with their contracts.
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

#include <optional>
#include <unordered_map>

namespace ADS::mod {

/// \cond
namespace detail {

/*--------------------------------------------------------------------------*/
/*------------------------------- THE KINDS --------------------------------*/
/*--------------------------------------------------------------------------*/

template< class L , class F >
void visitKind( std::size_t k , F && f )
{
 [ & ]< class ... E >( list< E... > * ) {
  std::size_t i = 0;
  ( ( ( i++ == k ) ? ( f.template operator()< E >() , void() ) : void() ) ,
    ... );
  }( static_cast< L * >( nullptr ) );
 }

template< class A , class O , class E , class Var >
bool giveRun( O & o , std::span< const Var > R )
{
 bool leave = false;
 if constexpr( calls_span< A , O , E > ) {
  std::vector< E > v;
  v.reserve( R.size() );
  for( const auto & x : R )
   v.push_back( std::get< E >( x ) );
  A::many( o , std::span< const E >( v ) );
  }
 else
  for( const auto & x : R ) {
   if constexpr( calls_keep< A , O , E > )
    leave = ( A::one( o , std::get< E >( x ) ) == Keep::Detach ) || leave;
   else
    A::one( o , std::get< E >( x ) );
   }
 return( leave );
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASS Slots ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class L , class Obs >
template< class A , class D >
int Slots< L , Obs >::attach( D & o )
{
 check< A , D , L >();
 Slot s;
 s.o = & o;
 s.mask = kinds_of< A , D , L >::value;
 if constexpr( calls_mask< A , D > )
  s.mask &= A::mask( o );
 [ & ]< std::size_t ... I >( std::index_sequence< I... > ) {
  ( fill< A , D , I >( s ) , ... ); }( std::make_index_sequence< K >() );
 if constexpr( calls_span< A , D , Var > )
  s.all = []( void * p , std::span< const Var > C ) {
   A::many( *static_cast< D * >( p ) , C ); };
 if constexpr( calls_group< A , D > ) {
  s.begin = []( void * p , const Group & g ) {
   A::begin( *static_cast< D * >( p ) , g ); };
  s.end = []( void * p ) { A::end( *static_cast< D * >( p ) ); };
  }
 if constexpr( ! std::is_void_v< Obs > ) {
  if constexpr( calls_audit< A , D , Obs > )
   s.audit = []( const void * p , const void * x ) -> bool {
    return( A::audit( *static_cast< const D * >( p ) ,
                      *static_cast< const Obs * >( x ) ) ); };
  }
 f_s.push_back( s );
 f_mask |= s.mask;
 return( int( f_s.size() ) - 1 );
 }

template< class L , class Obs >
template< class A , class D , std::size_t I >
void Slots< L , Obs >::fill( Slot & s )
{
 using E = std::variant_alternative_t< I , Var >;
 if constexpr( calls< A , D , E > ) {
  s.one[ I ] = []( void * p , const void * e ) -> Keep {
   D & d = *static_cast< D * >( p );
   const E & x = *static_cast< const E * >( e );
   if constexpr( calls_keep< A , D , E > )
    return( A::one( d , x ) );
   else {
    A::one( d , x );
    return( Keep::Yes );
    } };
  s.run[ I ] = []( void * p , std::span< const Var > R ) -> bool {
   return( giveRun< A , D , E >( *static_cast< D * >( p ) , R ) ); };
  }
 if constexpr( calls_prepare< A , D , E > )
  s.prep[ I ] = []( const void * p , const void * e ) -> bool {
   return( A::prepare( *static_cast< const D * >( p ) ,
                       *static_cast< const E * >( e ) ) ); };
 }

template< class L , class Obs >
void Slots< L , Obs >::detach( int id )
{
 f_s[ std::size_t( id ) ].o = nullptr;
 remask();
 }

template< class L , class Obs >
void Slots< L , Obs >::remask( void )
{
 f_mask = 0;
 for( const auto & s : f_s )
  if( s.o )
   f_mask |= s.mask;
 }

template< class L , class Obs >
template< class E , bool T >
void Slots< L , Obs >::deliver( const E & e , Trail & t )
{
 constexpr std::size_t k = index_v< E , L >;
 bool gone = false;
 for( std::size_t i = 0 ; i < f_s.size() ; ++i ) {
  Slot & s = f_s[ i ];
  if( ( s.o == nullptr ) || ( ( ( s.mask >> k ) & 1u ) == 0 ) )
   continue;
  if constexpr( ADS_CONTRACTS >= 3 ) {
   ++s.counts.events;
   if constexpr( std::is_same_v< E , Rebuilt > )
    ++s.counts.rebuilds;
   }
  if constexpr( T )
   t.dy = i;
  if( s.one[ k ]( s.o , & e ) == Keep::Detach ) {
   s.o = nullptr;
   gone = true;
   }
  }
 if constexpr( T )
  t.dy = f_s.size();
 if( gone )
  remask();
 }

template< class L , class Obs >
template< class E , class Inv >
void Slots< L , Obs >::undo( const Inv & inv , const Trail & t ) noexcept
{
 constexpr std::size_t k = index_v< E , L >;
 constexpr std::size_t j = index_v< Inv , L >;
 if constexpr( j < K ) {
  bool gone = false;
  for( std::size_t i = t.dy ; i-- > 0 ; ) {
   Slot & s = f_s[ i ];
   if( ( s.o == nullptr ) || ( ( ( s.mask >> k ) & 1u ) == 0 ) ||
       ( s.one[ j ] == nullptr ) )
    continue;
   try {
    if( s.one[ j ]( s.o , & inv ) == Keep::Detach ) {
     s.o = nullptr;                     // honoured after the rollback
     gone = true;
     }
    }
   catch( ... ) {}                      // the first exception wins
   }
  if( gone )
   remask();
  }
 }

template< class L , class Obs >
template< class E >
bool Slots< L , Obs >::prepare( const E & e ) const
{
 constexpr std::size_t k = index_v< E , L >;
 for( const auto & s : f_s )
  if( s.o && ( ( s.mask >> k ) & 1u ) && s.prep[ k ] &&
      ! s.prep[ k ]( s.o , & e ) )
   return( false );
 return( true );
 }

template< class L , class Obs >
template< class S >
bool Slots< L , Obs >::audit( [[maybe_unused]] const S & x ) const
{
 if constexpr( std::is_same_v< S , Obs > ) {
  for( const auto & s : f_s )
   if( s.o && s.audit && ! s.audit( s.o , & x ) )
    return( false );
  }
 return( true );
 }

template< class L , class Obs >
void Slots< L , Obs >::deliverGroup( std::span< const Var > C ,
                                     std::uint64_t kinds , const Group & g ,
                                     std::vector< Var > & tmp )
{
 bool gone = false;
 for( std::size_t i = 0 ; i < f_s.size() ; ++i ) {
  Slot & s = f_s[ i ];
  const std::uint64_t m = s.mask;
  if( ( s.o == nullptr ) || ( ( m & kinds ) == 0 ) )
   continue;
  if constexpr( ADS_CONTRACTS >= 3 )
   ++s.counts.groups;
  if( s.begin )
   s.begin( s.o , g );
  bool leave = false;
  if( s.all ) {
   if( ( kinds & ~m ) == 0 )
    s.all( s.o , C );
   else {
    tmp.clear();
    for( const auto & e : C )
     if( ( m >> e.index() ) & 1u )
      tmp.push_back( e );
    s.all( s.o , std::span< const Var >( tmp ) );
    }
   }
  else {
   std::size_t a = 0;
   while( a < C.size() ) {
    const std::size_t k = C[ a ].index();
    std::size_t b = a + 1;
    while( ( b < C.size() ) && ( C[ b ].index() == k ) )
     ++b;
    if( ( ( m >> k ) & 1u ) && s.run[ k ] )
     leave = s.run[ k ]( s.o , C.subspan( a , b - a ) ) || leave;
    a = b;
    }
   }
  if( s.end )
   s.end( s.o );
  if( leave ) {
   s.o = nullptr;
   gone = true;
   }
  }
 if( gone )
  remask();
 }

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS Subject -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class L , class Obs , class St , class Dy >
template< class E , class U >
void Subject< L , Obs , St , Dy >::emit( const E & e , U && undo )
{
 static_assert( contains_v< E , L > ,
                "ADS: an event that the structure does not declare" );
 ++f_built;
 if( ( f_depth | f_quiet ) != 0 ) {
  defer( e );                           // nobody is told before the close
  return;
  }
 if( ! live< E >() )
  return;
 ++f_rev[ index_v< E , L > ];
 Trail t;
 try {
  f_st.template deliver< E , true >( e , t );
  if constexpr( dynamic ) {
   f_dy.f_busy = true;
   struct Done {
    bool & b;
    ~Done() { b = false; }
    } done{ f_dy.f_busy };
   f_dy.template deliver< E , true >( e , t );
   }
  }
 catch( ... ) {
  if constexpr( invertible< E > ) {
   const auto inv = inverse( e );
   if constexpr( dynamic ) {
    f_dy.f_busy = true;
    f_dy.template undo< E >( inv , t );
    f_dy.f_busy = false;
    }
   f_st.template undo< E >( inv , t );
   }
  undo();
  throw;
  }
 }

template< class L , class Obs , class St , class Dy >
template< class E >
void Subject< L , Obs , St , Dy >::defer( const E & e )
{
 if( f_quiet > 0 )
  return;
 if constexpr( ( role_of< E > == Role::BulkInsert ) ||
               ( role_of< E > == Role::BulkErase ) ) {
  using H = typename decltype( e.hs )::value_type;
  auto c = std::make_shared< const std::vector< H > >( e.hs.begin() ,
                                                      e.hs.end() );
  f_buf.emplace_back( E{ std::span< const H >( *c ) } );
  f_keep.push_back( std::move( c ) );
  }
 else if constexpr( role_of< E > == Role::Permute ) {
  using H = typename decltype( e.old_of_new )::value_type;
  auto c = std::make_shared< const std::vector< H > >(
                                e.old_of_new.begin() , e.old_of_new.end() );
  f_buf.emplace_back( E{ std::span< const H >( *c ) } );
  f_keep.push_back( std::move( c ) );
  }
 else
  f_buf.emplace_back( e );
 }

template< class L , class Obs , class St , class Dy >
template< class S >
void Subject< L , Obs , St , Dy >::close( const S & s )
{
 ADS_PRE( f_depth > 0 );
 if( --f_depth > 0 )
  return;
 flush();
 audit( s );
 }

template< class L , class Obs , class St , class Dy >
void Subject< L , Obs , St , Dy >::discard( std::size_t m )
{
 ADS_PRE( f_depth > 0 );
 if( m < f_buf.size() )
  f_buf.erase( f_buf.begin() + std::ptrdiff_t( m ) , f_buf.end() );
 if( --f_depth == 0 ) {
  f_buf.clear();
  f_keep.clear();
  }
 }

template< class L , class Obs , class St , class Dy >
void Subject< L , Obs , St , Dy >::flush( void )
{
 struct Hold {
  std::vector< std::shared_ptr< const void > > & k;
  ~Hold() { k.clear(); }
  } hold{ f_keep };
 if( f_buf.empty() )
  return;
 const std::vector< Event > C = coalesce_traits< L >::coalesce(
                                std::span< const Event >( f_buf ) );
 f_buf.clear();
 if( C.empty() )
  return;
 std::uint64_t kinds = 0;
 std::array< std::size_t , K > count{};
 for( const auto & e : C ) {
  kinds |= std::uint64_t( 1 ) << e.index();
  ++count[ e.index() ];
  }
 const std::uint64_t w = mask() & kinds;
 if( w == 0 )
  return;
 Group g;
 g.from = version();
 for( std::size_t k = 0 ; k < K ; ++k )
  if( ( w >> k ) & 1u )
   f_rev[ k ] += count[ k ];
 g.to = version();
 g.size = C.size();
 std::vector< Event > tmp;
 f_st.deliverGroup( std::span< const Event >( C ) , kinds , g , tmp );
 if constexpr( dynamic ) {
  f_dy.f_busy = true;
  struct Done {
   bool & b;
   ~Done() { b = false; }
   } done{ f_dy.f_busy };
  f_dy.deliverGroup( std::span< const Event >( C ) , kinds , g , tmp );
  }
 }

template< class L , class Obs , class St , class Dy >
void Subject< L , Obs , St , Dy >::rebuilt( void )
{
 if constexpr( contains_v< Rebuilt , L > ) {
  if( ! live< Rebuilt >() )
   return;
  ++f_rev[ index_v< Rebuilt , L > ];
  const Rebuilt e;
  Trail t;
  f_st.template deliver< Rebuilt , false >( e , t );
  if constexpr( dynamic ) {
   f_dy.f_busy = true;
   struct Done {
    bool & b;
    ~Done() { b = false; }
    } done{ f_dy.f_busy };
   f_dy.template deliver< Rebuilt , false >( e , t );
   }
  }
 }

template< class L , class Obs , class St , class Dy >
void Subject< L , Obs , St , Dy >::mute( bool rebuild )
{
 if( f_quiet++ == 0 )
  f_rebuild = false;
 f_rebuild = f_rebuild || rebuild;
 if( rebuild && ( f_depth > 0 ) )
  spoil();
 }

template< class L , class Obs , class St , class Dy >
template< class S >
void Subject< L , Obs , St , Dy >::unmute( const S & s )
{
 ADS_PRE( f_quiet > 0 );
 if( --f_quiet > 0 )
  return;
 const bool rebuild = std::exchange( f_rebuild , false );
 if( f_depth > 0 )
  return;                               // the batch has its Rebuilt
 if( rebuild )
  rebuilt();
 audit( s );
 }

/*--------------------------------------------------------------------------*/
/*---------------------------- THE COALESCING ------------------------------*/
/*--------------------------------------------------------------------------*/

template< class E >
struct isInserted : std::false_type {};
template< class H , class K , class V >
struct isInserted< Inserted< H , K , V > > : std::true_type {};

template< class E >
struct isErased : std::false_type {};
template< class H , class K , class V >
struct isErased< Erased< H , K , V > > : std::true_type {};

template< class E >
struct isChanged : std::false_type {};
template< class A , class H , class V >
struct isChanged< Changed< A , H , V > > : std::true_type {};

template< class E >
struct isShifted : std::false_type {};
template< class A , class D >
struct isShifted< Shifted< A , D > > : std::true_type {};

/// whether E is an event of one item, coalesced per handle
template< class E >
inline constexpr bool element = isInserted< E >::value ||
                                isErased< E >::value || isChanged< E >::value;

/// whether the handles of E can be hashed and compared
template< class E >
inline constexpr bool hashed = [] {
 if constexpr( element< E > )
  return( std::equality_comparable< typename E::handle_type > &&
          requires( const typename E::handle_type & h ) {
           { std::hash< typename E::handle_type >{}( h ) }
            -> std::convertible_to< std::size_t >; } );
 else
  return( false );
 }();

/// a == b, never if they cannot be compared
template< class X , class Y >
bool same( const X & a , const Y & b )
{
 if constexpr( std::equality_comparable_with< X , Y > )
  return( bool( a == b ) );
 else
  return( false );
 }

/// the value after two changes of one attribute, the first leading to
/// `to` and the second being \f$ x \f$, by the reduction R
template< class R , class V , class E >
void reduceTo( V & to , const E & x )
{
 if constexpr( std::is_same_v< R , reduce::min > ) {
  if( x.to < to )
   to = x.to;
  }
 else if constexpr( std::is_same_v< R , reduce::max > ) {
  if( to < x.to )
   to = x.to;
  }
 else if constexpr( std::is_same_v< R , reduce::sum > )
  to = to + ( x.to - x.from );
 else
  to = x.to;
 }

/// the value of the erasure \f$ e \f$ set to \f$ v \f$
template< class E , class X >
void restoreValue( E & e , const X & v )
{
 e.value = v;
 }

/// the event \f$ e \f$ of one handle pushed on the net events \f$ N \f$ of
/// that handle, by the rule of \ref theory_ADS_modifications
template< class L , class E >
void pushNet( std::vector< variant_t< L > > & N , E e )
{
 if constexpr( isChanged< E >::value ) {
  using A = typename E::attribute;
  if( same( e.from , e.to ) )
   return;                              // a touch
  for( std::size_t k = N.size() ; k-- > 0 ; ) {
   // 1 composed, 2 composed into no change, 0 commuted, 3 stopped
   const int r = std::visit( [ & ]( auto & y ) -> int {
    using Y = std::decay_t< decltype( y ) >;
    if constexpr( std::is_same_v< Y , E > ) {
     reduceTo< typename reduction_of< A >::type >( y.to , e );
     return( same( y.from , y.to ) ? 2 : 1 );
     }
    else if constexpr( isChanged< Y >::value )
     return( 0 );                       // another attribute: they commute
    else if constexpr( isInserted< Y >::value ) {
     if constexpr( std::is_same_v< A , Key > &&
                   std::is_same_v< typename Y::key_type ,
                                   typename E::value_type > ) {
      y.key = e.to;
      return( 1 );
      }
     else if constexpr( std::is_same_v< A , Value > &&
                        std::is_same_v< typename Y::value_type ,
                                        typename E::value_type > ) {
      y.value = e.to;
      return( 1 );
      }
     else
      return( 3 );
     }
    else
     return( 3 ); } , N[ k ] );
   if( r == 1 )
    return;
   if( r == 2 ) {
    N.erase( N.begin() + std::ptrdiff_t( k ) );
    return;
    }
   if( r == 3 )
    break;
   }
  N.emplace_back( std::move( e ) );
  }
 else if constexpr( isErased< E >::value ) {
  using H = typename E::handle_type;
  using K = typename E::key_type;
  using V = typename E::value_type;
  // the changes before it vanish, their first values restored in it
  while( ! N.empty() ) {
   const bool change = std::visit( [ & ]( const auto & y ) -> bool {
    using Y = std::decay_t< decltype( y ) >;
    if constexpr( isChanged< Y >::value ) {
     using A = typename Y::attribute;
     if constexpr( std::is_same_v< A , Key > &&
                   std::is_same_v< typename Y::value_type , K > )
      e.key = y.from;
     else if constexpr( std::is_same_v< A , Value > &&
                        std::is_same_v< typename Y::value_type , V > )
      restoreValue( e , y.from );
     return( true );
     }
    else
     return( false ); } , N.back() );
   if( ! change )
    break;
   N.pop_back();
   }
  // an insertion and an erasure cancel
  if constexpr( contains_v< Inserted< H , K , V > , L > )
   if( ! N.empty() &&
       std::holds_alternative< Inserted< H , K , V > >( N.back() ) ) {
    N.pop_back();
    return;
    }
  N.emplace_back( std::move( e ) );
  }
 else if constexpr( isInserted< E >::value ) {
  using H = typename E::handle_type;
  using K = typename E::key_type;
  using V = typename E::value_type;
  using Er = Erased< H , K , V >;
  if constexpr( contains_v< Er , L > ) {
   if( ! N.empty() && std::holds_alternative< Er >( N.back() ) ) {
    // an erasure and an insertion: the changes from one to the other
    const Er y = std::get< Er >( N.back() );
    const bool sk = same( y.key , e.key );
    bool sv = true;
    if constexpr( ! std::is_void_v< V > )
     sv = same( y.value , e.value );
    constexpr bool hasK = contains_v< KeyChanged< H , K > , L >;
    constexpr bool hasV = [] {
     if constexpr( std::is_void_v< V > )
      return( true );
     else
      return( contains_v< Assigned< H , V > , L > );
     }();
    if( ( sk || hasK ) && ( sv || hasV ) ) {
     N.pop_back();
     if constexpr( hasK )
      if( ! sk )
       N.emplace_back( KeyChanged< H , K >{ e.h , y.key , e.key } );
     if constexpr( ! std::is_void_v< V > ) {
      if constexpr( contains_v< Assigned< H , V > , L > )
       if( ! sv )
        N.emplace_back( Assigned< H , V >{ e.h , y.value , e.value } );
      }
     return;
     }
    }
   }
  N.emplace_back( std::move( e ) );
  }
 else
  N.emplace_back( std::move( e ) );
 }

/// an event that is not of one item appended to the net events \f$ C \f$:
/// consecutive shifts of one attribute add up, consecutive replacements of
/// one attribute are one
template< class L >
void pushGlobal( std::vector< variant_t< L > > & C ,
                 const variant_t< L > & x )
{
 if( ! C.empty() && ( C.back().index() == x.index() ) ) {
  const bool merged = std::visit( [ & ]( auto & y ) -> bool {
   using Y = std::decay_t< decltype( y ) >;
   if constexpr( isShifted< Y >::value ) {
    y.delta = y.delta + std::get< Y >( x ).delta;
    return( true );
    }
   else if constexpr( role_of< Y > == Role::Replace )
    return( true );
   else
    return( false ); } , C.back() );
  if( merged ) {
   const bool null = std::visit( []( const auto & y ) -> bool {
    using Y = std::decay_t< decltype( y ) >;
    if constexpr( isShifted< Y >::value )
     return( same( y.delta , decltype( y.delta )() ) );
    else
     return( false ); } , C.back() );
   if( null )
    C.pop_back();
   return;
   }
  }
 C.push_back( x );
 }

/// the net events of a run of events of items appended to \f$ C \f$: one
/// list of net events per handle, in the order of their first event
template< class L >
void coalesceRun( std::span< const variant_t< L > > R ,
                  std::vector< variant_t< L > > & C )
{
 using Var = variant_t< L >;
 struct Chain {
  Var first;                     ///< the first event of the handle
  std::vector< Var > net;        ///< its net events
  };
 std::vector< Chain > chains;
 std::unordered_multimap< std::size_t , std::size_t > at;
 at.reserve( R.size() );
 for( const auto & x : R ) {
  const std::optional< std::size_t > key = std::visit(
   []( const auto & e ) -> std::optional< std::size_t > {
    using E = std::decay_t< decltype( e ) >;
    if constexpr( hashed< E > )
     return( std::hash< typename E::handle_type >{}( e.h ) );
    else
     return( std::nullopt ); } , x );
  std::size_t c = chains.size();
  if( key ) {
   auto [ b , end ] = at.equal_range( *key );
   for( ; b != end ; ++b ) {
    const bool sameHandle = std::visit(
     [ & ]( const auto & p , const auto & q ) -> bool {
      using P = std::decay_t< decltype( p ) >;
      using Q = std::decay_t< decltype( q ) >;
      if constexpr( hashed< P > && hashed< Q > ) {
       if constexpr( std::is_same_v< typename P::handle_type ,
                                     typename Q::handle_type > )
        return( p.h == q.h );
       else
        return( false );
       }
      else
       return( false ); } , chains[ b->second ].first , x );
    if( sameHandle ) {
     c = b->second;
     break;
     }
    }
   }
  if( c == chains.size() ) {
   chains.push_back( Chain{ x , {} } );
   if( key )
    at.emplace( *key , c );
   }
  std::visit( [ & ]( const auto & e ) {
   pushNet< L >( chains[ c ].net , e ); } , x );
  }
 for( auto & ch : chains )
  for( auto & y : ch.net )
   C.push_back( std::move( y ) );
 }

}  // end( namespace detail )
/// \endcond

template< class L >
std::vector< variant_t< L > > coalesce_traits< L >::coalesce(
                                       std::span< const variant_t< L > > B )
{
 using Var = variant_t< L >;
 auto roleAt = []( const Var & x ) {
  return( std::visit( []( const auto & e ) {
   return( role_of< std::decay_t< decltype( e ) > > ); } , x ) );
  };
 auto itemAt = []( const Var & x ) {
  return( std::visit( []( const auto & e ) {
   return( detail::element< std::decay_t< decltype( e ) > > ); } , x ) );
  };
 // everything before the last Cleared or Rebuilt is dropped
 std::size_t i = 0;
 bool nuclear = false;
 for( std::size_t k = 0 ; k < B.size() ; ++k ) {
  const Role r = roleAt( B[ k ] );
  if( ( r == Role::Clear ) || ( r == Role::Rebuild ) ) {
   i = k;
   nuclear = true;
   }
  }
 std::vector< Var > C;
 C.reserve( B.size() - i );
 if( nuclear )
  C.push_back( B[ i++ ] );
 // the runs of events of items, between the other events
 while( i < B.size() ) {
  if( ! itemAt( B[ i ] ) ) {
   detail::pushGlobal< L >( C , B[ i ] );
   ++i;
   continue;
   }
  std::size_t j = i + 1;
  while( ( j < B.size() ) && itemAt( B[ j ] ) )
   ++j;
  detail::coalesceRun< L >( B.subspan( i , j - i ) , C );
  i = j;
  }
 return( C );
 }

}  // end( namespace ADS::mod )

namespace ADS {

/*--------------------------------------------------------------------------*/
/*------------------------------ CLASS UndoLog -----------------------------*/
/*--------------------------------------------------------------------------*/

template< class L >
template< class S >
void UndoLog< L >::rollback( S & s , std::size_t t )
{
 ADS_PRE( ( t >= f_floor ) && ( t <= checkpoint() ) );
 f_replay = true;
 struct Done {
  bool & b;
  ~Done() { b = false; }
  } done{ f_replay };
 auto b = ADS::batch( s );
 while( checkpoint() > t ) {
  const Event e = std::move( f_log.back() );
  f_log.pop_back();
  std::visit( [ & ]( const auto & x ) {
   if constexpr( mod::invertible< std::decay_t< decltype( x ) > > )
    s.apply( inverse( x ) ); } , e );
  }
 b.close();
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File Modification.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
