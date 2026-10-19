/*--------------------------------------------------------------------------*/
/*---------------------- File SequenceAlgorithms.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The algorithms of SequenceAlgorithms.h, with their contracts.
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
/*------------------------------- HELPERS ----------------------------------*/
/*--------------------------------------------------------------------------*/

namespace detail {

template< Sequence S >
void append( S & s , const typename S::Item & v )
{
 s.insert( s.next( s.tail() ) , v );
 }

/// the run of s from p to the end of a sorted stretch appended to d; the
/// position after it
template< Sequence S >
typename S::Pos copyRun( const S & s , typename S::Pos p , S & d )
{
 for( ; ; ) {
  append( d , s.read( p ) );
  const auto q = s.next( p );
  if( s.finished( q ) || ( s.read( q ) < s.read( p ) ) ) return( q );
  p = q;
  }
 }

/// the number of items of s
template< Sequence S >
long length( const S & s )
{
 long n = 0;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) ) ++n;
 return( n );
 }

/// no item of s appears twice, O( n^2 ), as a contract [see Sorted.h]
template< Sequence S >
bool distinct( const S & s )
{
 return( contract( [ & ]( void ) {
  for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
   for( auto q = s.next( p ) ; ! s.finished( q ) ; q = s.next( q ) )
    if( s.read( q ) == s.read( p ) ) return( false );
  return( true );
  } ) );
 }

}  // end( namespace detail )

template< Sequence S >
Order order( const S & s );

namespace detail {

/// s is sorted in increasing order, repetitions allowed, as a contract
/// [see Sorted.h]
template< Sequence S >
bool isSorted( const S & s )
{
 return( contract( [ & ]( void ) {
  const Order o = order( s );
  return( ( o == Order::EMPTY ) || ( o == Order::CONSTANT ) ||
          ( o == Order::INCREASING ) || ( o == Order::NONDECREASING ) );
  } ) );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------------- SEARCH -----------------------------------*/
/*--------------------------------------------------------------------------*/

template< Sequence S >
long search( const S & s , const typename S::Item & v )
{
 long i = 0;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) , ++i )
  if( s.read( p ) == v ) return( i );
 return( -( i + 1 ) );
 }

template< Sequence S >
long searchOrdered( const S & s , const typename S::Item & v )
{
 ADS_AUDIT( detail::isSorted( s ) );
 long i = 0;
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) ) {
  ++i;
  if( s.read( p ) == v ) return( i - 1 );
  if( v < s.read( p ) ) break;
  }
 return( i > 0 ? -i : -1 );              // never 0 , the place of a hit
 }

template< Sequence S >
std::pair< typename S::Pos , bool > lowerBound( const S & s ,
                                                const typename S::Item & v )
{
 ADS_AUDIT( detail::isSorted( s ) );
 auto p = s.head();
 while( ! s.finished( p ) && ( s.read( p ) < v ) )
  p = s.next( p );
 return( std::pair( p , ! s.finished( p ) && ( s.read( p ) == v ) ) );
 }

template< Sequence S >
S sublist( const S & s , typename S::Pos p , typename S::Pos q )
{
 S r;
 if( s.finished( p ) )
  return( r );
 for( ; ; p = s.next( p ) ) {
  ADS_PRE( ! s.finished( p ) );            // q follows p
  detail::append( r , s.read( p ) );
  if( p == q )
   return( r );
  }
 }

/*--------------------------------------------------------------------------*/
/*------------------------------- ORDER ------------------------------------*/
/*--------------------------------------------------------------------------*/

template< Sequence S >
Order order( const S & s )
{
 if( s.empty() ) return( Order::EMPTY );
 bool up = false , down = false , equal = false;
 for( auto p = s.head() , q = s.next( p ) ; ! s.finished( q ) ;
      p = q , q = s.next( q ) ) {
  if( s.read( p ) < s.read( q ) ) up = true;
  else if( s.read( q ) < s.read( p ) ) down = true;
  else equal = true;
  }
 if( up && down ) return( Order::NONE );
 if( ! up && ! down ) return( Order::CONSTANT );
 if( up ) return( equal ? Order::NONDECREASING : Order::INCREASING );
 return( equal ? Order::NONINCREASING : Order::DECREASING );
 }


/*--------------------------------------------------------------------------*/
/*------------------------------- SORTING ----------------------------------*/
/*--------------------------------------------------------------------------*/

template< Sequence S >
void bubbleSort( S & s )
{
 [[maybe_unused]] const long n = ADS_CONTRACTS >= 3 ? detail::length( s ) : 0;
 for( bool swapped = true ; swapped && ! s.empty() ; ) {
  swapped = false;
  for( auto p = s.head() , q = s.next( p ) ; ! s.finished( q ) ;
       p = q , q = s.next( q ) )
   if( s.read( q ) < s.read( p ) ) {
    const auto x = s.read( p );
    s.write( p , s.read( q ) );
    s.write( q , x );
    swapped = true;
    }
  }
 ADS_AUDIT( detail::isSorted( s ) && ( detail::length( s ) == n ) );
 }

template< Sequence S >
S mergeSorted( const S & a , const S & b )
{
 ADS_AUDIT( detail::isSorted( a ) && detail::isSorted( b ) );
 S c;
 auto p = a.head();
 auto q = b.head();
 while( ! a.finished( p ) && ! b.finished( q ) )
  if( b.read( q ) < a.read( p ) ) {
   detail::append( c , b.read( q ) );
   q = b.next( q );
   }
  else {
   detail::append( c , a.read( p ) );
   p = a.next( p );
   }
 for( ; ! a.finished( p ) ; p = a.next( p ) )
  detail::append( c , a.read( p ) );
 for( ; ! b.finished( q ) ; q = b.next( q ) )
  detail::append( c , b.read( q ) );
 ADS_AUDIT( detail::isSorted( c ) &&
            ( detail::length( c ) ==
              detail::length( a ) + detail::length( b ) ) );
 return( c );
 }

template< Sequence S >
void naturalMergeSort( S & s , long * passes )
{
 [[maybe_unused]] const long n = ADS_CONTRACTS >= 3 ? detail::length( s ) : 0;
 if( passes ) *passes = 0;
 for( ; ; ) {
  S a , b;                               // distribute
  long runs = 0;
  for( auto p = s.head() ; ! s.finished( p ) ; ++runs )
   p = detail::copyRun( s , p , runs % 2 ? b : a );
  if( runs <= 1 ) break;
  if( passes ) ++*passes;
  S m;                                   // merge the runs in pairs
  auto p = a.head();
  auto q = b.head();
  while( ! a.finished( p ) || ! b.finished( q ) ) {
   S x , y;
   if( ! a.finished( p ) ) p = detail::copyRun( a , p , x );
   if( ! b.finished( q ) ) q = detail::copyRun( b , q , y );
   const S z = mergeSorted( x , y );
   for( auto r = z.head() ; ! z.finished( r ) ; r = z.next( r ) )
    detail::append( m , z.read( r ) );
   }
  s = std::move( m );
  }
 ADS_AUDIT( detail::isSorted( s ) && ( detail::length( s ) == n ) );
 }

template< Sequence S >
void removeDuplicates( S & s )
{
 for( auto p = s.head() ; ! s.finished( p ) ; p = s.next( p ) )
  for( auto q = s.next( p ) ; ! s.finished( q ) ; )
   if( s.read( q ) == s.read( p ) ) q = s.remove( q );
   else q = s.next( q );
 ADS_AUDIT( detail::distinct( s ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File SequenceAlgorithms.tpp ---------------------*/
/*--------------------------------------------------------------------------*/
