/*--------------------------------------------------------------------------*/
/*------------------------- File SetOperations.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SetOperations [see SetOperations.h], with their contracts.
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

template< IterableSet S >
S setUnion( const S & A , const S & B )
{
 S C = A;
 B.forEach( [ & ]( const auto & x ) { C.insert( x ); } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '|' ) );
 return( C );
 }

template< IterableSet S >
S setIntersection( const S & A , const S & B )
{
 const bool a = A.size() <= B.size();
 const S & X = a ? A : B;
 const S & Y = a ? B : A;
 S C = X;
 X.forEach( [ & ]( const auto & x ) {
  if( ! Y.contains( x ) )
   C.remove( x );
  } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '&' ) );
 return( C );
 }

template< IterableSet S >
S setDifference( const S & A , const S & B )
{
 S C = A;
 A.forEach( [ & ]( const auto & x ) {
  if( B.contains( x ) )
   C.remove( x );
  } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '-' ) );
 return( C );
 }

template< IterableSet S >
S setSymmetricDifference( const S & A , const S & B )
{
 S C = setDifference( A , B );
 B.forEach( [ & ]( const auto & x ) {
  if( ! A.contains( x ) )
   C.insert( x );
  } );
 ADS_AUDIT( detail::isSetOperation( C , A , B , '^' ) );
 return( C );
 }

namespace detail {

template< class C , class A , class B >
bool isSetOperation( const C & c , const A & a , const B & b , char op )
{
 auto in = [ & ]( const auto & x ) {
  const bool p = a.contains( x ) , q = b.contains( x );
  return( op == '|' ? ( p || q ) : op == '&' ? ( p && q ) :
          op == '-' ? ( p && ! q ) : ( p != q ) );
  };
 bool ok = true;
 c.forEach( [ & ]( const auto & x ) { ok = ok && in( x ); } );
 auto agree = [ & ]( const auto & x ) {
  ok = ok && ( in( x ) == c.contains( x ) );
  };
 a.forEach( agree );
 b.forEach( agree );
 return( ok );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File SetOperations.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
