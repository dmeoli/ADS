/*--------------------------------------------------------------------------*/
/*-------------------------- File Enumeration.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Enumeration [see Enumeration.h], with their contracts.
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

template< class F >
long subsets( int n , int k , F && f )
{
 ADS_PRE( ( n >= 0 ) && ( k >= 0 ) );
 std::vector< bool > S( n , false );
 long count = 0;
 bool stop = false;
 auto rec = [ & ]( auto & self , int i , int c ) -> void {
  if( stop ) return;
  if( c == k ) {                                // a feasible solution
   ++count;
   stop = f( S );
   return;
   }
  if( ( i == n ) || ( c + ( n - i ) < k ) )     // pruned
   return;
  S[ i ] = true;
  self( self , i + 1 , c + 1 );
  S[ i ] = false;
  self( self , i + 1 , c );
  };
 rec( rec , 0 , 0 );
 return( count );
 }

namespace detail {

template< class T , class F >
long permute( const std::vector< T > & A , bool derange , F && f )
{
 const std::size_t n = A.size();
 std::vector< T > S;
 std::vector< bool > used( n , false );
 long count = 0;
 bool stop = false;
 auto rec = [ & ]( auto & self ) -> void {
  if( stop ) return;
  if( S.size() == n ) {
   ++count;
   stop = f( S );
   return;
   }
  for( std::size_t c = 0 ; c < n && ! stop ; ++c )
   if( ! used[ c ] && ! ( derange && ( c == S.size() ) ) ) {
    used[ c ] = true;                         // A.remove( c )
    S.push_back( A[ c ] );
    self( self );
    S.pop_back();
    used[ c ] = false;                        // A.insert( c )
    }
  };
 rec( rec );
 return( count );
 }

}  // end( namespace detail )

template< class T , class F >
long permutations( const std::vector< T > & A , F && f )
{
 return( detail::permute( A , false , f ) );
 }

template< class T , class F >
long derangements( const std::vector< T > & A , F && f )
{
 return( detail::permute( A , true , f ) );
 }

template< class T , class F >
long subsetSum( const std::vector< T > & A , T k , F && f )
{
 ADS_PRE( std::all_of( A.begin() , A.end() ,
                       []( const T & x ) { return( T( 0 ) < x ); } ) );
 std::vector< std::size_t > S;
 long count = 0;
 bool stop = false;
 auto rec = [ & ]( auto & self , std::size_t i , T sum ) -> void {
  if( stop ) return;
  if( sum == k ) {
   ++count;
   stop = f( S );
   return;
   }
  for( std::size_t j = i ; j < A.size() && ! stop ; ++j )
   if( sum + A[ j ] <= k ) {                 // pruned when it exceeds k
    S.push_back( j );
    self( self , j + 1 , sum + A[ j ] );
    S.pop_back();
    }
  };
 rec( rec , 0 , T( 0 ) );
 return( count );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Enumeration.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
