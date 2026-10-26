/*--------------------------------------------------------------------------*/
/*------------------------ File Nondeterminism.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Chooser [see Nondeterminism.h], with their contracts.
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

inline int Chooser::choice( int k )
{
 ADS_PRE( k >= 1 );
 if( f_depth == f_path.size() )
  f_path.push_back( { 0 , k } );
 const int c = f_path[ f_depth++ ].first;
 ADS_POST( ( c >= 0 ) && ( c < k ) );
 return( c );
 }

template< class A >
bool Chooser::run( A && alg , long * runs )
{
 f_path.clear();
 for( ; ; ) {
  f_depth = 0;
  if( runs ) ++*runs;
  if( alg( *this ) )
   return( true );                            // success
  f_path.resize( f_depth );                   // failure: the next sequence
  while( ! f_path.empty() && ( f_path.back().first + 1 ==
                               f_path.back().second ) )
   f_path.pop_back();
  if( f_path.empty() )
   return( false );
  ++f_path.back().first;
  }
 }

inline std::vector< int > Chooser::choices( void ) const
{
 std::vector< int > c;
 for( auto & [ x , k ] : f_path ) c.push_back( x );
 return( c );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File Nondeterminism.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
