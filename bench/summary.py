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
    ('sort', 'ADS::sort', 'IPS4o', ['ips4o', 'ips2ra']),
    ('sort', 'ADS::sort', 'x86-simd-sort', ['x86-simd-sort']),
    ('sort', 'ADS::sort', 'vqsort', ['vqsort']),
    ('sort', 'ADS::sort', 'ska_sort', ['ska_sort']),
    ('sort', 'ADS::sort', 'scandum', ['crumsort', 'fluxsort', 'quadsort']),
    ('sort', 'ADS::stableSort', 'scandum', ['fluxsort', 'quadsort']),
    ('sort record', 'ADS::radixSortBy (key)', 'IPS2Ra', ['ips2ra (key)']),
    ('sort record', 'ADS::radixSortBy (key)', 'ska_sort', ['ska_sort (key)']),
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
    ('priority', 'ADS::SequenceHeap', 'std', ['std::priority_queue']),
    ('priority', 'ADS::SequenceHeap', 'Boost',
     ['boost::heap::d_ary_heap d=4']),
    ('priority', 'ADS::SequenceHeap', 'tlx', ['tlx::DAryHeap d=4']),
    ('priority', 'ADS::MinMaxHeap', 'std', ['std::multiset']),
    ('priority', 'ADS::IntervalHeap', 'std', ['std::multiset']),
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
    ('sequences', 'ADS::TieredVector', 'std',
     ['std::vector', 'std::deque']),
    ('sequences', 'ADS::SmallVector', 'Boost',
     ['boost::container::small_vector']),
    ('sequences', 'ADS::ArrayStack', 'std',
     ['std::stack<vector>', 'std::stack<deque>']),
    ('sequences', 'ADS::CursorList', 'std', ['std::forward_list']),
    ('sequences', 'ADS::dutchNationalFlag', 'std', ['std::partition x2']),
    ('sequences', 'ADS::sortKSorted', 'std', ['std::sort']),
    ('sequences', 'ADS::naturalMergeSort', 'std', ['std::list::sort']),
    ('sequences', 'ADS::listSort', 'std', ['std::list::sort']),
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
    ('memory', 'ADS::SizeClassAllocator', 'std',
     ['new/delete', 'std::pmr::unsynchronized_pool_resource']),
    ('memory', 'ADS::SizeClassAllocator', 'mimalloc', ['mimalloc']),
    ('memory', 'ADS::SizeClassAllocator', 'jemalloc', ['jemalloc']),
    ('allocators', 'ADS::PoolAllocator', 'std',
     ['std::allocator', 'std::pmr::unsynchronized_pool_resource']),
    ('allocators', 'ADS::PoolAllocator', 'Boost',
     ['boost::fast_pool_allocator']),
    ('allocators', 'ADS::PoolAllocator', 'mimalloc', ['mimalloc']),
    ('allocators', 'ADS::PoolAllocator concurrent', 'std',
     ['std::allocator', 'std::pmr::synchronized_pool_resource',
      'std::pmr::unsynchronized_pool_resource']),
    ('allocators', 'ADS::PoolAllocator concurrent', 'Boost',
     ['boost::fast_pool_allocator', 'boost::fast_pool_allocator mutex']),
    ('allocators', 'ADS::PoolAllocator concurrent', 'mimalloc',
     ['mimalloc']),
    ('positions', 'ADS::PositionIndex', 'std', ['std::unordered_map']),
    ('positions', 'ADS::PositionIndex', 'Boost',
     ['boost::unordered_flat_map']),
    ('assembly', 'ADS::assembleCsc', 'std', ['std::map']),
    ('assembly', 'ADS::assembleCsc', 'Eigen', ['Eigen::setFromTriplets']),
    ('assembly', 'ADS::DynamicCsr', 'std', ['std::vector rows']),
    ('fingerprints', 'ADS::rowFingerprint', 'std', ['std::hash']),
    ('fingerprints', 'ADS::rowFingerprint', 'Boost', ['boost::hash_range']),
    ('fingerprints', 'ADS::sameRow', 'std', ['std::equal']),
    ('bignum', 'ADS::BigInteger', 'Boost',
     ['boost::multiprecision::cpp_int']),
    ('bignum', 'ADS::BigInteger', 'GMP', ['GMP mpz_class']),
    ('bignum', 'ADS::Rational', 'Boost', ['boost::rational<cpp_int>']),
    ('bignum', 'ADS::Rational', 'GMP', ['GMP mpq_class']),
    ('bignum', 'ADS::BigInteger own', 'GMP', ['GMP mpz_class']),
    ('bignum', 'ADS::Rational own', 'GMP', ['GMP mpq_class']),
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
    ('hash maps', 'ADS::DenseHashMap', 'std', ['std::unordered_map']),
    ('hash maps', 'ADS::DenseHashMap', 'unordered_dense',
     ['ankerl::unordered_dense::map']),
    ('hash maps', 'ADS::BucketCuckooTable', 'libcuckoo',
     ['libcuckoo::cuckoohash_map']),
    ('filters', 'ADS::BlockedBloomFilter', 'fastfilter',
     ['fastfilter::SimdBlockFilter']),
    ('filters', 'ADS::BinaryFuseFilter 8', 'xor_singleheader',
     ['xorfilter::binary_fuse8']),
    ('filters', 'ADS::BinaryFuseFilter 16', 'xor_singleheader',
     ['xorfilter::binary_fuse16']),
    ('filters', 'ADS::CuckooFilter 8', 'fastfilter',
     ['fastfilter::CuckooFilter 8']),
    ('filters', 'ADS::CuckooFilter 16', 'fastfilter',
     ['fastfilter::CuckooFilter 16']),
    # the prefix filter against the cuckoo filter of the same false
    # positives only: the blocked filter of Bloom answers by one line too,
    # with more false positives at the same bits per key
    ('filters', 'ADS::PrefixFilter', 'fastfilter',
     ['fastfilter::CuckooFilter 8']),
    ('filters', 'ADS::RibbonFilter 8', 'fastfilter',
     ['fastfilter::StandardRibbon 8']),
    ('filters', 'ADS::RibbonFilter 16', 'fastfilter',
     ['fastfilter::StandardRibbon 16']),
    ('perfect hashing', 'ADS::MinimalPerfectHash', 'PTHash', ['PTHash C']),
    ('perfect hashing', 'ADS::MinimalPerfectHash', 'PHOBIC',
     ['PHOBIC R-int']),
    ('perfect hashing', 'ADS::MinimalPerfectHash', 'BBHash', ['BBHash g=2']),
    ('perfect hashing', 'ADS::FingerprintPerfectHash g=2', 'BBHash',
     ['BBHash g=2']),
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
    ('gemm', 'ADS::gemm own', 'OpenBLAS', ['OpenBLAS']),
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
    ('bitvector', 'ADS::RankSelect lazy', 'sux',
     ['sux::Rank9Sel+SimpleSelect', 'sux::Rank9Sel']),
    ('bitvector', 'ADS::RankSelect lazy', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics',
      'pasta::WideRankSelect']),
    ('bitvector', 'ADS::CompactRankSelect', 'sdsl',
     ['sdsl::rank_support_v5']),
    ('bitvector', 'ADS::CompactRankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics']),
    ('bitvector', 'ADS::InterleavedRankSelect', 'sdsl',
     ['sdsl::rank_support_v5']),
    ('bitvector', 'ADS::InterleavedRankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics']),
    ('rrr', 'ADS::RRRBitVector', 'sdsl', ['sdsl::rrr_vector<63>']),
    ('eliasfano', 'ADS::EliasFano', 'sdsl', ['sdsl::sd_vector']),
    ('eliasfano', 'ADS::EliasFano', 'sux', ['sux::EliasFano']),
    ('eliasfano', 'ADS::EliasFano', 'succinct', ['succinct::elias_fano']),
    ('eliasfano', 'ADS::EliasFano lazy', 'sux', ['sux::EliasFano']),
    ('eliasfano', 'ADS::PartitionedEliasFano', 'sdsl', ['sdsl::sd_vector']),
    ('eliasfano', 'ADS::PartitionedEliasFano', 'sux', ['sux::EliasFano']),
    ('eliasfano', 'ADS::PartitionedEliasFano', 'succinct',
     ['succinct::elias_fano']),
    ('parentheses', 'ADS::BalancedParentheses', 'sdsl',
     ['sdsl::bp_support_sada']),
    ('parentheses', 'ADS::BalancedParentheses', 'succinct',
     ['succinct::bp_vector']),
    ('wavelet', 'ADS::WaveletMatrix', 'sdsl',
     ['sdsl::wm_int', 'sdsl::wt_int']),
    ('wavelet', 'ADS::HuffmanWaveletTree', 'sdsl', ['sdsl::wt_huff_int']),
    ('wavelet', 'ADS::HuffmanWaveletTree compact', 'sdsl',
     ['sdsl::wt_huff_int']),
    ('dac', 'ADS::DirectAccessCodes', 'sdsl',
     ['sdsl::dac_vector<4>', 'sdsl::dac_vector<8>']),
    ('dac', 'ADS::DirectAccessCodes b=8', 'sdsl', ['sdsl::dac_vector<8>']),
    ('entropy', 'ADS::huffmanCompress', 'huff0', ['huff0']),
    ('entropy', 'ADS::ransCompress', 'ryg_rans', ['ryg_rans']),
    ('entropy', 'ADS::tansCompress', 'FSE', ['FSE']),
    ('integers', 'ADS::streamVByteEncode', 'Stream VByte', ['streamvbyte']),
    ('integers', 'ADS::binaryPack', 'FastPFor',
     ['FastPFor::simdbinarypacking']),
    ('integers', 'ADS::patchedPack', 'FastPFor',
     ['FastPFor::simdfastpfor128']),
    ('integers', 'ADS::encodeGamma', 'sdsl', ['sdsl::elias_gamma']),
    ('integers', 'ADS::encodeDelta', 'sdsl', ['sdsl::elias_delta']),
    ('bwt', 'ADS::bwt', 'libsais', ['libsais']),
    ('rindex', 'ADS::RIndex', 'Prezza', ['ri::r_index']),
    ('rindex', 'ADS::MoveRIndex', 'Prezza', ['ri::r_index']),
    ('rindex', 'ADS::RIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
    ('fmindex', 'ADS::FMIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
    ('fmindex', 'ADS::HuffmanFMIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
    ('fmindex', 'ADS::HuffmanFMIndex compact', 'sdsl',
     ['sdsl::csa_wt<wt_huff>']),
    ('lz', 'ADS::lzssParseHashed', 'LZ4', ['LZ4_compress_default']),
    ('lz', 'ADS::lzssDecode', 'LZ4', ['LZ4_decompress_safe']),
    # the bits, against the structures of the same queries
    ('space', 'ADS::binaryPack', 'FastPFor',
     ['FastPFor::simdbinarypacking']),
    ('space', 'ADS::patchedPack', 'FastPFor',
     ['FastPFor::simdfastpfor128']),
    ('space', 'ADS::RankSelect', 'sdsl', ['sdsl::rank_support_v+mcl']),
    ('space', 'ADS::RankSelect', 'sux', ['sux::Rank9Sel+SimpleSelect']),
    ('space', 'ADS::RankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics',
      'pasta::WideRankSelect']),
    ('space', 'ADS::RankSelect', 'succinct', ['succinct::rs_bit_vector']),
    ('space', 'ADS::CompactRankSelect', 'sdsl', ['sdsl::rank_support_v5']),
    ('space', 'ADS::CompactRankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics']),
    ('space', 'ADS::InterleavedRankSelect', 'sdsl',
     ['sdsl::rank_support_v5']),
    ('space', 'ADS::InterleavedRankSelect', 'pasta',
     ['pasta::FlatRankSelect', 'pasta::FlatRankSelect intrinsics']),
    ('space', 'ADS::RRRBitVector', 'sdsl', ['sdsl::rrr_vector<63>']),
    ('space', 'ADS::EliasFano', 'sdsl', ['sdsl::sd_vector']),
    ('space', 'ADS::EliasFano', 'sux', ['sux::EliasFano']),
    ('space', 'ADS::EliasFano', 'succinct', ['succinct::elias_fano']),
    ('space', 'ADS::PartitionedEliasFano', 'sdsl', ['sdsl::sd_vector']),
    ('space', 'ADS::PartitionedEliasFano', 'sux', ['sux::EliasFano']),
    ('space', 'ADS::PartitionedEliasFano', 'succinct',
     ['succinct::elias_fano']),
    ('space', 'ADS::BalancedParentheses', 'sdsl', ['sdsl::bp_support_sada']),
    ('space', 'ADS::BalancedParentheses', 'succinct', ['succinct::bp_vector']),
    ('space', 'ADS::WaveletMatrix', 'sdsl', ['sdsl::wm_int', 'sdsl::wt_int']),
    ('space', 'ADS::HuffmanWaveletTree', 'sdsl', ['sdsl::wt_huff_int']),
    ('space', 'ADS::HuffmanWaveletTree compact', 'sdsl',
     ['sdsl::wt_huff_int']),
    ('space', 'ADS::DirectAccessCodes', 'sdsl',
     ['sdsl::dac_vector<4>', 'sdsl::dac_vector<8>']),
    ('space', 'ADS::huffmanCompress', 'huff0', ['huff0']),
    ('space', 'ADS::ransCompress', 'ryg_rans', ['ryg_rans']),
    ('space', 'ADS::tansCompress', 'FSE', ['FSE']),
    ('space', 'ADS::streamVByteEncode', 'Stream VByte', ['streamvbyte']),
    ('space', 'ADS::RIndex', 'Prezza', ['ri::r_index']),
    ('space', 'ADS::MoveRIndex', 'Prezza', ['ri::r_index']),
    ('space', 'ADS::RIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
    ('space', 'ADS::FMIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
    ('multiway merge', 'ADS::multiwayMerge', 'std',
     ['std::priority_queue']),
    ('multiway merge', 'ADS::multiwayMerge', 'libstdc++',
     ['libstdc++ multiway_merge']),
    ('multiway merge', 'ADS::multiwayMerge', 'tlx',
     ['tlx::multiway_merge', 'tlx::stable_multiway_merge']),
    ('multiway merge', 'ADS::FlatLoserTree', 'libstdc++',
     ['libstdc++ multiway_merge']),
    ('multiway merge', 'ADS::FlatLoserTree', 'tlx',
     ['tlx::multiway_merge', 'tlx::stable_multiway_merge']),
    ('book mergesort', 'ADS::FlatLoserTree', 'std', ['std::priority_queue']),
    ('book mergesort', 'ADS::FlatLoserTree', 'Boost',
     ['boost::heap::d_ary_heap d=2', 'boost::heap::d_ary_heap d=4']),
    ('book mergesort', 'ADS::LoserTree', 'std', ['std::priority_queue']),
    ('book mergesort', 'ADS::LoserTree', 'Boost',
     ['boost::heap::d_ary_heap d=2', 'boost::heap::d_ary_heap d=4']),
    ('sampling', 'ADS::sampleBySkips', 'std', ['std::sample']),

    # the algorithms of the book on the structures of each library
    ('bfs', 'ADS StaticGraph ArrayQueue', 'std', ['std vector queue']),
    ('bfs', 'ADS StaticGraph ArrayQueue', 'Boost', ['Boost csr queue']),
    ('bfm', 'ADS StaticGraph ArrayQueue', 'std', ['std vector queue']),
    ('bfm', 'ADS StaticGraph ArrayQueue', 'Boost', ['Boost csr queue']),
    ('scc', 'ADS StaticGraph', 'std', ['std vector']),
    ('scc', 'ADS StaticGraph', 'Boost', ['Boost csr']),
    ('kruskal', 'ADS UnionFind', 'Boost', ['Boost disjoint_sets']),
    ('kruskal', 'ADS DisjointSets rank full', 'Boost',
     ['Boost disjoint_sets']),
    ('union-find', 'ADS UnionFind', 'Boost', ['Boost disjoint_sets']),
    ('union-find', 'ADS DisjointSets rank full', 'Boost',
     ['Boost disjoint_sets']),
    ('dijkstra', 'ADS StaticGraph IndexedDaryHeap<4>', 'std',
     ['std vector set']),
    ('dijkstra', 'ADS StaticGraph IndexedDaryHeap<4>', 'Boost',
     ['Boost csr d_ary_heap<4>', 'Boost csr d_ary_heap<2>',
      'Boost csr pairing_heap', 'Boost csr fibonacci_heap']),
    ('dijkstra', 'ADS StaticGraph HeapPriorityQueue<4>', 'Boost',
     ['Boost csr d_ary_heap<4>']),
    ('dijkstra', 'ADS StaticGraph HeapPriorityQueue<2>', 'Boost',
     ['Boost csr d_ary_heap<2>']),
    ('dijkstra', 'ADS StaticGraph PairingHeap', 'Boost',
     ['Boost csr pairing_heap']),
    ('dijkstra', 'ADS StaticGraph FibonacciHeapPriorityQueue', 'Boost',
     ['Boost csr fibonacci_heap']),
    ('prim', 'ADS StaticGraph IndexedDaryHeap<4>', 'std',
     ['std vector set']),
    ('prim', 'ADS StaticGraph IndexedDaryHeap<4>', 'Boost',
     ['Boost csr d_ary_heap<4>', 'Boost csr d_ary_heap<2>',
      'Boost csr pairing_heap', 'Boost csr fibonacci_heap']),
    ('prim', 'ADS StaticGraph HeapPriorityQueue<4>', 'Boost',
     ['Boost csr d_ary_heap<4>']),
    ('prim', 'ADS StaticGraph HeapPriorityQueue<2>', 'Boost',
     ['Boost csr d_ary_heap<2>']),
    ('prim', 'ADS StaticGraph PairingHeap', 'Boost',
     ['Boost csr pairing_heap']),
    ('prim', 'ADS StaticGraph FibonacciHeapPriorityQueue', 'Boost',
     ['Boost csr fibonacci_heap']),
    ('perfect hashing', 'ADS::RecSplit l=8 b=100', 'RecSplit',
     ['RecSplit l=8 b=100']),
    ('perfect hashing', 'ADS::RecSplit l=8 b=100', 'BBHash',
     ['BBHash g=2']),
    ('concurrent maps', 'ADS::ConcurrentCuckooTable', 'libcuckoo',
     ['libcuckoo::cuckoohash_map']),
    ('concurrent maps', 'ADS::ConcurrentCuckooTable', 'Boost',
     ['boost::concurrent_flat_map']),
    ('concurrent maps', 'ADS::ConcurrentCuckooTable', 'std',
     ['std::unordered_map std::shared_mutex']),
    ('popcount', 'ADS::popcount', 'std', ['std::popcount']),
    ('popcount', 'ADS::popcount', 'Boost', ['boost::dynamic_bitset']),
    ('popcount', 'ADS::popcount', 'libpopcnt', ['libpopcnt']),
    ('popcount', 'ADS::popcountAnd', 'std', ['std::popcount']),
    ('morton', 'ADS::mortonEncode', 'libmorton', ['libmorton']),
    ('morton', 'ADS::mortonDecode', 'libmorton', ['libmorton']),
    ('permutations', 'ADS::forEachPermutation', 'std',
     ['std::next_permutation']),
    ('permutations', 'ADS::forEachPermutation', 'Hinnant',
     ['hinnant::for_each_permutation']),
    ('permutations', 'ADS::forEachPermutationLex', 'std',
     ['std::next_permutation']),
    ('combinations', 'ADS::forEachCombination', 'std',
     ['std::prev_permutation']),
    ('combinations', 'ADS::forEachCombination', 'Hinnant',
     ['hinnant::for_each_combination']),
    ('combinations', 'ADS::forEachCombinationMask', 'Hinnant',
     ['hinnant::for_each_combination']),
    ('exact cover', 'ADS::FastExactCover', 'Knuth',
     ['knuth::dlx1', 'knuth::dlx2', 'knuth::ssxcc']),
    ('book cdcl', 'ADS::DaryHeap d=4', 'std', ['std::priority_queue']),
    ('book cdcl', 'ADS::DaryHeap d=4', 'Boost',
     ['boost::heap::d_ary_heap d=4', 'boost::heap::pairing_heap']),
    ('book a-star', 'ADS::FlatHashTable', 'std',
     ['std::map', 'std::unordered_map']),
    ('book a-star', 'ADS::FlatHashTable', 'Boost',
     ['boost::unordered_flat_map']),
    ('book subset sum', 'ADS::BitVector', 'std', ['std::vector<bool>']),
    ('book subset sum', 'ADS::BitVector', 'Boost', ['boost::dynamic_bitset']),
    ('ordered', 'ADS::BTreeMap', 'std', ['std::map']),
    ('ordered', 'ADS::BTreeMap', 'Boost',
     ['boost::intrusive::set', 'boost::intrusive::avl_set',
      'boost::intrusive::sg_set', 'boost::intrusive::splay_set',
      'boost::intrusive::treap_set']),
    ('ordered', 'ADS::BTreeMap', 'Abseil', ['absl::btree_map']),
    ('ordered', 'ADS::BTreeMap', 'tlx', ['tlx::btree_map']),
    ('ordered', 'ADS::SortedArrayDictionary', 'Boost',
     ['boost::container::flat_map']),
    ('array search', 'ADS::SPlusTree', 'std', ['std::lower_bound']),
    ('array search', 'ADS::SPlusTree', 'Khuong-Morin', ['Eytzinger']),
    ('array search', 'ADS::PGMIndex', 'PGM', ['pgm::PGMIndex']),
    ('select', 'ADS::nthElement', 'std', ['std::nth_element']),
    ('select', 'ADS::nthElement', 'miniselect',
     ['miniselect::floyd_rivest_select', 'miniselect::pdqselect',
      'miniselect::median_of_ninthers']),
    ('rmq', 'ADS::WordRangeMinimum', 'sdsl',
     ['sdsl::rmq_succinct_sct', 'sdsl::rmq_succinct_sada',
      'sdsl::rmq_support_sparse_table']),
    ('intersection', 'ADS::intersect', 'std', ['std::set_intersection']),
    ('intersection', 'ADS::intersect', 'Lemire',
     ['Lemire SIMDintersection']),
    ('mfset', 'ADS::RemUnionFind', 'Boost', ['boost::disjoint_sets']),
    ('mfset', 'ADS::UnionFind', 'Boost', ['boost::disjoint_sets']),
    ('boolset', 'ADS::BoolSet', 'Boost', ['boost::dynamic_bitset']),
    ('boolset', 'ADS::BoolSet', 'Roaring', ['Roaring']),
    ('bitmap', 'ADS::RoaringBitmap', 'Roaring', ['Roaring']),
    ('tries', 'ADS::AdaptiveRadixTree', 'std',
     ['std::map', 'std::unordered_map']),
    ('tries', 'ADS::AdaptiveRadixTree', 'Abseil', ['absl::btree_map']),
    ('tries', 'ADS::AdaptiveRadixTree', 'tsl', ['tsl::htrie_map']),
    ('tries', 'ADS::AdaptiveRadixTree', 'libart', ['libart']),
    ('space', 'ADS::HuffmanFMIndex', 'sdsl', ['sdsl::csa_wt<wt_huff>']),
    ('space', 'ADS::HuffmanFMIndex compact', 'sdsl',
     ['sdsl::csa_wt<wt_huff>']),
]

# the parallel algorithms [see bench/parallel.cpp], on the pool of ADS
# against the parallel libraries and against the same algorithm on the
# other executors; a benchmark "parallel <algorithm> t<threads>" is a
# group of its own for each number of threads
PARALLEL = [
    ('sort uint64', 'ADS::sort', [
        ('oneTBB', ['tbb::parallel_sort']),
        ('std::execution', ['std::sort(par)']),
        ('GNU parallel', ['__gnu_parallel::sort']),
        ('IPS4o', ['ips4o::parallel::sort', 'ips2ra::parallel::sort']),
        ('ParlayLib', ['parlay::sort_inplace',
                       'parlay::integer_sort_inplace']),
        ('FastFlow', ['ff_DC mergesort (ADS leaves)'])]),
    ('sort double', 'ADS::sort', [
        ('oneTBB', ['tbb::parallel_sort']),
        ('std::execution', ['std::sort(par)']),
        ('GNU parallel', ['__gnu_parallel::sort']),
        ('IPS4o', ['ips4o::parallel::sort']),
        ('ParlayLib', ['parlay::sort_inplace']),
        ('FastFlow', ['ff_DC mergesort (ADS leaves)'])]),
    ('sort string', 'ADS::sort', [
        ('oneTBB', ['tbb::parallel_sort']),
        ('std::execution', ['std::sort(par)']),
        ('GNU parallel', ['__gnu_parallel::sort']),
        ('IPS4o', ['ips4o::parallel::sort']),
        ('ParlayLib', ['parlay::sort_inplace']),
        ('FastFlow', ['ff_DC mergesort (ADS leaves)'])]),
    ('stableSort uint64', 'ADS::stableSort', [
        ('std::execution', ['std::stable_sort(par)']),
        ('GNU parallel', ['__gnu_parallel::stable_sort']),
        ('ParlayLib', ['parlay::stable_sort_inplace',
                       'parlay::integer_sort_inplace']),
        ('FastFlow', ['ff_DC mergesort (ADS leaves)'])]),
    ('stableSort double', 'ADS::stableSort', [
        ('std::execution', ['std::stable_sort(par)']),
        ('GNU parallel', ['__gnu_parallel::stable_sort']),
        ('ParlayLib', ['parlay::stable_sort_inplace']),
        ('FastFlow', ['ff_DC mergesort (ADS leaves)'])]),
    ('stableSort string', 'ADS::stableSort', [
        ('std::execution', ['std::stable_sort(par)']),
        ('GNU parallel', ['__gnu_parallel::stable_sort']),
        ('ParlayLib', ['parlay::stable_sort_inplace']),
        ('FastFlow', ['ff_DC mergesort (ADS leaves)'])]),
    ('reduce', 'ADS::parallelReduce', [
        ('oneTBB', ['tbb::parallel_reduce']),
        ('std::execution', ['std::reduce(par)']),
        ('OpenMP', ['omp reduction']),
        ('ParlayLib', ['parlay::reduce']),
        ('FastFlow', ['ff::ParallelForReduce'])]),
    ('scan', 'ADS::parallelInclusiveScan', [
        ('oneTBB', ['tbb::parallel_scan']),
        ('std::execution', ['std::inclusive_scan(par)']),
        ('OpenMP', ['omp scan']),
        ('ParlayLib', ['parlay::scan_inclusive_inplace'])]),
    ('map', 'ADS::parallelForBlocks', [
        ('oneTBB', ['tbb::parallel_for']),
        ('std::execution', ['std::for_each(par)']),
        ('OpenMP', ['omp for']),
        ('ParlayLib', ['parlay::parallel_for']),
        ('FastFlow', ['ff::ParallelFor'])]),
    ('merge', 'ADS::multiwayMerge', [
        ('GNU parallel', ['__gnu_parallel::stable_multiway_merge']),
        ('tlx', ['tlx::stable_parallel_multiway_merge'])]),
    ('spmv', 'ADS::CsrMatrix', [
        ('oneTBB', ['tbb::parallel_for rows']),
        ('OpenMP', ['omp for rows']),
        ('Eigen', ['Eigen::SparseMatrix']),
        ('ParlayLib', ['parlay::parallel_for rows'])]),
    ('spmv', 'ADS::SellMatrix', [
        ('oneTBB', ['tbb::parallel_for rows']),
        ('OpenMP', ['omp for rows']),
        ('Eigen', ['Eigen::SparseMatrix']),
        ('ParlayLib', ['parlay::parallel_for rows'])]),
    ('gemm', 'ADS::gemm', [
        ('OpenBLAS', ['OpenBLAS dgemm']),
        ('Eigen', ['Eigen::Matrix'])]),
    ('bfs', 'ADS::bfsDistances', [
        ('GAPBS', ['gapbs DOBFS']),
        ('ParlayLib', ['parlay BFS_ligra'])]),
    ('cc', 'ADS::connectedComponents', [
        ('GAPBS', ['gapbs Afforest', 'gapbs ShiloachVishkin'])]),
    ('lcp', 'ADS::lcpArrayPhi', [('ParlayLib', ['parlay lcp'])]),
    ('sssp', 'ADS::deltaStepping', [
        ('GAPBS', ['gapbs DeltaStep']),
        ('GBBS', ['gbbs DeltaStepping', 'gbbs wBFS']),
        ('ParlayLib', ['parlay bucketed_dijkstra'])]),
    ('build wavelet matrix', 'ADS::WaveletMatrix', [
        ('sdsl', ['sdsl::wm_int']),
        ('pwm', ['pwm wm_pc', 'pwm wm_ppc', 'pwm wm_dd_pc'])]),
    ('build huffman wavelet tree', 'ADS::HuffmanWaveletTree', [
        ('sdsl', ['sdsl::wt_huff_int']),
        ('pwm', ['pwm wt_huff_pc', 'pwm wt_huff_dd_pc'])]),
    ('hash insert', 'ADS::ConcurrentCuckooTable', [
        ('oneTBB', ['tbb::concurrent_hash_map',
                    'tbb::concurrent_unordered_map']),
        ('libcuckoo', ['libcuckoo::cuckoohash_map']),
        ('ParlayLib', ['parlay::hashtable (keys only)'])]),
    ('hash find', 'ADS::ConcurrentCuckooTable', [
        ('oneTBB', ['tbb::concurrent_hash_map',
                    'tbb::concurrent_unordered_map']),
        ('libcuckoo', ['libcuckoo::cuckoohash_map']),
        ('ParlayLib', ['parlay::hashtable (keys only)'])]),
]
# the executors of ADS run the same algorithm: FastFlow, OpenMP and oneTBB
# against the pool, and the sequential algorithm (in the groups of 1
# thread); the tables are driven by the pool only
for b, ads, libs in PARALLEL:
    pool = ads if b.startswith('hash') else ads + ' pool'
    for lib, whos in libs:
        PAIRS.append(('parallel ' + b, pool, lib, whos))
    if b.startswith('hash'):
        continue
    for e in ['FastFlow', 'OpenMP', 'TBB']:
        PAIRS.append(('parallel ' + b, pool, 'ADS on ' + e,
                      [ads + ' ' + e]))
    PAIRS.append(('parallel ' + b, pool, 'ADS sequential',
                  [ads + ' (sequential)']))

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
