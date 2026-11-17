"""The backend nx-ads of NetworkX: the same call, run by ADS [see section
C.21 of the manual]."""

import networkx as nx

nx.config.warnings_to_ignore.add("cache")

G = nx.grid_2d_graph(30, 30)
for k, (u, v) in enumerate(G.edges):
    G.edges[u, v]["weight"] = 1 + k % 7

ours = nx.single_source_dijkstra_path_length(G, (0, 0), backend="ads")
theirs = nx.single_source_dijkstra_path_length(G, (0, 0))
print(ours[(29, 29)], ours == theirs)
print(nx.shortest_path(G, (0, 0), (2, 1), weight="weight", backend="ads"))
print(nx.number_connected_components(G, backend="ads"))
