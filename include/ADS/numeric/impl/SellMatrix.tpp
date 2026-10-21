/*--------------------------------------------------------------------------*/
/*--------------------------- File SellMatrix.tpp --------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The operations of SellMatrix [see SellMatrix.h], with their contracts.
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

template< std::size_t C >
SellMatrix< C >::SellMatrix( const CsrMatrix & A , std::size_t sigma )
 : f_m( A.rows() ) , f_n( A.cols() ) , f_nnz( A.nonzeros() )
{
 ADS_PRE( ( sigma >= 1 ) && ( f_m < ( std::size_t( 1 ) << 32 ) ) );
 const std::size_t * row = A.rowStart();
 const std::size_t m = f_m , chunks = ( m + C - 1 ) / C;
 auto len = [ & ]( std::size_t i ) { return( row[ i + 1 ] - row[ i ] ); };
 // the slots: the rows sorted by decreasing length within each window,
 // padded to whole chunks by the empty row m
 f_perm.resize( chunks * C );
 std::iota( f_perm.begin() , f_perm.begin() + std::ptrdiff_t( m ) ,
            std::uint32_t( 0 ) );
 std::fill( f_perm.begin() + std::ptrdiff_t( m ) , f_perm.end() ,
            std::uint32_t( m ) );
 for( std::size_t w = 0 ; w < m ; w += sigma )
  std::stable_sort( f_perm.begin() + std::ptrdiff_t( w ) ,
                    f_perm.begin() + std::ptrdiff_t( std::min( m ,
                                                               w + sigma ) ) ,
                    [ & ]( std::uint32_t a , std::uint32_t b ) {
                     return( len( a ) > len( b ) ); } );
 auto lenOf = [ & ]( std::uint32_t r ) {
  return( r < m ? len( r ) : std::size_t( 0 ) ); };
 f_start.assign( chunks + 1 , 0 );
 for( std::size_t c = 0 ; c < chunks ; ++c ) {
  std::size_t width = 0;
  for( std::size_t l = 0 ; l < C ; ++l )
   width = std::max( width , lenOf( f_perm[ c * C + l ] ) );
  f_start[ c + 1 ] = f_start[ c ] + width * C;
  }
 f_col.assign( f_start[ chunks ] , 0 );
 f_val.assign( f_start[ chunks ] , 0.0 );
 const std::uint32_t * col = A.columns();
 const double * val = A.values();
 for( std::size_t c = 0 ; c < chunks ; ++c )
  for( std::size_t l = 0 ; l < C ; ++l ) {  // the k-th nonzero of the row
   const std::uint32_t r = f_perm[ c * C + l ];   // of lane l at k C + l
   if( r >= m )
    continue;
   std::size_t p = f_start[ c ] + l;
   for( std::size_t q = row[ r ] ; q < row[ r + 1 ] ; ++q , p += C ) {
    f_col[ p ] = col[ q ];
    f_val[ p ] = val[ q ];
    }
   for( ; p < f_start[ c + 1 ] ; p += C )   // the padding reads a column
    f_col[ p ] = row[ r ] < row[ r + 1 ] ? col[ row[ r + 1 ] - 1 ] : 0;
   }                                        // of its row, in the cache
 }

template< std::size_t C >
void SellMatrix< C >::multiply( const double * x , double * y ) const
{
 multiply( x , y , 0 , f_start.size() - 1 );
 }

template< std::size_t C >
void SellMatrix< C >::multiply( const double * x , double * y ,
                                std::size_t first , std::size_t last ) const
{
 ADS_PRE( ( first <= last ) && ( last < f_start.size() ) );
 const std::size_t m = f_m;
 const std::uint32_t * col = f_col.data();
 const double * val = f_val.data();
 const std::uint32_t * perm = f_perm.data();
 for( std::size_t c = first ; c < last ; ++c ) {
  double s[ C ] = {};
  for( std::size_t p = f_start[ c ] , e = f_start[ c + 1 ] ; p < e ;
       p += C )
#if defined( __GNUC__ ) && ! defined( __clang__ )
   #pragma GCC ivdep
#endif
   for( std::size_t l = 0 ; l < C ; ++l )
    s[ l ] += val[ p + l ] * x[ col[ p + l ] ];
  for( std::size_t l = 0 ; l < C ; ++l )
   if( perm[ c * C + l ] < m ) y[ perm[ c * C + l ] ] = s[ l ];
  }
 }

template< std::size_t C >
std::vector< double > SellMatrix< C >::multiply(
                                      const std::vector< double > & x ) const
{
 ADS_PRE( x.size() == f_n );
 std::vector< double > y( f_m );
 multiply( x.data() , y.data() );
 return( y );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------- End File SellMatrix.tpp ------------------------*/
/*--------------------------------------------------------------------------*/
