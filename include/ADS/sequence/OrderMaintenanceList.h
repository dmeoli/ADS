/*--------------------------------------------------------------------------*/
/*----------------------- File OrderMaintenanceList.h ----------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The order maintenance of exercise 8.11 of Morin (his `Sequence`), beyond
 * the book: a list of elements with `addAfter( e , v )`, which adds a new
 * element right after \f$ e \f$ (at the front if \f$ e \f$ is nil), in
 * \f$ O( 1 ) \f$ amortized time, `remove( e )`, in \f$ O( 1 ) \f$ time, and
 * `testBefore( a , b )`, whether \f$ a \f$ comes before \f$ b \f$, in
 * \f$ O( 1 ) \f$ time.
 *
 * It is the structure of Bender, Cole, Demaine, Farach-Colton and Zito, a
 * list labeling on two levels (see \ref theory_sequence_morin):
 *
 * - the elements are cut into runs of consecutive elements, the buckets,
 *   of at most \f$ B = 64 \f$, and each element has a local label below
 *   \f$ 2^{63} \f$, which increases along its bucket; a new element takes
 *   the middle of the gap between its neighbors in the bucket, and the
 *   bucket is labeled again with equal gaps, in \f$ O( B ) \f$, when that
 *   gap is empty; a bucket of \f$ B + 1 \f$ elements splits into two
 *   halves;
 * - the buckets have labels below \f$ 2^{63} \f$ too, which increase along
 *   the list of the buckets; a new bucket takes the middle of the gap
 *   after the one it follows, and when the gap is empty the smallest range
 *   of \f$ 2^i \f$ labels aligned on \f$ 2^i \f$ around it whose buckets, the
 *   new one included, are at most \f$ ( 2 / T )^i \f$ is labeled again with
 *   equal gaps, \f$ T = 1.4 \f$ (their first algorithm), which writes
 *   \f$ O( \log m ) \f$ amortized labels for \f$ m \f$ buckets;
 * - `testBefore()` compares the labels of the buckets, and the local labels
 *   when the bucket is the same.
 *
 * A split happens once in \f$ B / 2 \f$ insertions into a bucket at least,
 * which pay its \f$ O( B + \log m ) \f$ labels, since
 * \f$ \log_2 m < 63 < B \f$; a removal unlinks its element, and its bucket
 * when it empties, with no relabeling. Hence an insertion costs
 * \f$ O( 1 ) \f$ amortized and a removal \f$ O( 1 ) \f$ in the worst case.
 * The labels of the buckets hold at most \f$ ( 2 / T )^{63} \f$, about
 * \f$ 5.8 \cdot 10^9 \f$, buckets. The scapegoat tree whose labels encode
 * the paths, the solution that the exercise has in mind, takes
 * \f$ O( \log n ) \f$ amortized (see the theory page). An element stays
 * valid until it is removed.
 *
 * The contracts [see `Contract.h`]: the elements given are elements of this
 * list, which holds fewer than \f$ 5.8 \cdot 10^9 \f$ buckets; the
 * invariant, checked on the buckets that an operation touches, is that the
 * labels increase, the buckets hold from 1 to \f$ B \f$ elements and the
 * elements point to their bucket. The operations are defined in
 * `OrderMaintenanceList.tpp`.
 *
 * \see M. A. Bender, R. Cole, E. D. Demaine, M. Farach-Colton, J. Zito, Two
 *      simplified algorithms for maintaining order in a list, Algorithms -
 *      ESA 2002, Lecture Notes in Computer Science 2461, 152-164, 2002,
 *      doi:10.1007/3-540-45749-6_17
 * \see P. F. Dietz, D. D. Sleator, Two algorithms for maintaining order in a
 *      list, Proceedings of the 19th ACM Symposium on Theory of Computing
 *      (STOC), 365-372, 1987, doi:10.1145/28395.28434
 * \see P. Morin, Open Data Structures (in C++), Athabasca University Press,
 *      2013, exercise 8.11, doi:10.15215/aupress/9781927356388.01
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

#ifndef __ADS_OrderMaintenanceList
 #define __ADS_OrderMaintenanceList
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

/*--------------------------------------------------------------------------*/
/*----------------------- CLASS OrderMaintenanceList -----------------------*/
/*--------------------------------------------------------------------------*/

