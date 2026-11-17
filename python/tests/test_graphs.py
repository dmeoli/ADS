"""The graphs of ADS from Python: built from the arrays of NumPy, read in
place, stored and opened again, and their algorithms against those of
NetworkX and SciPy."""

import pickle

import numpy as np
import pytest

import ads
from conftest import random_edges

nx = pytest.importorskip("networkx")

LAYOUTS = [ads.WeightedStaticGraph, ads.InterleavedStaticGraph]


def nx_graph(n, t, h, w, directed):
    G = nx.DiGraph() if directed else nx.Graph()
    G.add_nodes_from(range(n))
    for k in range(len(t)):
        G.add_edge(int(t[k]), int(h[k]), weight=float(w[k]))
    return G


@pytest.mark.parametrize("cls", LAYOUTS)
def test_views_without_copy(cls, edges):
    n, t, h, w, directed = edges
    g = cls.from_edges(n, t, h, w, directed)
    assert g.n == n and g.m == len(t)
    off, heads, weights = g.offsets, g.heads, g.weights
    assert not off.flags.writeable and not heads.flags.writeable
    # the views are over the memory of the graph, also when interleaved
    base = g.offsets.__array_interface__["data"][0]
    assert base == off.__array_interface__["data"][0]
    if cls is ads.InterleavedStaticGraph:
        assert heads.strides == (16,) and weights.strides == (16,)
    # the view keeps the graph alive
    del g
    assert off[-1] == len(heads) == len(weights)
    G = nx_graph(n, t, h, w, directed)
    for u in range(n):
        row = list(zip(heads[off[u]:off[u + 1]].tolist(),
                       weights[off[u]:off[u + 1]].tolist()))
        assert row == sorted((v, d["weight"]) for v, d in G[u].items())


@pytest.mark.parametrize("cls", LAYOUTS)
def test_from_csr_and_back(cls, edges):
    n, t, h, w, directed = edges
    g = cls.from_edges(n, t, h, w, directed)
    # the rows given unsorted are sorted in the copy
    off = np.array(g.offsets)
    perm = np.concatenate([np.arange(off[u], off[u + 1])[::-1]
                           for u in range(n)]).astype(np.int64)
    g2 = cls.from_csr(off, np.array(g.heads)[perm], np.array(g.weights)[perm],
                      directed)
    assert np.array_equal(g2.heads, g.heads)
    assert np.array_equal(g2.weights, g.weights)
    assert g2.m == g.m
    with pytest.raises(ValueError):
        cls.from_csr(np.array([0, 1], dtype=np.int32),
                     np.array([0], dtype=np.int32), np.array([1.0]))
    with pytest.raises(ValueError):  # an edge in one row only
        cls.from_csr(np.array([0, 1, 1], dtype=np.int32),
                     np.array([1], dtype=np.int32), np.array([1.0]), False)


def test_scipy_round_trip(edges):
    sp = pytest.importorskip("scipy.sparse")
    n, t, h, w, directed = edges
    g = ads.WeightedStaticGraph.from_edges(n, t, h, w, directed)
    a = ads.to_scipy(g)
    assert a.shape == (n, n) and a.nnz == g.arcs
    g2 = ads.from_scipy(a, directed=directed)
    assert np.array_equal(g2.heads, g.heads)
    m = sp.random(30, 40, density=0.2, format="csr", random_state=3)
    M = ads.from_scipy(m)
    x = np.arange(40, dtype=np.float64)
    assert np.allclose(M.multiply(x), m @ x)
    assert np.allclose(ads.to_scipy(M.transpose()).toarray(), m.T.toarray())


@pytest.mark.parametrize("cls", LAYOUTS + [ads.StaticGraph])
def test_persistence(cls, edges, tmp_path):
    n, t, h, w, directed = edges
    g = (cls.from_edges(n, t, h, directed) if cls is ads.StaticGraph else
         cls.from_edges(n, t, h, w, directed))
    p = tmp_path / "g.ads"
    g.store(p)
    for r in (cls.restore(p), cls.restore_mapped(p),
              cls.restore_mapped(p, verify=True),
              cls.from_bytes(g.to_bytes()), pickle.loads(pickle.dumps(g))):
        assert r.n == g.n and r.m == g.m and r.directed == g.directed
        assert np.array_equal(r.offsets, g.offsets)
        assert np.array_equal(r.heads, g.heads)
    if cls is ads.WeightedStaticGraph:
        # the arrays of a mapped graph lie in the mapping
        m = cls.restore_mapped(p)
        assert np.array_equal(m.weights, g.weights)
    with pytest.raises(ads.PersistError):
        ads.BitVector.restore(p)
    p.write_bytes(p.read_bytes()[:-8])
    with pytest.raises(ads.PersistError):
        cls.restore(p)


@pytest.mark.parametrize("cls", LAYOUTS)
@pytest.mark.parametrize("method", ["heap", "list", "fibonacci", "queue",
                                    "deque"])
