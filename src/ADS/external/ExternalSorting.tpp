/*--------------------------------------------------------------------------*/
/*------------------------ File ExternalSorting.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The functions of ExternalSorting.h, with their contracts.
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

/// log2( n! )
inline double log2Factorial( double n )
{
 return( std::lgamma( n + 1 ) / std::log( 2.0 ) );
 }

/// log2 of the binomial coefficient ( M choose B )
inline double log2Binomial( double M , double B )
{
 return( log2Factorial( M ) - log2Factorial( B ) -
         log2Factorial( M - B ) );
 }

/// the smallest integer not below x, with a margin for the rounding
inline std::uint64_t ceilUp( double x )
{
 return( x <= 0 ? 0 : std::uint64_t( std::ceil( x - 1e-9 ) ) );
 }

/// a and b hold the same items in the same order, by less
template< class T , class C >
bool sameItems( const std::vector< T > & a , const std::vector< T > & b ,
                C less )
{
 if( a.size() != b.size() ) return( false );
 for( std::size_t i = 0 ; i < a.size() ; ++i )
  if( less( a[ i ] , b[ i ] ) || less( b[ i ] , a[ i ] ) ) return( false );
 return( true );
 }

/// the runs of R = floor( M / B ) B items of in, each sorted in memory
template< class T , class C >
std::vector< DiskFile< T > > loadSortRuns( const DiskFile< T > & in ,
                                           C less )
{
 TwoLevelMemory & mem = in.memory();
 const std::size_t B = mem.blockSize() , R = mem.memorySize() / B * B;
 std::vector< DiskFile< T > > runs;
 MemoryLease lease( mem , R );
 std::vector< T > run;
 for( std::size_t b = 0 ; b < in.blocks() ; ) {
  run.resize( R );
  std::size_t len = 0;
  for( ; ( b < in.blocks() ) && ( len + B <= R ) ; ++b )
   len += in.readBlock( b , run.data() + len );
  run.resize( len );
  sort( run , less );
  runs.emplace_back( mem );
  for( std::size_t off = 0 ; off < len ; off += B )
   runs.back().appendBlock( run.data() + off , std::min( B , len - off ) );
  }
 return( runs );
 }

/// the runs of in by replacement selection, a heap of M - 2 B items
template< class T , class C >
std::vector< DiskFile< T > > snowPlowRuns( const DiskFile< T > & in ,
                                           C less )
{
 TwoLevelMemory & mem = in.memory();
 const std::size_t H = mem.memorySize() - 2 * mem.blockSize();
 struct Item {
  std::size_t run;                          // the run it belongs to
  T value;
  };
 auto better = [ & ]( const Item & a , const Item & b ) {
  return( ( a.run < b.run ) || ( ( a.run == b.run ) &&
                                 less( a.value , b.value ) ) ); };
 std::vector< DiskFile< T > > runs;
 {
  MemoryLease lease( mem , H );
  DiskReader< T > rd( in );
  DaryHeap< Item , decltype( better ) > heap( better );
  while( rd.hasNext() && ( heap.size() < H ) )
   heap.push( Item{ 0 , rd.next() } );
  DiskFile< T > cur( mem );
  std::unique_ptr< DiskWriter< T > > w;
  std::size_t r = 0;
  while( ! heap.isEmpty() ) {
   Item x = heap.pop();
   if( ! w || ( x.run != r ) ) {            // a new run starts
    if( w ) {
     w.reset();
     runs.push_back( std::move( cur ) );
     cur.clear();
     }
    r = x.run;
    w = std::make_unique< DiskWriter< T > >( cur );
    }
   w->push( x.value );
   if( rd.hasNext() ) {
    T y = rd.next();
    heap.push( Item{ less( y , x.value ) ? x.run + 1 : x.run , y } );
    }
   }
  if( w ) {
   w.reset();
   runs.push_back( std::move( cur ) );
   }
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  const auto ref = replacementSelection( in.contents() , H , less );
  if( ref.size() != runs.size() ) return( false );
  for( std::size_t i = 0 ; i < ref.size() ; ++i )
   if( ! sameItems( ref[ i ] , runs[ i ].contents() , less ) )
    return( false );
  return( true ); } ) );
 return( runs );
 }

/// the runs lo , ... , hi - 1 merged into dest through a tree of losers, a
/// block per run and one for dest; the runs are emptied
template< class T , class C >
void mergeGroup( std::vector< DiskFile< T > > & runs , std::size_t lo ,
                 std::size_t hi , DiskFile< T > & dest , C less )
{
 std::vector< std::unique_ptr< DiskReader< T > > > rd;
 std::vector< std::optional< T > > first;
 for( std::size_t i = lo ; i < hi ; ++i ) {
  rd.push_back( std::make_unique< DiskReader< T > >( runs[ i ] ) );
  first.push_back( rd.back()->hasNext() ?
                   std::optional< T >( rd.back()->next() ) : std::nullopt );
  }
 LoserTree< T , C > L( std::move( first ) , less );
 DiskWriter< T > w( dest );
 while( ! L.isEmpty() ) {
  const std::size_t j = L.winner();
  w.push( L.top() );
  L.replace( rd[ j ]->hasNext() ?
             std::optional< T >( rd[ j ]->next() ) : std::nullopt );
  }
 w.close();
 for( std::size_t i = lo ; i < hi ; ++i ) runs[ i ].clear();
 }

/// the passes of the merge of fan-in k, the last one into out
template< class T , class C >
void mergePasses( std::vector< DiskFile< T > > runs , std::size_t k ,
                  DiskFile< T > & out , C less )
{
 TwoLevelMemory & mem = out.memory();
 while( runs.size() > 1 ) {
  std::vector< DiskFile< T > > next;
  for( std::size_t g = 0 ; g < runs.size() ; g += k ) {
   const std::size_t hi = std::min( g + k , runs.size() );
   if( hi - g == 1 )                        // a lone run is not copied
    next.push_back( std::move( runs[ g ] ) );
   else {
    next.emplace_back( mem );
    mergeGroup( runs , g , hi , next.back() , less );
    }
   }
  runs = std::move( next );
  }
 if( runs.size() == 1 ) out = std::move( runs[ 0 ] );
 }

/// the items of F sorted into out by distribution with k - 1 pivots
template< class T , class R , class C >
void distribute( const DiskFile< T > & F , DiskWriter< T > & out , R & g ,
                 C less , std::size_t k )
{
 TwoLevelMemory & mem = F.memory();
 const std::size_t B = mem.blockSize() , M = mem.memorySize() ,
                   n = F.size();
 if( n <= M - B ) {                         // sorted in memory
  MemoryLease lease( mem , n );
  std::vector< T > A( n );
  for( std::size_t b = 0 , len = 0 ; b < F.blocks() ; ++b )
   len += F.readBlock( b , A.data() + len );
  sort( A , less );
  for( const T & x : A ) out.push( x );
  return;
  }
 const std::size_t a1 = std::size_t( std::ceil( 12 * std::log( double( k ) )
                                                ) );
 const std::size_t s = std::min( { a1 * k - 1 , ( M - 2 * B ) / 2 , n } );
 auto equal = [ & ]( const T & a , const T & b ) {
  return( ! less( a , b ) && ! less( b , a ) ); };
 for( int attempt = 0 ; ; ++attempt ) {
  std::vector< T > piv;
  {                                         // the sample and its positions
   MemoryLease lease( mem , 2 * s );
   std::vector< T > S;
   {
    DiskReader< T > rd( F );
    for( const std::size_t p : selectionSampling( s , n , g ) )
     S.push_back( rd.at( p ) );
    }
   sort( S , less );
   for( std::size_t i = 1 ; i < k ; ++i )
    piv.push_back( S[ i * s / k ] );
   piv.erase( std::unique( piv.begin() , piv.end() , equal ) , piv.end() );
   }
  const std::size_t P = piv.size();
  std::vector< DiskFile< T > > bucket;      // ranges at 2 j, pivots at 2 j + 1
  for( std::size_t i = 0 ; i < 2 * P + 1 ; ++i ) bucket.emplace_back( mem );
  {
   MemoryLease lease( mem , P );
   DiskReader< T > rd( F );
   std::vector< std::unique_ptr< DiskWriter< T > > > w;
   for( auto & b : bucket )
    w.push_back( std::make_unique< DiskWriter< T > >( b ) );
   while( rd.hasNext() ) {
    const T x = rd.next();
    const std::size_t j = lowerBound( piv , x , less );
    w[ ( j < P ) && ! less( x , piv[ j ] ) ? 2 * j + 1 : 2 * j ]->push( x );
    }
   }
  piv = std::vector< T >();                 // out of memory before the calls
  bool balanced = true;                     // Lemma 5.1: below 4 n / k
  for( std::size_t j = 0 ; j <= P ; ++j )
   if( ( bucket[ 2 * j ].size() > M - B ) &&
       ( bucket[ 2 * j ].size() * k >= 4 * n ) )
    balanced = false;
  if( ! balanced && ( attempt < 2 ) ) continue;
  for( std::size_t j = 0 ; j <= P ; ++j ) {
   distribute( bucket[ 2 * j ] , out , g , less , k );
   bucket[ 2 * j ].clear();
   if( j < P ) {                            // the items equal to a pivot
    {
     DiskReader< T > rd( bucket[ 2 * j + 1 ] );
     while( rd.hasNext() ) out.push( rd.next() );
     }
    bucket[ 2 * j + 1 ].clear();
    }
   }
  return;
  }
 }

}  // end( namespace detail )

inline std::uint64_t mergeSortIOs( std::size_t n , std::size_t M ,
                                   std::size_t B , std::size_t k )
{
 ADS_PRE( ( B >= 1 ) && ( M >= B ) && ( k >= 2 ) );
 if( n == 0 ) return( 0 );
 const std::uint64_t NB = ( n + B - 1 ) / B , R = M / B * B ,
                     r = ( n + R - 1 ) / R;
 std::uint64_t passes = 0;
 for( std::uint64_t x = 1 ; x < r ; x *= k ) ++passes;
 return( 2 * NB * ( 1 + passes ) );
 }

inline std::uint64_t sortingLowerBound( std::size_t n , std::size_t M ,
                                        std::size_t B )
{
 ADS_PRE( ( B >= 1 ) && ( M >= 2 * B ) );
 const std::uint64_t NB = ( n + B - 1 ) / B;
 const double need = detail::log2Factorial( double( n ) ) -
                     double( NB ) * detail::log2Factorial( double( B ) );
 const std::uint64_t reads = detail::ceilUp(
                need / detail::log2Binomial( double( M ) , double( B ) ) );
 return( NB + std::max( NB , reads ) );
 }

inline std::uint64_t permutingLowerBound( std::size_t n , std::size_t M ,
                                          std::size_t B )
{
 ADS_PRE( ( B >= 1 ) && ( M >= 2 * B ) );
 const std::uint64_t NB = ( n + B - 1 ) / B;
 const double need = detail::log2Factorial( double( n ) ) -
                     double( NB ) * detail::log2Factorial( double( B ) );
 const double per = std::log2( double( n ) + 1 ) +
                    detail::log2Binomial( double( M ) , double( B ) );
 return( std::max( 2 * NB , detail::ceilUp( need / per ) ) );
 }

template< class T , class C >
void binaryMergeSort( const DiskFile< T > & in , DiskFile< T > & out ,
                      C less )
{
 TwoLevelMemory & mem = in.memory();
 ADS_PRE( mem.memorySize() >= 3 * mem.blockSize() );
 ADS_PRE( ( &out.memory() == &mem ) && out.isEmpty() );
 auto before = detail::auditCopy( in.contents() );
 detail::mergePasses( detail::loadSortRuns( in , less ) , 2 , out , less );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sortedPermutation( out.contents() , std::move( before ) ,
                                     less ) ); } ) );
 }

template< class T , class C >
void multiwayMergeSort( const DiskFile< T > & in , DiskFile< T > & out ,
                        C less , bool snowPlow )
{
 TwoLevelMemory & mem = in.memory();
 const std::size_t k = mem.memorySize() / mem.blockSize() - 1;
 ADS_PRE( k >= 2 );
 ADS_PRE( ( &out.memory() == &mem ) && out.isEmpty() );
 auto before = detail::auditCopy( in.contents() );
 detail::mergePasses( snowPlow ? detail::snowPlowRuns( in , less ) :
                                 detail::loadSortRuns( in , less ) ,
                      k , out , less );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sortedPermutation( out.contents() , std::move( before ) ,
                                     less ) ); } ) );
 }

template< class T , class R , class C >
void distributionSort( const DiskFile< T > & in , DiskFile< T > & out ,
                       R & g , C less )
{
 TwoLevelMemory & mem = in.memory();
 const std::size_t B = mem.blockSize() , M = mem.memorySize();
 ADS_PRE( M >= 5 * B + 2 );
 ADS_PRE( ( &out.memory() == &mem ) && out.isEmpty() );
 auto before = detail::auditCopy( in.contents() );
 {
  DiskWriter< T > w( out );
  detail::distribute( in , w , g , less , ( M - B ) / ( 2 * B + 1 ) );
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::sortedPermutation( out.contents() , std::move( before ) ,
                                     less ) ); } ) );
 }

namespace detail {

/// pi is a permutation of 0 , ... , n - 1
inline bool isPermutation( const std::vector< std::size_t > & pi ,
                           std::size_t n )
{
 if( pi.size() != n ) return( false );
 std::vector< bool > seen( n , false );
 for( const std::size_t p : pi ) {
  if( ( p >= n ) || seen[ p ] ) return( false );
  seen[ p ] = true;
  }
 return( true );
 }

/// out[ i ] = in[ pi[ i ] ], checked item by item
template< class T >
bool permuted( const std::vector< T > & out , const std::vector< T > & in ,
               const std::vector< std::size_t > & pi )
{
 if( out.size() != pi.size() ) return( false );
 for( std::size_t i = 0 ; i < pi.size() ; ++i )
  if( ! detail::equal( out[ i ] , in[ pi[ i ] ] ) ) return( false );
 return( true );
 }

}  // end( namespace detail )

template< class T >
void permuteBySorting( const DiskFile< T > & in ,
                       const DiskFile< std::size_t > & pi ,
                       DiskFile< T > & out )
{
 TwoLevelMemory & mem = in.memory();
 ADS_PRE( ( &out.memory() == &mem ) && ( &pi.memory() == &mem ) &&
          out.isEmpty() );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::isPermutation( pi.contents() , in.size() ) ); } ) );
 struct Triple {
  std::size_t i;                            // where the item goes
  std::size_t p;                            // where it comes from
  T x;
  };
 DiskFile< Triple > P( mem ) , Q( mem );
 {                                          // scan: the pairs < i , pi[ i ] >
  DiskReader< std::size_t > rd( pi );
  DiskWriter< Triple > w( P );
  for( std::size_t i = 0 ; rd.hasNext() ; ++i )
   w.push( Triple{ i , rd.next() , T() } );
  }
 multiwayMergeSort( P , Q , []( const Triple & a , const Triple & b ) {
  return( a.p < b.p ); } );
 P.clear();
 {                                          // scan: the items filled in
  DiskReader< Triple > rq( Q );
  DiskReader< T > ri( in );
  DiskWriter< Triple > w( P );
  while( rq.hasNext() ) {
   Triple t = rq.next();
   t.x = ri.next();                         // t.p is the place of ri
   w.push( t );
   }
  }
 Q.clear();
 multiwayMergeSort( P , Q , []( const Triple & a , const Triple & b ) {
  return( a.i < b.i ); } );
 P.clear();
 {                                          // scan: the items in place
  DiskReader< Triple > rq( Q );
  DiskWriter< T > w( out );
  while( rq.hasNext() ) w.push( rq.next().x );
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::permuted( out.contents() , in.contents() ,
                            pi.contents() ) ); } ) );
 }

template< class T >
void permuteDirect( const DiskFile< T > & in ,
                    const DiskFile< std::size_t > & pi ,
                    DiskFile< T > & out )
{
 TwoLevelMemory & mem = in.memory();
 ADS_PRE( ( &out.memory() == &mem ) && ( &pi.memory() == &mem ) &&
          out.isEmpty() );
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::isPermutation( pi.contents() , in.size() ) ); } ) );
 {
  DiskReader< std::size_t > rp( pi );
  DiskReader< T > ri( in );
  DiskWriter< T > w( out );
  while( rp.hasNext() ) w.push( ri.at( rp.next() ) );
  }
 ADS_AUDIT( detail::contract( [ & ]( void ) {
  return( detail::permuted( out.contents() , in.contents() ,
                            pi.contents() ) ); } ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ExternalSorting.tpp ----------------------*/
/*--------------------------------------------------------------------------*/
