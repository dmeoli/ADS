/*--------------------------------------------------------------------------*/
/*--------------------------- File CsrMatrix.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of CsrMatrix [see CsrMatrix.h], with their contracts.
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

inline CsrMatrix::CsrMatrix( std::size_t m , std::size_t n )
 : f_m( m ) , f_n( n ) , f_row( m + 1 , std::size_t( 0 ) )
{
 }

inline CsrMatrix::CsrMatrix( std::size_t m , std::size_t n ,
                             std::vector< Triplet > T )
 : f_m( m ) , f_n( n ) , f_row( m + 1 , std::size_t( 0 ) )
{
 ADS_PRE( ( n <= std::size_t( UINT32_MAX ) + 1 ) &&
          std::all_of( T.begin() , T.end() , [ & ]( const Triplet & t ) {
           return( ( std::get< 0 >( t ) < m ) && ( std::get< 1 >( t ) < n ) );
           } ) );
 for( const auto & t : T ) ++f_row[ std::get< 0 >( t ) + 1 ];
 for( std::size_t i = 0 ; i < m ; ++i ) f_row[ i + 1 ] += f_row[ i ];
 std::vector< std::size_t > at( f_row.begin() , f_row.end() - 1 );
 std::vector< std::pair< std::uint32_t , double > > E( T.size() );
 for( const auto & [ i , j , v ] : T )     // by row, by a counting sort
  E[ at[ i ]++ ] = std::pair( std::uint32_t( j ) , v );
 f_col.reserve( T.size() );
 f_val.reserve( T.size() );
 std::size_t w = 0;
 for( std::size_t i = 0 ; i < m ; ++i ) {  // each row by column, the
  const auto b = E.begin() + std::ptrdiff_t( f_row[ i ] ) ,   // equal
             e = E.begin() + std::ptrdiff_t( f_row[ i + 1 ] );   // summed
  std::sort( b , e , []( const auto & x , const auto & y ) {
   return( x.first < y.first ); } );
  f_row[ i ] = w;
  for( auto p = b ; p != e ; ++p )
   if( ( f_col.size() > w ) && ( f_col.back() == p->first ) )
    f_val.back() += p->second;
   else {
    f_col.push_back( p->first );
    f_val.push_back( p->second );
    }
  w = f_col.size();
  }
 f_row[ m ] = w;
 ADS_AUDIT( sorted() );
 }

inline CsrMatrix::CsrMatrix( const SparseMatrix & S )
 : f_m( S.rows() ) , f_n( S.cols() ) ,
   f_row( S.rows() + 1 , std::size_t( 0 ) )
{
 for( std::size_t i = 0 ; i < f_m ; ++i ) {
  for( const auto [ j , v ] : S.rowEntries( i ) ) {
   f_col.push_back( std::uint32_t( j ) );
   f_val.push_back( v );
   }
  f_row[ i + 1 ] = f_col.size();
  }
 ADS_AUDIT( sorted() );
 }

inline double CsrMatrix::get( std::size_t i , std::size_t j ) const
{
 ADS_PRE( ( i < f_m ) && ( j < f_n ) );
 const auto b = f_col.begin() + std::ptrdiff_t( f_row[ i ] ) ,
            e = f_col.begin() + std::ptrdiff_t( f_row[ i + 1 ] );
 const auto p = std::lower_bound( b , e , std::uint32_t( j ) );
 return( ( p != e ) && ( *p == j ) ? f_val[ std::size_t( p -
                                                         f_col.begin() ) ]
                                   : 0.0 );
 }

inline void CsrMatrix::multiply( const double * x , double * y ) const
{
 const std::uint32_t * col = f_col.data();
 const double * val = f_val.data();
 const std::size_t * end = f_row.data() + 1;
 for( std::size_t i = 0 , p = 0 ; i < f_m ; ++i ) {   // one pass on the
  double s = 0;                             // nonzeros, p running through
  for( const std::size_t e = end[ i ] ; p < e ; ++p )   // the rows
   s += val[ p ] * x[ col[ p ] ];
  y[ i ] = s;
  }
 }

inline std::vector< double > CsrMatrix::multiply(
                                      const std::vector< double > & x ) const
{
 ADS_PRE( x.size() == f_n );
 std::vector< double > y( f_m );
 multiply( x.data() , y.data() );
 return( y );
 }

namespace detail {

/// the scatter of the nonzeros of A, rows m , offsets row , columns col ,
/// values val , into the transpose of offsets trow , by cursors of the
/// type I; through 256 buckets of columns if bucketed
template< class I >
void csrTransposeScatter( std::size_t m , std::size_t n , std::size_t nnz ,
                          const std::size_t * row , const std::uint32_t * col ,
                          const double * val , const std::size_t * trow ,
                          std::uint32_t * tcol , double * tval ,
                          bool bucketed )
{
 std::vector< I > at( trow , trow + n );
 if( ! bucketed ) {
  for( std::size_t i = 0 ; i < m ; ++i )    // the rows in order: each
   for( std::size_t p = row[ i ] ; p < row[ i + 1 ] ; ++p ) {   // column of
    const I q = at[ col[ p ] ]++;           // the transpose sorted
    tcol[ q ] = std::uint32_t( i );
    tval[ q ] = val[ p ];
    }
  return;
  }
 const std::size_t w = ( n + 255 ) / 256 , nb = ( n + w - 1 ) / w;
 std::vector< I > put( nb );
 for( std::size_t b = 0 ; b < nb ; ++b ) put[ b ] = I( trow[ b * w ] );
 struct Item {
  std::uint32_t row , col;
  double val;
  };
 std::vector< Item , DefaultInitAllocator< Item > > tmp( nnz );
 for( std::size_t i = 0 ; i < m ; ++i )
  for( std::size_t p = row[ i ] ; p < row[ i + 1 ] ; ++p )
   tmp[ put[ col[ p ] / w ]++ ] = Item{ std::uint32_t( i ) , col[ p ] ,
                                        val[ p ] };
 for( const Item & e : tmp ) {              // by buckets, in order
  const I q = at[ e.col ]++;
  tcol[ q ] = e.row;
  tval[ q ] = e.val;
  }
 }

}  // end( namespace detail )

inline CsrMatrix CsrMatrix::transpose( void ) const
{
 ADS_PRE( f_m <= std::size_t( UINT32_MAX ) + 1 );
 const std::size_t nnz = f_col.size();
 CsrMatrix T( f_n , f_m );
 for( const std::uint32_t j : f_col ) ++T.f_row[ j + 1 ];
 for( std::size_t j = 0 ; j < f_n ; ++j ) T.f_row[ j + 1 ] += T.f_row[ j ];
 T.f_col.resize( nnz );
 T.f_val.resize( nnz );
 // the buckets pay where the writes would scatter beyond the cache: the
 // columns that blocks of 64 rows span, in the median of up to 64 blocks,
 // hold more than 256 KB of the transpose
 bool bucketed = false;
 if( ( nnz >= ( std::size_t( 1 ) << 18 ) ) && ( f_n >= 1024 ) ) {
  std::vector< std::size_t > span;
  const std::size_t step = std::max< std::size_t >( 64 , f_m / 64 );
  for( std::size_t i0 = 0 ; i0 + 64 <= f_m ; i0 += step ) {
   std::uint32_t lo = UINT32_MAX , hi = 0;
   for( std::size_t p = f_row[ i0 ] ; p < f_row[ i0 + 64 ] ; ++p ) {
    lo = std::min( lo , f_col[ p ] );
    hi = std::max( hi , f_col[ p ] );
    }
   span.push_back( lo <= hi ? hi - lo : 0 );
   }
  if( ! span.empty() ) {
   std::nth_element( span.begin() , span.begin() + std::ptrdiff_t(
                      span.size() / 2 ) , span.end() );
   bucketed = double( span[ span.size() / 2 ] ) / double( f_n ) *
              double( nnz ) * 12 > double( 1 << 18 );
   }
  }
 if( nnz <= UINT32_MAX )
  detail::csrTransposeScatter< std::uint32_t >( f_m , f_n , nnz ,
                                                f_row.data() , f_col.data() ,
                                                f_val.data() , T.f_row.data() ,
                                                T.f_col.data() ,
                                                T.f_val.data() , bucketed );
 else
  detail::csrTransposeScatter< std::size_t >( f_m , f_n , nnz ,
                                              f_row.data() , f_col.data() ,
                                              f_val.data() , T.f_row.data() ,
                                              T.f_col.data() , T.f_val.data() ,
                                              bucketed );
 ADS_AUDIT( T.sorted() );
 return( T );
 }

inline bool CsrMatrix::sorted( void ) const
{
 if( ( f_row.size() != f_m + 1 ) || ( f_row[ 0 ] != 0 ) ||
     ( f_row[ f_m ] != f_col.size() ) || ( f_col.size() != f_val.size() ) )
  return( false );
 for( std::size_t i = 0 ; i < f_m ; ++i ) {
  if( f_row[ i ] > f_row[ i + 1 ] )
   return( false );
  for( std::size_t p = f_row[ i ] ; p < f_row[ i + 1 ] ; ++p )
   if( ( f_col[ p ] >= f_n ) ||
       ( ( p > f_row[ i ] ) && ( f_col[ p - 1 ] >= f_col[ p ] ) ) )
    return( false );
  }
 return( true );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File CsrMatrix.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
