/*--------------------------------------------------------------------------*/
/*--------------------------- File CsrMatrix.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CsrMatrix [see CsrMatrix.h], with their contracts.
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

template< class T , std::integral I , std::integral P >
BasicCsrMatrix< T , I , P >::BasicCsrMatrix( std::size_t m , std::size_t n )
    : f_m( m ) , f_n( n ) , f_row( m + 1 , P( 0 ) )
{ }

template< class T , std::integral I , std::integral P >
BasicCsrMatrix< T , I , P >::BasicCsrMatrix( std::size_t m , std::size_t n ,
					   std::vector< Triplet > E )
    : f_m( m ) , f_n( n ) , f_row( m + 1 , P( 0 ) )
{
 ADS_PRE( fits< I >( n ) && fits< P >( E.size() + 1 ) &&
	  std::all_of( E.begin() , E.end() , [ & ]( const Triplet & t ) {
	   return( ( std::get< 0 >( t ) < m ) && ( std::get< 1 >( t ) < n ) );
	   } ) );
 for( const auto & t : E )
  ++f_row[ std::get< 0 >( t ) + 1 ];
 for( std::size_t i = 0 ; i < m ; ++i )
  f_row[ i + 1 ] += f_row[ i ];
 std::vector< P > at( f_row.begin() , f_row.end() - 1 );
 std::vector< std::pair< I , T > > D( E.size() );
 for( const auto & [ i , j , v ] : E )  // by row, by a counting sort
  D[ std::size_t( at[ i ]++ ) ] = std::pair( I( j ) , v );
 f_col.reserve( E.size() );
 f_val.reserve( E.size() );
 std::size_t w = 0;
 for( std::size_t i = 0 ; i < m ; ++i ) {  // each row by column, the
  const auto b = D.begin() + std::ptrdiff_t( f_row[ i ] ) ,  // equal
    e = D.begin() + std::ptrdiff_t( f_row[ i + 1 ] );       // summed
  std::sort( b , e , []( const auto & x , const auto & y ) {
   return( x.first < y.first );
   } );
  f_row[ i ] = P( w );
  for( auto p = b ; p != e ; ++p )
   if( ( f_col.size() > w ) && ( f_col.back() == p->first ) )
    f_val.back() += p->second;
   else {
    f_col.push_back( p->first );
    f_val.push_back( p->second );
    }
  w = f_col.size();
  }
 f_row[ m ] = P( w );
 ADS_AUDIT( sorted() );
 }

template< class T , std::integral I , std::integral P >
BasicCsrMatrix< T , I , P >::BasicCsrMatrix( const SparseMatrix & S )
    : f_m( S.rows() ) , f_n( S.cols() ) , f_row( S.rows() + 1 , P( 0 ) )
{
 ADS_PRE( fits< I >( f_n ) );
 for( std::size_t i = 0 ; i < f_m ; ++i ) {
  for( const auto [ j , v ] : S.rowEntries( i ) ) {
   f_col.push_back( I( j ) );
   f_val.push_back( T( v ) );
   }
  ADS_PRE( fits< P >( f_col.size() + 1 ) );
  f_row[ i + 1 ] = P( f_col.size() );
  }
 ADS_AUDIT( sorted() );
 }

template< class T , std::integral I , std::integral P >
T BasicCsrMatrix< T , I , P >::get( std::size_t i , std::size_t j ) const
{
 ADS_PRE( ( i < f_m ) && ( j < f_n ) );
 const auto b = f_col.begin() + std::ptrdiff_t( f_row[ i ] ) ,
	    e = f_col.begin() + std::ptrdiff_t( f_row[ i + 1 ] );
 const auto p = std::lower_bound( b , e , I( j ) );
 return( ( p != e ) && ( std::size_t( *p ) == j )
	    ? f_val[ std::size_t( p - f_col.begin() ) ]
	    : T( 0 ) );
 }

template< class T , std::integral I , std::integral P >
void BasicCsrMatrix< T , I , P >::multiply( const T * x , T * y ) const
{
 const I * col = f_col.data();
 const T * val = f_val.data();
 const P * end = f_row.data() + 1;
 for( std::size_t i = 0 , p = 0 ; i < f_m ; ++i ) {  // one pass on the
  T s = T( 0 );  // nonzeros, p running through
  for( const auto e = std::size_t( end[ i ] ) ; p < e ; ++p )  // the rows
   s = detail::mulAdd( val[ p ] , x[ col[ p ] ] , s );
  y[ i ] = s;
  }
 }

template< class T , std::integral I , std::integral P >
std::vector< T >
BasicCsrMatrix< T , I , P >::multiply( const std::vector< T > & x ) const
{
 ADS_PRE( x.size() == f_n );
 std::vector< T > y( f_m );
 multiply( x.data() , y.data() );
 return( y );
 }

namespace detail
{

/// the scatter of the nonzeros of A, rows m , offsets row , columns col ,
/// values val , into the transpose of offsets trow , by cursors of the
/// type C; through 256 buckets of columns if bucketed
template< class C , class T , class I , class P >
void csrTransposeScatter( std::size_t m , std::size_t n , std::size_t nnz ,
			  const P * row , const I * col , const T * val ,
			  const P * trow , I * tcol , T * tval ,
			  bool bucketed )
{
 std::vector< C > at( trow , trow + n );
 if( ! bucketed ) {
  for( std::size_t i = 0 ; i < m ; ++i )            // the rows in order: each
   for( std::size_t p = std::size_t( row[ i ] ) ;  // column of the
	p < std::size_t( row[ i + 1 ] ) ; ++p ) {  // transpose sorted
    const C q = at[ std::size_t( col[ p ] ) ]++;
    tcol[ q ] = I( i );
    tval[ q ] = val[ p ];
    }
  return;
  }
 const std::size_t w = ( n + 255 ) / 256 , nb = ( n + w - 1 ) / w;
 std::vector< C > put( nb );
 for( std::size_t b = 0 ; b < nb ; ++b )
  put[ b ] = C( trow[ b * w ] );
 struct Item
 {
  I row , col;
  T val;
  };
 std::vector< Item , DefaultInitAllocator< Item > > tmp( nnz );
 for( std::size_t i = 0 ; i < m ; ++i )
  for( std::size_t p = std::size_t( row[ i ] ) ;
       p < std::size_t( row[ i + 1 ] ) ; ++p )
   tmp[ put[ std::size_t( col[ p ] ) / w ]++ ] =
     Item{ I( i ) , col[ p ] , val[ p ] };
 for( const Item & e : tmp ) {  // by buckets, in order
  const C q = at[ std::size_t( e.col ) ]++;
  tcol[ q ] = e.row;
  tval[ q ] = e.val;
  }
 }

}  // end( namespace detail )

template< class T , std::integral I , std::integral P >
BasicCsrMatrix< T , I , P > BasicCsrMatrix< T , I ,
					    P >::transpose( void ) const
{
 ADS_PRE( fits< I >( f_m ) );
 const std::size_t nnz = f_col.size();
 BasicCsrMatrix R( f_n , f_m );
 for( const I j : f_col )
  ++R.f_row[ std::size_t( j ) + 1 ];
 for( std::size_t j = 0 ; j < f_n ; ++j )
  R.f_row[ j + 1 ] += R.f_row[ j ];
 R.f_col.resize( nnz );
 R.f_val.resize( nnz );
 // the buckets pay where the writes would scatter beyond the cache: the
 // columns that blocks of 64 rows span, in the median of up to 64 blocks,
 // hold more than 256 KB of the transpose
 bool bucketed = false;
 if( ( nnz >= ( std::size_t( 1 ) << 18 ) ) && ( f_n >= 1024 ) ) {
  std::vector< std::size_t > span;
  const std::size_t step = std::max< std::size_t >( 64 , f_m / 64 );
  for( std::size_t i0 = 0 ; i0 + 64 <= f_m ; i0 += step ) {
   std::size_t lo = std::numeric_limits< std::size_t >::max() , hi = 0;
   for( std::size_t p = std::size_t( f_row[ i0 ] ) ;
	p < std::size_t( f_row[ i0 + 64 ] ) ; ++p ) {
    lo = std::min( lo , std::size_t( f_col[ p ] ) );
    hi = std::max( hi , std::size_t( f_col[ p ] ) );
    }
   span.push_back( lo <= hi ? hi - lo : 0 );
   }
  if( ! span.empty() ) {
   std::nth_element( span.begin() ,
		     span.begin() + std::ptrdiff_t( span.size() / 2 ) ,
		     span.end() );
   bucketed =
     double( span[ span.size() / 2 ] ) / double( f_n ) * double( nnz ) * 12 >
     double( 1 << 18 );
   }
  }
 if( nnz <= UINT32_MAX )
  detail::csrTransposeScatter< std::uint32_t >(
    f_m , f_n , nnz , f_row.data() , f_col.data() , f_val.data() ,
    R.f_row.data() ,
    R.f_col.data() , R.f_val.data() , bucketed );
 else
  detail::csrTransposeScatter< std::size_t >(
    f_m , f_n , nnz , f_row.data() , f_col.data() , f_val.data() ,
    R.f_row.data() ,
    R.f_col.data() , R.f_val.data() , bucketed );
 ADS_AUDIT( R.sorted() );
 return( R );
 }

template< class T , std::integral I , std::integral P >
bool BasicCsrMatrix< T , I , P >::sorted( void ) const
{
 if( ( f_row.size() != f_m + 1 ) || ( f_row[ 0 ] != 0 ) ||
     ( std::size_t( f_row[ f_m ] ) != f_col.size() ) ||
     ( f_col.size() != f_val.size() ) )
  return( false );
 for( std::size_t i = 0 ; i < f_m ; ++i ) {
  if( f_row[ i ] > f_row[ i + 1 ] )
   return( false );
  for( std::size_t p = std::size_t( f_row[ i ] ) ;
       p < std::size_t( f_row[ i + 1 ] ) ; ++p )
   if( std::cmp_less( f_col[ p ] , 0 ) ||
       std::cmp_greater_equal( f_col[ p ] , f_n ) ||
       ( ( p > std::size_t( f_row[ i ] ) ) &&
	 ( f_col[ p - 1 ] >= f_col[ p ] ) ) )
    return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

template< class T , std::integral I , std::integral P >
template< class A >
inline void BasicCsrMatrix< T , I , P >::persist( A & a )
{
 a( f_m , f_n , f_row , f_col , f_val );
 a.require( ( f_row.size() == f_m + 1 ) && ( f_row.front() == 0 ) &&
	      ( std::size_t( f_row.back() ) == f_col.size() ) &&
	      ( f_val.size() == f_col.size() ) && fits< I >( f_n ) &&
	      fits< P >( f_col.size() + 1 ) ,
	    "the rows of a CsrMatrix" );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File CsrMatrix.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
