# ADS: Algorithms and Data Structures, and Advanced ones

[![CI](https://github.com/dmeoli/ADS/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/dmeoli/ADS/actions/workflows/ci.yml)
[![Coverage Status](https://coveralls.io/repos/github/dmeoli/ADS/badge.svg?branch=main)](https://coveralls.io/github/dmeoli/ADS?branch=main)
[![Documentation](https://img.shields.io/badge/docs-online-blue)](https://dmeoli.github.io/ADS/)
[![Manual](https://img.shields.io/badge/docs-Manual-blue)](https://dmeoli.github.io/ADS/manual/)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)
[![Header-only](https://img.shields.io/badge/header--only-yes-brightgreen)](https://github.com/dmeoli/ADS#use)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey)](https://github.com/dmeoli/ADS/actions/workflows/ci.yml)
[![License: BSL-1.0](https://img.shields.io/badge/License-BSL_1.0-blue.svg)](https://www.boost.org/LICENSE_1_0.txt)
[![Release](https://img.shields.io/github/v/release/dmeoli/ADS)](https://github.com/dmeoli/ADS/releases)

ADS: Algorithms and Data Structures, and Advanced ones. ADS is a header-only
C++20 library of the data structures and of the parallelism that the other
libraries lack. Its structures report their changes as typed modifications,
which observers follow incrementally, check their contracts at a level
chosen at compile time, and expose capabilities that the algorithms query
through concepts. It is meant to complement the standard library, Boost,
LEMON and GMP, leaving to them what they already do as well, and it runs its
parallelism on TBB, OpenMP or MPI.

ADS also realizes the algorithms and the data structures of the (Italian)
textbook

> A. Montresor, *Algoritmi e Strutture Dati*, 2025,
> <https://algoritmicamente.it> (CC BY-NC 4.0)

(a revision of A. A. Bertossi and A. Montresor, *Algoritmi e Strutture di
Dati*, 3rd edition, Città Studi, 2014), each exercise of which is a test,
together with the structures and the algorithms that a course on the subject
usually adds and a few engineered ones meant for speed. Each abstract data
type is a C++20 concept, whose realizations follow the pseudocode and the
names of the book (`empty()`, `head()`, `next()`, `read()`, `push()`,
`enqueue()`, and so on), and hence the code can be read side by side with
the text. Where the book leaves a choice open, or its pseudocode has a
defect (a few figures, too, disagree with the formulae they illustrate), the
header states the choice made and its reason.

## Performance

The table below compares the engineered structures of ADS with those of
the standard library (libstdc++), of Boost 1.83, of GMP 6.3.0 and of the
libraries specialized in each area, as measured by the benchmarks in
[`bench/`](bench/) (`make bench`). Among the latter are IPS4o and scandum
for the sorts, sdsl, sux and pasta for the succinct structures, and
libsais for the suffix arrays. `BigInteger` and `Rational` run on GMP, and
`gemm()` on a CBLAS, wherever the library is found (see the section Use),
and the benchmarks measure them on these backends; the rows marked `own`
in [`results/`](results/README.md) measure the code of the book that runs
without them. Conversely, where a build with Boost puts a structure on
Boost (e.g., `PointerTable` on `boost::unordered_flat_set`), the
benchmarks keep the code of ADS, and the rows of Boost give the time of
such a build.

All the benchmarks were run once, from 02:28 UTC of 22 November 2026 to
03:47 of the next day, on a shared AMD EPYC 9654 (Zen 4) with g++ 13.3 at
`-O3 -march=native`, with Highway (`ADS_WITH_HIGHWAY`) and, for the
parallel ones, hwloc. Each structure ran in a separate process pinned by
`taskset` to core 178 (the parallel ones to cores 184 to 191), its SMT
sibling left idle. The memory was bound by `numactl` to the NUMA node of
these cores, while other programs ran on the other node. A structure was
measured only after no other process had run on its cores for 5 minutes,
and measured again when one appeared during its run (32 times in this
run). The checks cover only the cores measured, and hence the third level
of the cache, which core 178 shares with 7 others, may have been shared
with other programs for part of the run. In each benchmark, a cell is the
median of 11 runs, and the clock does not include the building of the
input.

A row takes the structure of ADS that is compared with the most libraries
in its benchmark. It gives the time of the standard library divided by
that of ADS, and the same ratio for the strongest library, i.e., the one
of the smallest median (above 1 when ADS is faster). Both are the median
and the range over the cells of the benchmark, i.e., its cases (e.g.,
random, sorted or few distinct keys for the sorts) and its sizes ($10^5$
and $10^6$ items for the containers); when a library has more than one
candidate, a cell takes the fastest of them (e.g., the faster of IPS4o and
IPS2Ra). The first structure of each benchmark was run a second time (the
rows "A/A" of [`results/`](results/README.md)): over 2116 cells, the two
runs differ by 0.3% in the median and by 2.1% at the 90th percentile, and
the latter is the band $b$ of the noise used below. Larger differences, up
to 38%, are in cells below one nanosecond per item (the parallel sorts of
TBB on sorted keys) or of a few nanoseconds (the visits of Roaring), where
they are the resolution of the clock, and in the rank errors of
`MultiQueue`, which vary from run to run by their nature.

The column after each range is the verdict against this band. It is +
when the range lies above $1 + b$, i.e., ADS is faster than the noise in
every cell, and − when the range lies below $1 / ( 1 + b )$, i.e., ADS is
slower in every cell. It is ≈ when the median is between $1 / ( 1 + b )$
and $1 + b$, even if some cells fall outside, and ± otherwise, i.e., when
the median is outside the band but some cell is not (usually, ADS is then
faster in some cells and slower in others). Over the 879 comparisons of
[`results/`](results/README.md) (each structure of ADS against each
library of its benchmark), there are 402 +, 50 −, 170 ≈ and 257 ±.

<table>
<tr><th>benchmark</th><th>ADS</th><th>std: median</th><th>range</th><th></th><th>strongest</th><th>median</th><th>range</th><th></th></tr>
<tr><td>sort double</td><td><code>ADS::sort</code></td><td align="right">28</td><td align="right">14&nbsp;to&nbsp;73</td><td align="center">+</td><td>scandum</td><td align="right">3.20</td><td align="right">0.95&nbsp;to&nbsp;4.87</td><td align="center">±</td></tr>
<tr><td>sort int64</td><td><code>ADS::sort</code></td><td align="right">27</td><td align="right">9.37&nbsp;to&nbsp;66</td><td align="center">+</td><td>scandum</td><td align="right">2.55</td><td align="right">0.95&nbsp;to&nbsp;3.52</td><td align="center">±</td></tr>
<tr><td>sort record</td><td><code>ADS::sort</code></td><td align="right">5.89</td><td align="right">1.87&nbsp;to&nbsp;28</td><td align="center">+</td><td>IPS4o</td><td align="right">1.12</td><td align="right">0.71&nbsp;to&nbsp;12</td><td align="center">±</td></tr>
<tr><td>sort string</td><td><code>ADS::sort</code></td><td align="right">7.51</td><td align="right">5.00&nbsp;to&nbsp;9.76</td><td align="center">+</td><td>ska_sort</td><td align="right">2.34</td><td align="right">1.17&nbsp;to&nbsp;2.84</td><td align="center">+</td></tr>
<tr><td>sort uint32</td><td><code>ADS::sort</code></td><td align="right">40</td><td align="right">8.78&nbsp;to&nbsp;114</td><td align="center">+</td><td>vqsort</td><td align="right">2.41</td><td align="right">0.97&nbsp;to&nbsp;42</td><td align="center">±</td></tr>
<tr><td>dictionaries</td><td><code>ADS::FlatHashTable</code></td><td align="right">5.18</td><td align="right">2.71&nbsp;to&nbsp;7.98</td><td align="center">+</td><td>Boost</td><td align="right">0.93</td><td align="right">0.73&nbsp;to&nbsp;2.11</td><td align="center">±</td></tr>
<tr><td>priority</td><td><code>ADS::DaryHeap d=4</code></td><td align="right">1.10</td><td align="right">0.90&nbsp;to&nbsp;2.02</td><td align="center">±</td><td>Boost</td><td align="right">1.02</td><td align="right">0.98&nbsp;to&nbsp;2.07</td><td align="center">≈</td></tr>
<tr><td>sequences</td><td><code>ADS::ArraySequence</code></td><td align="right">1.03</td><td align="right">0.99&nbsp;to&nbsp;1.12</td><td align="center">±</td><td>Boost</td><td align="right">1.00</td><td align="right">0.95&nbsp;to&nbsp;1.52</td><td align="center">≈</td></tr>
<tr><td>memory</td><td><code>ADS::ObjectPool</code></td><td align="right">8.92</td><td align="right">3.62&nbsp;to&nbsp;14</td><td align="center">+</td><td>Boost</td><td align="right">1.68</td><td align="right">1.16&nbsp;to&nbsp;11</td><td align="center">+</td></tr>
<tr><td>allocators</td><td><code>ADS::PoolAllocator concurrent</code></td><td align="right">1.61</td><td align="right">1.24&nbsp;to&nbsp;1.90</td><td align="center">+</td><td>mimalloc</td><td align="right">0.97</td><td align="right">0.81&nbsp;to&nbsp;1.54</td><td align="center">±</td></tr>
<tr><td>positions</td><td><code>ADS::PositionIndex</code></td><td align="right">1.52</td><td align="right">0.73&nbsp;to&nbsp;2.43</td><td align="center">±</td><td>Boost</td><td align="right">1.07</td><td align="right">1.02&nbsp;to&nbsp;1.72</td><td align="center">±</td></tr>
<tr><td>assembly</td><td><code>ADS::assembleCsc</code></td><td align="right">27</td><td align="right">20&nbsp;to&nbsp;48</td><td align="center">+</td><td>Eigen</td><td align="right">1.31</td><td align="right">1.00&nbsp;to&nbsp;1.68</td><td align="center">±</td></tr>
<tr><td>fingerprints</td><td><code>ADS::rowFingerprint</code></td><td align="right">5.55</td><td align="right">3.18&nbsp;to&nbsp;8.41</td><td align="center">+</td><td>Boost</td><td align="right">7.01</td><td align="right">1.89&nbsp;to&nbsp;9.87</td><td align="center">+</td></tr>
<tr><td>bignum</td><td><code>ADS::BigInteger</code></td><td></td><td></td><td></td><td>GMP</td><td align="right">1.00</td><td align="right">0.79&nbsp;to&nbsp;1.12</td><td align="center">≈</td></tr>
<tr><td>hash functions</td><td><code>ADS::FastHash</code></td><td align="right">2.44</td><td align="right">1.94&nbsp;to&nbsp;6.11</td><td align="center">+</td><td>Abseil</td><td align="right">0.97</td><td align="right">0.82&nbsp;to&nbsp;1.73</td><td align="center">±</td></tr>
<tr><td>hash maps</td><td><code>ADS::FlatHashTable</code></td><td align="right">4.37</td><td align="right">1.69&nbsp;to&nbsp;22</td><td align="center">+</td><td>unordered_dense</td><td align="right">1.00</td><td align="right">0.07&nbsp;to&nbsp;2.03</td><td align="center">≈</td></tr>
<tr><td>filters</td><td><code>ADS::CuckooFilter 8</code></td><td></td><td></td><td></td><td>fastfilter</td><td align="right">1.89</td><td align="right">1.28&nbsp;to&nbsp;3.35</td><td align="center">+</td></tr>
<tr><td>perfect hashing</td><td><code>ADS::MinimalPerfectHash</code></td><td></td><td></td><td></td><td>PTHash</td><td align="right">1.38</td><td align="right">1.31&nbsp;to&nbsp;1.45</td><td align="center">+</td></tr>
<tr><td>search</td><td><code>ADS::StringSearcher</code></td><td align="right">4.76</td><td align="right">0.28&nbsp;to&nbsp;78</td><td align="center">±</td><td>libc</td><td align="right">3.41</td><td align="right">0.31&nbsp;to&nbsp;31</td><td align="center">±</td></tr>
<tr><td>suffix array</td><td><code>ADS::suffixArray</code></td><td></td><td></td><td></td><td>libsais</td><td align="right">1.21</td><td align="right">0.86&nbsp;to&nbsp;12</td><td align="center">±</td></tr>
<tr><td>lcp</td><td><code>ADS::lcpArrayPhi</code></td><td></td><td></td><td></td><td>libsais</td><td align="right">1.08</td><td align="right">0.77&nbsp;to&nbsp;1.18</td><td align="center">±</td></tr>
<tr><td>edit distance</td><td><code>ADS::levenshtein</code></td><td></td><td></td><td></td><td>rapidfuzz</td><td align="right">1.63</td><td align="right">0.97&nbsp;to&nbsp;141</td><td align="center">±</td></tr>
<tr><td>lcs</td><td><code>ADS::lcsLength</code></td><td></td><td></td><td></td><td>rapidfuzz</td><td align="right">1.30</td><td align="right">1.12&nbsp;to&nbsp;1.39</td><td align="center">+</td></tr>
<tr><td>approximate search</td><td><code>ADS::approximateMatches</code></td><td></td><td></td><td></td><td>edlib</td><td align="right">1.39</td><td align="right">1.01&nbsp;to&nbsp;1.81</td><td align="center">±</td></tr>
<tr><td>gcd</td><td><code>ADS::binaryGcd</code></td><td align="right">1.41</td><td align="right">0.71&nbsp;to&nbsp;2.12</td><td align="center">±</td><td>Boost</td><td align="right">1.12</td><td align="right">1.08&nbsp;to&nbsp;1.17</td><td align="center">+</td></tr>
<tr><td>powmod</td><td><code>ADS::powMod</code></td><td></td><td></td><td></td><td>hurchalla</td><td align="right">0.87</td><td align="right">0.86&nbsp;to&nbsp;0.95</td><td align="center">−</td></tr>
<tr><td>isprime</td><td><code>ADS::isPrime</code></td><td></td><td></td><td></td><td>hurchalla</td><td align="right">0.92</td><td align="right">0.91&nbsp;to&nbsp;0.94</td><td align="center">−</td></tr>
<tr><td>factorize</td><td><code>ADS::factorize</code></td><td></td><td></td><td></td><td>hurchalla</td><td align="right">0.91</td><td align="right">0.88&nbsp;to&nbsp;1.00</td><td align="center">±</td></tr>
<tr><td>generators</td><td><code>ADS::Xoshiro256</code></td><td align="right">2.23</td><td align="right">2.16&nbsp;to&nbsp;2.30</td><td align="center">+</td><td>PCG</td><td align="right">1.18</td><td align="right">1.14&nbsp;to&nbsp;1.22</td><td align="center">+</td></tr>
<tr><td>discrete</td><td><code>ADS::AliasTable</code></td><td align="right">12</td><td align="right">6.78&nbsp;to&nbsp;17</td><td align="center">+</td><td>Boost</td><td align="right">1.97</td><td align="right">1.87&nbsp;to&nbsp;2.05</td><td align="center">+</td></tr>
<tr><td>bounded</td><td><code>ADS::uniformBelow</code></td><td align="right">1.12</td><td align="right">1.12&nbsp;to&nbsp;1.12</td><td align="center">+</td><td>Boost</td><td align="right">1.76</td><td align="right">1.75&nbsp;to&nbsp;1.77</td><td align="center">+</td></tr>
<tr><td>shuffle</td><td><code>ADS::shuffleFast</code></td><td align="right">1.18</td><td align="right">1.10&nbsp;to&nbsp;1.26</td><td align="center">+</td><td></td><td></td><td></td><td></td></tr>
<tr><td>sampling</td><td><code>ADS::sampleBySkips</code></td><td align="right">66</td><td align="right">7.50&nbsp;to&nbsp;9860</td><td align="center">+</td><td></td><td></td><td></td><td></td></tr>
<tr><td>queue</td><td><code>ADS::RingDeque</code></td><td align="right">1.57</td><td align="right">1.41&nbsp;to&nbsp;1.58</td><td align="center">+</td><td>Boost</td><td align="right">1.52</td><td align="right">1.00&nbsp;to&nbsp;2.11</td><td align="center">±</td></tr>
<tr><td>deque</td><td><code>ADS::RingDeque</code></td><td align="right">1.09</td><td align="right">1.09&nbsp;to&nbsp;1.10</td><td align="center">+</td><td>Boost</td><td align="right">0.96</td><td align="right">0.96&nbsp;to&nbsp;0.96</td><td align="center">−</td></tr>
<tr><td>stack</td><td><code>ADS::RingDeque</code></td><td align="right">0.95</td><td align="right">0.95&nbsp;to&nbsp;0.95</td><td align="center">−</td><td>Boost</td><td align="right">2.17</td><td align="right">1.97&nbsp;to&nbsp;2.37</td><td align="center">+</td></tr>
<tr><td>spsc</td><td><code>ADS::SPSCQueue</code></td><td></td><td></td><td></td><td>rigtorp</td><td align="right">0.94</td><td align="right">0.69&nbsp;to&nbsp;1.05</td><td align="center">±</td></tr>
<tr><td>mpsc</td><td><code>ADS::MPSCInbox</code></td><td></td><td></td><td></td><td>moodycamel</td><td align="right">0.34</td><td align="right">0.30&nbsp;to&nbsp;0.38</td><td align="center">−</td></tr>
<tr><td>hull</td><td><code>ADS::convexHull</code></td><td></td><td></td><td></td><td>CGAL</td><td align="right">4.56</td><td align="right">2.93&nbsp;to&nbsp;8.44</td><td align="center">+</td></tr>
<tr><td>gemm</td><td><code>ADS::gemm</code></td><td></td><td></td><td></td><td>OpenBLAS</td><td align="right">1.00</td><td align="right">1.00&nbsp;to&nbsp;1.01</td><td align="center">≈</td></tr>
<tr><td>transpose</td><td><code>ADS::transpose</code></td><td></td><td></td><td></td><td>Eigen</td><td align="right">1.78</td><td align="right">0.96&nbsp;to&nbsp;2.88</td><td align="center">±</td></tr>
<tr><td>spmv</td><td><code>ADS::CsrMatrix</code></td><td></td><td></td><td></td><td>Eigen</td><td align="right">1.10</td><td align="right">0.92&nbsp;to&nbsp;1.20</td><td align="center">±</td></tr>
<tr><td>sptranspose</td><td><code>ADS::CsrMatrix</code></td><td></td><td></td><td></td><td>Eigen</td><td align="right">1.03</td><td align="right">0.72&nbsp;to&nbsp;1.72</td><td align="center">±</td></tr>
<tr><td>bitvector</td><td><code>ADS::RankSelect</code></td><td></td><td></td><td></td><td>sdsl</td><td align="right">1.13</td><td align="right">0.19&nbsp;to&nbsp;27</td><td align="center">±</td></tr>
<tr><td>rrr</td><td><code>ADS::RRRBitVector</code></td><td></td><td></td><td></td><td>sdsl</td><td align="right">1.24</td><td align="right">0.83&nbsp;to&nbsp;7.63</td><td align="center">±</td></tr>
<tr><td>eliasfano</td><td><code>ADS::EliasFano</code></td><td></td><td></td><td></td><td>sux</td><td align="right">0.91</td><td align="right">0.72&nbsp;to&nbsp;8.23</td><td align="center">±</td></tr>
<tr><td>parentheses</td><td><code>ADS::BalancedParentheses</code></td><td></td><td></td><td></td><td>succinct</td><td align="right">1.33</td><td align="right">1.01&nbsp;to&nbsp;16</td><td align="center">±</td></tr>
<tr><td>wavelet</td><td><code>ADS::WaveletMatrix</code></td><td></td><td></td><td></td><td>sdsl</td><td align="right">1.27</td><td align="right">0.89&nbsp;to&nbsp;11</td><td align="center">±</td></tr>
<tr><td>dac</td><td><code>ADS::DirectAccessCodes</code></td><td></td><td></td><td></td><td>sdsl</td><td align="right">0.52</td><td align="right">0.47&nbsp;to&nbsp;0.56</td><td align="center">−</td></tr>
<tr><td>entropy</td><td><code>ADS::huffmanCompress</code></td><td></td><td></td><td></td><td>huff0</td><td align="right">0.96</td><td align="right">0.87&nbsp;to&nbsp;1.36</td><td align="center">±</td></tr>
<tr><td>integers</td><td><code>ADS::binaryPack</code></td><td></td><td></td><td></td><td>FastPFor</td><td align="right">0.97</td><td align="right">0.82&nbsp;to&nbsp;13</td><td align="center">±</td></tr>
<tr><td>bwt</td><td><code>ADS::bwt</code></td><td></td><td></td><td></td><td>libsais</td><td align="right">0.85</td><td align="right">0.72&nbsp;to&nbsp;0.87</td><td align="center">−</td></tr>
<tr><td>rindex</td><td><code>ADS::RIndex</code></td><td></td><td></td><td></td><td>sdsl</td><td align="right">2.36</td><td align="right">0.21&nbsp;to&nbsp;16</td><td align="center">±</td></tr>
<tr><td>fmindex</td><td><code>ADS::FMIndex</code></td><td></td><td></td><td></td><td>sdsl</td><td align="right">1.49</td><td align="right">0.50&nbsp;to&nbsp;3.29</td><td align="center">±</td></tr>
<tr><td>lz</td><td><code>ADS::lzssParseHashed</code></td><td></td><td></td><td></td><td>LZ4</td><td align="right">0.07</td><td align="right">0.06&nbsp;to&nbsp;0.10</td><td align="center">−</td></tr>
<tr><td>space</td><td><code>ADS::RankSelect</code></td><td></td><td></td><td></td><td>pasta</td><td align="right">0.68</td><td align="right">0.66&nbsp;to&nbsp;0.70</td><td align="center">−</td></tr>
<tr><td>multiway merge</td><td><code>ADS::multiwayMerge</code></td><td align="right">2.39</td><td align="right">1.87&nbsp;to&nbsp;3.30</td><td align="center">+</td><td>tlx</td><td align="right">1.72</td><td align="right">0.96&nbsp;to&nbsp;2.03</td><td align="center">±</td></tr>
<tr><td>book mergesort</td><td><code>ADS::FlatLoserTree</code></td><td align="right">1.25</td><td align="right">1.17&nbsp;to&nbsp;1.30</td><td align="center">+</td><td>Boost</td><td align="right">1.11</td><td align="right">1.07&nbsp;to&nbsp;1.16</td><td align="center">+</td></tr>
<tr><td>bfs</td><td><code>ADS StaticGraph ArrayQueue</code></td><td align="right">1.05</td><td align="right">0.81&nbsp;to&nbsp;1.65</td><td align="center">±</td><td>Boost</td><td align="right">0.98</td><td align="right">0.80&nbsp;to&nbsp;1.02</td><td align="center">±</td></tr>
<tr><td>bfm</td><td><code>ADS StaticGraph ArrayQueue</code></td><td align="right">1.05</td><td align="right">0.93&nbsp;to&nbsp;1.05</td><td align="center">±</td><td>Boost</td><td align="right">0.93</td><td align="right">0.92&nbsp;to&nbsp;1.05</td><td align="center">±</td></tr>
<tr><td>scc</td><td><code>ADS StaticGraph</code></td><td align="right">1.04</td><td align="right">1.02&nbsp;to&nbsp;1.10</td><td align="center">±</td><td>Boost</td><td align="right">1.06</td><td align="right">1.01&nbsp;to&nbsp;1.10</td><td align="center">±</td></tr>
<tr><td>kruskal</td><td><code>ADS UnionFind</code></td><td></td><td></td><td></td><td>Boost</td><td align="right">1.02</td><td align="right">1.00&nbsp;to&nbsp;1.07</td><td align="center">≈</td></tr>
<tr><td>union-find</td><td><code>ADS UnionFind</code></td><td></td><td></td><td></td><td>Boost</td><td align="right">1.10</td><td align="right">1.09&nbsp;to&nbsp;1.10</td><td align="center">+</td></tr>
<tr><td>dijkstra</td><td><code>ADS StaticGraph IndexedDaryHeap<4></code></td><td align="right">2.79</td><td align="right">1.25&nbsp;to&nbsp;4.73</td><td align="center">+</td><td>Boost</td><td align="right">1.51</td><td align="right">0.99&nbsp;to&nbsp;1.93</td><td align="center">±</td></tr>
<tr><td>prim</td><td><code>ADS StaticGraph IndexedDaryHeap<4></code></td><td align="right">4.41</td><td align="right">2.22&nbsp;to&nbsp;6.14</td><td align="center">+</td><td>Boost</td><td align="right">1.55</td><td align="right">0.92&nbsp;to&nbsp;2.17</td><td align="center">±</td></tr>
<tr><td>concurrent maps</td><td><code>ADS::ConcurrentCuckooTable</code></td><td align="right">2.68</td><td align="right">1.51&nbsp;to&nbsp;9.46</td><td align="center">+</td><td>Boost</td><td align="right">0.91</td><td align="right">0.58&nbsp;to&nbsp;1.37</td><td align="center">±</td></tr>
<tr><td>popcount</td><td><code>ADS::popcount</code></td><td align="right">0.84</td><td align="right">0.84&nbsp;to&nbsp;0.84</td><td align="center">−</td><td>libpopcnt</td><td align="right">0.83</td><td align="right">0.82&nbsp;to&nbsp;0.85</td><td align="center">−</td></tr>
<tr><td>morton</td><td><code>ADS::mortonEncode</code></td><td></td><td></td><td></td><td>libmorton</td><td align="right">1.31</td><td align="right">1.07&nbsp;to&nbsp;1.55</td><td align="center">+</td></tr>
<tr><td>permutations</td><td><code>ADS::forEachPermutation</code></td><td align="right">2.12</td><td align="right">2.10&nbsp;to&nbsp;2.14</td><td align="center">+</td><td>Hinnant</td><td align="right">2.14</td><td align="right">2.11&nbsp;to&nbsp;2.16</td><td align="center">+</td></tr>
<tr><td>combinations</td><td><code>ADS::forEachCombination</code></td><td align="right">6.42</td><td align="right">6.21&nbsp;to&nbsp;33</td><td align="center">+</td><td>Hinnant</td><td align="right">2.23</td><td align="right">1.92&nbsp;to&nbsp;2.26</td><td align="center">+</td></tr>
<tr><td>exact cover</td><td><code>ADS::FastExactCover</code></td><td></td><td></td><td></td><td>Knuth</td><td align="right">0.99</td><td align="right">0.90&nbsp;to&nbsp;1.07</td><td align="center">≈</td></tr>
<tr><td>book cdcl</td><td><code>ADS::DaryHeap d=4</code></td><td align="right">1.01</td><td align="right">1.01&nbsp;to&nbsp;1.02</td><td align="center">≈</td><td>Boost</td><td align="right">0.99</td><td align="right">0.99&nbsp;to&nbsp;0.99</td><td align="center">≈</td></tr>
<tr><td>book a-star</td><td><code>ADS::FlatHashTable</code></td><td align="right">1.15</td><td align="right">1.15&nbsp;to&nbsp;1.15</td><td align="center">+</td><td>Boost</td><td align="right">0.98</td><td align="right">0.97&nbsp;to&nbsp;0.98</td><td align="center">±</td></tr>
<tr><td>book subset sum</td><td><code>ADS::BitVector</code></td><td align="right">1.60</td><td align="right">1.60&nbsp;to&nbsp;1.61</td><td align="center">+</td><td>Boost</td><td align="right">0.95</td><td align="right">0.95&nbsp;to&nbsp;0.95</td><td align="center">−</td></tr>
<tr><td>ordered</td><td><code>ADS::BTreeMap</code></td><td align="right">10</td><td align="right">3.09&nbsp;to&nbsp;77</td><td align="center">+</td><td>Abseil</td><td align="right">2.45</td><td align="right">0.97&nbsp;to&nbsp;7.17</td><td align="center">±</td></tr>
<tr><td>array search</td><td><code>ADS::SPlusTree</code></td><td align="right">6.30</td><td align="right">3.61&nbsp;to&nbsp;9.47</td><td align="center">+</td><td>Khuong-Morin</td><td align="right">2.41</td><td align="right">1.76&nbsp;to&nbsp;3.23</td><td align="center">+</td></tr>
<tr><td>select</td><td><code>ADS::nthElement</code></td><td align="right">6.34</td><td align="right">1.17&nbsp;to&nbsp;14</td><td align="center">+</td><td>miniselect</td><td align="right">1.51</td><td align="right">0.69&nbsp;to&nbsp;6.92</td><td align="center">±</td></tr>
<tr><td>rmq</td><td><code>ADS::WordRangeMinimum</code></td><td></td><td></td><td></td><td>sdsl</td><td align="right">1.15</td><td align="right">0.74&nbsp;to&nbsp;28</td><td align="center">±</td></tr>
<tr><td>intersection</td><td><code>ADS::intersect</code></td><td align="right">9.24</td><td align="right">2.26&nbsp;to&nbsp;68</td><td align="center">+</td><td>Lemire</td><td align="right">1.21</td><td align="right">0.80&nbsp;to&nbsp;2.28</td><td align="center">±</td></tr>
<tr><td>mfset</td><td><code>ADS::RemUnionFind</code></td><td></td><td></td><td></td><td>Boost</td><td align="right">1.02</td><td align="right">0.82&nbsp;to&nbsp;7.17</td><td align="center">≈</td></tr>
<tr><td>boolset</td><td><code>ADS::BoolSet</code></td><td></td><td></td><td></td><td>Roaring</td><td align="right">1.99</td><td align="right">1.59&nbsp;to&nbsp;14</td><td align="center">+</td></tr>
<tr><td>bitmap</td><td><code>ADS::RoaringBitmap</code></td><td></td><td></td><td></td><td>Roaring</td><td align="right">1.23</td><td align="right">0.32&nbsp;to&nbsp;11</td><td align="center">±</td></tr>
<tr><td>tries</td><td><code>ADS::AdaptiveRadixTree</code></td><td align="right">1.26</td><td align="right">0.80&nbsp;to&nbsp;4.67</td><td align="center">±</td><td>libart</td><td align="right">1.11</td><td align="right">0.77&nbsp;to&nbsp;1.59</td><td align="center">±</td></tr>
</table>

`ADS::sort()` is faster than `std::sort` in all the cells, by 5.9 to 40
times in the median of each type of key, the radix sorts accounting for
most of it. It is also faster than the strongest of the other libraries
in the median of each type, by 1.1 to 3.2 times (scandum on the 64-bit
numbers, vqsort on the 32-bit integers, ska_sort on the strings and IPS4o
on the records), although these tie or win on some cells (e.g., IPS4o is
up to 1.4 times faster on the records). In several other benchmarks the
engineered structures are ahead by 1.3 to 2.5 times in the median, e.g.,
the B-tree against that of Abseil, the S+-tree against that of Khuong and
Morin, the selection against miniselect and the multiway merge against
tlx. So are the cuckoo filters against fastfilter, `BoolSet` against
Roaring, the transposes and the assembly of sparse matrices against Eigen,
the edit distances against rapidfuzz and edlib, and the alias table, the
stack and the shortest paths and spanning trees against Boost. Also,
`intersect()` and `suffixArray()` are 1.21 times faster than the SIMD
intersections of Lemire and than libsais in the median (with cells down to
0.80 and 0.86, respectively), `MinimalPerfectHash` answers 1.38 times
faster than PTHash, and the convex hull is 4.6 times faster than that of
CGAL.

On the graphs, the union-find, the sequences, the priority queues, the
LCP arrays, the products of sparse matrices and the hash maps, ADS is
usually within 10% of the other libraries. For instance, `FlatHashTable`
ties `boost::unordered_flat_map` in the median of the hash maps (1.03),
with cells from 1.7 times slower to 2 times faster, and `FastHash` ties
the hash of Abseil (0.97). It is fair to remark, however, that ADS loses
where the other libraries have kernels engineered for one task. LZ4 parses
and decodes 12 to 14 times faster than the LZSS of ADS, and the queue of
moodycamel is 2.9 times faster than `MPSCInbox`; also, sdsl accesses its
direct access codes 1.9 times faster than `DirectAccessCodes` (which ties
it when given $b = 8$). The prefix filter of fastfilter
answers 1.5 times faster than `PrefixFilter`, libsais builds the BWT 1.2
times faster than `bwt()`, and the modular arithmetic of hurchalla is 1.09
to 1.15 times faster on `powMod()` and `isPrime()`. Finally,
`ConcurrentCuckooTable` is 1.1 times slower than the concurrent map of
Boost in the median, and, somewhat unexpectedly, `ADS::popcount()` is 1.2
times slower than `std::popcount` on arrays. The tables of all the
structures, those of the book included, are in
[`results/`](results/README.md) together with the raw file of the run.

## Layout

The headers are grouped by abstract data type, one header per realization.
In `include/ADS/ADT.h` are the concepts of the book, next to the contracts
(`Contract.h`), the iterators (`Iterators.h`), the portable intrinsics
(`Portable.h`), the typed modifications by which a structure notifies its
observers of a change (`Modification.h`) and the capabilities that an
algorithm may require of a structure (`Capabilities.h`). The folders of the
types also hold the algorithms on them (e.g., the operations on sets or the
visits of a graph):

- `sequence/`, the SEQUENCE and the LIST (arrays, linked lists, cursors);
- `stack/`, the STACK and the algorithms that use it;
- `queue/`, the QUEUE and the deque;
- `tree/`, the TREE, the binary trees, the search trees and the B-trees;
- `dictionary/`, the DICTIONARY (hash tables, perfect hashing, tries);
- `set/`, the SET, the operations on sets and the filters;
- `priority/`, the PRIORITY QUEUE (heaps, bucket and radix queues);
- `mfset/`, the MFSET, i.e., the merge-find sets;
- `graph/`, the GRAPH, its visits, paths, spanning trees and flows.

The other folders group the algorithms by technique or by problem:

- `search/`, the searches;
- `sort/`, the sorts;
- `numeric/`, the products of integers, polynomials and matrices;
- `string/`, the algorithms on strings;
- `optimization/`, the problems of optimization (knapsack, intervals);
- `geometry/`, the convex hull, the closest pair, the interval tree and
  the range tree;
- `backtrack/`, the enumerations by backtrack;
- `complexity/`, the problems of NP and their reductions;
- `intractable/`, the techniques for the intractable problems;
- `memory/`, the allocators, the pools and the arena;
- `concurrent/`, the queues between threads, the concurrent hash table
  and the concurrent union-find;
- `parallel/`, the executors, the parallel algorithms on threads and the
  runtime of the long-lived workers;
- `distributed/`, the sort and the runtime over the processes of MPI;
- `interop/parallel/`, the executors on the runtimes of a program;
- `external/`, the two-level memory and the algorithms on it;
- `compression/`, the codes, the compressors and the compressed indexes;
- `succinct/`, the compressed data structures;
- `combinatorial/`, the broadword tricks and the combinatorial generation;
- `algorithm/`, the rest.

A header is included by its path (e.g., `#include "ADS/graph/ListGraph.h"`)
and includes what it needs. It declares its classes and functions with their
contracts, while the definitions of its templates are in the folder `impl/`
next to it (e.g., `ADS/graph/impl/ListGraph.tpp`) and are included at the
end of the header, as in Boost. Thus, `include/` is the whole library.

## Base and advanced

By their origin, the headers are divided into two sets, a division that
changes no path. The base set holds what follows a textbook or an academic
source (Montresor, The Art of Computer Programming, Ferragina's book and
notes, Morin's Open Data Structures). It is written to be read beside the
text even when it is fast, as are, e.g., the red-black tree of the book,
`BigInteger` on GMP and the PGM-index. Instead, the advanced set holds what
is written for speed beyond those sources (e.g., `UnionFind`, `DaryHeap`,
`StaticGraph`, `CompactRankSelect`), the concurrent and parallel structures,
the executors, the distributed layer, the typed modifications and the
observers. An advanced structure extends a base one through the same
concepts and by composition, with no inheritance, and a few base headers use
an advanced one inside (e.g., `SuffixArray.h` sorts by `RadixSort.h`).

A third set, core, holds the infrastructure that both sets include (the
contracts, the iterators, the portable primitives, the capabilities, the
concept of the executors, the pool of the nodes, the random hashing). A
header that held a part of each set is split into a base header, which keeps
its path, and an advanced one that it includes (e.g., `MaxFlow.h` and
`PushRelabel.h`), and therefore its includes give the same as before. The
headers of `ADS/interop/`, which join ADS to Boost, to LEMON, to the newest
parts of the standard library and to the parallel runtimes of a program,
form with `distributed/BoostMPI.h` a fourth set, interop. No entry point
includes them and no module exports them, since each is included by the
program that wants it, next to the headers of the other library.

`#include "ADS/base.h"` (or `import ads.base;`) gives the base set with
core, `ADS/advanced.h` (`import ads.advanced;`) the advanced one with core,
`ADS/core.h` (`import ads.core;`) core only, and `ADS/ADS.h` (`import ads;`)
everything. Hence, a program that wants only the book compiles neither the
concurrent structures nor the backends of MPI and oneTBB. The set of each
header is given in one place, [`cmake/ads_sets.txt`](cmake/ads_sets.txt),
from which `cmake/ads_sets.py` writes the entry points and the table below.
CMake runs it at configure time and the makefile at each `make`; it stops on
a header with no set or on an include across the sets that the file does not
allow.

<!-- sets: written by cmake/ads_sets.py -->

459 headers: 245 base, 167 advanced, 19 core and 28 interop.

| Folder | Base | Advanced | Core | Interop |
|---|---|---|---|---|
| `ADS/` | `ADT` | `Algebra`, `Modification` | `Abi`, `Capabilities`, `Contract`, `Iterators`, `Portable`, `Simd`, `SimdInline`, `Wide` |  |
| `algorithm/` | `AkraBazzi`, `BinaryCounter`, `Hanoi`, `MasterTheorem`, `MaximumSegments`, `MonteCarlo`, `Paradigms`, `SequenceQueries`, `Subsequences` |  |  |  |
| `backtrack/` | `Anagrams`, `DancingLinks`, `Enumeration`, `KnightTour`, `NQueens`, `SatSolvers`, `Sudoku` | `FastExactCover` |  |  |
| `combinatorial/` | `Bitwise`, `Generation` | `FastBitwise`, `FastGeneration` |  |  |
| `complexity/` | `Domino`, `EasyCases`, `Nondeterminism`, `Problems`, `Reductions` |  |  |  |
| `compression/` | `ArithmeticCoding`, `BurrowsWheeler`, `CanonicalHuffman`, `Entropy`, `FMIndex`, `IntegerCodes`, `LempelZiv`, `PPM` | `BinaryPacking`, `DirectAccessCodes`, `HuffmanCoder`, `HuffmanFMIndex`, `LZ77Linear`, `MoveRIndex`, `RIndex`, `Rans`, `StreamVByte`, `Tans` | `PackedBits` |  |
| `concurrent/` |  | `Backoff`, `ConcurrentCuckooTable`, `ConcurrentFlatTable`, `ConcurrentUnionFind`, `EpochReclaim`, `EventCount`, `HazardPointer`, `LockFreePriorityQueue`, `LockFreeQueue`, `LockFreeStack`, `MPMCQueue`, `MPSCInbox`, `MPSCQueue`, `MinBoard`, `MultiQueue`, `Reclaim`, `SPSCQueue`, `SingleWriterFlatTable`, `StealablePriorityQueue`, `WorkStealingDeque` |  |  |
| `dictionary/` | `BinaryTrie`, `BitmapTrie`, `ChainedHashTable`, `CuckooHashing`, `DirectTable`, `HashFunctions`, `IndexedSkipList`, `KnuthHashing`, `OpenHashTable`, `PatriciaTrie`, `PerfectHashing`, `SkipList`, `SortedArrayDictionary`, `TableSize`, `Trie`, `TwoLevelDictionary`, `UniversalHashing`, `XFastTrie`, `YFastTrie` | `AdaptiveRadixTree`, `BucketCuckooTable`, `ConsensusRecSplit`, `DenseHashMap`, `FastHash`, `FingerprintPerfectHash`, `FlatHashTable`, `MinimalPerfectHash`, `PointerTable`, `PositionIndex`, `PtrHash`, `RecSplit`, `RowFingerprint`, `SuccinctPatriciaTrie` | `RandomHashing` |  |
| `distributed/` |  | `DistributedSort`, `MPIExecutor`, `MPIMailbox` |  | `BoostMPI` |
| `external/` | `BlockBTree`, `BlockStore`, `ExternalSorting`, `ListRanking`, `TwoLevelMemory` |  | `MappedFile`, `Persistence`, `RecordFile` |  |
| `geometry/` | `ClosestPair`, `ConvexHull` | `IntervalTree`, `RangeTree` |  |  |
| `graph/` | `AllPairsShortestPaths`, `ArrayGraph`, `EulerianCycle`, `FlowApplications`, `Graph`, `Grid`, `IncidenceGraph`, `ListGraph`, `Matching`, `MatrixGraph`, `MaxFlow`, `MinimumSpanningTree`, `Paths`, `Properties`, `ShortestPaths`, `Visit` | `Assignment`, `Biconnectivity`, `BlossomMatching`, `GraphBuilder`, `GridGraph`, `Observable`, `PointerGraph`, `PushRelabel`, `Reoptimization`, `StaticGraph`, `StrongComponents` |  |  |
| `interop/` |  |  |  | `Csr` |
| `interop/arrow/` |  |  |  | `Arrow` |
| `interop/boost/` |  |  |  | `Dijkstra`, `FromBGL`, `Graph`, `Heap`, `Iterator` |
| `interop/cereal/` |  |  |  | `Cereal` |
| `interop/croaring/` |  |  |  | `CRoaring` |
| `interop/eigen/` |  |  |  | `Eigen` |
| `interop/graphblas/` |  |  |  | `GraphBLAS` |
| `interop/kahip/` |  |  |  | `KaHIP` |
| `interop/lemon/` |  |  |  | `Digraphs`, `Heaps`, `Maps`, `Observers`, `Parallel`, `Warm` |
| `interop/metis/` |  |  |  | `Metis` |
| `interop/networkit/` |  |  |  | `NetworKit` |
| `interop/parallel/` |  |  |  | `KokkosExecutor`, `SenderExecutor`, `StdParExecutor`, `TaskflowExecutor` |
| `interop/sdsl/` |  |  |  | `Sdsl` |
| `interop/std/` |  |  |  | `FlatMap`, `Pmr` |
| `intractable/` | `Approximation`, `BranchAndBound`, `FifteenPuzzle`, `Heuristics`, `Pseudopolynomial` |  |  |  |
| `memory/` | `Allocators`, `CellHeap` | `Arena`, `ConcurrentPool`, `PageHeap`, `PoolAllocator`, `SizeClassAllocator` | `AllocatorAware`, `HugePageAllocator`, `ObjectPool`, `Prefetch` |  |
| `mfset/` | `ListMFSet`, `MFSet` | `RemUnionFind`, `RollbackUnionFind`, `UnionFind` |  |  |
| `numeric/` | `BigInteger`, `BinaryInteger`, `Catalan`, `Combinatorics`, `Matrix`, `MatrixChain`, `NumberTheory`, `Polynomial`, `RSA`, `RandomNumbers`, `RandomSampling`, `Rational`, `SparseMatrix` | `CsrMatrix`, `DenseMatrix`, `DynamicCsr`, `RandomGenerators`, `SellMatrix`, `SparseAssembly` |  |  |
| `optimization/` | `CoinChange`, `CoinGame`, `FractionalKnapsack`, `GridPaths`, `HouseProblems`, `IntervalCover`, `Intervals`, `Knapsack`, `KnapsackVariants`, `LinearPartition`, `OptimalSearchTree`, `SequenceProblems`, `ShortestJobFirst`, `StockTrading`, `WeightedIntervals` |  |  |  |
| `parallel/` |  | `Affinity`, `Channels`, `Farm`, `FastFlowExecutor`, `Mailbox`, `OpenMPExecutor`, `ParallelBiconnectivity`, `ParallelGraph`, `ParallelGreedy`, `ParallelMerge`, `ParallelNumeric`, `ParallelShortestPaths`, `ParallelSketch`, `ParallelSort`, `ParallelText`, `Scratch`, `SpeculativeFor`, `TBBExecutor`, `ThreadPool` | `Executor` |  |
| `priority/` | `FibonacciHeapPriorityQueue`, `HeapPriorityQueue`, `ListPriorityQueue`, `LoserTree`, `MeldableHeap`, `PriorityQueue`, `TreePriorityQueue` | `BucketQueue`, `DaryHeap`, `DialQueue`, `IndexedDaryHeap`, `IndexedHeapPair`, `LinkedHeapPriorityQueue`, `MinMaxHeap`, `MultiwayMerge`, `OrderKey`, `PairingHeap`, `QueueCustomization`, `RadixHeap`, `SequenceHeap` |  |  |
| `queue/` | `ArrayQueue`, `Dequeue`, `LinkedQueue`, `MinDeque`, `MinQueue`, `QueueAlgorithms`, `RandomQueue`, `RealTimeMinDeque`, `RealTimeMinQueue`, `UniqueQueue` | `RingDeque`, `SlidingAggregate` |  |  |
| `search/` | `BinarySearch`, `InterpolationSearch`, `KnuthSearches`, `LocalOptima`, `Minimum`, `MonotoneSearch`, `PGMIndex`, `Selection`, `SortedSearches` | `CompressedPGMIndex`, `CountLess`, `DynamicPGMIndex`, `NthElement`, `RangeMinimum`, `StaticSearch`, `WordRangeMinimum` |  |  |
| `sequence/` | `ArraySequence`, `ContiguousDeque`, `CubishArrayStack`, `CursorList`, `DualArrayDeque`, `IndexedList`, `List`, `OrderMaintenanceList`, `Rearrangements`, `RootishArrayStack`, `SEList`, `SequenceAlgorithms`, `SequenceRange`, `SinglyList`, `SkipListSequence`, `TreapList`, `Treque`, `XorList` | `Fenwick`, `ListSort`, `ObservableArray`, `SmallVector`, `TieredVector` |  |  |
| `set/` | `Bag`, `BloomFilter`, `BloomVariants`, `BoolSet`, `DictionarySet`, `HashMultiset`, `LinkedHashSet`, `ListSet`, `SetInterfaces`, `SetOperations`, `SortedIntersection`, `SortedListSet`, `SpaceEfficientSortedSet` | `BinaryFuseFilter`, `BlockedBloomFilter`, `CountMinSketch`, `CuckooFilter`, `GrafiteFilter`, `MinHashSketch`, `PrefixFilter`, `RibbonFilter`, `RoaringBitmap`, `UltraLogLog`, `VectorIntersection` |  |  |
| `sort/` | `CountingSort`, `EngineeredQuickSorts`, `ExchangeSorts`, `ExternalSort`, `Heapsort`, `InsertionSort`, `KnuthSorts`, `MergeSort`, `MultikeyQuickSort`, `PigeonholeSort`, `QuickSort`, `SelectionSort`, `ShellSort`, `SortingNetworks`, `StrandSort`, `TreeSort` | `LearnedSort`, `PdqSort`, `QuicksortAdversary`, `RadixSort`, `RunSorts`, `SampleSort`, `Sort`, `StringSort`, `VectorQuickSort` | `Sorted` |  |
| `stack/` | `ArrayStack`, `Expressions`, `MinStack`, `MultiStack`, `SequenceStack`, `StackAlgorithms` |  |  |  |
| `string/` | `Alignment`, `Anagrams`, `ApproximateMatching`, `FrontCoding`, `Huffman`, `LongestCommonSubsequence`, `Lyndon`, `Palindromes`, `Parentheses`, `PrefixSearch`, `RearCoding`, `Segmentations`, `StringBreaking`, `StringMatching`, `SubsequenceProblems`, `SuffixApplications`, `SuffixArray`, `SuffixTree`, `WordBreak` | `AhoCorasick`, `EditDistance`, `LongestCommonExtension`, `MultiSearch`, `StringSearch`, `SuffixAutomaton`, `SuffixSorting` |  |  |
| `succinct/` | `BPTree`, `BVGraph`, `BalancedParentheses`, `BitStream`, `BitVector`, `EliasFano`, `EliasFanoGraph`, `HuffmanWaveletTree`, `JacobsonTree`, `K2Tree`, `LoudsTree`, `RankSelect` | `CompactRankSelect`, `InterleavedRankSelect`, `LAVector`, `MoveStructure`, `PackedArray`, `PartitionedEliasFano`, `RRRBitVector`, `TinyRankSelect`, `WaveletMatrix` |  |  |
| `tree/` | `AVLTree`, `ArrayBinaryTree`, `BPlusTree`, `BTree`, `BinaryTree`, `BinaryTreeAlgorithms`, `ChildListTree`, `CursorBinaryTree`, `CursorTree`, `ExpressionTree`, `FingerTreap`, `HashedTreap`, `HeapByInsertion`, `LeftLeaningRedBlackTree`, `MeasuredSearchTree`, `ParentTree`, `PartialRebuildingTree`, `PreorderArrayTree`, `RandomizedSearchTree`, `ScapegoatTree`, `SearchTree`, `ThreadedTree`, `Treap`, `Tree` | `BTreeMap`, `JoinAlgorithms`, `JoinTree`, `LinkCutTree`, `MaterializedAggregate`, `SplayTree` |  |  |

<!-- end of sets -->

## Where each thing comes from

Each header and each test cites the section, the example or the exercise of
the book that it implements, and [`test/tests.txt`](test/tests.txt) lists
the tests with their source. The solutions of the exercises are in the
tests, one function per exercise. Each implementation is checked on the
example of the book, on its edge cases and (where an enumeration is
possible) on random instances against the enumeration. The license of the
book (CC BY-NC 4.0) does not allow its text in a repository under a license
that permits commercial use. Hence, ADS cites the book as the source of the
abstract data types and of each algorithm, without reproducing it. The
pseudocode of the comments and of the manual, like their prose, is written
anew from the code of ADS, and is therefore no translation of the book.

| Chapter | Section | What | Header | Tests |
|---|---|---|---|---|
| [1](https://algoritmicamente.it/materiali/libro/capitoli/cap01.pdf) | 1.2 | minimum, binary search | [`search/Minimum.h`](include/ADS/search/Minimum.h), [`search/BinarySearch.h`](include/ADS/search/BinarySearch.h) | `ch01.sec1.2.*` |
| | 1.4 | multiplication of Ahmes | [`numeric/BinaryInteger.h`](include/ADS/numeric/BinaryInteger.h) | `ch01.sec1.4.ahmes` |
| [2](https://algoritmicamente.it/materiali/libro/capitoli/cap02.pdf) | 2.6 | sum and products of binary integers, Karatsuba | [`numeric/BinaryInteger.h`](include/ADS/numeric/BinaryInteger.h) | `ch02.sec2.6.*` |
| | 2.7 | Selection, Insertion, Merge, Counting Sort, lower bound | [`sort/`](include/ADS/sort) | `ch02.sec2.7.*` |
| | 2.9 | solutions of the recurrences (Teoremi 2.2–2.5) | [`algorithm/MasterTheorem.h`](include/ADS/algorithm/MasterTheorem.h) | `ch02.sec2.9.recurrences` |
| [3](https://algoritmicamente.it/materiali/libro/capitoli/cap03.pdf) | 3.2 | SEQUENCE, SET, DICTIONARY | [`ADT.h`](include/ADS/ADT.h) | `ch03.*` |
| [4](https://algoritmicamente.it/materiali/libro/capitoli/cap04.pdf) | 4.1 | LIST, bidirectional circular with sentinel | [`sequence/List.h`](include/ADS/sequence/List.h) | `ch04.spec.List` |
| | 4.2 | sequence on a vector that doubles | [`sequence/ArraySequence.h`](include/ADS/sequence/ArraySequence.h) | `ch04.spec.ArraySequence`, `ch04.sec4.2.doubling` |
| | 4.2.1 | binary counter, amortized analysis | [`algorithm/BinaryCounter.h`](include/ADS/algorithm/BinaryCounter.h) | `ch04.sec4.2.1.counter` |
| | 4.3 | STACK, QUEUE | [`ADT.h`](include/ADS/ADT.h) | |
| | 4.3.1 | stack simulated with a sequence | [`stack/SequenceStack.h`](include/ADS/stack/SequenceStack.h) | `ch04.sec4.3.1.stack-on-list` |
| | 4.3.2 | stack on a vector | [`stack/ArrayStack.h`](include/ADS/stack/ArrayStack.h) | `ch04.spec.ArrayStack` |
| | 4.3.3 | queue on a circular vector | [`queue/ArrayQueue.h`](include/ADS/queue/ArrayQueue.h) | `ch04.spec.ArrayQueue` |
| | 4.4 | towers of Hanoi, recursive and iterative | [`algorithm/Hanoi.h`](include/ADS/algorithm/Hanoi.h) | `ch04.sec4.4.hanoi` |
| | 4.6 | exercises 4.1–4.11; the singly linked list of 4.6 and the linked queue of 4.10 | [`test/ch04.cpp`](test/ch04.cpp), [`sequence/SinglyList.h`](include/ADS/sequence/SinglyList.h), [`queue/LinkedQueue.h`](include/ADS/queue/LinkedQueue.h) | `ch04.ex4.*` |
| [5](https://algoritmicamente.it/materiali/libro/capitoli/cap05.pdf) | 5.1, 5.3 | ordered TREE, parent/first child/sibling | [`tree/Tree.h`](include/ADS/tree/Tree.h) | `ch05.spec.Tree` |
| | 5.2 | previsit, postvisit, invisit, by levels | [`tree/Tree.h`](include/ADS/tree/Tree.h) | `ch05.sec5.2.visits` |
| | 5.4 | vector of parents | [`tree/ParentTree.h`](include/ADS/tree/ParentTree.h) | `ch05.sec5.4.parents` |
| | 5.5 | binary TREE and its visits, ordered tree as binary | [`tree/BinaryTree.h`](include/ADS/tree/BinaryTree.h) | `ch05.spec.BinaryTree`, `ch05.sec5.5.*` |
| | 5.6 | complete d-ary tree in a vector | [`tree/BinaryTree.h`](include/ADS/tree/BinaryTree.h) | `ch05.sec5.6.dary` |
| | 5.8 | exercises 5.1–5.13 | [`test/ch05.cpp`](test/ch05.cpp) | `ch05.ex5.*` |
| [6](https://algoritmicamente.it/materiali/libro/capitoli/cap06.pdf) | 6.1 | binary search tree (ABR), DICTIONARY | [`tree/SearchTree.h`](include/ADS/tree/SearchTree.h) | `ch06.sec6.1.*` |
| | 6.2 | Red-Black tree, Teoremi 6.1–6.3 | [`tree/SearchTree.h`](include/ADS/tree/SearchTree.h) | `ch06.spec.RedBlackTree`, `ch06.sec6.2.*` |
| | 6.3 | B+-tree, insertion, search, interval | [`tree/BPlusTree.h`](include/ADS/tree/BPlusTree.h) | `ch06.sec6.3.*` |
| | 6.4 | exercises 6.1–6.10 | [`test/ch06.cpp`](test/ch06.cpp) | `ch06.ex6.*` |
| [7](https://algoritmicamente.it/materiali/libro/capitoli/cap07.pdf) | 7 | table with direct access | [`dictionary/DirectTable.h`](include/ADS/dictionary/DirectTable.h) | `ch07.sec7.direct-access` |
| | 7.1 | extraction, XOR, division, multiplication, middle square | [`dictionary/HashFunctions.h`](include/ADS/dictionary/HashFunctions.h) | `ch07.sec7.1.*` |
| | 7.2 | lists of overflow, open addressing, deleted places | [`dictionary/ChainedHashTable.h`](include/ADS/dictionary/ChainedHashTable.h), [`dictionary/OpenHashTable.h`](include/ADS/dictionary/OpenHashTable.h) | `ch07.sec7.2.*` |
| | 7.3 | I(α) and S(α) measured against the formulas | [`dictionary/OpenHashTable.h`](include/ADS/dictionary/OpenHashTable.h) | `ch07.sec7.3.complexity` |
| | 7.4 | restructuring, amortized cost | [`dictionary/TableSize.h`](include/ADS/dictionary/TableSize.h) | `ch07.sec7.4.restructure` |
| | – | exercises 7.1–7.10 | [`test/ch07.cpp`](test/ch07.cpp) | `ch07.ex7.*` |
| [8](https://algoritmicamente.it/materiali/libro/capitoli/cap08.pdf) | 8.1 | SET on a boolean vector | [`set/BoolSet.h`](include/ADS/set/BoolSet.h) | `ch08.sec8.1.boolean` |
| | 8.2 | SET on an unsorted list | [`set/ListSet.h`](include/ADS/set/ListSet.h) | `ch08.sec8.2.unsorted-list` |
| | 8.3 | SET on a sorted list, operations by merging | [`set/SortedListSet.h`](include/ADS/set/SortedListSet.h) | `ch08.sec8.3.sorted-list` |
| | 8.4 | SET on hash tables | [`set/DictionarySet.h`](include/ADS/set/DictionarySet.h), [`set/SetOperations.h`](include/ADS/set/SetOperations.h) | `ch08.sec8.4.hash` |
| | 8.5 | SET on Red-Black trees | [`set/DictionarySet.h`](include/ADS/set/DictionarySet.h) | `ch08.sec8.5.tree` |
| | 8.7 | exercises 8.1–8.10, LinkedHashSet, multiset | [`set/LinkedHashSet.h`](include/ADS/set/LinkedHashSet.h), [`set/HashMultiset.h`](include/ADS/set/HashMultiset.h), [`test/ch08.cpp`](test/ch08.cpp) | `ch08.ex8.*` |
| [9](https://algoritmicamente.it/materiali/libro/capitoli/cap09.pdf) | 9.1 | definitions, the case of the duke McPollock | [`test/ch09.cpp`](test/ch09.cpp) | `ch09.sec9.1.*` |
| | 9.2, 9.4 | GRAPH, lists and vectors of adjacency | [`graph/Graph.h`](include/ADS/graph/Graph.h), [`graph/ListGraph.h`](include/ADS/graph/ListGraph.h), [`graph/ArrayGraph.h`](include/ADS/graph/ArrayGraph.h) | `ch09.spec.ListGraph`, `ch09.sec9.3.matrices` |
| | 9.3 | adjacency matrix, incidence matrix | [`graph/MatrixGraph.h`](include/ADS/graph/MatrixGraph.h) | `ch09.spec.MatrixGraph`, `ch09.sec9.3.matrices` |
| | 9.5 | bfs, distances, dfs, components, dfs schema, cycles, topological order, strong components | [`graph/Visit.h`](include/ADS/graph/Visit.h), [`graph/StrongComponents.h`](include/ADS/graph/StrongComponents.h) | `ch09.sec9.5.*` |
| | 9.7 | exercises 9.1–9.11 | [`test/ch09.cpp`](test/ch09.cpp) | `ch09.ex9.*` |
| [10](https://algoritmicamente.it/materiali/libro/capitoli/cap10.pdf) | 10.1 | PRIORITYQUEUE on a d-ary heap, on lists, on a Red-Black tree | [`priority/`](include/ADS/priority) | `ch10.spec.*PriorityQueue`, `ch10.sec10.1.3.heap` |
| | 10.1.4 | heapBuild(), heapsort() | [`sort/Heapsort.h`](include/ADS/sort/Heapsort.h) | `ch10.sec10.1.4.heapsort` |
| | 10.2 | MFSET on a forest (rank, compression of the paths) and on lists, incremental components | [`mfset/`](include/ADS/mfset) | `ch10.sec10.2.*` |
| | 10.4 | exercises 10.1–10.9 | [`test/ch10.cpp`](test/ch10.cpp) | `ch10.ex10.*` |
| [11](https://algoritmicamente.it/materiali/libro/capitoli/cap11.pdf) | 11.1–11.2 | shortest paths, theorem of Bellman, the prototype | [`graph/ShortestPaths.h`](include/ADS/graph/ShortestPaths.h) | `ch11.sec11.1.bellman`, `ch11.sec11.2.prototype` |
| | 11.3–11.5 | Dijkstra, Johnson, Fredman-Tarjan on a Fibonacci heap | [`graph/ShortestPaths.h`](include/ADS/graph/ShortestPaths.h), [`priority/FibonacciHeapPriorityQueue.h`](include/ADS/priority/FibonacciHeapPriorityQueue.h) | `ch11.sec11.3.dijkstra`, `ch11.sec11.5.fibonacci` |
| | 11.6–11.8 | Bellman-Ford-Moore, the stack, Pape-D'Esopo on a dequeue | [`graph/ShortestPaths.h`](include/ADS/graph/ShortestPaths.h), [`queue/Dequeue.h`](include/ADS/queue/Dequeue.h) | `ch11.sec11.6.*`, `ch11.sec11.7.stack`, `ch11.sec11.8.pape` |
| | 11.10 | exercises 11.1–11.10 | [`test/ch11.cpp`](test/ch11.cpp) | `ch11.ex11.*` |
| [12](https://algoritmicamente.it/materiali/libro/capitoli/cap12.pdf) | 12.1 | search by interpolation | [`search/InterpolationSearch.h`](include/ADS/search/InterpolationSearch.h) | `ch12.sec12.1.interpolation` |
| | 12.2 | Quicksort, its improvements, on lists | [`sort/QuickSort.h`](include/ADS/sort/QuickSort.h) | `ch12.sec12.2.*` |
| | 12.3 | products of polynomials, Karatsuba | [`numeric/Polynomial.h`](include/ADS/numeric/Polynomial.h) | `ch12.sec12.3.polynomials` |
| | 12.4 | Strassen | [`numeric/Matrix.h`](include/ADS/numeric/Matrix.h) | `ch12.sec12.4.strassen` |
| | 12.6 | exercises 12.1–12.13 | [`test/ch12.cpp`](test/ch12.cpp) | `ch12.ex12.*` |
| [13](https://algoritmicamente.it/materiali/libro/capitoli/cap13.pdf) | 13 | binomial coefficients, Fibonacci | [`numeric/Combinatorics.h`](include/ADS/numeric/Combinatorics.h) | `ch13.sec13.binomial`, `ch13.sec13.4.fibonacci` |
| | 13.1 | approximate string matching, edit distance | [`string/ApproximateMatching.h`](include/ADS/string/ApproximateMatching.h) | `ch13.sec13.1.matching` |
| | 13.2 | weighted independent intervals | [`optimization/WeightedIntervals.h`](include/ADS/optimization/WeightedIntervals.h) | `ch13.sec13.2.intervals` |
| | 13.3 | chain of matrices | [`numeric/MatrixChain.h`](include/ADS/numeric/MatrixChain.h) | `ch13.sec13.3.matrix-chain` |
| | 13.5 | Floyd-Warshall, insertion of an edge | [`graph/AllPairsShortestPaths.h`](include/ADS/graph/AllPairsShortestPaths.h) | `ch13.sec13.5.floyd-warshall` |
| | 13.6 | knapsack, by a table and by memoization | [`optimization/Knapsack.h`](include/ADS/optimization/Knapsack.h) | `ch13.sec13.6.knapsack` |
| | 13.7 | LCS, diff, longest common substring, alignments | [`string/`](include/ADS/string) | `ch13.sec13.7.lcs` |
| | 13.8 | exercises 13.1–13.13 | [`test/ch13.cpp`](test/ch13.cpp), [`algorithm/Subsequences.h`](include/ADS/algorithm/Subsequences.h) | `ch13.ex13.*` |
| [14](https://algoritmicamente.it/materiali/libro/capitoli/cap14.pdf) | 14 | change with coins | [`optimization/CoinChange.h`](include/ADS/optimization/CoinChange.h) | `ch14.sec14.change` |
| | 14.1 | real knapsack | [`optimization/FractionalKnapsack.h`](include/ADS/optimization/FractionalKnapsack.h) | `ch14.sec14.1.real-knapsack` |
| | 14.2 | independent set of intervals, coloring, towers | [`optimization/Intervals.h`](include/ADS/optimization/Intervals.h) | `ch14.sec14.2.intervals` |
| | 14.3 | Kruskal and Prim, Theorem 14.1 | [`graph/MinimumSpanningTree.h`](include/ADS/graph/MinimumSpanningTree.h) | `ch14.sec14.3.*` |
| | 14.4 | code of Huffman | [`string/Huffman.h`](include/ADS/string/Huffman.h) | `ch14.sec14.4.huffman` |
| | 14.5 | exercises 14.1–14.10, Shortest Job First | [`test/ch14.cpp`](test/ch14.cpp), [`optimization/ShortestJobFirst.h`](include/ADS/optimization/ShortestJobFirst.h) | `ch14.ex14.*` |
| [15](https://algoritmicamente.it/materiali/libro/capitoli/cap15.pdf) | 15 | local search of the minimum spanning tree | [`test/ch15.cpp`](test/ch15.cpp) | `ch15.sec15.local-search` |
| | 15.1 | Shell Sort, four sequences of gaps | [`sort/ShellSort.h`](include/ADS/sort/ShellSort.h) | `ch15.sec15.1.*` |
| | 15.2 | maximum flow (Ford-Fulkerson, Edmonds-Karp, Dinic, and the push-relabel of Goldberg and Tarjan beyond the book), from the flow there is after any change of the capacities, minimum cut, circulations | [`graph/MaxFlow.h`](include/ADS/graph/MaxFlow.h), [`graph/PushRelabel.h`](include/ADS/graph/PushRelabel.h) | `ch15.sec15.2.max-flow` |
| | 15.4 | exercises 15.1–15.10, Eulerian cycle | [`test/ch15.cpp`](test/ch15.cpp), [`graph/EulerianCycle.h`](include/ADS/graph/EulerianCycle.h) | `ch15.ex15.*` |
| [16](https://algoritmicamente.it/materiali/libro/capitoli/cap16.pdf) | 16.1 | convex hull, O(n³) and Graham | [`geometry/ConvexHull.h`](include/ADS/geometry/ConvexHull.h) | `ch16.sec16.1.*` |
| | 16.2 | enumeration: subsets, permutations, derangements, subset sum | [`backtrack/Enumeration.h`](include/ADS/backtrack/Enumeration.h) | `ch16.sec16.2.subsets` |
| | 16.3 | Sudoku | [`backtrack/Sudoku.h`](include/ADS/backtrack/Sudoku.h) | `ch16.sec16.3.sudoku` |
| | 16.4 | exercises 16.1–16.8, n queens, tour of the knight, anagrams | [`backtrack/`](include/ADS/backtrack), [`test/ch16.cpp`](test/ch16.cpp) | `ch16.ex16.*` |
| [17](https://algoritmicamente.it/materiali/libro/capitoli/cap17.pdf) | 17 | Monte Carlo: null polynomial, Freivalds, half-satisfiability | [`algorithm/MonteCarlo.h`](include/ADS/algorithm/MonteCarlo.h) | `ch17.sec17.monte-carlo`, `ch17.ex17.4`, `ch17.ex17.5` |
| | 17.1 | selection of Hoare, median of medians | [`search/Selection.h`](include/ADS/search/Selection.h) | `ch17.sec17.1.selection` |
| | 17.2 | Euclid, powers, Miller-Rabin, primality on 64 bits | [`numeric/NumberTheory.h`](include/ADS/numeric/NumberTheory.h) | `ch17.sec17.2.primality`, `ch17.ex17.1`–`ex17.3` |
| | 17.3 | RSA | [`numeric/RSA.h`](include/ADS/numeric/RSA.h) | `ch17.sec17.3.rsa` |
| [18](https://algoritmicamente.it/materiali/libro/capitoli/cap18.pdf) | 18 | the problems in NP, certificates and enumerations | [`complexity/Problems.h`](include/ADS/complexity/Problems.h), [`complexity/Domino.h`](include/ADS/complexity/Domino.h) | `ch18.sec18.examples` |
| | 18.2–18.3 | nondeterminism simulated by enumeration | [`complexity/Nondeterminism.h`](include/ADS/complexity/Nondeterminism.h) | `ch18.sec18.2.nondeterminism` |
| | 18.5–18.7 | reductions, linear tiling in P, Teoremi 18.2–18.9 | [`complexity/Reductions.h`](include/ADS/complexity/Reductions.h) | `ch18.sec18.5.*`, `ch18.sec18.7.*` |
| | 18.9 | exercises 18.1–18.16 | [`test/ch18.cpp`](test/ch18.cpp) | `ch18.ex18.*` |
| [19](https://algoritmicamente.it/materiali/libro/capitoli/cap19.pdf) | 19.1 | pseudopolynomial algorithms, scheduling | [`intractable/Pseudopolynomial.h`](include/ADS/intractable/Pseudopolynomial.h) | `ch19.sec19.1.subset-sum` |
| | 19.2 | approximation: ff, ffd, triangular salesman, apSubsetSum | [`intractable/Approximation.h`](include/ADS/intractable/Approximation.h) | `ch19.sec19.2.*` |
| | 19.3 | branch-&-bound: bbTsp, 1-tree, Little, clique, knapsack | [`intractable/BranchAndBound.h`](include/ADS/intractable/BranchAndBound.h) | `ch19.sec19.3.bb-tsp` |
| | 19.4 | heuristics: greedyTsp, min-max-greedy, local search | [`intractable/Heuristics.h`](include/ADS/intractable/Heuristics.h) | `ch19.sec19.4.*` |
| | 19.5 | the fifteen puzzle | [`intractable/FifteenPuzzle.h`](include/ADS/intractable/FifteenPuzzle.h) | `ch19.sec19.5.fifteen` |
| | 19.6 | exercises 19.1–19.19 | [`test/ch19.cpp`](test/ch19.cpp) | `ch19.ex19.*` |
| Conclusion | Tab. 19.1 | easy twins of NP-hard problems, interval graphs | [`complexity/EasyCases.h`](include/ADS/complexity/EasyCases.h) | `conclusion.*` |

## Beyond the book

Several structures and algorithms that the book does not describe, but
that a course on the subject usually covers (e.g., the realizations with
cursors, the parenthesized lists of trees, the general schemes of the
techniques), are written on the same classes and tested in
[`test/beyond.cpp`](test/beyond.cpp).

| What | Header | Test |
|---|---|---|
| binary tree in a positional vector | [`tree/ArrayBinaryTree.h`](include/ADS/tree/ArrayBinaryTree.h) | `beyond.array-binary-tree` |
| binary and ordered trees with cursors | [`tree/CursorBinaryTree.h`](include/ADS/tree/CursorBinaryTree.h), [`tree/CursorTree.h`](include/ADS/tree/CursorTree.h) | `beyond.cursor-*` |
| ordered tree in a vector in preorder | [`tree/PreorderArrayTree.h`](include/ADS/tree/PreorderArrayTree.h) | `beyond.preorder-array-tree` |
| heap on a binary tree with pointers | [`priority/LinkedHeapPriorityQueue.h`](include/ADS/priority/LinkedHeapPriorityQueue.h) | `beyond.linked-heap` |
| graph of node objects with pointers | [`graph/PointerGraph.h`](include/ADS/graph/PointerGraph.h) | `beyond.pointer-graph` |
| k stacks in one vector | [`stack/MultiStack.h`](include/ADS/stack/MultiStack.h) | `beyond.multistack` |
| search, order, bubble and natural merge sort on sequences | [`sequence/SequenceAlgorithms.h`](include/ADS/sequence/SequenceAlgorithms.h) | `beyond.sequence-*` |
| palindromes and sorting with a stack | [`stack/StackAlgorithms.h`](include/ADS/stack/StackAlgorithms.h) | `beyond.stack-algorithms` |
| naive and Knuth-Morris-Pratt string matching | [`string/StringMatching.h`](include/ADS/string/StringMatching.h) | `beyond.string-matching` |
| exercises on lists, strings, trees and arrays | [`test/beyond.cpp`](test/beyond.cpp) | `beyond.*-exercises`, `beyond.even-odd-strings` |
| list with cursors in a shared space with a free list | [`sequence/CursorList.h`](include/ADS/sequence/CursorList.h) | `beyond.cursor-list`, `ch04.spec.CursorList` |
| range of C++ over any sequence | [`sequence/SequenceRange.h`](include/ADS/sequence/SequenceRange.h) | `beyond.sequence-range` |
| search with the place of insertion and sublists of a sorted sequence | [`sequence/SequenceAlgorithms.h`](include/ADS/sequence/SequenceAlgorithms.h) | `beyond.sorted-list` |
| postfix expressions, infix to postfix (shunting-yard) | [`stack/Expressions.h`](include/ADS/stack/Expressions.h) | `beyond.postfix` |
| queue with no duplicates, positive items of a queue | [`queue/UniqueQueue.h`](include/ADS/queue/UniqueQueue.h), [`queue/QueueAlgorithms.h`](include/ADS/queue/QueueAlgorithms.h) | `beyond.queue-algorithms` |
| dictionary on a sorted vector, pseudo-random inspection | [`dictionary/SortedArrayDictionary.h`](include/ADS/dictionary/SortedArrayDictionary.h), [`dictionary/OpenHashTable.h`](include/ADS/dictionary/OpenHashTable.h) | `beyond.sorted-array-dictionary`, `beyond.hashing` |
| decision tree of the binary search | [`search/BinarySearch.h`](include/ADS/search/BinarySearch.h) | `beyond.decision-tree` |
| trees as parenthesized lists, tree of an expression, tree with lists of children | [`tree/BinaryTree.h`](include/ADS/tree/BinaryTree.h), [`tree/ExpressionTree.h`](include/ADS/tree/ExpressionTree.h), [`tree/ChildListTree.h`](include/ADS/tree/ChildListTree.h) | `beyond.list-forms`, `beyond.expression-tree`, `beyond.child-list-tree` |
| union by size in the MFSET | [`mfset/MFSet.h`](include/ADS/mfset/MFSet.h) | `contract_mfset.mfset` |
| general visit of a graph, unilateral connection | [`graph/Visit.h`](include/ADS/graph/Visit.h) | `beyond.generic-visit`, `beyond.unilateral-incidence` |
| general schemes of the techniques (enumeration, backtracking, greedy, divide et impera) | [`algorithm/Paradigms.h`](include/ADS/algorithm/Paradigms.h) | `beyond.paradigms` |
| Knuth's strong failure function, Hibbard's gaps, sentinels, minimum and maximum together | [`string/StringMatching.h`](include/ADS/string/StringMatching.h), [`sort/`](include/ADS/sort), [`search/Minimum.h`](include/ADS/search/Minimum.h) | `beyond.kmp-strong`, `beyond.slide-sorts` |
| greedy and backtracking knapsack, eight puzzle by breadth and depth | [`intractable/BranchAndBound.h`](include/ADS/intractable/BranchAndBound.h), [`intractable/FifteenPuzzle.h`](include/ADS/intractable/FifteenPuzzle.h) | `beyond.knapsack-heuristics`, `beyond.eight-puzzle` |

## From The Art of Computer Programming

ADS also has the structures and the algorithms of Knuth's *The Art of
Computer Programming* that the book leaves out, from volumes 1, 2, 3, 4A and
4B, with a few relatives from the later literature. They are tested in
[`test/taocp.cpp`](test/taocp.cpp) against the standard library, against a
slower method that is obviously correct (the definition of the transform,
the enumeration of the subsets, the backtrack of chapter 16) and against the
counts and the bounds of their analysis.

| What | Header | Test |
|---|---|---|
| Fibonacci search and uniform binary search (6.2.1) | [`search/KnuthSearches.h`](include/ADS/search/KnuthSearches.h) | `taocp.knuth-searches` |
| AVL tree (6.2.3) and B-tree with removal (6.2.4) | [`tree/AVLTree.h`](include/ADS/tree/AVLTree.h), [`tree/BTree.h`](include/ADS/tree/BTree.h) | `taocp.balanced-trees` |
| skip list, treap, splay tree | [`dictionary/SkipList.h`](include/ADS/dictionary/SkipList.h), [`tree/Treap.h`](include/ADS/tree/Treap.h), [`tree/SplayTree.h`](include/ADS/tree/SplayTree.h) | `taocp.balanced-trees` |
| trie and compact trie (Patricia) (6.3) | [`dictionary/Trie.h`](include/ADS/dictionary/Trie.h) | `taocp.tries` |
| coalesced hashing with a cellar and Brent's double hashing (6.4) | [`dictionary/KnuthHashing.h`](include/ADS/dictionary/KnuthHashing.h) | `taocp.knuth-hashing` |
| binary and list insertion, address calculation, radix exchange, tournament sort, Ford-Johnson merge insertion (5.2, 5.3.1) | [`sort/KnuthSorts.h`](include/ADS/sort/KnuthSorts.h) | `taocp.knuth-sorts` |
| replacement selection and polyphase merge (5.4) | [`sort/ExternalSort.h`](include/ADS/sort/ExternalSort.h) | `taocp.external-sort` |
| threaded binary tree (2.3.1) | [`tree/ThreadedTree.h`](include/ADS/tree/ThreadedTree.h) | `taocp.threaded-tree` |
| boundary-tag allocator with first, best and next fit, buddy system (2.5) | [`memory/Allocators.h`](include/ADS/memory/Allocators.h) | `taocp.allocators` |
| heap of cells with the marking of Deutsch, Schorr and Waite (2.3.5) | [`memory/CellHeap.h`](include/ADS/memory/CellHeap.h) | `taocp.cell-heap` |
| sparse matrix on orthogonal lists, pivot step (2.2.6) | [`numeric/SparseMatrix.h`](include/ADS/numeric/SparseMatrix.h) | `taocp.sparse-matrix` |
| broadword tricks: sideways sum, ruler, Gray code, Gosper, Morton code (7.1.3) | [`combinatorial/Bitwise.h`](include/ADS/combinatorial/Bitwise.h) | `taocp.bitwise` |
| tuples, permutations, combinations, partitions, nested parentheses, with the Gray orders (7.2.1) | [`combinatorial/Generation.h`](include/ADS/combinatorial/Generation.h) | `taocp.generation` |
| linear congruential generators, chi-square, Kolmogorov-Smirnov, serial and spectral tests, shuffling and sampling (3.2-3.4) | [`numeric/RandomNumbers.h`](include/ADS/numeric/RandomNumbers.h) | `taocp.random-numbers` |
| integers of any length: Algorithms A, S, M and D, Karatsuba and the NTT, the conversions of 4.4, the gcd of Lehmer, or GMP where it is found (4.3, 4.4, 4.5.2) | [`numeric/BigInteger.h`](include/ADS/numeric/BigInteger.h) | `taocp.big-integer*` |
| exact rationals (4.5.1) | [`numeric/Rational.h`](include/ADS/numeric/Rational.h) | `taocp.rational` |
| exact cover by dancing links, Algorithm X (7.2.2.1) | [`backtrack/DancingLinks.h`](include/ADS/backtrack/DancingLinks.h) | `taocp.dancing-links` |
| SAT by DPLL and by conflict-driven clause learning (7.2.2.2) | [`backtrack/SatSolvers.h`](include/ADS/backtrack/SatSolvers.h) | `taocp.sat-solvers` |

## From Ferragina's algorithm engineering

The algorithms of P. Ferragina's *Pearls of Algorithm Engineering*, from the
maximum sub-array sum to the compressed data structures, are in ADS as well.
Beside the algorithms of its chapter, each header gives the variants of the
later literature (e.g., SA-IS beside the skew algorithm, or the sampled
select of the practical libraries beside Clark's scheme). Their tests, in
`test/pearls_*.cpp`, check them against brute force, against the standard
library and against the examples of the notes, with their numbers. A class
simulates the two-level memory of chapters 3 to 6 and counts the reads and
the writes of blocks, and therefore the tests can check the bounds on the
I/Os that those chapters prove.

| What | Headers | Tests |
|---|---|---|
| maximum sub-array sum, with lengths in a range and maximum density (2) | [`algorithm/MaximumSegments.h`](include/ADS/algorithm/MaximumSegments.h) | `pearls_io.ch2.*` |
| random sampling, on disk and on streams, Vitter's skips (3) | [`numeric/RandomSampling.h`](include/ADS/numeric/RandomSampling.h) | `pearls_io.ch3.*` |
| list ranking by pointer jumping and independent sets, in memory and on disk (4) | [`external/ListRanking.h`](include/ADS/external/ListRanking.h) | `pearls_io.ch4.*` |
| two-level memory, binary and multiway mergesort, distribution sort, loser tree, I/O lower bounds, the quicksort with the three-way partition, the dual-pivot quicksort (5) | [`external/`](include/ADS/external), [`priority/LoserTree.h`](include/ADS/priority/LoserTree.h), [`sort/EngineeredQuickSorts.h`](include/ADS/sort/EngineeredQuickSorts.h) | `pearls_io.ch5.*`, `sorting.engineered-quicksorts` |
| sorted set intersection: merge, binary search, mutual partition, doubling, two-level (6) | [`set/SortedIntersection.h`](include/ADS/set/SortedIntersection.h) | `pearls_io.ch6.*` |
| multikey quicksort of strings (7) | [`sort/MultikeyQuickSort.h`](include/ADS/sort/MultikeyQuickSort.h) | `sorting.radix-sorts` |
| universal hashing, d-left, FKS, minimal ordered perfect hashing, cuckoo hashing, Bloom filter and its variants (8) | [`dictionary/`](include/ADS/dictionary), [`set/BloomFilter.h`](include/ADS/set/BloomFilter.h), [`set/BloomVariants.h`](include/ADS/set/BloomVariants.h) | `pearls_hashing.*`, `taocp.bloom-filter` |
| prefix search, front coding, Patricia trie, two-level dictionary (9) | [`string/PrefixSearch.h`](include/ADS/string/PrefixSearch.h), [`string/FrontCoding.h`](include/ADS/string/FrontCoding.h), [`dictionary/PatriciaTrie.h`](include/ADS/dictionary/PatriciaTrie.h) | `pearls_hashing.*` |
| suffix arrays (six constructions), LCP, range minimum queries, suffix trees and their applications (10) | [`string/SuffixArray.h`](include/ADS/string/SuffixArray.h), [`string/SuffixTree.h`](include/ADS/string/SuffixTree.h), [`search/RangeMinimum.h`](include/ADS/search/RangeMinimum.h) | `pearls_suffix.*` |
| integer codes: gamma, delta, Rice, Golomb, PForDelta, variable-byte, (s,c)-dense, interpolative, Elias-Fano (11) | [`compression/IntegerCodes.h`](include/ADS/compression/IntegerCodes.h), [`succinct/EliasFano.h`](include/ADS/succinct/EliasFano.h) | `pearls_succinct.*` |
| canonical Huffman, arithmetic coding, PPM (12), LZ77, LZSS, LZ78, LZW (13), Burrows-Wheeler, bzip, FM-index (14) | [`compression/`](include/ADS/compression) | `pearls_compression.*` |
| bit vectors with rank and select, succinct trees (Jacobson, LOUDS, balanced parentheses) and graphs (k2-tree, Boldi-Vigna) (15) | [`succinct/`](include/ADS/succinct) | `pearls_succinct.*` |

## The exercises and the exams of the course

Beyond the exercises of the book, the tests cover the material of the course
on which the book is based: the chapter collections, the thematic
collections, more than 80 exams from 2011 to 2026 with their official
solutions, the exercise sessions and the laboratory projects. Each problem
is implemented after its official solution and checked on the examples of
its statement. In addition, it is checked on random instances against an
independent oracle (brute force, a simpler algorithm, the standard library),
which usually exposes the slips of the official solutions. In such a case,
the test implements the correct version and shows the counterexample. All
these tests are in `test/exam_*.cpp`, one program per theme (e.g.,
backtracking, dynamic programming, flows) or per source, and the name of a
test gives the problem and the date of its exam.

## Engineered structures

Beside the classes of the book, which follow its pseudocode, a few
structures are written for speed, for the programs that need it. They are
tested in [`test/engineered.cpp`](test/engineered.cpp), the heaps in
[`test/heaps.cpp`](test/heaps.cpp), the vectors in
[`test/sequences.cpp`](test/sequences.cpp) and the hashing in
[`test/hashing.cpp`](test/hashing.cpp), while those of the notes of
Ferragina are tested in
[`test/engineered_pearls.cpp`](test/engineered_pearls.cpp) and the
combinatorial ones in
[`test/engineered_combinatorial.cpp`](test/engineered_combinatorial.cpp).
Those of the allocators and of the structures of the rows of a model are in
[`test/engineered_memory.cpp`](test/engineered_memory.cpp) and
[`test/engineered_sparse.cpp`](test/engineered_sparse.cpp), and the
concurrent structures are also tested on real threads under ThreadSanitizer.

`FlatHashTable` is the hash table that
[`bench/dictionaries.cpp`](bench/dictionaries.cpp) compares with
`boost::unordered_flat_map`, while `OpenHashTable` stays the table of
section 7.2 of the book. Furthermore,
[`bench/hashing.cpp`](bench/hashing.cpp) compares the hashing with Abseil,
unordered_dense, robin-map, libcuckoo, XXH3, wyhash, rapidhash,
fastfilter_cpp, xor_singleheader, PTHash, BBHash, RecSplit and
Consensus-RecSplit, and the concurrent cuckoo table with libcuckoo,
`boost::concurrent_flat_map` and a `std::unordered_map` under a
`std::shared_mutex`.
[`bench/priority.cpp`](bench/priority.cpp) compares the heaps with
Boost.Heap, tlx and radix-heap, also in Dijkstra's algorithm and in
best-first searches on a grid and on the 15-puzzle. Similarly,
[`bench/memory.cpp`](bench/memory.cpp) compares the arena and the pool with
`std::pmr`, Boost.Pool, mimalloc and jemalloc, and
[`bench/allocators.cpp`](bench/allocators.cpp) measures the standard
allocators in the node containers, in `std::allocate_shared` and across two
threads. [`bench/graphs.cpp`](bench/graphs.cpp) runs the algorithms of the
book, which are templates on the structure of the graph, on the graphs of
the standard library, of Boost and of ADS, with the same counts of work on
the three. It also runs the visit in breadth and Dijkstra's algorithm on the
implicit grid of `GridGraph`, against its explicit graph and the
`grid_graph` of Boost. The combinatorial structures are compared by
[`bench/combinatorial.cpp`](bench/combinatorial.cpp) and
[`bench/backtrack.cpp`](bench/backtrack.cpp) with libpopcnt, libmorton,
Hinnant's combinations and the programs DLX1, DLX2 and SSXCC of Knuth.
Finally, [`bench/positions.cpp`](bench/positions.cpp),
[`bench/assembly.cpp`](bench/assembly.cpp) and
[`bench/fingerprints.cpp`](bench/fingerprints.cpp) measure the structures of
the rows of a model (the index of their positions, their assembly for the
solvers of linear programs and their fingerprints) against the standard
library, Boost and Eigen.

| What | Header | Test |
|---|---|---|
| two-level bucket queue for integer keys, the large second keys of a layer in sparse runs until they are many, and behind the interface of a heap of packed keys (Dial; Burns et al.) | [`priority/BucketQueue.h`](include/ADS/priority/BucketQueue.h) | `engineered.bucket-queue`, `engineered.bucket-queue-sparse` |
| d-ary heap of plain items, no handles | [`priority/DaryHeap.h`](include/ADS/priority/DaryHeap.h) | `engineered.dary-heap` |
| d-ary heap of the ids with decrease-key, increase-key and erase anywhere, the places in a vector, a PRIORITYQUEUE whose handles are the ids; its insertions, erasures, changes of key and, on demand, the moves of its items told through a subject policy | [`priority/IndexedDaryHeap.h`](include/ADS/priority/IndexedDaryHeap.h) | `engineered_graph.indexed-dary-heap`, `engineered_search.indexed-heap-erase`, `engineered_emitters.emit-heap*` |
| the ids in two indexed heaps, by a key of selection and by a bound, the least of each in O( 1 ) (the frontier of the bounded suboptimal searches and of the branch and bound by best estimate) | [`priority/IndexedHeapPair.h`](include/ADS/priority/IndexedHeapPair.h) | `engineered_search.indexed-heap-pair` |
| graph in compressed rows, the heads and the weights in two arrays or in one array of pairs (a parameter), the rows sorted by head; transpose and relabelling | [`graph/StaticGraph.h`](include/ADS/graph/StaticGraph.h) | `engineered_graph.static-graph` |
| graph of a grid map, the cells in one vector of costs and the moves to four or eight neighbors computed, the rules on the corners and the octile costs of the benchmarks, each move weighed by the cell entered or by the cell left, the maps in their format; Manhattan, octile and Chebyshev bounds for best-first search (Sturtevant; Harabor and Grastien) | [`graph/GridGraph.h`](include/ADS/graph/GridGraph.h) | `engineered_graph.grid-graph`, `engineered_graph.grid-graph-paths`, `engineered_graph.grid-graph-huge`, `engineered_graph.grid-map` |
| builder of a graph one edge at a time, frozen into `StaticGraph` or `ArrayGraph` by counting sort | [`graph/GraphBuilder.h`](include/ADS/graph/GraphBuilder.h) | `engineered_graph.graph-builder` |
| union-find in one vector, by size or rank, the paths halved or compressed (Tarjan and van Leeuwen) | [`mfset/UnionFind.h`](include/ADS/mfset/UnionFind.h) | `engineered_graph.union-find` |
| typed modifications: events with the old and the new state, observers whose optional parts are found at compile time, subject policies with no code when nobody listens, nested batches coalesced per handle, revisions, mutes, the strong guarantee by inverse events, an undo log and a shadow audit; the recompute policies of the observers, on counts of work and never on times, and the compensated sum of Neumaier | [`Modification.h`](include/ADS/Modification.h) | `engineered_modification.mod-*`, `engineered_reopt.reopt-*` |
| algebra of the aggregates: monoids, commutative monoids, groups, selective monoids, actions and exact values as concepts on static members, with the sums, minima and maxima | [`Algebra.h`](include/ADS/Algebra.h) | `engineered_emitters.fenwick`, `engineered_emitters.sliding-aggregate` |
| array that tells its writes, appends, removals, clearings and rebuildings through a subject policy, the code of a vector when nobody listens | [`sequence/ObservableArray.h`](include/ADS/sequence/ObservableArray.h) | `engineered_emitters.emit-array` |
| prefix aggregates by the tree of Fenwick over any monoid, the update by difference in a group, and their observer of an observable array | [`sequence/Fenwick.h`](include/ADS/sequence/Fenwick.h) | `engineered_emitters.fenwick`, `engineered_emitters.fenwick-observer` |
| aggregate of a sliding window over any monoid, by subtract-on-evict, the monotone deque, Two-Stacks Lite or the real-time queue of Hood and Melville, as the algebra and the policy allow | [`queue/SlidingAggregate.h`](include/ADS/queue/SlidingAggregate.h) | `engineered_emitters.sliding-*` |
| capabilities of the structures: concepts on members, semantic opt-ins such as the stable references, queries that the wrappers forward, how a priority queue lowers a key (native or lazy), customization points with defaults for the types of other libraries | [`Capabilities.h`](include/ADS/Capabilities.h) | `engineered_capabilities.cap-*` |
| graph that sends its changes as events to observers, through a subject policy: masks of interest, no event built when no one listens, batches coalesced into their net effect | [`graph/Observable.h`](include/ADS/graph/Observable.h) | `engineered_dynamic.observable-*`, `engineered_modification.mod-observed-graph` |
| shortest paths, minimum spanning forest and maximum flow kept up to date through the events, each with a certificate of optimality (reduced costs, conditions of cycle and cut, minimum cut) that proves most changes harmless, repaired or computed from scratch otherwise; the shortest paths repaired after the raises too (Ramalingam and Reps, in the batch form of Bauer and Wagner), the flow after any change of the capacities, below the flow too (Kohli and Torr); the minimum spanning forest on a link-cut tree, each change in $O( \log n )$ amortized but the raises and the deletions of its edges, whose replacement is sought from the smaller side and by weight at once; the assignment, a row changed in $O( n^2 )$; each observer chooses between a repair and a computation from scratch by its recompute policy, and a batch of edges that exits by an exception leaves the graph and the observers as before | [`graph/Reoptimization.h`](include/ADS/graph/Reoptimization.h) | `engineered_dynamic.reopt-*`, `engineered_reopt.reopt-*` |
| union-find by rank with the undo of its merges, and the offline dynamic connectivity on it (a segment tree over the times) | [`mfset/RollbackUnionFind.h`](include/ADS/mfset/RollbackUnionFind.h) | `engineered_dynamic.rollback-union-find`, `engineered_dynamic.offline-connectivity` |
| link-cut trees (Sleator and Tarjan): link, cut, evert, root, lowest common ancestor and the aggregates of a commutative monoid on the paths, in $O( \log n )$ amortized | [`tree/LinkCutTree.h`](include/ADS/tree/LinkCutTree.h) | `engineered_dynamic.link-cut-tree` |
| the assignment by the shortest augmenting paths (Jonker and Volgenant, Crouse), $O( n^3 )$, kept optimal after the changes of its costs by freeing one pair and augmenting once, $O( n^2 )$ per row or column changed (Toroslu and Üçoluk; Mills-Tettey, Stentz and Dias) | [`graph/Assignment.h`](include/ADS/graph/Assignment.h) | `engineered_dynamic.assignment`, `engineered_dynamic.reopt-assignment` |
| radix heap, monotone integer keys in buckets by digits of B bits (Ahuja et al.) | [`priority/RadixHeap.h`](include/ADS/priority/RadixHeap.h) | `heaps.radix-heap` |
| double-ended queues: min-max heap (Atkinson et al.) and interval heap (van Leeuwen and Wood) | [`priority/MinMaxHeap.h`](include/ADS/priority/MinMaxHeap.h) | `heaps.double-ended-heaps` |
| monotone bucket queue on a circular window, overflow to a radix heap (Dial) | [`priority/DialQueue.h`](include/ADS/priority/DialQueue.h) | `heaps.dial-queue` |
| sequence heap, sorted sequences merged in batches by trees of losers (Sanders) | [`priority/SequenceHeap.h`](include/ADS/priority/SequenceHeap.h) | `heaps.sequence-heap` |
| pairing heap with decrease-key, increase-key and erase by handle, two-pass or auxiliary two-pass (Fredman et al.; Stasko and Vitter) | [`priority/PairingHeap.h`](include/ADS/priority/PairingHeap.h) | `heaps.pairing-heap`, `engineered_search.pairing-heap-erase` |
| sort of a List by relinking its cells: radix sort of ( key , cell ) or merge of the natural runs | [`sequence/ListSort.h`](include/ADS/sequence/ListSort.h) | `sequences.list-sort` |
| tiered vector: access in O( 1 ), insertion and removal anywhere in O( sqrt n ) (Goodrich and Kloss) | [`sequence/TieredVector.h`](include/ADS/sequence/TieredVector.h) | `sequences.tiered-vector` |
| vector with room for N items inside itself | [`sequence/SmallVector.h`](include/ADS/sequence/SmallVector.h) | `sequences.small-vector`, `engineered_boost.boost-small-vector` |
| hash table with control bytes in groups of 15 matched by SSE2, NEON or SWAR, the hash mixed on 128 bits, at most 7/8 full (Swiss tables; Boost's `unordered_flat_map`) | [`dictionary/FlatHashTable.h`](include/ADS/dictionary/FlatHashTable.h) | `engineered.flat-hash-table`, `engineered_boost.boost-flat-hash-table` |
| hash table of pointers, linear probing, Fibonacci hashing of a hash given, sized in advance by `slotsFor()`, the places from any allocator | [`dictionary/PointerTable.h`](include/ADS/dictionary/PointerTable.h) | `engineered.pointer-table`, `engineered_search.pointer-table-presize`, `engineered_boost.boost-pointer-table` |
| arena of objects in blocks, stable addresses | [`memory/Arena.h`](include/ADS/memory/Arena.h) | `engineered.arena` |
| pool of objects in doubling blocks, the slot freed last made first | [`memory/ObjectPool.h`](include/ADS/memory/ObjectPool.h) | `engineered.pool` |
| allocator of small blocks by size classes, a free list and a page of 64 KB per class (mimalloc, TLSF) | [`memory/SizeClassAllocator.h`](include/ADS/memory/SizeClassAllocator.h) | `memory.size-class-allocator` |
| allocator of the large blocks aligned to the transparent huge pages | [`memory/HugePageAllocator.h`](include/ADS/memory/HugePageAllocator.h) | `engineered.huge-page-allocator` |
| standard allocator of size classes of 8 bytes, free lists by page, for the node containers and `std::allocate_shared` (mimalloc) | [`memory/PoolAllocator.h`](include/ADS/memory/PoolAllocator.h) | `engineered_memory.pool-allocator-containers` |
| pool of size classes shared by threads, a heap per thread, the frees of the others sent back in batches (mimalloc; snmalloc) | [`memory/ConcurrentPool.h`](include/ADS/memory/ConcurrentPool.h) | `engineered_memory.concurrent-pool-threads` |
| index of the positions of objects in a vector, renumbered in one pass after an erasure by rank (Jacobson) | [`dictionary/PositionIndex.h`](include/ADS/dictionary/PositionIndex.h) | `engineered_sparse.position-index-renumber` |
| fingerprints of rows of doubles, dense and sparse, +0 equal to -0 and every NaN to every NaN | [`dictionary/RowFingerprint.h`](include/ADS/dictionary/RowFingerprint.h) | `engineered_sparse.row-fingerprint` |
| compressed sparse columns or rows from triplets, repetitions summed, into the arrays of the caller (Gustavson; Davis) | [`numeric/SparseAssembly.h`](include/ADS/numeric/SparseAssembly.h) | `engineered_sparse.sparse-assembly` |
| compressed sparse rows that grow by rows and shrink by sets of rows in one pass | [`numeric/DynamicCsr.h`](include/ADS/numeric/DynamicCsr.h) | `engineered_sparse.dynamic-csr` |
| SplitMix64, Zobrist and simple tabulation hashing | [`dictionary/RandomHashing.h`](include/ADS/dictionary/RandomHashing.h) | `engineered.random-hashing` |
| hashes of words and strings by folded products of 128 bits, stripes in AVX2 or AVX-512 past 256 bytes (wyhash, XXH3) | [`dictionary/FastHash.h`](include/ADS/dictionary/FastHash.h) | `hashing.fast-hash` |
| filter of Bloom in blocks of 256 bits, one bit in each of 8 words (Putze et al.; Parquet) | [`set/BlockedBloomFilter.h`](include/ADS/set/BlockedBloomFilter.h) | `hashing.blocked-bloom` |
| binary fuse filter, fingerprints of 8 or 16 bits (Graf and Lemire) | [`set/BinaryFuseFilter.h`](include/ADS/set/BinaryFuseFilter.h) | `hashing.binary-fuse` |
| cuckoo filter with removals, fingerprints of 8 or 16 bits, buckets of 4 places in one word (Fan et al.) | [`set/CuckooFilter.h`](include/ADS/set/CuckooFilter.h) | `hashing.cuckoo-filter` |
| prefix filter, bins of 25 fingerprints as pocket dictionaries of 32 bytes, the overflow in a blocked filter of Bloom (Even, Even and Morrison) | [`set/PrefixFilter.h`](include/ADS/set/PrefixFilter.h) | `hashing.prefix-filter` |
| standard Ribbon filter, Gauss elimination on the fly, solution by columns (Dillinger and Walzer) | [`set/RibbonFilter.h`](include/ADS/set/RibbonFilter.h) | `hashing.ribbon-filter` |
| range filter Grafite, false positives at most $\ell / 2^{B-2}$ for a range of $\ell$ keys with B bits per key whatever the queries, by a locality-preserving hash and Elias-Fano; the bucketing filter of the same paper (Costa, Ferragina and Vinciguerra) | [`set/GrafiteFilter.h`](include/ADS/set/GrafiteFilter.h) | `hashing.grafite-filter`, `hashing.bucketing-filter` |
| distinct items in registers of a byte with the two bits below the top level, FGRA and maximum likelihood estimators, merge and downsizing (UltraLogLog of Ertl) | [`set/UltraLogLog.h`](include/ADS/set/UltraLogLog.h) | `sketches.ultraloglog-estimates`, `sketches.ultraloglog-merge` |
| frequencies by the Count-Min sketch, plain and conservative update, and by the Count sketch with its second moment (Cormode and Muthukrishnan; Estan and Varghese; Charikar et al.) | [`set/CountMinSketch.h`](include/ADS/set/CountMinSketch.h) | `sketches.count-min`, `sketches.count-sketch` |
| similarity of Jaccard by minwise hashing of one permutation with the optimal densification (Li et al.; Shrivastava) | [`set/MinHashSketch.h`](include/ADS/set/MinHashSketch.h) | `sketches.minhash-jaccard` |
| parallel construction of the mergeable sketches by the merges of the sketches of blocks | [`parallel/ParallelSketch.h`](include/ADS/parallel/ParallelSketch.h) | `sketches.parallel-sketch` |
| minimal perfect hash function by pilots, about 3 bits per key (PTHash) | [`dictionary/MinimalPerfectHash.h`](include/ADS/dictionary/MinimalPerfectHash.h) | `hashing.minimal-perfect-hash` |
| minimal perfect hash function by recursive splitting, about 1.8 bits per key, leaves by rotation fitting (RecSplit; Bez et al.) | [`dictionary/RecSplit.h`](include/ADS/dictionary/RecSplit.h) | `hashing.recsplit` |
| minimal perfect hash function by pilots of 8 bits placed by evictions, parts of equal slots, streaming queries with prefetching, parts built in parallel (PtrHash; Groot Koerkamp) | [`dictionary/PtrHash.h`](include/ADS/dictionary/PtrHash.h) | `hashing.ptrhash` |
| minimal perfect hash function by recursive splitting with the seeds searched and encoded together, near 1.44 bits per key, chunks searched in parallel (Consensus-RecSplit; Lehmann, Sanders, Walzer, Ziegler) | [`dictionary/ConsensusRecSplit.h`](include/ADS/dictionary/ConsensusRecSplit.h) | `hashing.consensus-recsplit` |
| minimal perfect hash function by levels of fingerprints, ranks by blocks of 512 bits (Müller, Sanders, Schulze, Zhou; BBHash; FMPH) | [`dictionary/FingerprintPerfectHash.h`](include/ADS/dictionary/FingerprintPerfectHash.h) | `hashing.fingerprint-perfect-hash` |
| cuckoo table with buckets of 4 places, tags of 8 bits, search in breadth (Li et al.; libcuckoo) | [`dictionary/BucketCuckooTable.h`](include/ADS/dictionary/BucketCuckooTable.h) | `hashing.bucket-cuckoo` |
| hash map with its pairs in a dense vector and an index of positions in groups of 12 control bytes, a removal by moving the last pair (the dictionaries of CPython 3.6; `ankerl::unordered_dense`) | [`dictionary/DenseHashMap.h`](include/ADS/dictionary/DenseHashMap.h) | `hashing.dense-hash-map` |
| population count of arrays by the carry-save adders of Harley and Seal (AVX-512, AVX2), pdep, pext, select in a word (Mula, Kurz and Lemire) | [`combinatorial/FastBitwise.h`](include/ADS/combinatorial/FastBitwise.h) | `engineered_combinatorial.fast-bitwise` |
| permutations by Heap's method and in lexicographic order, combinations, with the innermost places in straight-line code | [`combinatorial/FastGeneration.h`](include/ADS/combinatorial/FastGeneration.h) | `engineered_combinatorial.fast-generation` |
| exact cover with colors on sparse sets, the dancing cells of Knuth's SSXCC | [`backtrack/FastExactCover.h`](include/ADS/backtrack/FastExactCover.h) | `engineered_combinatorial.fast-exact-cover` |
| lock-free queue from one producer to one consumer | [`concurrent/SPSCQueue.h`](include/ADS/concurrent/SPSCQueue.h) | `engineered.spsc-queue` |
| lock-free inbox of many producers | [`concurrent/MPSCInbox.h`](include/ADS/concurrent/MPSCInbox.h) | `engineered.mpsc-inbox` |
| wavelet matrix: access, rank, select, range quantiles (Claude, Navarro and Ordonez) | [`succinct/WaveletMatrix.h`](include/ADS/succinct/WaveletMatrix.h) | `engineered_succinct.wavelet-matrix` |
| code of Huffman of bytes, lengths of at most 11 bits by package-merge, four streams, up to three symbols a lookup | [`compression/HuffmanCoder.h`](include/ADS/compression/HuffmanCoder.h) | `engineered_succinct.huffman-coder` |
| rANS of order 0 with four interleaved states (Duda; Giesen) | [`compression/Rans.h`](include/ADS/compression/Rans.h) | `engineered_succinct.rans-coder` |
| Stream VByte, the lengths apart, one shuffle per 4 integers (Lemire, Kurz and Rupp) | [`compression/StreamVByte.h`](include/ADS/compression/StreamVByte.h) | `engineered_succinct.stream-vbyte` |
| rank and select in about 3.9% of the bits, one 128-bit entry per 4096 bits (Kurpicz; Zhou, Andersen and Kaminsky) | [`succinct/CompactRankSelect.h`](include/ADS/succinct/CompactRankSelect.h) | `engineered_succinct.compact-rank-select` |
| rank and select with the counts inside the lines of the bits, rank in one access (SPIDER, Laws et al.) | [`succinct/InterleavedRankSelect.h`](include/ADS/succinct/InterleavedRankSelect.h) | `engineered_succinct.interleaved-rank-select` |
| rank and select in about 0.8% of the bits, a select in constant time in the worst case by a tree of samples of three levels (3*, Kurpicz, Rigi-Luperti and Sanders) | [`succinct/TinyRankSelect.h`](include/ADS/succinct/TinyRankSelect.h) | `engineered_succinct.tiny-rank-select` |
| compressed vector of bits in m H0 + 0.175 m bits with rank and select (Raman, Raman and Rao; Claude and Navarro) | [`succinct/RRRBitVector.h`](include/ADS/succinct/RRRBitVector.h) | `engineered_succinct.rrr-bit-vector` |
| arrays of fields of 1 to 32 bits packed in words, saturated if asked, and of trits five to a byte (Breyer and Korf), built on an executor | [`succinct/PackedArray.h`](include/ADS/succinct/PackedArray.h) | `engineered_search.packed-array`, `engineered_search.trit-array` |
| tANS of order 0 with 2^11 states, two halves of four states each (Duda) | [`compression/Tans.h`](include/ADS/compression/Tans.h) | `engineered_succinct.tans-coder` |
| directly addressable codes with the optimal widths (Brisaboa, Ladra and Navarro) | [`compression/DirectAccessCodes.h`](include/ADS/compression/DirectAccessCodes.h) | `engineered_succinct.direct-access-codes` |
| parse of LZ77 with no window in O(n) time (Karkkainen, Kempa and Puglisi) | [`compression/LZ77Linear.h`](include/ADS/compression/LZ77Linear.h) | `engineered_succinct.lz77-linear` |
| tree of losers with the keys in its nodes, matches with no jump on the integers (Knuth; MCSTL, tlx) | [`priority/MultiwayMerge.h`](include/ADS/priority/MultiwayMerge.h) | `engineered_pearls.flat-loser-tree` |
| merge of k sorted ranges by a lazy funnel of binary mergers (Frigo et al.; Brodal et al.) | [`priority/MultiwayMerge.h`](include/ADS/priority/MultiwayMerge.h) | `engineered_pearls.multiway-merge` |
| cuckoo table shared by threads, buckets of 4 places, locks on 2048 stripes, paths searched in breadth without locks (Li et al.; libcuckoo) | [`concurrent/ConcurrentCuckooTable.h`](include/ADS/concurrent/ConcurrentCuckooTable.h) | `engineered.concurrent-cuckoo`, `engineered.concurrent-cuckoo-threads` |
| cuckoo table shared by threads, buckets of 4 places, one lock per region of buckets, lookups without locks under versions (seqlock), bits of overflow that keep a lookup on one bucket, doublings migrated by the threads that wait, huge pages (Li et al.; Breslow et al.; Maier, Sanders and Dementiev; libcuckoo) | [`concurrent/ConcurrentCuckooTable.h`](include/ADS/concurrent/ConcurrentCuckooTable.h) | `engineered.concurrent-cuckoo`, `engineered.concurrent-cuckoo-threads`, `engineered.concurrent-cuckoo-growth` |
| hash table of linear probing shared by threads, lookups with no lock and no store, find-or-insert with the hash given, pairs in the words, in places of two words by a compare-and-swap of 16 bytes, or in nodes that never move, growth while the threads insert (Maier, Sanders and Dementiev) | [`concurrent/ConcurrentFlatTable.h`](include/ADS/concurrent/ConcurrentFlatTable.h) | `engineered.concurrent-flat-table`, `engineered.concurrent-flat-table-threads`, `engineered.concurrent-flat-table-words` |
| relaxed priority queue shared by threads, the better of two queues, stickiness and buffers, rank errors of 5 n / 6, each queue a heap or a bucket queue on ( f , g ) (Rihani, Sanders and Dementiev; Williams et al.; Walzer and Williams; Zhang et al.) | [`concurrent/MultiQueue.h`](include/ADS/concurrent/MultiQueue.h) | `engineered_queues.multi-queue`, `.multi-queue-buckets`, `.multi-queue-rank`, `.multi-queue-threads` |
| exact priority queue of a worker of which a thief steals an interleaved half, offered by the owner at its next operation, with no read-modify-write for the owner, and the frontier of the workers of a best-first search or branch-and-bound with its termination, with a second key, the bound, whose least over the pairs of the owner is exact (Hendler and Shavit; Acar et al.; Tschöke, Lüling and Monien) | [`concurrent/StealablePriorityQueue.h`](include/ADS/concurrent/StealablePriorityQueue.h) | `engineered_queues.stealable-pq`, `.stealable-pq-threads`, `.stealable-pq-bound`, `.stealable-pq-bound-threads`, `engineered_runtime.stealing-frontier`, `.stealing-frontier-stop`, `.stealing-frontier-bound` |
| exact priority queue shared by threads, linearizable and lock-free, a skiplist whose deleted nodes stay as a prefix cut off in batches, one fetch-and-or per DeleteMin, memory reclaimed by epochs (Lindén and Jonsson; Fraser) | [`concurrent/LockFreePriorityQueue.h`](include/ADS/concurrent/LockFreePriorityQueue.h) | `engineered_queues.lockfree-pq`, `.lockfree-pq-threads`, `.lockfree-pq-linearizable` |
| lock-free queue of many producers, a lane of recycled blocks each | [`concurrent/MPSCQueue.h`](include/ADS/concurrent/MPSCQueue.h) | `engineered_queues.mpsc-queue` |
| bounded queue of many producers and many consumers, a sequence in each place (Vyukov) | [`concurrent/MPMCQueue.h`](include/ADS/concurrent/MPMCQueue.h) | `engineered_queues.mpmc-queue` |
| hash table of linear probing that one thread fills and many read, no read-modify-write per insertion, readers that read on across a growth, clear() in O(1) by epochs | [`concurrent/SingleWriterFlatTable.h`](include/ADS/concurrent/SingleWriterFlatTable.h) | `engineered.single-writer-flat-table`, `engineered.single-writer-flat-table-threads` |
| board of the least keys of p threads, a slot per line of cache, relaxed publishes skipped when unchanged | [`concurrent/MinBoard.h`](include/ADS/concurrent/MinBoard.h) | `engineered_queues.min-board`, `engineered_queues.min-board-threads` |
| event count, the sleep of a thread until a condition holds, a fence and a read when nobody waits | [`concurrent/EventCount.h`](include/ADS/concurrent/EventCount.h) | `engineered_queues.event-count` |
| exponential backoff of a waiting thread, pauses then yields (Anderson) | [`concurrent/Backoff.h`](include/ADS/concurrent/Backoff.h) | `engineered_search.backoff-threads` |
| sparse matrices in compressed rows and in SELL-C-sigma (Kreutzer et al.) | [`numeric/CsrMatrix.h`](include/ADS/numeric/CsrMatrix.h), [`numeric/SellMatrix.h`](include/ADS/numeric/SellMatrix.h) | `engineered_numeric.csr-matrix`, `.sell-matrix` |
| generators xoshiro256++, xoshiro256**, wyrand and the counter-based Philox4x32 and Philox4x64 (Salmon et al.), the alias method, normal and exponential deviates by ziggurats (Marsaglia and Tsang) | [`numeric/RandomGenerators.h`](include/ADS/numeric/RandomGenerators.h) | `engineered_numeric.generators`, `.philox`, `.ziggurat`, `engineered_boost.boost-xoshiro` |
| work-stealing deque (Chase and Lev; Lê et al.) | [`concurrent/WorkStealingDeque.h`](include/ADS/concurrent/WorkStealingDeque.h) | `engineered_queues.work-stealing-deque` |
| wavelet tree shaped by the code of Huffman, n(H0+1) bits (Makinen and Navarro) | [`succinct/HuffmanWaveletTree.h`](include/ADS/succinct/HuffmanWaveletTree.h) | `engineered_succinct.huffman-wavelet-tree` |
| FM-index on the wavelet tree of Huffman, the succinct suffix array (Makinen and Navarro) | [`compression/HuffmanFMIndex.h`](include/ADS/compression/HuffmanFMIndex.h) | `engineered_succinct.huffman-fm-index` |
| partitioned Elias-Fano, blocks full, as vectors or by Elias-Fano (Ottaviano and Venturini) | [`succinct/PartitionedEliasFano.h`](include/ADS/succinct/PartitionedEliasFano.h) | `engineered_succinct.partitioned-elias-fano` |
| binary packing SIMD-BP128 and its patched form PFor (Lemire and Boytsov; Zukowski et al.) | [`compression/BinaryPacking.h`](include/ADS/compression/BinaryPacking.h) | `engineered_succinct.binary-packing` |
| r-index: count and locate in space proportional to the runs of the transform (Gagie, Navarro and Prezza) | [`compression/RIndex.h`](include/ADS/compression/RIndex.h) | `engineered_succinct.r-index` |
| move structure, a permutation of shifted intervals in O(1) a step (Nishimoto and Tabei) | [`succinct/MoveStructure.h`](include/ADS/succinct/MoveStructure.h) | `engineered_succinct.move-structure` |
| r-index on move structures (Bertram, Fischer and Nalbach) | [`compression/MoveRIndex.h`](include/ADS/compression/MoveRIndex.h) | `engineered_succinct.move-r-index` |
| LA-vector, a sorted sequence as the segments of an optimal piecewise linear approximation and the corrections of the items, with access, rank and select (Boffa, Ferragina and Vinciguerra) | [`succinct/LAVector.h`](include/ADS/succinct/LAVector.h) | `engineered_succinct.la-vector` |
| compressed PGM-index, the slopes shared by the stabbing of their intervals and the keys and the places by Elias-Fano, and dynamic PGM-index by the logarithmic method (Ferragina and Vinciguerra) | [`search/CompressedPGMIndex.h`](include/ADS/search/CompressedPGMIndex.h), [`search/DynamicPGMIndex.h`](include/ADS/search/DynamicPGMIndex.h) | `ordered.pgm-compressed`, `ordered.pgm-dynamic` |
| static dictionaries of strings: the rear coding with buckets in one string of bytes (Martínez-Prieto et al.) and the Patricia trie in DFUDS, the strings numbered in lexicographic order (Benoit et al.) | [`string/RearCoding.h`](include/ADS/string/RearCoding.h), [`dictionary/SuccinctPatriciaTrie.h`](include/ADS/dictionary/SuccinctPatriciaTrie.h) | `engineered_strings.string-dictionaries` |

## Sorting

Beside the sorts of the book, ADS has the classical ones that the book
leaves out and the engineered ones of the current libraries. They are tested
in [`test/sorting.cpp`](test/sorting.cpp) on all the permutations of up to 7
items and on the patterns that usually defeat a sort (sorted, reversed,
sawtooth, organ pipe, few distinct keys), and measured by
[`bench/sort.cpp`](bench/sort.cpp) against `std::sort`, `std::stable_sort`
and Boost.Sort (`make bench`, or CMake with `-DADS_BENCHMARKS=ON`).
`ADS::sort()` and `ADS::stableSort()`
([`sort/Sort.h`](include/ADS/sort/Sort.h)) select among them by the type of
the items and by the order (the radix sort for numbers, the MSD radix sort
for strings, and pdqsort and Powersort for the rest), with thresholds chosen
on the measures of [`results/`](results/README.md).

| What | Header |
|---|---|
| bubble, cocktail shaker, odd-even transposition, comb and gnome sort | [`sort/ExchangeSorts.h`](include/ADS/sort/ExchangeSorts.h) |
| tree sort, strand sort | [`sort/TreeSort.h`](include/ADS/sort/TreeSort.h), [`sort/StrandSort.h`](include/ADS/sort/StrandSort.h) |
| sorting networks of Batcher (merge exchange, bitonic) | [`sort/SortingNetworks.h`](include/ADS/sort/SortingNetworks.h) |
| pdqsort, dual-pivot and three-way quicksort | [`sort/PdqSort.h`](include/ADS/sort/PdqSort.h), [`sort/EngineeredQuickSorts.h`](include/ADS/sort/EngineeredQuickSorts.h) |
| Powersort | [`sort/RunSorts.h`](include/ADS/sort/RunSorts.h) |
| LSD radix sort of numbers and by a key, American flag sort, MSD radix sort of strings | [`sort/RadixSort.h`](include/ADS/sort/RadixSort.h) |
| multikey quicksort of strings | [`sort/MultikeyQuickSort.h`](include/ADS/sort/MultikeyQuickSort.h) |
| sort of strings by integer keys of 8 characters, sorted in the vector registers | [`sort/StringSort.h`](include/ADS/sort/StringSort.h) |
| learned sort of numbers, buckets by a model of the CDF of a sample, sequential and on the threads of an executor (Kristo et al.; Ferragina and Odorisio) | [`sort/LearnedSort.h`](include/ADS/sort/LearnedSort.h), [`parallel/ParallelSort.h`](include/ADS/parallel/ParallelSort.h) |

## Parallelism

The parallel algorithms of ADS take an *executor* as their first argument,
i.e., any object that runs the calls `f( 0 )`, ..., `f( n - 1 )` of a
function on its threads and returns when they have returned (`bulk()`), and
that returns the number of its threads (`concurrency()`). The sequential
algorithms, which take none, are unchanged; an algorithm called with one
thread, or on a "small" input, is the sequential one. The executors are
`Sequential` and `ThreadPool`, the pool of ADS on `std::thread` (a deque of
work stealing of Chase and Lev per thread, the idle threads asleep on an
event count after 2 ms). As options, `OpenMPExecutor`, `TBBExecutor` and
`FastFlowExecutor` run the same calls on the threads of OpenMP, oneTBB and
FastFlow. On them, [`parallel/Executor.h`](include/ADS/parallel/Executor.h)
builds the loops, the reductions and the prefix sums in one pass over the
memory (the decoupled look-back of Merrill and Garland). Since the blocks of
a reduction do not depend on the threads, a sum of doubles is the same on
any executor. In turn, the other headers of
[`parallel/`](include/ADS/parallel) give the algorithms below, each with the
results of its sequential version (bit for bit for the products of matrices,
up to the rounding for the transform).

| What | Header |
|---|---|
| executors: the pool of ADS, OpenMP, oneTBB, FastFlow | [`parallel/ThreadPool.h`](include/ADS/parallel/ThreadPool.h), [`parallel/OpenMPExecutor.h`](include/ADS/parallel/OpenMPExecutor.h), [`parallel/TBBExecutor.h`](include/ADS/parallel/TBBExecutor.h), [`parallel/FastFlowExecutor.h`](include/ADS/parallel/FastFlowExecutor.h) |
| executors on the runtimes of a program: the parallel algorithms of the standard library, the senders of stdexec, Taskflow, Kokkos | [`interop/parallel/StdParExecutor.h`](include/ADS/interop/parallel/StdParExecutor.h), [`interop/parallel/SenderExecutor.h`](include/ADS/interop/parallel/SenderExecutor.h), [`interop/parallel/TaskflowExecutor.h`](include/ADS/interop/parallel/TaskflowExecutor.h), [`interop/parallel/KokkosExecutor.h`](include/ADS/interop/parallel/KokkosExecutor.h) |
| parallel loops, reductions and prefix sums, fork and join | [`parallel/Executor.h`](include/ADS/parallel/Executor.h) |
| the processors of the process and the pinning of a thread to one of them | [`parallel/Affinity.h`](include/ADS/parallel/Affinity.h) |
| `sort()` and `stableSort()` by distribution (radix classifier for the numbers, tree of splitters with buckets of equality for the rest) | [`parallel/ParallelSort.h`](include/ADS/parallel/ParallelSort.h) |
| `multiwayMerge()` by exact multisequence selection | [`parallel/ParallelMerge.h`](include/ADS/parallel/ParallelMerge.h) |
| products of `CsrMatrix`, `SellMatrix` and `gemm()` | [`parallel/ParallelNumeric.h`](include/ADS/parallel/ParallelNumeric.h) |
| visit in breadth that optimizes its direction, connected components by Afforest | [`parallel/ParallelGraph.h`](include/ADS/parallel/ParallelGraph.h) |
| shortest paths by the Delta-stepping of Meyer and Sanders, on buckets of each thread and two barriers a step, and by the rho-stepping of Dong, Gu, Sun and Zhang, with the fusion of the buckets | [`parallel/ParallelShortestPaths.h`](include/ADS/parallel/ParallelShortestPaths.h) |
| builds of the directories of rank and select, of the wavelet matrix (up to 8 levels per pass, by the reversed prefixes of the bits) and of the wavelet tree of Huffman | [`succinct/RankSelect.h`](include/ADS/succinct/RankSelect.h), [`succinct/CompactRankSelect.h`](include/ADS/succinct/CompactRankSelect.h), [`succinct/WaveletMatrix.h`](include/ADS/succinct/WaveletMatrix.h), [`succinct/HuffmanWaveletTree.h`](include/ADS/succinct/HuffmanWaveletTree.h) |
| LCP array by the permuted LCP | [`parallel/ParallelText.h`](include/ADS/parallel/ParallelText.h) |
| union-find with no lock, for many threads | [`concurrent/ConcurrentUnionFind.h`](include/ADS/concurrent/ConcurrentUnionFind.h) |
| balanced trees whose only balancing is join (AVL, Red-Black, by weight, treap): split, join, union, intersection and difference with the optimal work, batches of insertions and removals, filter, map and reduce, augmented maps with the sums of ranges, assignments by the differences of a group, selection by a prefix and filter by the aggregates, typed modifications at no cost when nobody listens, aggregates kept by an observer, the interval tree and the 2D range tree | [`tree/JoinAlgorithms.h`](include/ADS/tree/JoinAlgorithms.h), [`tree/JoinTree.h`](include/ADS/tree/JoinTree.h), [`tree/MaterializedAggregate.h`](include/ADS/tree/MaterializedAggregate.h), [`geometry/IntervalTree.h`](include/ADS/geometry/IntervalTree.h), [`geometry/RangeTree.h`](include/ADS/geometry/RangeTree.h) |
| deterministic reservations, `speculativeFor()` | [`parallel/SpeculativeFor.h`](include/ADS/parallel/SpeculativeFor.h) |
| Kruskal and filter-Kruskal, spanning forest, maximal independent set and maximal matching, the results of the sequential greedy algorithms | [`parallel/ParallelGreedy.h`](include/ADS/parallel/ParallelGreedy.h) |
| long-lived workers on every executor (`run()`, `lock_type`), the channels between them (inboxes under a lock, stacks of batches, lanes, the mesh of the pairs), the farm with feedback, and the mailbox that detects the termination, merges the messages of one key in its outboxes and audits the consumer at a quiescent point | [`parallel/Executor.h`](include/ADS/parallel/Executor.h), [`parallel/Channels.h`](include/ADS/parallel/Channels.h), [`parallel/Farm.h`](include/ADS/parallel/Farm.h), [`parallel/Mailbox.h`](include/ADS/parallel/Mailbox.h) |
| sort over the processes of MPI by regular sampling | [`distributed/DistributedSort.h`](include/ADS/distributed/DistributedSort.h) |
| the processes of MPI as workers: their executor, the mailbox of batches sent without blocking with the four counters of Mattern for the termination, the minimum of a bound by reductions that do not block | [`distributed/MPIExecutor.h`](include/ADS/distributed/MPIExecutor.h), [`distributed/MPIMailbox.h`](include/ADS/distributed/MPIMailbox.h) |

```cpp
#include "ADS/parallel/ParallelSort.h"
#include "ADS/parallel/ThreadPool.h"

ADS::ThreadPool pool( 8 );               // 8 threads, the caller included
ADS::sort( pool , A );                   // the result of ADS::sort( A )
```

While the pool needs only the threads of the standard library (with CMake,
`ADS::ADS` links them), the other executors are compiled only on request.
With CMake, `-DADS_WITH_OPENMP=ON`, `-DADS_WITH_TBB=ON` and
`-DADS_WITH_FASTFLOW=ON` (all off by default) link OpenMP and oneTBB and
take the headers of FastFlow from `FASTFLOW_DIR` or, if it is not given,
from its repository. Without CMake, the flags are `-fopenmp`,
`-DADS_WITH_TBB -ltbb` and `-DADS_WITH_FASTFLOW -I<fastflow>`. Note that the
header of FastFlow defines `NO_DEFAULT_MAPPING` (unless
`ADS_FASTFLOW_MAPPING` is defined), since FastFlow otherwise pins its
threads to the first cores of the machine whatever the affinity of the
process.

A program that already runs on one of these runtimes may give its threads to
the algorithms, and hence need not start the pool of ADS beside them.
`TBBExecutor` runs on a separate arena of `p` threads, on an arena of the
program (`TBBExecutor ex( arena )`, whose constraints of NUMA node, type of
core and threads then hold for ADS too) or on the arena of each caller
(`TBBExecutor ex( TBBExecutor::current )`). `OpenMPExecutor`, called inside
a region `omp parallel` of the program, makes its calls the tasks of a
`taskloop` of the team that is there, where a nested region would get one
thread. The headers of [`interop/parallel/`](include/ADS/interop/parallel),
each compiled where the headers of its runtime are found, add four more
executors. `StdParExecutor< Policy >` runs on the parallel algorithms of the
standard library (`std::for_each` of `std::execution::par` on the indices)
and `SenderExecutor< Sch >` on a scheduler of the senders of stdexec (the
`std::execution` of C++26). Finally, `TaskflowExecutor` runs on a
`tf::Executor` and `KokkosExecutor< Space >` on a space of the host of
Kokkos:

```cpp
#include "ADS/interop/parallel/TaskflowExecutor.h"

tf::Executor workers( 8 );               // the executor of the program
ADS::TaskflowExecutor ex( workers );
ADS::sort( ex , A );                     // on the workers of Taskflow
```

The executors on oneTBB, on OpenMP and on the standard library add nothing
to the calls of their runtime, and their calls nest as its tasks do. Those
on the senders and on Kokkos run a call of `bulk()` made inside one of their
calls in order on its thread, since a `sync_wait()` and a dispatch of Kokkos
hold the thread that waits. Hence, the fork and join of ADS (the recursion
of `parallelInvoke()`, the trees by join, the lower levels of the sorts)
then lose their nested parallelism, while the loops, the reductions, the
prefix sums and the top level of the sorts keep theirs. The executor on
Taskflow nests, by co-running the nested graph on the worker that waits, but
it builds a graph at each call. None of the four has `run()`, since their
runtimes do not promise that the calls of a loop run at the same time. With
CMake, `ADS_WITH_STDPAR`, `ADS_WITH_STDEXEC`, `ADS_WITH_TASKFLOW` and
`ADS_WITH_KOKKOS` are `AUTO` (cf. section Build and test, which also gives
the variables of the makefile). Each runtime found gives a target,
`ADS::StdPar`, `ADS::stdexec`, `ADS::Taskflow` or `ADS::Kokkos`, with the
macro of its header, the runtime and `ADS::ADS`, which carries none of them.
An installed ADS gives the same targets as components,
`find_package(ADS COMPONENTS Taskflow)`, found again where the runtime was;
the hints are `STDEXEC_INCLUDE_DIR`, `Taskflow_INCLUDE_DIR` and the
`CMAKE_PREFIX_PATH` of Kokkos.

The calls of `bulk()` are tasks, which a thread may run one after the other.
A computation of workers that live as long as it does and exchange messages
(e.g., a parallel search, or a master and its workers) runs instead on
`run( p , f )`, whose `p` calls run at the same time on `p` threads of any
executor. Each executor has its lock (`lock_type`: `std::mutex`,
`omp_lock_t`, `tbb::spin_mutex`, the spin lock of FastFlow). On `run()`, the
channels of [`parallel/Channels.h`](include/ADS/parallel/Channels.h) give
each worker an inbox for batches of messages, in four families with one
interface, and `feedbackFarm()` is the farm with feedback of FastFlow, a
farm of FastFlow on its executor and emulated on the others. The `Mailbox`
adds outboxes of batches and detects, for each worker, the end of the whole
computation, by a counter of the activities and of the messages touched once
per batch. No send waits, since what a full queue refuses stays in its
outbox for a later push. `stop()` ends the computation at once, and
`leftovers()` then hands back each message that no poll has handed, exactly
once (e.g., the nodes that a search stopped by a limit puts back into its
frontier). The locks are Lockable (`try_lock()` included), the farm gives
each worker and the master an index for a private state, and `isRunCaller()`
returns whether a worker runs on the thread that called `run()`.

By `MPIExecutor` and `MPIMailbox`, the processes of MPI are workers too,
with the same stop and the same `leftovers()`. Their batches are sent
without blocking, and their termination is detected by the four counters of
Mattern on reductions that do not block. Messages that are not trivially
copyable go through `MPICodec`, and messages of bytes with a tag through
`MPIByteMailbox`. MPI is a runtime like OpenMP and oneTBB, and its option is
off by default, since `ADS::ADS` built with it carries MPI to any program
that links it. With CMake, `ADS_WITH_MPI` is `OFF` by default, `AUTO` to
turn it on when FindMPI finds Open MPI or MPICH and `ON` to require it (for
the makefile, cf. section Build and test). A program of Boost.MPI passes its
`boost::mpi::communicator`, which converts to `MPI_Comm`, and
[`distributed/BoostMPI.h`](include/ADS/distributed/BoostMPI.h) gives
`BoostMPICodec< Msg >`, which turns a message into bytes by
Boost.Serialization for the mailbox (with CMake the target `ADS::BoostMPI`,
`ADS_WITH_BOOST_MPI` `AUTO`, and `make test-boost-mpi`). Both the sort over
the processes, `ADS::distributed::sort()`, and the runtime are tested on 1
to 4 processes by `ctest` or by `make test-mpi`.

Being also methods of `AVLTree`, `Treap` and `RedBlackTree`, the operations
of the trees based on join are tested, with their edge cases, against
`std::set` and `std::map` in [`test/join_trees.cpp`](test/join_trees.cpp).
In turn, [`bench/joins.cpp`](bench/joins.cpp) compares them with the
operations of `std::map` one key at a time and, when compiled with
`PAM=<dir>`, with `pam_map`.

The benchmark of the parallel algorithms,
[`bench/parallel.cpp`](bench/parallel.cpp), runs each of them on 1, 2, 4 and
8 threads. Its competitors are the parallel versions of oneTBB, of
`std::execution` (libstdc++ on oneTBB), of the parallel mode of libstdc++,
of OpenMP, of ParlayLib, of IPS4o and IPS2Ra, of FastFlow (its
`ParallelFor`, `ParallelForReduce` and `ff_DC`), of tlx, of Eigen and
OpenBLAS and of the GAP benchmark suite. For the hash tables, they are the
concurrent tables of oneTBB, libcuckoo, ParlayLib and growt, and for the
priority queues the concurrent one of oneTBB, the reference MultiQueue of
Williams, Sanders and Dementiev and the k-LSM. For the branch-and-bound by
best first, they are a `std::priority_queue` under a lock per worker, of
which a thief takes half, and the shared queues of oneTBB and of ADS. Each
structure runs in a separate process, since the threads that a runtime keeps
spinning between its calls may slow down the next structure. This benchmark
was run once on cores 184 to 191 of the EPYC 9654 of section Performance,
which share the third level of the cache, with their SMT siblings left idle,
11 runs per cell and the same checks on the other processes.

All the ratios below are those of 8 threads, i.e., the time of the library
divided by that of the pool of ADS (the median, and the range over the cells
in parentheses). In the median, the sorts of ADS are faster than oneTBB,
`std::execution` and the parallel mode of libstdc++ by 1.8 to 4.6 times, and
than ParlayLib by 1.6 to 3.9 times. Against IPS4o they are on a par on
64-bit integers (1.01, 0.23 to 3.98), 1.4 times faster on doubles and 1.1
times slower on strings (0.89, 0.38 to 0.94). The stable sorts are 1.7 to
5.9 times faster than those of ParlayLib, `std::execution` and the parallel
mode. Furthermore, the parallel `multiwayMerge()` is 1.5 to 1.6 times faster
than those of tlx and libstdc++, the LCP array 1.9 times faster than that of
ParlayLib, and the connected components 1.9 times faster than Afforest of
the GAP benchmark suite. In addition, the prefix sums are 1.2 to 1.4 times
faster than those of OpenMP, ParlayLib and oneTBB, since they read the input
once. The parallel builds of the wavelet matrix and of the Huffman wavelet
tree are 40 and 9 times faster than the sequential ones of sdsl. Also,
`gemm()` is on a par with OpenBLAS, which it calls, and the delta-stepping
with that of the GAP benchmark suite (1.02, 0.58 to 1.91). It is fair to
remark, however, that the visit in breadth of the GAP benchmark suite, which
follows the same algorithm, is 1.7 times faster than `bfsDistances()` (0.59,
0.50 to 0.65). Similarly, the reduction and the map of OpenMP are 1.2 to 1.3
times faster than those of the pool (0.78 and 0.84, on 2 cells each). In the
hash tables, the open addressing of ParlayLib (on keys only) inserts and
finds keys about 3 times faster than `ConcurrentCuckooTable` (0.36 and
0.34). The cuckoo table of ADS is also 1.3 times slower than libcuckoo and
oneTBB when the table grows, but faster than both on the other operations
(1.05 to 4.2). On the executor of oneTBB the same algorithms run within a
few percent of the pool, and on those of OpenMP and FastFlow too in most
cells. However, the sorts of strings are about 1.8 times faster on these two
executors than on the pool (0.52 to 0.56), and so are the visit in breadth
and the connected components, by 1.4 to 1.8 times (0.57 to 0.74).

## Design by contract

ADS follows the design by contract of B. Meyer, in which each operation
checks its specification as a contract
([`Contract.h`](include/ADS/Contract.h)). A contract is of one of four
kinds: `ADS_PRE` for the preconditions of the book, `ADS_POST` for the
postconditions, `ADS_INVARIANT` for the invariant of a structure, and
`ADS_AUDIT` for a condition that typically costs more than the operation
(e.g., that a vector is sorted before a binary search, or that a write left
the other items as they were). Which of them are evaluated is set by the
macro `ADS_CONTRACTS` (0 none, 1 the preconditions, which is the default, 2
also the postconditions, 3 also the invariants and the audits). With
`NDEBUG` defined, as in a release build, the default is 0, as for `assert`.
A violated contract throws `ADS::contract_violation`, a `std::logic_error`
whose message gives the condition, the function and the place. Tests are
built at the level 3, and the axioms of each abstract data type are checked
on all its realizations (`contract_*.*-axioms`), e.g.,
`top( push( S , v ) ) = v` and `pop( push( S , v ) ) = S` for a stack.

## The standard interface

Beside the operations of the book, the containers have the interface of the
standard library, i.e., `begin()` and `end()`, the usual member types, and
iterators of the strongest category that the structure allows, both ends
having the same type, as the algorithms of C++17 and of Boost require. Thus,
one can run a range-for, the algorithms of `<algorithm>` and of
`std::ranges`, and those of Boost.Range and Boost.Sort on the containers of
ADS as on those of the standard library. Where the items can change in place
(the sequences, the deque, the arena) the iterators are mutable. A heap, an
ordered set or a dictionary keeps an invariant on its items, and therefore
its iterators are constant, the operations of the book being the only safe
way to change it. Several containers share the iterators of
[`Iterators.h`](include/ADS/Iterators.h). In
[`test/interop.cpp`](test/interop.cpp), the tests check the category of each
iterator at compile time and run the algorithms of std (and of Boost, when
it is installed) on each container against its operations.

| Category | Containers |
|---|---|
| contiguous | `ArraySequence` (mutable), `ArrayStack`, `DaryHeap`, `SortedArrayDictionary`, the adjacent nodes of `ArrayGraph` |
| random access | `ArrayQueue` (on the circular vector), `HeapPriorityQueue`, `Arena` (mutable), the values of `PreorderArrayTree`, the nodes of `ArrayGraph` |
| bidirectional | `List` and `Dequeue` (mutable), `UniqueQueue`, `ListPriorityQueue`, `TreePriorityQueue`, the sets on a list, `LinkedHashSet`, `BoolSet`, the open-addressing tables, `DirectTable`, `PointerTable`, the search trees, `AVLTree`, `Treap`, `SplayTree`, `ThreadedTree`, `BPlusTree`, `LinkedHeapPriorityQueue`, the nodes and the adjacent nodes of `ListGraph` and `MatrixGraph`, the nodes of the cursor trees |
| forward | `SinglyList` and `CursorList` (mutable), `LinkedQueue`, `SequenceStack`, the stacks of `MultiStack`, `BucketQueue`, `ChainedHashTable` and the sets on it, `HashMultiset`, `SkipList`, `Trie`, `RadixTree`, `BTree`, `FibonacciHeapPriorityQueue`, the children and the preorder of `Tree`, the preorder and the symmetric order of `BinaryTree`, the children of `CursorTree`, the rows and the columns of `SparseMatrix` |

A dictionary gives the pairs `( key , value )` and a priority queue the
pairs `( priority , item )`, in the order of the structure (i.e., the order
of the keys in a search tree or a trie, of the places in a hash table, of
the heap in a heap). In some containers the item is built on the fly (a pair
of references, a key rebuilt along a trie, the number of a node). The
iterator then models the concept of C++20, but it declares the category of
an input iterator to the older algorithms, as the views of the standard
library do. Hence, with these iterators one should use `std::ranges::prev`
and the algorithms of `std::ranges` in place of `std::prev` and of those of
`<algorithm>`.

## Using ADS with other libraries

A program written on another library can call the algorithms of that library
on the structures of ADS, and those of ADS on its structures, usually by
changing one type. [Appendix
C](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html) of the
manual gives an example of each case, and the examples are also the tests
[`interop_examples`](test/interop_examples.cpp) and
[`interop_examples_mpi`](test/interop_examples_mpi.cpp).

| Library | What works | Option | Examples |
|---|---|---|---|
| standard library | `std::flat_map` on `TieredVector`, `std::ranges` on the dictionaries, the sorts on any range, the pools as `std::pmr` resources, `std::priority_queue` as a frontier | none | [C.1](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-1) |
| Boost.Graph | its algorithms on `StaticGraph`, `ArrayGraph`, `ListGraph`; the algorithms of ADS on its graphs; the heaps of ADS in its Dijkstra | `ADS_WITH_BOOST` | [C.2](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-2) |
| Boost.Heap, Boost.Unordered | its heaps as frontiers of ADS; `FastHash` as its hasher | `ADS_WITH_BOOST` | [C.3](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-3) |
| LEMON | the heaps and maps of ADS in its algorithms, the graphs both ways, its notifier told to the observers of ADS | `ADS_WITH_LEMON`, `ADS::LEMON` | [C.4](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-4) |
| GMP | `BigInteger` and `Rational` to and from `mpz_class` and `mpq_class` | `ADS_WITH_GMP` | [C.5](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-5) |
| BLAS | any CBLAS on `DenseMatrix`, `gemm()` on the CBLAS | `ADS_WITH_BLAS` | [C.6](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-6) |
| oneTBB, OpenMP | the parallel algorithms in an arena or a region of the program | `ADS_WITH_TBB`, `ADS_WITH_OPENMP` | [C.7](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-7), [C.8](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-8) |
| std::execution, stdexec, Taskflow, Kokkos | the parallel algorithms on their threads | `ADS_WITH_STDPAR`, `ADS_WITH_STDEXEC`, `ADS_WITH_TASKFLOW`, `ADS_WITH_KOKKOS` | [C.9](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-9) to [C.12](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-12) |
| MPI, Boost.MPI | the distributed sort and mailbox on a communicator of the program | `ADS_WITH_MPI`, `ADS_WITH_BOOST_MPI` | [C.13](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-13) |

For instance, the algorithm of Dijkstra of Boost.Graph runs on a graph of
ADS with no copy.

```cpp
#include "ADS/interop/boost/Graph.h"

const ADS::StaticGraph< double > g( n , arcs );    // was a CSR graph of Boost
boost::dijkstra_shortest_paths( g , s , boost::weight_map(
  get( boost::edge_weight , g ) ).distance_map( d.data() ) );
```

Similarly, that of LEMON runs on a heap of ADS.

```cpp
#include "ADS/interop/lemon/Heaps.h"

using H = ADS::lemon_interop::DaryHeap< int , GR::NodeMap< int > >;
lemon::Dijkstra< GR , LM >::SetStandardHeap< H >::Create d( g , l );
```

Finally, the parallel sort of ADS runs inside an arena of oneTBB of the
program.

```cpp
#include "ADS/parallel/TBBExecutor.h"

ADS::TBBExecutor ex( arena );                      // was ADS::ThreadPool
ADS::sort( ex , A );
```

Each of the other libraries has an option, `AUTO` by default (cf. section
Build and test). `ADS_WITH_CEREAL` (hint `cereal_INCLUDE_DIR`) gives
`ADS::cereal`, the headers of cereal under `ADS/interop/cereal/Cereal.h`, by
which the persistent structures of `ADS/external/Persistence.h` (saved and
read back by `store()` and `restore()`, or opened in place by
`restoreMapped()`) also go into the archives of cereal. Likewise,
`ADS_WITH_EIGEN`, `ADS_WITH_METIS`, `ADS_WITH_KAHIP`, `ADS_WITH_GRAPHBLAS`
and `ADS_WITH_NETWORKIT` give `ADS::Eigen`, `ADS::METIS`, `ADS::KaHIP`,
`ADS::GraphBLAS` and `ADS::NetworKit`, under the headers of
`ADS/interop/eigen/`, `metis/`, `kahip/`, `graphblas/` and `networkit/`, by
which the algorithms of these libraries run on the structures of ADS. The
matrices of ADS become maps of Eigen and read-only matrices of GraphBLAS,
and METIS and KaHIP partition the rows of a `StaticGraph`, with no copy
where the layouts agree (and a copy where they do not). Also, a graph of
NetworKit is kept in step with one of ADS, whose changes reach the dynamic
algorithms of NetworKit. Their hints are `EIGEN3_INCLUDE_DIR`,
`<Lib>_INCLUDE_DIR` and `<Lib>_LIBRARY`, and `tlx_INCLUDE_DIR` for
NetworKit. In the same way, `ADS_WITH_CROARING` (hint `roaring_DIR`, or
`CRoaring_INCLUDE_DIR` and `CRoaring_LIBRARY`) gives `ADS::CRoaring`, under
which `ADS/interop/croaring/CRoaring.h` converts a `RoaringBitmap` to and
from the bitmaps of CRoaring. Similarly, `ADS_WITH_SDSL` (hint
`sdsl_INCLUDE_DIR`) gives `ADS::sdsl`, under which `ADS/interop/sdsl/Sdsl.h`
converts a `BitVector` and a `PackedArray` to and from the `bit_vector` and
the `int_vector` of sdsl-lite 3. Finally, `ADS/interop/arrow/Arrow.h`, which
hands the sequences of ADS to Arrow by its C Data Interface, needs no
library.

From Python, the package `ads-python` of [`python/`](python/) binds on
nanobind the structures and the algorithms of ADS that NumPy, SciPy and
NetworkX lack. These are the graphs in compressed rows with their parallel
algorithms, the succinct structures, the filters, `RoaringBitmap` in the
portable format that pyroaring reads, the indexed heaps, and a graph that
reports its changes as the typed events of ADS. Each array of a structure is
a read-only view of NumPy, and each static structure may be opened in place
from its file by `restore_mapped()`. The package also registers `nx-ads`, a
backend of NetworkX by which the shortest paths, the visits and the
components of NetworkX run on ADS. Its wheels (one per platform, on the
stable ABI of Python 3.12 and later) and its sdist are attached to each
release, to be installed by `pip install <file>`. From a checkout, it is
built by `pip install python/` or by CMake under `ADS_WITH_PYTHON` (`AUTO`,
i.e., on where a Python with its headers and nanobind are found), whose
ctest `python.pytest` runs its tests. An example of its use is in
[§C.21](https://dmeoli.github.io/ADS/manual/chapters/C-libraries.html#sec-C-21)
of the manual.

```python
import ads, networkx as nx

g = ads.WeightedStaticGraph.restore_mapped("roads.ads")  # O(1), no copy
d, parent = ads.shortest_paths(g, 0)
nx.single_source_dijkstra_path_length(G, 0, backend="ads")
```

## Manual

The manual is in [`manual/`](manual/), with one chapter for each area of
`include/ADS`. Each chapter gives the definitions and the invariants of each
structure, figures of its state in memory, the pseudocode and the code of
its operations, the analysis of their costs and exercises tied to the tests.
It comes in two forms, generated from the same Markdown sources in
`manual/chapters`:

- PDF, a single file, [ADS - A User Manual (PDF)](manual/ADS-manual.pdf),
  also attached to each release;
- Web, <https://dmeoli.github.io/ADS/manual/>.

## Further reading

Besides the book, the headers cite the textbooks and the papers from which
the structures and the algorithms beyond it come. Textbooks that cover much
of the same ground are

- T. H. Cormen, C. E. Leiserson, R. L. Rivest, C. Stein, *Introduction to
  Algorithms*, 3rd ed., MIT Press, 2009;
- C. Demetrescu, I. Finocchi, G. F. Italiano, *Algoritmi e strutture
  dati*, 2nd ed., McGraw-Hill, 2008;
- R. Sedgewick, *Algorithms in C++, Parts 1-4*, 3rd ed., Addison-Wesley,
  1998;
- A. V. Aho, J. E. Hopcroft, J. D. Ullman, *Data Structures and
  Algorithms*, Addison-Wesley, 1983;
- D. E. Knuth, *The Art of Computer Programming*, vol. 1 (3rd ed., 1997),
  vol. 2 (3rd ed., 1998), vol. 3 (2nd ed., 1998), vol. 4A (2011) and
  vol. 4B (2022), Addison-Wesley;
- P. Ferragina, *Pearls of Algorithm Engineering*, Cambridge University
  Press, 2023;
- B. Meyer, *Object-Oriented Software Construction*, 2nd ed., Prentice
  Hall, 1997, for the design by contract;
- M. Cadoli, M. Lenzerini, P. Naggar, A. Schaerf, *Fondamenti della
  progettazione dei programmi: principi, tecniche e loro applicazioni in
  C++*, CittàStudi, 1997, for the specification of the abstract data types
  and their realization in C++.

## Build and test

With CMake,

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

Alternatively, the makefile builds and runs the same tests (with `-j4`, four
programs at a time):

```sh
make -j4 test
```

One can run a single exercise with `ctest -R ch04.ex4.4`, and a test program
also runs one test by name (`build/test/ch04 ex4.4`) and lists its tests
(`-l`). CMake builds in `Release` when no build type is given, while the
makefile builds in `BUILD` (`build-make`) with `CXX` (`g++`) and `CXXFLAGS`
(`-std=c++20 -O2 -Wall -Wextra` by default). In the CI, both are tested with
g++ and clang++ on Linux, macOS (the clang++ of Xcode and the g++ of
Homebrew) and Windows (those of MinGW-w64 in MSYS2), and CMake also with
MSVC.

The optional libraries follow one convention. Each is turned on by an option
`ADS_WITH_<X>`, of the same name for CMake and for the makefile. It is
`AUTO` (`auto` for the makefile) to turn the library on where it is found,
`ON` (`1`) to require it, with an error at configure time if it is missing,
and `OFF` (`0`) to leave it out. All of these options are `AUTO` by default,
except those of OpenMP, oneTBB, FastFlow and MPI, which are off (cf. section
Parallelism), and the packages are built with all of them off (cf. section
Use).

The makefile takes ADS from `lib/makefile-c`, which defines `ADSH` (the
headers), `ADSINC` (the `-I`, the `-D` and the options of the backends
turned on), `ADSLIB` (their libraries, `-pthread`, and `-latomic` where
the compiler needs it) and `ADSOBJ`, the object of the C++20 module when it
is built. Of the other files, `lib/makefile` gives only the headers,
`lib/makefile-OS` the settings of the operating system and of the compiler,
and `lib/makefile-mod` the module.
GMP, a CBLAS, the headers of Boost and Highway are used wherever a program
that calls them compiles and links (`ADS_WITH_GMP`, `ADS_WITH_BLAS`,
`ADS_WITH_BOOST` and `ADS_WITH_HIGHWAY` are `auto`), and `make backends`
reports what was found. Instead, the other optional backends are turned on
by `ADS_WITH_OPENMP=1`, `ADS_WITH_TBB=1`, `ADS_WITH_FASTFLOW=1` and
`ADS_WITH_MPI=auto` or `1` (whose flags come from `mpicxx`, or from
`MPICXX`), as the CMake options of the same names are. As for the runtimes
of a program (`ADS_WITH_STDPAR`, `ADS_WITH_STDEXEC`, `ADS_WITH_TASKFLOW`,
with `STDEXEC_ROOT=<dir>` and `Taskflow_ROOT=<dir>`), they are `auto` and go
into `ADSINTEROPINC` and `ADSINTEROPLIB` only, whereas Kokkos is available
only with CMake, whose package carries its flags. In the same way, the
archives of cereal for the persistent structures (`ADS_WITH_CEREAL`,
`cereal_ROOT=<dir>`) go into `ADSCEREALINC`, and CRoaring and sdsl-lite
(`ADS_WITH_CROARING` and `ADS_WITH_SDSL`, `CRoaring_ROOT=<prefix>` and
`sdsl_ROOT=<dir>`) into `ADSCOLUMNARINC` and `ADSCOLUMNARLIB`, all of them
`auto`. The libraries are looked for where
`extlib/makefile-default-paths-<os>` puts them, i.e., GMP, oneTBB and libomp
in the prefix of Homebrew on macOS and where the compiler looks elsewhere,
the CBLAS of Accelerate on macOS, and FastFlow in `/opt/fastflow`,
`/Library/fastflow` or `C:/fastflow`. A copy of that file as
`extlib/makefile-paths`, which git ignores, points them elsewhere.

The documentation, at [dmeoli.github.io/ADS](https://dmeoli.github.io/ADS/),
is a site of Sphinx with the theme of Read the Docs, whose pages come from
this README and whose API reference Breathe builds from the comments of the
headers, extracted by Doxygen, with the Doxygen site in its folder
`doxygen`. Locally, with Doxygen 1.17,
`pip install -r docs/requirements.txt && bash docs/build.sh` writes both to
`build-docs/html`.

## Use

```sh
cmake --install build --prefix <dir>     # or: make install PREFIX=<dir>
```

This also writes `ADS/ADSConfig.h`, whose `ADS_VERSION` (and
`ADS_VERSION_MAJOR`, `ADS_VERSION_MINOR`, `ADS_VERSION_PATCH`) is the
version of git, i.e., the last tag `vX.Y.Z` (that of `MODULE.bazel` in a
clone without the tags). Then, another CMake project uses it by

```cmake
find_package(ADS REQUIRED)
target_link_libraries(<target> PRIVATE ADS::ADS)
```

or, with pkg-config, `pkg-config --cflags ads`. Without CMake, the makefile
of a program takes ADS from its sources, with the backends requested on its
command line:

```make
ADSSDR = <the directory of ADS>
ADSODR = <the directory of the objects>
include $(ADSSDR)/lib/makefile-c

prog: prog.cpp $(ADSH) $(ADSOBJ)
	$(CXX) $(CXXFLAGS) $(ADSINC) -o $@ prog.cpp $(ADSOBJ) $(ADSLIB)
```

The packages of vcpkg (`ads`), conda-forge (`cpp-ads`), Homebrew, Debian and
Ubuntu (`libads-dev`), Conan, Spack, Nix, the AUR, MSYS2 and xmake install
the same files, from the recipes of [packaging](packaging/README.md).
Without installing anything, a CMake project can also take ADS at configure
time, with FetchContent (CMake 3.24 or later):

```cmake
include(FetchContent)
FetchContent_Declare(ADS
    URL https://github.com/dmeoli/ADS/releases/download/v0.1.0/ADS-0.1.0.tar.gz
    FIND_PACKAGE_ARGS 0.1)               # an installed ADS, if there is one
FetchContent_MakeAvailable(ADS)
target_link_libraries(<target> PRIVATE ADS::ADS)
```

The same holds with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake):

```cmake
CPMAddPackage("gh:dmeoli/ADS@0.1.0")
target_link_libraries(<target> PRIVATE ADS::ADS)
```

As a subproject, ADS builds neither its tests nor its benchmarks. With
Bazel, `bazel_dep(name = "ads", version = "0.1.0")` in `MODULE.bazel` and
`deps = ["@ads"]`, from the Bazel Central Registry; with Meson,
`meson wrap install ads` and `dependency('ads')`, from WrapDB; in both, the
targets compile in C++20.

`BigInteger` and `Rational` run on the backend of [GMP](https://gmplib.org),
and `gemm()` of `double` and `float` on that of a CBLAS (OpenBLAS, the
reference CBLAS, or Accelerate on macOS), whenever the library is found. GMP
then performs any operation of both at any length, and the CBLAS any product
of `double` and `float`, with the algorithms and the kernels that it chooses
for the processor. Where neither is found (e.g., with MSVC), the headers run
the algorithms of the book, written to be portable and readable rather than
tuned. With CMake, `ADS_WITH_GMP` and `ADS_WITH_BLAS` are `AUTO` (on when a
program that calls the library compiles and links), and when on they define
the macro of the same name on `ADS::ADS` and link the library, also for the
projects that find the installed ADS (`ads.pc` carries them too). The
makefile does the same: `make test ADS_WITH_GMP=0 ADS_WITH_BLAS=0` tests the
code of the book, and `GMP=<prefix>` and `BLAS=<prefix>` point to the
libraries where they are not where the compiler looks. By hand, one adds
`-DADS_WITH_GMP` with `-lgmp`, and `-DADS_WITH_BLAS` with the CBLAS (e.g.,
`-lopenblas`, or `-DADS_BLAS_ACCELERATE -framework Accelerate`). As with the
other backends, the packages are built with both off, so that they depend on
no library, and a project that has the libraries may define the macros. Note
that a multithreaded CBLAS (e.g., OpenBLAS, unless `OPENBLAS_NUM_THREADS=1`)
runs its threads in `gemm()`, and the parallel `gemm()` hands it the whole
product in one call, leaving the threads to it. GMP is under the LGPL v3 (or
the GPL v2), whose terms apply to a program that enables it; without the
macros, no part of either library is included.

With the headers of [Boost](https://www.boost.org) 1.83 or later,
`PointerTable` runs on `boost::unordered_flat_set` and `SmallVector` on
`boost::container::small_vector`, and from Boost 1.86 `Xoshiro256` and
`Xoshiro256StarStar` run on Boost.Random; `FlatHashTable` runs on
`boost::unordered_flat_map` only with `ADS_FLATHASH_BOOST` defined to 1,
since its own code is on a par with it (see the table below). They keep
the same names and members (the differences, e.g., the forward iterators
of the tables, are in the comments of the headers), and without Boost each
runs the code of ADS. `ADS_WITH_BOOST` (with `BOOST=<prefix>` for the
makefile, where Boost is not where the compiler looks) links no library,
and `ADS_POINTERTABLE_BOOST`, `ADS_SMALLVECTOR_BOOST` or `ADS_RANDOM_BOOST`
defined to 0 keeps the code of ADS for one structure (the benchmarks do so
for all of them, since they compare it with Boost).

With [Google Highway](https://github.com/google/highway) (1.0.7 or later,
its libraries `hwy` and `hwy_contrib`), the vector kernels of ADS are
compiled for each set of vector instructions that Highway knows. These
kernels are the population counts, the intersections of sorted arrays, the
containers of `RoaringBitmap`, the hash of the long strings, Stream VByte
and the binary packing, and `vectorQuickSort()` on the quicksort of Highway.
Highway chooses the best one of the processor when the program starts, and
ADS therefore needs no flag of the machine and tests no processor, compiler
or system. Without Highway, each structure runs its scalar code, with the
same results, and `ADS::prefetch( p )` does nothing. `ADS_WITH_HIGHWAY`
takes `HWY=<prefix>` with the makefile, where Highway is not where the
compiler looks. A translation unit that includes ADS with Highway may
neither include `hwy/highway.h` before it nor run a separate dispatch of
Highway, since Highway runs one per unit.

However, the kernels that run in the middle of a search (the groups of the
hash tables, the nodes of `AdaptiveRadixTree`, the bins of the filters, the
searches in the nodes of the B-trees) are inlined. These vector paths follow
the flags of the compilation of the program, with no dispatch at run time: a
program built for the baseline of x86-64 gets SSE2, and `-march=x86-64-v3`
turns the paths of AVX2 on. Those classes whose layout or inline code
changes with these flags (`FlatHashTable`, `DenseHashMap`,
`ConcurrentFlatTable`, `FastHash`) are declared in an inline namespace named
after them (cf. [`Abi.h`](include/ADS/Abi.h)), e.g., `ADS::abi_s1c1b0v3`.
Hence, two translation units built with different flags (e.g., a "hot" file
with `-march=x86-64-v3` and the rest with the default) get two classes with
the same name in `ADS`, in place of two definitions of one class that the
linker would merge. In fact, the test `abi-units` links two such units into
one program.

With [hwloc](https://www.open-mpi.org/projects/hwloc/) (2.0 or later),
`affinityCpus()`, `pinThread()` and `currentCpu()` of
[`Affinity.h`](include/ADS/parallel/Affinity.h) and the count of the
physical cores of the executor on FastFlow read the topology of the machine
from hwloc, which places the threads on any system that it knows. Without
it, the threads stay where the system puts them, and the executors count the
threads of the hardware. For `ADS_WITH_HWLOC`, the hints are
`HWLOC_INCLUDE_DIR` and `HWLOC_LIBRARY` with CMake and `HWLOC=<prefix>` with
the makefile.

With `ADS_WITH_BOOST`, the headers of `include/ADS/interop/boost/` join ADS
to Boost in both directions. [`Graph.h`](include/ADS/interop/boost/Graph.h)
makes `StaticGraph`, `ArrayGraph`, `ListGraph` and `ObservedGraph` graphs of
Boost.Graph, and therefore its algorithms (the visits, Dijkstra,
Bellman-Ford, the strong components, the topological order, the maximum
flows) run on them with no copy, while its `add_edge()` and `remove_edge()`
on an `ObservedGraph` reach the observers of ADS.
[`FromBGL.h`](include/ADS/interop/boost/FromBGL.h) goes the other way, any
graph of Boost.Graph as a graph of ADS,
[`Dijkstra.h`](include/ADS/interop/boost/Dijkstra.h) puts any priority queue
of ADS in the visit of Boost.Graph, and
[`Heap.h`](include/ADS/interop/boost/Heap.h) lets the shortest paths and
Prim's algorithm of ADS lower a key on the handles of Boost.Heap. Also,
[`Iterator.h`](include/ADS/interop/boost/Iterator.h) declares to Boost the
traversal of the iterators of ADS that build their item, so that Boost.Range
takes the dictionaries and the trees. A project that needs these headers
finds ADS by `find_package(ADS REQUIRED COMPONENTS Boost)` (`GMP` and `BLAS`
are components in the same way). With the standard library only,
[`interop/std/FlatMap.h`](include/ADS/interop/std/FlatMap.h) gives
`std::flat_set` and `std::flat_map` of C++23 on `TieredVector`
(`TieredFlatMap`, whose insertions take $O( \sqrt{n} )$ time), and
[`interop/std/Pmr.h`](include/ADS/interop/std/Pmr.h) the pools of ADS as a
`std::pmr::memory_resource`. The sorts take any random-access range
(`std::span`, `std::deque`, the sequences of ADS) beside `std::vector`, in
place on a contiguous one (and, those of the book, on any). With
`ADS_WITH_GMP`, `BigInteger::fromGmp()`, `toGmp()` and the constructors from
`mpz_class` and `mpq_class` pass the numbers to and from GMP. The tests are
in [`test/engineered_interop.cpp`](test/engineered_interop.cpp).

With [LEMON](https://lemon.cs.elte.hu) (the maintained fork abLEMON, or any
LEMON whose headers compile in C++20, which LEMON 1.3 does not), the headers
of `include/ADS/interop/lemon` let a program written on LEMON take a
structure of ADS where it may do better, by the named template parameters of
the algorithms of LEMON. Conversely, they let the algorithms of ADS run on
the graphs of LEMON. They give the heaps of ADS as heaps of LEMON (e.g.,
`Dijkstra< GR , LM >::SetStandardHeap< ADS::lemon_interop::DaryHeap< int ,
GR::NodeMap< int > > >`), the arrays of ADS as maps of LEMON, a
`StaticGraph` as a digraph of LEMON and a digraph of LEMON as a GRAPH of
ADS. Also, the notifications of LEMON are forwarded to the observers of ADS,
a `ListDigraph` is kept in step with a graph of ADS, and the runs of LEMON
may use the threads of ADS (see the chapter of the graphs of the manual).
With them, `maxCardinalityMatching()` and `maxWeightMatching()` solve on the
matchings of LEMON, and `Assignment` solves from scratch on the
`MinCostAssignment` of abLEMON, the warm repairs staying those of ADS
(`ADS_MATCHING_LEMON` or `ADS_ASSIGNMENT_LEMON` defined to 0 keeps the code
of ADS, as the benchmarks do). With CMake, `ADS_WITH_LEMON` builds the
target `ADS::LEMON` (the component `LEMON` of
`find_package(ADS COMPONENTS LEMON)`, and `ads-lemon.pc`), and with the
makefile its flags go in `$(ADSLEMONINC)` (`LEMON=<prefix>` where LEMON is
not where the compiler looks). Unlike the other backends, LEMON is never
included by `ADS::ADS` and `$(ADSINC)`, so that a program that does not
request it compiles none of it, and its headers are left out of the module
`ads`.

With LEMON, ADS also writes the warm starts that LEMON lacks into private
copies of six of its headers (`network_simplex.h`, `cost_scaling.h`,
`cycle_canceling.h`, `capacity_scaling.h`, `bits/mcf_feasible_flow.h` and
`preflow.h`). The warm starts are `runWarm()` of the four minimum cost flows
from the last basis or flow, the setters of one arc or node in constant time
(`warmCost()`, `warmUpper()`, `warmSupply()`), and `Preflow::initWarm()`
from a flow with deficits, with its `runWarm()`. These copies are rewritten
at configure time by the rules of
`include/ADS/interop/lemon/shim/rewrites.txt`, each a line
`header|anchor|what` whose anchor is a line of LEMON that must occur exactly
once (otherwise the rewrite stops). CMake writes them in the build directory
and the makefile in `$(ADSODR)/lemon-shim` (`ADS_LEMON_WARM=0` leaves them
out), and they come before LEMON on the include path of `ADS::LEMON` and of
`$(ADSLEMONINC)`, which define `ADS_LEMON_SHIM`; an installed ADS writes
them again from the LEMON that its user finds. On them,
`ADS/interop/lemon/Warm.h` builds an engine of `MaxFlowObserver` on
`Preflow`, one of `Assignment` on `NetworkSimplex` and an observer of a
minimum cost flow. A program must compile all its translation units on the
same rewritten copies, and a module of LEMON must be built from them too. In
fact, two different copies of a header of LEMON in one binary give two
layouts of one class, a violation of the one definition rule that compilers
do not diagnose.

```cpp
#include "ADS/sequence/List.h"

ADS::List< int > L{ 3 , 1 , 2 };
for( auto p = L.head() ; ! L.finished( p ) ; p = L.next( p ) )
 std::cout << L.read( p );               // the positions of the book
std::ranges::reverse( L );               // the algorithms of std
for( int x : L ) std::cout << x;
```

## C++20 module

The module `ads` (`src/ADS.cppm`) exports each name that the headers declare
in the namespace `ADS` and in those nested in it (but for the namespaces
`detail`). Hence, a translation unit can write `import ads;` in place of the
`#include` of the headers, which keep working as before. It does so by
exporting the modules of the three sets of [Base and
advanced](#base-and-advanced), `ads.core`, `ads.base` and `ads.advanced`
(`src/ADSCore.cppm`, `src/ADSBase.cppm`, `src/ADSAdvanced.cppm`), each of
which exports the names of the headers of its set. The last two also export
those of `ads.core`, which they do not import, since Clang 18 does not
compile a module that imports another one when both include Highway. Thus,
`import ads.base;` compiles only the base headers, the core and the few
advanced headers that the base ones use inside. With CMake 3.28 or later,
the Ninja generator and Clang 16 (with `clang-scan-deps`) or GCC 15 or
later, `-DADS_BUILD_MODULE=ON` (off by default, and turned off with a
warning where one of these is missing) builds the target `ADS::ADS_module`,
and `ADS::ADS_module_core`, `ADS::ADS_module_base` and
`ADS::ADS_module_advanced` for the sets, installed with the package.

```cmake
find_package(ADS REQUIRED)
target_link_libraries(<target> PRIVATE ADS::ADS_module)
```

Without CMake, `make test ADSMOD=1 CXX=clang++` (or `CXX=g++`) builds the
module in `build-make` and the test that imports it (the makefile of a
program that imports the module compiles it with `$(ADSMODSW)` and links
`$(ADSOBJ)`; cf. `lib/makefile-mod`). AppleClang does not build it, while
the Clang of LLVM on macOS does. GCC 14 stops on it with an internal error,
and MSVC discards the operators of the standard library (e.g., those of
`std::complex`) that the templates of the module use only in their
instantiations, and therefore an importer does not compile. Note that a
module exports no macro, and therefore a file that uses those of the
contracts (e.g., `ADS_PRE`) still includes `ADS/Contract.h`, and those that
configure the headers (e.g., `ADS_CONTRACTS`, `ADS_WITH_GMP`) take the value
they have where the module is compiled. Also, the executors of OpenMP,
oneTBB and FastFlow and the sort on MPI are in the module only when it is
compiled with their library. Finally, the files of the modules are written
by `cmake/ads_module_exports.py` from the AST of the headers and from
`cmake/ads_sets.txt`, and they are written again when a header gains or
loses a name or changes its set.

The gain in compile time is quite small, since the library is made of
templates, which an importer still instantiates and compiles. On a server
with 384 cores and a load of 55 to 73 (hence, the times are only
indicative), Clang 18 at `-O2` compiles a program with Dijkstra's algorithm
on a `ListGraph` in 1.0 to 1.2 s with `import`, against 1.3 to 1.5 s with
the headers. Yet, the peak memory grows, from 160 to 265 MB, since the
importer loads the module of the whole library.

## Code style

The code is written in the style of the repository
[code-style](https://github.com/dmeoli/code-style), and so is the C++ of the
blocks `\code` of the comments and of the theory pages. This style is the
`.clang-format` of that repository (of which the one at the root of ADS is a
copy, so that clang-format and the IDEs find it) together with what its
`format.py` adds, for lack of an option of clang-format (e.g.,
`return( x );` and a space before each comma). On a pull request, the
continuous integration checks the lines that the request changes and posts
as suggestions the changes that put them in the style; on each push, it
checks all the files in full. With that repository cloned in `~/code-style`,
the command

```sh
~/code-style/code-format --since origin/main FILE...
```

does the same before a commit (clang-format 22 is needed, e.g., `pip
install clang-format==22.1.5`).

## License

This software is released under the [Boost Software License, version
1.0](https://www.boost.org/LICENSE_1_0.txt) (see the file
[LICENSE](LICENSE)), the license of Boost and of LEMON. As for the book of
Alberto Montresor, under CC BY-NC 4.0, it is cited without being reproduced,
since the pseudocode and the prose of ADS are written anew (cf. section
Where each thing comes from).
