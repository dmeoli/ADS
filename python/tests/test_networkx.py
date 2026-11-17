"""The backend nx-ads: discovered by NetworkX, its results equal to those
of NetworkX on random graphs, and the tests of NetworkX itself run on it
by its own machinery of the backends."""

import os
import subprocess
import sys

import numpy as np
import pytest

nx = pytest.importorskip("networkx")
import nx_ads  # noqa: E402


def random_graph(n, m, seed, directed, negative=False):
    rng = np.random.default_rng(seed)
    G = nx.DiGraph() if directed else nx.Graph()
    G.add_nodes_from(f"v{i}" for i in range(n))
    while G.number_of_edges() < m:
        u, v = rng.integers(0, n, 2)
        lo = -3 if negative else 1
        G.add_edge(f"v{u}", f"v{v}", weight=int(rng.integers(lo, 10)),
                   cost=float(rng.random()))
    return G


def test_discovered():
    assert "ads" in nx.utils.backends.backends
    assert nx.utils.backends.backend_info["ads"]["backend_name"] == "ads"


@pytest.mark.parametrize("directed", [True, False])
def test_shortest_paths_equal(directed):
    G = random_graph(150, 600, 1, directed)
    G.remove_edges_from(nx.selfloop_edges(G))
    s = "v0"
    for weight in (None, "weight", "cost"):
        ref = nx.shortest_path_length(G, s, weight=weight)
        got = nx.shortest_path_length(G, s, weight=weight, backend="ads")
        assert set(got) == set(ref)
        assert all(got[v] == pytest.approx(ref[v]) for v in ref)
        paths = nx.shortest_path(G, s, weight=weight, backend="ads")
        for v, p in paths.items():
            assert p[0] == s and p[-1] == v
            length = (len(p) - 1 if weight is None
                      else nx.path_weight(G, p, weight))
            assert length == pytest.approx(ref[v])
        t = max(ref, key=ref.get)
        assert nx.shortest_path_length(G, s, t, weight=weight,
                                       backend="ads") == \
            pytest.approx(ref[t])
    d, p = nx.single_source_dijkstra(G, s, backend="ads")
    assert d == pytest.approx(nx.single_source_dijkstra(G, s)[0])
    assert nx.single_source_shortest_path_length(G, s, cutoff=2,
                                                 backend="ads") == \
        nx.single_source_shortest_path_length(G, s, cutoff=2)
    assert nx.has_path(G, s, t, backend="ads") == nx.has_path(G, s, t)
    with pytest.raises(nx.NodeNotFound):
        nx.shortest_path(G, "absent", backend="ads")


def test_bellman_ford_and_negative_cycles():
    G = random_graph(80, 200, 2, True, negative=False)
    G.add_edge("v1", "v2", weight=-4)
    G.remove_edges_from(nx.selfloop_edges(G))
    try:
        ref = nx.single_source_bellman_ford_path_length(G, "v0")
    except nx.NetworkXUnbounded:
        with pytest.raises(nx.NetworkXUnbounded):
            nx.single_source_bellman_ford_path_length(G, "v0", backend="ads")
    else:
        assert nx.single_source_bellman_ford_path_length(
            G, "v0", backend="ads") == ref
    H = nx.DiGraph([("a", "b", {"weight": 1}), ("b", "a", {"weight": -2})])
    with pytest.raises(nx.NetworkXUnbounded):
        nx.bellman_ford_path(H, "a", "b", backend="ads")
    with pytest.raises(ValueError):
        nx.dijkstra_path(H, "a", "b", backend="ads")


def key(components):
    return sorted(sorted(c) for c in components)


def test_components_equal():
    for seed in range(3):
        U = random_graph(300, 250, seed, False)
        D = random_graph(300, 330, seed, True)
        assert key(nx.connected_components(U, backend="ads")) == \
            key(nx.connected_components(U))
        assert key(nx.strongly_connected_components(D, backend="ads")) == \
            key(nx.strongly_connected_components(D))
        assert key(nx.weakly_connected_components(D, backend="ads")) == \
            key(nx.weakly_connected_components(D))
        assert nx.is_connected(U, backend="ads") == nx.is_connected(U)
        assert nx.number_strongly_connected_components(D, backend="ads") == \
            nx.number_strongly_connected_components(D)
        assert nx.descendants(D, "v0", backend="ads") == \
            nx.descendants(D, "v0")
        assert nx.ancestors(D, "v0", backend="ads") == nx.ancestors(D, "v0")
        assert nx.is_directed_acyclic_graph(D, backend="ads") == \
            nx.is_directed_acyclic_graph(D)


def test_converted_once():
    G = random_graph(50, 100, 4, True)
    A = nx_ads.from_networkx(G, "weight")
    assert A.__networkx_backend__ == "ads"
    ref = nx.single_source_dijkstra_path_length(G, "v0")
    assert nx.single_source_dijkstra_path_length(A, "v0") == ref
    H = nx_ads.to_networkx(A)
    assert set(H.edges) == {e for e in G.edges if e[0] != e[1]}


SUITES = ["algorithms/shortest_paths", "algorithms/components",
          "algorithms/traversal", "algorithms/tests/test_dag.py"]


def test_networkx_suites_on_the_backend():
    root = os.path.dirname(nx.__file__)
    here = os.path.dirname(os.path.dirname(os.path.abspath(nx_ads.__file__)))
    path = os.pathsep.join([here] + sys.path)
    env = dict(os.environ, NETWORKX_TEST_BACKEND="ads",
               NETWORKX_FALLBACK_TO_NX="True", PYTHONPATH=path)
    r = subprocess.run(
        [sys.executable, "-m", "pytest", "-q", "-p", "no:cacheprovider",
         "--rootdir", root, "-x"] + [os.path.join(root, s) for s in SUITES],
        env=env, capture_output=True, text=True, cwd=root)
    assert r.returncode == 0, r.stdout[-4000:] + r.stderr[-2000:]
