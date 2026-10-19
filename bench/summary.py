#!/usr/bin/env python3
###############################################################################
#                               summary.py                                    #
#                                                                             #
# Turns two files of results/, written by two runs of bench/run.sh on the     #
# same machine, into the summary table at the top of the README: for each     #
# structure of ADS and each library it is compared with, the ratio of the     #
# time of the library to that of ADS (above 1 when ADS is faster), as the     #
# median and the range over the cases and the sizes of its benchmark, each    #
# cell the mean of the two runs. The noise is the relative difference of      #
# every cell between the two runs, the same code measured twice (A/A).        #
#                                                                             #
#   python3 bench/summary.py results/<run 1>.tsv results/<run 2>.tsv          #
###############################################################################

import statistics
import sys

# ( benchmark , structure of ADS , library , its structures ): the time of
# the library in a cell is that of its fastest structure there
PAIRS = [
    ('sort', 'ADS::sort', 'std', ['std::sort']),
    ('sort', 'ADS::sort', 'Boost', ['boost::pdqsort', 'boost::spreadsort']),
    ('sort', 'ADS::stableSort', 'std', ['std::stable_sort']),
    ('sort', 'ADS::stableSort', 'Boost',
     ['boost::flat_stable_sort', 'boost::spinsort']),
    ('dictionaries', 'ADS::FlatHashTable', 'std', ['std::unordered_map']),
    ('dictionaries', 'ADS::FlatHashTable', 'Boost',
     ['boost::unordered_flat_map']),
    ('dictionaries', 'ADS::RedBlackTree', 'std', ['std::map']),
    ('priority', 'ADS::DaryHeap d=4', 'std', ['std::priority_queue']),
    ('priority', 'ADS::DaryHeap d=4', 'Boost',
     ['boost::heap::d_ary_heap d=4']),
    ('priority', 'ADS::FibonacciHeapPriorityQueue', 'Boost',
     ['boost::heap::fibonacci_heap']),
    ('sequences', 'ADS::ArraySequence', 'std', ['std::vector']),
    ('sequences', 'ADS::List', 'std', ['std::list']),
    ('sequences', 'ADS::SinglyList', 'std', ['std::forward_list']),
    ('bignum', 'ADS::BigInteger', 'Boost',
     ['boost::multiprecision::cpp_int']),
    ('bignum', 'ADS::BigInteger', 'GMP', ['GMP mpz_class']),
    ('bignum', 'ADS::Rational', 'Boost', ['boost::rational<cpp_int>']),
    ('bignum', 'ADS::Rational', 'GMP', ['GMP mpq_class']),
    ('fft', 'ADS::fft', 'FFTW', ['FFTW estimate', 'FFTW measure']),
    ('hash functions', 'ADS::FastHash', 'std', ['std::hash']),
    ('hash functions', 'ADS::FastHash', 'Boost', ['boost::hash']),
    ('hash functions', 'ADS::FastHash', 'Abseil', ['absl::Hash']),
    ('hash functions', 'ADS::FastHash', 'unordered_dense', ['ankerl::hash']),
    ('hash functions', 'ADS::FastHash', 'xxHash', ['XXH3']),
    ('hash functions', 'ADS::FastHash', 'wyhash', ['wyhash']),
    ('hash functions', 'ADS::FastHash', 'rapidhash', ['rapidhash']),
    ('hash maps', 'ADS::FlatHashTable', 'std', ['std::unordered_map']),
    ('hash maps', 'ADS::FlatHashTable', 'Boost',
     ['boost::unordered_flat_map', 'boost::unordered_node_map',
      'boost::unordered_map']),
    ('hash maps', 'ADS::FlatHashTable', 'Abseil', ['absl::flat_hash_map']),
    ('hash maps', 'ADS::FlatHashTable', 'unordered_dense',
     ['ankerl::unordered_dense::map']),
    ('hash maps', 'ADS::FlatHashTable', 'robin-map', ['tsl::robin_map']),
    ('hash maps', 'ADS::BucketCuckooTable', 'libcuckoo',
     ['libcuckoo::cuckoohash_map']),
    ('filters', 'ADS::BlockedBloomFilter', 'fastfilter',
     ['fastfilter::SimdBlockFilter']),
    ('filters', 'ADS::BinaryFuseFilter 8', 'xor_singleheader',
     ['xorfilter::binary_fuse8']),
    ('filters', 'ADS::BinaryFuseFilter 16', 'xor_singleheader',
     ['xorfilter::binary_fuse16']),
    ('perfect hashing', 'ADS::MinimalPerfectHash', 'PTHash', ['PTHash C']),
    ('perfect hashing', 'ADS::MinimalPerfectHash', 'PHOBIC',
     ['PHOBIC R-int']),
    ('perfect hashing', 'ADS::MinimalPerfectHash', 'BBHash', ['BBHash g=2']),
    ('perfect hashing', 'ADS::MinimalPerfectHash', 'RecSplit',
     ['RecSplit l=8 b=100']),
]

