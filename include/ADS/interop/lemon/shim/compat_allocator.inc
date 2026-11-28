// The allocator of the maps and of the paths of LEMON 1.3, whose
// construct() and destroy(), members of std::allocator removed by C++20,
// go through std::allocator_traits; written by ADS before the namespace
// lemon of lemon/bits/array_map.h and of lemon/path.h [see compat.txt].
#ifndef ADS_LEMON_COMPAT_ALLOCATOR
#define ADS_LEMON_COMPAT_ALLOCATOR
#include <memory>
#include <utility>
namespace ADS::lemon_interop {
template< typename T >
struct CompatAllocator : std::allocator< T > {
 template< typename U , typename... A >
 void construct( U * p , A &&... a ) {
  std::allocator_traits< std::allocator< T > >::construct(
   *this , p , std::forward< A >( a )... );
  }
 template< typename U >
 void destroy( U * p ) {
  std::allocator_traits< std::allocator< T > >::destroy( *this , p );
  }
 };
}
#endif
