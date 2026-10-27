/*--------------------------------------------------------------------------*/
/*-------------------------- File Smoothsort.tpp ---------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The sort of Smoothsort.h, with its contracts.
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

/// the Leonardo numbers that fit in a std::size_t
inline const std::vector< std::size_t > & leonardo( void )
{
 static const std::vector< std::size_t > L = [] {
  std::vector< std::size_t > v = { 1 , 1 };
  while( v.back() <= std::size_t( -1 ) - v[ v.size() - 2 ] - 1 &&
         v.size() < 90 )
   v.push_back( v.back() + v[ v.size() - 2 ] + 1 );
  return( v );
  }();
 return( L );
 }

/// moves m[ head ] down the Leonardo heap of order pshift rooted there
template< class T , class C >
void smoothSift( std::vector< T > & m , std::size_t pshift ,
                 std::size_t head , C & less )
{
 const auto & L = leonardo();
 T val = std::move( m[ head ] );
 while( pshift > 1 ) {
  const std::size_t rt = head - 1 , lf = head - 1 - L[ pshift - 2 ];
  if( ! less( val , m[ lf ] ) && ! less( val , m[ rt ] ) )
   break;
  if( ! less( m[ lf ] , m[ rt ] ) ) {       // the left child is larger
   m[ head ] = std::move( m[ lf ] );
   head = lf;
   pshift -= 1;
   }
  else {
   m[ head ] = std::move( m[ rt ] );
   head = rt;
   pshift -= 2;
   }
  }
 m[ head ] = std::move( val );
 }

/// moves m[ head ] left along the roots of the forest p, then down its heap
template< class T , class C >
void smoothTrinkle( std::vector< T > & m , std::uint64_t p ,
                    std::size_t pshift , std::size_t head , bool trusty ,
                    C & less )
{
 const auto & L = leonardo();
 T val = m[ head ];
 while( p != 1 ) {
  const std::size_t stepson = head - L[ pshift ];
  if( ! less( val , m[ stepson ] ) )
   break;
  if( ! trusty && ( pshift > 1 ) ) {
   const std::size_t rt = head - 1 , lf = head - 1 - L[ pshift - 2 ];
   if( ! less( m[ rt ] , m[ stepson ] ) || ! less( m[ lf ] , m[ stepson ] ) )
    break;
   }
  m[ head ] = std::move( m[ stepson ] );
  head = stepson;
  const int trail = std::countr_zero( p & ~std::uint64_t( 1 ) );
  p >>= trail;
  pshift += std::size_t( trail );
  trusty = false;
  }
 if( ! trusty ) {
  m[ head ] = std::move( val );
  smoothSift( m , pshift , head , less );
  }
 }

}  // end( namespace detail )

template< class T , class C >
void smoothsort( std::vector< T > & A , C less )
{
 // the forest is a word of 64 bits, the orders from pshift up: no heap of
 // order 64, nor larger
 ADS_PRE( ( detail::leonardo().size() <= 64 ) ||
          ( A.size() < detail::leonardo()[ 64 ] ) );
 const auto before = detail::auditCopy( A );
 const std::size_t size = A.size();
 if( size > 1 ) {
  const auto & L = detail::leonardo();
  std::uint64_t p = 1;
  std::size_t pshift = 1 , head = 0;
  while( head < size - 1 ) {                // the forest grows
   if( ( p & 3 ) == 3 ) {                   // merge the last two heaps
    detail::smoothSift( A , pshift , head , less );
    p >>= 2;
    pshift += 2;
    }
   else {
    if( L[ pshift - 1 ] >= size - 1 - head )
     detail::smoothTrinkle( A , p , pshift , head , false , less );
    else
     detail::smoothSift( A , pshift , head , less );
    if( pshift == 1 ) {
     p <<= 1;
     --pshift;
     }
    else {
     p <<= ( pshift - 1 );
     pshift = 1;
     }
    }
   p |= 1;
   ++head;
   }
  detail::smoothTrinkle( A , p , pshift , head , false , less );
  while( ( pshift != 1 ) || ( p != 1 ) ) {  // the forest shrinks
   if( pshift <= 1 ) {
    const int trail = std::countr_zero( p & ~std::uint64_t( 1 ) );
    p >>= trail;
    pshift += std::size_t( trail );
    }
   else {
    p <<= 2;
    p ^= 7;
    pshift -= 2;
    detail::smoothTrinkle( A , p >> 1 , pshift + 1 ,
                           head - L[ pshift ] - 1 , true , less );
    detail::smoothTrinkle( A , p , pshift , head - 1 , true , less );
    }
   --head;
   }
  }
 ADS_AUDIT( detail::sortedPermutation( A , before , less ) );
 }

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*------------------------ End File Smoothsort.tpp -------------------------*/
/*--------------------------------------------------------------------------*/
