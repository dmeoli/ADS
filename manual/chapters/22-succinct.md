# 22. Succinct structures {#ch-22}

[Source: `include/ADS/succinct/BitVector.h`, `BitStream.h`,
`RankSelect.h`, `CompactRankSelect.h`, `InterleavedRankSelect.h`,
`RRRBitVector.h`, `EliasFano.h`, `PartitionedEliasFano.h`, `LAVector.h`,
`PackedArray.h`, `JacobsonTree.h`, `LoudsTree.h`, `BalancedParentheses.h`,
`BPTree.h`, `WaveletMatrix.h`, `HuffmanWaveletTree.h`, `MoveStructure.h`,
`EliasFanoGraph.h`, `BVGraph.h`, `K2Tree.h`]

We are concerned in this chapter with the data structures that take about
as many bits as the information they hold, and that answer their queries
without being decoded. The headers of `ADS/succinct` follow chapter 15,
Compressed Data Structures, of [Ferragina 2023](Z-bibliography.md#ref-ferragina2023),
and the representation of Elias and Fano of its chapter 11, Integer
Coding; the book of Montresor does not treat them. Both operations on which all of them are built, the rank and the select of a vector of bits,
are defined in [§22.1](22-succinct.md#sec-22-1) together with the lower
bounds against which the space is measured. We then give the vector of
bits and the streams of bits ([§22.2](22-succinct.md#sec-22-2)), the
directories of rank and select, from the fastest to the smallest
([§22.3](22-succinct.md#sec-22-3) and [§22.4](22-succinct.md#sec-22-4)),
and the vector compressed to its entropy
([§22.5](22-succinct.md#sec-22-5)). On them we build, in turn, the sequences of integers searched in place ([§22.6](22-succinct.md#sec-22-6)), the arrays
of small fields ([§22.7](22-succinct.md#sec-22-7)), the trees
([§22.8](22-succinct.md#sec-22-8)), the sequences of symbols and the
permutations ([§22.9](22-succinct.md#sec-22-9)) and the graphs ([§22.10](22-succinct.md#sec-22-10)). Finally, [§22.11](22-succinct.md#sec-22-11) describes how the directories and the wavelet structures are built on
several threads.

Some members are common to the classes of the chapter, and we describe
them once here. All the static sequences (`BitVector`, the directories of
`RankSelect.h` and `CompactRankSelect.h`, `EliasFano`,
`PartitionedEliasFano`, `LAVector` and `WaveletMatrix`) are random-access
ranges with the aliases of the standard containers (`value_type`,
`size_type`, `difference_type`, `reference`, `const_reference`,
`const_iterator` and `iterator`, the same), whose iterators read the items
by subscript (`SubscriptIterator` of `BitVector.h`), from `begin()` and
`cbegin()` to `end()` and `cend()`. Each structure gives its space in bits
by `sizeInBits()`; besides, the directories give their bits beyond those
of the vector by `directoryBits()`, `EliasFano` and `BVGraph` the bits of
their codes alone by `codeBits()`, `BalancedParentheses` the bits of its
range min-max tree by `treeBits()`, and `LAVector` the bits of its
corrections by `correctionsInBits()` (and their width by
`correctionBits()`). Finally, the trees mark a missing node (or place) by
the constant `nil`.

## 22.1 Rank, select and the lower bounds {#sec-22-1}

A vector of bits $B[ 0 , m )$ with $n$ 1s has, for $b \in \{ 0 , 1 \}$,
the operations

$$
\mathrm{rank}_b( i ) = | \{ j < i : B[ j ] = b \} | , \qquad
\mathrm{select}_b( k ) = \min \{ i : \mathrm{rank}_b( i + 1 ) = k \} ,
\tag{22.1}
$$

i.e., the number of the bits $b$ before the place $i$ (cf. (22.1)), and the place of the
$k$-th bit $b$, with $1 \le k$ up to the number of the bits $b$. Hence
$\mathrm{rank}_b( \mathrm{select}_b( k ) ) = k - 1$, and
$\mathrm{rank}_0( i ) = i - \mathrm{rank}_1( i )$. We count the places from
0, as the library does, while Ferragina's book counts them from 1; its
$\mathrm{Rank}_1$ and $\mathrm{Select}_1$ are therefore our
$\mathrm{rank}_1$ and $\mathrm{select}_1$ with the places shifted by one.
We regard the two operations as the primitives of the area: a structure that codes
its objects as bits uses the select to go from the number of an object to
its place in the bits, and the rank to go back.

A representation of the objects of a class of $N$ elements needs
$Z = \lceil \log_2 N \rceil$ bits for some of them, the
*information-theoretic lower bound*. We call it *succinct* if it takes $Z + o( Z )$ bits, and *compact* if it takes $O( Z )$ bits. In both cases it should answer its queries in about the time of the plain representation (possibly up to a constant). Three bounds recur in the chapter. The vectors of $m$ bits with $n$ 1s are $\binom{m}{n}$. Similarly, the sets of $n$ integers of $[ 0 , u )$ are
$\binom{u}{n}$, i.e., about $n \log_2( u / n ) + n \log_2 e$ bits when $n$ is much smaller than $u$. Finally, the ordered trees of $n$ nodes (and the binary
trees of $n$ nodes) are the Catalan number
$C_n = \binom{2 n}{n} / ( n + 1 ) \approx 4^n / ( n^{3/2} \sqrt{\pi} )$,
i.e., $2 n - \Theta( \log n )$ bits.

<span id="lem-22-1"></span>**Lemma 22.1.** *With $H_0( B ) = \frac{n}{m}
\log_2 \frac{m}{n} + \frac{m - n}{m} \log_2 \frac{m}{m - n}$, the empirical
entropy of a vector of $m$ bits with $n$ 1s,*

$$
\log_2 \binom{m}{n} \le m H_0( B ) . \tag{22.2}
$$

*Proof.* With $p = n / m$, one has $\binom{m}{n} p^n ( 1 - p )^{m - n} \le
\sum_k \binom{m}{k} p^k ( 1 - p )^{m - k} = 1$, and the logarithm of
$p^{-n} ( 1 - p )^{-( m - n )}$ is $m H_0( B )$. $\square$

[Table 22.1](22-succinct.md#tab-22-1) collects the notation of the area. We analyze the costs in the model of the word RAM with words of
$\Theta( \log m )$ bits, in which the sideways sum of a word (the number of
its 1s, $\lvert x \rvert$ or $\mathrm{popcount}( x )$) and the place of
its $k$-th 1 cost $O( 1 )$ time. Indeed, the first is one instruction of today's processors, and the second a few broadword operations
([§22.2](22-succinct.md#sec-22-2)).

<span id="tab-22-1"></span>**Table 22.1.** The notation of the chapter.

| Symbol | Meaning |
|---|---|
| $B$, $m$, $n$ | a vector of bits, its length and the number of its 1s |
| $\mathrm{rank}_b( i )$, $\mathrm{select}_b( k )$ | as in (22.1); `rank1()`, `select1()`, `rank0()`, `select0()` in the code |
| $H_0$ | the empirical entropy of order 0 of a vector of bits, or of a sequence of symbols of frequencies $f_c$, $\sum_c ( f_c / n ) \log_2( n / f_c )$ |
| $x_0 , \ldots , x_{n-1}$, $u$ | a non-decreasing sequence of integers, and the bound of its items |
| $\ell$, $L$, $H$ | the low bits of an item of Elias-Fano, and the vectors of the low and of the high parts |
| $\sigma$, $w$ | the size of an alphabet of symbols, and the bits of a symbol of a wavelet matrix |
| $E( i )$ | the excess of a sequence of parentheses, the open ones less the closed ones in $B[ 0 , i ]$ |
| $\lvert x \rvert$, $\mathrm{popcount}( x )$ | the number of the 1s of a word |

## 22.2 Vectors of bits and streams {#sec-22-2}

`BitVector` packs the bits in words of 64 bits, the bit $i$ in the bit
$i \bmod 64$ (from the least significant one) of the word
$\lfloor i / 64 \rfloor$ ([Figure 22.1](22-succinct.md#fig-22-1)). Besides the access to a bit in $O( 1 )$ time, the class appends a bit or a field of up to
64 bits (`push_back()` and `append()`) in $O( 1 )$ amortized time, and it
reads a field of up to 64 bits (`get()`) in $O( 1 )$ time with two shifts
at most ([Algorithm 22.1](22-succinct.md#alg-22-1)). Its words fill lines of the cache of 64 bytes, with at least one word past the last bit. They start on a line (on a huge page from 2 MB on, which may spare the misses of the TLB of a random access), and the space is therefore $m + O( 512 )$ bits for $m$ bits. We keep the bits past the size at 0, which is the invariant of the class. For this reason a field is read with no test of the end, and the
rank of a word needs no mask at the end of the vector. Unlike
`std::vector< bool >`, whose layout is left to the implementation,
`BitVector` exposes its words, which a directory of rank or a code read a
word at a time needs. Its words are a `Words`, a vector on the allocator of
the lines; `copyBits()` ors a run of bits of a word array into another
from any place, `reserve( n )` makes room for $n$ bits and `toString()`
writes the bits as `'0'` and `'1'`, the bit 0 first.

![Figure 22.1. Three words of a `BitVector` of $n = 100$ bits, each word with its bit 0 on the right: the field read by `get( 58 , 13 )` crosses from the word 0 to the word 1, and the bits past the size are 0 (the word 2 is there although no bit of the vector is in it). Below, a stream that holds $\gamma( 9 ) \gamma( 3 )$, read from the place $p$.](../figures/succinct-bitvector.svg){#fig-22-1}

<span id="alg-22-1"></span>**Algorithm 22.1.** The append and the read of
a field of `BitVector`; $W$ are the words.

--8<-- "generated/BitVector-append.pseudo.md"

--8<-- "generated/BitVector-get.pseudo.md"

Finding the place of the $k$-th 1 of a word, by `selectInWord()`, is the last step of
every select of the library, in place of the tables of the small blocks of
the classical analysis ([Jacobson 1989](Z-bibliography.md#ref-jacobson1989);
[Clark 1996](Z-bibliography.md#ref-clark1996)). It is the broadword method
of [Vigna 2008](Z-bibliography.md#ref-vigna2008)
([Algorithm 22.2](22-succinct.md#alg-22-2)), with
$L_8 = \sum_{j < 8} 2^{8 j}$ and $H_8 = 2^7 L_8$.

<span id="alg-22-2"></span>**Algorithm 22.2.** The broadword select in a
word ([Vigna 2008](Z-bibliography.md#ref-vigna2008)).

--8<-- "generated/BitVector-selectInWord.pseudo.md"

<span id="lem-22-2"></span>**Lemma 22.2.** *Let $s_j$ be the number of the
1s of the bytes $0 , \ldots , j$ of $x$, and $k < \lvert x \rvert$. The
byte $j$ of $s + ( 127 - k ) L_8$ has its high bit if and only if
$s_j > k$, and the first such byte holds the $k$-th 1 of $x$, counting from
0.*

*Proof.* Since the sideways sums of the 8 bytes are at most 8, one product by $L_8$ puts their prefix sums $s_j \le 64$ in the bytes, and no byte overflows,
since $s_j < 256$. Adding $127 - k$ to the byte $j$ gives at most
$64 + 127 < 256$, and therefore no carry crosses into the next byte; the
sum is at least 128, i.e., it has its high bit, if and only if
$s_j \ge k + 1$. Hence the first byte $b$ with $s_b > k$ is the one that holds
the 1 of rank $k$, which is the $( k - s_{b-1} )$-th 1 of the byte. $\square$

In the implementation, the trailing zeros of the masked word give $b$, and the 1 inside the byte
is found by clearing its lower 1s, at most 7 times. With the instructions
BMI2, the implementation deposits the single bit $2^k$ on the 1s of the word instead,
and the place is then its trailing zeros.

We write the codes of the integers of [§21.7](21-compression.md#sec-21-7)
on the streams of bits of `BitStream.h`. A `BitWriter` appends
bits to a `BitVector`, and a `BitReader` reads them back from a place that
moves forward, both in $O( 1 )$ time per field of up to 64 bits
([Algorithm 22.3](22-succinct.md#alg-22-3)). `write()` and `read()` move a field with its least significant bit first, the order of the words. `writeBinary()` and `readBinary()` move it with its most significant bit first, the order in which the codes are written on paper, and they turn the field around by three rounds of masks and a swap of the bytes. Also,
`writeUnary( q )` writes $q$ zeros and a 1, and `readUnary()` counts the
zeros up to the 1 a word at a time, by the trailing zeros of the word, in
$O( 1 + q / 64 )$ time. A single bit is moved by `writeBit()` and
`readBit()`; `seek( p )` moves the reader to the place $p$, `atEnd()`
tells whether all the bits have been read, and `release()` hands the bits
written over to the caller, leaving the writer empty. In the literature the fast decoders write the codes into words rather than bytes, e.g., the streams of the WebGraph framework of [Boldi and Vigna 2004](Z-bibliography.md#ref-boldi2004). Indeed, a codeword of up to 64 bits spans at most two words, and the unary part is
decoded by one instruction per word.

<span id="alg-22-3"></span>**Algorithm 22.3.** The binary and the unary
fields of the streams of bits.

--8<-- "generated/BitStream-BitWriter-writeBinary.pseudo.md"

--8<-- "generated/BitStream-BitWriter-writeUnary.pseudo.md"

--8<-- "generated/BitStream-BitReader-readUnary.pseudo.md"

## 22.3 Rank and select {#sec-22-3}

One can reduce the rank to a few counts by the two-level directory of Jacobson.

<span id="lem-22-3"></span>**Lemma 22.3 (the directory of
Jacobson).** *With superblocks of $Z$ bits, each with the 1s before it in
$\log_2 m$ bits, and blocks of $z$ bits, each with the 1s before it since
its superblock in $\log_2 Z$ bits,*

$$
\mathrm{rank}_1( i ) = R_S[ \lfloor i / Z \rfloor ] +
R_b[ \lfloor i / z \rfloor ] + \mathrm{rank}_1 \text{ of }
B[ z \lfloor i / z \rfloor , i ) , \tag{22.3}
$$

*and with $Z = \log_2^2 m$ and $z = \log_2 m / 2$ the counts take $o( m )$
bits, with the rank in $O( 1 )$ time.*

*Proof.* Clearly, the 1s before $i$ are those before its superblock, those of its superblock before its block, and those of its block before $i$, which is (22.3). Also, the superblocks take $( m / \log_2^2 m ) \log_2 m = m / \log_2 m$
bits, and the blocks $( 2 m / \log_2 m ) \log_2 \log_2^2 m =
O( m \log \log m / \log m )$, both $o( m )$. Finally, the last term of (22.3) is read in a table of the $2^z = \sqrt{m}$ blocks of $z$ bits and of their $z$
prefixes, of $O( \sqrt{m} \log m \log \log m )$ bits. $\square$

In the library we replace the table by the sideways sum of a word, one
instruction of the processor, which lets $z$ be a word and $Z$ a constant. `RankSelect` (chapter 15 of
[Ferragina 2023](Z-bibliography.md#ref-ferragina2023)) keeps one word for
each block of 256 bits: the 1s before the block in its 40 high bits, and
those before its words 1, 2 and 3, relative to the block, in 8 bits each
([Figure 22.2](22-succinct.md#fig-22-2)). Hence one can compute (22.3) by one read of the directory and one of the bits, with no branch, at the price of
$64 / 256 = 25\%$ of space more instead of $o( m )$
([Algorithm 22.4](22-succinct.md#alg-22-4)). This fraction is constant, but it is "small" at the practical sizes, since $m / \log_2 m$ is still 2.5% of $m$ for $m = 2^{40}$, a "large" vector. Above $2^{40}$ bits, a third level counts the 1s
before each $2^{40}$ bits in 64 bits.

![Figure 22.2. The directory of `RankSelect`. Above, a block of 256 bits and its entry $D[ t ]$, with the 1s before the block and the relative ranks 13, 53 and 60 of its words 1, 2 and 3 (the sideways sums of the words being 13, 40, 7 and 22): the rank of a place $i$ in the word 2 adds 53 and the 1s of the word before $i$ (violet). Below, a group of the inventory of select, its first place $P[ j ]$ and the offsets of its 1s of rank multiple of $2^g$.](../figures/succinct-rankselect.svg){#fig-22-2}

<span id="alg-22-4"></span>**Algorithm 22.4.** The directory of
`RankSelect` and its rank, with the blocks of 256 bits.

--8<-- "generated/RankSelect-BasicRankSelect-directory.pseudo.md"

--8<-- "generated/RankSelect-BasicRankSelect-rank1.pseudo.md"

We build the directory in one pass on the words. Here the sideways sums $p_0 , \ldots , p_3$ of the words of a block, at most 64 each, are put in
the bytes of a word, and one product gives their prefix sums in the bytes,
which do not overflow,

$$
( p_0 + 2^8 p_1 + 2^{16} p_2 ) \cdot ( 1 + 2^8 + 2^{16} ) \bmod 2^{24} =
p_0 + 2^8 ( p_0 + p_1 ) + 2^{16} ( p_0 + p_1 + p_2 ) , \tag{22.4}
$$

i.e., the three relative ranks of the entry. When the bits come from words
in memory above $2^{23}$ bits, the words are copied in the same pass, since
reading them twice costs more than a copy in the caches. The implementation follows the pseudocode. Note that the 1s of the word up to $i$ need no test of the end, since the words exist past the size.

We do not reduce the select to the rank by a binary search, which would cost $O( \log m )$ steps. [Clark 1996](Z-bibliography.md#ref-clark1996) cuts the
1s in groups of $\log_2^2 m$ and writes down the places of all the 1s of a
group that spans more than $\log_2^4 m$ bits; these take
$O( m / \log m )$ bits in all, since such groups are at most
$m / \log_2^4 m$. All the other groups are cut again, and the last level is solved by a table. This gives $O( 1 )$ time in $o( m )$ bits, at the price of three levels whose constants are rather large at the sizes of the practice.
We follow instead the inventories of the simple select of
[Vigna 2008](Z-bibliography.md#ref-vigna2008), with groups of constant
size, for the 1s and for the 0s. We cut the 1s in groups of $2^\ell$, with
$\ell = \lfloor \log_2( 1.5 \cdot 4096 \, k / m ) \rfloor$ for $k$ of them,
and a group thus spans between 3072 and 6144 bits on average. For each group
the place of its first 1 is kept in 64 bits, and the places of its 1s of
rank multiple of $2^g$, $g = \max\{ \ell - 5 , 0 \}$, in 16 bits from it,
i.e., at most 32 offsets, a line of the cache. Hence $\mathrm{select}_1( k )$ reads the place of the 1 of rank $k - 1$ rounded down to a multiple of $2^g$. It counts the rest of the 1s word by word, by the sideways sums, and it ends in the word by `selectInWord()` ([Algorithm 22.5](22-succinct.md#alg-22-5)).

<span id="alg-22-5"></span>**Algorithm 22.5.** The select of `RankSelect`
by the inventories of [Vigna 2008](Z-bibliography.md#ref-vigna2008).

--8<-- "generated/RankSelect-BasicRankSelect-select.pseudo.md"

A select is thus (typically) three accesses to the memory and a few words, $O( 1 )$
time where the 1s are spread evenly, with about $576 / 6144$ to
$576 / 3072$, i.e., 9.4% to 18.8%, of $m$ bits more for each value. A group that spans $2^{16}$ bits or more (quite a "sparse" one) has offsets that do not fit in 16
bits; it is *sparse*, and its 1s are found by a binary search on the
directory of rank between its ends, $O( \log( m / 256 ) )$ time in the worst
case. A group that spans less is scanned over fewer than $2^{16} / 64 = 1024$ words, and usually over a few, as long as the bits are not "clustered". Also, a group of fewer than 32 bits $b$ (below about 1% of them) keeps the place of each one, and its select reads no bit at all. These are the trade-offs of the practical designs: we find them in the rank9 and the inventories of [Vigna 2008](Z-bibliography.md#ref-vigna2008), in the darray of [Okanohara and Sadakane 2007](Z-bibliography.md#ref-okanohara2007), and in the structures of [Gog and Petri 2014](Z-bibliography.md#ref-gog2014a).

We build the inventories of select in the constructor (`RankSelect`, the default), or at the first select of each value (`LazyRankSelect`), in a pass on the directory. A structure that only ranks then never pays for them; also, a `RankSelectOptions::Selects` asks for the selects of the 1s,
of the 0s, of both or of neither. Note that the first call builds an inventory once,
whichever thread calls it, with `std::call_once` and a flag read with
acquire semantics before it, and a copy keeps the inventories already
built. We remark that the lazy form may cost the select 10 to 15%, since the load of the flag with acquire semantics makes the compiler read the fields of the inventory again at each query, whereas the eager form (the template parameter `Lazy` false) keeps them in plain fields.

## 22.4 Compact and interleaved directories {#sec-22-4}

`RankSelect` spends about 50% of $m$ bits (with both selects) for the
fastest queries. `CompactRankSelect` is its compact counterpart, the flat
directory of [Kurpicz 2022](Z-bibliography.md#ref-kurpicz2022), after the
poppy of [Zhou, Andersen and Kaminsky 2013](Z-bibliography.md#ref-zhou2013),
in about 3.5% of $m$ bits. It cuts the bits in superblocks of 4096 bits,
each one in 8 blocks of 512 bits, a line of the cache. One entry of 128
bits per superblock holds the 1s before it since its top region of
$2^{32}$ bits, in 32 bits, and the 1s before each of its blocks since the
superblock, $c_0 = 0 , c_1 , \ldots , c_7 \le 3584$, in 12 bits each; a
count of 64 bits per top region completes the rank
([Figure 22.3](22-succinct.md#fig-22-3)). Hence

$$
\mathrm{rank}_1( i ) = T[ \lfloor i / 2^{32} \rfloor ] +
S[ \lfloor i / 4096 \rfloor ] + c_{\lfloor i / 512 \rfloor \bmod 8} +
\sum_{j < 8} \big\lvert w_j \wedge ( 2^{\min( 64 , \max( 0 , o - 64 j ) )}
- 1 ) \big\rvert , \tag{22.5}
$$

with $o = i \bmod 512$ and $w_j$ the words of the block: one access to the
entries, one to the line of the block, and the sideways sums of its 8 words
cut at $i$, with no branch ([Algorithm 22.6](22-succinct.md#alg-22-6)).
With AVX2, we count the bytes of the line before the one of $i$ 32 at a time, by a table of the 16 nibbles ([Mula, Kurz and Lemire 2018](Z-bibliography.md#ref-mula2018)). In all, the entries take $128 / 4096 = 3.125\%$ of $m$ bits.

![Figure 22.3. (a) A superblock of `CompactRankSelect`, its 8 blocks of 512 bits and its entry of 128 bits: the rank of a place $i$ in the block 5 adds the count $S$ of the superblock, $c_5$ and the 1s of the line before $i$ (violet). (b) A line of `InterleavedRankSelect`, its count $h_\ell$ of 16 bits in front of its 496 bits.](../figures/succinct-compact.svg){#fig-22-3}

<span id="alg-22-6"></span>**Algorithm 22.6.** The rank and the select of
`CompactRankSelect` ([Kurpicz 2022](Z-bibliography.md#ref-kurpicz2022)).

--8<-- "generated/CompactRankSelect-rank1.pseudo.md"

--8<-- "generated/CompactRankSelect-select.pseudo.md"

Its select keeps the superblock of every 4096-th bit $b$, in 32 bits each,
i.e., at most $32 / 4096 < 0.8\%$ of $m$ bits for each value. It finds the $k$-th bit $b$ by a binary search on the entries between two samples. Then it finds the block by the 7 counts of the superblock, and the word by the sums of the 8 words of its line, both by comparisons with no branch; the bit is found by `selectInWord()`. This is $O( \log( m / 4096 ) )$ steps in the worst case,
and $O( 1 )$ where the bits $b$ are spread "evenly" (a sample every 2 to 8 superblocks for a density between 1/2 and 1/8). Its trade with `RankSelect` is, roughly, the "classical" one of the literature: a sum of up to 8 words and a
search of the entries instead of one word and an inventory, in 15 times
fewer bits.

`InterleavedRankSelect` takes the layout of SPIDER of
[Laws et al. 2024](Z-bibliography.md#ref-laws2024), with the counts inside
the lines of the bits. A line of 512 bits holds 496 bits of $B$ and, in its
first 16 bits, the 1s before it since its superblock of 128 lines (63488
bits, so that the count fits in 16 bits); a superblock keeps the 1s before
it in 64 bits apart. Hence

$$
\mathrm{rank}_1( i ) = S[ \lfloor \ell / 128 \rfloor ] + h_\ell +
\lvert B_\ell[ 0 , i \bmod 496 ) \rvert , \qquad
\ell = \lfloor i / 496 \rfloor , \tag{22.6}
$$

with one access to the memory of the vector, the line, against the two of the directories kept apart. Its array of the superblocks, one word per 63488 bits, typically stays in the caches, and the counts take $16 / 512 = 3.125\%$ of the line. Its select keeps the line of every 8192-th bit $b$ with the bits $b$ before that line, in 96 bits, i.e., at most $96 / 8192 < 1.2\%$ of $m$ bits for each value. As SPIDER does, it guesses the line of the $k$-th bit $b$ between two samples
$( \ell_j , c_j )$ by interpolation,

$$
\hat\ell = \ell_j + \frac{( k - 1 - c_j ) ( \ell_{j+1} - \ell_j )}
{c_{j+1} - c_j} , \tag{22.7}
$$

and then corrects the guess a line at a time by the counts of the lines (hopefully a step or two where the bits $b$ are spread evenly, but $\ell_{j+1} - \ell_j$ in the worst case). It ends in the line by the sums of its words and
`selectInWord()` ([Algorithm 22.7](22-succinct.md#alg-22-7)).

<span id="alg-22-7"></span>**Algorithm 22.7.** The rank and the select of
`InterleavedRankSelect`
([Laws et al. 2024](Z-bibliography.md#ref-laws2024)).

--8<-- "generated/InterleavedRankSelect-rank1.pseudo.md"

--8<-- "generated/InterleavedRankSelect-select.pseudo.md"

[Table 22.2](22-succinct.md#tab-22-2) summarizes the four vectors of bits
with rank and select. In our experience `RankSelect` is usually preferable when the queries dominate, since its rank typically reads one entry and one word; `CompactRankSelect`
and `InterleavedRankSelect` cost about seven times fewer bits and a sum of
up to 8 words per rank, the second with one miss of the cache instead of
two. `RRRBitVector` ([§22.5](22-succinct.md#sec-22-5)) is perhaps the one to choose when the 1s (or the 0s) are "rare", since its space follows (22.2) instead of
$m$, at the price of the decoding of a block at each query. Their measured
speeds belong to the campaign of Appendix B.

<span id="tab-22-2"></span>**Table 22.2.** The vectors of bits with rank
and select, for $m$ bits.

| Realization | Space beyond $m$ | `rank1( i )` | `select1( k )` |
|---|---|---|---|
| `RankSelect`, `LazyRankSelect` | 25% for the rank, 9.4% to 18.8% for the select of each value | $O( 1 )$, one entry and one word | $O( 1 )$ when the bits are spread evenly, $O( \log( m / 256 ) )$ in a sparse group |
| `CompactRankSelect` | 3.125%, and less than 0.8% for the select of each value | $O( 1 )$, one entry and the 8 words of a line | $O( 1 )$ when the bits are spread evenly, $O( \log( m / 4096 ) )$ in the worst case |
| `InterleavedRankSelect` | 3.125% inside the lines, and less than 1.2% for the select of each value | $O( 1 )$, one line | $O( 1 )$ when the bits are spread evenly, the lines between two samples in the worst case |
| `RRRBitVector` | the whole vector in $m H_0( B ) + 0.175 m$ bits | $O( 32 + b )$, blocks of $b = 63$ bits | $O( \log( m / 2016 ) + 32 + b )$ |

## 22.5 Compressed vectors of bits {#sec-22-5}

`RRRBitVector` is the compressed vector of
[Raman, Raman and Rao 2007](Z-bibliography.md#ref-raman2007) in the
practical form of [Claude and Navarro 2008](Z-bibliography.md#ref-claude2008).
It cuts $B$ in blocks of $b = 63$ bits, and writes a block of $c$ 1s as its
*class* $c$, in 6 bits, and as its *offset*, its place among the
$\binom{b}{c}$ blocks of class $c$, in
$\lceil \log_2 \binom{b}{c} \rceil$ bits. We compute the offset by the combinatorial number system.

<span id="lem-22-4"></span>**Lemma 22.4 (the combinatorial number
system).** *The vectors of $b$ bits with $c$ 1s at the places
$p_1 < \cdots < p_c$ are numbered from 0 to $\binom{b}{c} - 1$ by*

$$
o = \sum_{t=1}^{c} \binom{p_t}{t} , \tag{22.8}
$$

*their rank in the colexicographic order; and the place $p$, scanned from
$b - 1$ down, holds the $t$-th 1, $t$ being the 1s not yet met, if and only
if $\binom{p}{t} \le o$, where $o$ is the remainder after the 1s above.*

*Proof.* Note that the vectors that precede a given one in the colexicographic order are, for each $t$, those that agree with it above $p_t$ and have their $t$
lowest 1s below $p_t$, i.e., $\binom{p_t}{t}$ of them, which gives (22.8).
Since $\sum_{t' < t} \binom{p_{t'}}{t'} < \binom{p_t}{t}$, the highest
term of (22.8) is the largest $\binom{p}{c} \le o$, which gives the
decoding. $\square$

![Figure 22.4. A block of $b = 8$ bits (63 in the library) with its 1s at the places 1, 4 and 6: its class 3 and its offset 27 by (22.8), in 6 bits. Below, the classes of four blocks and their offsets of variable width; the blocks of class 0 and 8 take no bit of offset.](../figures/succinct-rrr.svg){#fig-22-4}

<span id="thm-22-1"></span>**Theorem 22.1.** *`RRRBitVector` takes at most
$m H_0( B ) + 0.175 m$ bits, with the rank in $O( 32 + b )$ time and the
select in $O( \log( m / 2016 ) + 32 + b )$ time.*

*Proof.* Note that the product of the $\binom{b}{c_j}$ of the blocks counts a part of
the vectors of $m$ bits with $n$ 1s, and therefore by (22.2) $\sum_j \log_2 \binom{b}{c_j} \le m H_0( B )$; the offsets, rounded up,
take at most $m H_0( B ) + m / b$ bits, and the classes $6 m / b$. A superblock of 32 blocks (2016 bits) keeps the 1s before it and the place of its first offset, at most $128 / 2016 = 6.3\%$ of $m$. In all, one has
$m H_0( B ) + ( 7 / 63 + 128 / 2016 ) m < m H_0( B ) + 0.175 m$. As for the time, the rank of a place sums the classes of the blocks of its superblock before its own,
and of their offsets the lengths, at most 31 of each, and rebuilds its
block from the highest place down by
[Lemma 22.4](22-succinct.md#lem-22-4), at most $b$ steps. Similarly, the select finds the superblock by a binary search on the counts, the block by the classes,
and the 1 in the rebuilt block. $\square$

<span id="alg-22-8"></span>**Algorithm 22.8.** The construction, the rank
and the select of `RRRBitVector`
([Raman, Raman and Rao 2007](Z-bibliography.md#ref-raman2007)).

--8<-- "generated/RRRBitVector-build.pseudo.md"

--8<-- "generated/RRRBitVector-rank1.pseudo.md"

--8<-- "generated/RRRBitVector-select1.pseudo.md"

Note that a query walks only the places it needs: the rank stops at its place or at
the last 1, the select at its bit. We write the walk with no branch, the binomials coming from a table of $64 \times 64$ entries.

`select0()` does the same with the 0s of each block, counted as its length less its class. We rebuild the block in full when it is sparse in 0s.

## 22.6 Sequences of integers {#sec-22-6}

In the representation of [Elias 1974](Z-bibliography.md#ref-elias1974) and
Fano (chapters 11 and 15 of [Ferragina 2023](Z-bibliography.md#ref-ferragina2023),
`EliasFano.h`) one codes a non-decreasing sequence
$x_0 \le \cdots \le x_{n-1}$ of integers of $[ 0 , u )$ in about the bits
of the lower bound, and searches it in place.

<span id="thm-22-2"></span>**Theorem 22.2 (Elias and Fano).** *Let each
$x_i$ be split into its $\ell = \lceil \log_2( u / n ) \rceil$ low bits,
written in $L$, and its bucket $\lfloor x_i / 2^\ell \rfloor$, written by
setting the bit*

$$
H[ \lfloor x_i / 2^\ell \rfloor + i ] = 1 . \tag{22.9}
$$

*Then the sequence takes at most $2 n + n \lceil \log_2( u / n ) \rceil$
bits, and $x_i = 2^\ell ( \mathrm{select}_1( H , i + 1 ) - i ) + L[ i ]$.*

*Proof.* Note that the places (22.9) are strictly increasing, since the buckets do not decrease and $i$ grows, and therefore the $( i + 1 )$-th 1 of $H$ is the one of
$x_i$, from which its bucket is read back by subtracting $i$. Also, the 0s of $H$ before that 1 are as many as the bucket, and hence $H$ has $n$ 1s and at most
$\lfloor ( u - 1 ) / 2^\ell \rfloor + 1 \le n$ 0s, since
$2^\ell \ge u / n$; $L$ has $n \ell$ bits. $\square$

Against the $n \log_2( u / n ) + n \log_2 e$ bits of the lower bound, this
is less than 2 bits per item more when $n$ is much smaller than $u$
([Figure 22.5](22-succinct.md#fig-22-5)). With the directory of
`RankSelect` on $H$, `access( i )` takes $O( 1 )$ time, a select and a
field ([Algorithm 22.9](22-succinct.md#alg-22-9)).

![Figure 22.5. The sequence 3, 4, 7, 13, 14, 15, 21, 43 in $[ 0 , 44 )$ by Elias-Fano, with $\ell = \lceil \log_2( 44 / 8 ) \rceil = 3$: the low parts $L$, the buckets, and $H$, a 1 for each item and a 0 at the end of each bucket. The access to $x_6$ (violet) finds the seventh 1 of $H$ at the place 8.](../figures/succinct-eliasfano.svg){#fig-22-5}

<span id="alg-22-9"></span>**Algorithm 22.9.** The construction, the
access and the search of Elias-Fano.

--8<-- "generated/EliasFano-BasicEliasFano-build.pseudo.md"

--8<-- "generated/EliasFano-BasicEliasFano-access.pseudo.md"

--8<-- "generated/EliasFano-BasicEliasFano-nextGEQ.pseudo.md"

`nextGEQ( x )` gives the place of the first item $\ge x$, which is the
rank of $x$ (the items $< x$) and gives the predecessor and the successor.

<span id="lem-22-5"></span>**Lemma 22.5.** *Let $h = \lfloor x / 2^\ell
\rfloor$. The first item $\ge x$ is either in the bucket $h$, whose items
start after the $h$-th 0 of $H$, or it is the first item of a later
bucket; and among the items of the bucket $h$ the low bits decide, since
they are sorted.*

*Proof.* Clearly, the items of a bucket $h' < h$ are below $h 2^\ell \le x$, and
those of a bucket $h' > h$ are at least $( h + 1 ) 2^\ell > x$. Those of the bucket $h$ share the high bits of $x$, and they are compared with
$x$ by their low bits, which are in non-decreasing order within the bucket.
Finally, the $h$-th 0 of $H$ ends the bucket $h - 1$, and the 1s after it, up to the
next 0, are the items of the bucket $h$. $\square$

Hence `nextGEQ()` costs a $\mathrm{select}_0$ and a scan of the bucket, in
$O( 1 + c )$ time for a bucket of $c$ items, and `contains( x )` tests its
result ($\ell$ itself is `lowWidth()`). A bucket holds $n 2^\ell / u \le 2$ items on average (for "random" items), and the search is $O( 1 )$ expected when the items are spread evenly, which is often (but not always) the case. Past 8 items, which the "clusters" of a sequence may give, the end of the bucket is found by
$\mathrm{select}_0$ too and its low bits by a binary search, in
$O( \log c )$ time.

Ferragina's book computes the rank by a binary search on the select, in
$O( \log n )$ time; the bucket found by $\mathrm{select}_0$ is the design
of the quasi-succinct indices of [Vigna 2013](Z-bibliography.md#ref-vigna2013)
and of the sdarray of [Okanohara and Sadakane 2007](Z-bibliography.md#ref-okanohara2007).
Viewed as the places of the 1s of a vector of $u$ bits with $n$ 1s, as in
chapter 15 of Ferragina, `access( i )` is $\mathrm{select}_1( i + 1 )$ of
that vector and `nextGEQ( i )` is $\mathrm{rank}_1( i )$, in
$n ( 2 + \log( u / n ) ) + o( n )$ bits rather than $u + o( u )$. We write
$H$ with the 1s for the items and the 0s for the buckets, as in chapter 11
of Ferragina; its chapter 15 swaps them, which only swaps
$\mathrm{select}_0$ and $\mathrm{select}_1$. `LazyEliasFano` builds the
inventories of $H$ at the first query that needs them, as `LazyRankSelect`
does.

Elias-Fano depends only on $n$ and $u$, and it ignores the "clusters" of a
sequence. In the partitioned form of [Ottaviano and Venturini 2014](Z-bibliography.md#ref-ottaviano2014)
(`PartitionedEliasFano`) we cut a strictly increasing sequence into blocks of 128 items. We code the block $j$, of $m$ items, relative to $b_j = x_{128 j - 1} + 1$, the end of the block before (0 for the first block). Its values $v = x - b_j$ are in $[ 0 , u_j )$, with $u_j$ the last of them plus 1, and the block is coded in the cheapest of three codes
([Figure 22.6](22-succinct.md#fig-22-6)). It takes no bit if $u_j = m$ (all the values are there). It is a vector of $u_j$ bits, the bit $v$ for the value $v$, if $u_j$ is at most the bits of Elias-Fano, and otherwise it is Elias-Fano with $\ell = \lfloor \log_2( u_j / m ) \rfloor$, in
$m \ell + m + \lfloor ( u_j - 1 ) / 2^\ell \rfloor + 1$ bits. `kinds()`
counts the blocks of each of the three codes, and `contains( x )` is a
`nextGEQ()` here too, as in `LAVector`.

![Figure 22.6. The sequence 3, 4, 5, 6, 7, 8, 9, 10, 20, 41, 70, 99 in blocks of 4 items (128 in the library): the first block is a vector of 7 bits, the second (violet) holds all the values of its range and takes no bit, the third is coded by Elias-Fano; each block has a record with its last item and the place of its code.](../figures/succinct-pef.svg){#fig-22-6}

<span id="lem-22-6"></span>**Lemma 22.6.** *A block of $m$ items with
$u > m$ costs at most $\min\{ u , \, 2 m + m \lceil \log_2( u / m ) \rceil \}$
bits, i.e., never more than Elias-Fano with the $\ell$ of
[Theorem 22.2](22-succinct.md#thm-22-2) on its universe (with equality for
$u = m 2^\ell$); hence the codes of all the blocks take at most*

$$
\sum_j m_j \Bigl( 2 + \Bigl\lceil \log_2 \frac{u_j}{m_j} \Bigr\rceil \Bigr)
\le n \Bigl( 3 + \log_2 \frac{u}{n} \Bigr) \tag{22.10}
$$

*bits, at most one bit per item above the bound of Theorem 22.2 with the
logarithm not rounded up.*

*Proof.* Let $\ell = \lfloor \log_2( u / m ) \rfloor$, the width of the
code, so that $m 2^\ell \le u < m 2^{\ell + 1}$. Its Elias-Fano form takes
$m \ell + m + \lfloor ( u - 1 ) / 2^\ell \rfloor + 1$ bits, and
$( u - 1 ) / 2^\ell < 2 m$ gives at most $m \ell + 3 m$. If $u / m$ is not
a power of 2, $\lceil \log_2( u / m ) \rceil = \ell + 1$ and this is
$2 m + m \lceil \log_2( u / m ) \rceil$. If $u = m 2^\ell$, the term
$\lfloor ( u - 1 ) / 2^\ell \rfloor + 1$ is $m$, and the cost is
$m \ell + 2 m = 2 m + m \log_2( u / m )$. The code taken is the cheapest of
the vector of $u$ bits and of this form (and no bit when $u = m$), which
gives the first claim. For the sum, $\lceil y \rceil < y + 1$ gives
$\sum_j m_j ( 3 + \log_2( u_j / m_j ) )$, and $\sum_j u_j \le u$, since the
universes of the blocks are disjoint ranges of $[ 0 , u )$; by the
concavity of the logarithm (the inequality of Jensen on the weights
$m_j / n$), $\sum_j m_j \log_2( u_j / m_j ) \le n \log_2( \sum_j u_j / n )
\le n \log_2( u / n )$. $\square$

We remark that the extra bit per item of (22.10) comes from the rounding
of the logarithm in each block, and it can be reached: on two blocks with
$u_1 / m = 1.5$ and $u_2 / m = 2.5$ the ceilings sum to 3 per item, while
the whole sequence, with $u / n = 2$, has 1. A program that compares the
Elias-Fano form of the code with the formula of Theorem 22.2 for all
$m < 300$ and $m < u < 50 m$ finds it never above, and equal on the
powers of 2.

Besides, the records of the blocks cost a field of fixed width per block: the last item, the place of the code, and its kind with $\ell$ in 8 bits (so that a query computes no logarithm). Also, the last item of every eighth block is kept apart for the searches. On the dense runs of an inverted index the cost is usually far smaller than that of Elias-Fano. `access( i )` reads the
records of the block of $i$ and of the one before (next to each other) and
selects the item in the code of the block, a scan of its few words. Also, `nextGEQ( x )` finds the first block whose last item is at least $x$ by a search of the items apart in the layout of Eytzinger ([Khuong and Morin 2017](Z-bibliography.md#ref-khuong2017)), a complete binary tree by levels whose descent fetches the nodes three levels ahead. At most 8 records follow, and the item is found in its block by the high
part, or by the sideways sums of the vector up to $x$
([Algorithm 22.10](22-succinct.md#alg-22-10)). Ottaviano and Venturini also
choose the ends of the blocks by an approximate dynamic program on the cost
of the codes, which gains a further 10% or so on the lists of an index; the
blocks here have the fixed size of their uniform partition.

<span id="alg-22-10"></span>**Algorithm 22.10.** The construction, the
access and the search of the partitioned Elias-Fano
([Ottaviano and Venturini 2014](Z-bibliography.md#ref-ottaviano2014)).

--8<-- "generated/PartitionedEliasFano-build.pseudo.md"

--8<-- "generated/PartitionedEliasFano-access.pseudo.md"

--8<-- "generated/PartitionedEliasFano-nextGEQ.pseudo.md"

Elias-Fano spends on each item the same bits, whatever the regularity of
the gaps. In the LA-vector of [Boffa, Ferragina and Vinciguerra 2021](Z-bibliography.md#ref-boffa2021) (`LAVector`) one spends them instead on
the distance of each item from a line. We cover the points $( i , x_i )$ by the fewest segments whose lines are within $\epsilon$ of each of their
points, the optimal piecewise linear approximation of the PGM-index
([Ferragina and Vinciguerra 2020](Z-bibliography.md#ref-ferragina2020),
`ADS/search/PGMIndex.h`), and each item is stored as its *correction* with
respect to the line of its segment ([Figure 22.7](22-succinct.md#fig-22-7)).

![Figure 22.7. The points $( i , x_i )$ of a sequence of 16 items covered by two segments, each line with its band of half-width $\epsilon = 2$; the item 5 (violet) is stored as its correction $C[ 5 ]$, its distance from the line of its segment.](../figures/succinct-lavector.svg){#fig-22-7}

<span id="lem-22-7"></span>**Lemma 22.7 (the corrections).** *Let the
segment $j$, over the places $[ p_j , p_{j+1} )$, have a line
$f( i ) = x_{p_j} + \gamma + \alpha ( i - p_j )$ with
$| f( i ) - x_i | \le \epsilon$ for each of its places, and let
$d_i = x_i - x_{p_j} - \lfloor \alpha ( i - p_j ) \rfloor$. Then the $d_i$
take at most $2 \epsilon + 2$ values, and with $\beta_j = x_{p_j} +
\min_i d_i$*

$$
x_i = \beta_j + \lfloor \alpha ( i - p_j ) \rfloor + C[ i ] , \qquad
0 \le C[ i ] < 2^c \text{ for } \epsilon = 2^{c-1} - 1 . \tag{22.11}
$$

*Proof.* $x_i - x_{p_j} - \alpha ( i - p_j )$ lies in
$[ \gamma - \epsilon , \gamma + \epsilon ]$, and taking off the integer
part of $\alpha ( i - p_j )$ instead of the whole of it adds less than 1.
Hence the $d_i$ are integers in an interval of length less than
$2 \epsilon + 2$, at most $2 \epsilon + 2 = 2^c$ of them, and
$C[ i ] = d_i - \min_i d_i$ takes $c$ bits. $\square$

In the code the slope $\alpha$ is kept in a `double`, whose rounding may spread the
$d_i$ by a few units on items of 64 bits. `LAVector` therefore computes the
corrections with the rounded slope and gives each segment the width $w_j$
of the range of the corrections of the segment, so that (22.11) holds with $w_j$ in
place of $c$ ($c$, or one bit more). One can count the space as follows. A segment takes 24 bytes (the base, the slope, and the place of its corrections with their width; its first item is its base plus its first correction). Its first place takes a field of $\lceil \log_2 n \rceil$ bits, and the two tables that find a segment by a place and by a value about two fields of $\lceil \log_2 m \rceil$ bits per segment. Hence the sequence takes about

$$
\sum_j w_j ( p_{j+1} - p_j ) + m \big( 192 + \lceil \log_2 n \rceil +
2 \lceil \log_2 m \rceil \big) \tag{22.12}
$$

bits for $m$ segments ([Algorithm 22.11](22-succinct.md#alg-22-11)).

<span id="alg-22-11"></span>**Algorithm 22.11.** The construction, the
access and the search of the LA-vector
([Boffa, Ferragina and Vinciguerra 2021](Z-bibliography.md#ref-boffa2021)),
and the choice of the width of the corrections.

--8<-- "generated/LAVector-build.pseudo.md"

--8<-- "generated/LAVector-access.pseudo.md"

--8<-- "generated/LAVector-nextGEQ.pseudo.md"

--8<-- "generated/LAVector-tuned.pseudo.md"

Accessing $x_i$ is (22.11) once the segment of $i$ is known, which a
table of the segment of every $2^t$-th place gives in $O( 1 )$ expected
time for $2^t \approx n / m$. In turn, the rank of $x$ first finds the last segment
whose first item is below $x$, through a table of the first segment of each
bucket of the high bits of the items, as the buckets of Elias-Fano. Then,
since by (22.11) $\beta_j + \lfloor \alpha ( i - p_j ) \rfloor \le x_i <
\beta_j + \lfloor \alpha ( i - p_j ) \rfloor + 2^{w_j}$, the answer is
among the places where the line goes from $x - 2^{w_j}$ to $x + 1$, about
$2^{w_j} / \alpha$ of them. A binary search on the corrections settles it
in $O( w_j - \log_2 \alpha )$ steps; when the floating point misplaces the
window, which the search checks on the two items at its ends, the whole
segment is searched.

Against Elias-Fano, which takes $2 + \lceil \log_2( u / n ) \rceil$ bits per item, the LA-vector may win when the items follow lines "closely". This happens, e.g., on the places of a pattern repeated with some noise, on the keys of a process that grows at a steady rate, or on the dense runs of an inverted index, where $m$ is small and $c$ is the logarithm of the noise instead of the average gap. On gaps drawn at random no line may follow the items better than the average gap does, the corrections need about $\log_2( u / n )$ bits as
the low bits of Elias-Fano, and the segments are an overhead. `tuned()`
therefore tries the widths up to six bits above the logarithm of the
average gap (or half the bits of the items, if more) and keeps the one of
the fewest bits. In the journal form of [Boffa, Ferragina and Vinciguerra 2022](Z-bibliography.md#ref-boffa2022), the LA-vector chooses the width segment by segment, by a shortest path over the segments
of all the widths. Its gain comes from segments of different widths, which the width $w_j$ that each segment takes from its corrections already gives in part. On the sequences of the benchmark of the library the shortest path came within a few hundredths of the bits of `tuned()`, with a slower construction, and it is therefore not kept.

## 22.7 Arrays of packed fields {#sec-22-7}

An array of $n$ values below $2^B$ takes $n B$ bits when its fields are
packed in words, against the $8 n$ of a byte per value. In the searches, the tables of "large" spaces of states may have $10^9$ entries and more, e.g., the pattern databases of the heuristic search ([Felner, Korf and Hanan 2004](Z-bibliography.md#ref-felner2004)), and they fit in the memory only so packed.
`PackedArray< B >` takes the widths that divide 64, $B \in \{ 1 , 2 , 4 , 8 , 16 , 32 \}$. A word then holds $64 / B$ fields (the constant `PER`, and `MAX` is $2^B - 1$, the largest value of a field), and the field $i$ is the bits $( i \bmod 64 / B ) B$ on of the word $\lfloor i B / 64 \rfloor$. A read is one load, one shift and one mask, with no test of a field that crosses two words, which `BitVector::get( i , len )` pays for the other widths
([Figure 22.8](22-succinct.md#fig-22-8)). A write clears the field by a mask
and puts the value by an or, in the same word.

A *saturated* array (the template parameter `Saturate`) stores a value
above $2^B - 1$ as $2^B - 1$, and counts it in `clamped()`, instead of
breaking the contract. When the values are lower bounds, e.g., the
distances of a pattern database, a value cut down is still a "good" lower bound,
and $B$ is then chosen for most of the values rather than for the largest
one.

![Figure 22.8. Above, a word of `PackedArray< 4 >` with its 16 fields, the field 15 on the left; the field 5 (violet) was written by `set( 5 , 9 )`. Below, a byte of `TritArray`, which holds five values in base 3, and the write of one digit by an addition.](../figures/succinct-packed.svg){#fig-22-8}

A value of three states carries $\log_2 3 \approx 1.585$ bits of
information, but takes 2 bits in a field of `PackedArray< 2 >`. We may put five such values (*trits*) $t_0 , \ldots , t_4$ in one byte as the number

$$
y = \sum_{j = 0}^{4} t_j 3^j \le 3^5 - 1 = 242 , \tag{22.13}
$$

i.e., in 1.6 bits per value, 0.015 above the entropy and one fifth less than the fields of 2 bits ([Breyer and Korf 2010](Z-bibliography.md#ref-breyer2010), whose tables keep the distances modulo 3, from which the distance of a state is recovered along a path to the goal). `TritArray` reads the digit $j = i \bmod 5$ of the byte $\lfloor i / 5 \rfloor$ from a table of the five digits of each of the 243 bytes, 1215 bytes that stay in the cache of the first level. It writes the digit by adding $( v - t_j ) 3^j$ to the byte, which by
(22.13) changes only that digit.

Six values would need $3^6 = 729 > 256$ codes, and $3^{40} < 2^{64} <
3^{41}$ would put 40 values in a word, 1.6 bits each again, at the price of
a division by a power of 3 per read; the byte is the unit with the same
density and a read by a table. Both arrays are also built from a function
$f$ of the place by blocks of whole words (of whole bytes for the trits) on
the threads of an executor ([Algorithm 22.12](22-succinct.md#alg-22-12)). A
word is written by the one thread that owns its block, and therefore no
write needs an atomic and the array is the same as the sequential one; the
work is $O( n )$ calls of $f$, with the depth of one block.

<span id="alg-22-12"></span>**Algorithm 22.12.** The parallel construction
and the write of `PackedArray` and of `TritArray`.

--8<-- "generated/PackedArray-build.pseudo.md"

--8<-- "generated/PackedArray-set.pseudo.md"

--8<-- "generated/PackedArray-TritArray-build.pseudo.md"

--8<-- "generated/PackedArray-TritArray-set.pseudo.md"

## 22.8 Trees {#sec-22-8}

One can code a tree by its shape and nothing more, in about the $2 n$ bits of the
lower bound of [§22.1](22-succinct.md#sec-22-1), and its nodes are then
numbered by the places of their bits; the data of a node go in a vector
indexed by its number. We give three codes, which differ in the operations
that they answer in constant time.

<span id="lem-22-8"></span>**Lemma 22.8 (the binary tree of
Jacobson).** *Complete a binary tree of $n$ nodes with a leaf in place of
each missing child, and write by levels a 1 for a node and a 0 for a leaf
added, in $B[ 1 , 2 n + 1 ]$. Then the children of the $x$-th node are at
the places $2 x$ and $2 x + 1$, and the item at the place $p$ is the
$\mathrm{Rank}_1( p )$-th node if $B[ p ] = 1$.*

*Proof.* A visit by levels lists the root at the place 1, and then the children of the nodes in the order of the nodes, two per node; hence the
children of the first $x - 1$ nodes take the places $2 , \ldots ,
2 x - 1$, and those of the $x$-th one the next two. $\square$

One can see that, with rank and select in $O( 1 )$ time, the left child of
the $x$-th node is the $\mathrm{Rank}_1( 2 x )$-th node, if $B[ 2 x ] = 1$,
and its parent the $\lfloor \mathrm{Select}_1( x ) / 2 \rfloor$-th; in the
notation of Ferragina's book, which names a node by its place $p$, the left
child is at the place $2 \, \mathrm{Rank}_1( p )$ and the parent at
$\mathrm{Select}_1( \lfloor p / 2 \rfloor )$. Hence `JacobsonTree`
([Jacobson 1989](Z-bibliography.md#ref-jacobson1989)) takes
$2 n + 1 + o( n )$ bits, within $O( \log n )$ of the lower bound, with
`left()`, `right()` and `parent()` in $O( 1 )$ time
([Figure 22.9](22-succinct.md#fig-22-9)). In the library the nodes are numbered $0 , \ldots , n - 1$ by levels and the places from 0. Then the node $v$ is at the place $\mathrm{select}_1( v + 1 )$, its left child is the node $\mathrm{rank}_1( 2 v + 1 )$ if that place holds a 1, and its parent is
$\lfloor ( \mathrm{select}_1( v + 1 ) + 1 ) / 2 \rfloor - 1$
([Algorithm 22.13](22-succinct.md#alg-22-13)). This representation answers
the navigation, but not the size of a subtree, which needs the order of a
visit in depth.

![Figure 22.9. A binary tree of 6 nodes completed with a leaf (faintest tint) in place of each missing child, the places of its nodes by levels from 1, and its code $B$ of $2 n + 1 = 13$ bits; C, the third node, has its children E and F at the places 6 and 7 (violet).](../figures/succinct-jacobson.svg){#fig-22-9}

<span id="alg-22-13"></span>**Algorithm 22.13.** The code by levels of a
binary tree, and its left child and parent
([Jacobson 1989](Z-bibliography.md#ref-jacobson1989)).

--8<-- "generated/JacobsonTree-levelCode.pseudo.md"

--8<-- "generated/JacobsonTree-left.pseudo.md"

--8<-- "generated/JacobsonTree-parent.pseudo.md"

For a tree of any degree, LOUDS (the level-order unary degree sequence of
Jacobson, `LoudsTree`) puts a dummy root of degree 1 above the root and
writes the degree $d$ of each node, by levels, as $0^d 1$. Hence the code has $n$ 0s, one per node (each node is the child of somebody), and $n + 1$ 1s,
one per node and the dummy root, i.e., $2 n + 1$ bits
([Figure 22.10](22-succinct.md#fig-22-10)).

<span id="lem-22-9"></span>**Lemma 22.9.** *In the code of LOUDS, the
$k$-th 0 is the $k$-th node by levels, and the children of the $k$-th node
are the 0s after the $( k + 1 )$-th 1. Hence, with the nodes numbered from
0 and the places from 0,*

$$
\mathrm{parent}( v ) = \mathrm{select}_0( v + 1 ) - v - 1 , \qquad
\mathrm{leftmostChild}( v ) = \mathrm{select}_1( v + 1 ) - v , \tag{22.14}
$$

*the latter when a 0 follows the $( v + 1 )$-th 1, and
$\mathrm{degree}( v ) = \mathrm{select}_1( v + 2 ) - \mathrm{select}_1( v +
1 ) - 1$.*

*Proof.* Each node writes a 0 for each of its children, in the order by levels. These 0s come in the order of the nodes that they stand for, since the visit by levels lists the children of the nodes in the order of the nodes, and the dummy root writes the 0 of the root. Furthermore, the $( k + 1 )$-th 1 ends the unary degree of the $k$-th node (the first 1 being that of the
dummy root), and its children are the 0s that follow it. In particular, the 0 of $v$ is at
$q = \mathrm{select}_0( v + 1 )$, and the 1s before it are $q - v$, of which
the first is the dummy root's; hence its parent is the node $q - v - 1$.
Similarly, the 0 after the $( v + 1 )$-th 1, at $p + 1$ with
$p = \mathrm{select}_1( v + 1 )$, is the $( p + 1 - v )$-th 0, i.e., the
node $p - v$. $\square$

![Figure 22.10. An ordered tree of 8 nodes, numbered by levels and, in parentheses, in preorder. Above, its code by LOUDS after the dummy root 01, the unary degree of each node bracketed; the degree of the node 1 (violet) is the 0s 6 and 7, its children 4 and 5. Below, its balanced parentheses in preorder and their excess $E$: the pair of the node 1 opens at 1 and closes at 8, the first place after 1 where the excess falls to $E( 1 ) - 1 = 1$ (dashed).](../figures/succinct-louds-bp.svg){#fig-22-10}

Its functions are `leftmostChild()` and `rightSibling()`, the names of the
TREE of the book (`ADS/tree/Tree.h`); the $i$-th child is the first one
plus $i$, since the children of a node are consecutive in the order by
levels, which a visit in depth does not give ([Algorithm 22.14](22-succinct.md#alg-22-14)).
LOUDS does not answer the size of a subtree nor the depth of a node; it is the simplest of the succinct trees and, arguably, the "fastest" for the
navigation from the root down
([Arroyuelo, Cánovas, Navarro and Sadakane 2010](Z-bibliography.md#ref-arroyuelo2010)),
e.g., in the tries of the indices.

<span id="alg-22-14"></span>**Algorithm 22.14.** The code by LOUDS of an
ordered tree.

--8<-- "generated/LoudsTree-louds.pseudo.md"

In turn, the balanced parentheses (`BPTree`, after
[Munro and Raman 2001](Z-bibliography.md#ref-munro2001)) write an open
parenthesis, a 1, when a visit in depth enters a node, and a closed one, a
0, when it leaves it. Hence the nodes of a subtree are consecutive in preorder, and a node is the pair of its parentheses. With the excess
$E( i )$, the open parentheses less the closed ones in $B[ 0 , i ]$ (and
$E( -1 ) = 0$), one has

$$
\mathrm{findClose}( i ) = \min \{ j > i : E( j ) = E( i ) - 1 \} ,
\tag{22.15}
$$

since the parentheses strictly inside the pair of $i$ are balanced and keep
the excess at least $E( i )$ (cf. [Figure 22.10](22-succinct.md#fig-22-10)).
Similarly, `findOpen( j )` is $k + 1$ for the last $k < j$ with
$E( k ) = E( j )$, and `enclose( i )`, the open parenthesis of the pair
that encloses the one at $i$, is $k + 1$ for the last $k < i$ with
$E( k ) = E( i ) - 2$. Since the excess moves by $\pm 1$, one can see that the first place with an excess at most a target has that very excess, and every
operation is therefore a search for the first place, forward or backward,
whose excess reaches a target. Note that the excess of a place is
$E( i ) = 2 \, \mathrm{rank}_1( i + 1 ) - ( i + 1 )$, in $O( 1 )$ time.

`BalancedParentheses` answers the searches by the range min-max tree of
[Navarro and Sadakane 2014](Z-bibliography.md#ref-navarro2014a). It keeps,
for each leaf of 512 bits, for each group of 16 leaves and for each node of
a complete binary tree over the groups, the total excess of its range and
its minimum prefix excess. A search runs on the excess relative to its
start, and needs no rank. It scans the leaf of $i$ a byte at a time, with the excess, the minimum and the place of each target of each of the 256 bytes in tables. Then it scans the leaves of the group of $i$ by their excess and minimum in 16 bits. At last it climbs the tree, whose leaves are the groups, to the first range on its side whose minimum reaches the target, and goes down to its group, its leaf and its byte
([Algorithm 22.15](22-succinct.md#alg-22-15)). A range whose minimum stays
above the target holds no answer, and it is skipped whole; the search thus
takes $O( \log( n / 8192 ) + 16 + 512 / 8 )$ time, and most searches of a tree usually end in the leaf of $i$, at the cost of (usually) one miss of the cache.

<span id="alg-22-15"></span>**Algorithm 22.15.** The range min-max tree, and
the forward search of `findClose()`
([Navarro and Sadakane 2014](Z-bibliography.md#ref-navarro2014a)).

--8<-- "generated/BalancedParentheses-build.pseudo.md"

--8<-- "generated/BalancedParentheses-findClose.pseudo.md"

--8<-- "generated/BalancedParentheses-findOpen.pseudo.md"

--8<-- "generated/BalancedParentheses-enclose.pseudo.md"

In space, the tree takes $32 / 512$ bits per parenthesis for the leaves (6.25%) and
at most $4 \cdot 128 / 8192$ for the groups, besides the directory of rank
of `CompactRankSelect`, 3.5% of the bits, since no search needs a rank.
`forwardSearch( i , d )` gives the first $j > i$ with
$E( j ) - E( i ) = d < 0$, which may also be the end of a subtree of the code
by the degrees in depth (`ADS/dictionary/SuccinctPatriciaTrie.h`).
[Jacobson 1989](Z-bibliography.md#ref-jacobson1989) gives `findClose()` in
$O( \log n )$ time with the pioneers, and Munro and Raman in $O( 1 )$ time
with two levels of pioneers and tables, in $2 n + o( n )$ bits. Then the range min-max tree of Navarro and Sadakane reaches $O( 1 )$ time with a tree of degree $\Theta( \log n )$ and tables. Its binary form with blocks of a few
hundred bits, the one here, is usually the fastest in practice, at least on the trees of the benchmarks
([Arroyuelo, Cánovas, Navarro and Sadakane 2010](Z-bibliography.md#ref-arroyuelo2010);
[Gog and Petri 2014](Z-bibliography.md#ref-gog2014a);
[Grossi and Ottaviano 2013](Z-bibliography.md#ref-grossi2013)).

`BPTree` numbers the nodes in preorder, the node $v$ being the
$( v + 1 )$-th open parenthesis. `parent( v )` is `enclose()` of its open
parenthesis, `leftmostChild( v )` is $v + 1$ if the next parenthesis is an
open one, and `rightSibling( v )` is the open parenthesis after its closed
one, if any ([Algorithm 22.16](22-succinct.md#alg-22-16)). Furthermore,
`subtreeSize( v )` is $( j - i + 1 ) / 2$ for its parentheses at $i$ and
$j$, `depth( v )` is the excess at its open parenthesis less 1, and $u$ is
an ancestor of $v$ if $u \le v < u + \mathrm{subtreeSize}( u )$, the test
of `isAncestor( u , v )`. These may cost a search of the range min-max tree, or a select and a rank for
`leftmostChild()`, `depth()` and `isLeaf()`. Against LOUDS, the parentheses
give the size of a subtree, the depth and the ancestors, and number the
nodes of a subtree consecutively (from $v$ to
$v + \mathrm{subtreeSize}( v ) - 1$), at the price of the searches. We remark that DFUDS of [Benoit et al. 2005](Z-bibliography.md#ref-benoit2005), the degrees in unary in preorder, also gives the $i$-th child in $O( 1 )$ time with the same matching; chapter 15 of Ferragina does not treat it, and neither does the library.

<span id="alg-22-16"></span>**Algorithm 22.16.** The parentheses of a tree
by a visit in depth, and the parent and the next sibling in `BPTree`.

--8<-- "generated/BPTree-parentheses.pseudo.md"

--8<-- "generated/BPTree-parent.pseudo.md"

--8<-- "generated/BPTree-rightSibling.pseudo.md"

[Table 22.3](22-succinct.md#tab-22-3) compares the trees, and the graphs of
[§22.10](22-succinct.md#sec-22-10).

<span id="tab-22-3"></span>**Table 22.3.** The trees and the graphs and
their costs, for $n$ nodes and $m$ edges.

| Realization | Space in bits | Operations |
|---|---|---|
| `JacobsonTree` | $2 n + 1 + o( n )$ | the children and the parent of a binary tree in $O( 1 )$ |
| `LoudsTree` | $2 n + 1 + o( n )$ | the parent, the degree, the $i$-th child and the next sibling in $O( 1 )$ |
| `BPTree`, on `BalancedParentheses` | $2 n$, and 6.25% of them for the leaves of the range min-max tree, at most as much for its groups and 3.5% for a rank directory | also the size of a subtree, the depth and the ancestors, by a search of the excess |
| `EliasFanoGraph` | $m ( 2 + \lceil \log_2( n^2 / m ) \rceil ) + n ( 2 + \lceil \log_2( m / n ) \rceil )$ | the outdegree and the $i$-th successor in $O( 1 )$, an edge in $O( 1 )$ expected |
| `BVGraph` | about 3 per edge on the Web graphs | a list in $O( R d )$, $R$ the longest chain of references |
| `K2Tree` | $k^2 m ( \log_{k^2}( n^2 / m ) + O( 1 ) )$ in the worst case | an edge in $O( h )$, the successors and the predecessors of a node |

## 22.9 Sequences of symbols and permutations {#sec-22-9}

Our wavelet matrix, that of [Claude, Navarro and Ordóñez 2015](Z-bibliography.md#ref-claude2015)
(`WaveletMatrix`), holds a static sequence $S[ 0 .. n - 1 ]$ of integers of
$w$ bits in $n w$ bits plus the directories of rank and select, with the
access, the rank and the select of any symbol in $O( w )$ time.

<span id="lem-22-10"></span>**Lemma 22.10 (the wavelet matrix).** *Let the
level $\ell$ of $S$ hold the bit $w - 1 - \ell$ of each item, the items
passing to the next level stably sorted by that bit, the $Z_\ell$ with a 0
first. Then the item at the place $i$ of a level is at the place*

$$
\mathrm{rank}_0( B_\ell , i ) \text{ if its bit is 0}, \qquad
Z_\ell + \mathrm{rank}_1( B_\ell , i ) \text{ if it is 1} , \tag{22.16}
$$

*of the next one, and the items equal to $c$ end consecutive, in the order
of $S$, at the last level.*

*Proof.* A stable partition by a bit sends an item to the place given by
the items with its bit before it, which is (22.16), and keeps the order
among the items with the same bit. By induction on the levels, the items
that share the first $\ell$ bits stay consecutive, in the order of $S$, and
the place of the range of $c$ at the last level depends on the bits of $c$
alone. $\square$

![Figure 22.11. The wavelet matrix of the sequence 4 7 6 5 3 2 1 0 2 1 4 1 of 3 bits: each level holds a bit of each item (the items above the bits, in the order of the level), the 0s first at the next level; the access to the item 2, the 6 = 110 (violet), moves to the places 9 and 11 by (22.16).](../figures/succinct-waveletmatrix.svg){#fig-22-11}

Hence `access()`, `rank()` and `select()` follow a place, or the two ends of
a range, down the $w$ levels by (22.16), or up by the select, in $O( w )$
time ([Figure 22.11](22-succinct.md#fig-22-11)); `rank( c , i )` follows
the places 0 and $i$ down along the bits of $c$, and the difference at the
last level is the answer.

Furthermore, the $k$-th smallest item of a range, `quantile( i , j , k )`, goes down to
the 0s or to the 1s according to how many 0s the range holds at each level,
and `countLess( i , j , c )` sums the 0s that the range leaves at the levels
where $c$ has a 1 ([Algorithm 22.17](22-succinct.md#alg-22-17)). Instead, the wavelet tree of [Grossi, Gupta and Vitter 2003](Z-bibliography.md#ref-grossi2003) splits the alphabet in halves at its nodes, which need a pointer or an offset each. Our matrix keeps instead one vector of $n$ bits per level and no tree, at the same costs, and it is arguably the form of choice for the large alphabets, and the base
of the compressed indices of the integers ([Navarro 2014](Z-bibliography.md#ref-navarro2014b)
surveys the uses of the wavelet trees). Each level carries the directory of
`RankSelect`, and the matrix therefore takes about $1.5 \, n w$ bits, a
query of rank being $2 w$ ranks of one load and one sideways sum each.

<span id="alg-22-17"></span>**Algorithm 22.17.** The access, the rank, the
select, the quantile and the count of the wavelet matrix
([Claude, Navarro and Ordóñez 2015](Z-bibliography.md#ref-claude2015)).

--8<-- "generated/WaveletMatrix-access.pseudo.md"

--8<-- "generated/WaveletMatrix-rank.pseudo.md"

--8<-- "generated/WaveletMatrix-select.pseudo.md"

--8<-- "generated/WaveletMatrix-quantile.pseudo.md"

--8<-- "generated/WaveletMatrix-countLess.pseudo.md"

A wavelet tree shaped by the code of Huffman
([Mäkinen and Navarro 2005](Z-bibliography.md#ref-makinen2005),
`HuffmanWaveletTree`) takes $n ( H_0 + 1 )$ bits instead of $n w$, and it may be somewhat faster on a "skewed" alphabet, quite often the case of a text. We give the symbol $c$ the canonical code of Huffman of its frequency, of length $\ell_c$ (`codeLength( c )`, 0 for a symbol absent), with its bits from the highest. Our tree is the trie of the codes, and its internal node $v$,
reached by the prefix $\pi_v$, keeps the bit after $\pi_v$ of the code of
each item whose code starts with $\pi_v$, in the order of the sequence
([Figure 22.12](22-succinct.md#fig-22-12)). We give the canonical codes by length and then by symbol, the first one 0, which is another convention than
(21.5) of [§21.2](21-compression.md#sec-21-2), with the same lengths. All the nodes have their bits in one vector $B$ with rank and select, the node $v$
from the place $o_v$, with $R_v = \mathrm{rank}_1( B , o_v )$ kept. An
item of place $i$ in the node $v$ moves to the place

$$
i' = \mathrm{rank}_b( B , o_v + i ) - \mathrm{rank}_b( B , o_v ) \tag{22.17}
$$

of its child by the bit $b = B[ o_v + i ]$. A descent along the code of $c$ gives `access( i )`, together with the rank of the symbol found (`accessRank()`, the LF-mapping of an FM-index in one descent). It also gives $\mathrm{rank}_c( i )$, one rank a step, while $\mathrm{select}_c( k )$
climbs from the leaf of $c$ by one select a step,
$k' = \mathrm{select}_b( B , \mathrm{rank}_b( B , o_v ) + k ) - o_v + 1$
([Algorithm 22.18](22-succinct.md#alg-22-18)).

![Figure 22.12. The wavelet tree of abracadabra shaped by the canonical code a 0, b 100, c 101, d 110, r 111 of its frequencies: each internal node keeps a bit for each item that reaches it (the items above the bits), and the bits of all the nodes are one vector, the nodes laid by levels.](../figures/succinct-huffmanwt.svg){#fig-22-12}

<span id="lem-22-11"></span>**Lemma 22.11.** *The bits of the nodes of
`HuffmanWaveletTree` are $\sum_c f_c \ell_c < n ( H_0 + 1 )$, for the
frequencies $f_c$ of the symbols.*

*Proof.* An item of the symbol $c$ leaves one bit in each of the $\ell_c$
internal nodes on the code of $c$. Hence the sum is $n$ times the average length
of the code of Huffman of the frequencies, which is below $H_0 + 1$ by
(21.4). $\square$

Beside the bits of $B$, the tree takes the directory of rank and select,
25% of them with `RankSelect` and 3.5% with `CompactRankSelect` (the
template parameter `RS`, time against bits), and $O( \sigma )$ words of the
nodes and of the codes. With `withSelect` false, the directory of `RankSelect` builds no inventory. This suits a tree that only ranks, such as the one of the FM-index of [§21.11](21-compression.md#sec-21-11).

<span id="alg-22-18"></span>**Algorithm 22.18.** The construction, the
access with the rank, the rank and the select of the wavelet tree of
Huffman ([Mäkinen and Navarro 2005](Z-bibliography.md#ref-makinen2005)).

--8<-- "generated/HuffmanWaveletTree-build.pseudo.md"

--8<-- "generated/HuffmanWaveletTree-accessRank.pseudo.md"

--8<-- "generated/HuffmanWaveletTree-rank.pseudo.md"

--8<-- "generated/HuffmanWaveletTree-select.pseudo.md"

Finally, the move structure of [Nishimoto and Tabei 2021](Z-bibliography.md#ref-nishimoto2021)
(`MoveStructure`) represents a permutation $\pi$ of $[ 0 , n )$ that
shifts each of $r$ intervals $[ p_i , p_{i+1} )$ alike, such as the
LF-mapping of a transform of Burrows and Wheeler on its runs, or the
function $\phi$ of the r-index ([§21.11](21-compression.md#sec-21-11)). It
keeps the starts $p_i$, their images $q_i$ and the interval $d_i$ that holds
$q_i$, so that

$$
\pi( x ) = q_i + ( x - p_i ) , \qquad x \in [ p_i , p_{i+1} ) , \tag{22.18}
$$

and the interval of $\pi( x )$ is found from $d_i$ forward, past the
starts $p_j \le \pi( x )$.

We say that the structure is *balanced* when no image $[ q_i , q_i + p_{i+1} - p_i )$
holds 4 starts or more. Its construction ensures it by cutting an interval
whose image holds 4 starts at the third of them, so that the first part
holds 2; the new start may weigh on another image (the cut may cascade), which is checked in
turn ([Algorithm 22.19](22-succinct.md#alg-22-19)). Then the intervals become at most $2 r$ (Nishimoto and Tabei, for this balance), and the search forward
takes at most 3 steps; hence each step of a walk costs $O( 1 )$ time, in
$O( r )$ words instead of the $n$ of the permutation written in full. On
the runs of the transform of abracadabra (cf. [Table 21.7](21-compression.md#tab-21-7))
the image of the run of a, the rows $[ 6 , 10 )$, is the rows $[ 2 , 6 )$,
which hold the 4 starts 2, 3, 4 and 5; the balance cuts the run at the
row 8 ([Figure 22.13](22-succinct.md#fig-22-13)). We keep the intervals and the images in two ordered maps during the construction, $O( r \log r )$ time, and each
interval carries a tag (the symbol of its run), which its parts keep.

![Figure 22.13. The LF-mapping of the transform ard\$rcaaaabb of abracadabra as a move structure: each interval of rows (above, between the bars) is shifted alike onto its image (below). The run of a (violet) is cut at the row 8 by the balance (the orange bar), since its image $[ 2 , 6 )$ held 4 starts.](../figures/succinct-move.svg){#fig-22-13}

<span id="alg-22-19"></span>**Algorithm 22.19.** The balance and the move
of [Nishimoto and Tabei 2021](Z-bibliography.md#ref-nishimoto2021).

--8<-- "generated/MoveStructure-balance.pseudo.md"

--8<-- "generated/MoveStructure-move.pseudo.md"

[Table 22.4](22-succinct.md#tab-22-4) summarizes the sequences of the area.

<span id="tab-22-4"></span>**Table 22.4.** The sequences and their costs.

| Realization | Space in bits | Access | Search |
|---|---|---|---|
| `EliasFano` | $n ( 2 + \lceil \log_2( u / n ) \rceil )$, plus the directory of $H$ | $O( 1 )$ | `nextGEQ()` in $O( 1 )$ expected, $O( \log c )$ for a bucket of $c$ items |
| `PartitionedEliasFano` | at most that of Elias-Fano on each block of 128 items, plus a record per block | a scan of the code of a block | `nextGEQ()` by a search of the records and the scan of a block |
| `LAVector` | (22.12) | $O( 1 )$ expected | `nextGEQ()` in $O( w_j - \log_2 \alpha )$ once the segment is found |
| `WaveletMatrix` | $n w$, plus a directory per level | $O( w )$ | rank, select, quantile and range counts in $O( w )$ |
| `HuffmanWaveletTree` | less than $n ( H_0 + 1 )$, plus the directory and $O( \sigma )$ words | $O( \ell_c )$, $H_0 + 1$ on average | rank and select of $c$ in $O( \ell_c )$ |
| `MoveStructure` | $O( r )$ words for a permutation of $r$ intervals | the image of a place and its interval in $O( 1 )$ | the interval of a place by a binary search |

## 22.10 Graphs {#sec-22-10}

A graph of $n$ nodes and $m$ edges, as sorted adjacency lists, is a set of
$m$ pairs among $n^2$, i.e., $\log_2 \binom{n^2}{m} \approx
m \log_2( n^2 / m )$ bits for a graph with no regularity.
`EliasFanoGraph` writes the edge $( u , v )$ as the integer $u n + v$. Its lists, each one sorted, are then one increasing sequence of $[ 0 , n^2 )$ coded by [Theorem 22.2](22-succinct.md#thm-22-2), and a second sequence of the prefix sums of the outdegrees gives where each list starts ([Figure 22.14](22-succinct.md#fig-22-14)). This is the
representation of the adjacency lists of large graphs in Unicorn
([Curtiss et al. 2013](Z-bibliography.md#ref-curtiss2013)) and in the
quasi-succinct indices of [Vigna 2013](Z-bibliography.md#ref-vigna2013).
Hence the outdegree and the $i$-th successor are accesses, in $O( 1 )$ time, and
an edge is a `nextGEQ()`, in $O( 1 )$ expected time
([Algorithm 22.20](22-succinct.md#alg-22-20)).

![Figure 22.14. (a) The lists of a graph of 4 nodes as the sequence of the integers $u n + v$ and the sequence of the starts of the lists; the list of 2 is the items 3 and 4 (violet). (b) The list of the node 16 of [Boldi and Vigna 2004](Z-bibliography.md#ref-boldi2004) coded by reference to that of the node 15: the copy bits of the reference (the items copied in violet), the extra nodes (orange), and the fields of its code with $L_{\min} = 2$.](../figures/succinct-graphs.svg){#fig-22-14}

<span id="alg-22-20"></span>**Algorithm 22.20.** The construction of the
graph in Elias-Fano.

--8<-- "generated/EliasFanoGraph-build.pseudo.md"

<span id="lem-22-12"></span>**Lemma 22.12.** *Coding each list apart, in the universe of the list, takes*

$$
\sum_u d_u \log_2 \frac{n}{d_u} \le m \log_2 \frac{n^2}{m} \tag{22.19}
$$

*bits beyond the $2 m$ of the unary parts, $d_u$ being the outdegree of
$u$.*

*Proof.* By the concavity of the logarithm (the inequality of Jensen on the
weights $d_u / m$), $\sum_u ( d_u / m ) \log_2( n / d_u ) \le
\log_2( \sum_u ( d_u / m ) ( n / d_u ) ) = \log_2( n^2 / m )$, the sum
running over the $u$ with $d_u > 0$, which are at most $n$. $\square$

Coded apart, the lists can thus take fewer bits when the degrees are skewed, at
the price of a directory and an offset per list, the form that chapter 15
of Ferragina describes; the single sequence keeps one directory and the
same $O( 1 )$ access. Neither exploits the locality and the similarity of
the lists of the Web, which the codes of
[Boldi and Vigna 2004](Z-bibliography.md#ref-boldi2004) do. With the pages
sorted by URL, most links stay in the host (*locality*), and the pages of a
host link to the same pages (*similarity*). `BVGraph` codes the list of $x$ as its outdegree, in $\gamma$, followed by the *reference* $r$: the list of $x - r$, among the previous $W$ lists, that shares the most successors with that of $x$ (0 for none). Then comes the *copy list* of the reference, a bit per successor of $x - r$ that records whether it is a successor of $x$ too. It is written as *copy blocks*, i.e., its first bit and the lengths less 1 of its runs, the last one omitted since the outdegree of the reference implies it. Next come the successors not copied, the *extra nodes*: the runs of at
least $L_{\min}$ consecutive integers as intervals (each an `Interval`, its
left extreme and its length), and the *residuals* as
gaps less 1, the first ones of each relative to $x$ by `signedToNatural()`
([Algorithm 22.21](22-succinct.md#alg-22-21)).

<span id="alg-22-21"></span>**Algorithm 22.21.** The coding and the
decoding of a list of `BVGraph`, and two of its steps
([Boldi and Vigna 2004](Z-bibliography.md#ref-boldi2004)).

--8<-- "generated/BVGraph-encode.pseudo.md"

--8<-- "generated/BVGraph-successors.pseudo.md"

--8<-- "generated/BVGraph-copyBlocks.pseudo.md"

--8<-- "generated/BVGraph-intervals.pseudo.md"

A list is decoded from its place in the stream, kept for each node in an
Elias-Fano sequence, after the list of its reference. Since the chains of references are at most $R$ long, a list is decoded in $O( R d )$ time for outdegrees up to $d$. With the window $W = 7$, the
chains $R = 3$ and $L_{\min} = 4$ (the defaults of the WebGraph framework),
the Web graphs usually take about 3 bits per edge, as Boldi and Vigna measure,
against the $\log_2 n$ bits of a plain list and the
$2 + \log_2( n^2 / m )$ of Elias-Fano. Also, the WebGraph framework codes the gaps
by the $\zeta$ codes, tuned to their power-law distribution, and reorders
the nodes (e.g., by the layered label propagation of
[Boldi et al. 2011](Z-bibliography.md#ref-boldi2011)) to make the locality
stronger; neither is in chapter 15 of Ferragina, nor in the library.
`copyList()`, `copyBlocks()`, `intervals()` and `residualGaps()` are the
steps of the coding, given for the examples of that chapter.

The $k^2$-tree of [Brisaboa, Ladra and Navarro 2014](Z-bibliography.md#ref-brisaboa2014) (`K2Tree`) splits the adjacency matrix, padded to the side $N = k^h \ge n$, into $k^2$ submatrices of side $N / k$. Each one is a child of the root, marked 1 if it has an edge and 0 otherwise, and each child marked 1 is split again, down to the cells. We write the marks by levels: $T$ has those of all
the levels but the last, and $L$ those of the cells
([Figure 22.15](22-succinct.md#fig-22-15)).

<span id="lem-22-13"></span>**Lemma 22.13.** *The children of the node
marked at the place $p$ of $T$ start at the place
$k^2 \, \mathrm{rank}_1( T , p + 1 )$ of $T$ followed by $L$.*

*Proof.* Recall that the marks are written by levels, the $k^2$ children of each node
marked 1 in the order of the marks. Thus the children of the root take the
places $0 , \ldots , k^2 - 1$, and those of the $j$-th mark 1 of $T$ come
after the children of the root and of the $j - 1$ marks 1 before it, i.e.,
they start at $k^2 j$, with $j = \mathrm{rank}_1( T , p + 1 )$. $\square$

![Figure 22.15. The adjacency matrix of a graph of 8 nodes and 7 edges split by $k = 2$ (the grey lines), and its marks by levels $T$ and $L$, in groups of $k^2 = 4$; `hasEdge( 5 , 5 )` follows the places 3, 8 and 23 (violet).](../figures/succinct-k2tree.svg){#fig-22-15}

Hence no pointer is stored, as in LOUDS, and one can walk the tree by ranks only. `hasEdge( u , v )` follows one path, in $O( h )$ time. `neighbors( u )` walks down the children that cross
the row $u$, $k$ of the $k^2$ at each level, and `reverseNeighbors( v )`
those that cross the column $v$, and the same bits therefore give the
successors and the predecessors, which the lists give only by doubling
them ([Algorithm 22.22](22-succinct.md#alg-22-22)).

<span id="alg-22-22"></span>**Algorithm 22.22.** The construction of the
$k^2$-tree by levels from the edges, the successors of a node, and the test
of an edge ([Brisaboa, Ladra and Navarro 2014](Z-bibliography.md#ref-brisaboa2014)).

--8<-- "generated/K2Tree-build.pseudo.md"

--8<-- "generated/K2Tree-neighbors.pseudo.md"

--8<-- "generated/K2Tree-hasEdge.pseudo.md"

A row is read in $O( \sqrt{m} )$ time on average for $m$ edges spread
uniformly, and in $O( n )$ in the worst case. Each edge sets at most $k^2$ marks per level, and below the level where the submatrices outnumber the edges these marks are distinct. This gives the bound
$k^2 m ( \log_{k^2}( n^2 / m ) + O( 1 ) )$ in the worst case, as Brisaboa,
Ladra and Navarro prove, and possibly far fewer bits when the edges "cluster" and the
empty submatrices are large, as in the Web graphs ordered by URL. A larger
$k$ gives fewer levels and faster queries, a smaller one fewer bits; the
authors use a larger $k$ at the top levels and a smaller one below
([Brisaboa, Ladra and Navarro 2009](Z-bibliography.md#ref-brisaboa2009)).
We build the tree from the edges, by levels, in $O( m h )$ time, with no visit of the matrix, which the recursive construction of chapter 15 of Ferragina performs.

## 22.11 Parallel construction {#sec-22-11}

All the structures built by a scan of their input are also built on the threads
of an executor (`ADS/parallel/Executor.h`), with the same entries, bits and
inventories as the sequential construction; a query thus gives the same
answer whichever built the structure. In the directories of rank of `RankSelect` and `CompactRankSelect`, we cut the blocks into chunks of
consecutive blocks. A first pass counts the 1s of each chunk, and the prefix sums of the counts give the 1s before each chunk, $c_k$. A second pass writes the entries of each chunk from $c_k$ on; these are those of the
sequential scan, since the entry of a block depends only on the 1s before
it ([Algorithm 22.23](22-succinct.md#alg-22-23)). The counts of the top level are the $c_k$ of the chunks that start a top block. Indeed, the chunks are a power of 2 of blocks below the size of a top block, which then holds whole chunks, or a multiple of it above; a top block that does not start a chunk lies in one, which writes its count. We build the inventories of select in two passes over the chunks: the first finds the places of the first bit of each group, and the second the offsets of the sampled bits from the place of their group, each sample found from the entry of its block by a select in a word.
In all, the work is $O( m / 64 )$ words, as in the sequential scan, plus the
samples, and the depth is that of a chunk and of the prefix sums.

<span id="alg-22-23"></span>**Algorithm 22.23.** The parallel construction
of the directories of `RankSelect` and of `CompactRankSelect`.

--8<-- "generated/RankSelect-BasicRankSelect-directory-2.pseudo.md"

--8<-- "generated/CompactRankSelect-directory.pseudo.md"

We build the wavelet matrix faster, also on one thread, by a property of
its levels that lets one pass over the items write several levels.

<span id="lem-22-14"></span>**Lemma 22.14 (the levels by the reversed
prefixes).** *In the wavelet matrix of [Lemma 22.10](22-succinct.md#lem-22-10),
let $b_j( c )$ be the bit at the level $\ell + j$ of the item $c$, and*

$$
\kappa_j( c ) = \sum_{i < j} 2^i \, b_i( c ) , \qquad 0 \le j \le k ,
\tag{22.20}
$$

*the bits of the levels $\ell , \ldots , \ell + j - 1$ read from the last
one. Then the order of the items at the level $\ell + j$ is the stable sort
of their order at the level $\ell$ by $\kappa_j$.*

*Proof.* Clearly, the level $\ell + j$ is reached by $j$ stable partitions by the bits $b_0 , \ldots , b_{j-1}$, the last by $b_{j-1}$. This is the radix sort
of the least significant digit first by the key whose most significant bit
is $b_{j-1}$, i.e., $\kappa_j$. In fact, the same key orders the stable sort, and both
keep the order of the level $\ell$ among the items of equal key. $\square$

Hence the place of an item $c$ at the level $\ell + j$ is

$$
P_j[ \kappa_j( c ) ] + | \{ c' \text{ before } c \text{ at the level }
\ell : \kappa_j( c' ) = \kappa_j( c ) \} | , \tag{22.21}
$$

with $P_j[ a ]$ the items whose key is below $a$, and $Z_{\ell + j}$ is the
number of the items with $b_j = 0$; both are sums of the counts of the $2^k$
values of $\kappa_k$, from which each $\kappa_j = \kappa_k \bmod 2^j$
follows. One scan of the items in the order of the level $\ell$ therefore writes the bits of the $k$ levels $\ell , \ldots , \ell + k - 1$ at the places (22.21), and moves the items to their order at the level $\ell + k$. By contrast, the construction by the partitions of [Lemma 22.10](22-succinct.md#lem-22-10)
moves them $k$ times, once per level. With $k \le 8$ this is $\lceil w / 8 \rceil$ passes over the items instead of $w$, the bits being written into $2^k - 1$ streams, one per value of $\kappa_j$ for each $j < k$. We recognize here the sort by the prefixes of the bits of
[Dinklage, Ellert, Fischer, Kurpicz and Löbel 2021](Z-bibliography.md#ref-dinklage2021),
applied to $k$ levels at a time ([Algorithm 22.24](22-succinct.md#alg-22-24)).

On several threads the items are cut into slices of at most $2^{18}$
items, about four per thread, whose $k$ levels, of 256 KB at most, stay in
the second level of the cache while the slice writes them. Each slice writes the levels of its items at the places that the counts of its keys give by (22.21) within the slice. The levels are then gathered from the slices: each block of words of a level copies the runs of bits of the slices that fall in it, in the order (22.21) of the runs, and therefore no two threads write a word. This is the domain decomposition of
[Fuentes-Sepúlveda et al. 2017](Z-bibliography.md#ref-fuentes2017) and of
[Labeit, Shun and Blelloch 2017](Z-bibliography.md#ref-labeit2017), with
the merge of Dinklage et al.; the items move to the places (22.21) of the
level $\ell + k$ directly, since those of two slices never meet.

<span id="alg-22-24"></span>**Algorithm 22.24.** The construction of the
wavelet matrix by the stable sorts by the prefixes of the bits, and the
parallel construction of the wavelet tree of Huffman.

--8<-- "generated/WaveletMatrix-build.pseudo.md"

--8<-- "generated/WaveletMatrix-build-2.pseudo.md"

--8<-- "generated/HuffmanWaveletTree-build-2.pseudo.md"

We build the wavelet tree of Huffman in slices too. Here the counts of the symbols of each slice give the place of each slice in each node of the trie
of the codes, and each slice writes the bits of its items along their codes
into a stream per node, with no item moved. A stream gathers its bits into a word and stores the word when it is full; the store is plain when the word is inside its range, and an atomic or when the word is at one of its ends, which it may share with the stream of another slice (the words are 0 at first, and only the 1s are written; cf. the class `BitRuns` of `BitVector.h`). On one
thread the tree is built by the sequential scan, which writes each bit at
its place. In all, the work is $O( n w )$ bits, $O( n w / 64 )$ words copied and
$O( n \lceil w / 8 \rceil )$ moves of items for the matrix, or
$O( \sum_c f_c \ell_c )$ bits for the tree, plus $O( q 2^k )$ counts for
$q$ slices ($O( q \sigma )$ for the tree, whose slices therefore hold at
least $\sigma$ items each), and the depth is that of a slice. The other parallel constructions of the literature ([Shun 2015](Z-bibliography.md#ref-shun2015wt); [Dinklage, Fischer, Kurpicz and Tarnowski 2023](Z-bibliography.md#ref-dinklage2023)) trade the passes against the memory in other ways, and we leave the comparison to Appendix B.

## 22.12 Discussion and references {#sec-22-12}

The rank and the select in $o( m )$ bits are those of
[Jacobson 1989](Z-bibliography.md#ref-jacobson1989) and
[Clark 1996](Z-bibliography.md#ref-clark1996), and the compressed vector
that of [Raman, Raman and Rao 2007](Z-bibliography.md#ref-raman2007); the
book of [Navarro 2016](Z-bibliography.md#ref-navarro2016) treats them, with
the trees, the sequences and the graphs, in the form that the practice
uses. Our directories follow the engineering of
[Vigna 2008](Z-bibliography.md#ref-vigna2008),
[Gog and Petri 2014](Z-bibliography.md#ref-gog2014a),
[Zhou, Andersen and Kaminsky 2013](Z-bibliography.md#ref-zhou2013),
[Kurpicz 2022](Z-bibliography.md#ref-kurpicz2022) and
[Laws et al. 2024](Z-bibliography.md#ref-laws2024), and the representation
of [Elias 1974](Z-bibliography.md#ref-elias1974) and Fano is that of
[Vigna 2013](Z-bibliography.md#ref-vigna2013), with its partitioned form by
[Ottaviano and Venturini 2014](Z-bibliography.md#ref-ottaviano2014) and the
learned one by [Boffa, Ferragina and Vinciguerra 2021](Z-bibliography.md#ref-boffa2021).
As for the trees, those by parentheses are due to
[Munro and Raman 2001](Z-bibliography.md#ref-munro2001) on the range
min-max tree of [Navarro and Sadakane 2014](Z-bibliography.md#ref-navarro2014a),
and the wavelet structures those of
[Grossi, Gupta and Vitter 2003](Z-bibliography.md#ref-grossi2003),
[Mäkinen and Navarro 2005](Z-bibliography.md#ref-makinen2005) and
[Claude, Navarro and Ordóñez 2015](Z-bibliography.md#ref-claude2015). Finally, the graphs are those of [Boldi and Vigna 2004](Z-bibliography.md#ref-boldi2004)
and [Brisaboa, Ladra and Navarro 2014](Z-bibliography.md#ref-brisaboa2014),
and the move structure that of
[Nishimoto and Tabei 2021](Z-bibliography.md#ref-nishimoto2021). Chapter 15 of [Ferragina 2023](Z-bibliography.md#ref-ferragina2023) is the source of the structures of the book. Our chapter departs from it where the code departs, as remarked in place: the places from 0, the 1s and the 0s of $H$ as in its
chapter 11, the construction of the $k^2$-tree from the edges, and the single
sequence of the edges of `EliasFanoGraph`.

The structures share the design of the practice, not that of the analysis. Indeed, the tables of the small blocks give way to the sideways sum and to the broadword select of a word, the levels of the directories have constant sizes chosen for the lines of the cache, and the space is a constant fraction of $m$ instead of $o( m )$. How much each choice
costs or gains, against the libraries of reference (e.g., the SDSL of
[Gog, Beller, Moffat and Petri 2014](Z-bibliography.md#ref-gog2014b)), is
measured by the campaign of Appendix B, and no number is given here.

## 22.13 Exercises {#sec-22-13}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Realize a vector of bits packed in words, with the bits past the size
   kept at 0, the append and the read of a field of up to 64 bits. Test:
   `pearls_succinct.bit-vector`.
2. Find the place of the $k$-th 1 of a word by the broadword method of
   [Lemma 22.2](22-succinct.md#lem-22-2). Test:
   `pearls_succinct.select-in-word`.
3. Realize the streams of bits with the binary fields read from the most
   significant bit and the unary codes read a word at a time. Test:
   `pearls_succinct.bit-stream`.
4. Realize the directory of rank with the blocks of 256 bits and the
   inventories of select, eager and lazy, also from words in memory and
   above $2^{40}$ bits. Tests: `pearls_succinct.rank-select`,
   `engineered_succinct.rank-select-lazy`,
   `engineered_succinct.rank-select-words`,
   `succinct_top.rank-select-top`, `succinct_top.rank-select-words-top`.
5. Realize the flat directory of [§22.4](22-succinct.md#sec-22-4) and the
   one with the counts inside the lines. Tests:
   `engineered_succinct.compact-rank-select`,
   `engineered_succinct.interleaved-rank-select`.
6. Realize the compressed vector of
   [Theorem 22.1](22-succinct.md#thm-22-1), with the offsets of the
   combinatorial number system. Test:
   `engineered_succinct.rrr-bit-vector`.
7. Realize Elias-Fano with `access()` and `nextGEQ()`
   ([Lemma 22.5](22-succinct.md#lem-22-5)), its partitioned form, and the
   LA-vector. Tests: `pearls_succinct.elias-fano`,
   `engineered_succinct.partitioned-elias-fano`,
   `engineered_succinct.la-vector`.
8. Realize the arrays of fields of $B$ bits, saturated or not, and the
   arrays of trits five to a byte, built on several threads. Tests:
   `engineered_search.packed-array`, `engineered_search.trit-array`.
9. Realize the binary tree of Jacobson, the tree of LOUDS
   ([Lemma 22.9](22-succinct.md#lem-22-9)), the balanced parentheses with
   the range min-max tree and the tree on them. Tests:
   `pearls_succinct.jacobson-tree`, `pearls_succinct.louds-tree`,
   `pearls_succinct.balanced-parentheses`, `pearls_succinct.bp-tree`.
10. Realize the wavelet matrix with the quantiles and the counts of a
    range, and the wavelet tree of Huffman. Tests:
    `engineered_succinct.wavelet-matrix`,
    `engineered_succinct.huffman-wavelet-tree`.
11. Realize the move structure with the balance of Nishimoto and Tabei.
    Test: `engineered_succinct.move-structure`.
12. Realize the graph in Elias-Fano, the codes of Boldi and Vigna on the
    lists of [Figure 22.14](22-succinct.md#fig-22-14), and the
    $k^2$-tree. Tests: `pearls_succinct.elias-fano-graph`,
    `pearls_succinct.bv-graph`, `pearls_succinct.k2-tree`.
13. Build the directories of rank and select, the wavelet matrix and the
    wavelet tree of Huffman on several threads, with the same bits as the
    sequential construction. Tests:
    `engineered_parallel.parallel-rank-select`,
    `succinct_top.parallel-rank-select-top`,
    `engineered_parallel.parallel-wavelet-matrix`,
    `engineered_parallel.parallel-huffman-wavelet-tree`.