/// a list of items of type `T` whose order between two elements is
/// tested in \f$ O( 1 ) \f$ by labels on two levels
template< class T >
class OrderMaintenanceList {

 struct Node;
 struct Bucket;

/*--------------------------------------------------------------------------*/
 public:

 using Item = T;    ///< the type of the items

 /// an element of the list, valid until it is removed
 using Element = const Node *;

 /// the most elements of a bucket
 static constexpr std::size_t B = 64;

 /// the empty list
 OrderMaintenanceList( void ) = default;

 /// no copy, since the elements are pointers
 OrderMaintenanceList( const OrderMaintenanceList & ) = delete;

 /// no copy, since the elements are pointers
 OrderMaintenanceList & operator=( const OrderMaintenanceList & ) = delete;

 /// frees the elements
 ~OrderMaintenanceList();

 /// the number \f$ n \f$ of elements
 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }

 /// a new element of \f$ v \f$ right after \f$ e \f$, at the front if
 /// \f$ e \f$ is nil, in \f$ O( 1 ) \f$ amortized time
 ///
 /// In pseudocode (Bender et al., beyond the book), \f$ \ell \f$ the local
 /// labels and \f$ b \f$ the bucket of \f$ e \f$ (the first bucket if
 /// \f$ e \f$ is nil, a new one if the list is empty):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{addAfter}( e , v ) \cr
 /// 2 & \quad u \gets \text{a node of } v \text{ linked after } e
 ///     \text{ in the list and in } b \cr
 /// 3 & \quad \mathit{lo} \gets \ell( \text{the element before } u
 ///     \text{ in } b ) \text{, or } 0 \cr
 /// 4 & \quad \mathit{hi} \gets \ell( \text{the element after } u
 ///     \text{ in } b ) \text{, or } 2^{63} \cr
 /// 5 & \quad \textbf{if}\ \mathit{hi} - \mathit{lo} \ge 2\ \textbf{then}
 ///     \ \ell( u ) \gets \mathit{lo} + \lfloor ( \mathit{hi} - \mathit{lo}
 ///     ) / 2 \rfloor \cr
 /// 6 & \quad \textbf{else}\ \text{label the elements of } b \text{ with }
 ///     \text{equal gaps} \cr
 /// 7 & \quad \textbf{if}\ |b| > B\ \textbf{then} \cr
 /// 8 & \qquad b' \gets \text{a new bucket of the last }
 ///     \lceil |b| / 2 \rceil \text{ elements of } b \cr
 /// 9 & \qquad \mathrm{addBucketAfter}( b , b' );\ \text{label } b
 ///     \text{ and } b' \text{ with equal gaps} \cr
 /// 10 & \quad \textbf{return}\ u
 /// \end{array}}
 /// \f]
 /// \pre \f$ e \f$ is nil or an element of this list
 Element addAfter( Element e , const T & v );

 /// \f$ e \f$ out of the list, and its bucket if it empties, in
 /// \f$ O( 1 ) \f$ time, no label changed
 /// \pre \f$ e \f$ is an element of this list
 void remove( Element e );

 /// whether \f$ a \f$ comes before \f$ b \f$, by the labels of their
 /// buckets and by their local labels, in \f$ O( 1 ) \f$ time
 /// \pre \f$ a \f$ and \f$ b \f$ are elements of this list
 [[nodiscard]] bool testBefore( Element a , Element b ) const {
  ADS_PRE( a && b && a->bucket && b->bucket );
  //! [testBefore]
  return( a->bucket == b->bucket ? a->label < b->label
                                 : a->bucket->label < b->bucket->label );
  //! [testBefore]
  }

 /// the item of \f$ e \f$
 [[nodiscard]] static const T & value( Element e ) { return( e->item ); }

 /// the first element, nil if the list is empty
 [[nodiscard]] Element first( void ) const { return( f_first ); }

 /// the element after \f$ e \f$, nil after the last
 /// \pre \f$ e \f$ is an element of this list
 [[nodiscard]] Element next( Element e ) const {
  ADS_PRE( e && e->bucket );
  return( e->next );
  }

 /// the number of the relabelings of a range of buckets so far, and the
 /// labels written by them and by the buckets labeled again
 [[nodiscard]] std::pair< std::size_t , std::size_t > relabelWork( void )
  const {
  return( std::pair< std::size_t , std::size_t >( f_relabels , f_work ) );
  }

