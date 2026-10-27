/*--------------------------------------------------------------------------*/
/*---------------------- File SubsequenceProblems.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of SubsequenceProblems [see SubsequenceProblems.h], with
 * their contracts.
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

template< class S > requires std::is_class_v< S >
bool isInterleaving( const S & X , const S & Y , const S & Z )
{
 const std::size_t n = X.size() , m = Y.size();
 if( Z.size() != n + m )
  return( false );
 std::vector< char > D( m + 1 , 0 );           // the row i of the table
 D[ 0 ] = 1;
 for( std::size_t j = 1 ; j <= m ; ++j )
  D[ j ] = D[ j - 1 ] && ( Y[ j - 1 ] == Z[ j - 1 ] );
 for( std::size_t i = 1 ; i <= n ; ++i ) {
  D[ 0 ] = D[ 0 ] && ( X[ i - 1 ] == Z[ i - 1 ] );
  for( std::size_t j = 1 ; j <= m ; ++j )
   D[ j ] = ( D[ j ] && ( X[ i - 1 ] == Z[ i + j - 1 ] ) ) ||
            ( D[ j - 1 ] && ( Y[ j - 1 ] == Z[ i + j - 1 ] ) );
  }
 return( D[ m ] != 0 );
 }

/*--------------------------------------------------------------------------*/

inline bool isInterleaving( const std::string & X , const std::string & Y ,
                            const std::string & Z )
{
 return( isInterleaving< std::string >( X , Y , Z ) );
 }

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
std::size_t scsLength( const S & X , const S & Y )
{
 const std::size_t n = X.size() , m = Y.size();
 std::vector< std::size_t > L( m + 1 , 0 );    // the row i of the LCS
 for( std::size_t i = 1 ; i <= n ; ++i ) {
  std::size_t diagonal = 0;                     // L[ i - 1 ][ j - 1 ]
  for( std::size_t j = 1 ; j <= m ; ++j ) {
   const std::size_t up = L[ j ];
   L[ j ] = X[ i - 1 ] == Y[ j - 1 ] ? diagonal + 1
                                     : std::max( up , L[ j - 1 ] );
   diagonal = up;
   }
  }
 const std::size_t r = n + m - L[ m ];
 ADS_AUDIT( r == n + m - lcs( X , Y ).size() );
 return( r );
 }

/*--------------------------------------------------------------------------*/

inline std::size_t scsLength( const std::string & X , const std::string & Y )
{
 return( scsLength< std::string >( X , Y ) );
 }

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
S shortestCommonSupersequence( const S & X , const S & Y )
{
 const auto L = lcsTable( X , Y );
 std::size_t i = X.size() , j = Y.size();
 S out;
 while( ( i > 0 ) && ( j > 0 ) )
  if( X[ i - 1 ] == Y[ j - 1 ] ) {
   out.push_back( X[ i - 1 ] );
   --i;
   --j;
   }
  else if( L[ i - 1 ][ j ] >= L[ i ][ j - 1 ] )
   out.push_back( X[ --i ] );
  else
   out.push_back( Y[ --j ] );
 while( i > 0 )
  out.push_back( X[ --i ] );
 while( j > 0 )
  out.push_back( Y[ --j ] );
 std::reverse( out.begin() , out.end() );
 ADS_POST( out.size() == X.size() + Y.size() -
                         std::size_t( L[ X.size() ][ Y.size() ] ) );
 ADS_AUDIT( detail::isSubsequence( X , out ) &&
            detail::isSubsequence( Y , out ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

inline std::string shortestCommonSupersequence( const std::string & X ,
                                                const std::string & Y )
{
 return( shortestCommonSupersequence< std::string >( X , Y ) );
 }

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
std::uint64_t subsequenceOccurrences( const S & P , const S & T )
{
 const std::size_t m = P.size();
 std::vector< std::uint64_t > C( m + 1 , 0 );  // the row of T[ 0 .. i )
 C[ 0 ] = 1;
 for( std::size_t i = 0 ; i < T.size() ; ++i )
  for( std::size_t j = std::min( m , i + 1 ) ; j > 0 ; --j )
   if( T[ i ] == P[ j - 1 ] )
    C[ j ] += C[ j - 1 ];
 return( C[ m ] );
 }

/*--------------------------------------------------------------------------*/

inline std::uint64_t subsequenceOccurrences( const std::string & P ,
                                             const std::string & T )
{
 return( subsequenceOccurrences< std::string >( P , T ) );
 }

/*--------------------------------------------------------------------------*/

template< class S > requires std::is_class_v< S >
S lcs3( const S & X , const S & Y , const S & Z )
{
 const std::size_t a = X.size() , b = Y.size() , c = Z.size();
 std::vector< std::size_t > D( ( a + 1 ) * ( b + 1 ) * ( c + 1 ) , 0 );
 auto at = [ & ]( std::size_t i , std::size_t j , std::size_t k )
                                                       -> std::size_t & {
  return( D[ ( i * ( b + 1 ) + j ) * ( c + 1 ) + k ] );
  };
 for( std::size_t i = 1 ; i <= a ; ++i )
  for( std::size_t j = 1 ; j <= b ; ++j )
   for( std::size_t k = 1 ; k <= c ; ++k )
    at( i , j , k ) = ( X[ i - 1 ] == Y[ j - 1 ] ) &&
                      ( Y[ j - 1 ] == Z[ k - 1 ] )
                      ? at( i - 1 , j - 1 , k - 1 ) + 1
                      : std::max( { at( i - 1 , j , k ) ,
                                    at( i , j - 1 , k ) ,
                                    at( i , j , k - 1 ) } );
 S out;
 for( std::size_t i = a , j = b , k = c ; ( i > 0 ) && ( j > 0 ) &&
                                          ( k > 0 ) ; )
  if( ( X[ i - 1 ] == Y[ j - 1 ] ) && ( Y[ j - 1 ] == Z[ k - 1 ] ) ) {
   out.push_back( X[ i - 1 ] );
   --i;
   --j;
   --k;
   }
  else if( at( i , j , k ) == at( i - 1 , j , k ) )
   --i;
  else if( at( i , j , k ) == at( i , j - 1 , k ) )
   --j;
  else
   --k;
 std::reverse( out.begin() , out.end() );
 ADS_POST( out.size() == at( a , b , c ) );
 ADS_AUDIT( detail::isSubsequence( out , X ) &&
            detail::isSubsequence( out , Y ) &&
            detail::isSubsequence( out , Z ) );
 return( out );
 }

/*--------------------------------------------------------------------------*/

inline std::string lcs3( const std::string & X , const std::string & Y ,
                         const std::string & Z )
{
 return( lcs3< std::string >( X , Y , Z ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------- End File SubsequenceProblems.tpp --------------------*/
/*--------------------------------------------------------------------------*/
