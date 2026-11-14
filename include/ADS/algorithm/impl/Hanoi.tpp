/*--------------------------------------------------------------------------*/
/*----------------------------- File Hanoi.tpp -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The definitions of the functions of Hanoi.h [see Hanoi.h].
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
void hanoi_recursive( int n , int source , int dest , int middle , F && f )
{
 ADS_PRE( ( n >= 1 ) && ( source != dest ) && ( source != middle ) &&
          ( dest != middle ) );
 if( n == 1 )
  f( source , dest );
 else {
  hanoi_recursive( n - 1 , source , middle , dest , f );   // (1)
  f( source , dest );
  hanoi_recursive( n - 1 , middle , dest , source , f );   // (2)
  }
 }

template< class F >
void hanoi_iterative( int n , int source , int dest , int middle , F && f )
{
 ADS_PRE( ( n >= 1 ) && ( source != dest ) && ( source != middle ) &&
          ( dest != middle ) );
 ArrayStack< std::tuple< int , int , int , int > > S( n > 0 ? n : 1 );
 bool end_of_the_world = false;
 while( ! end_of_the_world ) {
  while( n != 1 ) {
   S.push( { n , source , dest , middle } );
   --n;
   std::swap( dest , middle );
   }
  f( source , dest );
  if( S.empty() )
   end_of_the_world = true;
  else {
   std::tie( n , source , dest , middle ) = S.pop();
   f( source , dest );
   --n;
   std::swap( source , middle );
   }
  }
 }

template< class F >
void hanoi_iterative_labels( int n , int source , int dest , int middle ,
                             F && f )
{
 ADS_PRE( ( n >= 1 ) && ( source != dest ) && ( source != middle ) &&
          ( dest != middle ) );
 enum Label { START , AFTER1 , END , STOP };
 using Frame = std::tuple< int , int , int , int , Label >;
 ArrayStack< Frame > S( 2 * ( n > 0 ? n : 1 ) );
 Label pos = START;
 while( pos != STOP )
  if( pos == START ) {
   if( n == 1 ) {
    f( source , dest );
    pos = END;
    }
   else {
    S.push( { n , source , dest , middle , AFTER1 } );
    --n;
    std::swap( dest , middle );
    }
   }
  else
   if( pos == AFTER1 ) {
    f( source , dest );
    S.push( { n , source , dest , middle , END } );
    --n;
    std::swap( source , middle );
    pos = START;
    }
   else   // END
    if( S.empty() )
     pos = STOP;
    else
     std::tie( n , source , dest , middle , pos ) = S.pop();
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------------- End File Hanoi.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
