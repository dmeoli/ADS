/*--------------------------------------------------------------------------*/
/*--------------------------- File MappedFile.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * A file mapped in memory, shared by the structures that are read from it
 * in place [see `Persistence.h`]: `MappedFile` maps a whole file privately
 * and writably (`mmap( PROT_READ | PROT_WRITE , MAP_PRIVATE )` on a POSIX
 * system), so that a page is read from the disk at its first access and a
 * page that a structure writes is copied for the process and never reaches
 * the file; elsewhere the file is read whole into a block aligned to a
 * line of the cache. The copies of a `MappedFile` share the mapping, which
 * is released with the last of them, as a `std::shared_ptr` does, but in a
 * single pointer, since the allocators of the words of the succinct
 * structures hold one [see `BitVector.h`].
 *
 * An allocator that holds a mapping takes the bytes that the reader of a
 * file offers it, `offer()`, at its next allocation of that size, `take()`,
 * instead of new memory, constructs nothing over them, and gives back
 * nothing that lies in the mapping, `holds()`: a vector is thus made over
 * the bytes of the file in \f$ O( 1 ) \f$ time, with no copy. The offer is
 * one at a time, made and taken by the thread that reads the file.
 * `MappableAllocator` is such an allocator, which is `std::allocator`
 * when it holds no mapping (the vectors of the filters and of the perfect
 * hash functions are on it), and `detail::LineAllocator` of `BitVector.h`
 * is another (the words of the succinct structures). A copy of a vector
 * on them is made on new memory (`select_on_container_copy_construction()`
 * holds no mapping), while a move and a swap take the mapping with the
 * memory, so that each block goes back to the allocator that gave it.
 *
 * An error of the system (a file that cannot be opened, read or mapped)
 * throws `PersistError`, a `std::runtime_error` whose message names the
 * file. The functions are defined in `MappedFile.tpp`.
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_MappedFile
 #define __ADS_MappedFile
/* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS
{

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS PersistError ---------------------------*/
/*--------------------------------------------------------------------------*/

/// the error of a file (or a stream, or a buffer) of a structure that
/// cannot be read: not a file of ADS, of another structure or version,
/// truncated, corrupted, or refused by the system
class PersistError : public std::runtime_error
{
 public:
 using std::runtime_error::runtime_error;
 };

/*--------------------------------------------------------------------------*/
/*---------------------------- CLASS MappedFile ----------------------------*/
/*--------------------------------------------------------------------------*/

/// a whole file mapped privately in memory, shared by its copies
class MappedFile
{

/*--------------------------------------------------------------------------*/
 public:
 /// no file
 MappedFile( void ) = default;

 /// the file at \f$ p \f$, mapped (or read, where it cannot be mapped)
 explicit MappedFile( const std::filesystem::path & p );

 /// the same mapping, shared
 MappedFile( const MappedFile & o ) noexcept;

 /// the mapping of \f$ o \f$, which is left with no file
 MappedFile( MappedFile && o ) noexcept;

 /// the same mapping as \f$ o \f$, shared
 MappedFile & operator=( const MappedFile & o ) noexcept;

 /// the mapping of \f$ o \f$, which is left with no file
 MappedFile & operator=( MappedFile && o ) noexcept;

 /// the mapping released if this is its last copy
 ~MappedFile( void );

 /// the bytes of the file, which start on a line of the cache (on a page
 /// where the file is mapped) and may be written by the process alone
 [[nodiscard]] std::span< std::byte > bytes( void ) const;

 /// whether there is a file
 [[nodiscard]] explicit operator bool( void ) const { return( f_r ); }

 /// whether the file is mapped by the system, rather than read
 [[nodiscard]] bool mapped( void ) const;

 /// whether \f$ p \f$ lies in the bytes of the file
 [[nodiscard]] bool holds( const void * p ) const;

 /// the bytes at \f$ p \f$, \f$ m \f$ of them, offered to the next
 /// `take()` of \f$ m \f$ bytes
 /// \pre there is a file, which holds \f$ p \f$
 void offer( void * p , std::size_t m ) const;

 /// the bytes offered, if \f$ m \f$ of them, taken; else null
 [[nodiscard]] void * take( std::size_t m ) const;

 /// whether the two are copies of the same mapping
 friend bool operator==( const MappedFile & a , const MappedFile & b ) {
  return( a.f_r == b.f_r );
  }

/*--------------------------------------------------------------------------*/
 private:
 /// the mapping, and the count of its copies
 struct Region
 {
  std::atomic< std::size_t > refs{ 1 };  ///< the copies
  std::byte * base = nullptr;            ///< the first byte
  std::size_t size = 0;                  ///< the bytes of the file
  bool system = false;                   ///< mapped, rather than read
  void * offered = nullptr;              ///< the bytes offered
  std::size_t offeredSize = 0;           ///< how many
  };

