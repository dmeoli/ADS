"""The fixtures of the tests of the package ads: random graphs as arrays
of NumPy and as graphs of NetworkX, the same edges in both."""

import numpy as np
import pytest


def random_edges(n, m, seed, directed=True, weighted=True):
    """m distinct edges with no loops on n nodes, and their weights."""
    rng = np.random.default_rng(seed)
    seen = set()
    tails, heads = [], []
    while len(tails) < m:
        u, v = (int(x) for x in rng.integers(0, n, 2))
        key = (u, v) if directed else (min(u, v), max(u, v))
        if u == v or key in seen:
            continue
        seen.add(key)
        tails.append(u)
        heads.append(v)
    w = rng.integers(1, 20, m).astype(np.float64) if weighted else None
    return (np.array(tails, dtype=np.int32), np.array(heads, dtype=np.int32),
            w)


@pytest.fixture(params=[(True, 0), (False, 1), (True, 2)])
def edges(request):
    directed, seed = request.param
    t, h, w = random_edges(200, 900, seed, directed)
    return 200, t, h, w, directed


@pytest.fixture(autouse=True)
def _no_cache_warning():
    nx = pytest.importorskip("networkx")
    nx.config.warnings_to_ignore.add("cache")
    yield
