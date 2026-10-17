/*--------------------------------------------------------------------------*/
/*---------------------------- File Huffman.tpp ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of Huffman [see Huffman.h], with their contracts.
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

namespace detail {

/// no code is a prefix of another, and none is empty
template< class C >
bool prefixFree( const std::map< C , std::string > & code )
{
 for( const auto & [ c , s ] : code ) {
  if( s.empty() ) return( false );
  for( const auto & [ d , t ] : code )
   if( ( c != d ) && ( t.compare( 0 , s.size() , s ) == 0 ) )
    return( false );
  }
 return( true );
 }

}  // end( namespace detail )

template< class C , class F >
std::map< C , std::string > huffman( const std::map< C , F > & freq )
{
 ADS_PRE( ! freq.empty() );
 ADS_PRE( std::all_of( freq.begin() , freq.end() , []( const auto & e ) {
  return( !( e.second < F( 0 ) ) ); } ) );
 struct Node { int left , right; C c; };
 std::vector< Node > T;                         // the nodes of the tree
 HeapPriorityQueue< int , F > Q( int( 2 * freq.size() ) );
 for( const auto & [ c , f ] : freq ) {
  T.push_back( { -1 , -1 , c } );
  Q.insert( int( T.size() ) - 1 , f );
  }
 while( Q.size() > 1 ) {
  const F f1 = Q.minPriority();
  const int c1 = Q.deleteMin();
  const F f2 = Q.minPriority();
  const int c2 = Q.deleteMin();
  T.push_back( { c1 , c2 , C() } );              // a virtual character
  Q.insert( int( T.size() ) - 1 , f1 + f2 );
  }
 std::map< C , std::string > code;
 auto walk = [ & ]( auto & self , int x , const std::string & s ) -> void {
  if( T[ x ].left < 0 ) {
   code[ T[ x ].c ] = s.empty() ? "0" : s;
   return;
   }
  self( self , T[ x ].left , s + '0' );
  self( self , T[ x ].right , s + '1' );
  };
 walk( walk , Q.deleteMin() , "" );
 ADS_POST( code.size() == freq.size() );
 ADS_AUDIT( detail::prefixFree( code ) );
 return( code );
 }

template< class C >
std::string encode( const std::basic_string< C > & text ,
                    const std::map< C , std::string > & code )
{
 std::string bits;
 for( C c : text ) {
  ADS_PRE( code.count( c ) > 0 );
  bits += code.at( c );
  }
 return( bits );
 }

template< class C >
std::basic_string< C > decode( const std::string & bits ,
                               const std::map< C , std::string > & code )
{
 ADS_AUDIT( detail::prefixFree( code ) );
 std::map< std::string , C > back;
 for( const auto & [ c , s ] : code )
  back[ s ] = c;
 std::basic_string< C > text;
 std::string s;
 for( char b : bits ) {
  s += b;
  if( auto it = back.find( s ) ; it != back.end() ) {
   text += it->second;
   s.clear();
   }
  }
 ADS_PRE( s.empty() );                   // no bits left over
 return( text );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- End File Huffman.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
