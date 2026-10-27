/*--------------------------------------------------------------------------*/
/*---------------------------- File Sudoku.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Sudoku [see Sudoku.h], with their contracts.
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

namespace detail {

/// a square grid of side m^2, with values in [ 0 , m^2 ]
inline bool sudokuGrid( const Grid & S )
{
 const std::size_t n = S.size();
 std::size_t m = 0;
 while( m * m < n ) ++m;
 if( m * m != n ) return( false );
 for( const auto & row : S ) {
  if( row.size() != n ) return( false );
  for( int c : row )
   if( ( c < 0 ) || ( c > int( n ) ) ) return( false );
  }
 return( true );
 }

/// each value once per row, column and square, and no empty square if
/// complete
inline bool consistent( const Grid & S , bool complete )
{
 const std::size_t n = S.size();
 for( std::size_t x = 0 ; x < n ; ++x )
  for( std::size_t y = 0 ; y < n ; ++y ) {
   const int c = S[ x ][ y ];
   if( c == 0 ) {
    if( complete ) return( false );
    continue;
    }
   Grid T = S;
   T[ x ][ y ] = 0;
   if( ! check( T , x , y , c ) ) return( false );
   }
 return( true );
 }

}  // end( namespace detail )

inline bool check( const Grid & S , std::size_t x , std::size_t y , int c )
{
 ADS_PRE( ( x < S.size() ) && ( y < S.size() ) && ( c >= 1 ) &&
          ( c <= int( S.size() ) ) );
 const std::size_t N = S.size();
 std::size_t n = 1;
 while( n * n < N ) ++n;
 for( std::size_t j = 0 ; j < N ; ++j )
  if( ( S[ x ][ j ] == c ) || ( S[ j ][ y ] == c ) )   // the row, the column
   return( false );
 const std::size_t bx = x / n * n , by = y / n * n;
 for( std::size_t i = 0 ; i < n ; ++i )                 // the block
  for( std::size_t j = 0 ; j < n ; ++j )
   if( S[ bx + i ][ by + j ] == c )
    return( false );
 return( true );
 }

inline bool sudoku( Grid & S , std::size_t i )
{
 const std::size_t N = S.size();
 ADS_PRE( i <= N * N );
 if( i == 0 )
  ADS_AUDIT( detail::sudokuGrid( S ) && detail::consistent( S , false ) );
 if( i == N * N ) {
  ADS_AUDIT( detail::consistent( S , true ) );
  return( true );
  }
 const std::size_t x = i / N , y = i % N;
 if( S[ x ][ y ] != 0 )                                 // a given number
  return( sudoku( S , i + 1 ) );
 for( int c = 1 ; c <= int( N ) ; ++c )
  if( check( S , x , y , c ) ) {
   S[ x ][ y ] = c;
   if( sudoku( S , i + 1 ) )
    return( true );
   }
 S[ x ][ y ] = 0;
 return( false );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Sudoku.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
