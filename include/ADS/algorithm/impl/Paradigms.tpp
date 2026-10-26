/*--------------------------------------------------------------------------*/
/*--------------------------- File Paradigms.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The schemes of Paradigms.h.
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

template< class Z , class Next , class Last , class A , class O >
auto enumerateSearch( Z z , Next && next , Last && last , A && a , O && o )
 -> std::optional< decltype( o( z ) ) >
{
 for( ; ; z = next( z ) ) {
  if( a( z ) )
   return( o( z ) );
  if( last( z ) )
   return( std::nullopt );
  }
 }

template< class Z , class Next , class Last , class A , class O , class M ,
          class B >
auto enumerateOptimize( Z z , Next && next , Last && last , A && a ,
                        O && o , M && m , B && better )
 -> std::optional< decltype( o( z ) ) >
{
 std::optional< Z > best;
 for( ; ; z = next( z ) ) {
  if( a( z ) && ( ! best || better( m( z ) , m( *best ) ) ) )
   best = z;
  if( last( z ) )
   break;
  }
 if( ! best )
  return( std::nullopt );
 return( o( *best ) );
 }

template< class N , class C , class P , class L , class V >
bool backtrack( const N & node , C && children , P && prune , L && leaf ,
                V && visit )
{
 if( prune( node ) )
  return( false );
 if( leaf( node ) )
  return( visit( node ) );
 for( const auto & c : children( node ) )
  if( backtrack( c , children , prune , leaf , visit ) )
   return( true );
 return( false );
 }

template< class X , class Cand , class Best >
std::vector< X > greedy( int k , Cand && candidates , Best && best )
{
 std::vector< X > x;
 for( int i = 0 ; i < k ; ++i ) {
  const std::vector< X > c = candidates( i , x );
  if( ! c.empty() )
   x.push_back( best( c ) );
  }
 return( x );
 }

template< class X , class S , class D , class V , class C >
auto divideEtImpera( const X & x , S && small , D && direct , V && divide ,
                     C && combine ) -> decltype( direct( x ) )
{
 if( small( x ) )
  return( direct( x ) );
 std::vector< decltype( direct( x ) ) > parts;
 for( const auto & y : divide( x ) )
  parts.push_back( divideEtImpera( y , small , direct , divide , combine ) );
 return( combine( parts ) );
 }

template< class T , class B >
std::vector< T > maximal( const std::vector< T > & X , B && better )
{
 std::vector< T > M;
 for( const auto & x : X ) {
  bool dominated = false;
  for( const auto & y : X )
   if( better( y , x ) ) {
    dominated = true;
    break;
    }
  if( ! dominated )
   M.push_back( x );
  }
 return( M );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File Paradigms.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