def test_shortest_paths(cls, method, edges):
    n, t, h, w, directed = edges
    g = cls.from_edges(n, t, h, w, directed)
    G = nx_graph(n, t, h, w, directed)
    d, p = ads.shortest_paths(g, 0, method)
    ref = nx.single_source_dijkstra_path_length(G, 0)
    for v in range(n):
        assert d[v] == ref.get(v, np.inf)
        if v != 0 and v in ref:
            assert d[p[v]] + G[p[v]][v]["weight"] == d[v]
    for pd in (ads.delta_stepping(g, 0, 5.0), ads.rho_stepping(g, 0, 64)):
        assert np.array_equal(pd, d)


def test_negative_weights():
    g = ads.WeightedStaticGraph.from_edges(
        3, np.array([0, 1, 0], dtype=np.int32),
        np.array([1, 2, 2], dtype=np.int32), np.array([4.0, -3.0, 2.0]))
    with pytest.raises(ValueError):
        ads.shortest_paths(g, 0)
    d, p = ads.shortest_paths(g, 0, "queue")
    assert d.tolist() == [0, 4, 1] and p.tolist() == [-1, 0, 1]
    c = ads.WeightedStaticGraph.from_edges(
        2, np.array([0, 1], dtype=np.int32), np.array([1, 0], dtype=np.int32),
        np.array([1.0, -2.0]))
    with pytest.raises(ads.NegativeCycle):
        ads.shortest_paths(c, 0, "queue")


@pytest.mark.parametrize("parallel", [False, True])
def test_bfs_and_components(parallel, edges):
    n, t, h, w, directed = edges
    g = ads.StaticGraph.from_edges(n, t, h, directed)
    G = nx_graph(n, t, h, w, directed)
    d = ads.bfs(g, 3, parallel=parallel,
                transpose=g.transpose() if directed else None)
    ref = nx.single_source_shortest_path_length(G, 3)
    assert d.tolist() == [ref.get(v, -1) for v in range(n)]
    dist, parent = ads.bfs_tree(g, 3)
    assert np.array_equal(dist, d) and parent[3] == -1
    c = ads.connected_components(g, parallel=parallel)
    weak = (nx.weakly_connected_components(G) if directed
            else nx.connected_components(G))
    for comp in weak:
        assert {int(c[v]) for v in comp} == {min(comp)}


def test_strong_components_and_orders(edges):
    n, t, h, w, directed = edges
    if not directed:
        pytest.skip("oriented graphs")
    g = ads.StaticGraph.from_edges(n, t, h, True)
    G = nx_graph(n, t, h, w, True)
    for how in ("pearce", "tarjan"):
        c = ads.strongly_connected_components(g, how)
        for comp in nx.strongly_connected_components(G):
            assert {int(c[v]) for v in comp} == {min(comp)}
    assert ads.has_cycle(g) == (not nx.is_directed_acyclic_graph(G))
    keep = t < h
    dag = ads.StaticGraph.from_edges(n, t[keep], h[keep], True)
    order = ads.topological_sort(dag).tolist()
    place = {v: i for i, v in enumerate(order)}
    assert all(place[int(a)] < place[int(b)]
               for a, b in zip(t[keep], h[keep]))
    with pytest.raises(ValueError):
        ads.topological_sort(g if ads.has_cycle(g) else
                             ads.StaticGraph.from_edges(
                                 2, np.array([0, 1], dtype=np.int32),
                                 np.array([1, 0], dtype=np.int32)))


def test_independent_set_relabel_and_pool():
    t, h, _ = random_edges(300, 1200, 5, False, False)
    g = ads.StaticGraph.from_edges(300, t, h, False)
    pool = ads.ThreadPool(4)
    assert pool.threads == 4
    s = ads.maximal_independent_set(g, seed=7, pool=pool)
    one = ads.maximal_independent_set(g, seed=7, pool=ads.ThreadPool(1))
    assert np.array_equal(s, one)
    inset = np.zeros(300, dtype=bool)
    inset[s] = True
    off, heads = g.offsets, g.heads
    for u in range(300):
        nbrs = heads[off[u]:off[u + 1]]
        if inset[u]:
            assert not inset[nbrs].any()
        else:
            assert inset[nbrs].any()
    order = g.bfs_order(0)
    r = g.relabel(order)
    assert r.m == g.m and sorted(r.heads.tolist()) == sorted(
        order[g.heads].tolist())
    with pytest.raises(IndexError):
        ads.bfs(g, 300)
    with pytest.raises(ValueError):
        g.relabel(np.zeros(300, dtype=np.int32))


def test_weights_in_place():
    g = ads.InterleavedStaticGraph.from_edges(
        3, np.array([0, 1], dtype=np.int32), np.array([1, 2], dtype=np.int32),
        np.array([1.0, 2.0]), False)
    w = g.weights
    assert g.weight(1, 0) == 1.0 and g.weight(0, 2) is None
    assert g.set_weight(0, 1, 5.0)
    assert w[0] == 5.0 and g.weight(1, 0) == 5.0
    assert g.neighbors(1).tolist() == [0, 2]
    assert g.neighbor_weights(1).tolist() == [5.0, 2.0]
