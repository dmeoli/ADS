#!/usr/bin/env python3
###############################################################################
#                          bench/python/compare.py                            #
#                                                                             #
# The times of the package ads against NetworkX, SciPy, NumPy and pyroaring   #
# on the same inputs, for a campaign on an idle machine: the shortest paths   #
# (ADS by its heap, delta-stepping and rho-stepping, SciPy by csgraph,        #
# NetworkX), the visit in breadth and the components (sequential and          #
# parallel), the strong components, the conversion of the backend nx-ads and  #
# its calls through NetworkX, the rank against a prefix sum of NumPy, and the #
# bitmaps against pyroaring. Each time is the median of the repetitions,      #
# after one run that is not timed; the load of the machine is printed first,  #
# and with --strict the script stops if the load average of the last minute   #
# exceeds 1, since times taken on a busy machine are not comparable.          #
#                                                                             #
#   python3 bench/python/compare.py [ -n 200000 ] [ -m 2000000 ]              #
#                                   [ --repeat 5 ] [ --threads 0 ]            #
#                                   [ --strict ]                              #
###############################################################################

import argparse
import os
import statistics
import sys
import time

import numpy as np

import ads


def timed(f, repeat):
    """The median time of f over repeat runs, after one more."""
    f()
    t = []
    for _ in range(repeat):
        s = time.perf_counter()
        f()
        t.append(time.perf_counter() - s)
    return statistics.median(t)


def graph(n, m, seed):
    """m arcs with no loops on n nodes, uniform, and their weights."""
    rng = np.random.default_rng(seed)
    t = rng.integers(0, n, m, dtype=np.int32)
    h = rng.integers(0, n, m, dtype=np.int32)
    keep = t != h
    w = rng.integers(1, 100, m).astype(np.float64)
    return t[keep], h[keep], w[keep]


def main():
    p = argparse.ArgumentParser()
    p.add_argument("-n", type=int, default=200000)
    p.add_argument("-m", type=int, default=2000000)
    p.add_argument("--repeat", type=int, default=5)
    p.add_argument("--threads", type=int, default=0)
    p.add_argument("--strict", action="store_true")
    a = p.parse_args()
    load = os.getloadavg()[0]
    print(f"load average {load:.2f}, {os.cpu_count()} hardware threads")
    if a.strict and load > 1:
        sys.exit("the machine is busy: no times taken")

    import networkx as nx
    from scipy.sparse import csgraph

    nx.config.warnings_to_ignore.add("cache")

    pool = ads.ThreadPool(a.threads)
    t, h, w = graph(a.n, a.m, 1)
    g = ads.WeightedStaticGraph.from_edges(a.n, t, h, w, True)
    gi = ads.InterleavedStaticGraph.from_edges(a.n, t, h, w, True)
    u = ads.StaticGraph.from_edges(a.n, t, h, False)
    gt = g.transpose()
    S = ads.to_scipy(g)
    U = ads.to_scipy(u)
    G = nx.DiGraph()
    G.add_nodes_from(range(a.n))
    G.add_weighted_edges_from(zip(t.tolist(), h.tolist(), w.tolist()))
    H = G.to_undirected()
    r = a.repeat
    rows = []

    def row(what, who, f):
        rows.append((what, who, timed(f, r)))

    # the shortest paths
    row("shortest paths", "ads heap", lambda: ads.shortest_paths(g, 0))
    row("shortest paths", "ads heap, interleaved",
        lambda: ads.shortest_paths(gi, 0))
    row("shortest paths", "ads delta-stepping",
        lambda: ads.delta_stepping(gi, 0, 50.0, pool))
    row("shortest paths", "ads rho-stepping",
        lambda: ads.rho_stepping(gi, 0, pool=pool))
    row("shortest paths", "scipy csgraph",
        lambda: csgraph.dijkstra(S, indices=0))
    row("shortest paths", "networkx",
        lambda: nx.single_source_dijkstra_path_length(G, 0))
    row("shortest paths", "networkx, backend ads",
        lambda: nx.single_source_dijkstra_path_length(G, 0, backend="ads"))
    # the visit in breadth
    row("bfs", "ads", lambda: ads.bfs(g, 0))
    row("bfs", "ads parallel",
        lambda: ads.bfs(g, 0, parallel=True, transpose=gt, pool=pool))
    row("bfs", "scipy csgraph",
        lambda: csgraph.breadth_first_order(S, 0, return_predecessors=False))
    row("bfs", "networkx", lambda: nx.single_source_shortest_path_length(G, 0))
    # the components
    row("components", "ads", lambda: ads.connected_components(u))
    row("components", "ads afforest",
        lambda: ads.connected_components(u, parallel=True, pool=pool))
    row("components", "scipy csgraph",
        lambda: csgraph.connected_components(U, directed=False))
    row("components", "networkx",
        lambda: list(nx.connected_components(H)))
    row("strong components", "ads pearce",
        lambda: ads.strongly_connected_components(g))
    row("strong components", "scipy csgraph",
        lambda: csgraph.connected_components(S, connection="strong"))
    row("strong components", "networkx",
        lambda: list(nx.strongly_connected_components(G)))
    # the conversion of the backend, paid once per graph and attribute
    import nx_ads
    row("conversion", "nx_ads.from_networkx",
        lambda: nx_ads.from_networkx(G, "weight"))
    # rank and select
    bits = np.random.default_rng(2).random(64 * a.n) < 0.5
    rs = ads.RankSelect(ads.BitVector.from_bools(bits))
    q = np.random.default_rng(3).integers(0, len(bits), 10 ** 6,
                                          dtype=np.uint64)
    cum = np.concatenate([[0], np.cumsum(bits)])
    row("rank, 10^6", "ads", lambda: rs.rank1(q))
    row("rank, 10^6", "numpy cumsum (built)", lambda: cum[q])
    # the bitmaps
    v = np.unique(np.random.default_rng(4).integers(
        0, 1 << 28, a.m, dtype=np.uint64)).astype(np.uint32)
    row("roaring build", "ads", lambda: ads.RoaringBitmap(v))
    rb = ads.RoaringBitmap(v)
    row("roaring serialize", "ads", lambda: rb.serialize())
    try:
        import pyroaring
        row("roaring build", "pyroaring", lambda: pyroaring.BitMap(v))
        pb = pyroaring.BitMap(v)
        row("roaring serialize", "pyroaring", lambda: pb.serialize())
    except ImportError:
        pass

    print(f"\nn = {a.n}, m = {len(t)}, {pool.threads} threads, "
          f"median of {r}\n")
    print("| task | code | seconds |")
    print("|---|---|---:|")
    for what, who, s in rows:
        print(f"| {what} | {who} | {s:.4f} |")


if __name__ == "__main__":
    main()
