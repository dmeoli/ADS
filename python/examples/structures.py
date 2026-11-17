"""The structures of ADS from Python: a graph read in place by NumPy,
stored and opened again in O(1) time, its parallel algorithms, a bitmap
read by pyroaring, and the events of a graph that changes [see section
C.21 of the manual]."""

import os
import tempfile

import numpy as np

import ads

# a graph of the rows of NumPy: one copy, and its arrays are views
offsets = np.array([0, 2, 3, 4, 4], dtype=np.int32)
heads = np.array([1, 2, 3, 3], dtype=np.int32)
weights = np.array([1.0, 4.0, 2.0, 1.0])
g = ads.static_graph(offsets, heads, weights, layout="interleaved")
print(g, g.heads.strides, g.weights.tolist())

# stored once, opened in place by every later run
path = os.path.join(tempfile.mkdtemp(), "g.ads")
g.store(path)
h = ads.InterleavedStaticGraph.restore_mapped(path)
d, parent = ads.shortest_paths(h, 0)
print(d.tolist(), parent.tolist())
print(ads.delta_stepping(h, 0, 2.0).tolist(), ads.bfs(h, 0).tolist())

# a bitmap in the portable format of Roaring
r = ads.RoaringBitmap(np.arange(0, 1 << 20, 3, dtype=np.uint32))
data = r.serialize()
print(len(r), len(data), ads.RoaringBitmap.deserialize(data) == r)

# the events of the changes, a batch coalesced into its net effect
dg = ads.DynamicGraph(3)
with dg.batch():
    dg.add_edge(0, 1, 5.0)
    dg.set_weight(0, 1, 2.0)
    dg.add_edge(1, 2, 1.0)
    dg.remove_edge(1, 2)
print(dg.events())
