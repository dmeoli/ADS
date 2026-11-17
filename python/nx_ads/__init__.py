"""nx-ads, a backend of NetworkX that runs the algorithms of ADS.

Once the package is installed, NetworkX finds the backend by its entry
point, and

    nx.shortest_path(G, source, backend="ads")

converts G into a graph in compressed rows of ADS (an AdsGraph, which can
also be made once by from_networkx() and passed in place of G) and runs
the algorithm of ADS on it; the result is that of NetworkX, with the
names of the nodes of G.
"""

from .backend import AdsGraph, BackendInterface, from_networkx, to_networkx

__all__ = ["AdsGraph", "BackendInterface", "from_networkx", "to_networkx",
           "get_info"]


def get_info():
    """The information of the backend, read by NetworkX from the entry
    point networkx.backend_info."""
    return {
        "backend_name": "ads",
        "project": "ads-python",
        "package": "nx_ads",
        "url": "https://github.com/dmeoli/ADS",
        "short_summary": "The algorithms of ADS on graphs in compressed "
                         "rows.",
        "functions": {name: {} for name in BackendInterface.algorithms},
        "default_config": {},
    }