 /// the mapping released if this is its last copy
 void release( void ) noexcept;

 Region * f_r = nullptr;

 };  // end( class MappedFile )

namespace detail
{

/// a hash of the parameters of a class, for its `persistTag()`: the
/// numbers \f$ v \f$ combined in order
[[nodiscard]] constexpr std::uint64_t
persistMix( std::initializer_list< std::uint64_t > v )
{
 std::uint64_t h = 0x9E3779B97F4A7C15ULL;
 for( const std::uint64_t x : v ) {
  h ^= x + 0x9E3779B97F4A7C15ULL + ( h << 6 ) + ( h >> 2 );
  h *= 0xBF58476D1CE4E5B9ULL;
  }
 return( h );
 }

/// a hash of a name, for the `persistTag()` of a class whose parameter is
/// a persistent class: FNV-1a on 64 bits
[[nodiscard]] constexpr std::uint64_t persistHash( std::string_view s )
{
 std::uint64_t h = 0xCBF29CE484222325ULL;
 for( const char c : s )
  h = ( h ^ std::uint64_t( static_cast< unsigned char >( c ) ) ) *
      0x100000001B3ULL;
 return( h );
 }

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*------------------------ CLASS MappableAllocator -------------------------*/
/*--------------------------------------------------------------------------*/

/// `std::allocator`, which also makes a vector over the bytes of a mapped
/// file [see `restoreMapped()`]; an item constructed with no argument is
/// value-initialized if \f$ V \f$, else left uninitialized (the numbers
/// about to be written of a matrix)
template< class T , bool V = true >
class MappableAllocator
{
 public:
 /// the type of an item
 using value_type = T;
 /// a vector on it may be a view of a mapped file
 using adopts_mapping = void;
 /// the mapping goes with the memory
 using propagate_on_container_move_assignment = std::true_type;
 /// the mapping goes with the memory
 using propagate_on_container_swap = std::true_type;
 /// two allocators of different mappings differ
 using is_always_equal = std::false_type;
 /// the same allocator, of items of the type U
 template< class U >
 struct rebind
 {
  using other = MappableAllocator< U , V >;
  };

 /// an allocator of new memory only
 MappableAllocator( void ) = default;
 /// an allocator that takes the bytes that \f$ m \f$ offers
 explicit MappableAllocator( MappedFile m ) noexcept
     : f_map( std::move( m ) ) { }
 template< class U >
 MappableAllocator( const MappableAllocator< U , V > & o ) noexcept
     : f_map( o.f_map ) { }

 /// the allocator of a copy, of new memory only
 [[nodiscard]] MappableAllocator
 select_on_container_copy_construction( void ) const {
  return( MappableAllocator() );
  }

 [[nodiscard]] T * allocate( std::size_t n ) {
  if( f_map )
   if( void * p = f_map.take( n * sizeof( T ) ) )
    return( static_cast< T * >( p ) );
  return( std::allocator< T >().allocate( n ) );
  }

 void deallocate( T * p , std::size_t n ) noexcept {
  if( f_map && f_map.holds( p ) )
   return;
  std::allocator< T >().deallocate( p , n );
  }

 /// an item constructed with no argument over the mapping is left as the
 /// file has it (an item of a mapped array is trivially copyable, and
 /// there by the bytes of the file), any other one as \f$ V \f$ asks
 template< class U , class... A >
 void construct( U * p , A &&... a ) {
  if constexpr( ( sizeof...( A ) == 0 ) && std::is_trivially_copyable_v< U > )
   if( f_map && f_map.holds( p ) )
    return;
  if constexpr( ( sizeof...( A ) == 0 ) && ! V )
   ::new( static_cast< void * >( p ) ) U;
  else
   ::new( static_cast< void * >( p ) ) U( std::forward< A >( a )... );
  }

 /// whether the two hold the same mapping, or none
 friend bool operator==( const MappableAllocator & a ,
			 const MappableAllocator & b ) noexcept {
  return( a.f_map == b.f_map );
  }

 private:
 template< class U , bool W >
 friend class MappableAllocator;

 MappedFile f_map;  ///< the mapping, if any

 };  // end( class MappableAllocator )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/external/impl/MappedFile.tpp"

/*--------------------------------------------------------------------------*/

#endif /* MappedFile.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File MappedFile.h --------------------------*/
/*--------------------------------------------------------------------------*/
