# 21. Compression {#ch-21}

[Source: `include/ADS/compression/Entropy.h`, `CanonicalHuffman.h`,
`HuffmanCoder.h`, `PackedBits.h`, `ArithmeticCoding.h`, `Rans.h`,
`Tans.h`, `PPM.h`, `IntegerCodes.h`, `BinaryPacking.h`, `StreamVByte.h`,
`DirectAccessCodes.h`, `LempelZiv.h`, `LZ77Linear.h`, `LZBicriteria.h`,
`BurrowsWheeler.h`, `FMIndex.h`, `HuffmanFMIndex.h`, `RIndex.h`,
`MoveRIndex.h`]

We are concerned in this chapter with the lossless compression of a text, i.e.,
with an encoder that turns a string of symbols into a shorter string of bits
and a decoder that gives the string back, and with the indexes that search a
text kept in compressed form. The headers of `ADS/compression` follow chapters
11 to 14 of [Ferragina 2023](Z-bibliography.md#ref-ferragina2023) (Integer
Coding, Statistical Coding, Dictionary-Based Compressors and Block-Sorting
Compression), which the book of Montresor meets only in the code of Huffman of
its section 14.4. Beside each algorithm of the book they give the engineered
form that the fast compressors of today use. We first fix the measure against
which a compressor is judged, the empirical entropy
([§21.1](21-compression.md#sec-21-1)). Then come the statistical codes, which
give each symbol a number of bits that depends on its probability: the code of
Huffman, as the book presents it and as the coders by tables decode it
([§21.2](21-compression.md#sec-21-2) and [§21.3](21-compression.md#sec-21-3)),
the arithmetic code ([§21.4](21-compression.md#sec-21-4)), the asymmetric
numeral systems ([§21.5](21-compression.md#sec-21-5)) and the prediction by
partial matching ([§21.6](21-compression.md#sec-21-6)). We treat the codes of
the integers, which assume a distribution instead of measuring one, in
[§21.7](21-compression.md#sec-21-7), and their engineered forms in
[§21.8](21-compression.md#sec-21-8). Two families of compressors need no model
at all: those of Lempel and Ziv replace a string with a reference to an earlier
occurrence of it ([§21.9](21-compression.md#sec-21-9)), and the transform of
Burrows and Wheeler sorts the contexts of the symbols, after which the symbols
are easy to code ([§21.10](21-compression.md#sec-21-10)). The transform is also
an index of the text, which [§21.11](21-compression.md#sec-21-11) builds in
four forms of decreasing space. In the sets of
[§1.9](01-introduction.md#sec-1-9), `ArithmeticCoding.h`, `BurrowsWheeler.h`,
`CanonicalHuffman.h`, `Entropy.h`, `FMIndex.h`, `IntegerCodes.h`, `LempelZiv.h`
and `PPM.h` are base and the other headers of this chapter are advanced, while
`PackedBits.h` is in the core that the two sets include.

## 21.1 Entropy {#sec-21-1}

A text $s = s[ 0 ] s[ 1 ] \cdots s[ n - 1 ]$ is a string of $n$ symbols of an
ordered alphabet $\Sigma$ of $\sigma$ symbols (usually the bytes); we write $s[
i .. j ]$ for the substring from $i$ to $j$, $s[ i .. ]$ for the suffix from
$i$, and $n_c$ for the occurrences of the symbol $c$ in $s$. We call *code* a
map of each symbol $c$ to a binary *codeword* of length $\ell( c )$, and of a
text to the concatenation of the codewords of its symbols; we say that it is
*prefix-free* when no codeword is a prefix of another one; then the
concatenation is decoded from left to right with no separator, since a codeword
is recognized when its last bit is read. [Table
21.1](21-compression.md#tab-21-1) collects the notation that the headers of the
area share.

<span id="tab-21-1"></span>**Table 21.1.** The notation of the chapter.

| Symbol | Meaning |
|---|---|
| $n_c$, $P[ c ]$ | the occurrences of $c$ in $s$, and its probability ($n_c / n$ for the empirical one) |
| $H_0$, $H_k$ | the entropy of order 0 and of order $k$, in bits per symbol, (21.1) and (21.2) |
| $\ell( c )$, $\bar\ell$ | the length of the codeword of $c$, and the average $\sum_c P[ c ] \ell( c )$ |
| $B( x )$ | the binary code of the integer $x \ge 1$, of $1 + \lfloor \log_2 x \rfloor$ bits |
| $\mathit{SA}$ | the suffix array of $s \$$, with $\$$ a sentinel smaller than all the symbols |
| $F$, $L$ | the first and the last column of the sorted rotations of $s \$$ |
| $C[ c ]$ | the symbols of $s \$$ smaller than $c$ |
| $\mathrm{rank}_c( L , i )$ | the occurrences of $c$ in $L[ 0 .. i - 1 ]$ |
| $r$, $z$ | the runs of equal symbols of $L$, and the phrases of a parse of Lempel and Ziv |
| $p$, $\mathit{occ}$ | the length of a pattern $P$, and the number of its occurrences in $s$ |

The *entropy of order 0* of a distribution $P$ on $\Sigma$, and the
*empirical* entropy of $s$, which takes $P[ c ] = n_c / n$, are

$$
H_0 = \sum_{c \in \Sigma} P[ c ] \log_2 \frac{1}{P[ c ]} , \tag{21.1}
$$

where the term of a symbol of probability 0 is 0 ([Shannon
1948](Z-bibliography.md#ref-shannon1948)). One has $0 \le H_0 \le \log_2
\sigma$ (cf. (21.1)), with 0 when one symbol has all the probability and
$\log_2 \sigma$ when the symbols are equally likely. As [Lemma
21.1](21-compression.md#lem-21-1) shows, no code of the symbols one by one
takes fewer than $n H_0$ bits on average. A code that looks at the context of a
symbol may do much better on the texts of a language, where a symbol is often
(but not always) all but fixed by the few symbols before it. With $w_s$ the
string of the symbols that follow the occurrences of $w$ in $s$, in their
order, the *entropy of order $k$* of $s$ is

$$
H_k( s ) = \frac{1}{n} \sum_{w \in \Sigma^k} | w_s | \, H_0( w_s ) ,
\tag{21.2}
$$

the average entropy of the symbols in their contexts of $k$ symbols
([Manzini 2001](Z-bibliography.md#ref-manzini2001)). One has
$H_{k+1} \le H_k \le H_0$, since conditioning on a longer context does not
increase the entropy, and $n H_k$ is the measure of the compressors of
order $k$. In `Entropy.h`, three functions `entropy()` compute (21.1)
on a map of frequencies and on a string, and (21.2) on a string
([Algorithm 21.1](21-compression.md#alg-21-1)). They take $O( n \log n )$
time, with the counts in ordered maps, and their contract checks that
$H_k \le H_0$ on the result, up to the rounding.

<span id="alg-21-1"></span>**Algorithm 21.1.** The empirical entropies of
order 0 and of order $k$.

--8<-- "generated/Entropy-entropy.pseudo.md"

--8<-- "generated/Entropy-entropy-2.pseudo.md"

--8<-- "generated/Entropy-entropy-3.pseudo.md"

We remark that (21.2) divides by $n$ while only the $n - k$ symbols with a full
context of $k$ symbols are counted. Indeed, the first $k$ symbols are left out
of the sum, as in the definition of Manzini, and their contribution vanishes as
$n$ grows.

## 21.2 Prefix-free codes and the code of Huffman {#sec-21-2}

We first show that the lengths of a prefix-free code cannot be all "small", and
the bound on them is also a bound on the average length of the code.

<span id="lem-21-1"></span>**Lemma 21.1 (Kraft and Shannon).** *The lengths
of a prefix-free code satisfy*

$$
\sum_{c \in \Sigma} 2^{-\ell( c )} \le 1 , \tag{21.3}
$$

*any lengths that satisfy (21.3) are those of a prefix-free code, and the
average length of a prefix-free code is at least $H_0$.*

*Proof.* A codeword of length $\ell( c )$ is a node at the depth
$\ell( c )$ of the complete binary tree of depth
$\ell_{\max} = \max_c \ell( c )$. Since no codeword is an ancestor of
another one, the $2^{\ell_{\max} - \ell( c )}$ leaves below the codewords
are disjoint, which is (21.3) multiplied by $2^{\ell_{\max}}$. Conversely,
the canonical code below ([Lemma 21.2](21-compression.md#lem-21-2)) gives
prefix-free codewords of any lengths that satisfy (21.3). Finally, by the
concavity of the logarithm (the inequality of Jensen) and by (21.3),

$$
H_0 - \bar\ell = \sum_c P[ c ] \log_2 \frac{2^{-\ell( c )}}{P[ c ]} \le
\log_2 \sum_c 2^{-\ell( c )} \le 0 . \qquad \square
$$

Hence one can see that the lengths $\ell( c ) = \log_2( 1 / P[ c ] )$ would
reach $H_0$ if they were integers, and the question is how close to $H_0$ the
integer lengths can come. This is answered by the code of [Huffman
1952](Z-bibliography.md#ref-huffman1952). It merges the two least probable
symbols into one of their total probability, $\sigma - 1$ times, and the
codewords of the two are those of the merged symbol followed by 0 and by 1. In
ADS we make the merges by `huffman()` of `ADS/string/Huffman.h`, with a heap,
in $O( \sigma \log \sigma )$ time, and `huffmanLengths()` of
`CanonicalHuffman.h` reads the lengths off its codewords.

<span id="thm-21-1"></span>**Theorem 21.1 (Huffman).** *The code of Huffman
has the least average length among the prefix-free codes of the symbols,
and*

$$
H_0 \le \bar\ell_H < H_0 + 1 . \tag{21.4}
$$

*Proof.* We show by induction on $\sigma$ that the merges give an optimal code;
for $\sigma \le 2$ the codewords have at most 1 bit. Let $x$ and $y$ be the two
least probable symbols. The tree of an optimal code is full (a node with one
child could be replaced by the child, shortening the codewords below it), and
hence its deepest level holds two sibling leaves; exchanging their symbols with
$x$ and $y$ does not increase the average, since $x$ and $y$ are the least
probable, and therefore some optimal code $T$ has $x$ and $y$ as sibling
leaves. Replacing the two leaves with a leaf of a new symbol $z$ of probability
$P[ x ] + P[ y ]$ gives a code $T'$ of $\sigma - 1$ symbols with $\bar\ell( T )
= \bar\ell( T' ) + P[ x ] + P[ y ]$. The code $H$ of the merges is the code
$H'$ of the merges on the $\sigma - 1$ symbols with $z$ in place of $x$ and
$y$, whose leaf is split into the two, and $\bar\ell( H ) = \bar\ell( H' ) + P[
x ] + P[ y ]$ in the same way. By induction $\bar\ell( H' ) \le \bar\ell( T'
)$, and hence $\bar\ell( H ) \le \bar\ell( T )$. As for (21.4), its lower bound
is [Lemma 21.1](21-compression.md#lem-21-1), and its upper bound holds since
the lengths $\lceil \log_2( 1 / P[ c ] ) \rceil$ satisfy (21.3) and have an
average below $H_0 + 1$, and the optimum is no longer than they are. $\square$

We remark that only the lengths of a code matter for its size. We call
*canonical* a code that gives the symbols of each length consecutive codewords,
in the order of the symbols, and therefore neither the encoder nor the decoder
needs the tree ([Schwartz and Kallick
1964](Z-bibliography.md#ref-schwartz1964)). With $\mathit{num}[ l ]$ the
symbols of length $l$ and $\ell_{\max}$ the longest length, the first codeword
of each length is

$$
\mathit{fc}[ \ell_{\max} ] = 0 , \qquad
\mathit{fc}[ l ] = \left\lceil \frac{\mathit{fc}[ l + 1 ] +
\mathit{num}[ l + 1 ]}{2} \right\rceil \quad ( l < \ell_{\max} ) ,
\tag{21.5}
$$

i.e., the parent of the first node of the level $l + 1$ that is not used. Then
the symbols of length $l$ get the codewords $\mathit{fc}[ l ]$, $\mathit{fc}[ l
] + 1$, and so on, on $l$ bits ([Algorithm 21.2](21-compression.md#alg-21-2)),
and hence the tree is skewed to the left, the longest codewords being all zeros
at the start ([Figure 21.1](21-compression.md#fig-21-1)). Chapter 12 of
[Ferragina 2023](Z-bibliography.md#ref-ferragina2023) divides in (21.5) without
rounding up. This is the same on the "complete" codes that the merges of
Huffman give (the sum is even there), while the ceiling also keeps the decoder
right on a code that is not complete, e.g., one cut by a limit on the lengths.

![Figure 21.1. The canonical code of the lengths a 4, b 4, c 3, d 2, e 2, f 3, g 3 (a code of Huffman of the probabilities 0.05, 0.05, 0.1, 0.2, 0.3, 0.2, 0.1, whose ties `huffmanLengths()` breaks otherwise, into a 5, b 5, c 3, d 2, e 2, f 2, g 4, of the same average 2.6), with the codewords under the leaves, and the arrays $\mathit{fc}$ and $\mathit{symb}$ that replace the tree; the codeword 11 of e (violet) is decoded at $l = 2$, the first length at which the value of the bits read, 3, reaches $\mathit{fc}[ l ] = 2$.](../figures/compression-huffman.svg){#fig-21-1}

<span id="alg-21-2"></span>**Algorithm 21.2.** The canonical code of given
lengths, and the decoding of a codeword ([Ferragina 2023, chapter 12,
Statistical Coding](Z-bibliography.md#ref-ferragina2023)).

--8<-- "generated/CanonicalHuffman-CanonicalHuffman.pseudo.md"

--8<-- "generated/CanonicalHuffman-decode.pseudo.md"

<span id="lem-21-2"></span>**Lemma 21.2.** *If the lengths satisfy (21.3),
the codewords of (21.5) fit in their lengths, the code is prefix-free, and
the decoding of [Algorithm 21.2](21-compression.md#alg-21-2) returns the
symbol whose codeword is next in the input.*

*Proof.* Let $a_l = \mathit{fc}[ l ] + \mathit{num}[ l ]$, the value past
the last codeword of length $l$, and $K_l = \sum_{l' \ge l}
\mathit{num}[ l' ] \, 2^{-l'}$. By induction from $l = \ell_{\max}$ down,
$a_l \le \lceil 2^l K_l \rceil$: it holds with equality at $\ell_{\max}$,
and $\mathit{fc}[ l ] = \lceil a_{l+1} / 2 \rceil \le
\lceil 2^l K_{l+1} \rceil$ gives $a_l \le \lceil 2^l K_{l+1} \rceil +
\mathit{num}[ l ] = \lceil 2^l K_l \rceil$. Since $K_l \le 1$ by (21.3),
$a_l \le 2^l$, i.e., the codewords of length $l$ fit in $l$ bits. Next, the
first $l$ bits of a codeword $v$ of length $l + 1$ are
$\lfloor v / 2 \rfloor \le \lfloor ( a_{l+1} - 1 ) / 2 \rfloor <
\lceil a_{l+1} / 2 \rceil = \mathit{fc}[ l ]$. By induction on the length,
the first $l$ bits of a longer codeword are below $\mathit{fc}[ l ]$ too,
since its first $l + 1$ bits are below
$\mathit{fc}[ l + 1 ] \le 2 \, \mathit{fc}[ l ]$. Hence no codeword of
length $l$, which is at least $\mathit{fc}[ l ]$, is a prefix of a longer
one. Finally, the decoder reads one bit at a time and stops at the first $l$ at
which the value $v$ of the bits read reaches $\mathit{fc}[ l ]$, i.e., at
the end of the codeword, whose symbol is the
$( v - \mathit{fc}[ l ] )$-th of length $l$. $\square$

Hence the preamble of a canonical code is the lengths alone (cf. (21.5)). Also,
we decode a codeword of $l$ bits by $l$ steps on two arrays that fit in the
cache, instead of $l$ steps along the pointers of a tree, each a possible miss.
The implementation follows the pseudocode, with $\mathit{fc}[ 1 .. \ell_{\max}
]$ and the symbols by length in vectors.

Encoding a symbol is a lookup of its codeword and its length in $O( 1 )$ time
([Algorithm 21.3](21-compression.md#alg-21-3)), and the construction takes $O(
\sigma \log \sigma )$ time. The class (whose symbols are of the type
`symbol_type`) is built from the lengths or, by `fromFrequencies()`, from the
frequencies through `huffmanLengths()`; `maxLength()` gives the longest length
and `firstCode( l )` the value $\mathit{fc}[ l ]$ of the first codeword of
length $l$. We write and read the bits by `PackedBitWriter` and
`PackedBitReader` of `PackedBits.h`, which pack them in bytes with the first
bit in the most significant bit of the first byte, and whose `position()`
counts the bits read (past the end too). Hence the value of the bits read so
far is the integer whose binary digits they are, which is the order in which
the canonical decoder and the arithmetic decoder of
[§21.4](21-compression.md#sec-21-4) read their input; a reader also gives zeros
past the end of its bytes, as the arithmetic decoder expects. (The two classes
only move bits, and we therefore give them no pseudocode.)

<span id="alg-21-3"></span>**Algorithm 21.3.** The encoding of a text by
the canonical code, and the lengths of a code of Huffman of no length above
$L$ by the iterative scaling of the frequencies.

--8<-- "generated/CanonicalHuffman-encode.pseudo.md"

--8<-- "generated/CanonicalHuffman-limitedLengths.pseudo.md"

The codewords of Huffman can be rather long. Frequencies that grow as the
numbers of Fibonacci give a codeword of 33 bits after about $10^7$ symbols
([Ferragina 2023, chapter 12](Z-bibliography.md#ref-ferragina2023)), while a
"fast" decoder typically wants the codewords to fit in a word, or in the index
of a table. The function `limitedLengths()` bounds them to a given $L$ by
halving the frequencies, rounding up, until the code of Huffman of the halved
ones has no length above $L$.

<span id="lem-21-3"></span>**Lemma 21.3.** *If $2^L \ge \sigma$, the loop
of `limitedLengths()` ends after at most
$\lceil \log_2 f_{\max} \rceil + 1$ codes of Huffman, $f_{\max}$ being the
largest frequency.*

*Proof.* Halving with rounding up maps 1 to 1 and any frequency
$f \ge 2$ to $\lceil f / 2 \rceil < f$, and after
$\lceil \log_2 f_{\max} \rceil$ halvings all the frequencies are 1,
since $f \le 2^k$ gives $\lceil f / 2 \rceil \le 2^{k-1}$. On $\sigma \ge
2$ equal frequencies, the tree of the code of Huffman is full and optimal
(Theorem 21.1), and its leaves lie on two consecutive levels: if a leaf
were at a depth $d \le D - 2$, with $D$ the largest depth, moving two
sibling leaves of depth $D$ below it would turn their parent into a leaf
and change the sum of the depths by $-2 D + ( D - 1 ) - d + 2 ( d + 1 ) =
d + 1 - D < 0$, against the optimality. With the leaves on the levels $D -
1$ and $D$, each of the $2^{D-1}$ nodes of the level $D - 1$ is a leaf or
has two children, and at least one has children; hence $\sigma \ge
2^{D-1} + 1$, i.e., $D \le \lceil \log_2 \sigma \rceil \le L$, and the
loop ends there at the latest. $\square$

Halving is simple but (possibly) not optimal: when it comes to the end it gives
a code of fixed length, whereas the package-merge of [Larmore and Hirschberg
1990](Z-bibliography.md#ref-larmore1990) finds the optimal code of lengths at
most $L$ in $O( \sigma L )$ time (see [§21.3](21-compression.md#sec-21-3)).

## 21.3 The code of Huffman with tables {#sec-21-3}

The functions `huffmanCompress()` and `huffmanDecompress()` of `HuffmanCoder.h`
code a string of bytes as the fast coders do, with the code of Huffman of the
bytes and four choices that the canonical code of
[§21.2](21-compression.md#sec-21-2) does not make. First, the lengths are the
optimal ones of at most 11 bits, found by the package-merge of [Larmore and
Hirschberg 1990](Z-bibliography.md#ref-larmore1990) ([Algorithm
21.4](21-compression.md#alg-21-4)), and the decoding table therefore stays in
the cache of the first level. This limit usually costs a fraction of a percent
on a text of bytes, since the optimum under it is found exactly. Second, the
code is canonical, and its preamble is the lengths of the 256 bytes in 4 bits
each. Third, the codewords are written from the least significant bit of a word
of 64 bits, each turned around to keep the code prefix-free in the order of
reading, and four of them are joined by shifts and stored at once. Fourth, the
input is cut in four quarters, each coded into a separate stream. Since the
four streams are decoded together, the four chains of dependences of the
decoder (a lookup gives the length of a codeword, which gives the place of the
next lookup) overlap in the pipeline of the processor; this is the interleaving
of Zstandard ([Collet and Kucherawy 2021](Z-bibliography.md#ref-collet2021)).

<span id="alg-21-4"></span>**Algorithm 21.4.** The optimal lengths of at
most $L$ bits by package-merge
([Larmore and Hirschberg 1990](Z-bibliography.md#ref-larmore1990)), and the
compression of a string of bytes.

--8<-- "generated/HuffmanCoder-limitedHuffmanLengths.pseudo.md"

--8<-- "generated/HuffmanCoder-huffmanCompress.pseudo.md"

Our decoder peeks at the next $W$ bits of a stream, and hopefully finds more
than one codeword there. In a table of $2^W$ entries of 32 bits it finds the
symbols whose codewords fit whole in them, up to 3 in the low 24 bits of the
entry, followed by the number of their bits in 4 bits and by the number of the
symbols in 2 ([Figure 21.2](21-compression.md#fig-21-2)). A lookup is two
shifts and a mask, and therefore it has no branch, and it may decode about
$\min( 3 , \lfloor W / \bar\ell \rfloor )$ symbols for an average length
$\bar\ell$; these are the decoders by tables of [Moffat and Turpin
1997](Z-bibliography.md#ref-moffat1997). Its window is $W = 12$ bits from
$2^{18}$ bytes on, a table of 16 KB that gives more symbols a lookup, and $W =
11$ below, where half the table is somewhat quicker to build. Since a codeword
has at most 11 bits, it fits whole in any window, and each lookup on a valid
stream decodes at least one symbol.

![Figure 21.2. The four quarters of the input coded into four streams, and the entry of the decoding table for a window of $W = 6$ bits (12 or 11 in the library) whose bits, in the order of reading, hold the codewords 10 of d and 11 of e of [Figure 21.1](21-compression.md#fig-21-1) and the first two bits of a codeword of 3 bits: the entry gives d and e, the 4 bits that they take, and the count 2.](../figures/compression-huffmancoder.svg){#fig-21-2}

<span id="alg-21-5"></span>**Algorithm 21.5.** The decompression by the
tables of up to 3 symbols, on four streams.

--8<-- "generated/HuffmanCoder-huffmanDecompress.pseudo.md"

Four lookups of at most 12 bits fit in the bits of a word after a refill,
which leaves at least 57 bits to read, and the loop of the decoder
therefore loads each word once each four lookups. Note that each lookup
stores the 4 bytes of its entry, of which the next lookup overwrites those
past its symbols.

This loop runs while each stream has room for 12 more symbols, and the rest is
decoded one symbol at a time with the table of the single symbols. A string of
one distinct byte is written as that byte and its count, since the code of
Huffman of one symbol has no bit. Also, the loads and the stores of
little-endian words at any address, and the histogram of the bytes on eight
tables of counts (one for each byte of a word, so that a run of equal bytes
does not wait for the store of the count before), are in the internal header
`impl/Unaligned.h`, shared with the coders of
[§21.5](21-compression.md#sec-21-5); we do not describe it further. In size, we
have the code of Huffman of the bytes, within one bit per byte of $H_0$ by
(21.4). Its speed comes from the table and from the streams, as [Table
21.2](21-compression.md#tab-21-2) summarizes for the statistical coders of the
chapter.

<span id="tab-21-2"></span>**Table 21.2.** The statistical coders of the
area on a text of $n$ symbols, and their costs.

| Coder | Bits | Time per symbol | Model |
|---|---|---|---|
| `CanonicalHuffman` | $n \bar\ell_H < n ( H_0 + 1 )$ | $O( \ell( c ) )$ to decode, a bit at a time | static, any alphabet, the lengths as preamble |
| `huffmanCompress()` | as above, lengths at most 11 | up to 3 symbols per lookup to decode | static, bytes, four streams |
| `arithmeticEncode()` | $\le 2 + n H_0$ (exact), about $10^{-4} n$ more on 32 bits | $O( \log \sigma )$ adaptive (a tree of Fenwick) | static or adaptive |
| `ransCompress()`, `tansCompress()` | $n H_0$ plus a fraction of a percent | $O( 1 )$, a multiplication or a lookup | static, bytes |
| `ppmEncode()` | approaches $n H_K$ plus the escapes | $O( K ( K + \sigma_K ) \log n )$ | contexts of up to $K$ symbols |

## 21.4 The arithmetic code {#sec-21-4}

As we have seen, a code of the symbols one by one may lose up to one bit per
symbol against $H_0$ by (21.4), which is "much" on a skewed distribution: a
symbol of probability 0.99 carries about 0.015 bits of information, and it
costs 1 bit anyway. Instead, the *arithmetic code* codes the text as one
number. With the cumulative probabilities $f_c = \sum_{c' < c} P[ c' ]$, the
symbols narrow an interval $[ l , l + s )$ of $[ 0 , 1 )$ in turn, each to the
part of its probability,

$$
s_i = s_{i-1} P[ s[ i ] ] , \qquad l_i = l_{i-1} + s_{i-1} f_{s[ i ]} ,
\qquad l_0 = 0 , \ s_0 = 1 , \tag{21.6}
$$

and the code is a short binary fraction inside the last interval
([Figure 21.3](21-compression.md#fig-21-3)).

<span id="thm-21-2"></span>**Theorem 21.2 (the arithmetic code).** *With
$s_n = \prod_i P[ s[ i ] ]$ the size of the last interval
$[ l_n , l_n + s_n )$, the first $d = \lceil \log_2( 2 / s_n ) \rceil$
bits of $l_n + s_n / 2$ are a prefix-free code of $s$ among the texts of
$n$ symbols, and with the empirical probabilities*

$$
d \le 2 + \sum_i \log_2 \frac{1}{P[ s[ i ] ]} = 2 + n H_0 . \tag{21.7}
$$

*Proof.* Truncating $x = l_n + s_n / 2$ to $d$ bits loses less than
$2^{-d} \le s_n / 2$. Hence the fraction $x'$ written, and each fraction
that begins with its bits, lie in $[ x' , x' + 2^{-d} ) \subseteq
[ l_n , l_n + s_n )$. Moreover, the intervals of two texts of $n$ symbols
are disjoint, since (21.6) splits each interval into disjoint parts, and
therefore no code is a prefix of another one. Finally, the sum in (21.7) is
$\sum_c n_c \log_2( n / n_c ) = n H_0$, and
$d < \log_2( 2 / s_n ) + 1 = 2 + \log_2( 1 / s_n )$. $\square$

Hence the arithmetic code is within 2 bits of $n H_0$ for the entire text,
against the up to $n$ bits more of (21.4). Furthermore, one can code equally
well probabilities that change at each symbol, as long as the decoder changes
them in the same way. The functions `arithmeticInterval()`,
`arithmeticCodeExact()` and `arithmeticDecodeExact()` compute (21.6) and the
code in exact rationals (`ADS/numeric/Rational.h`), in a time that may grow
quickly with the length of the numbers ([Algorithm
21.6](21-compression.md#alg-21-6)). We use them as the reference for the
integer coder below, and they give the numbers of the example of chapter 12 of
[Ferragina 2023](Z-bibliography.md#ref-ferragina2023).

![Figure 21.3. The interval of abac narrowed symbol by symbol, with $P[ a ] = 1 / 2$ and $P[ b ] = P[ c ] = 1 / 4$: each row is the interval of the prefix read so far, split into the parts of the three symbols, and the part of the next symbol (violet) becomes the next row. The last interval is $\lbrack 19 / 64 , 20 / 64 )$, of size $s_4 = 1 / 64$, and the code is the first $d = 7$ bits of its midpoint, 0100111.](../figures/compression-arithmetic.svg){#fig-21-3}

<span id="alg-21-6"></span>**Algorithm 21.6.** The interval of a text, its
code, and the decoding, in exact arithmetic.

--8<-- "generated/ArithmeticCoding-arithmeticInterval.pseudo.md"

--8<-- "generated/ArithmeticCoding-arithmeticCodeExact.pseudo.md"

--8<-- "generated/ArithmeticCoding-arithmeticDecodeExact.pseudo.md"

Note that the exact code needs numbers of $d$ bits. Instead, the coder of
[Witten, Neal and Cleary 1987](Z-bibliography.md#ref-witten1987)
(`RangeEncoder`, the range coding of the same chapter of Ferragina) keeps the
interval in integers of $w$ bits, $[ L , H ) \subseteq [ 0 , M )$ with $M =
2^w$, and we take a probability as a count out of a total $T$. In it, the
interval of a symbol of cumulative counts $[ a , b )$ is

$$
[ L + \lfloor ( H - L ) a / T \rfloor , L + \lfloor ( H - L ) b / T
\rfloor ) , \tag{21.8}
$$

and the intervals of the symbols tile the current one with no gap. After each
symbol we expand the interval. If it lies in $[ 0 , M / 2 )$ the coder emits 0,
and if it lies in $[ M / 2 , M )$ it emits 1; if it lies in $[ M / 4 , 3 M / 4
)$, straddling the middle, it emits nothing but counts an *underflow*, whose
bit (the opposite of the next one emitted) follows that bit. In the three cases
the interval is then doubled ([Algorithm 21.7](21-compression.md#alg-21-7)).

<span id="alg-21-7"></span>**Algorithm 21.7.** The encoding of a symbol and
its decoding by the integer coder of
[Witten, Neal and Cleary 1987](Z-bibliography.md#ref-witten1987);
$p$ is the number of the underflows pending, and $v$ the $w$ bits read.

--8<-- "generated/ArithmeticCoding-RangeEncoder-encode.pseudo.md"

--8<-- "generated/ArithmeticCoding-RangeDecoder-target.pseudo.md"

--8<-- "generated/ArithmeticCoding-RangeDecoder-consume.pseudo.md"

<span id="lem-21-4"></span>**Lemma 21.4.** *After the expansions of
`RangeEncoder::encode()`, $H - L > M / 4$; hence, with a total $T \le M / 4$
(the `maxTotal()` of the encoder and of the decoder), the interval (21.8) of a
symbol of positive count is nonempty.*

*Proof.* When the loop of the expansions stops, none of its three cases
applies, i.e., $L < M / 2 < H$ and either $L < M / 4$ or $H > 3 M / 4$. In the
first case $H - L > M / 2 - M / 4$, in the second $H - L > 3 M / 4 - M / 2$,
and both are $M / 4$. Then $( H - L ) ( b - a ) / T > ( M / 4 ) / T \ge 1$ for
$b > a$, and the two ends of (21.8) differ by at least 1. $\square$

In the implementation, which follows the pseudocode, the bits are emitted by
`emit()`, which also writes the pending underflows.

At the end, `finish()` emits 01 or 10, with the pending bits, according to
whether $L < M / 4$; this picks a point of the last interval whatever bits
follow. On its side, the decoder keeps the $w$ bits read in $v$ and inverts
(21.8) by `target()`, whose result $t$ satisfies $a \le t < b$ for the symbol
coded (cf. [Algorithm 21.7](21-compression.md#alg-21-7)). It then applies the
same expansions to $v$, reading zeros past the end of the code. With $w = 32$
the rounding of (21.8) typically costs about $10^{-4}$ bits per symbol against
the exact code, as Witten, Neal and Cleary measured. The functions
`arithmeticEncode()` and `arithmeticDecode()` code a text with a static model
of counts, whose total must not exceed $M / 4 = 2^{w-2}$ by [Lemma
21.4](21-compression.md#lem-21-4) ([Algorithm
21.8](21-compression.md#alg-21-8)).

<span id="alg-21-8"></span>**Algorithm 21.8.** The arithmetic code with a
static model of counts, and with the adaptive model of chapter 12 of
[Ferragina 2023](Z-bibliography.md#ref-ferragina2023).

--8<-- "generated/ArithmeticCoding-arithmeticEncode.pseudo.md"

--8<-- "generated/ArithmeticCoding-arithmeticDecode.pseudo.md"

--8<-- "generated/ArithmeticCoding-adaptiveArithmeticEncode.pseudo.md"

--8<-- "generated/ArithmeticCoding-adaptiveArithmeticDecode.pseudo.md"

An adaptive coder sends nothing in advance, which is perhaps its main
convenience. It starts from $P[ c ] = 1 / \sigma$ and, after a prefix of length
$l$ with $l_c$ occurrences of $c$, takes $P[ c ] = ( l_c + 1 ) / ( l + \sigma
)$. Its counts are halved, rounding up, when their total passes $\max( 2^{16} ,
2 \sigma )$, which keeps the total below $M / 4$ and makes the model follow a
text whose statistics change. Furthermore, the cumulative counts are in a tree
of [Fenwick 1994](Z-bibliography.md#ref-fenwick1994), where an increment, a
prefix sum and the search of the symbol of a cumulative count take $O( \log
\sigma )$ time; hence a symbol costs $O( \log \sigma )$ time, for alphabets of
up to $2^{20}$ symbols. In practice the coders typically emit bytes instead of
bits (the range coder of [Martin 1979](Z-bibliography.md#ref-martin1979)), or
use the asymmetric numeral systems of the next section, which reach the same
compression with a multiplication or a lookup per symbol. [Howard and Vitter
1994](Z-bibliography.md#ref-howard1994) survey the implementations in integers.

## 21.5 Asymmetric numeral systems {#sec-21-5}

The asymmetric numeral systems of [Duda 2013](Z-bibliography.md#ref-duda2013)
replace the interval of the arithmetic code with a single integer state
$x$. With the frequencies $f_c$ of the symbols scaled to a total
$M = 2^{12}$, and $c_c$ their prefix sums (the cumulative frequency of $c$),
the symbol $c$ moves the state to

$$
C( c , x ) = M \left\lfloor \frac{x}{f_c} \right\rfloor + c_c +
( x \bmod f_c ) , \tag{21.9}
$$

which multiplies $x$ by about $M / f_c$, i.e., adds about
$\log_2( M / f_c )$ bits to it, the information of $c$ under the scaled
frequencies. Conversely, the decoder reads $c$ off $x \bmod M$, which lies in
$[ c_c , c_c + f_c )$, and recovers the previous state. Since the decoder
undoes the last symbol first, the encoder of `Rans.h` takes the symbols from
the last to the first; the decoder, which reads the stream backward, then
meets them in their order.

<span id="lem-21-5"></span>**Lemma 21.5.** *(i) For each $c$, the map
$x \mapsto C( c , x )$ is a bijection of the naturals onto the naturals
$y$ with $y \bmod M \in [ c_c , c_c + f_c )$, whose inverse is*

$$
D( y ) = f_c \lfloor y / M \rfloor + ( y \bmod M ) - c_c ; \tag{21.10}
$$

*(ii) if the encoder moves the 16 low bits of the state to the stream
whenever $x \ge 2^{20} f_c$ before (21.9), the states between two symbols
stay in $[ 2^{16} , 2^{32} )$, and at most one word of 16 bits is moved per
symbol.*

*Proof.* (i) Write $x = q f_c + r$ with $0 \le r < f_c$. Then $C( c , x ) = q
M + c_c + r$, whose remainder modulo $M$ is $c_c + r \in [ c_c , c_c + f_c )$ and
whose quotient is $q$; (21.10) returns $f_c q + r = x$, and each $y$ with such
a remainder is reached by $x = D( y )$. (ii) Let $2^{16} \le x < 2^{32}$. If $x
\ge 2^{20} f_c$, the state becomes $\lfloor x / 2^{16} \rfloor$, which is at
least $2^4 f_c$ and below $2^{16} \le 2^{20} f_c$, and a second move is not
needed. Otherwise $2^{16} \le x < 2^{20} f_c$, and $x \ge 2^{16} \ge 2^4 f_c$
since $f_c \le 2^{12}$. In both cases $16 \le \lfloor x / f_c \rfloor <
2^{20}$; hence, (21.9) gives $2^{16} \le C( c , x ) < 2^{12} \cdot 2^{20}$.
After (21.10) the decoder finds a state below $2^{16}$ if and only if the
encoder had moved a word, which it reads back. $\square$

Hence we pay about $\log_2( M / f_c )$ bits for a symbol, and a text about
$\sum_i \log_2( M / f_{s[ i ]} )$ bits plus the final states. With $f_c$
proportional to $n_c$ this is $n H_0$; the rounding of the frequencies to the
total $M$ (each byte present kept at 1 at least, by `scaleFrequencies()`) costs
a fraction of a percent on a text of bytes. Unlike the arithmetic coder (cf.
(21.6)), which narrows an interval and emits the bits that its ends share, the
state needs one multiplication per symbol, no division in the decoder, and a
renormalization with no carry.

<span id="alg-21-9"></span>**Algorithm 21.9.** The rANS coder of order 0
of [Duda 2013](Z-bibliography.md#ref-duda2013) with the interleaving of
[Giesen 2014](Z-bibliography.md#ref-giesen2014), and the scaling of the
frequencies; $Z[ j ]$ is the symbol of the slot $j \in [ 0 , M )$.

--8<-- "generated/Rans-scaleFrequencies.pseudo.md"

--8<-- "generated/Rans-ransCompress.pseudo.md"

--8<-- "generated/Rans-ransDecompress.pseudo.md"

[Table 21.3](21-compression.md#tab-21-3) follows (21.9) and (21.10) on a small
example, with a total $M = 8$ in place of $2^{12}$ and no renormalization. One
can see that the state grows by about $M / f_c$ per symbol in the encoder, and
shrinks back in the decoder.

<span id="tab-21-3"></span>**Table 21.3.** rANS on the text abac with
$M = 8$, $f_a = 4$, $f_b = f_c = 2$ (hence $c_a = 0$, $c_b = 4$, $c_c = 6$),
from the state 8: the encoder takes the symbols from the last, and the
decoder reads each symbol off the state modulo 8 and undoes (21.9) by
(21.10).

| Step | Encoder: $c$ | $x$ | $C( c , x )$ | Decoder: $y$ | $y \bmod 8$ | $c$ | $D( y )$ |
|---|---|---|---|---|---|---|---|
| 1 | c | 8 | $8 \cdot 4 + 6 + 0 = 38$ | 600 | 0 | a | $4 \cdot 75 + 0 - 0 = 300$ |
| 2 | a | 38 | $8 \cdot 9 + 0 + 2 = 74$ | 300 | 4 | b | $2 \cdot 37 + 4 - 4 = 74$ |
| 3 | b | 74 | $8 \cdot 37 + 4 + 0 = 300$ | 74 | 2 | a | $4 \cdot 9 + 2 - 0 = 38$ |
| 4 | a | 300 | $8 \cdot 75 + 0 + 0 = 600$ | 38 | 6 | c | $2 \cdot 4 + 6 - 6 = 8$ |

Four states take the symbols in turn, $x_{i \bmod 4}$ coding the symbol $i$,
and four chains of dependences thus overlap in the pipeline (the interleaving
of [Giesen 2014](Z-bibliography.md#ref-giesen2014)); the four states share one
stream of words of 16 bits. In the encoder the division of (21.9) by $f_c$ is a
multiplication by a reciprocal and a shift ([Granlund and Montgomery
1994](Z-bibliography.md#ref-granlund1994)), exact on the states below $2^{32}$.
Its decoder is a table of the $M$ slots, which gives for each slot $j$ its
symbol, $f_c$ and $j - c_c$. We may thus write a step of it as a mask, a
lookup, a multiplication and an addition, and the word of 16 bits is read with
no branch, the pointer into the stream moving by twice the bit that is 1 when
the state fell below $2^{16}$.

The tabled variant of `Tans.h` (tANS, the entropy coder of order 0 of
Zstandard) makes the transitions of the state lookups, with no multiplication.
With $L = 2^R$ states, $R = 11$, and the frequencies scaled to sum to $L$, the
$L$ states are dealt to the symbols, $f_c$ each, by the step $L / 2 + L / 8 +
3$ modulo $L$. This step is odd, and therefore it visits all the states; also,
it spreads the states of a symbol over the table. In the state $x \in [ 0 , L
)$, the $i$-th state of the symbol $c$ (counting from 0), the decoder emits $c$
and moves to

$$
x' = ( f_c + i ) \, 2^{b} - L + v , \qquad
b = R - \lfloor \log_2( f_c + i ) \rfloor , \tag{21.11}
$$

$v$ being the next $b$ bits of the stream; since $f_c + i \in [ f_c , 2 f_c )$,
one has $( f_c + i ) 2^b \in [ L , 2 L )$ and $x' \in [ 0 , L )$. In turn, the
encoder inverts (21.11). In the state $X \in [ L , 2 L )$ it emits the $k$ low
bits of $X$, with $k$ the number of bits that brings $\lfloor X / 2^k \rfloor$
into $[ f_c , 2 f_c )$ (one of two values, found by one addition and a shift),
and moves to $L$ plus the state of the $( \lfloor X / 2^k \rfloor - f_c )$-th
place of $c$ ([Algorithm 21.10](21-compression.md#alg-21-10)). A symbol costs
about $\log_2( L / f_c )$ bits, and therefore the code usually comes within a
fraction of a percent of $n H_0$ when $L$ is a few times the alphabet; [Dube
and Yokoo 2019](Z-bibliography.md#ref-dube2019) analyze the loss of the spread.
We show the table of a "small" decoder in [Table
21.4](21-compression.md#tab-21-4).

Both directions of (21.11) are then exact, and we can state them as
follows (with $L = 2^R$, $R \ge 3$, as in `Tans.h`).

<span id="lem-21-6"></span>**Lemma 21.6.** *(i) The step
$L / 2 + L / 8 + 3$ modulo $L$ deals each of the $L$ states once. (ii) Let
$f_c \ge 1$ and $m = \lfloor \log_2 f_c \rfloor$. In a state
$X \in [ L , 2 L )$ the encoder of $c$ emits $k \in \{ R - m - 1 , R - m
\}$ bits and moves to a state $X' \in [ L , 2 L )$; in the state $X' - L$
the decoder emits $c$, reads $b = k$ bits, those emitted, and by (21.11)
moves to $X - L$.*

*Proof.* (i) Since $R \ge 3$, $L / 2$ and $L / 8$ are even, and the step
is odd, i.e., prime to $L = 2^R$; hence its multiples modulo $L$ are
distinct. (ii) Let $a = \lfloor X / 2^{R - m} \rfloor$, which lies in
$[ 2^m , 2^{m + 1} )$ since $2^R \le X < 2^{R + 1}$. If $a \ge f_c$, then
$a < 2^{m + 1} \le 2 f_c$ and $k = R - m$; otherwise
$\lfloor X / 2^{R - m - 1} \rfloor \in \{ 2 a , 2 a + 1 \}$ is below
$2 f_c$ and at least $2^{m + 1} > f_c$, and $k = R - m - 1$. In both cases
$\lfloor X / 2^k \rfloor = f_c + i$ with $0 \le i < f_c$, and the encoder
moves to $L$ plus the $i$-th state $x$ of $c$ (line 8 of
[Algorithm 21.10](21-compression.md#alg-21-10)), a state of $[ 0 , L )$
by (i). From $( f_c + i ) 2^k \le X < ( f_c + i + 1 ) 2^k$ and
$2^R \le X < 2^{R + 1}$ one has $2^{R - k} \le f_c + i < 2^{R + 1 - k}$,
i.e., $\lfloor \log_2( f_c + i ) \rfloor = R - k$, and the decoder in $x$
reads $b = k$ bits. Since the stream is read backward from its end, these
are the $k$ low bits of $X$, the last ones written, and (21.11) gives
$( f_c + i ) 2^k - L + ( X \bmod 2^k ) = X - L$. $\square$

By induction from the final states, which the decoder reads first
(line 6), `tansDecompress()` thus returns the input of `tansCompress()`.

<span id="tab-21-4"></span>**Table 21.4.** The decoding table of tANS with
$L = 16$ states ($R = 4$, step 13) for $f_a = 8$, $f_b = f_c = 4$: the
symbol of each state, its rank $i$ among the states of the symbol, the bits
$b$ that the decoder reads and the base $( f_c + i ) 2^b - L$ to which it
adds them.

| $x$ | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| symbol | a | a | b | c | a | b | c | a |
| $i$ | 0 | 1 | 0 | 0 | 2 | 1 | 1 | 3 |
| $b$ | 1 | 1 | 2 | 2 | 1 | 2 | 2 | 1 |
| base | 0 | 2 | 0 | 0 | 4 | 4 | 4 | 6 |
| **$x$** | **8** | **9** | **10** | **11** | **12** | **13** | **14** | **15** |
| symbol | b | c | a | a | c | a | a | b |
| $i$ | 2 | 2 | 4 | 5 | 3 | 6 | 7 | 3 |
| $b$ | 2 | 2 | 1 | 1 | 2 | 1 | 1 | 2 |
| base | 8 | 8 | 8 | 10 | 12 | 12 | 14 | 12 |

<span id="alg-21-10"></span>**Algorithm 21.10.** The tANS coder of order 0
of [Duda 2013](Z-bibliography.md#ref-duda2013), for one half of the text;
$\mathit{next}[ c_c + j ]$ is $L$ plus the $j$-th state dealt to $c$.

--8<-- "generated/Tans-tansCompress.pseudo.md"

--8<-- "generated/Tans-tansDecompress.pseudo.md"

We cut the text in two halves, each with a stream of its own and four states
that take its symbols in turn. A stream is written forward from the low bit of
a word and read backward from its end, 4 symbols of at most 11 bits between two
loads of the reader, and the two halves are decoded together; eight chains of
dependences thus overlap. At the end of the stream of each half are its four
final states, $R$ bits each. Against rANS, tANS needs no multiplication (cf.
(21.11)), but it needs a table per alphabet, which the encoder builds too. Both
keep the static model of the frequencies in the header (2 bytes for each of the
256 bytes), and both check the header in the decoder: the frequencies sum to
the total and the stream fits in the input, so that a "corrupted" input stops
the decoder instead of making it read out of bounds.

## 21.6 Prediction by partial matching {#sec-21-6}

So far, our models give a symbol a probability that does not depend on the
symbols before it. The *prediction by partial matching* of [Cleary and Witten
1984](Z-bibliography.md#ref-cleary1984) (`PPM.h`, `ppmEncode()` and its inverse
`ppmDecode()`) codes each symbol arithmetically in the context of the $K$
symbols before it, and we follow the version C of [Moffat
1990](Z-bibliography.md#ref-moffat1990), which works as follows. For each
context $\alpha$ of length $0 \le l \le K$ the model counts how many times each
symbol has followed $\alpha$ in the text seen so far. Then the symbol $s[ i ]$
is coded in the longest context $s[ i - l .. i - 1 ]$ that has already seen it,
starting from $l = \min( K , i )$. Each context on the way that does not
predict it emits an *escape*, whose count in the version C is the number $q$ of
the distinct symbols seen in the context; hence a symbol of count $c( \sigma )$
in a context seen $m$ times has the probability $c( \sigma ) / ( m + q )$, and
the escape $q / ( m + q )$. After $K + 1$ escapes the context of order $-1$
gives the same probability to the symbols not yet excluded.

By the principle of *exclusion*, a context that escapes informs the decoder
that the symbol is none of those it predicts, and the shorter contexts leave
them out of their counts. Note that the escape keeps its count $q$, as in the
example of chapter 12 of [Ferragina 2023](Z-bibliography.md#ref-ferragina2023);
the context of order $-1$ gives $1 / ( \sigma - | X | )$ to each symbol outside
the set $X$ of the excluded ones, and a context whose symbols are all excluded
emits nothing. After each symbol the counts of all its contexts grow by one,
and those of a context are halved, rounding up, when their total passes
$2^{16}$. The decoder repeats the walk of the encoder, reading in the code
whether each context escapes ([Algorithm 21.11](21-compression.md#alg-21-11)),
and [Table 21.5](21-compression.md#tab-21-5) follows the walk on a symbol.

The walk is decodable, and its totals stay within those of the coder,
by the following lemma.

<span id="lem-21-7"></span>**Lemma 21.7.** *Between two symbols the
counts of each context sum to at most $2^{16}$. Hence each step of
`steps( c )` codes an item of count at least 1 out of a total
$N \le 2^{17}$, within the totals $T \le M / 4$ of the coder on $w = 32$
bits; and `ppmDecode( ppmEncode( s , K ) , |s| , K )` is $s$.*

*Proof.* An update adds 1 to a count of each context of the symbol. If
the sum then exceeds $2^{16}$, it is $2^{16} + 1$, and the halving with
rounding up leaves at most $( 2^{16} + 1 + q ) / 2$, $q \le \sigma \le
2^{16}$ being the symbols of the context; since the sum is an integer, it
is at most $2^{16}$. A symbol enters a context with count 1, which the
halving keeps at 1 at least; hence, in a context that codes, each symbol
not excluded has a count of at least 1, the escape has the count
$q \ge 1$, and $N \le 2^{16} + 2^{16}$. The excluded symbols are those of
the contexts that escaped, which did not predict $c$; hence $c$ has the
count 1 out of $\sigma - | X | \ge 1$ in the context of order $-1$.
Finally, `decode()` makes the walk of `steps()` on the same counts, since
both sides update the model after each symbol, and at each context it
takes the item whose range holds the target of the decoder, which is the
item coded by [Lemma 21.4](21-compression.md#lem-21-4). $\square$

<span id="alg-21-11"></span>**Algorithm 21.11.** The steps that code a
symbol $c$ in PPM with exclusion, version C, the update of the counts, and
the coder and the decoder of a text; $h$ is the history of the last $K$
symbols and $X$ the set of the symbols excluded.

--8<-- "generated/PPM-PPMModel-steps.pseudo.md"

--8<-- "generated/PPM-PPMModel-update.pseudo.md"

--8<-- "generated/PPM-ppmEncode.pseudo.md"

--8<-- "generated/PPM-ppmDecode.pseudo.md"

<span id="tab-21-5"></span>**Table 21.5.** The steps of PPM of order
$K = 2$ on the bytes for the symbol c after abra (the fifth symbol of
abracadabra): the contexts from the longest, the symbols they predict
after the exclusions, the item coded and its probability.

| Context | Counts seen | Excluded before | Item coded | Probability |
|---|---|---|---|---|
| ra | none | | nothing (the context is new) | 1 |
| a | b 1 ($q = 1$) | | escape | $1 / ( 1 + 1 ) = 1 / 2$ |
| (empty) | a 2, b 1, r 1 ($q = 3$) | b | escape | $3 / ( 2 + 1 + 3 ) = 1 / 2$ |
| order $-1$ | each byte 1 | a, b, r | c | $1 / 253$ |

The symbol c of [Table 21.5](21-compression.md#tab-21-5) costs $1 + 1 + \log_2
253 \approx 10$ bits, more than the 8 of a byte, since the model has seen only
four symbols, a "small" history. Contexts typically pay off on long texts,
where most symbols are coded at the first or the second context. In general,
the compression approaches the entropy of order $K$, and on the texts of a
language it usually grows with $K$ up to 4 or 5 symbols, though this may depend
on the text; beyond that, the escapes cost more than the longer contexts gain,
as chapter 12 of [Ferragina 2023](Z-bibliography.md#ref-ferragina2023)
observes. Coding a symbol takes $O( K ( K + \sigma_K ) \log n )$ time, with
$\sigma_K$ the largest number of symbols seen in a context, since the contexts
are kept in a balanced tree keyed by their strings. In practice they are kept
in a trie of the contexts with links to the suffixes ([Moffat
1990](Z-bibliography.md#ref-moffat1990)), where the next shorter context is a
step away. Among the escape methods of the literature are A, $1 / ( m + 1 )$,
and B, $q / m$, of [Cleary and Witten 1984](Z-bibliography.md#ref-cleary1984);
C, here; D, $q / 2 m$; P, X and XC of [Witten and Bell
1991](Z-bibliography.md#ref-witten1991), estimated from the symbols seen once.
Also, the PPMd of [Shkarin 2002](Z-bibliography.md#ref-shkarin2002) estimates
the escape from the statistics of similar contexts, and PPM* of [Cleary, Teahan
and Witten 1995](Z-bibliography.md#ref-cleary1995) uses contexts of unbounded
length.

## 21.7 Codes of the integers {#sec-21-7}

The codes of `IntegerCodes.h` (chapter 11, Integer Coding, of [Ferragina
2023](Z-bibliography.md#ref-ferragina2023)) write a sequence of positive
integers one after the other, each in a self-delimiting codeword that is short
for the small integers. We write them on the streams of bits of
`ADS/succinct/BitStream.h` ([§22.2](22-succinct.md#sec-22-2)), the most
significant bit first, and the stream thus equals the codeword on paper. By
[Lemma 21.1](21-compression.md#lem-21-1) a code of lengths $\ell( x )$ is
optimal for the distribution $P[ x ] = 2^{-\ell( x )}$, and choosing a code is
therefore (implicitly) choosing the distribution of the data. For instance, the
*d-gaps* of a sorted list (its first item and the differences of consecutive
items, `dGaps()`) are small and skewed, and a code with short codewords for the
small integers takes few bits on them. The function `signedToNatural()` maps an
integer of any sign to a natural number ($x \ge 0$ to $2 x$, $x < 0$ to $-2 x +
1$), and the codes then apply to differences of any sign. Each code has its
encoder (`encodeUnary()` for the unary one) and its decoder (`decodeUnary()`,
`decodeGamma()`, `decodeDelta()`, `decodeRice()`, `decodeGolomb()`,
`decodeVByte()`, `decodeDense()`) and the function of its length in bits or
bytes (`unaryLength()`, `gammaLength()`, `deltaLength()`, `riceLength()`,
`golombLength()`, `vbyteLength()`, `denseLength()`), which the encoders ensure
at the level of the postconditions; `binaryLength( x )` is $\lvert B( x )
\rvert$, `naturalToSigned()` inverts `signedToNatural()`, and `prefixSums()`
inverts `dGaps()`. We list the codes, with their lengths and their codewords
for $x = 9$, in [Table 21.6](21-compression.md#tab-21-6); of course, the
"optimal" distributions of its fourth column are those of [Lemma
21.1](21-compression.md#lem-21-1).

<span id="tab-21-6"></span>**Table 21.6.** The codes of `IntegerCodes.h`;
$B( x )$ is the binary code of $x$, of $\lvert B( x ) \rvert = 1 +
\lfloor \log_2 x \rfloor$ bits, and the last column the codeword of 9
($B( 9 ) = 1001$).

| Code | Codeword | Length | Optimal for | $x = 9$ |
|---|---|---|---|---|
| unary $U( x )$ | $0^{x - 1} 1$ | $x$ | $2^{-x}$ | 000000001 |
| $\gamma$ (Elias) | $0^{\lvert B( x ) \rvert - 1} B( x )$ | $2 \lfloor \log_2 x \rfloor + 1$ | $1 / ( 2 x^2 )$ | 0001001 |
| $\delta$ (Elias) | $\gamma( \lvert B( x ) \rvert ) \, B( x )$ | $1 + 2 \lfloor \log_2 \lvert B( x ) \rvert \rfloor + \lvert B( x ) \rvert$ | about $1 / ( 2 x ( \log_2 x )^2 )$ | 001001001 |
| Rice $R_k$ | $U( q + 1 )$, then $r$ in $k$ bits | $\lfloor ( x - 1 ) / 2^k \rfloor + 1 + k$ | geometric, $2^k \approx \ln 2 / p$ | 00100 ($k = 2$) |
| Golomb, modulus $b$ | $U( q + 1 )$, then $r$ in truncated binary | $q + 1 + \lfloor \log_2 b \rfloor$ or one more | $( 1 - p )^{x-1} p$, $b \approx \ln 2 / p$ | 00111 ($b = 3$) |
| variable byte | groups of 7 bits, a flag each | $8 \lceil \lvert B( x ) \rvert / 7 \rceil$ | whole bytes | 00001001 |
| $( s , c )$-dense | continuers, then a stopper | $k$ words for $s c^{k-1}$ integers | the $s$ of `denseOptimalS()` | the word 9 ($s = c = 128$) |

Note that the unary code is optimal for $2^{-x}$, and it is the part of the
other codes that gives a length. Then the $\gamma$ code of [Elias
1975](Z-bibliography.md#ref-elias1975) writes the length of $B( x )$ in unary,
as $\lvert B( x ) \rvert - 1$ zeros, and then $B( x )$, whose leading 1 ends
the unary part. Its decoder counts the zeros up to the first 1, a word at a
time by the trailing zeros of the word, and reads as many bits more ([Algorithm
21.12](21-compression.md#alg-21-12)). In turn, the $\delta$ code writes the
length in $\gamma$, and takes $1 + o( 1 )$ times the bits of $B( x )$. Chapter
11 of Ferragina keeps the leading 1 of $B( x )$ after the length, which Elias
drops since the length implies it; so do these functions, at the price of one
bit more than the code of Elias.

<span id="alg-21-12"></span>**Algorithm 21.12.** The codes $\gamma$ and
$\delta$ of [Elias 1975](Z-bibliography.md#ref-elias1975).

--8<-- "generated/IntegerCodes-encodeGamma.pseudo.md"

--8<-- "generated/IntegerCodes-decodeGamma.pseudo.md"

--8<-- "generated/IntegerCodes-encodeDelta.pseudo.md"

--8<-- "generated/IntegerCodes-decodeDelta.pseudo.md"

We write a codeword of up to 63 bits in one field of the stream, turned around
so that its most significant bit is read first, and a longer one by its unary
part and its binary part apart. Its decoder is `readUnary()` followed by
`readBinary()` of the stream (cf. [§22.2](22-succinct.md#sec-22-2)).

The code of [Golomb 1966](Z-bibliography.md#ref-golomb1966) of modulus $b$
writes $x - 1 = q b + r$ as $U( q + 1 )$ followed by the remainder $r$ in the
truncated binary code, i.e., $\lceil \log_2 b \rceil - 1$ bits for the first $t
= 2^{\lceil \log_2 b \rceil} - b$ remainders and one more for the others. It is
optimal for the geometric distribution $( 1 - p )^{x - 1} p$ when $b$ is about
$\ln 2 / p$ ([Gallager and van Voorhis
1975](Z-bibliography.md#ref-gallager1975)), the distribution of the gaps
between the occurrences of a word that occurs independently with probability
$p$. In particular, the code of [Rice 1979](Z-bibliography.md#ref-rice1979) is
the case $b = 2^k$, whose remainder is plain binary in $k$ bits ([Algorithm
21.13](21-compression.md#alg-21-13)).

<span id="alg-21-13"></span>**Algorithm 21.13.** The codes of Rice and of
Golomb.

--8<-- "generated/IntegerCodes-encodeRice.pseudo.md"

--8<-- "generated/IntegerCodes-encodeGolomb.pseudo.md"

--8<-- "generated/IntegerCodes-decodeGolomb.pseudo.md"

All these codes are decoded one bit at a time, at least in principle, while the
decoders of the practice want bytes or blocks. The *variable-byte* code writes
$B( x )$ in groups of 7 bits from the most significant, each in a byte whose
high bit is 1 except in the last. It is generalized by the $( s , c )$-dense
codes of [Brisaboa, Fariña, Navarro and Paramá
2007](Z-bibliography.md#ref-brisaboa2007). They split the $s + c = 2^w$ values
of a word of $w$ bits into $s$ *stoppers*, which end a codeword, and $c$
*continuers*, and then $s c^{k - 1}$ integers take $k$ words (e.g., 128
integers take one byte when $s = 128$); `denseOptimalS()` tries each $s$ for
given frequencies and keeps the one of the smallest output. In these terms, the
variable-byte code has $s = c = 128$, but it wastes the codewords that begin
with a group of zeros, which the dense code with $s = c = 128$ does not
([Algorithm 21.14](21-compression.md#alg-21-14)).

<span id="alg-21-14"></span>**Algorithm 21.14.** The variable-byte code and
the dense codes.

--8<-- "generated/IntegerCodes-encodeVByte.pseudo.md"

--8<-- "generated/IntegerCodes-encodeDense.pseudo.md"

--8<-- "generated/IntegerCodes-decodeDense.pseudo.md"

--8<-- "generated/IntegerCodes-denseOptimalS.pseudo.md"

PForDelta ([Zukowski et al. 2006](Z-bibliography.md#ref-zukowski2006)) codes a
block of values in $b$ bits each after subtracting a base (the record
`PForDeltaBlock`, with $b$, the base, the slots and the exceptions, whose
`size()` is the number of values and `codeBits()` the $n b$ bits of the slots
plus 64 for each exception; `decodePForDelta()` decodes it). A value out of $[
\mathit{base} , \mathit{base} + 2^b - 2 ]$ is an *exception*, marked in its
slot by the escape $2^b - 1$ and written apart in 64 bits, and
`pforDeltaWidth()` chooses $b$ so that 90% of the values fit. Its decoder
unpacks the slots with no branch per value and patches the escapes afterwards,
which is, arguably, the main reason for the design. In the original the
exceptions are linked in place of the escapes, and the OptPFD of [Yan, Ding and
Suel 2009](Z-bibliography.md#ref-yan2009) chooses $b$ for each block by the
cost of its exceptions, as `patchedPack()` of
[§21.8](21-compression.md#sec-21-8) does.

--8<-- "generated/IntegerCodes-encodePForDelta.pseudo.md"

The codes above take one integer at a time, while the *interpolative* code of
[Moffat and Stuiver 2000](Z-bibliography.md#ref-moffat2000)
(`encodeInterpolative()`, decoded by `decodeInterpolative()`) takes instead a
strictly increasing sequence $x[ l .. r - 1 ]$ in $[ \mathit{low} , \mathit{hi}
]$ at once. It writes the middle item $x[ m ]$, $m = \lfloor ( l + r - 1 ) / 2
\rfloor$, in the range that the items around it leave to it, by the binary code
of the range (`encodeBounded()`, decoded by `decodeBounded()`); then it writes
the two halves recursively, each in the range that $x[ m ]$ leaves it
([Algorithm 21.15](21-compression.md#alg-21-15)).

<span id="alg-21-15"></span>**Algorithm 21.15.** The interpolative code of
[Moffat and Stuiver 2000](Z-bibliography.md#ref-moffat2000).

--8<-- "generated/IntegerCodes-interpolative.pseudo.md"

<span id="lem-21-8"></span>**Lemma 21.8.** *The middle item of
$x[ l .. r - 1 ]$ lies in $[ \mathit{low} + m - l , \mathit{hi} - ( r - 1 -
m ) ]$, and is written in*

$$
\lceil \log_2( \mathit{hi} - \mathit{low} - r + l + 1 ) \rceil
\tag{21.12}
$$

*bits; hence a run of consecutive integers that fills its range costs no
bit.*

*Proof.* Since the $m - l$ items before $x[ m ]$ are distinct integers of $[
\mathit{low} , x[ m ] - 1 ]$, $x[ m ] \ge \mathit{low} + m - l$; symmetrically,
$x[ m ] \le \mathit{hi} - ( r - 1 - m )$. Hence the range holds $\mathit{hi} -
\mathit{low} - ( r - 1 - l ) + 1$ values, whose binary code takes (21.12) bits.
When the $r - l$ items fill $[ \mathit{low} , \mathit{hi} ]$, the range of the
middle item holds one value, and so does the range of each item in the
recursion; one value takes 0 bits. $\square$

This code adapts to the clusters of a sequence, and therefore it suits the
lists of an index. Since it is not a code of the items one by one (nor
prefix-free item by item), it can beat the code of Huffman, which is optimal
only among the static prefix-free codes of the items, at the price of a
decoding that is recursive and rather slow. Finally, the code of Elias and Fano
of chapter 11 of Ferragina, which is also searched in place, is in
[§22.6](22-succinct.md#sec-22-6).

## 21.8 Engineered codes of the integers {#sec-21-8}

The codes of [§21.7](21-compression.md#sec-21-7) are decoded one integer at a
time, each with a test of its length that the processor may not predict on
"irregular" data. We now see three engineered forms that remove that test. They
put the lengths apart from the bits of the integers, and a block of integers is
then decoded by a fixed sequence of shifts and masks, or by one shuffle of
bytes.

*Binary packing.* First, the SIMD-BP128 of
[Lemire and Boytsov 2015](Z-bibliography.md#ref-lemire2015)
(`BinaryPacking.h`) cuts a sequence of integers of 32 bits into blocks of
128. It writes a block with $b$ bits per integer, $b$ being the bits of its
largest one, i.e., in $16 b$ bytes, and the widths of 16 blocks are 16
bytes before them. Inside a block the integers are laid out vertically over the
4 lanes of a register of 128 bits, the integer $4 q + l$ in the lane $l$ at
the bit $q b$ of the $32 b$ bits of the lane
([Figure 21.4](21-compression.md#fig-21-4)). Four integers are then packed
or unpacked at once by the shifts and the masks of the four lanes,

$$
x_{4 q + l} = \big( w_{\lfloor q b / 32 \rfloor} \gg ( q b \bmod 32 )
\;\vee\; w_{\lfloor q b / 32 \rfloor + 1} \ll ( 32 - q b \bmod 32 )
\big) \wedge ( 2^b - 1 ) , \tag{21.13}
$$

on the lane $l$ of the words $w$ of the block, the second word only where the
field crosses a word. A block of each width has a separate unrolled code, a
template with each shift a constant, chosen by a table of 33 functions. For a
sorted sequence, the code is that of the differences at a distance of 4, $d_i =
x_i - x_{i-4}$, the differential code D4 of [Lemire, Boytsov and Kurz
2016](Z-bibliography.md#ref-lemire2016). Its decoder adds the previous register
of four integers to each register it unpacks, i.e., one addition per four
integers instead of the chain of a prefix sum, for (typically) a fraction of a
bit more per integer ([Algorithm 21.16](21-compression.md#alg-21-16)). The four
lanes are those of the vectors of Highway, with `ADS_WITH_HIGHWAY` on a machine
of little-endian order; without it the same layout is packed and unpacked a
lane at a time, into the same bytes.

![Figure 21.4. The vertical layout of SIMD-BP128 for $b = 5$: the integer $4 q + l$ is in the lane $l$ at the bit $5 q$, the words 0 of the four lanes form the register $r_0$ and the words 1 the register $r_1$, and the field of $x_{24}, \ldots , x_{27}$ (violet) crosses from one word of its lane to the next, which (21.13) reads from both.](../figures/compression-binarypacking.svg){#fig-21-4}

<span id="alg-21-16"></span>**Algorithm 21.16.** The binary packing and its
patched form ([Lemire and Boytsov 2015](Z-bibliography.md#ref-lemire2015)).

--8<-- "generated/BinaryPacking-binaryPack.pseudo.md"

--8<-- "generated/BinaryPacking-binaryUnpack.pseudo.md"

--8<-- "generated/BinaryPacking-patchedPack.pseudo.md"

--8<-- "generated/BinaryPacking-patchedUnpack.pseudo.md"

A block takes the bits of its largest integer, to the bit, at the price of the
integers that are much "larger" than the others of their block. The patched
code, the PFor of [Zukowski et al. 2006](Z-bibliography.md#ref-zukowski2006) in
the form of the FastPFor of Lemire and Boytsov, packs a block with a width $b$
below the width $\beta$ of its largest integer. It writes apart the $c( b )$
integers of $2^b$ or more, the exceptions, as their places (a byte each) and
their high bits ($\beta - b$ each), packed after the block. Its width is the
one of least cost,

$$
b^* = \arg\min_{0 \le b \le \beta} \big( 128 \, b + c( b ) \, ( 8 + \beta -
b ) + 8 \, [ b < \beta ] \big) , \tag{21.14}
$$

where the last term is the byte of $\beta$, which only a block with exceptions
writes; the counts $c( b )$ for all $b$ come from the counts of the widths of
the integers of the block, in one scan. On decoding, the block is unpacked as
above, and then the high bits of the exceptions are set, each by a load of 8
bytes. A sorted sequence is summed at a distance of 4 in a separate pass, once
its exceptions are set, since a sum read right after the scattered writes of
the exceptions might wait for them to reach the cache. A block of the patched
code is $b$ and $c$ in a byte each, $\beta$ in a byte if $c > 0$, the block of
width $b$, the places and the high bits; 8 bytes of 0 end the code.

<span id="lem-21-9"></span>**Lemma 21.9.** *Let $B = \lceil n / 128
\rceil$ and $b_k$ the bit width of the largest integer of the block $k$
(of the differences $d_i$ if `sorted`, with $x_{-4} = \cdots = x_{-1} =
0$). (i) `binaryPack()` writes $16 \lceil B / 16 \rceil + \sum_k 16 b_k$
bytes, and `binaryUnpack()` of them, with the same $n$ and `sorted`,
returns $x$. (ii) `patchedPack()` gives each block the width $b$ that
minimizes (21.14), the largest one on a tie, and writes for it
$2 + [ c > 0 ] + 16 b + c + \lceil c ( \beta - b ) / 8 \rceil$ bytes,
$c = c( b )$, plus 8 bytes at the end; `patchedUnpack()` of them returns
$x$. Both take $O( n )$ time.*

*Proof.* (i) Each group of up to 16 blocks has its 16 widths, and a block
of width $b_k$ fills $32 b_k$ bits in each of its 4 lanes, $16 b_k$ bytes
in all. Every integer of the block is below $2^{b_k}$, and (21.13) reads
back its $b_k$ bits. If `sorted`, the decoder adds to each register the
previous one, i.e., $x_i = d_i + x_{i - 4}$ modulo $2^{32}$, which is
$x_i$ since $x$ is nondecreasing. (ii) The code scans $t = \beta - 1 ,
\ldots , 0$ with $c( t )$ accumulated from the counts of the widths, and
replaces the best width only on a strictly smaller cost. The block holds
$b$ and $c$, $\beta$ if $c > 0$, the $b$ low bits of each integer in
$16 b$ bytes, a place below 128 in a byte for each exception and the
$\beta - b$ high bits of each, packed; the decoder adds $2^b$ times these
high bits to the integers at their places, which gives the integers of
the block, and then sums at a distance of 4 as in (i). Each block takes
$O( 128 + 33 )$ operations. $\square$


*Stream VByte.* The variable-byte code of [§21.7](21-compression.md#sec-21-7)
keeps the length of an integer in the bytes of the integer, a flag bit per
byte, which the decoder tests one byte at a time, as we have seen. The Stream
VByte of [Lemire, Kurz and Rupp 2018](Z-bibliography.md#ref-lemire2018svb)
(`StreamVByte.h`) writes each integer of 32 bits in as many bytes as its
significant bytes, $\ell_i = \max( 1 , \lceil \mathrm{bitwidth}( x_i ) / 8
\rceil )$, and moves the lengths apart. They take 2 bits per integer: the
lengths less 1 of the integers $4 i$, $4 i + 1$, $4 i + 2$ and $4 i + 3$ are in
the bits 0-1, 2-3, 4-5 and 6-7 of the control byte $i$, and all the control
bytes come before the data. In all, the code takes
$$
\lceil n / 4 \rceil + \sum_{i=0}^{n-1} \ell_i \tag{21.15}
$$

bytes, i.e., the variable-byte code with 2 control bits per integer
instead of 1 per byte. A decoder that reads a control byte knows where the
four integers start, with no test on the data. With Highway, on the
vectors of the processor found at run time, one shuffle of bytes moves the
16 bytes from the place
of the first integer to their four places of 4 bytes, the missing high
bytes made 0, with the entry of the control byte in a table of 256 masks
as its mask; the place then moves by the sum of the four lengths, from a
table too ([Figure 21.5](21-compression.md#fig-21-5)). The encoder finds
the control bytes of 8 integers at once, each of their bytes made 0 or 1,
each pair of bytes saturated to one byte and a saturated addition leaving
the 2 bits of each integer in the top bits of its 2 bytes, which the mask
of the top bits gathers, and moves the bytes of 4 integers by the shuffle
of the inverse table. Its decoder takes 8
control bytes at a time and decodes 32 integers with no
test while enough bytes follow; elsewhere the integers are moved one at a
time, still with no test ([Algorithm 21.17](21-compression.md#alg-21-17)).

![Figure 21.5. The integers 1024, 12, 70000 and 5 in Stream VByte: their control byte holds the lengths less 1 (1, 0, 2 and 0, the integer 0 in the low bits), their 7 bytes of data are little-endian, and one shuffle moves the bytes to the four places of 4 bytes of a register, the missing high bytes made 0 (in the faintest tint).](../figures/compression-streamvbyte.svg){#fig-21-5}

<span id="alg-21-17"></span>**Algorithm 21.17.** Stream VByte
([Lemire, Kurz and Rupp 2018](Z-bibliography.md#ref-lemire2018svb)), one
integer at a time in the encoder and four at a time in the decoder.

--8<-- "generated/StreamVByte-streamVByteEncode.pseudo.md"

--8<-- "generated/StreamVByte-streamVByteDecode.pseudo.md"

*Directly addressable codes.* None of the codes so far reads the $i$-th integer
without decoding the ones before it. The directly addressable codes of
[Brisaboa, Ladra and Navarro 2013](Z-bibliography.md#ref-brisaboa2013)
(`DirectAccessCodes.h`) cut the bits of each integer into chunks, the low $b_0$
bits, the next $b_1$, and so on. Its level $\ell$ holds, for each integer that
has bits beyond the first $s_\ell = b_0 + \cdots + b_{\ell - 1}$, its chunk
$\ell$ in an array $A_\ell$ of fixed width $b_\ell$. It also holds a bit of a
vector $B_\ell$ that records whether the integer continues past the level (none
on the last level). An integer at the place $i$ of a level is at the place
$\mathrm{rank}_1( B_\ell , i )$ of the next one, among those that continue
([Figure 21.6](21-compression.md#fig-21-6)), and hence

$$
x_i = \sum_{\ell} A_\ell[ i_\ell ] \, 2^{s_\ell} , \qquad i_0 = i , \quad
i_{\ell + 1} = \mathrm{rank}_1( B_\ell , i_\ell ) , \tag{21.16}
$$

over the levels that $x_i$ reaches. Each level costs $O( 1 )$ time with
the rank of `RankSelect` (cf. [§22.3](22-succinct.md#sec-22-3)), i.e., one load
of the directory and one sideways sum.

![Figure 21.6. The sequence 5, 1, 18, 3, 260, 7, 40 in directly addressable codes with chunks of 3 bits: the level 0 holds the low chunk of each integer and the bits that mark which continue; the three that continue are, in their order, the items of the level 1, and so on. The access to $x_4$ (violet) reads a chunk per level and moves by the rank of its bit.](../figures/compression-dac.svg){#fig-21-6}

We write $n_s$ for the integers of more than $s$ bits ($n_0 = n$) and $w$ for
the bits of the largest one; then a level that holds the bits $[ s , t )$ costs
$n_s ( t - s )$ bits of chunks. Unless it is the last, it also costs $n_s$
continuation bits, with a quarter more for their rank. Its authors find the
widths that minimize the sum by a dynamic program over the places of the cuts.
Each level past the first, however, costs an access a rank and a branch on its
continuation bit, which is hard to predict, and the optimum often takes many
levels of 1 or 2 bits. As the authors propose, we therefore bound the levels to
$h$, 3 by default (`Levels::DEFAULT`), and to none with `Levels::UNCAPPED`,
i.e., $h = w$. With $\mathcal{C}_h( s )$ the least cost of the bits $[ s , w )$
in at most $h$ levels, one has

$$
\mathcal{C}_1( s ) = n_s ( w - s ) , \qquad
\mathcal{C}_h( s ) = \min \Big( \mathcal{C}_1( s ) , \min_{s < t < w}
\big( n_s ( t - s + 1.25 ) + \mathcal{C}_{h - 1}( t ) \big) \Big) .
\tag{21.17}
$$

<span id="lem-21-10"></span>**Lemma 21.10.** *$\mathcal{C}_h( 0 )$ is the
least number of bits, counted as above, of the levels of a cut of the bits
$[ 0 , w )$ into at most $h$ levels, and the dynamic program of
`DirectAccessCodes` finds a cut that attains it in $O( h w^2 )$ time from
the counts $n_s$. For $h \ge w$, in particular with `Levels::UNCAPPED`, it
is the least cost over all the cuts.*

*Proof.* A cut into $k \le h$ levels is a sequence $0 = s_0 < s_1 < \cdots
< s_k = w$, and its cost is a sum of terms, each of which depends only on a
pair $( s_j , s_{j+1} )$ of consecutive cuts and on whether $s_{j+1} = w$.
A cut of $[ s , w )$ into at most $h$ levels is therefore either the single
level $[ s , w )$, of cost $\mathcal{C}_1( s )$, or a first level $[ s , t
)$ with $t < w$, of cost $n_s ( t - s + 1.25 )$, followed by a cut of $[ t ,
w )$ into at most $h - 1$ levels, and by induction on $h$ the least cost is
(21.17). The program computes $\mathcal{C}_l( s )$ for $l = 1 , \ldots , h$
and $s = w - 1$ down to 0, each by a minimum over fewer than $w$ values of
$t$, and keeps the first cut that attains it; following these cuts from $(
h , 0 )$, one level less at each step, gives a cut of at most $h$ levels and
of cost $\mathcal{C}_h( 0 )$. Finally, a level holds at least one bit, so
that no cut has more than $w$ levels, and $\mathcal{C}_w( 0 )$ is the least
cost over all the cuts. $\square$

`DirectAccessCodes` reads an item by `access( i )` or `operator[]`, and is
a random-access range with the aliases of the standard containers
(`value_type`, `size_type`, `difference_type`, `reference`,
`const_reference`, and `const_iterator`, the same as `iterator`, with
`begin()`, `end()`, `cbegin()` and `cend()`); `widths()` gives the widths
of the levels and `sizeInBits()` the bits of the chunks, of the
continuation bits and of their ranks.

<span id="alg-21-18"></span>**Algorithm 21.18.** The construction of the
directly addressable codes by (21.17), with at most $h$ levels, and the
access (21.16)
([Brisaboa, Ladra and Navarro 2013](Z-bibliography.md#ref-brisaboa2013)).

--8<-- "generated/DirectAccessCodes-DirectAccessCodes.pseudo.md"

--8<-- "generated/DirectAccessCodes-access.pseudo.md"

A fixed width $b$ for all the levels (the second constructor) trades bits for
fewer levels, i.e., for fewer ranks per access. With $b = 7$ the chunks are
those of the variable-byte code, its continuation bits moved to a separate
vector with rank. A chunk of 8 or 16 bits is read as its byte or its pair of
bytes, one load that stays in its line of the cache.

## 21.9 The compressors of Lempel and Ziv {#sec-21-9}

The compressors of `LempelZiv.h` (chapter 13, Dictionary-Based Compressors, of
[Ferragina 2023](Z-bibliography.md#ref-ferragina2023)) need no model of the
source. Their dictionary is the text itself, and they replace a string seen
before with a reference to it. LZ77 ([Ziv and Lempel
1977](Z-bibliography.md#ref-ziv1977)) parses the text into *phrases*. At the
place $p$, the longest prefix $\alpha$ of $s[ p .. ]$ that starts in the window
of the last $w$ symbols (and may end past it, overlapping $\alpha$ itself)
gives the triple $\langle d , \lvert \alpha \rvert , c \rangle$, with $d$ the
distance back to the copy and $c$ the symbol after it (none at the end of the
text). Then the parse moves $\lvert \alpha \rvert + 1$ places, and among the
longest prefixes the closest is chosen. Its decoder copies one symbol at a
time, which is right on an overlapping copy too (e.g., on a run of one symbol)
([Figure 21.7](21-compression.md#fig-21-7)).

![Figure 21.7. The parse of LZ77 of aabbabab with no bound on the window: four phrases, the copies in violet and each symbol copied from the one at the tail of its arrow; the last phrase copies 3 symbols from 2 places back, and its third symbol is copied from its first one, which the decoder has just written.](../figures/compression-lz77.svg){#fig-21-7}

<span id="alg-21-19"></span>**Algorithm 21.19.** The parse of LZ77 by a
search of the window, and its decoding.

--8<-- "generated/LempelZiv-lz77Parse.pseudo.md"

--8<-- "generated/LempelZiv-lz77Decode.pseudo.md"

A search of the window takes $O( w ( \ell + 1 ) )$ time for a phrase of length
$\ell$, hence $O( n w )$ in the worst case ([Lemma
21.11](21-compression.md#lem-21-11) below). LZSS ([Storer and Szymanski
1982](Z-bibliography.md#ref-storer1982), `lzssParse()`, decoded by
`lzssDecode()`, [Algorithm 21.20](21-compression.md#alg-21-20)) emits either a
copy $\langle d , \ell \rangle$ or a literal $\langle 0 , c \rangle$, a copy
only if $\ell$ reaches a minimum (1 in chapter 13 of Ferragina, 3 in gzip). We
find the copies as gzip does in `lzssParseHashed()`, which is perhaps the most
used form of the search: the places are chained by the hash of the 3 symbols
that start there, from the most recent, and the search follows the chain of the
next 3 symbols within the window, up to a number of candidates. In this way the
levels of gzip trade the length of the copies for the speed; with no bound on
the chain, the function gives the parse of `lzssParse()` with minimum 3
([Algorithm 21.20](21-compression.md#alg-21-20)). We keep the encoding of the
fields of the phrases apart, and the codes of this chapter apply to them (gzip,
e.g., uses a code of Huffman for the literals and the lengths and one for the
distances).

<span id="alg-21-20"></span>**Algorithm 21.20.** LZSS by the search of the
longest copy in the window, its decoder, and LZSS by the chains of the
hashes of gzip.

--8<-- "generated/LempelZiv-lzssParse.pseudo.md"

--8<-- "generated/LempelZiv-lzssDecode.pseudo.md"

--8<-- "generated/LempelZiv-lzssParseHashed.pseudo.md"

The phrases are the records `LZ77Phrase` (the fields `distance`, `length`
and `next`, the last one empty at the end of the text), `LZSSPhrase`
(`distance`, 0 for a literal, `length` and `literal`) and `LZ78Phrase`
(`id` and `next`), compared field by field.
LZ78 ([Ziv and Lempel 1978](Z-bibliography.md#ref-ziv1978), `lz78Parse()`)
keeps an explicit dictionary of phrases in a trie, which starts with the
empty phrase 0. There the longest phrase $f$ that prefixes the rest of the text
gives $\langle \mathit{id}( f ) , c \rangle$, and $f c$ becomes a new
phrase; the dictionary is therefore closed under prefixes, the trie has one
symbol per edge, and a phrase costs one step of the trie per symbol. A text
that ends inside a phrase $f$ ends with the pair of the parent of $f$ and
its last symbol. When the dictionary has the given number of phrases it is
discarded and restarts empty (freezing it, or replacing the phrase used
least recently, are the other choices of chapter 13 of Ferragina). LZW
([Welch 1984](Z-bibliography.md#ref-welch1984), `lzwEncode()`) starts with
the 256 phrases of one byte and emits only the identifiers: the phrase $f$
followed by $c$, with $f c$ not in the dictionary, gives
$\mathit{id}( f )$ and adds $f c$, and the next phrase starts at $c$
([Algorithm 21.21](21-compression.md#alg-21-21)).

<span id="alg-21-21"></span>**Algorithm 21.21.** LZ78 and LZW, with their
decoders.

--8<-- "generated/LempelZiv-lz78Parse.pseudo.md"

--8<-- "generated/LempelZiv-lz78Decode.pseudo.md"

--8<-- "generated/LempelZiv-lzwEncode.pseudo.md"

--8<-- "generated/LempelZiv-lzwDecode.pseudo.md"

The decoder of LZW adds $f c$ one step late, when it reads the next phrase
$f'$, whose first symbol is $c$; the identifier of $f'$ may then be the one
of the phrase about to be added, which the decoder does not know yet. We
prove that the decoders are right in this case and in the overlapping
copies of LZ77, and that the dictionary of LZ78 is a trie with one symbol
per edge.

<span id="lem-21-11"></span>**Lemma 21.11.** *(i) The decoder of LZ77,
which copies one symbol at a time, writes $s[ p .. p + \ell - 1 ]$ for a
phrase $\langle d , \ell , c \rangle$ at the place $p$ also when $d <
\ell$. (ii) The dictionary of LZ78 is closed under prefixes, and each
phrase is a node of the trie whose parent is the phrase without its last
symbol. (iii) If the decoder of LZW, after the phrase $f$, reads the
identifier of the phrase that it is about to add, that phrase is $f$
followed by the first symbol of $f$. (iv) The search of the window of
[Algorithm 21.19](21-compression.md#alg-21-19) costs $O( w ( \ell + 1 ) )$
for a phrase of length $\ell$, hence $O( n w )$ in all.*

*Proof.* (i) The symbol $p + j$ is copied from $p + j - d < p + j$, which
the decoder has already written, whether it lies before $p$ or inside the
phrase; and by the definition of the phrase $s[ p + j ] = s[ p + j - d ]$
for $j < \ell$. (ii) The empty phrase is in the dictionary, and a new
phrase $f c$ is added only when $f$ is in it; by induction each prefix of
a phrase is a phrase, and $f c$ is the child of $f$ by the edge $c$. (iii)
The encoder adds $f c$ after emitting $f$, where $c$ is the symbol after
$f$ in the text, and the next phrase $f'$ starts at $c$. If the identifier
of $f'$ is that of $f c$, then $f' = f c$, whose first symbol is that of
$f$; since $f'$ also starts with $c$, the first symbol of $f$ is $c$, and
the phrase is $f$ followed by its first symbol, which the decoder writes
without knowing $c$ in advance. (iv) Each of the at most $w$ places of the
window is compared with $s[ p .. ]$ for at most $\ell + 1$ symbols, since
a match longer than $\ell$ would give a longer phrase; the phrases cover
the $n$ symbols, and the sum of $\ell + 1$ over them is at most $2 n$.
$\square$

All these compressors are *universal*: on the texts of an "ergodic" source
their bits per symbol tend to its entropy ([Ziv and Lempel
1978](Z-bibliography.md#ref-ziv1978)). Furthermore, the code of the $z$ phrases
of LZ78 takes at most $n H_k + O( n \log \log n / \log n )$ bits for each fixed
$k$ ([Kosaraju and Manzini 2000](Z-bibliography.md#ref-kosaraju2000)), but with
a term that vanishes rather slowly. In practice LZ77 is used with long windows
and "fast" searches, and with no window at all the parse is found in $O( n )$
time from the suffix array of the text (`lz77ParseLinear()` of `LZ77Linear.h`).
This rests on the following property of the suffixes, which [Kärkkäinen, Kempa
and Puglisi 2013](Z-bibliography.md#ref-karkkainen2013) exploit.

<span id="lem-21-12"></span>**Lemma 21.12.** *Let $r$ be the rank of the
suffix $i$ in $\mathit{SA}$, and let*

$$
\mathit{PSV}( i ) = \mathit{SA}[ \max \{ j < r : \mathit{SA}[ j ] < i \} ]
, \qquad
\mathit{NSV}( i ) = \mathit{SA}[ \min \{ j > r : \mathit{SA}[ j ] < i \} ]
\tag{21.18}
$$

*be the suffixes of the closest ranks before and after $r$ that start before
$i$ (an absent one sharing no prefix with $s[ i .. ]$). Then the longest prefix
of $s[ i .. ]$ that starts at an earlier place has the length $\ell( i ) = \max
\{ \mathrm{lcp}( i , \mathit{PSV}( i ) ) , \mathrm{lcp}( i , \mathit{NSV}( i )
) \}$, and it starts at the one of the two that attains the maximum.*

*Proof.* Recall that the longest common prefix of two suffixes is the least of
the longest common prefixes of the suffixes adjacent in $\mathit{SA}$ between
them, and it can therefore only decrease as the rank moves away from $r$. Among
the suffixes that start before $i$, those of rank below $r$ share with $s[ i ..
]$ at most the prefix that $\mathit{PSV}( i )$ shares, and those of rank above
$r$ at most the prefix that $\mathit{NSV}( i )$ shares. $\square$

Both values of each place come from one scan of $\mathit{SA}$ with a
stack of places that increase from the bottom (lines 3 to 6 of
[Algorithm 21.22](21-compression.md#alg-21-22) below). A place pops the
larger ones, of which it is the next smaller value, and the top that
remains is its previous smaller value; each place is pushed and popped
once.

<span id="thm-21-3"></span>**Theorem 21.3.** *`lz77ParseLinear()` gives
the phrases of LZ77 with no window, with the lengths of `lz77Parse()`, in
$O( n )$ time and two arrays of $n$ integers of 32 bits besides the text
and the suffix array.*

*Proof.* We first show that lines 3 to 6 of [Algorithm
21.22](21-compression.md#alg-21-22) compute (21.18). After the scan of
$\mathit{SA}[ 0 .. j ]$, the stack holds, from the bottom, the places of
$\mathit{SA}[ 0 .. j ]$ that are smaller than each place scanned after them, in
increasing order: this holds before the scan, and the place $x = \mathit{SA}[
j + 1 ]$ removes from the set exactly the places larger than $x$, which are at
the top since the stack increases, and then enters it. A place $y$ popped by
$x$ was smaller than each place scanned between them; hence $x$ is the first
place after $y$ in the order of $\mathit{SA}$ that is smaller than $y$, i.e.,
$\mathit{NSV}( y )$. The top $z$ that remains is smaller than $x$, and each
place $w$ scanned after $z$ and before $x$ is larger than $x$: otherwise $w$,
or a place smaller than $w$ that popped it, would be in the stack above $z$;
hence $z = \mathit{PSV}( x )$. A place that is never popped has no next smaller
value, and one that finds the stack empty has no previous one. By [Lemma
21.12](21-compression.md#lem-21-12) the length of each phrase is then that of
the longest earlier copy. Also, the scan costs $O( n )$, and so does the suffix
array of SA-IS ([Nong, Zhang and Chan 2011](Z-bibliography.md#ref-nong2011)),
which is freed before the parse. At the start $i$ of a phrase of length $\ell$
the two longest common prefixes are found by comparing symbols, at most $\ell +
1$ comparisons each; since the phrases cover the text, these sum to $O( n )$.
$\square$

<span id="alg-21-22"></span>**Algorithm 21.22.** The parse of LZ77 in linear
time ([Kärkkäinen, Kempa and Puglisi
2013](Z-bibliography.md#ref-karkkainen2013)).

--8<-- "generated/LZ77Linear-lz77ParseLinear.pseudo.md"

Note that the source of a copy is the previous or the next smaller value, not
the closest copy that `lz77Parse()` chooses; the distances may therefore differ
while the lengths are the same, and the contract of the function checks only
that the phrases decode to the text. Furthermore, the parse that minimizes the
bits of the code rather than the number of the phrases is studied by
[Ferragina, Nitto and Venturini 2013](Z-bibliography.md#ref-ferragina2013).
LZMA, Zstandard and Brotli, in turn, add models of context and codes of the
arithmetic family to the fields of the phrases.

The parse of least bits optimizes one criterion, while the time of the
decompression may well conflict with the space. A copy from far away, for
instance, saves bytes at the price of an access outside the caches, and a long
run of literals is read fast but stored as it is. `lzBicriteriaParse()` of
`LZBicriteria.h` finds, among the parses into copies and runs of literals, one
of least time within a bound on the size of the code (or, vice versa, one of
least size within a bound on the time). It follows the algorithm of [Farruggia,
Ferragina, Frangioni and Venturini 2014](Z-bibliography.md#ref-farruggia2014)
(cf. [Farruggia et al. 2019](Z-bibliography.md#ref-farruggia2019) for the
journal version, and [Farruggia, Ferragina and Venturini
2014](Z-bibliography.md#ref-farruggia2014b) for its engineering in BC-ZIP).
Here a parse is a path from 0 to $n$ in the *graph of the parses*, whose nodes
are the places of the text and whose arcs are the phrases, each with a space
$s( e )$ and a time $t( e )$. This makes the problem a shortest path with a
constraint on a second weight, which is NP-hard on a general graph; the
structure of this graph makes it approximable within an additive term.

We model the costs on the code that ADS writes (`lzBicriteriaEncode()`, decoded
by `lzBicriteriaDecode()`), which is byte-oriented and writes all the fields in
the variable-byte code of [§21.7](21-compression.md#sec-21-7). It holds the
length $n$ of the text, then for each phrase a token: $2 ( \ell - 1 ) + 1$ for
a copy of $\ell$ symbols followed by $d - 1$ for its distance $d$, or $2 ( k -
1 )$ for a run of $k$ literals followed by the $k$ bytes themselves. With $v( x
)$ the bytes of the code of $x$, a phrase thus takes

$$
s( \langle d , \ell \rangle ) = v( 2 \ell - 1 ) + v( d - 1 ) , \qquad
s( \langle k \text{ literals} \rangle ) = v( 2 k - 2 ) + k
$$

bytes, and the decoder is charged with

$$
t( e ) = \tau_p + \tau_b \, s( e ) + \tau_{q( d )}
$$

units of time, where $\tau_p$ is the cost of a phrase, $\tau_b$ that of a byte
of the code, and $\tau_q$ that of the access to a copy whose distance takes
$q + 1$ bytes, i.e., $d \le 2^{7 ( q + 1 )}$ (0 for a run). This is the model
$t( d , \ell ) = t( d ) + s( d , \ell )$ of the paper, with a cost per phrase
added and with the classes of the distances aligned to those of the code. In
`LZBicriteriaModel`, the defaults ($\tau_p = 8$, $\tau_b = 1$, and 0, 0, 20,
100 and 200 for a copy within 128 bytes, 16 KiB, 2 MiB, 256 MiB and beyond) are
rough cycles of a current processor, and they should be calibrated on the
machine that decompresses (BC-ZIP takes three classes, with boundaries near
$2^{14}$ and $2^{21}$, as ours). We leave out the time of copying the $n$
symbols, which all the parses pay. We remark, however, that a decoder that
copies by blocks of 16 bytes, as ours, pays somewhat less for a few long copies
than for many short ones, which the model ignores. Both weights do not decrease
with $d$ and $\ell$, and a run stops at 64 literals
(`lzBicriteriaMaxLiterals`); its token then takes one byte, and no arc weighs
more than $s_{\max}$ in space and $t_{\max}$ in time.

For each place $i$ and each class $q$ of the distances, the graph keeps the
longest copy $L_q( i )$ that the parse knows within $2^{7 ( q + 1 )}$ places,
from the distance $D_q( i )$. The copy of $\ell$ symbols from $i$ then weighs
as the one of the least class $q$ with $L_q( i ) \ge \ell$, a step function of
$\ell$ whose steps end at the lengths $L_q( i )$ and at the lengths where the
token grows (64, 8192, $2^{20}$ and $2^{27}$); these ends are the *maximal*
arcs of the paper, at most nine for each place. For the source with no window
it takes the longest previous factor of [Crochemore and Ilie
2008](Z-bibliography.md#ref-crochemore2008), which one scan of the suffix array
with a stack finds for all the places (as in [Theorem
21.3](21-compression.md#thm-21-3), but with the LCP array giving the two
longest common prefixes). Closer sources come from a walk of the suffix array
away from the rank of $i$, by decreasing lcp, for at most
`LZBicriteriaModel::candidates` suffixes. Also, each class inherits from $i -
1$ the copy one symbol shorter from the same distance (line 15 of [Algorithm
21.Zlz3](21-compression.md#alg-21-Zlz3)). Hence, the lengths of the bounded windows
are exact when the walk ends before its bound, and possibly too short
otherwise, and what follows holds w.r.t. the best parse of this graph, which
need not be the best parse of the text.

<span id="alg-21-Zlz3"></span>**Algorithm 21.Zlz3.** The bicriteria parse
([Farruggia, Ferragina, Frangioni and Venturini
2014](Z-bibliography.md#ref-farruggia2014)), its code and its decoder.

--8<-- "generated/LZBicriteria-lzBicriteriaParse.pseudo.md"

--8<-- "generated/LZBicriteria-lzBicriteriaEncode.pseudo.md"

--8<-- "generated/LZBicriteria-lzBicriteriaDecode.pseudo.md"

We first collect the properties of the graph on which the rest depends; (iii)
is Theorem 4 of the paper, which allows each shortest path to relax only the
maximal arcs.

<span id="lem-21-Zlz1"></span>**Lemma 21.Zlz1.** *Let $i$ be a place of the text.
(i) The space and the time of the arc $( i , i + \ell )$, a copy or a run, do
not decrease with $\ell$. (ii) If $( i , j )$ is an arc, then for each $i <
i' < j$ the graph has an arc $( i' , j )$ of the same kind, whose space and
time are at most those of $( i , j )$. (iii) For each path from 0 to $n$ there
is a path whose copies are all maximal arcs and whose space and time are at
most those of the first.*

*Proof.* (i) A run takes $v( 2 k - 2 ) + k$ bytes, which grow with $k$. A copy
takes the token $v( 2 \ell - 1 )$, which does not decrease with $\ell$, and the
distance of the shortest source kept at $i$ whose length reaches $\ell$; by
line 19 the classes of the sources kept increase with their lengths, and
therefore neither the class nor the two weights decrease with $\ell$. (ii) The
suffix of a run is a run. Let $( i , j )$ be a copy from a source of class $q$,
so that $L_q( i ) \ge j - i$. Since line 15 sets $L_q( i + 1 ) \ge L_q( i ) -
1$ and the later lines only raise $L_q$, by induction $L_q( i' ) \ge j - i'$;
the least class whose length reaches $j - i'$ at $i'$ is then at most $q$, the
source kept for it (line 19) is of no greater class, and the token of $j - i'$
is not longer than that of $j - i$. (iii) Let $( v , w )$ be the first copy of
the path that is not maximal, and $( v , z )$, with $z > w$, the end of its
step, of the same weights. Let $( h , k )$ be the arc of the path with $h <
z \le k$. Replacing the arcs from $v$ to $k$ with $( v , z )$ and, if $z < k$,
with the arc $( z , k )$ of (ii) gives a path of no greater weights; its first
copy that is not maximal, if any, lies to the right of $v$, and hence the
replacements end. $\square$

Let $c$ be the minimized weight and $r$ the bounded one. By (iii), a shortest
path for the weight $c + \lambda r$, with $\lambda \ge 0$, may relax only the
maximal copies. Runs, whose weight $A + B k$ is affine in their length $k$, are
relaxed all together: the best run into $j$ comes from the least $W( i ) - B i$
over the last 64 places $i$ (with $W$ the distances found so far), which a
deque keeps in $O( 1 )$ amortized time. Each shortest path costs therefore $O(
n )$. With $R$ the bound for the phrases (the header of $v( n )$ bytes is the
same for all the parses) and $\Lambda( \pi , \lambda ) = c( \pi ) + \lambda (
r( \pi ) - R )$, the Lagrangian function $\varphi( \lambda ) = \min_\pi
\Lambda( \pi , \lambda )$ is a lower bound on the optimum for each $\lambda \ge
0$, since a parse within the bound has $r( \pi ) \le R$. Its maximum is found
by the cutting planes of [Handler and Zang
1980](Z-bibliography.md#ref-handler1980), i.e., the hull approach of [Mehlhorn
and Ziegelmann 2000](Z-bibliography.md#ref-mehlhorn2000), who bound its
iterations by a logarithm of $n$ and of the largest weights when these are
integers. It keeps a path $\pi_L$ above the bound and a path $\pi_R$ within it,
and moves $\lambda$ to the crossing of their lines (lines 5 to 10). When the
loop stops at line 8, both $\pi_L$ and $\pi_R$ attain $\varphi( \lambda )$,
i.e., they are $\lambda$-optimal. Yet, neither of them needs to be near the
optimum of the constrained problem, and the second phase of the paper joins
them. The *path swap* $\mathrm{ps}( \pi_1 , \pi_2 , v )$ at a place $v$ of
either path joins a prefix of $\pi_1$ and a suffix of $\pi_2$. It takes the
prefix of $\pi_1$ up to its last place $v' \le v$ and the arc $( v' , v )$ cut
from the arc of $\pi_1$ across $v$, then the arc $( v , v'' )$ cut from the arc
of $\pi_2$ across $v$, with $v''$ the first place of $\pi_2$ not before $v$,
and the suffix of $\pi_2$ from $v''$; both cut arcs are in the graph, by (ii)
of [Lemma 21.Zlz1](21-compression.md#lem-21-Zlz1) and by the prefix of a copy being
a copy from the same source.

<span id="lem-21-Zlz2"></span>**Lemma 21.Zlz2.** *Let $c_{\max}$ and $r_{\max}$ be
the largest weights of an arc. (i) If $\pi_1$ and $\pi_2$ are
$\lambda$-optimal, then $\Lambda( \mathrm{ps}( \pi_1 , \pi_2 , v ) , \lambda
) \le \varphi( \lambda ) + c_{\max} + \lambda r_{\max}$ for each place $v$ of
either path. (ii) For any two paths and two consecutive places $v < w$ of
either of them, the swaps at $v$ and at $w$ differ by at most $c_{\max}$ in $c$
and by at most $r_{\max}$ in $r$.*

*Proof.* We follow Lemmas 10 and 11 of [Farruggia et al.
2014](Z-bibliography.md#ref-farruggia2014), with $w_\lambda = c + \lambda r$
and $\Lambda = w_\lambda - \lambda R$. (i) Let $\pi_A = \mathrm{ps}( \pi_1 ,
\pi_2 , v )$ and $\pi_B = \mathrm{ps}( \pi_2 , \pi_1 , v )$. If $v$ is on both
paths, $\pi_A$ and $\pi_B$ share out the arcs of $\pi_1$ and $\pi_2$, and
$w_\lambda( \pi_A ) + w_\lambda( \pi_B ) = w_\lambda( \pi_1 ) + w_\lambda(
\pi_2 )$. If $v$ is inside the arc $( v' , v'' )$ of $\pi_1$, this arc is
replaced by $( v' , v )$ in $\pi_A$ and by $( v , v'' )$ in $\pi_B$, and since
$w_\lambda( v' , v ) \le w_\lambda( v' , v'' )$ by (i) of [Lemma
21.Zlz1](21-compression.md#lem-21-Zlz1) one has $w_\lambda( \pi_A ) + w_\lambda(
\pi_B ) \le w_\lambda( \pi_1 ) + w_\lambda( \pi_2 ) + c_{\max} + \lambda
r_{\max}$; the case of $v$ inside an arc of $\pi_2$ is symmetric. Hence
$\Lambda( \pi_A , \lambda ) + \Lambda( \pi_B , \lambda ) \le 2 \varphi( \lambda
) + c_{\max} + \lambda r_{\max}$, and $\Lambda( \pi_B , \lambda ) \ge \varphi(
\lambda )$ gives the claim. (ii) If $v$ is on $\pi_1$, the prefix up to $w$ is
the prefix up to $v$ followed by the arc $( v , w )$; otherwise, it is the same
prefix with its last arc $( u , v )$ replaced by $( u , w )$, which weighs no
less by (i) of [Lemma 21.Zlz1](21-compression.md#lem-21-Zlz1) and at most the
largest weights. Either way it grows by between 0 and $c_{\max}$ in $c$ and
between 0 and $r_{\max}$ in $r$. Symmetrically, by (ii) of the same lemma the
suffix from $w$ weighs less than that from $v$ by between 0 and the largest
weights. The difference of the two swaps, i.e., the growth of the prefix less
the decrease of the suffix, lies therefore between $-c_{\max}$ and $c_{\max}$
in $c$ and between $-r_{\max}$ and $r_{\max}$ in $r$. $\square$

In line 11, the scan evaluates each swap in $O( 1 )$, from the weights of the
prefixes of $\pi_L$ and of the suffixes of $\pi_R$, and returns the one of
least $c$ within $R + \sigma$, for a slack $\sigma \ge 0$ (0 by default). The
paper takes $\sigma = 2 t_{\max}$ when the time is bounded; the following bound
covers any slack, $\sigma = 0$ (a parse within the bound) included.

<span id="thm-21-Zlz4"></span>**Theorem 21.Zlz4.** *If the loop of [Algorithm
21.Zlz3](21-compression.md#alg-21-Zlz3) stops at line 8 with the multiplier
$\lambda^*$, then the parse $\pi$ returned with the slack $\sigma$ has $r( \pi
) \le R + \sigma$ and*

$$
c( \pi ) \le \varphi( \lambda^* ) + c_{\max} +
\lambda^* \max \{ 0 , 2 r_{\max} - \sigma \} ,
$$

*where $\varphi( \lambda^* )$ is at most the least $c$ of a parse of the graph
within $R$. In particular, $\sigma = 0$ gives a parse within the bound with an
additive gap of $c_{\max} + 2 \lambda^* r_{\max}$, and $\sigma = 2 r_{\max}$
gives the gap $c_{\max}$ of Theorem 5 of the paper.*

*Proof.* Note that the scan considers $\pi_R$, the swap at 0, which is within
$R$, and $\pi_L$, the swap at $n$. If $r( \pi_L ) \le R + \sigma$, then $c(
\pi_L ) = \varphi( \lambda^* ) - \lambda^* ( r( \pi_L ) - R ) \le \varphi(
\lambda^* )$, since $r( \pi_L ) > R$. Otherwise, as $v$ runs over the places of
the two paths from 0 to $n$, the weight $r$ of the swap goes from at most $R$
to more than $R + \sigma$ by steps of at most $r_{\max}$ ([Lemma
21.Zlz2](21-compression.md#lem-21-Zlz2), (ii)); hence some swap $\pi'$ has $R +
\sigma - r_{\max} < r( \pi' ) \le R + \sigma$, and by (i) of the same lemma $c(
\pi' ) \le \varphi( \lambda^* ) + c_{\max} + \lambda^* ( R + r_{\max} - r( \pi'
) ) < \varphi( \lambda^* ) + c_{\max} + \lambda^* ( 2 r_{\max} - \sigma )$. The
swap returned is no costlier than $\pi_L$ or $\pi'$. Finally, $c( \pi ) \ge
\Lambda( \pi , \lambda ) \ge \varphi( \lambda )$ for each parse $\pi$ within
$R$ and each $\lambda \ge 0$. $\square$

The result of `lzBicriteriaParse()` carries the phrases, their space (the
header included) and time, the lower bound $\varphi( \lambda^* )$, the
multiplier and the gap of [Theorem 21.Zlz4](21-compression.md#thm-21-Zlz4); the gap
is infinite if the loop reaches `LZBicriteriaModel::iterations` shortest paths
before line 8, and a bound out of reach gives the parse of least bounded
weight, marked as not feasible. Since $c_{\max}$ and $r_{\max}$ are the weights
of the "heaviest" arc (a run of 64 literals, or a copy of $n$ symbols from $n$
places back), the gap of the theorem is, on the texts of the tests, far larger
than the difference between $c( \pi )$ and $\varphi( \lambda^* )$, which the
result also allows one to compute a posteriori. In all, the parse costs $O( n
)$ time for the suffix array and the LCP array, $O( n K )$ for the walk with
$K$ candidates, and $O( n )$ for each of the shortest paths (two for the
extremes, and at most the limit of the iterations for the cutting planes). Its
memory is about four arrays of $n$ words while the graph is built, and the
sources of the graph afterwards. The decoder moves a copy of distance at least
16 by blocks of 16 bytes, each of which ends at least 16 bytes before the place
it is written to, and a copy of shorter distance one byte at a time, which is
right on an overlap too (cf. (i) of [Lemma
21.11](21-compression.md#lem-21-11)). In the benchmark of Appendix B, LZ4 and
Zstandard are compared with this decoder on the parse whose space is bounded by
the size of their code, i.e., at the same ratio. It is fair to remark, however,
that Zstandard makes its code smaller by its entropy coders, and that a
byte-oriented code may not reach its sizes at all, in which case the benchmark
has no cell for it.

## 21.10 The transform of Burrows and Wheeler {#sec-21-10}

The rows of the matrix $M'$ are the $n + 1$ rotations of $s \$$ in sorted
order, with $\$$ a sentinel smaller than all the symbols, $F$ its first column
and $L$ its last ([Burrows and Wheeler
1994](Z-bibliography.md#ref-burrows1994); chapter 14, Block-Sorting
Compression, of [Ferragina 2023](Z-bibliography.md#ref-ferragina2023)). The
transform `bwt()` of `BurrowsWheeler.h` is $L$ without the $\$$, together with
the row $r$ of the $\$$ in $L$ (the *primary* row), the two fields `last` and
`primary` of the record `BWT`. Since $\$$ is the smallest symbol and occurs
once, sorting the rotations is sorting the suffixes, and

$$
L[ i ] = s[ \mathit{SA}[ i ] - 1 ] , \tag{21.19}
$$

the symbol before the $i$-th smallest suffix ($\$$ for the suffix 0). Hence the
transform costs the $O( n )$ time of the suffix array of SA-IS ([Nong, Zhang
and Chan 2011](Z-bibliography.md#ref-nong2011)), with the suffix $\$$ put
first. `bwt()`, `HuffmanFMIndex`, `RIndex` and `MoveRIndex` take it from the
engineered SA-IS of `ADS/string/SuffixSorting.h`, with entries of 32 bits for
the texts of fewer than $2^{31} - 1$ symbols and of 64 bits beyond, and with
the row of the $\$$ given by the arithmetic of the places instead of a shift of
the array; `FMIndex` takes instead the SA-IS of `ADS/string/SuffixArray.h`,
with entries of the width of `std::size_t`. Also, the rows that start with a
string $w$ are contiguous, and the symbols that precede the occurrences of $w$
are thus a substring of $L$. On a text whose symbols depend on their context,
as those of a language usually do, $L$ is made of long runs of few distinct
symbols ([Figure 21.8](21-compression.md#fig-21-8)).

![Figure 21.8. The sorted rotations of abracadabra\$, with the suffix array, the first column $F$ and the last column $L$ = ard\$rcaaaabb; on the right, the ranges of the rows prefixed by a, ra, bra and abra, which the backward search of abra finds in this order ([§21.11](21-compression.md#sec-21-11)).](../figures/compression-bwt.svg){#fig-21-8}

<span id="alg-21-23"></span>**Algorithm 21.23.** The transform, the
LF-mapping and the inverse transform
([Burrows and Wheeler 1994](Z-bibliography.md#ref-burrows1994)).

--8<-- "generated/BurrowsWheeler-bwt.pseudo.md"

--8<-- "generated/BurrowsWheeler-lfMapping.pseudo.md"

--8<-- "generated/BurrowsWheeler-inverseBwt.pseudo.md"

The transform can be inverted, by a property that links its two columns.

<span id="lem-21-13"></span>**Lemma 21.13 (the LF-mapping).** *The $k$-th
occurrence of a symbol $c$ in $L$ and the $k$-th occurrence of $c$ in $F$
are the same symbol of $s$; hence the row*

$$
\mathit{LF}( i ) = C[ L[ i ] ] + \mathrm{rank}_{L[ i ]}( L , i )
\tag{21.20}
$$

*starts with the symbol $L[ i ]$ of the row $i$, i.e., it is the rotation
of the row $i$ by one place to the right.*

*Proof.* Two rows $i < j$ with $L[ i ] = L[ j ] = c$, rotated by one place
to the right, become $c$ followed by the row $i$ and $c$ followed by the
row $j$ (without its last symbol), which compare as the rows $i$ and $j$
did. Hence the rows that start with $c$ are in the order of the occurrences
of $c$ in $L$, and they begin at the row $C[ c ]$ of $F$. $\square$

<span id="lem-21-14"></span>**Lemma 21.14.** *Starting from the row 0,
the $n$ symbols $L[ i_0 ] , L[ i_1 ] , \ldots , L[ i_{n-1} ]$ with $i_0 = 0$
and $i_{j+1} = \mathit{LF}( i_j )$ are $s[ n - 1 ] , s[ n - 2 ] , \ldots ,
s[ 0 ]$; hence the inverse transform of
[Algorithm 21.23](21-compression.md#alg-21-23) gives back $s$ in $O( n +
\sigma )$ time.*

*Proof.* The row 0 is the rotation $\$ s$, whose last symbol is $s[ n -
1 ]$. By [Lemma 21.13](21-compression.md#lem-21-13) the row $\mathit{LF}( i
)$ is the row $i$ rotated by one place to the right, and its last symbol is
therefore the symbol that precedes, in the cyclic string $s \$$, the last
symbol of the row $i$. By induction the row $i_j$ is $s \$$ rotated right
by $j$ places, whose last symbol is $s[ n - 1 - j ]$ for $j < n$. The
counts of $L$, hence $C$ and the ranks of (21.20) for all the rows, are
found by one scan of $L$ and a prefix sum over the $\sigma$ symbols, and
the $n$ steps cost $O( 1 )$ each. $\square$

The inverse transform thus spells $s$ backward by $n$ steps of (21.20),
with the counts of $L$ taken in one scan. The
implementation takes the symbols of $L$ as the bytes plus 1 and the $\$$
as 0, and computes the LF-mapping as a counting sort of the rows by their
last symbol.

Each step of the inverse is a random access to $\mathit{LF}$, i.e., a "likely"
miss of the cache on a large text; the transform is cheap to invert in
operations but not (necessarily) in accesses to the memory. On the example of
[Figure 21.8](21-compression.md#fig-21-8), the row 0 ends with a, which is the
first a of $L$ and is thus mapped to the row $\mathit{LF}( 0 ) = C[ a ] + 0 =
1$, a\$abracadabr, which ends with r; and so on, until the row 3 of the $\$$
after 11 steps.

Next come the transforms of chapter 14 of Ferragina. The *move-to-front* of
[Bentley, Sleator, Tarjan and Wei 1986](Z-bibliography.md#ref-bentley1986)
replaces each symbol with its place in a list of the alphabet, and then moves
it to the front. Hence the runs of few symbols of $L$ become runs of small
integers (on the alphabet abcno, bananacocco becomes 1 1 3 1 1 1 3 4 1 0 1), in
$O( \sigma )$ time per symbol here ([Algorithm
21.24](21-compression.md#alg-21-24)).

<span id="lem-21-15"></span>**Lemma 21.15.** *Writing the place $p$ of each
symbol in the move-to-front of $s$ as $\gamma( p + 1 )$ takes at most
$n ( 2 H_0 + 1 ) + O( \sigma \log \sigma )$ bits.*

*Proof.* At its first occurrence a symbol has a place below $\sigma$, which
costs at most $2 \log_2 \sigma + 1$ bits. Any other occurrence of $c$ at the
distance $d$ from the previous one has at most $d - 1$ distinct symbols in
front of it in the list; hence $p + 1 \le d$, and its codeword takes at most $2
\log_2 d + 1$ bits. The $m = n_c - 1$ distances of $c$ sum to at most $n$, and
by the concavity of the logarithm their logarithms sum to at most $m \log_2( n
/ m ) \le n_c \log_2( n / n_c ) + \log_2 e$. The sum over $c$ is $n H_0 + O(
\sigma )$. $\square$

Each of the three codes of
[Algorithm 21.24](21-compression.md#alg-21-24) has its inverse,
`inverseMoveToFront()` (on a string or on the bytes), `runLengthDecode()`
and `zeroRunDecode()`, which the audits of the encoders apply to their
output; the forms of `moveToFront()` and `inverseMoveToFront()` without an
alphabet take the bytes $0 , \ldots , 255$ in their order.

<span id="alg-21-24"></span>**Algorithm 21.24.** The move-to-front and the
run-length codes, as bzip2 chains them, with their inverses.

--8<-- "generated/BurrowsWheeler-moveToFront.pseudo.md"

--8<-- "generated/BurrowsWheeler-inverseMoveToFront.pseudo.md"

--8<-- "generated/BurrowsWheeler-runLengthEncode.pseudo.md"

--8<-- "generated/BurrowsWheeler-runLengthDecode.pseudo.md"

--8<-- "generated/BurrowsWheeler-zeroRunEncode.pseudo.md"

--8<-- "generated/BurrowsWheeler-zeroRunDecode.pseudo.md"

--8<-- "generated/BurrowsWheeler-bzipCompress.pseudo.md"

The run-length encoding replaces a run of $\ell$ symbols $c$ with $\langle \ell
, c \rangle$. In the library, bzip chains these codes as bzip2 does. First
comes the transform of each block (of 900000 symbols by default), then the
move-to-front of $L$, and then the RLE0 of the runs of 0 (and of no other
symbol), with the other integers grown by 1 and a run of $m$ zeros written in
the code of Wheeler (the binary digits of $m + 1$ after its leading 1, as the
symbols 0 and 1; e.g., the integers 0 0 0 0 0 3 become 1 0 4). Last comes the
canonical code of Huffman of the block, with lengths up to 20, written in a
preamble of 5 bits each ([Algorithm 21.25](21-compression.md#alg-21-25)). bzip2
writes the digits of the runs from the least significant one, and it splits a
block in groups of 50 symbols coded by up to 6 tables, which we do not
reproduce. Coding $L$ in this way takes at most $( 5 + \epsilon ) n H_k^*( s )$
bits plus lower terms ([Manzini 2001](Z-bibliography.md#ref-manzini2001)), with
$H_k^*$ a variant of (21.2). Coding instead the pieces of $L$ of each context
of $k$ symbols by a code of order 0 reaches $n H_k$ plus lower terms for each
$k$ at once, which is the compression boosting of [Ferragina, Giancarlo,
Manzini and Sciortino 2005](Z-bibliography.md#ref-ferragina2005b).

<span id="alg-21-25"></span>**Algorithm 21.25.** The decompression of
bzip, each block undone in the opposite order.

--8<-- "generated/BurrowsWheeler-bzipDecompress.pseudo.md"

## 21.11 Compressed indexes {#sec-21-11}

The transform is also an index of the substrings of $s$, which replaces both
the text and its suffix array (each index of this section gives its space by
`sizeInBits()`) ([Ferragina and Manzini
2005](Z-bibliography.md#ref-ferragina2005a)). Indeed, the occurrences of a
pattern $P$ are the rows of $M'$ prefixed by $P$, a range of rows, and the
range can be found from $L$ with no access to the text.

<span id="lem-21-16"></span>**Lemma 21.16 (the backward search).** *If the
rows prefixed by a string $Q$ are $[ \mathit{first} , \mathit{last} )$,
those prefixed by $c Q$ are*

$$
[ C[ c ] + \mathrm{rank}_c( L , \mathit{first} ) , C[ c ] +
\mathrm{rank}_c( L , \mathit{last} ) ) . \tag{21.21}
$$

*Proof.* A row is prefixed by $c Q$ if and only if it is the image by
(21.20) of a row prefixed by $Q$ whose last symbol is $c$. The occurrences
of $c$ in $L[ \mathit{first} .. \mathit{last} - 1 ]$ are those of ranks
$\mathrm{rank}_c( L , \mathit{first} )$ to
$\mathrm{rank}_c( L , \mathit{last} ) - 1$ among the occurrences of $c$ in
$L$, and by [Lemma 21.13](21-compression.md#lem-21-13) their images are the
consecutive rows that (21.21) gives. $\square$

Hence `count( P )` of `FMIndex.h` applies (21.21) from the last symbol of
$P$ to the first, $p$ steps of two ranks each, starting from all the rows;
the occurrences are $\mathit{last} - \mathit{first}$, found with no access
to the text ([Algorithm 21.26](21-compression.md#alg-21-26)). On the example of
[Figure 21.8](21-compression.md#fig-21-8), the pattern abra goes
through the ranges $[ 1 , 6 )$ of a, $[ 10 , 12 )$ of ra, $[ 6 , 8 )$ of
bra and $[ 2 , 4 )$ of abra. The last step, from bra, is
$C[ a ] + \mathrm{rank}_a( L , 6 ) = 1 + 1 = 2$ and $C[ a ] +
\mathrm{rank}_a( L , 8 ) = 1 + 3 = 4$.

<span id="alg-21-26"></span>**Algorithm 21.26.** The construction, the rank
in $L$, the backward search, the location of the occurrences and the
extraction of a substring in the FM-index
([Ferragina and Manzini 2005](Z-bibliography.md#ref-ferragina2005a)).

--8<-- "generated/FMIndex-FMIndex.pseudo.md"

--8<-- "generated/FMIndex-rank.pseudo.md"

--8<-- "generated/FMIndex-range.pseudo.md"

--8<-- "generated/FMIndex-locate.pseudo.md"

--8<-- "generated/FMIndex-extract.pseudo.md"

The implementation of the search follows the pseudocode, with $C[ c ]$
counting the $\$$ too.

To locate the occurrences, the rows of the suffixes that start at a multiple of
$\mu$ are marked, with their places. From any row, the LF-mapping, which moves
one place to the left in $s$, reaches a marked row in $t < \mu$ steps, and the
place is that of the marked row plus $t$. Since the marks are a vector of bits
with the rank of `RankSelect` ([§22.3](22-succinct.md#sec-22-3)), a row is
tested and its sample found in $O( 1 )$ time; the samples take $O( ( n / \mu )
\log n )$ bits. The function `extract( i , j )` gives $s[ i .. j - 1 ]$ from
the row of the first sampled suffix at or after $j$, by $j - i + O( \mu )$
steps of the LF-mapping, each of which reads the symbol before the current
suffix.

<span id="lem-21-17"></span>**Lemma 21.17.** *From any row, at most
$\mu - 1$ steps of the LF-mapping reach a marked row, and the suffix of the
starting row is the sample of the marked row plus the number of steps;
`extract( i , j )` makes at most $j - i + \mu$ steps.*

*Proof.* By [Lemma 21.13](21-compression.md#lem-21-13), a step from the row
of the suffix $x \ge 1$ leads to the row of the suffix $x - 1$. Among $x ,
x - 1 , \ldots , x - \mu + 1$ one is a multiple of $\mu$ (or the suffix 0
is reached first, which is a multiple of $\mu$), and the row of a multiple
of $\mu$ is marked; after $t$ steps the suffix is $x - t$, hence $x$ is
the sample plus $t$. For `extract()`, the first sampled suffix at or after
$j$ is below $j + \mu$ (or it is the end of the text, the row 0), and the
steps from it down to $i$ are at most $j + \mu - i$; each reads the
symbol $L$ of the current row, which precedes the current suffix.
$\square$

In `FMIndex`, the rank of a symbol in $L$ is the count of the symbol stored
each $b$ places for each symbol of the text, plus a scan of fewer than $b$
symbols, 8 at a time by the zero bytes of a word. This is $O( b )$ time and
$\sigma ( n / b )$ counts of 32 bits, with the defaults $b = 64$ and $\mu =
32$. These counts are the simplest rank on a sequence, which a structure of
rank and select on sequences can replace ([§22.9](22-succinct.md#sec-22-9)).
The wavelet tree of [Grossi, Gupta and Vitter
2003](Z-bibliography.md#ref-grossi2003) gives the rank in $O( \log \sigma )$
time on $n H_0 + o( n \log \sigma )$ bits, and the compressed representations
of [Ferragina, Manzini, Mäkinen and Navarro
2007](Z-bibliography.md#ref-ferragina2007) reach $n H_k + o( n \log \sigma )$
bits, the space of the compressed text.

In `HuffmanFMIndex.h` we follow the second way, with the succinct suffix array
of [Mäkinen and Navarro 2005](Z-bibliography.md#ref-makinen2005): $L$ is a
wavelet tree shaped by the code of Huffman (`HuffmanWaveletTree`,
[§22.9](22-succinct.md#sec-22-9)), in $n ( H_0 + 1 ) ( 1 + o( 1 ) )$ bits. A
step of the backward search takes the two ranks of $c$ by one descent of the
tree along the codeword of $c$, since both meet the same nodes, in $O( H_0 + 1
)$ steps on average. A step of `locate()` is one descent too, since
`accessRank()` gives $L[ r ]$ and its rank, i.e., the LF-mapping, in one pass.
Its samples are $\lceil n / \mu \rceil$ places and as many rows of $\lceil
\log_2( n + 1 ) \rceil$ bits each. Its marks are $n + 1$ bits with the compact
rank of `CompactRankSelect`, since a step of `locate()` tests only the bit and
the rank is needed once per occurrence. Finally, the template parameter `RS`
chooses the rank and select of the tree, `RankSelect` for time or
`CompactRankSelect` for space ([Algorithm 21.27](21-compression.md#alg-21-27)).

<span id="alg-21-27"></span>**Algorithm 21.27.** The construction, the
backward search, the location and the extraction in the FM-index on the
wavelet tree of Huffman
([Mäkinen and Navarro 2005](Z-bibliography.md#ref-makinen2005)).

--8<-- "generated/HuffmanFMIndex-HuffmanFMIndex.pseudo.md"

--8<-- "generated/HuffmanFMIndex-range.pseudo.md"

--8<-- "generated/HuffmanFMIndex-locate.pseudo.md"

--8<-- "generated/HuffmanFMIndex-extract.pseudo.md"

Also, the samples each $\mu$ places cost $O( ( n / \mu ) \log n )$ bits. This
may be far more than $L$ on a "repetitive" text (the versions of a document,
the genomes of a species), where the number $r$ of the runs of $L$ is far
smaller than $n$. The r-index of [Gagie, Navarro and Prezza
2020](Z-bibliography.md#ref-gagie2020) (`RIndex.h`) keeps $O( r )$ words
instead. $L$ is kept as its runs: the start $p_j$ of each run in an Elias-Fano
sequence ([§22.6](22-succinct.md#sec-22-6)), and the symbol of each run (its
*head* $h_j$) in a wavelet tree of Huffman. A further Elias-Fano sequence holds
the starts of the runs in the column $F$, those of each symbol $c$ after those
of the symbols before it, in one sequence of $r$ items rather than one for each
symbol. Then the rank of $c$ in $L[ 0 .. i - 1 ]$ is

$$
\mathrm{rank}_c( L , i ) = S_c[ k ] + [ h_j = c ] \, ( i - p_j ) ,
\qquad k = \mathrm{rank}_c( h , j ) , \tag{21.22}
$$

where $j$ is the run of the place $i - 1$ (one predecessor search),
$S_c[ k ]$ the total length of the first $k$ runs of $c$ (read from the
starts in $F$, where the run $k$ of $c$ starts at $C[ c ] + S_c[ k ]$), and
$k$ the rank of $c$ among the heads before $j$ (one descent of the tree).
The function `count( P )` is the backward search of (21.21) with it, and
[Table 21.7](21-compression.md#tab-21-7) lists the runs of the example.

<span id="tab-21-7"></span>**Table 21.7.** The $r = 8$ runs of the
transform $L$ = ard\$rcaaaabb of abracadabra, with their first and last
rows, their heads, the samples of $\mathit{SA}$ at their two ends, and the
LF-mapping of their first rows, onto which each run is shifted alike.

| Run $j$ | Rows $[ p_j , e_j ]$ | Head $h_j$ | $\mathit{SA}[ p_j ]$ | $\mathit{SA}[ e_j ]$ | $\mathit{LF}( p_j )$ |
|---|---|---|---|---|---|
| 0 | $[ 0 , 0 ]$ | a | 11 | 11 | 1 |
| 1 | $[ 1 , 1 ]$ | r | 10 | 10 | 10 |
| 2 | $[ 2 , 2 ]$ | d | 7 | 7 | 9 |
| 3 | $[ 3 , 3 ]$ | \$ | 0 | 0 | 0 |
| 4 | $[ 4 , 4 ]$ | r | 3 | 3 | 11 |
| 5 | $[ 5 , 5 ]$ | c | 5 | 5 | 8 |
| 6 | $[ 6 , 9 ]$ | a | 8 | 6 | 2 |
| 7 | $[ 10 , 11 ]$ | b | 9 | 2 | 6 |

To locate the occurrences, two properties of the suffix array on the runs
replace the samples each $\mu$ places.

<span id="lem-21-18"></span>**Lemma 21.18 (the toehold).** *Let
$[ \mathit{first} , \mathit{last} )$ be the rows prefixed by $Q$, and
$v = \mathit{SA}[ \mathit{last} - 1 ]$. If $L[ \mathit{last} - 1 ] = c$,
the last row prefixed by $c Q$ is $\mathit{LF}( \mathit{last} - 1 )$, of
the suffix $v - 1$. Otherwise, if $c Q$ occurs, its last row comes from the
last occurrence of $c$ in $L[ \mathit{first} .. \mathit{last} - 2 ]$,
which ends a run, and its suffix is the sample at the end of that run less
1.*

*Proof.* By [Lemma 21.16](21-compression.md#lem-21-16), the last row
prefixed by $c Q$ is the image of the last row $k$ of the range with
$L[ k ] = c$. Since the LF-mapping moves one place to the left in $s$, the
suffix of the image is $\mathit{SA}[ k ] - 1$. If
$k = \mathit{last} - 1$ this is $v - 1$. Otherwise $L[ k + 1 ] \ne c$,
since either $k + 1 = \mathit{last} - 1$ or $k + 1$ is in the range after
the last $c$; hence $k$ ends a run, whose sample is $\mathit{SA}[ k ]$.
$\square$

Along the backward search, then, we keep in `locate( P )` the suffix of the
last row of the range (the *toehold*), with $O( 1 )$ work per step beyond
the ranks. All the other occurrences follow from the function
$\phi( \mathit{SA}[ i ] ) = \mathit{SA}[ i - 1 ]$, which lists the suffixes
of the range from the last row up.

<span id="lem-21-19"></span>**Lemma 21.19 (the function $\phi$).** *With
$p_j$ the first row of the run $j$,*

$$
\phi( x ) = \phi( y ) + ( x - y ) , \qquad y = \max \{ \mathit{SA}[ p_j ] :
\mathit{SA}[ p_j ] \le x \} , \tag{21.23}
$$

*and $\phi( \mathit{SA}[ p_j ] ) = \mathit{SA}[ p_j - 1 ]$ is the sample at
the end of the run $j - 1$.*

*Proof.* If the row $i$ of $x = \mathit{SA}[ i ]$ starts no run, then $L[ i - 1
] = L[ i ]$, and [Lemma 21.13](21-compression.md#lem-21-13) maps the rows $i -
1$ and $i$ to adjacent rows, those of the suffixes $\phi( x ) - 1$ and $x - 1$.
Hence $\phi( x - 1 ) = \phi( x ) - 1$, i.e., $\phi$ shifts $x - 1$ and $x$
alike. Now, the places $x$ whose row starts a run are the $\mathit{SA}[ p_j ]$,
and (21.23) follows by induction on $x - y$. $\square$

Hence one predecessor search among the $r$ places $\mathit{SA}[ p_j ]$, kept
sorted in an Elias-Fano sequence, gives each step of $\phi$, together with the
number $j$ of the run (in $\lceil \log_2 r \rceil$ bits, cf. (21.23)), whose
sample at the end of the run $j - 1$ is $\phi( \mathit{SA}[ p_j ] )$. On the
example, the search of ab ends on the rows $[ 2 , 4 )$ with the toehold
$\mathit{SA}[ 3 ] = 0$. By (21.23) with $y = 0 = \mathit{SA}[ p_3 ]$, $\phi( 0
)$ is the sample at the end of the run 2, i.e., 7, and the occurrences are 0
and 7. All of it takes $O( r )$ words: 5 sequences of $r$ integers of at most
$\lceil \log_2 n \rceil$ bits, and the heads, whose rank is that of
`CompactRankSelect`, the smallest ([Algorithm
21.28](21-compression.md#alg-21-28)). We build the transform from the suffix
array, in $O( n )$ time and words, while in the literature the r-index is built
in $O( r )$ words from the run-length transform, which `RIndex` does not
attempt.

<span id="alg-21-28"></span>**Algorithm 21.28.** The construction of the
r-index, the backward search on the runs, and the location by the toehold
and $\phi$ ([Gagie, Navarro and Prezza 2020](Z-bibliography.md#ref-gagie2020)).

--8<-- "generated/RIndex-RIndex.pseudo.md"

--8<-- "generated/RIndex-range.pseudo.md"

--8<-- "generated/RIndex-locate.pseudo.md"

Both the LF-mapping on the runs and $\phi$ on the places shift each interval of
a partition alike. Indeed, the LF-mapping shifts the run $j$ onto the rows from
$\mathit{LF}( p_j )$ (cf. [Table 21.7](21-compression.md#tab-21-7)), and $\phi$
the stretch of places from $\mathit{SA}[ p_j ]$ onto the places from
$\mathit{SA}[ p_j - 1 ]$. The move structures of [Nishimoto and Tabei
2021](Z-bibliography.md#ref-nishimoto2021) ([§22.9](22-succinct.md#sec-22-9))
split such intervals, and the interval of the image of a place is then found
from that of the place in $O( 1 )$ steps. `MoveRIndex.h`, after the move-r of
[Bertram, Fischer and Nalbach 2024](Z-bibliography.md#ref-bertram2024), keeps
the LF-mapping as a move structure on the runs, and $\phi$, taken cyclically
($\phi( \mathit{SA}[ 0 ] ) = \mathit{SA}[ n ]$), as a move structure on the
places. This removes the predecessor searches, at the price of a few words per
run, and may therefore be somewhat larger.

The backward search of `MoveRIndex` keeps the first and the last row of the
range, $s$ and $e$, with their intervals. For the symbol $c$ it moves $s$ to
the first row at or after it whose symbol is $c$ (the start of the next
interval of $c$, if its own is not of $c$), and $e$ to the last one at or
before it (the end of the previous interval of $c$). Both moves are a rank and
a select of $c$ on the tags of the intervals, kept in a wavelet tree of Huffman
on `RankSelect`; the search then maps both rows by the LF-mapping, in $O( 1 )$
time each. The function `locate( P )` keeps $\mathit{SA}[ e ]$ along the
search, from the samples of $\mathit{SA}$ at the end of each interval with the
interval of $\phi$ of each. It then lists the occurrences by $e - s$ steps of
$\phi$, each in $O( 1 )$ time ([Algorithm 21.29](21-compression.md#alg-21-29)).

<span id="alg-21-29"></span>**Algorithm 21.29.** The construction of the
r-index on move structures, its count and its location
([Bertram, Fischer and Nalbach 2024](Z-bibliography.md#ref-bertram2024)).

--8<-- "generated/MoveRIndex-MoveRIndex.pseudo.md"

--8<-- "generated/MoveRIndex-count.pseudo.md"

--8<-- "generated/MoveRIndex-locate.pseudo.md"

The backward search of `MoveRIndex` gives the same ranges as that of
`RIndex`, with a constant number of moves per symbol.

<span id="lem-21-20"></span>**Lemma 21.20.** *For a pattern $P$ of
$p \ge 1$ symbols, `count( P )` returns the number of the occurrences of
$P$ in $s$, after at most $2 p$ moves of the LF-mapping and, for each
symbol $c$ of $P$ whose tag differs from that of $i_s$ (or $i_e$), one
rank and one select of $c$ on the tags. `locate( P )` returns their places
in increasing order, after the same search, $\mathit{occ} - 1$ moves of
$\phi$ and a sort; each move takes $O( 1 )$ time.*

*Proof.* The rows prefixed by the suffix $Q$ of $P$ already read are
$[ s , e ]$ (at the start all the rows $[ 0 , n ]$). Each interval of the
LF-mapping lies inside a run of $L$, and carries its symbol as its tag;
hence the first row at or after $s$ with $L = c$ is $s$ if the tag of
$i_s$ is $c$, and otherwise the start of the next interval of tag $c$,
found by a rank and a select on the tags, and symmetrically for $e$. If
these two rows cross, $c Q$ does not occur; otherwise, by
[Lemma 21.16](21-compression.md#lem-21-16), their images are the first and
the last row prefixed by $c Q$. For `locate()`, the sample of $e$ follows
[Lemma 21.18](21-compression.md#lem-21-18), each interval keeping the
sample $\mathit{SA}$ of its last row with the interval of $\phi$ of the
place before, and the other occurrences follow by
[Lemma 21.19](21-compression.md#lem-21-19), $\phi$ shifting alike the
places between two samples of the starts of the runs. Both move
structures are balanced, and each move takes at most 3 steps by
[Lemma 22.19](22-succinct.md#lem-22-19). $\square$

We compare the four indexes in [Table 21.8](21-compression.md#tab-21-8). Their
space and speed, measured on a common set of texts, belong to the campaign
of benchmarks of Appendix B, and are not given here.

<span id="tab-21-8"></span>**Table 21.8.** The indexes of the area on a text
of $n$ bytes, for a pattern of $p$ symbols with $\mathit{occ}$
occurrences.

| Index | Space | `count( P )` | `locate( P )`, beyond the count |
|---|---|---|---|
| `FMIndex` | $8 n + 32 \sigma n / b$ bits, and the samples | $O( p b )$, a count each $b$ places and a scan | $O( \mathit{occ} \, \mu b )$ |
| `HuffmanFMIndex` | $n ( H_0 + 1 ) ( 1 + o( 1 ) )$ bits, and the samples | $O( p ( H_0 + 1 ) )$ on average, a descent of the tree per step | $O( \mathit{occ} \, \mu ( H_0 + 1 ) )$ on average |
| `RIndex` | $O( r )$ words | $O( p )$ ranks on the runs, each with a predecessor search | $O( \mathit{occ} )$ predecessor searches |
| `MoveRIndex` | $O( r )$ words, a few more per run | $O( p )$ steps of $O( 1 )$, plus the ranks for the intervals of another symbol | $O( \mathit{occ} )$, each in $O( 1 )$ |

## 21.12 The interface {#sec-21-12}

We collect here, header by header, the declarations of the namespace `ADS`
that a user calls in this chapter, without the contracts, the documentation
and the members that only the implementation uses; each block points to the
section that describes its functions, and the example at the end shows the
main structures at work. The program of the example compiles against the
headers as it stands, and its assertions hold.

`ArithmeticCoding.h` ([§21.4](21-compression.md#sec-21-4)).

```cpp
// declarations of <ADS/compression/ArithmeticCoding.h>
template< class C >
std::pair< Rational , Rational >
arithmeticInterval( std::span< const C > text ,
                    const std::map< C , Rational > & prob );
template< class C >
std::string arithmeticCodeExact( std::span< const C > text ,
                                 const std::map< C , Rational > & prob );
template< class C >
std::vector< C > arithmeticDecodeExact( const std::string & bits ,
                                        std::size_t n ,
                                        const std::map< C ,
                                                        Rational > & prob );
class RangeEncoder
{
 public:
 explicit RangeEncoder( int w = 32 );
 std::uint64_t maxTotal( void ) const;
 void encode( std::uint64_t low , std::uint64_t high , std::uint64_t total );
 std::vector< std::uint8_t > finish( void );
 std::size_t bits( void ) const;
 };
class RangeDecoder
{
 public:
 explicit RangeDecoder( std::span< const std::uint8_t > bytes , int w = 32 );
 std::uint64_t maxTotal( void ) const;
 std::uint64_t target( std::uint64_t total ) const;
 void consume( std::uint64_t low , std::uint64_t high , std::uint64_t total );
 };
template< class C >
std::vector< std::uint8_t >
arithmeticEncode( std::span< const C > text ,
                  const std::map< C , std::uint64_t > & counts , int w = 32 );
template< class C >
std::vector< C >
arithmeticDecode( std::span< const std::uint8_t > code , std::size_t n ,
                  const std::map< C , std::uint64_t > & counts , int w = 32 );
std::vector< std::uint8_t >
adaptiveArithmeticEncode( std::span< const std::uint32_t > x ,
                          std::uint32_t sigma );
std::vector< std::uint32_t >
adaptiveArithmeticDecode( std::span< const std::uint8_t > code ,
                          std::size_t n ,
                          std::uint32_t sigma );
std::vector< std::uint8_t > adaptiveArithmeticEncode( std::string_view s );
std::string adaptiveArithmeticDecode( std::span< const std::uint8_t > code ,
                                      std::size_t n );
```

`BinaryPacking.h` ([§21.8](21-compression.md#sec-21-8)).

```cpp
// declarations of <ADS/compression/BinaryPacking.h>
std::vector< std::uint8_t > binaryPack( std::span< const std::uint32_t > x ,
                                        bool sorted = false );
std::vector< std::uint32_t > binaryUnpack( std::span< const std::uint8_t > in ,
                                           std::size_t n ,
                                           bool sorted = false );
std::vector< std::uint8_t > patchedPack( std::span< const std::uint32_t > x ,
                                         bool sorted = false );
std::vector< std::uint32_t > patchedUnpack( std::span< const std::uint8_t > in ,
                                            std::size_t n ,
                                            bool sorted = false );
```

`BurrowsWheeler.h` ([§21.10](21-compression.md#sec-21-10)).

```cpp
// declarations of <ADS/compression/BurrowsWheeler.h>
struct BWT
{
 std::string last;
 std::size_t primary;
 };
BWT bwt( std::string_view s , std::size_t wideFrom = bwtWideFrom );
std::vector< std::size_t > lfMapping( const BWT & b );
std::string inverseBwt( const BWT & b );
std::vector< std::uint8_t > moveToFront( std::string_view s ,
                                         std::string_view alphabet );
std::vector< std::uint8_t > moveToFront( std::string_view s );
std::string inverseMoveToFront( const std::vector< std::uint8_t > & x ,
                                std::string_view alphabet );
std::string inverseMoveToFront( const std::vector< std::uint8_t > & x );
std::vector< std::pair< std::size_t , char > >
runLengthEncode( std::string_view s );
std::string
runLengthDecode( const std::vector< std::pair< std::size_t , char > > & R );
std::vector< std::uint32_t >
zeroRunEncode( const std::vector< std::uint8_t > & x );
std::vector< std::uint8_t >
zeroRunDecode( const std::vector< std::uint32_t > & y );
std::vector< std::uint8_t > bzipCompress( std::string_view s ,
                                          std::size_t block = 900000 );
std::string bzipDecompress( std::span< const std::uint8_t > code );
```

`CanonicalHuffman.h` ([§21.2](21-compression.md#sec-21-2)).

```cpp
// declarations of <ADS/compression/CanonicalHuffman.h>
template< class C , class F >
std::map< C , int > huffmanLengths( const std::map< C , F > & freq );
template< class C , class F >
std::map< C , int > limitedLengths( const std::map< C , F > & freq , int L );
template< class C >
class CanonicalHuffman
{
 public:
 using symbol_type = C;
 explicit CanonicalHuffman( const std::map< C , int > & lengths );
 template< class F >
 static CanonicalHuffman fromFrequencies( const std::map< C , F > & freq );
 const std::map< C , int > & lengths( void ) const;
 int maxLength( void ) const;
 std::uint64_t firstCode( int l ) const;
 std::string code( const C & c ) const;
 void encode( const C & c , PackedBitWriter & w ) const;
 std::vector< std::uint8_t > encode( std::span< const C > text ) const;
 C decode( PackedBitReader & r ) const;
 std::vector< C > decode( const std::vector< std::uint8_t > & bits ,
                          std::size_t n ) const;
 };
```

`DirectAccessCodes.h` ([§21.8](21-compression.md#sec-21-8)).

```cpp
// declarations of <ADS/compression/DirectAccessCodes.h>
class DirectAccessCodes
{
 public:
 DirectAccessCodes( void ) = default;
 enum class Levels : int {
  UNCAPPED = 0 ,
  DEFAULT = 3
  };
 explicit DirectAccessCodes( std::span< const std::uint64_t > x ,
                             Levels L = Levels::DEFAULT );
 DirectAccessCodes( std::span< const std::uint64_t > x , int b );
 std::size_t size( void ) const;
 bool empty( void ) const;
 std::uint64_t access( std::size_t i ) const;
 std::uint64_t operator[]( std::size_t i ) const;
 std::vector< int > widths( void ) const;
 std::size_t sizeInBits( void ) const;
 using const_iterator = SubscriptIterator< DirectAccessCodes , std::uint64_t >;
 const_iterator begin( void ) const;
 const_iterator end( void ) const;
 };
```

`Entropy.h` ([§21.1](21-compression.md#sec-21-1)).

```cpp
// declarations of <ADS/compression/Entropy.h>
template< class C , class F >
double entropy( const std::map< C , F > & freq );
double entropy( std::string_view s );
double entropy( std::string_view s , std::size_t k );
```

`FMIndex.h` ([§21.11](21-compression.md#sec-21-11)).

```cpp
// declarations of <ADS/compression/FMIndex.h>
class FMIndex
{
 public:
 explicit FMIndex( std::string_view s , std::size_t b = 64 ,
                   std::size_t mu = 32 );
 std::size_t size( void ) const;
 BWT transform( void ) const;
 std::size_t rank( char c , std::size_t k ) const;
 std::pair< std::size_t , std::size_t > range( std::string_view P ) const;
 std::size_t count( std::string_view P ) const;
 std::vector< std::size_t > locate( std::string_view P ) const;
 std::string extract( std::size_t i , std::size_t j ) const;
 std::size_t sizeInBits( void ) const;
 };
```

`HuffmanCoder.h` ([§21.3](21-compression.md#sec-21-3)).

```cpp
// declarations of <ADS/compression/HuffmanCoder.h>
std::vector< std::uint8_t >
huffmanCompress( std::span< const std::uint8_t > in );
std::vector< std::uint8_t >
huffmanDecompress( std::span< const std::uint8_t > in );
std::array< std::uint8_t , 256 >
limitedHuffmanLengths( const std::array< std::uint64_t , 256 > & f , int L );
```

`HuffmanFMIndex.h` ([§21.11](21-compression.md#sec-21-11)).

```cpp
// declarations of <ADS/compression/HuffmanFMIndex.h>
template< class RS = RankSelect >
class HuffmanFMIndex
{
 public:
 explicit HuffmanFMIndex( std::string_view s , std::size_t mu = 32 );
 std::size_t size( void ) const;
 std::pair< std::size_t , std::size_t > range( std::string_view P ) const;
 std::size_t count( std::string_view P ) const;
 std::vector< std::size_t > locate( std::string_view P ) const;
 std::string extract( std::size_t i , std::size_t j ) const;
 std::size_t sizeInBits( void ) const;
 };
```

`IntegerCodes.h` ([§21.7](21-compression.md#sec-21-7)).

```cpp
// declarations of <ADS/compression/IntegerCodes.h>
constexpr int binaryLength( std::uint64_t x );
constexpr std::uint64_t signedToNatural( std::int64_t x );
constexpr std::int64_t naturalToSigned( std::uint64_t y );
std::vector< std::uint64_t > dGaps( const std::vector< std::uint64_t > & x );
std::vector< std::uint64_t >
prefixSums( const std::vector< std::uint64_t > & g );
void encodeUnary( BitWriter & w , std::uint64_t x );
std::uint64_t decodeUnary( BitReader & r );
constexpr std::uint64_t unaryLength( std::uint64_t x );
void encodeGamma( BitWriter & w , std::uint64_t x );
std::uint64_t decodeGamma( BitReader & r );
constexpr int gammaLength( std::uint64_t x );
void encodeDelta( BitWriter & w , std::uint64_t x );
std::uint64_t decodeDelta( BitReader & r );
constexpr int deltaLength( std::uint64_t x );
void encodeRice( BitWriter & w , std::uint64_t x , int k );
std::uint64_t decodeRice( BitReader & r , int k );
constexpr std::uint64_t riceLength( std::uint64_t x , int k );
void encodeGolomb( BitWriter & w , std::uint64_t x , std::uint64_t b );
std::uint64_t decodeGolomb( BitReader & r , std::uint64_t b );
constexpr std::uint64_t golombLength( std::uint64_t x , std::uint64_t b );
struct PForDeltaBlock
{
 int b = 1;
 std::uint64_t base = 0;
 BitVector slots;
 std::vector< std::uint64_t > exceptions;
 std::size_t size( void ) const;
 std::size_t codeBits( void ) const;
 };
int pforDeltaWidth( const std::vector< std::uint64_t > & x ,
                    std::uint64_t base ,
                    double f = 0.9 );
PForDeltaBlock encodePForDelta( const std::vector< std::uint64_t > & x ,
                                int b ,
                                std::uint64_t base = 0 );
std::vector< std::uint64_t > decodePForDelta( const PForDeltaBlock & p );
void encodeVByte( std::vector< std::uint8_t > & out , std::uint64_t x );
std::uint64_t decodeVByte( const std::vector< std::uint8_t > & in ,
                           std::size_t & p );
constexpr int vbyteLength( std::uint64_t x );
void encodeDense( std::vector< std::uint32_t > & out , std::uint64_t x ,
                  std::uint32_t s , std::uint32_t c );
std::uint64_t decodeDense( const std::vector< std::uint32_t > & in ,
                           std::size_t & p , std::uint32_t s ,
                           std::uint32_t c );
constexpr int denseLength( std::uint64_t x , std::uint32_t s ,
                           std::uint32_t c );
std::uint32_t denseOptimalS( const std::vector< std::uint64_t > & f ,
                             int bits = 8 );
void encodeBounded( BitWriter & w , std::uint64_t x , std::uint64_t a ,
                    std::uint64_t b );
std::uint64_t decodeBounded( BitReader & r , std::uint64_t a ,
                             std::uint64_t b );
void encodeInterpolative( BitWriter & w ,
                          const std::vector< std::uint64_t > & x ,
                          std::uint64_t low , std::uint64_t hi );
std::vector< std::uint64_t > decodeInterpolative( BitReader & r ,
                                                  std::size_t n ,
                                                  std::uint64_t low ,
                                                  std::uint64_t hi );
```

`LempelZiv.h` ([§21.9](21-compression.md#sec-21-9)).

```cpp
// declarations of <ADS/compression/LempelZiv.h>
struct LZ77Phrase
{
 std::size_t distance , length;
 std::optional< char > next;
 };
struct LZSSPhrase
{
 std::size_t distance , length;
 char literal;
 };
struct LZ78Phrase
{
 std::size_t id;
 char next;
 };
std::vector< LZ77Phrase >
lz77Parse( std::string_view s ,
           std::size_t w = std::numeric_limits< std::size_t >::max() );
std::string lz77Decode( const std::vector< LZ77Phrase > & P );
std::vector< LZSSPhrase >
lzssParse( std::string_view s ,
           std::size_t w = std::numeric_limits< std::size_t >::max() ,
           std::size_t minLength = 1 );
std::vector< LZSSPhrase > lzssParseHashed(
  std::string_view s , std::size_t w , std::size_t minLength = 3 ,
  std::size_t maxChain = std::numeric_limits< std::size_t >::max() );
std::string lzssDecode( const std::vector< LZSSPhrase > & P );
std::vector< LZ78Phrase > lz78Parse(
  std::string_view s ,
  std::size_t maxPhrases = std::numeric_limits< std::size_t >::max() );
std::string lz78Decode(
  const std::vector< LZ78Phrase > & P ,
  std::size_t maxPhrases = std::numeric_limits< std::size_t >::max() );
std::vector< std::uint32_t > lzwEncode( std::string_view s );
std::string lzwDecode( const std::vector< std::uint32_t > & I );
```

`LZ77Linear.h` ([§21.9](21-compression.md#sec-21-9)).

```cpp
// declarations of <ADS/compression/LZ77Linear.h>
std::vector< LZ77Phrase > lz77ParseLinear( std::string_view s );
```

`LZBicriteria.h` ([§21.9](21-compression.md#sec-21-9)).

```cpp
// declarations of <ADS/compression/LZBicriteria.h>
enum class LZBicriteriaGoal
{
 time ,
 space
 };
constexpr std::size_t lzBicriteriaMaxLiterals = 64;
struct LZBicriteriaModel
{
 std::uint64_t phrase = 8;
 std::uint64_t byte = 1;
 std::array< std::uint64_t , 5 > distance = { 0 , 0 , 20 , 100 , 200 };
 std::size_t candidates = 64;
 std::size_t iterations = 128;
 };
struct LZBicriteriaPhrase
{
 std::size_t distance , length;
 };
struct LZBicriteriaParse
{
 std::vector< LZBicriteriaPhrase > phrases;
 std::uint64_t space = 0;
 std::uint64_t time = 0;
 double lowerBound = 0;
 double multiplier = 0;
 double gapBound = 0;
 bool feasible = true;
 };
LZBicriteriaParse
lzBicriteriaParse( std::string_view s , LZBicriteriaGoal goal ,
                   std::uint64_t bound , const LZBicriteriaModel & m = {} ,
                   std::uint64_t slack = 0 );
std::uint64_t
lzBicriteriaTime( const std::vector< LZBicriteriaPhrase > & P ,
                  const LZBicriteriaModel & m = {} );
std::vector< std::uint8_t >
lzBicriteriaEncode( std::string_view s ,
                    const std::vector< LZBicriteriaPhrase > & P );
std::size_t lzBicriteriaSize( const std::vector< std::uint8_t > & C );
std::size_t lzBicriteriaDecode( const std::vector< std::uint8_t > & C ,
                                std::span< char > out );
std::string lzBicriteriaDecode( const std::vector< std::uint8_t > & C );
```

`MoveRIndex.h` ([§21.11](21-compression.md#sec-21-11)).

```cpp
// declarations of <ADS/compression/MoveRIndex.h>
class MoveRIndex
{
 public:
 MoveRIndex( void );
 explicit MoveRIndex( std::string_view s );
 std::size_t size( void ) const;
 std::size_t intervals( void ) const;
 std::size_t count( std::string_view P ) const;
 std::vector< std::size_t > locate( std::string_view P ) const;
 std::size_t sizeInBits( void ) const;
 };
```

`PackedBits.h` ([§21.2](21-compression.md#sec-21-2)).

```cpp
// declarations of <ADS/compression/PackedBits.h>
class PackedBitWriter
{
 public:
 void put( bool b );
 void put( std::uint64_t v , int k );
 void put( const std::string & s );
 std::size_t size( void ) const;
 const std::vector< std::uint8_t > & bytes( void ) const;
 };
class PackedBitReader
{
 public:
 explicit PackedBitReader( std::span< const std::uint8_t > b );
 bool get( void );
 std::uint64_t get( int k );
 std::size_t position( void ) const;
 std::size_t size( void ) const;
 };
```

`PPM.h` ([§21.6](21-compression.md#sec-21-6)).

```cpp
// declarations of <ADS/compression/PPM.h>
class PPMModel
{
 public:
 struct Step
 {
  std::uint64_t low , high , total;
  };
 PPMModel( int order , std::uint32_t sigma = 256 );
 int order( void ) const;
 std::uint32_t sigma( void ) const;
 std::map< std::uint32_t , std::uint64_t >
 counts( const std::vector< std::uint32_t > & ctx ) const;
 std::vector< Step > steps( std::uint32_t c ) const;
 void encode( std::uint32_t c , RangeEncoder & e ) const;
 std::uint32_t decode( RangeDecoder & d ) const;
 void update( std::uint32_t c );
 };
std::vector< std::uint8_t > ppmEncode( std::string_view s , int K );
std::string ppmDecode( std::span< const std::uint8_t > code , std::size_t n ,
                       int K );
```

`Rans.h` ([§21.5](21-compression.md#sec-21-5)).

```cpp
// declarations of <ADS/compression/Rans.h>
std::vector< std::uint8_t > ransCompress( std::span< const std::uint8_t > in );
std::vector< std::uint8_t >
ransDecompress( std::span< const std::uint8_t > in );
std::array< std::uint32_t , 256 >
scaleFrequencies( const std::array< std::uint64_t , 256 > & f , int b );
```

`RIndex.h` ([§21.11](21-compression.md#sec-21-11)).

```cpp
// declarations of <ADS/compression/RIndex.h>
class RIndex
{
 public:
 RIndex( void );
 explicit RIndex( std::string_view s );
 std::size_t size( void ) const;
 std::size_t runs( void ) const;
 std::pair< std::size_t , std::size_t > range( std::string_view P ) const;
 std::size_t count( std::string_view P ) const;
 std::vector< std::size_t > locate( std::string_view P ) const;
 std::size_t sizeInBits( void ) const;
 };
```

`StreamVByte.h` ([§21.8](21-compression.md#sec-21-8)).

```cpp
// declarations of <ADS/compression/StreamVByte.h>
std::vector< std::uint8_t >
streamVByteEncode( std::span< const std::uint32_t > x );
std::vector< std::uint32_t >
streamVByteDecode( std::span< const std::uint8_t > code , std::size_t n );
```

`Tans.h` ([§21.5](21-compression.md#sec-21-5)).

```cpp
// declarations of <ADS/compression/Tans.h>
std::vector< std::uint8_t > tansCompress( std::span< const std::uint8_t > in );
std::vector< std::uint8_t >
tansDecompress( std::span< const std::uint8_t > in );
```


```cpp
#include "ADS/compression/CanonicalHuffman.h"
#include "ADS/compression/FMIndex.h"
#include "ADS/compression/Rans.h"
#include <cassert>
#include <string>

int main( void )
{
 const std::string s = "abracadabra";
 std::map< char , int > freq;
 for( char c : s )
  ++freq[ c ];
 const auto code = ADS::CanonicalHuffman< char >::fromFrequencies( freq );
 const std::vector< char > text( s.begin() , s.end() );
 const auto bits = code.encode( std::span< const char >( text ) );
 assert( code.decode( bits , text.size() ) == text );
 const std::span< const std::uint8_t > bytes(
   reinterpret_cast< const std::uint8_t * >( s.data() ) , s.size() );
 const auto z = ADS::ransCompress( bytes );
 const auto back = ADS::ransDecompress( z );
 assert( std::string( back.begin() , back.end() ) == s );
 const ADS::FMIndex fm( s );
 assert( ( fm.count( "abra" ) == 2 ) && ( fm.extract( 0 , 4 ) == "abra" ) );
 }
```

## 21.13 Discussion and references {#sec-21-13}

The entropy and the bound of [Lemma 21.1](21-compression.md#lem-21-1) are those
of [Shannon 1948](Z-bibliography.md#ref-shannon1948), the optimal code of the
symbols one by one is that of [Huffman
1952](Z-bibliography.md#ref-huffman1952), and the canonical code that of
[Schwartz and Kallick 1964](Z-bibliography.md#ref-schwartz1964). We follow the
decoders by tables of [Moffat and Turpin
1997](Z-bibliography.md#ref-moffat1997); the textbook of [Witten, Moffat and
Bell 1999](Z-bibliography.md#ref-witten1999) treats them, together with the
canonical code of the words of a text (the Huffword) and the indexes by
inverted lists. In integers, the arithmetic code is that of [Witten, Neal and
Cleary 1987](Z-bibliography.md#ref-witten1987), the range coder that of [Martin
1979](Z-bibliography.md#ref-martin1979), and the asymmetric numeral systems are
due to [Duda 2013](Z-bibliography.md#ref-duda2013); see also [Duda et al.
2015](Z-bibliography.md#ref-duda2015) for their use in place of the code of
Huffman. Models of context go back to [Cleary and Witten
1984](Z-bibliography.md#ref-cleary1984), with the escape methods recalled in
[§21.6](21-compression.md#sec-21-6). As for the integers, the codes are those
of [Elias 1975](Z-bibliography.md#ref-elias1975), [Golomb
1966](Z-bibliography.md#ref-golomb1966) and [Rice
1979](Z-bibliography.md#ref-rice1979), and the survey of [Pibiri and Venturini
2020](Z-bibliography.md#ref-pibiri2020) compares them with the codes by blocks
on the lists of the inverted indexes.

The dictionary compressors are those of [Ziv and Lempel
1977](Z-bibliography.md#ref-ziv1977) and [1978](Z-bibliography.md#ref-ziv1978),
and the linear parses of LZ77 were also studied by [Chen, Puglisi and Smyth
2008](Z-bibliography.md#ref-chen2008). The transform of [Burrows and Wheeler
1994](Z-bibliography.md#ref-burrows1994) became an index with the FM-index of
[Ferragina and Manzini 2000](Z-bibliography.md#ref-ferragina2000) and
[2005](Z-bibliography.md#ref-ferragina2005a). Two surveys, by [Navarro and
Mäkinen 2007](Z-bibliography.md#ref-navarro2007) and by [Navarro
2021](Z-bibliography.md#ref-navarro2021), cover the compressed indexes and
those of the "repetitive" texts, the latter also the r-index of [Gagie, Navarro
and Prezza 2018](Z-bibliography.md#ref-gagie2018), and section 3.4.2 of
[Navarro 2016](Z-bibliography.md#ref-navarro2016) treats the directly
addressable codes. Chapters 11 to 14 of [Ferragina
2023](Z-bibliography.md#ref-ferragina2023) are the source of the algorithms of
the book. This chapter departs from them where the code departs, as remarked in
place: the ceiling of (21.5), the leading 1 kept by the code $\delta$, the
order of the bits of the runs of RLE0, and the single table of bzip.

All the engineered coders of the chapter share one design. They move the
lengths (of a codeword, of an integer) out of the data, or bound them, and the
decoder then runs a "fixed" sequence of operations on whole words; also, they
interleave "independent" streams, so that the chains of dependences of the
decoder overlap. How much each of these choices may gain, against the plain
forms of the book and against the implementations of reference, is measured by
the campaign of Appendix B, and no number is given here.

The compressed indexes of [§21.11](21-compression.md#sec-21-11) (`FMIndex`,
`HuffmanFMIndex`, `RIndex` and `MoveRIndex`) are persistent
([§23.7](23-external.md#sec-23-7)). An index of a text or of a collection of
genomes, whose construction costs a suffix array, may thus be written once
and then opened in place by `restoreMapped()`, with its vectors of bits and
its wavelet trees as views over the mapping. We remark that an r-index read
from a file has no copy of its text; under the contracts of level 3, the
audit of `locate()` against the text is then skipped.

## 21.14 Exercises {#sec-21-14}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Compute the empirical entropy of order 0 and of order $k$ of a string,
   and check that $H_k \le H_0$ on random strings. Test:
   `pearls_compression.entropy`.
2. Build the canonical code of given lengths, with the first codewords
   (21.5) rounded up, and decode a codeword by the values
   $\mathit{fc}[ l ]$; check it on the lengths of
   [Figure 21.1](21-compression.md#fig-21-1). Tests:
   `pearls_compression.canonical-huffman-notes`,
   `pearls_compression.canonical-huffman`.
3. Bound the lengths of a code of Huffman by the iterative halving of the
   frequencies, and prove that it ends
   ([Lemma 21.3](21-compression.md#lem-21-3)). Test:
   `pearls_compression.limited-lengths`.
4. Code a string of bytes with the optimal lengths of at most 11 bits found
   by package-merge, and decode it by a table of up to 3 symbols a lookup
   on four interleaved streams. Test: `engineered_succinct.huffman-coder`.
5. Compute the arithmetic code of abac in exact arithmetic
   ([Figure 21.3](21-compression.md#fig-21-3)), and check that its length
   is within 2 bits of $n H_0$ on random texts. Test:
   `pearls_compression.arithmetic-exact`.
6. Realize the integer arithmetic coder with the underflows, prove
   [Lemma 21.4](21-compression.md#lem-21-4), and code a text with a static
   model and with the adaptive model on a tree of Fenwick. Tests:
   `pearls_compression.range-coder-notes`,
   `pearls_compression.arithmetic-static`,
   `pearls_compression.arithmetic-adaptive`.
7. Realize rANS with four interleaved states and the renormalization of
   [Lemma 21.5](21-compression.md#lem-21-5), and make the decoder stop on
   a corrupted input instead of reading out of bounds. Tests:
   `engineered_succinct.rans-coder`, `engineered_succinct.rans-corrupted`.
8. Realize tANS with the spread of the states by the step
   $L / 2 + L / 8 + 3$ and two halves of four states each. Test:
   `engineered_succinct.tans-coder`.
9. Realize PPM with exclusion, version C, and follow the steps of
   [Table 21.5](21-compression.md#tab-21-5). Tests:
   `pearls_compression.ppm-notes`, `pearls_compression.ppm`.
10. Write the codes unary, $\gamma$, $\delta$, Rice, Golomb, PForDelta,
    variable byte, $( s , c )$-dense and interpolative, each with its
    length function, and check them on the codewords of
    [Table 21.6](21-compression.md#tab-21-6). Tests:
    `pearls_succinct.elias-codes`, `pearls_succinct.rice-golomb`,
    `pearls_succinct.pfordelta`, `pearls_succinct.vbyte-dense`,
    `pearls_succinct.interpolative`, `pearls_succinct.signed-gaps`.
11. Pack blocks of 128 integers vertically in four lanes, with the
    differences at a distance of 4 for the sorted sequences, and with the
    width of least cost (21.14) of the patched code; check that the scalar
    layout is the same as the vectorial one. Tests:
    `engineered_succinct.binary-packing`,
    `succinct_top.binary-packing-scalar`.
12. Realize Stream VByte, with the lengths in control bytes and a decoder
    that moves four integers by one shuffle. Test:
    `engineered_succinct.stream-vbyte`.
13. Realize the directly addressable codes with the widths of (21.17), for any
    bound $h$ on the levels, and access an item in time proportional to its
    length. Test: `engineered_succinct.direct-access-codes`.
14. Parse a text by LZ77 (with and without a window), LZSS (by a search of
    the window and by the chains of the hashes of gzip), LZ78 and LZW, and
    decode each parse. Tests: `pearls_compression.lz77-notes`,
    `pearls_compression.lz77`, `pearls_compression.lz78`,
    `pearls_compression.lzw`.
15. Compute the parse of LZ77 with no window in $O( n )$ time by the
    previous and the next smaller values of the suffix array
    ([Theorem 21.3](21-compression.md#thm-21-3)). Test:
    `engineered_succinct.lz77-linear`.
16. Find the parse of LZ77 of least decompression time within a bound on the
    size of its byte-oriented code, and vice versa, by the cutting planes on
    the Lagrangian relaxation and the path swap, and check it against the
    gap of [Theorem 21.Zlz4](21-compression.md#thm-21-Zlz4) and, on small texts,
    against the optimum of a dynamic program. Test:
    `engineered_succinct.lz-bicriteria`.
17. Compute the transform of Burrows and Wheeler and invert it by the
    LF-mapping; chain the move-to-front, RLE0 and a canonical code of
    Huffman into a bzip. Tests: `pearls_compression.bwt-notes`,
    `pearls_compression.bwt`, `pearls_compression.mtf-rle`,
    `pearls_compression.bzip`.
18. Count, locate and extract by the FM-index, and follow the backward
    search of [Figure 21.8](21-compression.md#fig-21-8). Tests:
    `pearls_compression.fm-index-notes`, `pearls_compression.fm-index`.
19. Realize the FM-index on the wavelet tree of Huffman of $L$, with the
    LF-mapping of a step of `locate()` in one descent. Test:
    `engineered_succinct.huffman-fm-index`.
20. Realize the r-index, with the rank on the runs (21.22), the toehold
    and the function $\phi$ ([Lemma 21.18](21-compression.md#lem-21-18)
    and [Lemma 21.19](21-compression.md#lem-21-19)), and then replace its
    predecessor searches with move structures. Tests:
    `engineered_succinct.r-index`, `engineered_succinct.move-r-index`.
