# 18. Combinatorial generation {#ch-18}

[Source: `include/ADS/combinatorial/Bitwise.h`, `FastBitwise.h`,
`Generation.h`, `FastGeneration.h`]

We are concerned in this chapter with two families of algorithms of volume
4A of [Knuth 2011](Z-bibliography.md#ref-knuth2011), which go beyond the
textbook course that the rest of the library follows. The first one is the
*broadword* computation of his section 7.1.3, which treats the 64 bits of a
word as a vector of bits (or as a set of at most 64 items) and replaces a
loop over the bits by a few operations on the word
([§18.2](18-combinatorial.md#sec-18-2)); its engineered form works on
arrays of words and on the instructions of the recent processors
([§18.3](18-combinatorial.md#sec-18-3)). The second one is the
*generation* of the basic combinatorial objects of his section 7.2.1 (the
tuples, the permutations, the combinations, the partitions and the strings
of parentheses), each visited once in a fixed order
([§18.4](18-combinatorial.md#sec-18-4) to
[§18.7](18-combinatorial.md#sec-18-7)). Its engineered form brings the
work per object down to a few instructions
([§18.8](18-combinatorial.md#sec-18-8)). Each family therefore comes in
two headers, one that follows the text of Knuth and one written for speed,
and the tests check that the two give the same results. The enumeration by
backtracking of the subsets, the permutations and the other configurations
with constraints is instead the subject of
[Chapter 19](19-backtrack.md#ch-19).

## 18.1 Words as sets of bits {#sec-18-1}

A *word* $x$ is an unsigned integer of 64 bits (the type `Word` of
`Bitwise.h`), $x = \sum_{k=0}^{63} x_k 2^k$, with the bit $x_k$ in the
*place* $k$; the arithmetic on words is modulo $2^{64}$, and hence
$-x = \lnot x + 1$ (the complement plus 1). A word is also the set
$\{ k : x_k = 1 \} \subseteq \{ 0 , \ldots , 63 \}$, on which the
operations of C++ on the bits are those of the sets: $\&$ is the
intersection, $\mid$ the union, $\oplus$ the symmetric difference and
$\lnot$ the complement, while the shifts $x \gg s = \lfloor x / 2^s
\rfloor$ and $x \ll s = 2^s x \bmod 2^{64}$ translate the set by $s$
places. In the following, $\nu( x )$ is the *sideways sum* of $x$
(the number of its 1 bits, i.e., the size of the set), $\rho( x )$ the
*ruler function* (the place of the lowest 1 of $x \ne 0$, i.e., the number
of its trailing zeros) and $\lambda( x ) = \lfloor \log_2 x \rfloor$ the
place of its highest 1; a constant written $\mathtt{0x5555 \ldots 55}$ is
the word whose 8 bytes are all $\mathtt{0x55}$. For the generation, the
numbers of the objects are written $\binom{n}{t}$ (the binomial
coefficient), $p( n )$ (the partitions of the integer $n$), $\varpi_n$
(the Bell number, i.e., the partitions of a set of $n$ items) and
$C_n = \binom{2 n}{n} / ( n + 1 )$ (the Catalan number).

## 18.2 Broadword computation {#sec-18-2}

We now describe the functions of `Bitwise.h`, which compute a property of
a word, or a new word, with a number of operations that does not depend on
the bits that are set, and with no branch but the few that their contracts
need. Each one rests on an identity of the arithmetic modulo $2^{64}$,
which we state with its proof. The functions are `constexpr`, and hence the
compiler can evaluate them on constants.

**The sideways sum.** Counting the 1 bits one by one costs $\nu( x )$
iterations even with the trick $x \gets x \,\&\, ( x - 1 )$, which clears
the lowest 1 at each step. Instead, `sidewaysSum()` adds the bits in
parallel ([Algorithm 18.1](18-combinatorial.md#alg-18-1)). The word is
seen first as 32 fields of 2 bits, each of which receives the number of its
1 bits, then as 16 fields of 4 bits and as 8 fields of 8 bits (the bytes),
each holding the sum of its two halves; finally, a product adds the 8
bytes into the top one ([Figure 18.1](18-combinatorial.md#fig-18-1)).

<span id="alg-18-1"></span>**Algorithm 18.1.** The sideways sum of a word
([Knuth 2011, §7.1.3](Z-bibliography.md#ref-knuth2011)).

--8<-- "generated/Bitwise-sidewaysSum.pseudo.md"

![Figure 18.1. The sums formed in place by `sidewaysSum()`, on a word of 16 bits for brevity (the places above the row of the word): the fields of 2, 4 and 8 bits, and the product by $\mathtt{0x0101}$, whose top byte (in colour) is the sum of the two bytes.](../figures/combinatorial-sideways.svg){#fig-18-1}

<span id="lem-18-1"></span>**Lemma 18.1.** *For any word $x$,
`sidewaysSum(x)` returns $\nu( x )$, with 12 operations on words and no
branch.*

*Proof.* A field of 2 bits with the value $2 a + b$ has $a + b$ ones, and
$x - ( ( x \gg 1 ) \,\&\, \mathtt{0x5555 \ldots 55} )$ subtracts $a$ from
each field; since $2 a + b \ge a$, no subtraction borrows from the field
above, and line 2 therefore leaves in each field its count. Line 3
adds the fields in pairs into fields of 4 bits, whose sums are at most 4
and fit in 3 bits. Line 4 adds the fields of 4 bits in pairs, and it may
mask after the sum (instead of before, as line 3 does), since a sum is at
most 8 and fits in the lower half of its byte. Finally, the byte $7$ of the product by
$\mathtt{0x0101 \ldots 01}$ is the sum of the 8 bytes, since the product
adds the byte $i$ to the bytes $i , i + 1 , \ldots , 7$ and a sum never
exceeds 64; hence, no carry crosses a byte. $\square$

```cpp
--8<-- "generated/Bitwise-sidewaysSum.cpp"
```

**The lowest and the highest 1.** The lowest 1 of $x \ne 0$ is in the
place $\rho( x )$, and $x - 1$ has the same bits as $x$ above it, a 0 in
it and 1s below it; hence

$$
x \,\&\, -x = 2^{\rho( x )} , \qquad
x \mid ( x - 1 ) = x + 2^{\rho( x )} - 1 , \qquad
x \oplus ( x - 1 ) = 2^{\rho( x ) + 1} - 1 , \tag{18.1}
$$

which are, respectively, `rightmostOne()` (the lowest 1 alone),
`smearRight()` (the 0s below it set) and a run of $\rho( x ) + 1$ ones,
whose sideways sum minus 1 is `ruler()`.

--8<-- "generated/Bitwise-ruler.pseudo.md"

The highest 1 has no such identity. `lambda()` finds it by halving the
width of the search, i.e., by a binary search on its place: if $x \gg 32
\ne 0$ the place is at least 32 and the search goes on in the upper half,
and so on with the shifts 16, 8, 4, 2 and 1 (6 steps in all).

--8<-- "generated/Bitwise-lambda.pseudo.md"

The same doubling reverses the bits of a word (`reverseBits()`). The
fields of 1 bit are swapped in pairs, then those of 2, 4, 8 and 16 bits,
and finally the two halves, in $\log_2 64 = 6$ rounds of two shifts, two
masks and an or each. In general, the functions of this kind cost
$O( \log w )$ operations on a word of $w$ bits (cf. [Knuth 2011,
§7.1.3](Z-bibliography.md#ref-knuth2011)).

--8<-- "generated/Bitwise-reverseBits.pseudo.md"

**The Gray code.** The binary *reflected Gray code* of [Gray
1953](Z-bibliography.md#ref-gray1953) orders the $2^{64}$ words in such a
way that two consecutive ones differ in one bit only. Then `gray()` gives
the $x$-th word of the order, and `grayInverse()` gives the rank of a word.

--8<-- "generated/Bitwise-gray.pseudo.md"

--8<-- "generated/Bitwise-grayInverse.pseudo.md"

<span id="lem-18-2"></span>**Lemma 18.2.** *Let $g( x ) = x \oplus ( x
\gg 1 )$. Then $g( x )$ and $g( x + 1 )$ differ in exactly one bit for
$x < 2^{64} - 1$, and $g$ is a bijection whose inverse gives the bit $k$ of
$x$ as $x_k = \bigoplus_{j \ge k} g_j$; `grayInverse()` computes it with 6
shifts.*

*Proof.* Adding 1 to $x$ flips the run of the trailing 1s of $x$ (say, $r$
bits) and the 0 above it, i.e., $x \oplus ( x + 1 ) = 2^{r + 1} - 1$. Then
$g( x ) \oplus g( x + 1 ) = ( 2^{r + 1} - 1 ) \oplus ( 2^r - 1 ) = 2^r$,
a single bit. Furthermore, since $g_k = x_k \oplus x_{k+1}$ (with $x_{64} = 0$), one has
$x_k = g_k \oplus x_{k+1}$, and unrolling from the top gives the
suffix exclusive or of the statement. Indeed, the loop of `grayInverse()`
computes it by doubling: after the shifts by $1 , 2 , \ldots , 2^{i-1}$ the bit $k$
is the exclusive or of the bits $k , \ldots , k + 2^i - 1$ of $g$, and
$2^6 = 64$ covers the word. $\square$

**The next word with as many 1 bits.** One can visit the $t$-subsets of
$\{ 0 , \ldots , 63 \}$, taken as words, in increasing order by the trick
of Gosper ([Beeler et al. 1972, item
175](Z-bibliography.md#ref-beeler1972)). It computes the successor of a
word of $t$ ones with a constant number of operations
([Algorithm 18.2](18-combinatorial.md#alg-18-2),
[Figure 18.2](18-combinatorial.md#fig-18-2)).

<span id="alg-18-2"></span>**Algorithm 18.2.** The trick of Gosper
([Beeler et al. 1972](Z-bibliography.md#ref-beeler1972); [Knuth 2011,
§7.1.3](Z-bibliography.md#ref-knuth2011)).

--8<-- "generated/Bitwise-nextSameWeight.pseudo.md"

![Figure 18.2. The words of [Algorithm 18.2](18-combinatorial.md#alg-18-2) on $x = 01011100$: the lowest run of 1s of $x$, the bit that the carry sets, and the $r - 1 = 2$ bits that go back to the bottom (in colour).](../figures/combinatorial-gosper.svg){#fig-18-2}

<span id="lem-18-3"></span>**Lemma 18.3.** *For $x \ne 0$, with
$u = x \,\&\, -x$ and $v = x + u$, the next larger word with as many 1 bits
as $x$ is*

$$
v + ( ( ( v \oplus x ) / u ) \gg 2 ) , \tag{18.2}
$$

*provided that it is smaller than $2^{64}$, i.e., that $v \ne 0$.*

*Proof.* Let the lowest run of 1s of $x$ have $r \ge 1$ bits, from the
place $\rho( x )$ up to the place $\rho( x ) + r - 1$. A larger word with
as many 1 bits must change a 0 of $x$ into a 1. The least such word moves
the top bit of that run one place up (to the lowest 0 above a 1), and puts
the other $r - 1$ bits of the run in the places $0 , \ldots , r - 2$. By (18.1), $u = 2^{\rho( x )}$, and the carry
of $x + u$ clears the run and sets the bit above it; hence $v$ has the
bits of $x$ above the run and that bit, and $v \oplus x$ is a run of
$r + 1$ ones from the place $\rho( x )$. The division by $u$ brings it to
the place 0, and the shift by 2 shortens it to $r - 1$ ones, which the sum
adds to $v$ without carries. $\square$

```cpp
--8<-- "generated/Bitwise-nextSameWeight.cpp"
```

Note that the contract on $v$ also catches $x = 2^{63}$, whose successor
would need a 65th bit. The division typically costs a few tens of cycles,
which is why the engineered version of
[§18.8](18-combinatorial.md#sec-18-8) replaces it by a shift.

**The Morton code.** `interleave( x , y )` puts the 32 bits of $x$ in the
even places of a word and those of $y$ in the odd ones, i.e., the *Morton
code* of the point $( x , y )$ ([Morton 1966](Z-bibliography.md#ref-morton1966)),
and `deinterleave()` takes them apart. The order of the codes visits the
plane by squares of side $2^k$ for each $k$
([Figure 18.3](18-combinatorial.md#fig-18-3)). Hence, points close in the
plane often (but not always: see the jump from 7 to 8 in the figure) have
close codes, a property that spatial indexes and the cache-friendly layouts
of matrices exploit. The spread of the 32 bits to the even
places moves blocks of 16, 8, 4, 2 and 1 bits, each by an or with a shifted
copy and a mask, and the gather undoes the steps in the reverse order.

--8<-- "generated/Bitwise-interleave.pseudo.md"

![Figure 18.3. The Morton codes of the points of a grid of $4 \times 4$ (left, in binary, $y_1 x_1 y_0 x_0$) and their order on the grid, from 0 to 15 (right).](../figures/combinatorial-morton.svg){#fig-18-3}

**A zero byte.** `hasZeroByte()` decides whether one of the 8 bytes of a
word is 0 with four operations and a comparison, which is the test that
lets `strlen()` scan a string 8 bytes at a time.

--8<-- "generated/Bitwise-hasZeroByte.pseudo.md"

<span id="lem-18-4"></span>**Lemma 18.4.** *$( ( x - \mathtt{0x0101 \ldots
01} ) \,\&\, \lnot x \,\&\, \mathtt{0x8080 \ldots 80} ) \ne 0$ if and only if
one of the bytes of $x$ is 0.*

*Proof.* If no byte is 0, each byte $b \ge 1$ becomes $b - 1$ with no
borrow, and the top bit of $b - 1$ is set only if $b \ge \mathtt{0x81}$,
in which case the top bit of $b$ is set too and $\lnot x$ masks it. The
result is then 0. Otherwise, let the byte $i$ be the lowest one that is 0:
the bytes below it are not 0, and hence do not borrow, while the byte $i$
becomes $\mathtt{0xFF}$, whose top bit survives both masks. $\square$

The borrow from the byte $i$ may turn a byte above it that is 1 into a
byte whose top bit is set, i.e., a "false" zero byte. This does not change
the answer, since a zero byte exists anyway; yet, the word of the test
locates with certainty only the lowest zero byte.

```cpp
--8<-- "generated/Bitwise-hasZeroByte.cpp"
```

## 18.3 Engineered operations on words and arrays {#sec-18-3}

The processors of the last fifteen years have instructions for some of the
functions of [§18.2](18-combinatorial.md#sec-18-2): the population count
(`popcnt`), the trailing and the leading zeros (`tzcnt`, `lzcnt`), and the
scatter and the gather of bits of BMI2 (`pdep`, `pext`). C++20 reaches the
first ones portably through `std::popcount`, `std::countr_zero` and
`std::countl_zero` of `<bit>`. `FastBitwise.h` builds on them, and on the
vectors of AVX2 and AVX-512 when the compiler may use them, for four tasks:
(i) the count of the 1 bits of an array, (ii) the scatter and the gather,
(iii) the selection of the 1 of a given rank, and (iv) the Morton code.
Each function also has a portable branch, which gives the same results on
a machine without the instructions (the test
`engineered_combinatorial.fast-bitwise` checks them against loops over the
bits).

**The population count of an array.** The 1 bits of $n$ words are the sum
of their sideways sums, i.e., $n$ instructions `popcnt` and $n$ additions.
`popcount( w , n )` keeps four independent sums, and hence the additions
of consecutive words need not wait for one another. This is the algorithm
on a machine without AVX2, and on the arrays shorter than a block (64 words
with AVX2, 128 with AVX-512). On longer arrays it uses the method of Harley
and Seal in the form of [Muła et al. 2018](Z-bibliography.md#ref-mula2018),
which counts once per 16 vectors. A *carry-save adder* takes three
vectors of bits $a$, $b$, $c$
and gives, place by place, two vectors

$$
\mathrm{sum} = a \oplus b \oplus c , \qquad
\mathrm{carry} = \mathrm{maj}( a , b , c ) , \qquad
a + b + c = \mathrm{sum} + 2 \, \mathrm{carry} , \tag{18.3}
$$

where the last identity holds place by place (the majority is 1 when at
least two of the three bits are). A tree of 15 such adders takes a block of 16
vectors $v_0 , \ldots , v_{15}$ together with four accumulators $o$, $t$,
$f$, $e$, whose bits weigh 1, 2, 4 and 8, and gives four new accumulators
and a vector $s$ of the sixteens
([Algorithm 18.3](18-combinatorial.md#alg-18-3)); only $s$ is counted.

<span id="alg-18-3"></span>**Algorithm 18.3.** The population count of an
array by the method of Harley and Seal ([Muła et al.
2018](Z-bibliography.md#ref-mula2018)); $V$ is the number of words of a
vector.

--8<-- "generated/FastBitwise-popcount.pseudo.md"

<span id="lem-18-5"></span>**Lemma 18.5.** *After each block, the 1 bits
of the vectors read so far are $16 T + 8 \nu( e ) + 4 \nu( f ) + 2 \nu( t )
+ \nu( o )$, where $T$ is the sum of the counts $\nu( s )$ of the blocks;
hence [Algorithm 18.3](18-combinatorial.md#alg-18-3) returns the 1 bits of
the array, with one count of a vector and 15 carry-save adders per block of
16 vectors.*

*Proof.* Before the first block all the terms are 0. In a block, each adder replaces three vectors of the same weight $2^i$ with a
vector of weight $2^i$ (the sum) and one of weight $2^{i+1}$ (the carry),
which by (18.3) keeps the weighted number of 1 bits. The adders of lines 5
to 7 take the 16 new vectors and the old $o$, $t$, $f$, $e$, and leave one
vector for each weight 1, 2, 4, 8 (the new accumulators) and one, $s$, of
weight 16, whose count enters $T$. Line 9 then adds the accumulators with
their weights, and the vectors and words left after the last block with
weight 1. $\square$

An adder costs five logical operations on AVX2 (two exclusive ors, two ands
and an or) and two on AVX-512, whose instruction `vpternlogq` computes any
function of three vectors of bits (the truth tables $\mathtt{0x96}$ for the
sum and $\mathtt{0xE8}$ for the majority):

```cpp
--8<-- "generated/FastBitwise-csa256.cpp"
```

A vector is counted by the instruction `vpopcntq` of AVX-512 when the
processor has it. Otherwise, the method of [Muła et al.
2018](Z-bibliography.md#ref-mula2018) keeps the 16 counts of the nibbles as
a table of bytes in a register, looks up the counts of the two nibbles of
each byte by a shuffle of bytes (`vpshufb`), and adds the counts of the 8
bytes of each word by `vpsadbw`. With AVX2, a block of 16 vectors (64
words) thus costs 75 logical operations for the adders and about 10 for the
count of $s$, against 64 instructions `popcnt` and 64 additions for the
scalar loop. Whether this is faster may depend on the processor (it usually is on
long arrays, according to [Muła et al. 2018](Z-bibliography.md#ref-mula2018)),
and the comparison of the running times is left to the campaign of Appendix B. `popcountAnd( a , b , n )` counts the 1 bits of
$a \,\&\, b$ in the same way, the and being formed when the vectors are
loaded, which gives the size of the intersection of two sets of bits with
no array in between.

**Scatter, gather and select.** `pdep( x , m )` puts the low bits of $x$,
in order, in the places of the 1 bits of the mask $m$, and `pext( x , m )`
gathers the bits of $x$ in the places of the 1 bits of $m$ into the low
places. With BMI2 each is one instruction; otherwise it is a loop of
$\nu( m )$ iterations over the 1 bits of $m$. We remark that on the
processors of AMD before Zen 3 the two instructions are microcoded, with a
time that grows with $\nu( m )$, and the portable branch may well be
faster there; the header does not try to detect this case.

--8<-- "generated/FastBitwise-pdep.pseudo.md"

`select( x , r )` returns the place of the 1 of rank $r$ of $x$ (from 0),
i.e., the inverse of the rank $\nu( x \,\&\, ( 2^p - 1 ) )$ of a place
$p$, which the succinct structures of Chapter 22 need at each query. With
BMI2 it is the trailing zeros of $\mathrm{pdep}( 2^r , x )$, i.e.,
of the bit $r$ scattered to the place of the $r$-th 1 of $x$ ([Pandey et al.
2017](Z-bibliography.md#ref-pandey2017)). Without it, the broadword method
of [Vigna 2008](Z-bibliography.md#ref-vigna2008)
([Algorithm 18.4](18-combinatorial.md#alg-18-4)) computes the prefix sums
$s_j$ of the 1 bits of the bytes $0 , \ldots , j$ as in
[Lemma 18.1](18-combinatorial.md#lem-18-1), but keeps all of them (one per
byte) in the product by $\mathtt{0x0101 \ldots 01}$. It then compares $r$
with the 8 sums at once, as in [Lemma 18.4](18-combinatorial.md#lem-18-4):
the byte $j$ of $( ( r \cdot \mathtt{0x0101 \ldots 01} ) \mid
\mathtt{0x8080 \ldots 80} ) - s$ keeps its top bit if and only if
$s_j \le r$. Since the sums are nondecreasing, the bytes with
$s_j \le r$ are the lowest $k / 8$ ones, and the 1 of rank $r$ is the 1 of
rank $r - s_{k/8 - 1}$ of the byte $k / 8$. At most 7 steps of
$b \gets b \,\&\, ( b - 1 )$ and one count of the trailing zeros find it.

<span id="alg-18-4"></span>**Algorithm 18.4.** The 1 of rank $r$ of a word
([Vigna 2008](Z-bibliography.md#ref-vigna2008); [Pandey et al.
2017](Z-bibliography.md#ref-pandey2017) with BMI2).

--8<-- "generated/FastBitwise-select.pseudo.md"

Finally, `mortonEncode()` and `mortonDecode()` are `interleave()` and
`deinterleave()` of [§18.2](18-combinatorial.md#sec-18-2) by two
instructions `pdep` (resp. `pext`), with the masks $\mathtt{0x5555 \ldots
55}$ and $\mathtt{0xAAAA \ldots AA}$, and fall back on the functions of
`Bitwise.h` without BMI2.

## 18.4 The generation of combinatorial objects {#sec-18-4}

We now turn to the generation. A *generator* visits all the objects of a
kind, each once, in a fixed order. In `Generation.h` and
`FastGeneration.h` it is a function that receives a callable `visit` and
calls it on each object. If `visit` returns a value convertible to `bool`,
true stops the generation at once; a `visit` that returns `void` is never
tested (the choice is made at compile time). In both cases the function
returns the number of the objects visited, the last one included. The cost
of a generator is measured per object, i.e., amortized over the sequence,
and it does not include the visits; a generator is *loopless* when the work
between two consecutive objects is bounded by a constant even in the worst
case ([Knuth 2011, §7.2.1.1](Z-bibliography.md#ref-knuth2011)).
[Table 18.1](18-combinatorial.md#tab-18-1) lists the objects, their number
and their orders, and the tests check each count of the table on small
sizes.

<span id="tab-18-1"></span>**Table 18.1.** The objects of the generators,
their number, the functions that visit them (those whose names begin with
`forEach` are in `FastGeneration.h`, the others in `Generation.h`), and
their order; $m_v$ is the multiplicity of the item $v$ of a multiset, and
the order of the set partitions is that of their strings
([§18.7](18-combinatorial.md#sec-18-7)).

| Objects | Number | Function | Order |
|--------------|-----------|-----------------|-------------|
| $n$-tuples, $0 \le a_j < m_j$ | $\prod_j m_j$ | `mixedRadix()` | lexicographic |
| | | `grayTuples()` | reflected Gray |
| permutations of a multiset | $n! / \prod_v m_v!$ | `permutationsLex()` | lexicographic |
| | | `forEachPermutationLex()` | lexicographic |
| permutations of $n$ items | $n!$ | `plainChanges()` | adjacent swaps |
| | | `heapPermutations()` | one swap |
| | | `forEachPermutation()` | one swap |
| $t$-combinations of $n$ | $\binom{n}{t}$ | `combinationsLex()` | lexicographic |
| | | `forEachCombination()` | lexicographic |
| | | `forEachCombinationMask()` | increasing words |
| | | `revolvingDoor()` | revolving door |
| partitions of $n$ | $p( n )$ | `integerPartitions()` | reverse lexicographic |
| partitions of a set | $\varpi_n$ | `setPartitions()` | lexicographic |
| parentheses, $n$ pairs | $C_n$ | `nestedParentheses()` | lexicographic |

Most lexicographic generators of the area follow one scheme: from the
current object, find the last place that can grow, make it grow by the
least amount, and set the places after it to their least values. The cost
of a step is the length of the suffix that it scans. The argument of
[Lemma 18.6](18-combinatorial.md#lem-18-6) and
[Lemma 18.7](18-combinatorial.md#lem-18-7) is basically the same in both
cases: a long suffix changes rarely.

## 18.5 Tuples {#sec-18-5}

The $n$-tuples $( a_0 , \ldots , a_{n-1} )$ with $0 \le a_j < m_j$ are the
numbers of $n$ digits in the mixed radix $( m_0 , \ldots , m_{n-1} )$.
`mixedRadix()` visits them in lexicographic order by adding 1 with the
carries, with the last place as the least significant one (Algorithm
7.2.1.1M of [Knuth 2011](Z-bibliography.md#ref-knuth2011)).

--8<-- "generated/Generation-mixedRadix.pseudo.md"

<span id="lem-18-6"></span>**Lemma 18.6.** *If $m_j \ge 2$ for each $j$,
`mixedRadix()` costs $O( 1 )$ amortized time per tuple; the same holds for
`permutationsLex()` on $n$ distinct items, whose work per permutation is at
most proportional to $\sum_{k \ge 0} ( k + 1 ) / k! = 2 e$.*

*Proof.* A step that carries over the last $k$ places happens once every
$m_{n-1} m_{n-2} \cdots m_{n-k} \ge 2^k$ tuples, and it costs $O( k )$;
hence the work per tuple is at most proportional to
$\sum_{k \ge 1} k / 2^k = 2$. In a similar way, the step of
`permutationsLex()` that scans a suffix of length $k$ after the place of
the ascent (in lines 5 and 7, and in the reversal of line 8) costs
$O( k + 1 )$ and happens when that suffix is in decreasing order, i.e.,
once every $k!$ permutations, and the sum becomes
$\sum_{k \ge 0} ( k + 1 ) / k! = 2 e$. $\square$

We remark that the radices 1, which the contract allows, break the bound.
A place with $m_j = 1$ is always at the end of its range; hence, with the
radices $( 2 , 1 , \ldots , 1 )$ each of the two steps scans all the places.

In the *reflected Gray order* of the tuples, two consecutive tuples differ
by $\pm 1$ in one place. The order is defined as the lexicographic one with
a twist: the last place runs up and down alternately, and each other place
changes, by 1 in its current direction, when all the places after it are at
an end of their ranges, whereupon they reverse their direction instead of
going back to 0. `grayTuples()` is Algorithm 7.2.1.1H of [Knuth
2011](Z-bibliography.md#ref-knuth2011), which numbers the places from the
right (the place $j$ is `a[ n - 1 - j ]`), keeps the direction $o_j$ of each
place and adds the *focus pointers* $f_j$, so that the place to change is
read in $f_0$ instead of searched for
([Algorithm 18.5](18-combinatorial.md#alg-18-5)).

<span id="alg-18-5"></span>**Algorithm 18.5.** The loopless reflected
Gray order of the tuples (Algorithm 7.2.1.1H of [Knuth
2011](Z-bibliography.md#ref-knuth2011)).

--8<-- "generated/Generation-grayTuples.pseudo.md"

Roughly, the pointers skip the places that are at an end of their ranges
and wait for a place on their left to move. Knuth proves that, with the
updates of line 9, $f_0$ is always the place to change in the reflected
order; since line 9 costs a constant number of assignments, the algorithm
is loopless. [Table 18.2](18-combinatorial.md#tab-18-2) follows the algorithm on
$m = ( 2 , 3 )$.

<span id="tab-18-2"></span>**Table 18.2.** `grayTuples()` on the radices
$m = ( 2 , 3 )$: the tuple $( a_0 , a_1 )$ visited, then the directions
$( o_0 , o_1 )$ and the focus pointers $( f_0 , f_1 , f_2 )$ in the
numbering of the places from the right (the place 0 is $a_1$).

| Tuple | $o$ | $f$ | Changed next |
|---|---|---|---|
| $( 0 , 0 )$ | $( 1 , 1 )$ | $( 0 , 1 , 2 )$ | $a_1$ up |
| $( 0 , 1 )$ | $( 1 , 1 )$ | $( 0 , 1 , 2 )$ | $a_1$ up, to its end |
| $( 0 , 2 )$ | $( -1 , 1 )$ | $( 1 , 1 , 2 )$ | $a_0$ up, to its end |
| $( 1 , 2 )$ | $( -1 , -1 )$ | $( 0 , 2 , 2 )$ | $a_1$ down |
| $( 1 , 1 )$ | $( -1 , -1 )$ | $( 0 , 2 , 2 )$ | $a_1$ down, to its end |
| $( 1 , 0 )$ | $( 1 , -1 )$ | $( 2 , 1 , 2 )$ | none: $f_0 = n$ |

```cpp
--8<-- "generated/Generation-grayTuples.cpp"
```

Note that the precondition $m_j \ge 2$ is needed: a place of one value
would be at both ends of its range at once, and the test of line 8 could
not decide the direction to reverse.

## 18.6 Permutations {#sec-18-6}

`permutationsLex()` visits the permutations of a multiset in lexicographic
order, from the sorted one, by Algorithm 7.2.1.2L of [Knuth
2011](Z-bibliography.md#ref-knuth2011), which goes back to Narayana Pandita
in the fourteenth century. The suffix after the last ascent
$a_j < a_{j+1}$ is nonincreasing; the next permutation puts in the place $j$
the least item of the suffix larger than $a_j$, and reverses the suffix,
which makes it nondecreasing. Since the comparisons are $\ge$, equal items
are never swapped, and each distinct permutation of the multiset is visited
once, $n! / \prod_v m_v!$ in all (e.g., 12 for $( 1 , 1 , 2 , 3 )$).

--8<-- "generated/Generation-permutationsLex.pseudo.md"

The orders in which two consecutive permutations differ by one swap are
less obvious. The *plain changes* of Steinhaus, Johnson and Trotter
([Trotter 1962](Z-bibliography.md#ref-trotter1962); [Johnson
1963](Z-bibliography.md#ref-johnson1963)) swap two adjacent items at each
step. The largest item moves from one end of the permutation to the other
and back, by one place per step; between two of its sweeps, the other items
make one step of their plain changes, recursively. For $n = 3$ the order is
$012 , 021 , 201 , 210 , 120 , 102$, in which the item 2 goes left twice,
the items 0 and 1 swap, and 2 goes right twice. `plainChanges()` is
Algorithm 7.2.1.2P of [Knuth 2011](Z-bibliography.md#ref-knuth2011), with
a counter $c_j$ of the steps made by the item $j$ in its sweep and its
direction $o_j$ ([Algorithm 18.6](18-combinatorial.md#alg-18-6)). The
offset $s$ counts the larger items that sit at the left end, since they
shift the places of the smaller ones by one.

<span id="alg-18-6"></span>**Algorithm 18.6.** The plain changes
(Algorithm 7.2.1.2P of [Knuth 2011](Z-bibliography.md#ref-knuth2011)), the
places from 1.

--8<-- "generated/Generation-plainChanges.pseudo.md"

The algorithm costs $O( 1 )$ amortized time per permutation by the
argument of [Lemma 18.6](18-combinatorial.md#lem-18-6), since the inner
loop moves to the item $j - 1$ only when the item $j$ ends a sweep, i.e.,
once every $j$ steps. It is fair to remark, however, that `plainChanges()`
keeps the permutation in the places $1 , \ldots , n$ of its vector and
passes to `visit` a new vector of the $n$ items. Its cost per permutation
is therefore $\Theta( n )$ in the implementation, whereas
`heapPermutations()` and `forEachPermutation()` pass their vector.

*Heap's method* ([Heap 1963](Z-bibliography.md#ref-heap1963)) relaxes the
adjacency and makes one swap per permutation with the least bookkeeping of
all the methods compared by [Sedgewick
1977](Z-bibliography.md#ref-sedgewick1977). In its recursive form, one
generates the arrangements of the first $k$ places by generating those of
the first $k - 1$ places $k$ times, with a swap of the place $k - 1$ in
between: with the place $i$ (the loop counter) if $k$ is even, and with the
place 0 if $k$ is odd. `heapPermutations()` is the iterative form
([Algorithm 18.7](18-combinatorial.md#alg-18-7)), in which a counter $c_i$
of the swaps already made at the place $i$ replaces the stack of the
calls. Since the place $i$ closes a prefix of $i + 1$ items, the rule reads
"swap with $a_{c_i}$ if $i$ is odd, with $a_0$ if $i$ is even".

<span id="alg-18-7"></span>**Algorithm 18.7.** Heap's method, the
iterative form ([Heap 1963](Z-bibliography.md#ref-heap1963); [Sedgewick
1977](Z-bibliography.md#ref-sedgewick1977)).

--8<-- "generated/Generation-heapPermutations.pseudo.md"

<span id="tab-18-3"></span>**Table 18.3.** `heapPermutations( 4 )`: the
24 arrangements, by columns, the swap that produced each one (the places
swapped); each column is the block of the arrangements of the places
$0 , 1 , 2$ with one item in the place 3.

| | $a_3 = 3$ | | $a_3 = 2$ | | $a_3 = 1$ | | $a_3 = 0$ |
|---|---|---|---|---|---|---|---|
| | `0123` | 03 | `3102` | 13 | `0231` | 23 | `3210` |
| 01 | `1023` | 01 | `1302` | 01 | `2031` | 01 | `2310` |
| 02 | `2013` | 02 | `0312` | 02 | `3021` | 02 | `1320` |
| 01 | `0213` | 01 | `3012` | 01 | `0321` | 01 | `3120` |
| 02 | `1203` | 02 | `1032` | 02 | `2301` | 02 | `2130` |
| 01 | `2103` | 01 | `0132` | 01 | `3201` | 01 | `1230` |

In [Table 18.3](18-combinatorial.md#tab-18-3), inside each column the first
three places run through their 6 arrangements by the swaps
$01 , 02 , 01 , 02 , 01$, and the swaps between the columns bring each item
to the place 3 once. The proof that this happens for any $n$ hinges on the
net effect of a call, which depends on the parity of $k$.

<span id="lem-18-8"></span>**Lemma 18.8.** *Let $G( k )$ be the recursive
form of Heap's method on the places $0 , \ldots , k - 1$. Then $G( k )$
visits the $k!$ arrangements of those places, each once, with one swap
between two consecutive ones; furthermore, its net effect on the order of
the items is (i) the swap of the places 0 and $k - 1$ if $k$ is odd, and
(ii) if $k \ge 4$ is even, the arrangement*

$$
( a_{k-3} , a_{k-2} , a_1 , a_2 , \ldots , a_{k-4} , a_{k-1} , a_0 )
\tag{18.4}
$$

*of the initial items $a_0 , \ldots , a_{k-1}$ (and the swap of the two
places for $k = 2$).*

*Proof.* By induction on $k$; the cases $k \le 3$ are checked directly
([Table 18.3](18-combinatorial.md#tab-18-3), first column). It suffices to
show that the $k$ items that the place $k - 1$ holds during the $k$ calls
$G( k - 1 )$ are distinct, since each call visits all the arrangements of
the other $k - 1$ places by induction. Let $k \ge 4$ be even. By (i),
each call $G( k - 1 )$ swaps the places 0 and $k - 2$, and $G( k )$ is
the sequence of the swaps $( 0 , k - 2 )$ and $( i , k - 1 )$ for
$i = 0 , \ldots , k - 2$, followed by one more $( 0 , k - 2 )$. The places
$1 , \ldots , k - 3$ are untouched until the step $i$ that swaps them,
while the places 0 and $k - 2$ exchange $a_0$ and $a_{k-1}$ at each step
after the first. Following the items, one sees that the place $k - 1$
receives $a_{k-2} , a_1 , a_2 , \ldots , a_{k-3}$ and finally $a_0$, all
distinct from one another and from $a_{k-1}$, and that the final order is
(18.4). Let now $k \ge 5$ be odd, $m = k - 1$, and let $Q$ be the effect of a call
$G( m )$ followed by the swap $( 0 , m )$. By (18.4), $Q$ moves the items
along the single cycle of places $0 \to m - 1 \to m - 2 \to 1 \to 2 \to
\cdots \to m - 3 \to m \to 0$, of length $m + 1 = k$. After $j$
applications of $Q$, the place $m$ holds the item that was $j$ steps before
it along the cycle; hence the $m$ applications of $Q$ in $G( k )$ bring
$k - 1$ distinct items to the place $m$, none of them $a_m$. Finally,
$G( k )$ is $Q$ applied $m$ times followed by $G( m )$, i.e., $Q^{-1}$
followed by $G( m )$, which is the swap $( 0 , m )$ since $Q$ is $G( m )$
followed by that swap. $\square$

The counters of the iterative form run through the $n!$ values of a mixed
radix number with the radices $2 , 3 , \ldots , n$, and the place $i$
changes once every $i!$ arrangements. Thus, the loop of
[Algorithm 18.7](18-combinatorial.md#alg-18-7) makes one swap per
arrangement (cf. [Lemma 18.8](18-combinatorial.md#lem-18-8)), and resets $\sum_{i \ge 1} 1 / ( i + 1 )!
= e - 2$ counters per arrangement on the average, i.e., $e - 1$ iterations
in all:

```cpp
--8<-- "generated/Generation-heapPermutations.cpp"
```

## 18.7 Combinations, partitions and parentheses {#sec-18-7}

**Combinations.** A $t$-combination of $\{ 0 , \ldots , n - 1 \}$ is an
increasing tuple $c_0 < c_1 < \cdots < c_{t-1}$. The lexicographic
successor of `combinationsLex()` (the order of Algorithm 7.2.1.3L of
[Knuth 2011](Z-bibliography.md#ref-knuth2011)) finds the last place that
is not at its maximum $n - t + j$, increments it, and makes the places
after it consecutive.

--8<-- "generated/Generation-combinationsLex.pseudo.md"

<span id="lem-18-7"></span>**Lemma 18.7.** *The scans of line 5 of
`combinationsLex()` visit $\binom{n+1}{t}$ places over the whole
generation, i.e., $( n + 1 ) / ( n + 1 - t )$ per combination; hence the
cost is $O( 1 )$ amortized when $t \le \alpha n$ for a constant
$\alpha < 1$.*

*Proof.* The scan from a combination whose last $r$ places are at their
maxima, and the place before them is not, visits $r + 1$ places. The
combinations whose last $r$ places are at their maxima are
$\binom{n - r}{t - r}$ (the choices of the other $t - r$ items among the
$n - r$ below them). Summing the visits over the combinations gives
$\sum_{r=0}^{t} \binom{n - r}{t - r} = \binom{n+1}{t}$ by the
hockey-stick identity, and the ratio to $\binom{n}{t}$ is
$( n + 1 ) / ( n + 1 - t )$. The resets of line 7 are as many as the
places scanned, minus one per step. $\square$

In the *revolving door* order $R( n , t )$, two consecutive combinations
differ by one item in and one item out. The order is defined by

$$
R( n , t ) = R( n - 1 , t ) , \;
R( n - 1 , t - 1 )^{\mathrm{rev}} \cup \{ n - 1 \} , \tag{18.5}
$$

where $R( n , 0 )$ and $R( t , t )$ have one object each, and the second
part is the order $R( n - 1 , t - 1 )$ reversed, with the item $n - 1$
added to each combination. `revolvingDoor()` follows (18.5) recursively,
with a flag for the reversed orders: the reversal of (18.5) is the second
part not reversed followed by the first part reversed
([Algorithm 18.8](18-combinatorial.md#alg-18-8)). Knuth's Algorithm
7.2.1.3R generates the same order without recursion; the header follows
the recursion, which is closer to (18.5).

<span id="alg-18-8"></span>**Algorithm 18.8.** The revolving door, by the
recursion (18.5) ([Knuth 2011, §7.2.1.3](Z-bibliography.md#ref-knuth2011)).

--8<-- "generated/Generation-door.pseudo.md"

For instance, $R( 5 , 3 )$ is $012 , 023 , 123 , 013 , 034 , 134 , 234 ,
024 , 124 , 014$.

<span id="lem-18-9"></span>**Lemma 18.9.** *Two consecutive combinations
of $R( n , t )$ differ by one item in and one out, the first one is
$\{ 0 , \ldots , t - 1 \}$ and the last one, for $0 < t < n$, is
$\{ 0 , \ldots , t - 2 , n - 1 \}$; `revolvingDoor()` costs $O( t )$ time
per combination and recursion depth $O( n )$.*

*Proof.* The first and the last combinations follow from (18.5) by
induction (the first of $R( n , t )$ is that of $R( n - 1 , t )$, the last
is the first of $R( n - 1 , t - 1 )$ with $n - 1$ added). Inside each part
of (18.5) the claim holds by induction. At the junction, the last
combination of $R( n - 1 , t )$, $\{ 0 , \ldots , t - 2 , n - 2 \}$, is
followed by the last of $R( n - 1 , t - 1 )$ with $n - 1$ added,
$\{ 0 , \ldots , t - 3 , n - 2 , n - 1 \}$, which differs by $t - 2$ out
and $n - 1$ in (when $t - 1 = n - 1$ or $t = 1$ the same check on the
one-object orders is immediate). Finally, each node of the recursion with
$0 < t < n$ has two children; hence the nodes are $2 \binom{n}{t} - 1$,
each with $O( 1 )$ work but for the leaves $n = t$, which write $t$
places. $\square$

**Partitions of an integer.** `integerPartitions()` visits the partitions
of $n$, i.e., the nonincreasing sequences of positive parts with sum $n$,
in reverse lexicographic order, from $( n )$ to $( 1 , \ldots , 1 )$ (the
order of Algorithm 7.2.1.4P of [Knuth
2011](Z-bibliography.md#ref-knuth2011)). The trailing 1s are removed, and
the last part larger than 1, say $x + 1$, drops to $x$; the removed units,
with the one taken from that part, are split again into parts of $x$ and a
remainder.
For $n = 6$ the order is $6 , 51 , 42 , 411 , 33 , 321 , 3111 , 222 ,
2211 , 21111 , 111111$.

--8<-- "generated/Generation-integerPartitions.pseudo.md"

The implementation keeps the 1s explicitly and pops them one by one, and
hence its cost is not constant per partition. Indeed, the 1s of all the
partitions of $n$ are $\sum_{j \ge 1} p( n - j )$ (the partitions with at
least $j$ ones are $p( n - j )$), i.e., about $p( n ) \sqrt{6 n} / \pi$;
the pops therefore cost $\Theta( \sqrt{n} )$ per partition on the
average. Algorithm P of Knuth reaches $O( 1 )$ amortized by keeping the
index of the last part larger than 1 and by treating the parts equal to 2
apart, which is a possible improvement of the header.

**Partitions of a set.** A partition of $\{ 0 , \ldots , n - 1 \}$ into
blocks is encoded by its *restricted growth string* $a_0 a_1 \cdots
a_{n-1}$, in which $a_j$ is the block of the item $j$ and the blocks are
numbered in the order of their least items. Hence $a_0 = 0$ and
$a_j \le b_j = 1 + \max( a_0 , \ldots , a_{j-1} )$, and any string with
these properties encodes one partition. `setPartitions()` visits the
strings in lexicographic order (as Algorithm 7.2.1.5H of [Knuth
2011](Z-bibliography.md#ref-knuth2011)), and keeps the bounds $b_j$ next to
the string, which makes the test of line 6 cost $O( 1 )$ per place. For
$n = 4$ the $\varpi_4 = 15$ strings are $0000 , 0001 , 0010 , 0011 , 0012
, 0100 , \ldots , 0122 , 0123$.

--8<-- "generated/Generation-setPartitions.pseudo.md"

A step that raises the place $j$ costs $O( n - j )$, i.e., $O( n )$ in the
worst case. The steps that reach far to the left are rare, since the last
place changes most often (for $n \le 11$ the places scanned are fewer than
1.6 per partition); however, we do not prove a bound on the average here.

**Parentheses.** The strings of $n$ pairs of *balanced* parentheses (as
many opening as closing ones, and no prefix with more closing than opening
ones) are in bijection with the forests and with the binary trees of $n$
nodes ([Knuth 2011, §7.2.1.6](Z-bibliography.md#ref-knuth2011)), and
there are $C_n$ of them. `nestedParentheses()` builds them by a backtrack
([Chapter 19](19-backtrack.md#ch-19)) that appends an opening parenthesis
while fewer than $n$ are open, and a closing one while it closes an open
one. It tries the opening one first, which gives the lexicographic order
of the strings ($\mathtt{(} < \mathtt{)}$ in ASCII); for $n = 3$ the order
is
`((()))`, `(()())`, `(())()`, `()(())`, `()()()`.

--8<-- "generated/Generation-parentheses.pseudo.md"

<span id="lem-18-10"></span>**Lemma 18.10.** *Every leaf of the tree of
the calls of `nestedParentheses()` is at the depth $2 n$ and visits a
string; hence the generation costs $O( n )$ time per string and recursion
depth $2 n$.*

*Proof.* A node at the depth $i < 2 n$ has $o \le n$ and $c \le o$, and at
least one child: if $o < n$ the opening parenthesis is allowed, and
otherwise $o = n$ and $c < o$, since $c = o = n$ would mean $i = 2 n$.
Each node therefore lies on a path from the root to a leaf of depth $2 n$,
each leaf is one of the $C_n$ strings, and the nodes are at most
$2 n C_n + 1$, each with $O( 1 )$ work. $\square$

## 18.8 Engineered generation {#sec-18-8}

The generators of `FastGeneration.h` visit the same objects as their
counterparts of `Generation.h`, with the work per object brought down to a
few instructions; in a typical use, the visit then takes most of the time.
The common device is to unroll the last places, which change at each step.
The general step of the algorithm then runs once every few objects, and
the straight-line code in between does one store or one swap per object.

**Permutations by Heap's method.** `forEachPermutation( a , visit )`
visits the $n!$ arrangements of a vector of any items (the duplicates are
not merged), by Heap's method with the arrangements of the first 3 places
written as the 5 fixed swaps $01 , 02 , 01 , 02 , 01$ of the first column
of [Table 18.3](18-combinatorial.md#tab-18-3). The counters $c_i$ drive
only the places $i \ge 3$ ([Algorithm 18.9](18-combinatorial.md#alg-18-9)).

<span id="alg-18-9"></span>**Algorithm 18.9.** Heap's method with its
first 3 places unrolled.

--8<-- "generated/FastGeneration-forEachPermutation.pseudo.md"

<span id="lem-18-11"></span>**Lemma 18.11.** *`forEachPermutation()` makes
one swap per arrangement, and the steps of the loop of its counters (the
resets of line 6 and the swaps of line 8) are
$\sum_{i \ge 3} 1 / i! = e - 5/2 < 0.22$ per arrangement on the average,
against $\sum_{i \ge 1} 1 / i! = e - 1$ for the loop of
[Algorithm 18.7](18-combinatorial.md#alg-18-7).*

*Proof.* The swaps are those of Heap's method in the same order
([Lemma 18.8](18-combinatorial.md#lem-18-8)), one per arrangement. The
counter of the place $i \ge 3$ goes through its $i + 1$ values once every
$( i + 1 )!$ arrangements, with $i$ swaps and one reset; hence the steps
per arrangement are $\sum_{i \ge 3} ( i + 1 ) / ( i + 1 )! = \sum_{i \ge
3} 1 / i!$. The same count from $i = 1$ gives the second sum. $\square$

**Permutations of a multiset.** `forEachPermutationLex( a , visit )`
visits the distinct permutations of a multiset in lexicographic order, as
`std::next_permutation` from the sorted vector does, and leaves the vector
sorted. Algorithm L handles the places up to $n - 4$. The last three items
$x \le y \le z$ are nondecreasing after each step of L (the reversal of
the suffix makes them so), and they are written directly in their 6 orders
if they are distinct, in 3 if two are equal and in 1 if all are. The step
of L then starts from the place $n - 4$, since the last three items are
nonincreasing after their last order.

--8<-- "generated/FastGeneration-forEachPermutationLex.pseudo.md"

**Combinations as tuples.** `forEachCombination( n , t , visit )` gives
the last place a loop of its own, which visits the combinations that
differ in it only by one increment each. The successor of
`combinationsLex()` then runs only when the last place passes $n - 1$,
i.e., once every $n - c_{t-2} - 1$ combinations.

--8<-- "generated/FastGeneration-forEachCombination.pseudo.md"

**Combinations as words.** `forEachCombinationMask( n , t , visit )`
visits the words of $t$ ones below $2^n$ (for $n \le 64$) in increasing
order, i.e., the $t$-subsets of $\{ 0 , \ldots , n - 1 \}$ ordered as
binary numbers. The successor is the trick of Gosper in the form of
[Warren 2012, §2-1](Z-bibliography.md#ref-warren2012), which replaces the
division of (18.2) by a shift by $\rho( x ) + 1$ (the trailing zeros are
one instruction of the processor), i.e.,

$$
u = x \mid ( x - 1 ) , \qquad
x' = ( u + 1 ) \mid \big( ( ( \lnot u \,\&\, ( u + 1 ) ) - 1 ) \gg
( \rho( x ) + 1 ) \big) . \tag{18.6}
$$

where $u$ is $x$ with its trailing zeros set (cf. (18.1)), $u + 1$ is $v$
of [Lemma 18.3](18-combinatorial.md#lem-18-3), and $\lnot u \,\&\,
( u + 1 )$ is the bit set by the carry. Subtracting 1 from that bit gives
the run of 1s below it, which the shift by $\rho( x ) + 1$ shortens to the
$r - 1$ bits that go back to the bottom.

```cpp
--8<-- "generated/FastGeneration-nextCombination.cpp"
```

Furthermore, while the lowest 1 of $x$ has a 0 above it (and is below the
place $n - 1$), the next word is simply $x + ( x \,\&\, -x )$, i.e., the
lowest 1 moves one place up. The generator makes these moves by a loop of
its own, with two operations each, and calls (18.6) only at the end of a
run of the lowest 1 ([Algorithm 18.10](18-combinatorial.md#alg-18-10)).

<span id="alg-18-10"></span>**Algorithm 18.10.** The $t$-subsets of
$n \le 64$ items as words, in increasing order.

--8<-- "generated/FastGeneration-forEachCombinationMask.pseudo.md"

```cpp
--8<-- "generated/FastGeneration-forEachCombinationMask.cpp"
```

The run of the lowest 1 ends when it reaches another 1 or the place
$n - 1$. For a random $t$-subset the 1 above the lowest one is at a
distance of about $n / t$ places, and hence (18.6) should run for a
fraction of about $t / n$ of the words. The case $t = 64$ is treated apart, since
$2^{64} - 1$ cannot be written as a shift of 1.

## 18.9 Discussion and references {#sec-18-9}

The broadword tricks of [§18.2](18-combinatorial.md#sec-18-2) are collected
in [Knuth 2011, §7.1.3](Z-bibliography.md#ref-knuth2011) and in [Warren
2012](Z-bibliography.md#ref-warren2012); many of them circulated first in
the HAKMEM memo of [Beeler et al. 1972](Z-bibliography.md#ref-beeler1972),
whose item 175 is the trick of Gosper. The Gray code is in the patent of
[Gray 1953](Z-bibliography.md#ref-gray1953), and the interleaving of the
coordinates in the report of [Morton
1966](Z-bibliography.md#ref-morton1966). The population count of arrays by
carry-save adders is due to Harley and Seal, and [Muła et al.
2018](Z-bibliography.md#ref-mula2018) brought it to the vectors of AVX2,
together with the count of a vector by a shuffle of bytes. The broadword
select is that of [Vigna 2008](Z-bibliography.md#ref-vigna2008), and the
select by `pdep` that of [Pandey et al.
2017](Z-bibliography.md#ref-pandey2017). The generation of the tuples, the
permutations, the combinations and the partitions follows the algorithms
of [Knuth 2011, §7.2.1](Z-bibliography.md#ref-knuth2011), which also has
the history of each order; the plain changes are those of [Trotter
1962](Z-bibliography.md#ref-trotter1962) and [Johnson
1963](Z-bibliography.md#ref-johnson1963), and the method with one swap per
permutation is that of [Heap 1963](Z-bibliography.md#ref-heap1963), which
[Sedgewick 1977](Z-bibliography.md#ref-sedgewick1977) found to be the
fastest of the methods that he compared (on the machines of the time). The
comparison of the generators
of `FastGeneration.h` with `std::next_permutation` and with those of
`Generation.h`, and of `popcount()` with the scalar loop, belongs to the
campaign of Appendix B; we give no number here.

## 18.10 Exercises {#sec-18-10}

Each exercise names the test of the library that solves it (run it with
`ctest -R <name>`).

1. Count the 1 bits of a word, find its lowest and its highest 1, reverse
   its bits and convert it to and from the Gray code, each without a loop
   over the bits; check the Gray property of consecutive words and the
   zero-byte test on random words. Test: `taocp.bitwise`.
2. Using the trick of Gosper, visit the 3-subsets of 10 items as words
   and check that they are $\binom{10}{3} = 120$; explain why the
   successor of $2^{63}$ is refused. Test: `taocp.bitwise`.
3. Count the 1 bits of an array, and of the intersection of two arrays,
   by the method of Harley and Seal, and check the result against the
   scalar count at every length and offset; implement `pdep`, `pext` and
   `select` without BMI2. Test: `engineered_combinatorial.fast-bitwise`.
4. Generate the tuples in lexicographic and in reflected Gray order, the
   permutations by plain changes and by Heap's method, the combinations in
   lexicographic and in revolving-door order, the partitions of an integer
   and of a set, and the strings of parentheses, and check their numbers
   against $\prod_j m_j$, $n!$, $\binom{n}{t}$, $p( n )$, $\varpi_n$ and
   $C_n$. Test: `taocp.generation`.
5. Let a generator accept a visit that returns `void` (never stopping) or
   `bool` (stopping when true), with the test resolved at compile time, and
   check both on the empty object of $n = 0$. Test:
   `taocp.generation-visits`.
6. Unroll the first 3 places of Heap's method and the last 3 places of
   Algorithm L, give the last place of a combination a loop of its own,
   and check the results against the generators of `Generation.h`. Test:
   `engineered_combinatorial.fast-generation`.
7. Find the best way to cut a rod of length $L$ into pieces of given
   prices by trying all the partitions of $L$, and compare it with the
   dynamic program. Test: `exam_dp1.dp-8`.
8. Generate the strings of $n$ pairs of balanced parentheses and compare
   them with the $2^{2 n}$ strings of parentheses filtered by a counter of
   the open ones. Test: `exam_backtracka.backtrack-3`.
