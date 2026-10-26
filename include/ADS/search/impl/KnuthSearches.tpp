/*--------------------------------------------------------------------------*/
/*------------------------- File KnuthSearches.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The searches of KnuthSearches.h, with their contracts.
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

template< class T >
long fibonacciSearch( const std::vector< T > & A , const T & v , long * cmp )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 const long n = long( A.size() );
 long f2 = 0 , f1 = 1 , f = 1;              // F_{k-2} , F_{k-1} , F_k >= n
 while( f < n ) {
  f2 = f1;
  f1 = f;
  f = f1 + f2;
  }
 long offset = -1 , result = -1;            // the part ( offset , .. )
 while( f > 1 ) {
  const long i = std::min( offset + f2 , n - 1 );   // past the end: last
  if( cmp ) ++*cmp;
  if( A[ i ] < v ) {                        // the right part
   f = f1;
   f1 = f2;
   f2 = f - f1;
   offset = i;
   }
  else if( v < A[ i ] ) {                   // the left part
   f = f2;
   f1 = f1 - f2;
   f2 = f - f1;
   }
  else {
   result = i;
   break;
   }
  }
 if( ( result < 0 ) && ( f1 == 1 ) && ( offset + 1 < n ) &&
     !( A[ offset + 1 ] < v ) && !( v < A[ offset + 1 ] ) )
  result = offset + 1;
 ADS_POST( ( result >= 0 ) ==
           std::binary_search( A.begin() , A.end() , v ) );
 return( result );
 }

template< class T >
long uniformBinarySearch( const std::vector< T > & A , const T & v ,
                          long * cmp )
{
 ADS_AUDIT( std::is_sorted( A.begin() , A.end() ) );
 const long n = long( A.size() );
 std::vector< long > delta;                 // Algorithm C, 1-based
 for( long j = 1 ; ; ++j ) {
  const long d = ( n + ( 1L << ( j - 1 ) ) ) >> j;
  delta.push_back( d );
  if( d == 0 )
   break;
  }
 long result = -1;
 if( n > 0 ) {
  long i = delta[ 0 ] , j = 0;              // i is 1-based
  for( ; ; ) {                              // 0 and n + 1 are the keys
   const bool left = i > n , right = i < 1;   // -inf and +inf, not read
   if( ! left && ! right && cmp ) ++*cmp;
   if( left || ( ! right && ( v < A[ i - 1 ] ) ) ) {   // U3: to the left
    if( delta[ ++j ] == 0 ) break;
    i -= delta[ j ];
    }
   else if( right || ( A[ i - 1 ] < v ) ) {   // U4: to the right
    if( delta[ ++j ] == 0 ) break;
    i += delta[ j ];
    }
   else {
    result = i - 1;
    break;
    }
   }
  }
 ADS_POST( ( result >= 0 ) ==
           std::binary_search( A.begin() , A.end() , v ) );
 return( result );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File KnuthSearches.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
