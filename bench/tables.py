#!/usr/bin/env python3
###############################################################################
#                                tables.py                                    #
#                                                                             #
# Turns a file of results/ written by bench/run.sh into the Markdown tables   #
# of the README: one table per benchmark, a row per structure and a column    #
# per ( case , n ), each cell the time per operation in nanoseconds and,      #
# in parentheses, its ratio to the first structure of the benchmark (the      #
# one of the standard library). The noise of the machine is the largest       #
# relative difference between that structure and its run "A/A". The           #
# order of the columns keeps that of the cells of each structure, so that     #
# the lines of a run with a process per structure give the tables of one      #
# process.                                                                    #
#                                                                             #
#   python3 bench/tables.py results/<file>.tsv                                #
###############################################################################

import heapq
import sys
from collections import OrderedDict

def columns(R):
    """the ( case , n ) of the cells R, in an order that keeps that of the
    cells of each structure; where it leaves a choice, the earlier size of
    its structure first, then the earlier cell"""
    first, size, after, before = {}, {}, {}, {}
    last, sizes = {}, {}
    for c, n, w, _ in R:
        k = (c, n)
        seen = sizes.setdefault(w, [])
        if n not in seen:
            seen.append(n)
        if k not in first:
            first[k] = len(first)
            size[k] = seen.index(n)
            after[k], before[k] = set(), 0
        p = last.get(w)
        if p is not None and p != k and k not in after[p]:
            after[p].add(k)
            before[k] += 1
        last[w] = k
    rank = {k: (size[k], first[k]) for k in first}
    ready = [(rank[k], k) for k in first if before[k] == 0]
    heapq.heapify(ready)
    out, done = [], set()
    while len(out) < len(first):
        if not ready:               # a cycle: the earliest left goes first
            k = min((k for k in first if k not in done), key=rank.get)
            ready.append((rank[k], k))
        _, k = heapq.heappop(ready)
        if k in done:
            continue
        done.add(k)
        out.append(k)
        for q in after[k]:
            before[q] -= 1
            if before[q] == 0:
                heapq.heappush(ready, (rank[q], q))
    return out

def main(path):
    head, rows = [], []
    for line in open(path):
        line = line.rstrip('\n')
        if line.startswith('#'):
            head.append(line[2:].split('\t'))
        elif line:
            b, case, n, who, ns = line.split('\t')
            rows.append((b, case, int(n), who, float(ns)))
    for k, v in head:
        print('- %s: %s' % (k, v))
    print()
    benches = OrderedDict()
    for b, case, n, who, ns in rows:
        benches.setdefault(b, []).append((case, n, who, ns))
    for b, R in benches.items():
        if b.endswith(':count'):          # the counts that identify a run
            continue
        cols = columns(R)
        whos = list(OrderedDict.fromkeys(w for _, _, w, _ in R))
        cell = {(c, n, w): ns for c, n, w, ns in R}
        ref = whos[0]
        noise = [abs(cell[(c, n, ref + ' A/A')] - cell[(c, n, ref)]) /
                 cell[(c, n, ref)] for c, n in cols
                 if (c, n, ref + ' A/A') in cell and cell[(c, n, ref)] > 0]
        print('**%s** (%s)' % (b, 'noise A/A at most %.0f%%' %
                                (100 * max(noise)) if noise else
                                'no A/A run: the noise is that of the other '
                                'tables'))
        print()
        print('| | ' + ' | '.join('%s, n = %d' % (c, n) for c, n in cols)
              + ' |')
        print('|---|' + '---|' * len(cols))
        for w in whos:
            if w.endswith(' A/A'):
                continue
            out = []
            for c, n in cols:
                if (c, n, w) not in cell:
                    out.append('')
                    continue
                v = cell[(c, n, w)]
                r = cell.get((c, n, ref))
                out.append('%.3g' % v + (' (%.2g)' % (v / r) if r and w != ref
                                         else ''))
            print('| %s | %s |' % (w, ' | '.join(out)))
        print()

if __name__ == '__main__':
    main(sys.argv[1])

###############################################################################
#                             End of tables.py                                #
###############################################################################
