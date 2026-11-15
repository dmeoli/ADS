/*--------------------------------------------------------------------------*/
/*----------------------- File StrongComponents.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The components of StrongComponents.h [see StrongComponents.h], with
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

namespace ADS
{

template< Graph G >
std::vector< int > sccTarjan( const G & g )
{
 const int n = g.bound();
 const detail::Rows< G > A( g );
 std::vector< int > dt( n , 0 ) , low( n , 0 ) , id( n , 0 );
 std::vector< int > S;                            // the stack of Tarjan
 std::vector< std::pair< int , std::size_t > > R;  // the recursion: the
 int t = 0 , k = 0;                                // node, its next arc
 auto discover = [ & ]( int v ) {
  dt[ v ] = low[ v ] = ++t;
  S.push_back( v );
  R.emplace_back( v , A.first( v ) );
  };
 for( int r = 0 ; r < n ; ++r ) {
  if( ( dt[ r ] != 0 ) || ! g.hasNode( r ) )
   continue;
  discover( r );
  while( ! R.empty() ) {
   const int u = R.back().first;
   if( R.back().second < A.last( u ) ) {
    const int v = A.head( R.back().second++ );
    if( dt[ v ] == 0 )  // the call
     discover( v );
    else if( id[ v ] == 0 )  // v is on S
     low[ u ] = std::min( low[ u ] , dt[ v ] );
    continue;
    }
   R.pop_back();                // the return
   if( low[ u ] == dt[ u ] ) {  // u opened its component
    ++k;
    int v;
    do {
     v = S.back();
     S.pop_back();
     id[ v ] = k;
     } while( v != u );
    }
   if( ! R.empty() )
    low[ R.back().first ] = std::min( low[ R.back().first ] , low[ u ] );
   }
  }
 for( int & c : id )  // the components close in reverse
  if( c != 0 )        // topological order
   c = k + 1 - c;
 ADS_POST( S.empty() );
 ADS_AUDIT( detail::topologicalComponents( g , id ) );
 return( id );
 }

template< Graph G >
std::vector< int > sccPearce( const G & g )
{
 const int n = g.bound();
 const detail::Rows< G > A( g );
 std::vector< int > rindex( n , 0 );
 std::vector< bool > root( n , false );
 std::vector< int > S;
 std::vector< std::pair< int , std::size_t > > R;  // the recursion
 int i = 1 , c = n - 1;
 auto discover = [ & ]( int v ) {
  root[ v ] = true;
  rindex[ v ] = i++;
  R.emplace_back( v , A.first( v ) );
  };
 // the numbers of discovery in use are below i, the i - 1 open nodes are
 // at most the c + 1 names not closed, and a closed node is numbered above
 // c: its number is never below that of an open node
 for( int r = 0 ; r < n ; ++r ) {
  if( ( rindex[ r ] != 0 ) || ! g.hasNode( r ) )
   continue;
  discover( r );
  while( ! R.empty() ) {
   const int u = R.back().first;
   if( R.back().second < A.last( u ) ) {
    const int v = A.head( R.back().second++ );
    if( rindex[ v ] == 0 )  // the call
     discover( v );
    else if( rindex[ v ] < rindex[ u ] ) {
     rindex[ u ] = rindex[ v ];
     root[ u ] = false;
     }
    continue;
    }
   R.pop_back();      // the return
   if( root[ u ] ) {  // u opened its component
    --i;
    while( ! S.empty() && ( rindex[ u ] <= rindex[ S.back() ] ) ) {
     rindex[ S.back() ] = c;
     S.pop_back();
     --i;
     }
    rindex[ u ] = c--;
    }
   else
    S.push_back( u );
   if( ! R.empty() && ( rindex[ u ] < rindex[ R.back().first ] ) ) {
    rindex[ R.back().first ] = rindex[ u ];
    root[ R.back().first ] = false;
    }
   }
  }
 for( int u = 0 ; u < n ; ++u )  // the components close in reverse
  if( g.hasNode( u ) )         // topological order, from n - 1
   rindex[ u ] -= c;           // down to c + 1
  else
   rindex[ u ] = 0;
 ADS_POST( S.empty() && ( i == 1 ) );
 ADS_AUDIT( detail::topologicalComponents( g , rindex ) );
 return( rindex );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*--------------------- End File StrongComponents.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
