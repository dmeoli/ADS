/*--------------------------------------------------------------------------*/
/*-------------------------- File MultiStack.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of MultiStack [see MultiStack.h], with their contracts.
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

template< class T >
MultiStack< T >::MultiStack( int k )
{
 ADS_PRE( k > 0 );
 f_top.assign( k , -1 );
 f_size.assign( k , 0 );
 }

template< class T >
int MultiStack< T >::stacks( void ) const
{
 return( int( f_top.size() ) );
 }

template< class T >
bool MultiStack< T >::empty( int i ) const
{
 ADS_PRE( ( i >= 0 ) && ( i < stacks() ) );
 return( f_top[ i ] < 0 );
 }

template< class T >
int MultiStack< T >::size( int i ) const
{
 ADS_PRE( ( i >= 0 ) && ( i < stacks() ) );
 return( f_size[ i ] );
 }

template< class T >
const T & MultiStack< T >::top( int i ) const
{
 ADS_PRE( ( i >= 0 ) && ( i < stacks() ) && ( f_top[ i ] >= 0 ) );
 return( f_C[ f_top[ i ] ].value );
 }

template< class T >
void MultiStack< T >::push( int i , const T & v )
{
 ADS_PRE( ( i >= 0 ) && ( i < stacks() ) );
 int c = f_free;
 if( c < 0 ) {                          // v copied before f_C moves, as
  f_C.push_back( { v , f_top[ i ] } );  // it may be one of its items
  c = int( f_C.size() ) - 1;
  }
 else {
  f_free = f_C[ c ].below;
  f_C[ c ] = { v , f_top[ i ] };
  }
 f_top[ i ] = c;
 ++f_size[ i ];
 ADS_POST( f_top[ i ] == c );
 ADS_INVARIANT( partitioned() );
 }

template< class T >
T MultiStack< T >::pop( int i )
{
 ADS_PRE( ( i >= 0 ) && ( i < stacks() ) && ( f_top[ i ] >= 0 ) );
 [[maybe_unused]] const int n = f_size[ i ];
 const int c = f_top[ i ];
 T v = std::move( f_C[ c ].value );
 f_top[ i ] = f_C[ c ].below;
 f_C[ c ].below = f_free;
 f_free = c;
 --f_size[ i ];
 ADS_POST( f_size[ i ] == n - 1 );
 ADS_INVARIANT( partitioned() );
 return( v );
 }

template< class T >
bool MultiStack< T >::partitioned( void ) const
{
 std::vector< bool > seen( f_C.size() , false );
 auto walk = [ & ]( int c , int count ) {
  for( ; c >= 0 ; c = f_C[ c ].below , --count ) {
   if( ( c >= int( f_C.size() ) ) || seen[ c ] ) return( false );
   seen[ c ] = true;
   }
  return( count <= 0 );
  };
 for( int i = 0 ; i < stacks() ; ++i ) {
  int n = 0;
  for( int c = f_top[ i ] ; ( c >= 0 ) && ( n <= int( f_C.size() ) ) ;
       c = f_C[ c ].below ) ++n;
  if( ( n != f_size[ i ] ) || ! walk( f_top[ i ] , n ) ) return( false );
  }
 if( ! walk( f_free , 0 ) ) return( false );
 for( bool s : seen ) if( ! s ) return( false );
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File MultiStack.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
