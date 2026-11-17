"""The backend nx-ads of NetworkX: the conversions and the algorithms.

A graph of NetworkX (no multigraph) becomes an AdsGraph: a StaticGraph of
ADS, with no weights or with the weights of one attribute of the edges
(the one that the dispatcher of NetworkX asks for), and the names of its
nodes in the order of G. The loops, which a StaticGraph has not, are
dropped and remembered, since only a negative one changes a result (a
negative cycle) and any one makes a cycle of an oriented graph. Each
algorithm takes the arguments of the function of NetworkX of its name,
raises its exceptions, and gives its result with the names of the nodes;
can_run() leaves to NetworkX the arguments that ADS does not take (a
weight given by a function, a method of the shortest paths other than
Dijkstra and Bellman-Ford, the shortest paths between all the pairs).
"""

import inspect

import networkx as nx
import numpy as np

import ads

# the size of a graph, nodes plus edges, below which NetworkX runs faster
# than the conversion of the graph and the call of ADS together
SMALL = 10000


class AdsGraph:
    """A graph of ADS with the names of the nodes of a graph of
    NetworkX."""

    __networkx_backend__ = "ads"

    def __init__(self, graph, nodes, weight=None, integral=True,
                 negative_loop=False, loops=False, graph_attrs=None):
        self.graph = graph
        self.nodes = list(nodes)
        self.index = {v: i for i, v in enumerate(self.nodes)}
        self.weight = weight
        self.integral = integral
        self.negative_loop = negative_loop
        self.loops = loops
        self.graph_attrs = dict(graph_attrs or {})
        self._negative = None

    def is_directed(self):
        return self.graph.directed

    def is_multigraph(self):
        return False

    def __len__(self):
        return len(self.nodes)

    def __contains__(self, v):
        try:
            return v in self.index
        except TypeError:
            return False

    def number_of_nodes(self):
        return len(self.nodes)

    def number_of_edges(self):
        return self.graph.m

    @property
    def weighted(self):
        return self.weight is not None

    def negative(self):
        """Whether an arc has a negative weight."""
        if self._negative is None:
            self._negative = bool(
                self.weighted and len(self.graph.weights)
                and np.min(self.graph.weights) < 0)
        return self._negative


def from_networkx(G, weight=None, default=1):
    """The AdsGraph of G, with the weights of the attribute weight of its
    edges (default if an edge has none), or none if weight is None."""
    if isinstance(G, AdsGraph):
        return G
    if G.is_multigraph():
        raise NotImplementedError("nx-ads does not take multigraphs")
    nodes = list(G)
    index = {v: i for i, v in enumerate(nodes)}
    n = len(nodes)
    tails, heads, weights = [], [], []
    loops = negative_loop = False
    integral = True
    if weight is None:
        for u, v in G.edges():
            if u == v:
                loops = True
                continue
            tails.append(index[u])
            heads.append(index[v])
    else:
        for u, v, w in G.edges(data=weight, default=default):
            if u == v:
                loops = True
                negative_loop = negative_loop or w < 0
                continue
            tails.append(index[u])
            heads.append(index[v])
            weights.append(w)
            integral = integral and float(w).is_integer()
    t = np.asarray(tails, dtype=np.int32)
    h = np.asarray(heads, dtype=np.int32)
    if weight is None:
        g = ads.StaticGraph.from_edges(n, t, h, G.is_directed())
    else:
        w = np.asarray(weights, dtype=np.float64)
        g = ads.WeightedStaticGraph.from_edges(n, t, h, w, G.is_directed())
    return AdsGraph(g, nodes, weight, integral, negative_loop, loops,
                    G.graph)


def to_networkx(G):
    """The graph of NetworkX of the AdsGraph G (its loops excepted)."""
    H = nx.DiGraph() if G.is_directed() else nx.Graph()
    H.graph.update(G.graph_attrs)
    H.add_nodes_from(G.nodes)
    g, names = G.graph, G.nodes
    off = np.asarray(g.offsets)
    heads = np.asarray(g.heads)
    w = np.asarray(g.weights) if G.weighted else None
    for u in range(g.n):
        for k in range(off[u], off[u + 1]):
            v = int(heads[k])
            if not G.is_directed() and v < u:
                continue
            if w is None:
                H.add_edge(names[u], names[v])
            else:
                x = float(w[k])
                H.add_edge(names[u], names[v],
                           **{G.weight: int(x) if G.integral else x})
    return H


