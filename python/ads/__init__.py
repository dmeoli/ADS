"""The structures and the algorithms of ADS that NumPy, SciPy and NetworkX
lack, on nanobind.

The graphs in compressed rows (StaticGraph, WeightedStaticGraph and
InterleavedStaticGraph) with their sequential and parallel algorithms,
CsrMatrix, the succinct structures (BitVector, RankSelect, PackedArray*,
EliasFano), RoaringBitmap in the portable format of Roaring, the filters,
the indexed heaps, and DynamicGraph, which tells its changes as the typed
events of ADS. Every static structure has store(), restore() and
restore_mapped(), which opens a file in O(1) time, and is pickled by the
same format. The arrays of a structure are read-only views of NumPy over
its memory, and the results are arrays that own their data.
"""

from ._core import (  # noqa: F401
    BinaryFuseFilter8,
    BinaryFuseFilter16,
    BitVector,
    BlockedBloomFilter,
    BloomFilter,
    ContractViolation,
    CsrMatrix,
    CuckooFilter,
    DynamicGraph,
    EliasFano,
    GrafiteFilter,
    IndexedBinaryHeap,
    IndexedHeap,
    IndexedHeap8,
    IndexedHeapPair,
    InterleavedStaticGraph,
    NegativeCycle,
    PackedArray1,
    PackedArray2,
    PackedArray4,
    PackedArray8,
    PackedArray16,
    PackedArray32,
    PersistError,
    PrefixFilter,
    RankSelect,
    RibbonFilter,
    RoaringBitmap,
    StaticGraph,
    ThreadPool,
    WeightedStaticGraph,
    bfs,
    bfs_tree,
    connected_components,
    delta_stepping,
    has_cycle,
    maximal_independent_set,
    rho_stepping,
    shortest_paths,
    strongly_connected_components,
    topological_sort,
)

try:
    from importlib.metadata import PackageNotFoundError, version
    __version__ = version("ads-python")
except PackageNotFoundError:  # a build tree, not installed
    __version__ = "0"

_PACKED = {
    1: PackedArray1,
    2: PackedArray2,
    4: PackedArray4,
    8: PackedArray8,
    16: PackedArray16,
    32: PackedArray32,
}


def packed_array(values, bits):
    """The values in a PackedArray of fields of bits bits (1, 2, 4, 8, 16
    or 32)."""
    if bits not in _PACKED:
        raise ValueError("bits must be 1, 2, 4, 8, 16 or 32")
    return _PACKED[bits].from_values(values)


def static_graph(offsets, heads, weights=None, *, directed=True,
                 layout="separate"):
    """The graph of the compressed rows: a StaticGraph if weights is None,
    else a WeightedStaticGraph (layout "separate") or an
    InterleavedStaticGraph (layout "interleaved")."""
    if weights is None:
        return StaticGraph.from_csr(offsets, heads, directed)
    if layout == "separate":
        return WeightedStaticGraph.from_csr(offsets, heads, weights, directed)
    if layout == "interleaved":
        return InterleavedStaticGraph.from_csr(offsets, heads, weights,
                                               directed)
    raise ValueError("layout must be 'separate' or 'interleaved'")


def from_scipy(a, *, directed=True, layout="separate", weighted=True):
    """The graph of the square sparse matrix a of SciPy (its nonzeros the
    arcs, their values the weights unless not weighted), or, for a matrix
    that is not square, the CsrMatrix of a."""
    import numpy as np

    a = a.tocsr()
    a.sort_indices()
    if a.shape[0] != a.shape[1]:
        return CsrMatrix.from_arrays(
            a.shape[0], a.shape[1], a.indptr.astype(np.uint64),
            a.indices.astype(np.uint32), a.data.astype(np.float64))
    w = a.data.astype(np.float64) if weighted else None
    return static_graph(a.indptr.astype(np.int32),
                        a.indices.astype(np.int32), w, directed=directed,
                        layout=layout)


def to_scipy(g):
    """The csr_array of SciPy of a graph (the weights as values, or 1s) or
    of a CsrMatrix, whose arrays are copied."""
    import numpy as np
    from scipy.sparse import csr_array

    if isinstance(g, CsrMatrix):
        return csr_array((np.array(g.data), np.array(g.indices),
                          np.array(g.indptr)), shape=g.shape)
    w = getattr(g, "weights", None)
    data = np.ones(g.arcs) if w is None else np.array(w)
    return csr_array((data, np.array(g.heads), np.array(g.offsets)),
                     shape=(g.n, g.n))
