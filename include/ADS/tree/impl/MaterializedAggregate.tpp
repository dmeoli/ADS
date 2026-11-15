/*--------------------------------------------------------------------------*/
/*--------------------- File MaterializedAggregate.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MaterializedAggregate [see MaterializedAggregate.h].
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

namespace ADS
{

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
template< detail::pair_visitable S >
void MaterializedAggregate< G , Period >::bind( const S & s )
{
 f_src = &s;
 f_redo = []( const void * p , const MaterializedAggregate & m ) {
  const S & t = *static_cast< const S * >( p );
  if constexpr( kind == Kind::Selective ) {
   m.f_set.clear();
   t.forEach( [ & ]( const auto & k , const auto & v ) {
    m.f_set.insert( Value( G::from( k , v ) ) );
    } );
   m.f_a = m.f_set.empty() ? Value( G::identity() ) : *m.f_set.begin();
   }
  else
   m.f_a = fold( t );
  };
 recompute();
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
template< class H , class K , class V >
void MaterializedAggregate< G , Period >::on(
  const mod::Inserted< H , K , V > & e )
{
 if constexpr( kind == Kind::Selective ) {
  f_set.insert( Value( G::from( e.key , e.value ) ) );
  f_a = *f_set.begin();
  }
 else if( ! f_stale ) {
  f_a = G::combine( f_a , G::from( e.key , e.value ) );
  changed();
  }
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
template< class H , class K , class V >
void MaterializedAggregate< G , Period >::on(
  const mod::Erased< H , K , V > & e ) noexcept
{
 if constexpr( kind == Kind::Group ) {
  if( ! f_stale ) {
   f_a = G::combine( f_a , G::inverse( G::from( e.key , e.value ) ) );
   changed();
   }
  }
 else if constexpr( kind == Kind::Selective ) {
  const auto it = f_set.find( Value( G::from( e.key , e.value ) ) );
  if( it != f_set.end() )
   f_set.erase( it );
  f_a = f_set.empty() ? Value( G::identity() ) : *f_set.begin();
  }
 else
  f_stale = true;
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
template< class H , class V >
void MaterializedAggregate< G , Period >::on(
  const mod::Assigned< H , V > & e )
{
 if constexpr( kind == Kind::Group ) {
  if( ! f_stale ) {
   f_a = G::combine( G::combine( f_a , G::from( e.h , e.to ) ) ,
		     G::inverse( G::from( e.h , e.from ) ) );
   changed();
   }
  }
 else if constexpr( kind == Kind::Selective ) {
  f_set.insert( Value( G::from( e.h , e.to ) ) );
  const auto it = f_set.find( Value( G::from( e.h , e.from ) ) );
  if( it != f_set.end() )
   f_set.erase( it );
  f_a = *f_set.begin();
  }
 else
  f_stale = true;
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
void MaterializedAggregate< G , Period >::on( const mod::Cleared & ) noexcept
{
 if constexpr( kind == Kind::Selective )
  f_set.clear();
 f_a = G::identity();
 f_stale = false;
 f_drift = 0;
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
auto MaterializedAggregate< G , Period >::value( void ) const -> Value
{
 if( f_stale )
  recompute();
 return( f_a );
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
template< detail::pair_visitable S >
bool MaterializedAggregate< G , Period >::audit( const S & s ) const
{
 if( f_stale )
  return( true );  // nothing to compare yet
 const Value b = fold( s );
 if constexpr( std::is_floating_point_v< Value > )
  return( std::abs( f_a - b ) <= 1e-9 * ( 1 + std::abs( b ) ) );
 else if constexpr( std::equality_comparable< Value > )
  return( f_a == b );
 else
  return( true );
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
void MaterializedAggregate< G , Period >::changed( void ) noexcept
{
 if constexpr( ( kind == Kind::Group ) && ! exact_v< G > && ( Period > 0 ) )
  if( ( ++f_drift >= Period ) && f_src )
   f_stale = true;
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
void MaterializedAggregate< G , Period >::recompute( void ) const
{
 ADS_PRE( f_src != nullptr );
 f_redo( f_src , *this );
 f_stale = false;
 f_drift = 0;
 ++f_recomputed;
 }

template< class G , std::size_t Period >
 requires CommutativeMonoid< G >
template< class S >
auto MaterializedAggregate< G , Period >::fold( const S & s ) -> Value
{
 Value a = G::identity();
 s.forEach( [ & ]( const auto & k , const auto & v ) {
  a = G::combine( a , G::from( k , v ) );
  } );
 return( a );
 }

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File MaterializedAggregate.tpp ------------------*/
/*--------------------------------------------------------------------------*/