# ---------------------------------------------------------------------------
# the pieces of the algorithms


def _source(G, s, message):
    if s not in G:
        raise nx.NodeNotFound(message.format(s))
    return G.index[s]


def _target(G, source, target, message):
    """The index of target, whose absence is no path, as in the
    shortest paths by the weights of NetworkX."""
    if target not in G:
        raise nx.NetworkXNoPath(message.format(source=source,
                                               target=target))
    return G.index[target]


def _number(G, x):
    """A distance as NetworkX gives it: an int if the weights are."""
    return int(x) if G.integral else float(x)


def _weighted(G, weight):
    """Whether the shortest paths of the call are by the weights."""
    if weight is None or not G.weighted:
        return False
    if weight != G.weight:
        raise NotImplementedError(
            f"the AdsGraph holds the weights {G.weight!r}, not {weight!r}")
    return True


def _tree(G, s, weight, method="heap", dijkstra=True):
    """The distances and the parents of the shortest paths from the node
    of index s, by the weights or by the arcs."""
    if not _weighted(G, weight):
        d, p = ads.bfs_tree(G.graph, s)
        d = d.astype(np.float64)
        d[d < 0] = np.inf
        return d, p, False
    if G.negative():
        method = "queue"
    if G.negative_loop:
        if dijkstra:
            raise ValueError("Contradictory paths found: negative weights?")
        raise nx.NetworkXUnbounded("Negative cycle detected.")
    try:
        d, p = ads.shortest_paths(G.graph, s, method)
    except ads.NegativeCycle:
        if dijkstra:
            raise ValueError(
                "Contradictory paths found: negative weights?") from None
        raise nx.NetworkXUnbounded("Negative cycle detected.") from None
    return d, p, True


def _paths(G, d, p, cutoff=None):
    """The shortest path to each node reached, from the parents."""
    names = G.nodes
    paths = {}
    reached = np.flatnonzero(np.isfinite(d) if cutoff is None
                             else d <= cutoff)
    for i in reached[np.argsort(d[reached], kind="stable")]:
        i = int(i)
        if i in paths:
            continue
        chain = [i]
        while p[chain[-1]] >= 0 and int(p[chain[-1]]) not in paths:
            chain.append(int(p[chain[-1]]))
        top = int(p[chain[-1]])
        prefix = paths[top] if top >= 0 else []
        for j in reversed(chain):
            prefix = prefix + [names[j]]
            paths[j] = prefix
    return {names[i]: paths[i] for i in reached[np.argsort(
        d[reached], kind="stable")]}


def _path(G, d, p, t, source, target, message):
    if not np.isfinite(d[t]):
        raise nx.NetworkXNoPath(message.format(source=source,
                                               target=target))
    chain = [t]
    while p[chain[-1]] >= 0:
        chain.append(int(p[chain[-1]]))
    return [G.nodes[i] for i in reversed(chain)]


def _lengths(G, d, cutoff=None, weighted=True):
    reached = np.flatnonzero(np.isfinite(d) if cutoff is None
                             else d <= cutoff)
    order = reached[np.argsort(d[reached], kind="stable")]
    if weighted:
        return {G.nodes[i]: _number(G, d[i]) for i in order}
    return {G.nodes[i]: int(d[i]) for i in order}


def _components(G, labels):
    groups = {}
    for i, c in enumerate(labels.tolist()):
        groups.setdefault(c, set()).add(G.nodes[i])
    return groups.values()


def _null(G):
    if len(G) == 0:
        raise nx.NetworkXPointlessConcept(
            "Connectivity is undefined for the null graph.")


# ---------------------------------------------------------------------------
# the backend


