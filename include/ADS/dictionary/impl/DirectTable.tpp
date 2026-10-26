/*--------------------------------------------------------------------------*/
/*-------------------------- File DirectTable.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of DirectTable [see DirectTable.h], with their contracts.
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

template< class V >
DirectTable< V >::DirectTable( long a , long b ) : f_a( a )
{
 ADS_PRE( ( a <= b ) && ( std::size_t( b ) - std::size_t( a ) <
                          std::numeric_limits< std::size_t >::max() ) );
 f_A.resize( std::size_t( b ) - std::size_t( a ) + 1 );
 }

template< class V >
std::optional< V > DirectTable< V >::lookup( long k ) const
{
 return( f_A[ H( k ) ] );
 }

template< class V >
void DirectTable< V >::insert( long k , const V & v )
{
 auto & c = f_A[ H( k ) ];
 if( ! c )
  ++f_size;
 c = v;
 ADS_POST( f_A[ H( k ) ].has_value() );
 ADS_INVARIANT( counted() );
 }

template< class V >
void DirectTable< V >::remove( long k )
{
 auto & c = f_A[ H( k ) ];
 if( c )
  --f_size;
 c.reset();
 ADS_POST( ! f_A[ H( k ) ].has_value() );
 ADS_INVARIANT( counted() );
 }

template< class V >
std::size_t DirectTable< V >::size( void ) const
{
 return( f_size );
 }

template< class V >
std::size_t DirectTable< V >::capacity( void ) const
{
 return( f_A.size() );
 }

template< class V >
bool DirectTable< V >::inRange( long k ) const
{
 return( ( k >= f_a ) &&
         ( std::size_t( k ) - std::size_t( f_a ) < f_A.size() ) );
 }

template< class V >
std::size_t DirectTable< V >::H( long k ) const
{
 ADS_PRE( inRange( k ) );
 return( std::size_t( k ) - std::size_t( f_a ) );
 }

template< class V >
bool DirectTable< V >::counted( void ) const
{
 std::size_t n = 0;
 for( const auto & c : f_A )
  n += c.has_value();
 return( n == f_size );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File DirectTable.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
