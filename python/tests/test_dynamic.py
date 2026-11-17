"""The typed modifications of a graph of ADS read from Python: the events
of each change, the batches coalesced and undone, and the shortest paths
kept up to date."""

import math

import numpy as np
import pytest

import ads

nx = pytest.importorskip("networkx")


def test_events_one_by_one():
    g = ads.DynamicGraph(3, directed=False)
    g.add_edge(0, 1, 2.0)
    g.add_edge(1, 0, 3.0)
    g.set_weight(0, 1, 4.0)
    u = g.add_node()
    g.add_edge(2, u, 1.0)
    g.remove_node(2)
    assert g.events() == [
        ("edge_inserted", 0, 1, 2.0), ("weight_changed", 0, 1, 2.0, 3.0),
        ("weight_changed", 0, 1, 3.0, 4.0), ("node_inserted", 3),
        ("edge_inserted", 2, 3, 1.0), ("edge_deleted", 2, 3, 1.0),
        ("node_deleted", 2)]
    assert g.events() == []
    with pytest.raises(KeyError):
        g.remove_edge(0, 3)
    with pytest.raises(IndexError):
        g.add_edge(0, 9)


def test_batches_coalesce_and_undo():
    g = ads.DynamicGraph(4)
    g.add_edge(0, 1, 1.0)
    g.events()
    with g.batch():
        g.add_edge(1, 2, 5.0)
        g.set_weight(1, 2, 6.0)
        g.remove_edge(0, 1)
        g.add_edge(0, 1, 1.0)
        with g.batch():
            g.add_edge(2, 3, 1.0)
            g.remove_edge(2, 3)
        assert g.events() == []
    assert g.events() == [("edge_inserted", 1, 2, 6.0)]
    with pytest.raises(RuntimeError):
        with g.batch():
            g.set_weight(1, 2, 9.0)
            g.add_edge(3, 0, 2.0)
            raise RuntimeError
    assert g.weight(1, 2) == 6.0 and g.weight(3, 0) is None
    assert g.events() == [] and g.open_batches == 0


def test_shortest_paths_kept():
    rng = np.random.default_rng(4)
    n = 60
    g = ads.DynamicGraph(n)
    G = nx.DiGraph()
    G.add_nodes_from(range(n))
    for _ in range(300):
        u, v = (int(x) for x in rng.integers(0, n, 2))
        if u != v:
            w = float(rng.integers(1, 10))
            g.add_edge(u, v, w)
            G.add_edge(u, v, weight=w)
    g.track_shortest_paths(0)
    for step in range(40):
        u, v = list(G.edges)[int(rng.integers(0, G.number_of_edges()))]
        w = float(rng.integers(1, 10))
        g.set_weight(u, v, w)
        G[u][v]["weight"] = w
        ref = nx.single_source_dijkstra_path_length(G, 0)
        d = g.distances()
        assert all(d[x] == ref.get(x, math.inf) for x in range(n))
        assert g.reoptimization() in ("certified", "repaired", "recomputed")
    s = g.to_static()
    assert isinstance(s, ads.WeightedStaticGraph)
    assert s.m == G.number_of_edges()
    d, _ = ads.shortest_paths(s, 0)
    assert np.array_equal(d, g.distances())