class BackendInterface:
    """The backend ads of NetworkX: the conversions, can_run(),
    should_run(), and an algorithm for each name of NetworkX in
    algorithms."""

    algorithms = (
        "shortest_path", "shortest_path_length", "has_path",
        "single_source_shortest_path", "single_source_shortest_path_length",
        "bidirectional_shortest_path", "all_pairs_shortest_path_length",
        "dijkstra_path", "dijkstra_path_length", "single_source_dijkstra",
        "single_source_dijkstra_path", "single_source_dijkstra_path_length",
        "bellman_ford_path", "bellman_ford_path_length",
        "single_source_bellman_ford", "single_source_bellman_ford_path",
        "single_source_bellman_ford_path_length", "connected_components",
        "number_connected_components", "is_connected",
        "node_connected_component", "strongly_connected_components",
        "number_strongly_connected_components", "is_strongly_connected",
        "weakly_connected_components", "number_weakly_connected_components",
        "is_weakly_connected", "is_directed_acyclic_graph",
        "topological_sort", "descendants", "ancestors",
    )

    @staticmethod
    def convert_from_nx(G, edge_attrs=None, node_attrs=None,
                        preserve_edge_attrs=False, preserve_node_attrs=False,
                        preserve_graph_attrs=False, name=None,
                        graph_name=None):
        if isinstance(G, AdsGraph):
            return G
        weight, default = None, 1
        if edge_attrs:
            if len(edge_attrs) > 1:
                raise NotImplementedError("nx-ads keeps one weight")
            weight, default = next(iter(edge_attrs.items()))
            if default is None:
                default = 1
        elif preserve_edge_attrs is True:
            weight = "weight"
        elif isinstance(preserve_edge_attrs, str):
            weight = preserve_edge_attrs
        return from_networkx(G, weight, default)

    @staticmethod
    def convert_to_nx(obj, *, name=None):
        if isinstance(obj, AdsGraph):
            return to_networkx(obj)
        return obj

    @staticmethod
    def can_run(name, args, kwargs):
        if name not in BackendInterface.algorithms:
            return f"{name} is not implemented by nx-ads"
        f = getattr(BackendInterface, name)
        try:
            b = inspect.signature(f).bind(*args, **kwargs)
        except TypeError as e:
            return str(e)
        a = b.arguments
        G = a.get("G")
        if G is not None and G.is_multigraph():
            return "nx-ads does not take multigraphs"
        w = a.get("weight")
        if w is not None and not isinstance(w, str):
            return "nx-ads takes a weight only as the name of an attribute"
        if name in ("shortest_path", "shortest_path_length"):
            if a.get("source") is None:
                return "nx-ads needs a source"
            if a.get("method", "dijkstra") not in ("dijkstra",
                                                   "bellman-ford"):
                return "nx-ads runs Dijkstra and Bellman-Ford"
        return True

    @staticmethod
    def should_run(name, args, kwargs):
        G = args[0] if args else kwargs.get("G")
        if isinstance(G, AdsGraph):
            return True
        if G is not None and G.number_of_nodes() + G.number_of_edges() < \
                SMALL:
            return "the graph is too small to pay for the conversion"
        return True

    @staticmethod
    def on_start_tests(items):
        # the tests of NetworkX whose results differ from those of ADS
        # only where both are correct
        import pytest

        for item in items:
            for key, reason in _KNOWN.items():
                if key in item.nodeid:
                    item.add_marker(pytest.mark.xfail(reason=reason))

    # -- the shortest paths -------------------------------------------------

    @staticmethod
    def shortest_path(G, source=None, target=None, weight=None,
                      method="dijkstra"):
        s = _source(G, source, "Source {} is not in G")
        if target is not None:
            t = _source(G, target, "Target {} is not in G")
        d, p, _ = _tree(G, s, weight, dijkstra=method == "dijkstra")
        if target is None:
            return _paths(G, d, p)
        return _path(G, d, p, t, source, target,
                     "No path between {source} and {target}.")

    @staticmethod
    def shortest_path_length(G, source=None, target=None, weight=None,
                             method="dijkstra"):
        s = _source(G, source, "Source {} is not in G")
        if target is not None:
            t = _source(G, target, "Target {} is not in G")
        d, _, weighted = _tree(G, s, weight, dijkstra=method == "dijkstra")
        if target is None:
            return _lengths(G, d, weighted=weighted)
        if not np.isfinite(d[t]):
            raise nx.NetworkXNoPath(f"No path between {source} and "
                                    f"{target}.")
        return _number(G, d[t]) if weighted else int(d[t])

    @staticmethod
    def has_path(G, source, target):
        s = _source(G, source, "Source {} is not in G")
        t = _source(G, target, "Target {} is not in G")
        return bool(ads.bfs(G.graph, s)[t] >= 0)

    @staticmethod
    def single_source_shortest_path(G, source, cutoff=None):
        s = _source(G, source, "Source {} not in G")
        d, p, _ = _tree(G, s, None)
        return _paths(G, d, p, cutoff)

    @staticmethod
    def single_source_shortest_path_length(G, source, cutoff=None):
        s = _source(G, source, "Source {} is not in G")
        d = ads.bfs(G.graph, s).astype(np.float64)
        d[d < 0] = np.inf
        return _lengths(G, d, cutoff, weighted=False)

    @staticmethod
    def bidirectional_shortest_path(G, source, target):
        s = _source(G, source, "Source {} is not in G")
        t = _source(G, target, "Target {} is not in G")
        d, p, _ = _tree(G, s, None)
        return _path(G, d, p, t, source, target,
                     "No path between {source} and {target}.")

    @staticmethod
    def all_pairs_shortest_path_length(G, cutoff=None):
        for v in G.nodes:
            yield v, BackendInterface.single_source_shortest_path_length(
                G, v, cutoff)

    @staticmethod
    def dijkstra_path(G, source, target, weight="weight"):
        s = _source(G, source, "Node {} not found in graph")
        t = _target(G, source, target,
                    "Node {target} not reachable from {source}")
        d, p, _ = _tree(G, s, weight)
        return _path(G, d, p, t, source, target,
                     "Node {target} not reachable from {source}")

    @staticmethod
    def dijkstra_path_length(G, source, target, weight="weight"):
        s = _source(G, source, "Node {} not found in graph")
        t = _target(G, source, target,
                    "Node {target} not reachable from {source}")
        d, _, weighted = _tree(G, s, weight)
        if not np.isfinite(d[t]):
            raise nx.NetworkXNoPath(f"Node {target} not reachable from "
                                    f"{source}")
        return _number(G, d[t]) if weighted else int(d[t])

    @staticmethod
    def single_source_dijkstra(G, source, target=None, cutoff=None,
                               weight="weight"):
        s = _source(G, source, "Node {} not found in graph")
        d, p, weighted = _tree(G, s, weight)
        if target is None:
            return (_lengths(G, d, cutoff, weighted),
                    _paths(G, d, p, cutoff))
        t = _target(G, source, target, "No path to {target}.")
        if not np.isfinite(d[t]) or (cutoff is not None and d[t] > cutoff):
            raise nx.NetworkXNoPath(f"No path to {target}.")
        return (_number(G, d[t]) if weighted else int(d[t]),
                _path(G, d, p, t, source, target, ""))

    @staticmethod
    def single_source_dijkstra_path(G, source, cutoff=None,
                                    weight="weight"):
        s = _source(G, source, "Node {} not found in graph")
        d, p, _ = _tree(G, s, weight)
        return _paths(G, d, p, cutoff)

    @staticmethod
    def single_source_dijkstra_path_length(G, source, cutoff=None,
                                           weight="weight"):
        s = _source(G, source, "Node {} not found in graph")
        d, _, weighted = _tree(G, s, weight)
        return _lengths(G, d, cutoff, weighted)

    @staticmethod
    def bellman_ford_path(G, source, target, weight="weight"):
        s = _source(G, source, "Node {} not found in graph")
        t = _target(G, source, target,
                    "node {target} not reachable from {source}")
        d, p, _ = _tree(G, s, weight, dijkstra=False)
        return _path(G, d, p, t, source, target,
                     "node {target} not reachable from {source}")

    @staticmethod
    def bellman_ford_path_length(G, source, target, weight="weight"):
        s = _source(G, source, "Node {} not found in graph")
        t = _target(G, source, target,
                    "node {target} not reachable from {source}")
        d, _, weighted = _tree(G, s, weight, dijkstra=False)
        if not np.isfinite(d[t]):
            raise nx.NetworkXNoPath(f"node {target} not reachable from "
                                    f"{source}")
        return _number(G, d[t]) if weighted else int(d[t])

    @staticmethod
    def single_source_bellman_ford(G, source, target=None, weight="weight"):
        s = _source(G, source, "Node {} is not found in the graph")
        d, p, weighted = _tree(G, s, weight, dijkstra=False)
        if target is None:
            return _lengths(G, d, None, weighted), _paths(G, d, p)
        t = _target(G, source, target,
                    "Node {target} not reachable from {source}")
        if not np.isfinite(d[t]):
            raise nx.NetworkXNoPath(f"Node {target} not reachable from "
                                    f"{source}")
        return (_number(G, d[t]) if weighted else int(d[t]),
                _path(G, d, p, t, source, target, ""))

    @staticmethod
    def single_source_bellman_ford_path(G, source, weight="weight"):
        s = _source(G, source, "Node {} is not found in the graph")
        d, p, _ = _tree(G, s, weight, dijkstra=False)
        return _paths(G, d, p)

    @staticmethod
    def single_source_bellman_ford_path_length(G, source, weight="weight"):
        s = _source(G, source, "Node {} is not found in the graph")
        d, _, weighted = _tree(G, s, weight, dijkstra=False)
        return _lengths(G, d, None, weighted)

    # -- the components -----------------------------------------------------

    @staticmethod
    def connected_components(G):
        yield from _components(G, ads.connected_components(G.graph))

    @staticmethod
    def number_connected_components(G):
        c = ads.connected_components(G.graph)
        return int(np.count_nonzero(c == np.arange(len(c))))

    @staticmethod
    def is_connected(G):
        _null(G)
        return BackendInterface.number_connected_components(G) == 1

    @staticmethod
    def node_connected_component(G, n):
        i = _source(G, n, "{}")
        c = ads.connected_components(G.graph)
        return {G.nodes[j] for j in np.flatnonzero(c == c[i])}

    @staticmethod
    def strongly_connected_components(G):
        # in the reverse of a topological order, as Tarjan gives them
        c = ads.strongly_connected_components(G.graph, topological=True)
        comps = [set() for _ in range(int(c.max()) + 1 if len(c) else 0)]
        for i, k in enumerate(c.tolist()):
            comps[k].add(G.nodes[i])
        yield from reversed(comps)

    @staticmethod
    def number_strongly_connected_components(G):
        c = ads.strongly_connected_components(G.graph)
        return int(np.count_nonzero(c == np.arange(len(c))))

    @staticmethod
    def is_strongly_connected(G):
        _null(G)
        return BackendInterface.number_strongly_connected_components(G) == 1

    @staticmethod
    def weakly_connected_components(G):
        yield from _components(G, ads.connected_components(G.graph))

    @staticmethod
    def number_weakly_connected_components(G):
        return BackendInterface.number_connected_components(G)

    @staticmethod
    def is_weakly_connected(G):
        _null(G)
        return BackendInterface.number_connected_components(G) == 1

    # -- the orders ---------------------------------------------------------

    @staticmethod
    def is_directed_acyclic_graph(G):
        return (G.is_directed() and not G.loops
                and not ads.has_cycle(G.graph))

    @staticmethod
    def topological_sort(G):
        if not G.is_directed():
            raise nx.NetworkXError(
                "Topological sort not defined on undirected graphs.")
        if G.loops or ads.has_cycle(G.graph):
            raise nx.NetworkXUnfeasible(
                "Graph contains a cycle or graph changed during iteration")
        for i in ads.topological_sort(G.graph).tolist():
            yield G.nodes[i]

    @staticmethod
    def descendants(G, source):
        if source not in G:
            raise nx.NetworkXError(f"The node {source} is not in the graph.")
        d = ads.bfs(G.graph, G.index[source])
        return {G.nodes[i] for i in np.flatnonzero(d > 0)}

    @staticmethod
    def ancestors(G, source):
        if source not in G:
            raise nx.NetworkXError(f"The node {source} is not in the graph.")
        g = G.graph.transpose() if G.is_directed() else G.graph
        d = ads.bfs(g, G.index[source])
        return {G.nodes[i] for i in np.flatnonzero(d > 0)}


# the tests of NetworkX (by a part of their node id) that check a result
# where ADS gives another correct one, or a message of NetworkX
_KNOWN = {
    # the order is computed once, on the graph converted: a change of the
    # graph of NetworkX during the iteration is not seen
    "test_dag.py::TestDAG::test_topological_sort6":
        "the backend sorts a snapshot of the graph",
}