def read(path):
    head, cell = {}, {}
    for line in open(path):
        line = line.rstrip('\n')
        if line.startswith('#'):
            k, v = line[2:].split('\t')
            head[k] = v
        elif line:
            b, case, n, who, ns = line.split('\t')
            cell[(b, case, int(n), who)] = float(ns)
    return head, cell

def ratio(x):
    return '%.2f' % x if x < 9.95 else '%.0f' % x

def main(p1, p2):
    h1, c1 = read(p1)
    h2, c2 = read(p2)
    both = [k for k in c1 if k in c2 and c1[k] > 0 and c2[k] > 0]
    d = sorted(abs(c2[k] - c1[k]) / c1[k] for k in both)
    print('- machine: %s, one core of %s (core %s), %s' %
          (h1['machine'], h1['cores'], h1['core'], h1['compiler']))
    print('- each cell the median of %s runs; load %s at the start, %s at '
          'the end' % (h1['runs'], h1['load'].split()[0],
                       h2['load at the end'].split()[0]))
    print('- A/A over %d cells: median %.1f%%, 90th percentile %.1f%%, '
          'largest %.0f%%' % (len(d), 100 * statistics.median(d),
                              100 * d[int(0.9 * (len(d) - 1))],
                              100 * d[-1]))
    print()
    cell = {k: (c1[k] + c2[k]) / 2 for k in both}
    rows = {}                   # benchmark -> structure -> its rows
    for bench, ads, lib, whos in PAIRS:
        groups = sorted({k[0] for k in cell
                         if k[0] == bench or k[0].startswith(bench + ' ')})
        for b in groups:
            R = []
            for (bb, case, n, who), v in cell.items():
                if bb != b or who != ads:
                    continue
                t = [cell[(b, case, n, w)] for w in whos
                     if (b, case, n, w) in cell]
                if t:
                    R.append(min(t) / v)
            if R:
                rows.setdefault(b, {}).setdefault(ads, []).append(
                    '<td>%s</td><td align="right">%d</td><td align="right">'
                    '%s</td><td align="right">%s to %s</td>' %
                    (lib, len(R), ratio(statistics.median(R)),
                     ratio(min(R)), ratio(max(R))))
    # an HTML table, which GitHub renders, so that a benchmark and a
    # structure span their rows
    print('<table>')
    print('<tr><th>benchmark</th><th>ADS</th><th>against</th>'
          '<th>cells</th><th>median</th><th>range</th></tr>')
    for b, S in rows.items():
        first = '<td rowspan="%d">%s</td>' % (sum(map(len, S.values())), b)
        for ads, R in S.items():
            for k, r in enumerate(R):
                print('<tr>' + first +
                      ('<td rowspan="%d"><code>%s</code></td>' %
                       (len(R), ads) if k == 0 else '') + r + '</tr>')
                first = ''
    print('</table>')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])

###############################################################################
#                            End of summary.py                                #
###############################################################################
