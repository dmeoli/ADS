/*--------------------------------------------------------------------------*/
/*------------------------ File ParallelSketch.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The parallel construction of the sketches [see ParallelSketch.h].
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

template< Executor E , std::random_access_iterator I , class S >
 requires MergeableSketch< S , I >
S parallelSketch( E & ex , I first , I last , const S & s , std::size_t grain )
{
 const std::size_t n = std::size_t( last - first );
 return( parallelReduce(
   ex , 0 , n , s ,
   [ & ]( std::size_t lo , std::size_t hi ) {
    S b( s );
    if constexpr( requires { b.reseed( std::uint64_t( lo ) ); } )
     b.reseed( std::uint64_t( lo ) );
    for( std::size_t i = lo ; i < hi ; ++i )
     b.insert( first[ std::iter_difference_t< I >( i ) ] );
    return( b );
    } ,
   []( S a , const S & b ) {
    a.merge( b );
    return( a );
    } ,
   grain ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*---------------------- End File ParallelSketch.tpp -----------------------*/
/*--------------------------------------------------------------------------*/
