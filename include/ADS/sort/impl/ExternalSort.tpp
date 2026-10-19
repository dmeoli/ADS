/*--------------------------------------------------------------------------*/
/*------------------------- File ExternalSort.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of ExternalSort.h, with their contracts.
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

template< class T , class C >
std::vector< std::vector< T > > replacementSelection(
                       const std::vector< T > & input , std::size_t M ,
                       C less )
{
 ADS_PRE( M >= 1 );
 struct Item {
  std::size_t run;                          // the run it belongs to
  T value;
  };
 auto better = [ & ]( const Item & a , const Item & b ) {
  return( ( a.run < b.run ) || ( ( a.run == b.run ) &&
                                 less( a.value , b.value ) ) ); };
 DaryHeap< Item , decltype( better ) > H( better );
 std::size_t next = 0;
 for( ; ( next < input.size() ) && ( H.size() < M ) ; ++next )
  H.push( Item{ 0 , input[ next ] } );
 std::vector< std::vector< T > > runs;
 while( ! H.empty() ) {
  Item x = H.pop();
  if( x.run == runs.size() )
   runs.emplace_back();
  if( next < input.size() ) {               // the next item takes its place
   const T & y = input[ next++ ];
   H.push( Item{ less( y , x.value ) ? x.run + 1 : x.run , y } );
   }
  runs.back().push_back( std::move( x.value ) );
  }
 ADS_AUDIT( [ & ]( void ) {
  std::vector< T > all;
  for( const auto & r : runs ) {
   if( ! std::is_sorted( r.begin() , r.end() , less ) ) return( false );
   all.insert( all.end() , r.begin() , r.end() );
   }
  return( detail::samePermutation( all , input , less ) );
  }() );
 return( runs );
 }

template< class T , class C >
std::vector< T > polyphaseMerge( std::vector< std::vector< T > > runs ,
                                 std::size_t tapes , C less ,
                                 std::size_t * phases )
{
 ADS_PRE( tapes >= 3 );
 ADS_PRE( std::all_of( runs.begin() , runs.end() , [ & ]( const auto & r ) {
  return( std::is_sorted( r.begin() , r.end() , less ) ); } ) );
 std::vector< T > all;                      // for the audit
 if constexpr( ADS_CONTRACTS >= 3 )
  for( const auto & r : runs ) all.insert( all.end() , r.begin() , r.end() );
 const std::size_t P = tapes - 1;
 std::vector< std::size_t > a( P , 1 );     // a perfect distribution
 auto total = [ & ]( void ) {
  std::size_t s = 0;
  for( std::size_t x : a ) s += x;
  return( s ); };
 while( total() < runs.size() ) {           // the next level: a'_i =
  std::vector< std::size_t > b( P );        // a_1 + a_{i+1}
  for( std::size_t i = 0 ; i < P ; ++i )
   b[ i ] = a[ 0 ] + ( i + 1 < P ? a[ i + 1 ] : 0 );
  a = b;
  }
 std::vector< std::deque< std::vector< T > > > tape( tapes );
 std::size_t r = 0;                         // the real runs first, then
 for( std::size_t i = 0 ; i < P ; ++i )     // the dummy ( empty ) ones
  for( std::size_t k = 0 ; k < a[ i ] ; ++k )
   tape[ i ].push_back( r < runs.size() ? std::move( runs[ r++ ] )
                                        : std::vector< T >() );
 std::size_t out = P , count = 0;
 auto left = [ & ]( void ) {
  std::size_t s = 0;
  for( const auto & t : tape ) s += t.size();
  return( s ); };
 while( left() > 1 ) {                      // a phase
  std::size_t k = std::size_t( -1 );        // merges until an input empties
  for( std::size_t i = 0 ; i < tapes ; ++i )
   if( i != out ) k = std::min( k , tape[ i ].size() );
  for( std::size_t step = 0 ; step < k ; ++step ) {
   std::vector< std::vector< T > > in;
   for( std::size_t i = 0 ; i < tapes ; ++i )
    if( i != out ) {
     in.push_back( std::move( tape[ i ].front() ) );
     tape[ i ].pop_front();
     }
   std::vector< T > merged;                 // the k-way merge of a step
   std::vector< std::size_t > pos( in.size() , 0 );
   for( ; ; ) {
    std::size_t best = in.size();
    for( std::size_t j = 0 ; j < in.size() ; ++j )
     if( ( pos[ j ] < in[ j ].size() ) &&
         ( ( best == in.size() ) ||
           less( in[ j ][ pos[ j ] ] , in[ best ][ pos[ best ] ] ) ) )
      best = j;
    if( best == in.size() ) break;
    merged.push_back( std::move( in[ best ][ pos[ best ]++ ] ) );
    }
   tape[ out ].push_back( std::move( merged ) );
   }
  ++count;
  for( std::size_t i = 0 ; i < tapes ; ++i )   // the empty input tape is
   if( ( i != out ) && tape[ i ].empty() ) {   // the next output
    out = i;
    break;
    }
  }
 std::vector< T > result;
 for( auto & t : tape )
  if( ! t.empty() ) result = std::move( t.front() );
 if( phases ) *phases = count;
 ADS_AUDIT( detail::sortedPermutation( result , all , less ) );
 return( result );
 }

template< class T , class C >
std::vector< T > externalSort( const std::vector< T > & input ,
                               std::size_t M , std::size_t tapes , C less )
{
 return( polyphaseMerge( replacementSelection( input , M , less ) , tapes ,
                         less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*----------------------- End File ExternalSort.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