/*--------------------------------------------------------------------------*/
 private:

 /// an element of the list
 struct Node {
  T item;                       ///< the item
  Node * prev = nullptr;        ///< the previous element in the list
  Node * next = nullptr;        ///< the next element in the list
  Bucket * bucket = nullptr;    ///< its bucket, nil once removed
  std::uint64_t label = 0;      ///< the local label
  };

 /// a bucket, a run of consecutive elements
 struct Bucket {
  Bucket * prev = nullptr;      ///< the previous bucket
  Bucket * next = nullptr;      ///< the next bucket
  Node * head = nullptr;        ///< its first element
  std::size_t size = 0;         ///< its elements
  std::uint64_t label = 0;      ///< the label
  };

 /// the labels, local or not, are below \f$ 2^{63} \f$
 static constexpr std::uint64_t kU = std::uint64_t( 1 ) << 63;

 /// the elements of \f$ b \f$ labeled with equal gaps, in \f$ O( B ) \f$
 void relabelBucket( Bucket * b );

 /// the bucket \f$ b \f$ of \f$ B + 1 \f$ elements split into two, the
 /// second one after it, in \f$ O( B ) \f$ time and the
 /// \f$ O( \log m ) \f$ amortized of `addBucketAfter()`
 void split( Bucket * b );

 /// the new bucket \f$ c \f$ linked after \f$ b \f$ and labeled, in
 /// \f$ O( \log m ) \f$ amortized time for \f$ m \f$ buckets
 ///
 /// In pseudocode (the first algorithm of Bender et al., beyond the book),
 /// \f$ L \f$ the labels of the buckets, \f$ T = 1.4 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{procedure}\ \mathrm{addBucketAfter}( b , c ) \cr
 /// 2 & \quad \mathit{hi} \gets L( b.\mathit{next} ) \text{, or } 2^{63}
 ///     \text{ if it is nil} \cr
 /// 3 & \quad \textbf{if}\ \mathit{hi} - L( b ) \ge 2\ \textbf{then}
 ///     \ \text{link } c \text{ after } b;\ L( c ) \gets L( b ) + \lfloor (
 ///     \mathit{hi} - L( b ) ) / 2 \rfloor;\ \textbf{return} \cr
 /// 4 & \quad \textbf{for}\ i \gets 1\ \textbf{to}\ 63\ \textbf{do} \cr
 /// 5 & \qquad \mathit{lo} \gets L( b ) \text{ with its } i \text{ lowest }
 ///     \text{bits cleared} \cr
 /// 6 & \qquad m \gets 1 + \text{the buckets with a label in }
 ///     [ \mathit{lo} , \mathit{lo} + 2^i ) \cr
 /// 7 & \qquad \textbf{if}\ m \le ( 2 / T )^i\ \textbf{then break} \cr
 /// 8 & \quad \text{link } c \text{ after } b \cr
 /// 9 & \quad \text{give the } m \text{ buckets of the range the labels }
 ///     \mathit{lo} + k \lfloor 2^i / m \rfloor \text{, } k = 0 , \ldots ,
 ///     m - 1
 /// \end{array}}
 /// \f]
 /// \pre the buckets are fewer than \f$ ( 2 / T )^{63} \f$
 void addBucketAfter( Bucket * b , Bucket * c );

 /// the labels increasing in \f$ b \f$ and around it, its size, the
 /// buckets of its elements, \f$ O( B ) \f$
 [[nodiscard]] bool valid( const Bucket * b ) const;

 /// the thresholds \f$ ( 2 / T )^i \f$ of the ranges of \f$ 2^i \f$ labels
 [[nodiscard]] static const std::array< double , 64 > & caps( void );

 Bucket f_head;                    ///< the sentinel bucket, of label 0
 Node * f_first = nullptr;         ///< the first element
 std::size_t f_n = 0;              ///< the elements
 std::size_t f_relabels = 0;       ///< the relabelings of a range
 std::size_t f_work = 0;           ///< the labels written

 };  // end( class OrderMaintenanceList )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/sequence/impl/OrderMaintenanceList.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* OrderMaintenanceList.h included */

/*--------------------------------------------------------------------------*/
/*--------------------- End File OrderMaintenanceList.h --------------------*/
/*--------------------------------------------------------------------------*/
