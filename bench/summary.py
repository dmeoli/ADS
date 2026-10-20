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
# the library in a cell is that of its fastest structure there; the compact
# directories (about 3% of the bits) are compared with those of the same
# space
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
    ('priority', 'ADS::DaryHeap d=4', 'tlx', ['tlx::DAryHeap d=4']),
    ('priority', 'ADS::PairingHeap', 'Boost',
     ['boost::heap::pairing_heap']),
    ('priority', 'ADS::RadixHeap', 'tlx', ['tlx::RadixHeap']),
    ('priority', 'ADS::RadixHeap', 'radix-heap',
     ['radix_heap::pair_radix_heap']),
    ('priority', 'ADS::DialQueue', 'tlx', ['tlx::RadixHeap']),
    ('priority', 'ADS::DialQueue', 'radix-heap',
     ['radix_heap::pair_radix_heap']),
    ('sequences', 'ADS::ArraySequence', 'std', ['std::vector']),
    ('sequences', 'ADS::ArraySequence', 'Boost',
     ['boost::container::vector', 'boost::container::devector']),
    ('sequences', 'ADS::SmallVector', 'Boost',
     ['boost::container::small_vector']),
    ('sequences', 'ADS::ArrayStack', 'std',
     ['std::stack<vector>', 'std::stack<deque>']),
    ('sequences', 'ADS::CursorList', 'std', ['std::forward_list']),
    ('sequences', 'ADS::dutchNationalFlag', 'std', ['std::partition x2']),
    ('sequences', 'ADS::sortKSorted', 'std', ['std::sort']),
    ('sequences', 'ADS::naturalMergeSort', 'std', ['std::list::sort']),
    ('sequences', 'ADS::List', 'std', ['std::list']),
    ('sequences', 'ADS::SinglyList', 'std', ['std::forward_list']),
    ('memory', 'ADS::Arena', 'std',
     ['std::pmr::monotonic_buffer_resource', 'new/delete']),
    ('memory', 'ADS::Arena', 'Boost', ['boost::object_pool']),
    ('memory', 'ADS::Arena', 'mimalloc', ['mimalloc']),
    ('memory', 'ADS::Arena', 'jemalloc', ['jemalloc']),
    ('memory', 'ADS::ObjectPool', 'std',
     ['std::pmr::unsynchronized_pool_resource', 'new/delete']),
    ('memory', 'ADS::ObjectPool', 'Boost',
     ['boost::object_pool', 'boost::pool']),
    ('memory', 'ADS::ObjectPool', 'mimalloc', ['mimalloc']),
    ('memory', 'ADS::ObjectPool', 'jemalloc', ['jemalloc']),
    ('bignum', 'ADS::BigInteger', 'Boost',
     ['boost::multiprecision::cpp_int']),
    ('bignum', 'ADS::BigInteger', 'GMP', ['GMP mpz_class']),
    ('bignum', 'ADS::Rational', 'Boost', ['boost::rational<cpp_int>']),
    ('bignum', 'ADS::Rational', 'GMP', ['GMP mpq_class']),
    ('bignum', 'ADS::BigInteger GMP', 'GMP', ['GMP mpz_class']),
    ('bignum', 'ADS::Rational GMP', 'GMP', ['GMP mpq_class']),
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
    ('search', 'ADS::StringSearcher', 'std',
     ['std::string_view::find', 'std::search', 'std::boyer_moore_searcher',
      'std::boyer_moore_horspool_searcher']),
    ('search', 'ADS::StringSearcher', 'Boost',
     ['boost::boyer_moore', 'boost::boyer_moore_horspool',
      'boost::knuth_morris_pratt']),
    ('search', 'ADS::StringSearcher', 'libc', ['memmem']),
    ('suffix array', 'ADS::suffixArray', 'libsais', ['libsais']),
    ('suffix array', 'ADS::suffixArray', 'libdivsufsort', ['divsufsort']),
    ('lcp', 'ADS::lcpArrayPhi', 'libsais', ['libsais plcp+lcp']),
    ('edit distance', 'ADS::levenshtein', 'edlib', ['edlib']),
    ('edit distance', 'ADS::levenshtein', 'rapidfuzz', ['rapidfuzz']),
    ('lcs', 'ADS::lcsLength', 'rapidfuzz', ['rapidfuzz']),
    ('approximate search', 'ADS::approximateMatches', 'edlib', ['edlib']),
    ('gcd', 'ADS::binaryGcd', 'std', ['std::gcd']),
    ('gcd', 'ADS::binaryGcd', 'Boost', ['boost::integer::gcd']),
    ('powmod', 'ADS::powMod', 'Boost', ['boost::multiprecision::powm']),
    ('powmod', 'ADS::powMod', 'GMP', ['GMP mpz_powm']),
    ('powmod', 'ADS::powMod', 'hurchalla', ['hurchalla MontgomeryForm']),
    ('isprime', 'ADS::isPrime', 'Boost',
     ['boost::multiprecision::miller_rabin_test']),
    ('isprime', 'ADS::isPrime', 'GMP', ['GMP mpz_probab_prime_p']),
    ('isprime', 'ADS::isPrime', 'hurchalla', ['hurchalla::is_prime']),
    ('factorize', 'ADS::factorize', 'hurchalla', ['hurchalla::factorize']),
    ('generators', 'ADS::Xoshiro256', 'std',
     ['std::mt19937_64', 'std::minstd_rand']),
    ('generators', 'ADS::Xoshiro256', 'Boost',
     ['boost::random::mt19937_64', 'boost::random::taus88']),
    ('generators', 'ADS::Xoshiro256', 'PCG', ['pcg64', 'pcg32']),
    ('generators', 'ADS::WyRand', 'PCG', ['pcg64', 'pcg32']),
    ('generators', 'ADS::Xoshiro256StarStar', 'PCG', ['pcg64', 'pcg32']),
    ('discrete', 'ADS::AliasTable', 'std', ['std::discrete_distribution']),
    ('discrete', 'ADS::AliasTable', 'Boost',
     ['boost::random::discrete_distribution']),
    ('bounded', 'ADS::uniformBelow', 'std',
     ['std::uniform_int_distribution']),
    ('bounded', 'ADS::uniformBelow', 'Boost',
     ['boost::random::uniform_int_distribution']),
    ('shuffle', 'ADS::shuffleFast', 'std',
     ['std::shuffle', 'std::shuffle mt19937_64']),
    ('sampling', 'ADS::reservoirSampleL', 'std',
     ['std::sample forward', 'std::sample input']),
    ('sampling', 'ADS::reservoirSampleL input', 'std',
     ['std::sample input']),
    ('queue', 'ADS::RingDeque', 'std', ['std::deque']),
    ('queue', 'ADS::RingDeque', 'Boost',
     ['boost::circular_buffer', 'boost::container::devector']),
    ('deque', 'ADS::RingDeque', 'std', ['std::deque']),
    ('deque', 'ADS::RingDeque', 'Boost',
     ['boost::circular_buffer', 'boost::container::devector']),
    ('stack', 'ADS::RingDeque', 'std', ['std::vector', 'std::deque']),
    ('stack', 'ADS::RingDeque', 'Boost', ['boost::container::devector']),
    ('stack', 'ADS::ArrayStack', 'std', ['std::vector', 'std::deque']),
    ('spsc', 'ADS::SPSCQueue', 'Boost', ['boost::lockfree::spsc_queue']),
    ('spsc', 'ADS::SPSCQueue', 'rigtorp', ['rigtorp::SPSCQueue']),
    ('spsc', 'ADS::SPSCQueue', 'moodycamel',
     ['moodycamel::ReaderWriterQueue']),
    ('spsc', 'ADS::SPSCQueue', 'folly', ['folly::ProducerConsumerQueue']),
    ('mpsc', 'ADS::MPSCInbox', 'Boost', ['boost::lockfree::queue']),
    ('mpsc', 'ADS::MPSCInbox', 'moodycamel',
     ['moodycamel::ConcurrentQueue']),
    ('mpsc', 'ADS::MPSCQueue', 'Boost', ['boost::lockfree::queue']),
    ('mpsc', 'ADS::MPSCQueue', 'moodycamel',
     ['moodycamel::ConcurrentQueue',
      'moodycamel::ConcurrentQueue tokens']),
    ('hull', 'ADS::convexHull', 'Boost', ['boost::geometry::convex_hull']),
    ('hull', 'ADS::convexHull', 'CGAL',
     ['CGAL::convex_hull_2', 'CGAL::ch_graham_andrew']),
    ('gemm', 'ADS::gemm', 'Eigen', ['Eigen']),
    ('gemm', 'ADS::gemm', 'OpenBLAS', ['OpenBLAS']),
    ('transpose', 'ADS::transpose', 'Eigen', ['Eigen']),
    ('spmv', 'ADS::CsrMatrix', 'Eigen', ['Eigen']),
    ('sptranspose', 'ADS::CsrMatrix', 'Eigen', ['Eigen']),
    ('bitvector', 'ADS::RankSelect', 'sdsl',
     ['sdsl::rank_support_v+mcl', 'sdsl::rank_support_v5']),
    ('bitvector', 'ADS::RankSelect', 'sux',
     ['sux::Rank9Sel+SimpleSelect', 'sux::Rank9Sel']),
    ('bitvector', 'ADS::RankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics',
      'pasta::WideRankSelect']),
    ('bitvector', 'ADS::RankSelect', 'succinct',
     ['succinct::rs_bit_vector']),
    ('bitvector', 'ADS::CompactRankSelect', 'sdsl',
     ['sdsl::rank_support_v5']),
    ('bitvector', 'ADS::CompactRankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics']),
    ('rrr', 'ADS::RRRBitVector', 'sdsl', ['sdsl::rrr_vector<63>']),
    ('eliasfano', 'ADS::EliasFano', 'sdsl', ['sdsl::sd_vector']),
    ('eliasfano', 'ADS::EliasFano', 'sux', ['sux::EliasFano']),
    ('eliasfano', 'ADS::EliasFano', 'succinct', ['succinct::elias_fano']),
    ('parentheses', 'ADS::BalancedParentheses', 'sdsl',
     ['sdsl::bp_support_sada']),
    ('parentheses', 'ADS::BalancedParentheses', 'succinct',
     ['succinct::bp_vector']),
    ('wavelet', 'ADS::WaveletMatrix', 'sdsl',
     ['sdsl::wm_int', 'sdsl::wt_int']),
    ('dac', 'ADS::DirectAccessCodes', 'sdsl',
     ['sdsl::dac_vector<4>', 'sdsl::dac_vector<8>']),
    ('dac', 'ADS::DirectAccessCodes b=8', 'sdsl', ['sdsl::dac_vector<8>']),
    ('entropy', 'ADS::huffmanCompress', 'huff0', ['huff0']),
    ('entropy', 'ADS::ransCompress', 'ryg_rans', ['ryg_rans']),
    ('entropy', 'ADS::tansCompress', 'FSE', ['FSE']),
    ('integers', 'ADS::streamVByteEncode', 'Stream VByte', ['streamvbyte']),
    ('integers', 'ADS::encodeGamma', 'sdsl', ['sdsl::elias_gamma']),
    ('integers', 'ADS::encodeDelta', 'sdsl', ['sdsl::elias_delta']),
    ('bwt', 'ADS::bwt', 'libsais', ['libsais']),
    ('fmindex', 'ADS::FMIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
    ('lz', 'ADS::lzssParseHashed', 'LZ4', ['LZ4_compress_default']),
    ('lz', 'ADS::lzssDecode', 'LZ4', ['LZ4_decompress_safe']),
    # the bits, against the structures of the same queries
    ('space', 'ADS::RankSelect', 'sdsl', ['sdsl::rank_support_v+mcl']),
    ('space', 'ADS::RankSelect', 'sux', ['sux::Rank9Sel+SimpleSelect']),
    ('space', 'ADS::RankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics',
      'pasta::WideRankSelect']),
    ('space', 'ADS::RankSelect', 'succinct', ['succinct::rs_bit_vector']),
    ('space', 'ADS::CompactRankSelect', 'sdsl', ['sdsl::rank_support_v5']),
    ('space', 'ADS::CompactRankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics']),
    ('space', 'ADS::RRRBitVector', 'sdsl', ['sdsl::rrr_vector<63>']),
    ('space', 'ADS::EliasFano', 'sdsl', ['sdsl::sd_vector']),
    ('space', 'ADS::EliasFano', 'sux', ['sux::EliasFano']),
    ('space', 'ADS::EliasFano', 'succinct', ['succinct::elias_fano']),
    ('space', 'ADS::BalancedParentheses', 'sdsl', ['sdsl::bp_support_sada']),
    ('space', 'ADS::BalancedParentheses', 'succinct', ['succinct::bp_vector']),
    ('space', 'ADS::WaveletMatrix', 'sdsl', ['sdsl::wm_int', 'sdsl::wt_int']),
    ('space', 'ADS::DirectAccessCodes', 'sdsl',
     ['sdsl::dac_vector<4>', 'sdsl::dac_vector<8>']),
    ('space', 'ADS::huffmanCompress', 'huff0', ['huff0']),
    ('space', 'ADS::ransCompress', 'ryg_rans', ['ryg_rans']),
    ('space', 'ADS::tansCompress', 'FSE', ['FSE']),
    ('space', 'ADS::streamVByteEncode', 'Stream VByte', ['streamvbyte']),
    ('space', 'ADS::FMIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
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
