/*--------------------------------------------------------------------------*/
/*------------------------------ File Bag.tpp ------------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Bag [see Bag.h], with their contracts.
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

template< class T , class H , class Eq >
bool Bag< T , H , Eq >::insert( const T & x )
{
 [[maybe_unused]] const std::size_t c = count( x );
 //! [insert]
 if( std::vector< T > * g = f_T.find( x ) )
  g->push_back( x );
 else
  f_T.insert( x , std::vector< T >{ x } );
 ++f_n;
 //! [insert]
 ADS_POST( count( x ) == c + 1 );
 ADS_INVARIANT( counted() );
 return( true );
 }

template< class T , class H , class Eq >
bool Bag< T , H , Eq >::remove( const T & x )
{
 [[maybe_unused]] const std::size_t c = count( x );
 //! [remove]
 std::vector< T > * g = f_T.find( x );
 if( ! g )
  return( false );
 if( g->size() > 1 )
  g->pop_back();
 else
  f_T.remove( x );
 --f_n;
 //! [remove]
 ADS_POST( count( x ) == c - 1 );
 ADS_INVARIANT( counted() );
 return( true );
 }

template< class T , class H , class Eq >
const T * Bag< T , H , Eq >::find( const T & x ) const
{
 const std::vector< T > * g = f_T.find( x );
 return( g ? &g->front() : nullptr );
 }

template< class T , class H , class Eq >
std::vector< T > Bag< T , H , Eq >::findAll( const T & x ) const
{
 const std::vector< T > * g = f_T.find( x );
 return( g ? *g : std::vector< T >() );
 }

template< class T , class H , class Eq >
std::size_t Bag< T , H , Eq >::count( const T & x ) const
{
 const std::vector< T > * g = f_T.find( x );
 return( g ? g->size() : 0 );
 }

template< class T , class H , class Eq >
bool Bag< T , H , Eq >::counted( void ) const
{
 std::size_t s = 0;
 bool ok = true;
 f_T.forEach( [ & ]( const T & , const std::vector< T > & g ) {
  ok = ok && ! g.empty();
  s += g.size();
  } );
 return( ok && ( s == f_n ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------------- End File Bag.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
