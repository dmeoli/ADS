/*--------------------------------------------------------------------------*/
/*--------------------------- File BigInteger.h ----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * The integers of any length of Knuth (4.3.1), beyond the book: a sign and
 * the magnitude as a vector of digits (limbs) in base \f$ b = 2^{64} \f$,
 * the least significant first, with no leading zero, the numbers of one
 * or two digits kept in place with no allocation. The long operations
 * work on spans of digits with the scratch space allocated once per call,
 * and the digits are not zeroed before they are written. The product of
 * two digits in two and the sums with carry are one instruction where the
 * compiler offers them (`unsigned __int128` in GCC and Clang, `_umul128()` and
 * `_addcarry_u64()` in MSVC on x64), else the four products of the halves
 * of 32 bits; `ADS_BIGINT_PORTABLE` forces the latter. Where GCC or Clang
 * compile for x86-64 with BMI2 and ADX, the loops of the sums and of the
 * products by one digit are in assembly, with `MULX`, which leaves the
 * carry alone; `ADS_BIGINT_NOASM` keeps them in C++.
 *
 * - The sum and the difference are Algorithms 4.3.1A and 4.3.1S, a carry
 *   or a borrow per digit, \f$ O( n ) \f$.
 * - The product is Algorithm 4.3.1M, \f$ O( m n ) \f$, for the short factors:
 *   in the assembly by passes of \f$ k = 6 \f$ or 4 rows over the longer
 *   factor, a digit \f$ a_i \f$ at a time, the window of the \f$ k + 1 \f$
 *   places \f$ i , \ldots , i + k \f$ in registers, \f$ r_i \f$ and the high
 *   halves of the \f$ k \f$ products \f$ a_i b_c \f$ added on the chain of
 *   carries of `ADOX` and their low halves on that of `ADCX`: as
 *   \f$ W + a_i B + r_i \le ( b^k - 1 ) + ( b - 1 ) ( b^k - 1 ) + b - 1 =
 *   b^{k+1} - 1 \f$
 *   for the window \f$ W \f$ and the \f$ k \f$ digits \f$ B \f$ of \f$ b \f$,
 *   no carry leaves the window and both chains start again from 0 at the next
 *   digit. 31 instructions for 6 products, about 1.22 cycles per product on a
 *   Skylake, 1.4 on 32 digits; the rows left one at a time, and up to 8 digits
 *   the passes with no loop. Elsewhere two rows per pass in C++. A square
 *   \f$ a^2 = 2 T + D \f$ takes the products \f$ a_i a_j \f$, \f$ i < j \f$,
 *   from 32 digits by blocks of 6: those of a block with the digits after it
 *   by the passes of 6 rows, those inside it by a kernel of 15 products in
 *   registers, all in \f$ T \f$, and \f$ D \f$, the squares \f$ a_i^2 \f$ at
 *   \f$ b^{2i} \f$, added in the pass that doubles \f$ T \f$; below 32 by
 *   tiles of 4 digits, the passes of 4 rows after each tile in \f$ T \f$ and
 *   the squares of the tiles side by side in \f$ D \f$; the squares follow
 *   thresholds of their own (`kSqr...`). From `kKaratsuba` digits it is the
 *   product of Karatsuba (4.3.3A) in the subtractive form,
 *   \f[ a b = z_2 b^{2l} + \bigl( z_0 + z_2 - ( a_0 - a_1 ) ( b_0 - b_1 )
 *       \bigr) b^l + z_0 , \qquad z_0 = a_0 b_0 , \; z_2 = a_1 b_1 , \f]
 *   the sum of the middle in two passes on the two chains of carries, \f$ M( n
 *   ) = 3 M( n / 2 ) + O( n ) = O( n^{\log_2 3} ) \f$; from `kToom3` digits
 *   that of Toom-3, \f$ a b = r( x ) \f$, \f$ x = b^k \f$, from the five
 *   products \f$ v_t = a( t ) b( t ) \f$, \f$ t = 0 , 1 , -1 , 2 , \infty \f$,
 *   and the interpolation of Bodrato and Zanoni, \f$ r_3 = ( ( v_2 - v_{-1} )
 *   / 3 - ( v_1 - v_0 ) ) / 2 - 2 v_\infty \f$, \f$ r_1 = ( v_1 - v_{-1} ) /
 *   2 - r_3 \f$, \f$ r_2 = v_1 - v_0 - r_1 - r_3 - v_\infty \f$, the divisions
 *   exact, by 3 as the product by \f$ -( b - 1 ) / 3 \f$ and a running
 *   difference, \f$ M( n ) = 5 M( n / 3 ) + O( n ) \f$; from `kToom4` digits
 *   that of Toom-4, from the seven values at \f$ 0 , \pm 1 , \pm 2 , 1 / 2 \f$
 *   (times \f$ 2^6 \f$) and \f$ \infty \f$, by an interpolation whose steps
 *   all stay on nonnegative numbers: with \f$ E_1 = ( v_1 + v_{-1} ) / 2 \f$,
 *   \f$ O_1 = ( v_1 - v_{-1} ) / 2 \f$, \f$ E_2 = ( v_2 + v_{-2} ) / 2 \f$ and
 *   \f$ O_2 = ( v_2 - v_{-2} ) / 4 \f$, \f$ r_4 = ( ( E_2 - r_0 - 64 r_6 ) /
 *   4 - ( E_1 - r_0 - r_6 ) ) / 3 \f$, \f$ r_2 = E_1 - r_0 - r_6 - r_4 \f$,
 *   \f$ H = ( v_{1/2} - 64 r_0 - 16 r_2 - 4 r_4 - r_6 ) / 2 = 16 r_1 + 4 r_3 +
 *   r_5 \f$, \f$ r_3 = ( 17 O_1 - O_2 - H ) / 9 \f$,
 *   \f$ r_5 = ( O_2 - O_1 - 3 r_3 ) / 15 \f$ and
 *   \f$ r_1 = O_1 - r_3 - r_5 \f$, \f$ M( n ) = 7 M( n / 4 ) + O( n ) = O(
 *   n^{\log_4 7} ) \f$; from `kToom32` digits a factor up to \f$ 7 / 4 \f$ as
 *   long as the other goes by Toom-3/2 (\f$ a \f$ in 3 pieces, \f$ b
 *   \f$ in 2, the values at 0, 1, -1 and \f$ \infty \f$: \f$ r_2 = ( v_1 +
 *   v_{-1} ) / 2 - r_0 \f$, \f$ r_1 = ( v_1 - v_{-1} ) / 2 - r_3
 *   \f$), up to \f$ 5 / 2 \f$ by Toom-4/2 (also at 2: \f$ r_3 = ( ( v_2 -
 *   r_0 - 4 r_2 - 16 r_4 ) / 2 - O ) / 3 \f$); the longer factor cut in pieces
 *   as long as the shorter one when their lengths differ; from `kFFT` digits
 *   it is a convolution of the digits by the number theoretic transform
 *   (4.3.3, 4.6.4) and the Chinese remainder theorem (4.3.2), \f$ O( n \log n
 *   ) \f$, the top \f$ t \f$ digits of the longer factor left to rows of
 *   products by one digit when the transform of the rest is shorter by more
 *   than they cost, about \f$ 12 L \log_2 L \f$ cycles for \f$ L
 *   \f$ terms against \f$ 1.6 t n_b \f$. With AVX2 and the fused product and
 *   sum, the transform runs on lanes of doubles, one digit per term, modulo
 *   three primes \f$ p < 2^{50} \f$ with \f$ 3 \cdot 2^{30} \mid p - 1
 *   \f$, on \f$ 2^k \f$ or \f$ 3 \cdot 2^k \f$ terms, whichever is shorter,
 *   the residues exact integers in \f$ [ -p , p ] \f$ and the product of two
 *   of them reduced by the quotient rounded from the product by \f$ 1 / p \f$,
 *   two rounds per pass over the terms: exact, since a coefficient, below \f$
 *   n \, 2^{128} \f$, stays below the product of the primes, \f$ 2^{149}
 *   \f$, up to \f$ 2^{21} \f$ digits per factor. The coefficient \f$ i
 *   \f$ is \f$ y_1 + p_1 t_2 + p_1 p_2 t_3 \f$ by Garner, \f$ 0 \le y_1 , t_2
 *   , t_3 < 2^{50} \f$: one digit each, so the product is \f$ Y_1 + p_1 T_2 +
 *   p_1 p_2 T_3 \f$ for the numbers of those digits, three passes of products
 *   by one digit. Elsewhere and past that the digits are cut in two pieces of
 *   32 bits each, modulo three primes \f$ p < 2^{31} \f$ with \f$ 2^{26} \mid
 *   p - 1 \f$, on 8 lanes of AVX2 or one at a time, the products by the roots
 *   of unity those of Shoup and the others those of Montgomery: exact up to
 *   transforms of \f$ 2^{26} \f$ pieces, i.e., products of \f$ 2^{25} \f$
 *   digits, past which Karatsuba splits the factors. `multiplySchool()`,
 *   `multiplyKaratsuba()` and `multiplyFFT()` are kept apart for the
 *   comparison.
 * - The quotient and the remainder are Algorithm 4.3.1D for the short
 *   quotients: the divisor is shifted until its leading digit has the top bit
 *   set, and each trial quotient \f$ \hat q \f$ is the quotient of the three
 *   leading digits of the remainder by the two of the divisor, by their
 *   reciprocal (Moeller and Granlund, Algorithms 5 and 6), which exceeds the
 *   true one by at most 1 and makes the correction after the multiplication
 *   and subtraction ("add back") happen with probability about \f$ 2 / b \f$;
 *   below `kDivShort` digits on the stack. From `kDivBlock` digits of divisor
 *   (`kDivBlock6` for \f$ K = 6 \f$) the quotient goes \f$ K = 4 \f$ or 6
 *   digits at a time: the \f$ K \f$ digits \f$ Q \f$ of the window, below
 *   \f$ v b^K \f$, from its top \f$ t + K \f$ digits by the top
 *   \f$ t = K + 2 \f$ of \f$ v \f$, \f$ Q \le \hat Q \le Q + 1 \f$ as
 *   \f$ \hat Q - Q \le 1 + Q / v_t < 2 \f$, then the window less
 *   \f$ \hat Q v \f$ by one pass of \f$ K \f$ rows on the complements,
 *   \f$ r - a b = ( r_{top} - s ) b^n + \overline{\overline{r} + a b} \f$ with
 *   \f$ s \f$ the top digits of \f$ \overline{r} + a b \f$, and \f$ v \f$
 *   added back while it is negative. A division of two digits by one, in the
 *   short division, is two products by a reciprocal of the divisor computed
 *   once, from a table and three steps of Newton (Moeller and Granlund,
 *   Algorithms 2 and 4). The remainder alone by a digit below \f$ 2^{61} \f$
 *   from `kModFold` digits folds 4 digits per step into a remainder of two
 *   digits, by the residues of \f$ b^k \f$, with one division at the end;
 *   `divExact()` divides by one digit by its inverse modulo \f$ b \f$
 *   (Hensel), from `kDivExact2` digits both ways at once (Krandick and
 *   Jebelean): with \f$ q = q_h b^m + q_l \f$,
 *   \f$ q_l d = ( a \bmod b^m ) + c b^m \f$ for the last borrow \f$ c < d \f$
 *   of Hensel's division of the low \f$ m \f$ digits, so that
 *   \f$ \lfloor a / b^m \rfloor = q_h d + c \f$ and its quotient by \f$ d \f$
 *   is \f$ q_h \f$; \f$ m = n - n / 3 \f$, as the chain of the division from
 *   the top is about twice as long per digit. From `kDivide` digits of
 *   quotient the division is recursive (Burnikel and Ziegler; Brent and
 *   Zimmermann, Algorithm 1.8): the upper half of the quotient from the upper
 *   half of the divisor, corrected at most twice, then the lower half,
 *   \f$ O( M( n ) \log n ) \f$ for products in \f$ M( n ) \f$, a quotient
 *   longer than the divisor taken by blocks as long as the divisor. From
 *   `kNewton` digits of divisor, or `kNewtonKept` if its reciprocal is kept,
 *   it goes by blocks of about half the quotient, each block the product of
 *   the top digits of the remainder by a reciprocal of the top digits of the
 *   divisor (Newton's method, Brent and Zimmermann, Algorithm 3.5; Barrett),
 *   short by a few units at most, and the product of the block by the divisor
 *   taken modulo \f$ b^N - 1 \f$ by a cyclic convolution as long as the
 *   divisor, the digits that wrap known from the remainder and the lowest
 *   digit, \f$ O( M( n ) ) \f$. The reciprocal \f$ X = b^n + x \f$ of a of
 *   \f$ n \f$ digits, \f$ a X < b^{2n} \le a ( X + 2 ) \f$, is
 *   \f$ X = X_h b^l + \lfloor X_h t / b^{2h} \rfloor \f$ from that of the top
 *   \f$ h = n - l \f$ digits, \f$ t = b^{n+h} - a X_h < 2 a \f$: \f$ t \f$
 *   from \f$ a X_h \f$ modulo \f$ b^N - 1 \f$, \f$ N \ge n \f$, as \f$ t \f$,
 *   its sum with \f$ b^N - 1 \f$ and, \f$ X_h \f$ too large, its difference
 *   from it differ in the lowest digit, which is \f$ -a_0 x_0 \bmod b \f$; and
 *   of \f$ t \f$ only its \f$ h + 1 \f$ digits from the place \f$ l \f$ are
 *   not 0, about \f$ 1.8 \f$ to \f$ 2.1 M( n ) \f$ in all. The quotient alone,
 *   for `operator/()`, is first that of the top \f$ 2 m + 1 \f$ digits of the
 *   dividend by the top \f$ m + 2 \f$ of the divisor if it has \f$ m \f$
 *   digits, \f$ m < n - 3 \f$: with \f$ v = v_t b^s + v_l \f$ and
 *   \f$ u = u_t b^s + u_l \f$, the quotient \f$ q_t \f$ of \f$ u_t \f$ by
 *   \f$ v_t \f$ is \f$ q \f$ or \f$ q + 1 \f$, as
 *   \f$ u_t / v_t - u_t / ( v_t + 1 ) < 2 / b \f$, and it is \f$ q \f$ unless
 *   \f$ r_t b^s + u_l < q_t v_l < b^{|q_t| + s} \f$, which a remainder
 *   \f$ r_t \f$ longer than \f$ q_t \f$ rules out with no product. Else it is
 *   that of the dividend with one digit more, so that the last digit tells
 *   when the quotient may be one off: below `kDivCut` digits of divisor by
 *   Algorithm \f$ D \f$ with the divisor cut to the \f$ j + 2 \f$ leading
 *   digits for the digit \f$ j \f$, about half the work, and from `kNewton`
 *   digits with no remainder for the last block; the rare doubtful case is
 *   checked by a product. `divModSchool()` is Algorithm \f$ D \f$ alone. The
 *   quotient is truncated toward zero, as for the integers of C++.
 * - The conversion to and from the bases 2 to 36 (4.4) works with the largest
 *   power \f$ P \f$ of the base in one digit, \f$ 10^{19} \f$ for the base 10.
 *   In the bases that are powers of 2 it is a slice of bits per figure,
 *   \f$ O( n ) \f$. In the others it is divide and conquer (Brent and
 *   Zimmermann, section 1.7), with the powers \f$ P^{2^i} \f$ without their
 *   low digits 0 (about 30 per cent of the digits in the base 10) and their
 *   reciprocals kept per thread and base for the next conversions: the output
 *   splits the number by the power nearest the square root, the lower part
 *   padded with zeros, and the input joins two halves with one product per
 *   level, \f$ O( M( n ) \log n ) \f$; below `kConvert` digits they divide or
 *   multiply by \f$ P \f$ once per many figures, as `toStringSchool()` and
 *   `fromStringSchool()` do at every length. On the output, a remainder
 *   \f$ x < P^c \f$, \f$ c = 2^j \f$ chunks, below `kConvertF` digits goes by
 *   its fraction \f$ f = x / P^c \f$ (Bouvier and Zimmermann's scaled
 *   remainder tree), the product by the reciprocal of \f$ P^c \f$ kept,
 *   rounded up, to the \f$ q + 2 \f$ digits of \f$ P^c \f$ plus 2: with
 *   \f$ f = ( X + \theta ) / P^c \f$, \f$ X \f$ the integer of the \f$ c \f$
 *   chunks, the integer parts of \f$ f P^i \f$, \f$ i \le c \f$, are those of
 *   \f$ X / P^{c-i} \f$ while \f$ 0 \le \theta < 1 \f$. The low half of the
 *   chunks is the fraction of \f$ f P^{c/2} \f$, the same \f$ \theta \f$, by
 *   the band of the products that it needs (`mulBand()`, short by 2 units at
 *   most); the high half is the top digits of \f$ f \f$, its \f$ \theta \f$
 *   the fraction of the low half: \f$ f \f$ is rounded down if that fraction
 *   is at least 1/2, else up, and each half keeps its direction, so that
 *   \f$ \theta \f$ stays below 1/2 by less than \f$ 3 b^{-2} \f$ per level if
 *   up and above it if down, and no run of 9 or of 0 carries wrong. From
 *   `kFracLeaf` chunks down the chunks are the integer parts of \f$ f P \f$,
 *   \f$ f P^2 \f$, ..., \f$ f \f$ cut to the digits that the chunks left need,
 *   and the 19 figures of a chunk come by pairs from a table.
 * - `sqrtRem()` is the Karatsuba square root of Zimmermann (also Brent and
 *   Zimmermann, Algorithm 1.12): \f$ m \f$, shifted by an even number of bits
 *   to \f$ 2 k \f$ digits with the top one at least \f$ b / 4 \f$, is
 *   \f$ A \beta^2 + a_1 \beta + a_0 \f$ with
 *   \f$ \beta = b^{\lfloor k / 2 \rfloor} \f$; with \f$ ( s' , r' ) \f$ the
 *   root and the remainder of A and \f$ ( q , u ) \f$ the quotient and the
 *   remainder of \f$ r' \beta + a_1 \f$ by \f$ 2 s' \f$,
 *   \f$ s = s' \beta + q \f$ and \f$ r = u \beta + a_0 - q^2 \f$, if negative
 *   \f$ r + 2 s - 1 \f$ and \f$ s - 1 \f$, \f$ O( M( n ) ) \f$ for the
 *   products of the divisions; up to `kSqrtSmall` digits on the stack, two
 *   digits from the root in doubles plus the nearest integer to
 *   \f$ ( a - x^2 ) / 2 x \f$, then steps of 1. The remainder of \f$ a \f$,
 *   shifted by \f$ 2 t \f$ bits to \f$ m \f$, is
 *   \f$ ( r_m + 2 s_0 s_m - s_0^2 ) / 4^t \f$ for the \f$ t \f$ bits
 *   \f$ s_0 \f$ that the shift of the root drops. `sqrt()` needs no remainder:
 *   at the top \f$ l < h \f$ digits and the quotient alone of
 *   \f$ ( r' \beta + a_1 ) b \f$ by \f$ 2 s' \f$, \f$ q b + d \f$:
 *   \f$ d \ge 1 \f$ makes \f$ u \ge 2 s' / b \ge \beta \f$, so
 *   \f$ r \ge 0 \f$, with no remainder and no \f$ q^2 \f$, but with
 *   probability about \f$ 1 / b \f$; `sqrtNewton()` is Newton's method from
 *   above on the whole number.
 * - `pow()` is the binary method of 4.6.3, from left to right, squares and
 *   products by \f$ x \f$. `gcd()` is the algorithm of Lehmer (4.5.2L) on two
 *   digits: the leading 128 bits \f$ ( a_t , b_t ) \f$ of the two numbers run
 *   the algorithm of Euclid while the remainders keep more than 65 bits, so
 *   that by the lemma of Moeller its quotients are those of the numbers,
 *   \f$ ( a ; b ) = M ( \alpha ; \beta ) \f$ with \f$ \det M = 1 \f$ and
 *   entries below \f$ 2^{62} \f$, and one pass makes \f$ ( \alpha ; \beta ) =
 *   ( u_{11} a - u_{01} b ; u_{00} b - u_{10} a ) \f$, about 64 bits off both
 *   numbers; the pass has no subtraction, as
 *   \f$ -b = \overline{b} + 1 - b^n \f$ makes \f$ \alpha \equiv u_{11} a +
 *   u_{01} \overline{b} + u_{01} \f$ modulo \f$ b^n \f$, the two products of a
 *   digit summed below \f$ 2^{128} \f$. The steps run first on the leading
 *   digits alone down to remainders of \f$ 2^{33} \f$, then their matrix of
 *   entries below \f$ 2^{31} \f$ reduces the 128 bits exactly and the leading
 *   64 bits of the result run again: all in one digit, with the steps on each
 *   number written apart so that their state stays in registers; two digits
 *   only when the leading digits allow no step. From `kGcdDC` digits the top
 *   third goes to the half gcd of Moeller (after Knuth and Schoenhage):
 *   `hgcd()` of \f$ n \f$ digits reduces until |a - b| has
 *   \f$ s = \lfloor n / 2 \rfloor + 1 \f$ digits, from `kHgcd` digits by a
 *   recursive call on the top half, its matrix applied to the low digits, a
 *   few steps, and a second recursive call on the top of what is left, the two
 *   matrices multiplied (from `kHgcdMul7` digits by the 7 products of
 *   Winograd's form of Strassen's, on signed numbers),
 *   \f$ O( M( n ) \log n ) \f$; a step that the leading digits cannot make (a
 *   quotient of many digits) is a subtraction and a division. Down to two
 *   digits, where the binary algorithm ends; the number of one digit
 *   \f$ d = 2^t e \f$, \f$ e \f$ odd, meets the long one by the folded
 *   remainder by \f$ e \f$, or below `kHensel` digits by Hensel's division,
 *   whose \f$ a b^{-n} \f$ modulo \f$ e \f$ has the same gcd with \f$ e \f$;
 *   `gcdEuclid()` is the algorithm of Euclid.
 * - With `ADS_WITH_GMP` defined (by hand, or by the option of CMake of the
 *   same name), the products, the quotients, the conversions in the bases that
 *   are not powers of 2 and the greatest common divisor go to the mpn and mpz
 *   layers of GMP (`mpn_mul()`, `mpn_sqr()`, `mpn_tdiv_qr()`, `mpz_tdiv_q()`,
 *   `mpz_divexact()`, `mpz_get_str()`, `mpn_set_str()`, `mpz_gcd()`) in the
 *   ranges of lengths where GMP was the faster on a Xeon Gold 5120 (the
 *   thresholds `kGmp...`), on the digits of `BigInteger` as they are, read in
 *   place: for factors of \f$ n_a \ge n_b \f$ digits, the product when
 *   \f$ k_{mul} \le n_b < k_{top} \f$, with \f$ k_{top} \f$ `kGmpMulTop` if
 *   \f$ n_a < 2 n_b \f$ and `kGmpMulLongTop` otherwise, the square of
 *   \f$ n \f$ digits when \f$ n < \f$ `kGmpSqrTop`; for a divisor of \f$ n \f$
 *   digits and a quotient of \f$ m \f$ or \f$ m + 1 \f$, the division with the
 *   remainder when \f$ n < \f$ `kGmpDivTop` or \f$ m < \f$ `kGmpShort`, the
 *   quotient alone when \f$ m > n \f$ and \f$ n < \f$ `kGmpDivTop`, or when
 *   \f$ m \le n \f$, \f$ n \ge \f$ `kGmpQuot` and either \f$ n < \f$
 *   `kGmpQuotTop` or \f$ 2 m \le n \f$; the exact quotient (by Jebelean's
 *   algorithm, from the low digits) when \f$ n < \f$ `kGmpExactTop`; the
 *   conversion of \f$ n \f$ digits to a base not a power of 2 when `kConvert`
 *   \f$ \le n < \f$ `kGmpToStringTop`, and from one always; the gcd (by the
 *   subquadratic algorithm of Moeller) when the shorter argument has `kGmpGcd`
 *   digits or more. Elsewhere, and in the methods asked by name
 *   (`multiplySchool()`, `multiplyKaratsuba()`, `multiplyFFT()`,
 *   `divModSchool()`, `toStringSchool()`, `fromStringSchool()`,
 *   `gcdEuclid()`), `BigInteger` keeps its own code, which is all of it
 *   without the macro, when `<gmp.h>` is not even included; the same where a
 *   limb of GMP is not a word of 64 bits. GMP is free software under the LGPL
 *   v3 (or the GPL v2), as ADS is under the LGPL v3; note that GMP aborts
 *   where it cannot allocate, while `BigInteger` throws `std::bad_alloc`.
 *
 * The contracts [see `Contract.h`]: the divisor is not 0, the base is in
 * \f$ [ 2 , 36 ] \f$ and the string a number in it; the invariant is the
 * normal form (no leading zero, 0 not negative); `divMod()` ensures
 * \f$ a = q b + r \f$ with \f$ |r| < |b| \f$ and \f$ r \f$ of the sign of
 * \f$ a \f$, and that it equals `divModSchool()`, `operator/()` that its
 * quotient does, `divExact()` that its result times \f$ b \f$ is \f$ a \f$;
 * `multiplyKaratsuba()` and `multiplyFFT()` that they equal
 * `multiplySchool()`; the conversions that they equal `toStringSchool()` and
 * `fromStringSchool()`, `gcd()` that it equals `gcdEuclid()`, all audits. The
 * operations are defined in `BigInteger.tpp`.
 *
 * \see D. E. Knuth, The Art of Computer Programming, vol. 2, 3rd ed.,
 *      Addison-Wesley, 1998, sections 4.3.1, 4.3.2, 4.3.3, 4.4, 4.5.2 and
 *      4.6.3
 * \see A. Karatsuba, Yu. Ofman, Multiplication of multidigit numbers on
 *      automata, Soviet Physics Doklady 7, 595-596, 1963
 * \see A. L. Toom, The complexity of a scheme of functional elements
 *      realizing the multiplication of integers, Soviet Mathematics
 *      Doklady 3, 714-716, 1963
 * \see M. Bodrato, A. Zanoni, Integer and polynomial multiplication:
 *      towards optimal Toom-Cook matrices, Proceedings of ISSAC 2007,
 *      17-24, 2007, doi:10.1145/1277548.1277552
 * \see R. P. Brent, P. Zimmermann, Modern Computer Arithmetic, Cambridge
 *      University Press, 2010, sections 1.3, 1.4, 1.6, 1.7, 2.4 and 3.4
 * \see C. Burnikel, J. Ziegler, Fast recursive division, research report
 *      MPI-I-98-1-022, Max-Planck-Institut fuer Informatik, 1998
 * \see N. Moeller, T. Granlund, Improved division by invariant integers,
 *      IEEE Transactions on Computers 60(2), 165-175, 2011,
 *      doi:10.1109/TC.2010.143
 * \see P. L. Montgomery, Modular multiplication without trial division,
 *      Mathematics of Computation 44(170), 519-521, 1985,
 *      doi:10.1090/S0025-5718-1985-0777282-X
 * \see V. Shoup, NTL: a library for doing number theory, the products
 *      by a constant with its precomputed quotient
 * \see D. Harvey, Faster arithmetic for number-theoretic transforms,
 *      Journal of Symbolic Computation 60, 113-119, 2014,
 *      doi:10.1016/j.jsc.2013.09.002
 * \see T. Jebelean, An algorithm for exact division, Journal of Symbolic
 *      Computation 15(2), 169-180, 1993, doi:10.1006/jsco.1993.1012
 * \see W. Krandick, T. Jebelean, Bidirectional exact integer division,
 *      Journal of Symbolic Computation 21(4-6), 441-455, 1996,
 *      doi:10.1006/jsco.1996.0024
 * \see P. Barrett, Implementing the Rivest Shamir and Adleman public key
 *      encryption algorithm on a standard digital signal processor, Proc.
 *      CRYPTO 1986, Lecture Notes in Computer Science 263, 311-323, 1987,
 *      doi:10.1007/3-540-47721-7_24
 * \see C. Bouvier, P. Zimmermann, Division-free binary-to-decimal
 *      conversion, IEEE Transactions on Computers 63(8), 1895-1901, 2014,
 *      doi:10.1109/TC.2014.2315621
 * \see P. Zimmermann, Karatsuba square root, INRIA research report
 *      RR-3805, 1999, https://hal.inria.fr/inria-00072854
 * \see D. H. Lehmer, Euclid's algorithm for large numbers, The American
 *      Mathematical Monthly 45(4), 227-233, 1938,
 *      doi:10.1080/00029890.1938.11990797
 * \see A. Schoenhage, Schnelle Berechnung von Kettenbruchentwicklungen,
 *      Acta Informatica 1(2), 139-144, 1971, doi:10.1007/BF00289520
 * \see N. Moeller, On Schoenhage's algorithm and subquadratic integer gcd
 *      computation, Mathematics of Computation 77(261), 589-607, 2008,
 *      doi:10.1090/S0025-5718-07-02017-0
 * \see V. Strassen, Gaussian elimination is not optimal, Numerische
 *      Mathematik 13(4), 354-356, 1969, doi:10.1007/BF02165411
 * \see S. Winograd, On multiplication of 2 x 2 matrices, Linear Algebra and
 *      its Applications 4(4), 381-388, 1971,
 *      doi:10.1016/0024-3795(71)90009-7
 * \see T. Granlund and the GMP development team, GNU MP: The GNU Multiple
 *      Precision Arithmetic Library, version 6.3.0, 2023,
 *      https://gmplib.org/gmp-man-6.3.0.pdf, the layers mpn and mpz of the
 *      backend
 * \see H. S. Warren, Hacker's Delight, 2nd ed., Addison-Wesley, 2012,
 *      sections 9.2 and 9.4
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Università di Pisa \n
 *
 * \copyright &copy; by Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __ADS_BigInteger
 #define __ADS_BigInteger
                      /* self-identification: #endif constant */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/Contract.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// the width of the product of two digits done by the compiler: 128 with
// unsigned __int128, 64 with the intrinsics of MSVC on x64, 0 for none
#ifndef ADS_BIGINT_WIDE
 #if defined( ADS_BIGINT_PORTABLE )
  #define ADS_BIGINT_WIDE 0
 #elif defined( __SIZEOF_INT128__ )
  #define ADS_BIGINT_WIDE 128
 #elif defined( _MSC_VER ) && defined( _M_X64 )
  #define ADS_BIGINT_WIDE 64
 #else
  #define ADS_BIGINT_WIDE 0
 #endif
#endif
#if ADS_BIGINT_WIDE == 64
 #include <intrin.h>
#endif

// the lanes of the NTT: 256 with AVX2, else 0, one at a time
#ifndef ADS_BIGINT_SIMD
 #if defined( __AVX2__ )
  #define ADS_BIGINT_SIMD 256
 #else
  #define ADS_BIGINT_SIMD 0
 #endif
#endif
#if ADS_BIGINT_SIMD > 0
 #include <immintrin.h>
#endif

// the NTT on doubles, with the fused product and sum, where the lanes of
// AVX2 and the products of 128 bits are there
#ifndef ADS_BIGINT_FNTT
 #if ( ADS_BIGINT_SIMD >= 256 ) && defined( __FMA__ ) && \
     ( ADS_BIGINT_WIDE == 128 )
  #define ADS_BIGINT_FNTT 1
 #else
  #define ADS_BIGINT_FNTT 0
 #endif
#endif
#if ADS_BIGINT_FNTT
 #include <cmath>
#endif

// the loops of the sums and of the products by one digit in the assembly
// of x86-64, with MULX, where GCC or Clang compile for a processor that has
// BMI2 and ADX; ADS_BIGINT_NOASM keeps them in C++
#ifndef ADS_BIGINT_ASM
 #if ( ADS_BIGINT_WIDE == 128 ) && defined( __x86_64__ ) && \
     defined( __GNUC__ ) && defined( __BMI2__ ) && defined( __ADX__ ) && \
     ! defined( ADS_BIGINT_NOASM )
  #define ADS_BIGINT_ASM 1
 #else
  #define ADS_BIGINT_ASM 0
 #endif
#endif

// the backend of GMP, with ADS_WITH_GMP: the products, the quotients, the
// conversions and the greatest common divisor go to the mpn and mpz layers
// of GMP in the ranges of lengths where those are faster, on the digits of
// BigInteger as they are; only where a limb of GMP has 64 bits, all of
// them numbers, else BigInteger stays on its own code
#if defined( ADS_WITH_GMP )
 #include <cstring>
 #include <gmp.h>
 #if ! defined( ADS_BIGINT_GMP ) && ( GMP_LIMB_BITS == 64 ) && \
     ( GMP_NAIL_BITS == 0 )
  #define ADS_BIGINT_GMP 1
 #endif
#endif
#ifndef ADS_BIGINT_GMP
 #define ADS_BIGINT_GMP 0
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------- NAMESPACE ------------------------------------*/
/*--------------------------------------------------------------------------*/

namespace ADS {

namespace detail {

/// `std::allocator`, but the elements made with no value are left without
/// one: the digits are written before they are read, and the `memset` of
/// `std::vector` would cost a pass over them
template< class T >
struct NoInit : std::allocator< T > {
 template< class U > struct rebind { using other = NoInit< U >; };

 NoInit( void ) = default;
 template< class U > NoInit( const NoInit< U > & ) noexcept {}

 template< class U > void construct( U * p ) noexcept {
  ::new( static_cast< void * >( p ) ) U;
  }
 template< class U , class... A > void construct( U * p , A &&... a ) {
  ::new( static_cast< void * >( p ) ) U( std::forward< A >( a )... );
  }
 };

/// a function never inlined, the rare path of one called everywhere
#if defined( __GNUC__ ) || defined( __clang__ )
 #define ADS_BIGINT_NOINLINE __attribute__(( noinline ))
#elif defined( _MSC_VER )
 #define ADS_BIGINT_NOINLINE __declspec( noinline )
#else
 #define ADS_BIGINT_NOINLINE
#endif

/// the blocks of digits freed, kept per thread for the next numbers, 8 at
/// most of each capacity \f$ 2^k \f$, \f$ 2 \le k < 12 \f$, and 2 up to
/// \f$ k = 16 \f$, about 2 MiB in all at most, so that the
/// temporaries of the operations cost no call to the allocator once the
/// thread has made a few; trivial, so that the thread reads it with no
/// guard, the blocks freed at the end of the thread by `DigitReaper`
struct DigitPool {
 static constexpr unsigned kBins = 17 , kKeep = 8;
 std::uint64_t * bin[ kBins ][ kKeep ];
 unsigned cnt[ kBins ];
 bool armed , dead;                         ///< the reaper made, and done
 };

inline constinit thread_local DigitPool digitPool{};

struct DigitReaper {
 ~DigitReaper( void ) {
  for( unsigned k = 0 ; k < DigitPool::kBins ; ++k )
   for( unsigned i = 0 ; i < digitPool.cnt[ k ] ; ++i )
    std::allocator< std::uint64_t >().deallocate( digitPool.bin[ k ][ i ] ,
                                                  std::size_t( 1 ) << k );
  for( unsigned & c : digitPool.cnt ) c = 0;
  digitPool.dead = true;                    // the blocks freed later go back
  }
 };

inline thread_local DigitReaper digitReaper;

/// the digits of a number, as a `std::vector` but with up to two of them
/// in place, so that the numbers of one or two digits are made with no
/// allocation; the digits made with no value are left without one; the
/// capacities from 4 to \f$ 2^{16} \f$ are powers of 2, from `DigitPool`
class Digits {

 public:

 using T = std::uint64_t;

 Digits( void ) noexcept {}
 explicit Digits( std::size_t n ) { resize( n ); }
 Digits( std::size_t n , T v ) { resize( n , v ); }
 Digits( const T * b , const T * e ) {
  resize( std::size_t( e - b ) );
  std::copy( b , e , data() );
  }
 Digits( std::initializer_list< T > l ) {
  resize( l.size() );
  std::copy( l.begin() , l.end() , data() );
  }
 Digits( const Digits & o ) : Digits( o.data() , o.data() + o.f_n ) {}
 Digits( Digits && o ) noexcept { take( o ); }
 Digits & operator=( const Digits & o ) {
  if( this != &o ) {
   f_n = 0;
   resize( o.f_n );
   std::copy( o.data() , o.data() + o.f_n , data() );
   }
  return( *this );
  }
 Digits & operator=( Digits && o ) noexcept {
  if( this != &o ) {
   free();
   take( o );
   }
  return( *this );
  }
 ~Digits( void ) { free(); }

 [[nodiscard]] std::size_t size( void ) const { return( f_n ); }
 [[nodiscard]] bool empty( void ) const { return( f_n == 0 ); }
 T * data( void ) { return( f_p ); }
 const T * data( void ) const { return( f_p ); }
 T * begin( void ) { return( data() ); }
 T * end( void ) { return( data() + f_n ); }
 const T * begin( void ) const { return( data() ); }
 const T * end( void ) const { return( data() + f_n ); }
 T & operator[]( std::size_t i ) { return( data()[ i ] ); }
 const T & operator[]( std::size_t i ) const { return( data()[ i ] ); }
 T & back( void ) { return( data()[ f_n - 1 ] ); }
 const T & back( void ) const { return( data()[ f_n - 1 ] ); }

 void reserve( std::size_t n ) { grow( n , n ); }
 void resize( std::size_t n ) {
  grow( n , n );
  f_n = n;
  }
 void resize( std::size_t n , T v ) {
  grow( n , n );
  if( n > f_n ) std::fill( data() + f_n , data() + n , v );
  f_n = n;
  }
 void assign( std::size_t n , T v ) {
  f_n = 0;
  resize( n , v );
  }
 void push_back( T v ) {
  grow( f_n + 1 , 2 * f_n );
  data()[ f_n++ ] = v;
  }
 void pop_back( void ) { --f_n; }

 friend bool operator==( const Digits & a , const Digits & b ) {
  return( ( a.f_n == b.f_n ) &&
          std::equal( a.data() , a.data() + a.f_n , b.data() ) );
  }

 private:

 static constexpr std::size_t kIn = 2;      ///< the digits in place

 /// room for \f$ n \f$ digits, \f$ c \f$ if it must move to the heap
 void grow( std::size_t n , std::size_t c ) {
  if( n > f_cap ) regrow( n , c );
  }
 void free( void ) {
  if( f_p != f_in ) {
   release( f_p , f_cap );
   f_p = f_in;
   f_cap = kIn;
   }
  }
 /// the move to a block of \f$ \max( n , c ) \f$ digits, at least, out of
 /// line so that `grow()` and `free()`, called everywhere, stay a test and a
 /// jump
 ADS_BIGINT_NOINLINE void regrow( std::size_t n , std::size_t c ) {
  c = std::max( n , c );
  T * q;
  if( c < ( std::size_t( 1 ) << ( DigitPool::kBins - 1 ) ) ) {
   const unsigned k = unsigned( std::bit_width( std::max< std::size_t >( c ,
                                                              4 ) - 1 ) );
   c = std::size_t( 1 ) << k;
   DigitPool & P = digitPool;
   q = P.cnt[ k ] ? P.bin[ k ][ --P.cnt[ k ] ]
                  : std::allocator< T >().allocate( c );
   }
  else q = std::allocator< T >().allocate( c );
  std::copy( data() , data() + f_n , q );
  free();
  f_p = q;
  f_cap = c;
  }
 ADS_BIGINT_NOINLINE static void release( T * p , std::size_t cap ) {
  DigitPool & P = digitPool;
  const unsigned k = unsigned( std::countr_zero( cap ) );
  if( std::has_single_bit( cap ) && ( k < DigitPool::kBins ) &&
      ( P.cnt[ k ] < ( k < 12 ? DigitPool::kKeep : 2u ) ) && ! P.dead ) {
   if( ! P.armed ) {                        // the reaper, once per thread
    (void) &digitReaper;
    P.armed = true;
    }
   P.bin[ k ][ P.cnt[ k ]++ ] = p;
   }
  else std::allocator< T >().deallocate( p , cap );
  }
 void take( Digits & o ) {
  f_n = o.f_n;
  f_cap = o.f_cap;
  if( o.f_p != o.f_in ) f_p = o.f_p;
  else std::copy( o.f_in , o.f_in + f_n , f_in );
  o.f_p = o.f_in;
  o.f_n = 0;
  o.f_cap = kIn;
  }

 T * f_p = f_in;                            ///< `f_in` or on the heap
 std::size_t f_n = 0 , f_cap = kIn;
 T f_in[ kIn ] = {};

 };  // end( class Digits )

}  // end( namespace detail )

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS BigInteger -----------------------------*/
/*--------------------------------------------------------------------------*/

/// an integer of any length, sign and magnitude in base \f$ 2^{64} \f$
class BigInteger {

/*--------------------------------------------------------------------------*/
 public:

 using Limb = std::uint64_t;   ///< a digit, in base \f$ 2^{64} \f$

 // the thresholds, in digits, from the timings of bench/bignum.cpp on a
 // Xeon Gold 5120, against GMP
 static constexpr std::size_t kKaratsuba = 40;   ///< limbs for Karatsuba
 static constexpr std::size_t kToom3 = 120;      ///< limbs for Toom-3
 static constexpr std::size_t kToom4 = 200;      ///< limbs for Toom-4
 static constexpr std::size_t kToom32 = 64;      ///< limbs, Toom-3/2, 4/2
 static constexpr std::size_t kFFT = 768;        ///< limbs for the NTT
 static constexpr std::size_t kSqrKaratsuba = 56;   ///< the same, squares
 static constexpr std::size_t kSqrToom3 = 160;      ///< squares, Toom-3
 static constexpr std::size_t kSqrToom4 = 300;      ///< squares, Toom-4
 static constexpr std::size_t kSqrFFT = 768;        ///< squares, the NTT
 static constexpr std::size_t kDivide = 96;      ///< quotient limbs, recursion
 static constexpr std::size_t kConvert = 24;     ///< limbs, divide and conquer
 static constexpr std::size_t kConvertF = 64;    ///< limbs, by fractions
 static constexpr std::size_t kFracLeaf = 32;    ///< chunks, by products
 static constexpr std::size_t kFracLevel = 4;    ///< \f$ 2^k \f$ chunks
 static constexpr std::size_t kFracCyclic = 448; ///< limbs, cyclic products
 static constexpr std::size_t npos = std::size_t( -1 );   ///< no length
 static constexpr std::size_t kModFold = 16;     ///< limbs, folded remainder
 static constexpr std::size_t kHensel = 20;      ///< limbs, gcd by folding
 static constexpr std::size_t kNewton = 768;     ///< limbs, by the reciprocal
 static constexpr std::size_t kNewtonKept = 512; ///< limbs, the same if kept
 static constexpr std::size_t kRecip = 32;       ///< limbs, reciprocal by D
 static constexpr std::size_t kDivCut = 900;     ///< limbs, quotient alone
 static constexpr std::size_t kDivShort = 16;    ///< limbs, on the stack
 static constexpr std::size_t kDivBlock = 40;    ///< limbs, 4 digits a step
 static constexpr std::size_t kDivBlock6 = 64;   ///< limbs, 6 digits a step
 static constexpr std::size_t kDivExact2 = 32;   ///< limbs, exact both ways
 static constexpr std::size_t kHgcd = 128;       ///< limbs, recursive hgcd
 static constexpr std::size_t kGcdDC = 300;      ///< limbs, gcd by hgcd

 // with ADS_WITH_GMP, the lengths in digits that go to GMP, where it was
 // faster in the timings of the same benchmark: the products whose shorter
 // factor has [ kGmpMul , kGmpMulTop ) digits, [ kGmpMul , kGmpMulLongTop )
 // when the longer one is twice as long or more, the squares below
 // kGmpSqrTop; the quotients with the remainder whose divisor is below
 // kGmpDivTop or whose quotient is below kGmpShort, the quotients alone
 // longer than the divisor as those, the others from kGmpQuot digits of
 // divisor below kGmpQuotTop, or at most half as long as the divisor; the
 // exact quotients below kGmpExactTop, the conversions to a base not a
 // power of 2 from kConvert digits below kGmpToStringTop, from one at any
 // length; the greatest common divisors whose shorter argument has
 // kGmpGcd digits or more
 static constexpr std::size_t kGmpMul = 5;          ///< limbs, mpn_mul
 static constexpr std::size_t kGmpMulTop = 1536;      ///< the products
 static constexpr std::size_t kGmpMulLongTop = 704;   ///< the same, long
 static constexpr std::size_t kGmpSqrTop = 1920;    ///< limbs, mpn_sqr
 static constexpr std::size_t kGmpDivTop = 3584;    ///< mpn_tdiv_qr
 static constexpr std::size_t kGmpShort = 512;      ///< quotient limbs
 static constexpr std::size_t kGmpQuot = 8;         ///< mpz_tdiv_q
 static constexpr std::size_t kGmpQuotTop = 49152;    ///< the quotients
 static constexpr std::size_t kGmpExactTop = 262144;   ///< mpz_divexact
 static constexpr std::size_t kGmpToStringTop = 49152; ///< mpz_get_str
 static constexpr std::size_t kGmpGcd = 2048;       ///< limbs, mpz_gcd

 /// the number \f$ v \f$
 BigInteger( long long v = 0 );

 /// the number written in \f$ s \f$ in the base, with an optional sign
 ///
 /// In pseudocode (section 4.4 of Knuth and section 1.7 of Brent and
 /// Zimmermann, beyond the book), \f$ P \f$ the largest power of the base in
 /// a digit and \f$ c \f$ its figures:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{fromString}( s , \beta ) \cr
 /// 2 & \quad \textbf{if}\ \beta \text{ is a power of 2}\ \textbf{then}
 ///     \ \textbf{return}\ \text{the bits of the figures, side by side} \cr
 /// 3 & \quad \textbf{if}\ |s| \text{ is short}\ \textbf{then}
 ///     \ \textbf{return}\ \mathrm{fromStringSchool}( s , \beta ) \cr
 /// 4 & \quad h \gets \text{the figures of the lower half};\ ( s_1 , s_0 )
 ///     \gets s \text{ cut there} \cr
 /// 5 & \quad \textbf{return}\ \mathrm{fromString}( s_1 , \beta ) \cdot
 ///     \beta^h + \mathrm{fromString}( s_0 , \beta )
 /// \end{array}}
 /// \f]
 /// with the powers \f$ \beta^h = P^{2^i} \f$ kept per thread and base.
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$ and \f$ s \f$ a number in the
 ///      base
 /// \post `*this == fromStringSchool( s , base )` (audit)
 explicit BigInteger( const std::string & s , int base = 10 );

 /// the figures of the number in the base, with a sign if negative
 ///
 /// In pseudocode (section 1.7 of Brent and Zimmermann, beyond the book),
 /// the divisions by the powers \f$ P^{2^i} \f$ kept per thread and base:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{toString}( x , \beta ) \cr
 /// 2 & \quad \textbf{if}\ \beta \text{ is a power of 2}\ \textbf{then}
 ///     \ \textbf{return}\ \text{the slices of bits of } x \cr
 /// 3 & \quad \textbf{if}\ x \text{ is short}\ \textbf{then}
 ///     \ \textbf{return}\ \mathrm{toStringSchool}( x , \beta ) \cr
 /// 4 & \quad Q \gets \text{the power } P^{2^i} \text{ nearest }
 ///     \sqrt{x};\ ( q , r ) \gets \mathrm{divMod}( x , Q ) \cr
 /// 5 & \quad \textbf{return}\ \mathrm{toString}( q , \beta ) \cdot
 ///     \mathrm{toString}( r , \beta ) \text{, the latter padded with }
 ///     0 \cr
 /// \end{array}}
 /// \f]
 /// where the remainders below `kConvertF` digits go by their fraction
 /// \f$ r / Q \f$ instead (the scaled remainder tree of Bouvier and
 /// Zimmermann).
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$
 /// \post the result equals `toStringSchool( base )` (audit)
 [[nodiscard]] std::string toString( int base = 10 ) const;

 /// the figures of `toString()` by divisions by \f$ P \f$ alone,
 /// \f$ O( n^2 ) \f$
 ///
 /// In pseudocode (section 4.4 of Knuth, beyond the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{toStringSchool}( x , \beta ) \cr
 /// 2 & \quad \textbf{while}\ x \ne 0\ \textbf{do} \cr
 /// 3 & \qquad ( x , r ) \gets \mathrm{divMod}( x , P ) \text{, a division }
 ///     \text{by one digit} \cr
 /// 4 & \qquad \text{prepend the } c \text{ figures of } r
 ///     \text{ in the base } \beta \cr
 /// 5 & \quad \textbf{return}\ \text{the figures, the zeros on the left }
 ///     \text{removed}
 /// \end{array}}
 /// \f]
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$
 [[nodiscard]] std::string toStringSchool( int base = 10 ) const;
 /// the number of the constructor from a string, by products by \f$ P \f$
 /// alone, \f$ O( n^2 ) \f$: each chunk of \f$ c \f$ figures is
 /// \f$ x \gets x P + \mathit{chunk} \f$ (section 4.4 of Knuth)
 /// \pre \f$ 2 \le \mathit{base} \le 36 \f$ and \f$ s \f$ a number in the
 ///      base
 static BigInteger fromStringSchool( const std::string & s , int base = 10 );

 /// the sign, \f$ -1 \f$, 0 or 1
 [[nodiscard]] int sign( void ) const;
 /// the absolute value \f$ |x| \f$
 [[nodiscard]] BigInteger abs( void ) const;

 /// the number of digits (limbs) in base \f$ 2^{64} \f$
 [[nodiscard]] std::size_t limbs( void ) const;

 /// the opposite \f$ -x \f$
 BigInteger operator-( void ) const;
 /// the sum \f$ a + b \f$, by Algorithm 4.3.1A on the magnitudes of the same
 /// sign and 4.3.1S on those of opposite signs, \f$ O( n ) \f$
 friend BigInteger operator+( const BigInteger & a , const BigInteger & b );
 /// the difference \f$ a - b \f$, as the sum with the sign of \f$ b \f$
 /// changed
 friend BigInteger operator-( const BigInteger & a , const BigInteger & b );
 /// the product \f$ a b \f$, by the method of the length of the factors
 ///
 /// In pseudocode (sections 4.3.1 and 4.3.3 of Knuth, beyond the book),
 /// \f$ n_a \ge n_b \f$ the digits of the factors:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiply}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ n_b < \mathtt{kKaratsuba}\ \textbf{then
 ///     return}\ \mathrm{multiplySchool}( a , b ) \cr
 /// 3 & \quad \textbf{if}\ n_b \ge \mathtt{kFFT}\ \textbf{then return}
 ///     \ \text{the product by the NTT} \cr
 /// 4 & \quad \textbf{if}\ n_a > n_b\ \textbf{then}\ \text{Toom-3/2 or }
 ///     \text{Toom-4/2, else the pieces of } a \text{ as long as } b \cr
 /// 5 & \quad \textbf{else}\ \text{Karatsuba, Toom-3 or Toom-4 by the }
 ///     \text{thresholds, each part multiplied again}
 /// \end{array}}
 /// \f]
 friend BigInteger operator*( const BigInteger & a , const BigInteger & b );
 /// the quotient \f$ a / b \f$ truncated toward zero, with no remainder
 /// when it can be spared (see the file)
 /// \pre \f$ b \ne 0 \f$
 friend BigInteger operator/( const BigInteger & a , const BigInteger & b );
 /// the remainder of `divMod()`, of the sign of \f$ a \f$
 /// \pre \f$ b \ne 0 \f$
 friend BigInteger operator%( const BigInteger & a , const BigInteger & b );
 /// the sum \f$ x + b \f$ into \f$ x \f$
 BigInteger & operator+=( const BigInteger & b );
 /// the difference \f$ x - b \f$ into \f$ x \f$
 BigInteger & operator-=( const BigInteger & b );
 /// the product \f$ x b \f$ into \f$ x \f$, in place when \f$ b \f$ has one
 /// digit
 BigInteger & operator*=( const BigInteger & b );
 /// true if the two numbers are equal
 friend bool operator==( const BigInteger & a , const BigInteger & b ) =
                                                                     default;
 /// the order of \f$ a \f$ and \f$ b \f$, by the signs, the lengths and the
 /// digits from the most significant
 friend std::strong_ordering operator<=>( const BigInteger & a ,
                                          const BigInteger & b );

 /// the quotient truncated toward zero and the remainder
 ///
 /// In pseudocode (section 4.3.1 of Knuth and chapter 1 of Brent and
 /// Zimmermann, beyond the book), \f$ n \f$ the digits of the divisor and
 /// \f$ m \f$ those of the quotient:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{divMod}( a , b ) \cr
 /// 2 & \quad \textbf{if}\ |a| < |b|\ \textbf{then return}\ ( 0 , a ) \cr
 /// 3 & \quad \textbf{if}\ n = 1\ \textbf{then return}\ \text{the short }
 ///     \text{division by a reciprocal of the digit} \cr
 /// 4 & \quad \textbf{if}\ n \ge \mathtt{kNewton}\ \textbf{then return}
 ///     \ \text{the blocks by a reciprocal of the top of } b \cr
 /// 5 & \quad \textbf{if}\ m \ge \mathtt{kDivide}\ \textbf{then return}
 ///     \ \text{the recursive division of Burnikel and Ziegler} \cr
 /// 6 & \quad \textbf{return}\ \text{Algorithm D, by } K \text{ digits a }
 ///     \text{step from } \mathtt{kDivBlock} \text{ digits}
 /// \end{array}}
 /// \f]
 /// \pre \f$ b \ne 0 \f$
 /// \post \f$ a = q b + r \f$, \f$ |r| < |b| \f$, \f$ r = 0 \f$ or of the
 ///       sign of \f$ a \f$, and the pair equals `divModSchool( a , b )`
 ///       (audits)
 static std::pair< BigInteger , BigInteger > divMod( const BigInteger & a ,
                                                     const BigInteger & b );

 /// the quotient and the remainder by Algorithm D alone, \f$ O( m n ) \f$
 ///
 /// In pseudocode (Algorithm 4.3.1D of Knuth, beyond the book), in base
 /// \f$ \beta = 2^{64} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{divModSchool}( u , v ) \cr
 /// 2 & \quad \text{shift } u \text{ and } v \text{ left until the top bit }
 ///     \text{of } v_{n-1} \text{ is 1} \cr
 /// 3 & \quad \textbf{for}\ j \gets m\ \textbf{downto}\ 0\ \textbf{do} \cr
 /// 4 & \qquad \hat q \gets \text{the quotient of } u_{j+n} u_{j+n-1}
 ///     u_{j+n-2} \text{ by } v_{n-1} v_{n-2} \cr
 /// 5 & \qquad u_{j .. j+n} \gets u_{j .. j+n} - \hat q v \cr
 /// 6 & \qquad \textbf{if}\ \text{it is negative}\ \textbf{then}\ \hat q
 ///     \gets \hat q - 1;\ u_{j .. j+n} \gets u_{j .. j+n} + v \cr
 /// 7 & \qquad q_j \gets \hat q \cr
 /// 8 & \quad \textbf{return}\ ( q , u \text{ shifted back right} )
 /// \end{array}}
 /// \f]
 /// \pre \f$ b \ne 0 \f$
 static std::pair< BigInteger , BigInteger > divModSchool(
                                const BigInteger & a , const BigInteger & b );

 /// the product by Algorithm M, \f$ O( n_a n_b ) \f$
 ///
 /// In pseudocode (Algorithm 4.3.1M of Knuth, beyond the book), in base
 /// \f$ \beta = 2^{64} \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiplySchool}( a , b ) \cr
 /// 2 & \quad w \gets n_a + n_b \text{ zero digits} \cr
 /// 3 & \quad \textbf{for}\ j \gets 0\ \textbf{to}\ n_b - 1\ \textbf{do} \cr
 /// 4 & \qquad c \gets 0 \cr
 /// 5 & \qquad \textbf{for}\ i \gets 0\ \textbf{to}\ n_a - 1\ \textbf{do}
 ///     \ ( c , w_{i+j} ) \gets a_i b_j + w_{i+j} + c \cr
 /// 6 & \qquad w_{j+n_a} \gets c \cr
 /// 7 & \quad \textbf{return}\ w \text{ with the sign of } a b
 /// \end{array}}
 /// \f]
 /// \post `multiplyKaratsuba( a , b )` and `multiplyFFT( a , b )` equal
 ///       `multiplySchool( a , b )` (audit)
 static BigInteger multiplySchool( const BigInteger & a ,
                                   const BigInteger & b );
 /// the product by Karatsuba and by Toom-Cook at every length, never by the
 /// NTT
 ///
 /// In pseudocode (section 4.3.3 of Knuth, beyond the book), the step of
 /// Karatsuba in the subtractive form, \f$ a = a_1 \beta^l + a_0 \f$ and
 /// \f$ b = b_1 \beta^l + b_0 \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{karatsuba}( a , b ) \cr
 /// 2 & \quad z_0 \gets a_0 b_0;\ z_2 \gets a_1 b_1;\ z_1 \gets ( a_0 - a_1
 ///     ) ( b_0 - b_1 ) \cr
 /// 3 & \quad \textbf{return}\ z_2 \beta^{2l} + ( z_0 + z_2 - z_1 ) \beta^l +
 ///     z_0
 /// \end{array}}
 /// \f]
 /// \post the result equals `multiplySchool( a , b )` (audit)
 static BigInteger multiplyKaratsuba( const BigInteger & a ,
                                      const BigInteger & b );
 /// the product by the number theoretic transform at every length
 ///
 /// In pseudocode (sections 4.3.3 and 4.6.4 of Knuth, beyond the book),
 /// \f$ p_1 , p_2 , p_3 \f$ primes with roots of unity of the length:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{multiplyFFT}( a , b ) \cr
 /// 2 & \quad \textbf{foreach}\ p \in \{ p_1 , p_2 , p_3 \}\ \textbf{do} \cr
 /// 3 & \qquad c^{( p )} \gets \text{the cyclic convolution of the digits }
 ///     \text{(or pieces) modulo } p \cr
 /// 4 & \quad c \gets \text{the coefficients from the } c^{( p )} \text{ by }
 ///     \text{the Chinese remainder theorem} \cr
 /// 5 & \quad \textbf{return}\ \sum_i c_i \beta^i \text{, by the carries}
 /// \end{array}}
 /// \f]
 /// \post the result equals `multiplySchool( a , b )` (audit)
 static BigInteger multiplyFFT( const BigInteger & a , const BigInteger & b );

 /// the quotient \f$ a / b \f$ for \f$ b \f$ a divisor of \f$ a \f$: by one
 /// digit by the inverse of \f$ b \f$ modulo \f$ 2^{64} \f$ (Hensel's
 /// division, exercise 16 of section 4.3.1 of Knuth), with no division of
 /// two digits by one, in place of \f$ a \f$, else as \f$ a / b \f$
 ///
 /// In pseudocode (Hensel, beyond the book), for \f$ b = d \f$ odd of one
 /// digit (a power of 2 being shifted out first):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{divExact}( a , d ) \cr
 /// 2 & \quad v \gets d^{-1} \bmod 2^{64} \text{, by Newton};\ c \gets 0 \cr
 /// 3 & \quad \textbf{for}\ i \gets 0\ \textbf{to}\ n - 1\ \textbf{do} \cr
 /// 4 & \qquad q_i \gets ( a_i - c ) v \bmod 2^{64} \cr
 /// 5 & \qquad c \gets \lfloor q_i d / 2^{64} \rfloor + [ a_i < c ] \cr
 /// 6 & \quad \textbf{return}\ q
 /// \end{array}}
 /// \f]
 /// \pre \f$ b \ne 0 \f$ and \f$ b \f$ divides \f$ a \f$
 /// \post the result times \f$ b \f$ is \f$ a \f$ (audit)
 static BigInteger divExact( BigInteger a , const BigInteger & b );

 /// \f$ r \gets r + a b \f$, in place when \f$ a \f$ or \f$ b \f$ has one
 /// digit, \f$ r \f$ at least as long as the other and of the sign of the
 /// product; otherwise \f$ r \gets r + a \cdot b \f$ by `operator*()`
 static void addMul( BigInteger & r , const BigInteger & a ,
                     const BigInteger & b );

 /// the power \f$ x^n \f$ by the binary method
 ///
 /// In pseudocode (section 4.6.3 of Knuth, beyond the book), from the
 /// leftmost bit of \f$ n \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{pow}( x , n ) \cr
 /// 2 & \quad \textbf{if}\ n = 0\ \textbf{then return}\ 1 \cr
 /// 3 & \quad y \gets x \cr
 /// 4 & \quad \textbf{foreach}\ \text{bit } \nu \text{ of } n \text{ after }
 ///     \text{its leftmost 1, from the left}\ \textbf{do} \cr
 /// 5 & \qquad y \gets y^2;\ \textbf{if}\ \nu = 1\ \textbf{then}\ y \gets y x
 ///     \cr
 /// 6 & \quad \textbf{return}\ y
 /// \end{array}}
 /// \f]
 static BigInteger pow( const BigInteger & x , unsigned long n );

 /// the integer square root \f$ s = \lfloor \sqrt{a} \rfloor \f$ and
 /// \f$ r = a - s^2 \f$, by Zimmermann's Karatsuba square root
 ///
 /// In pseudocode (Zimmermann; Algorithm 1.12 of Brent and Zimmermann,
 /// beyond the book), \f$ a = A \beta^2 + a_1 \beta + a_0 \f$ normalized,
 /// \f$ \beta \f$ about the fourth root of \f$ a \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{sqrtRem}( a ) \cr
 /// 2 & \quad \textbf{if}\ a \text{ is short}\ \textbf{then return}
 ///     \ \text{the root by doubles and steps of 1} \cr
 /// 3 & \quad ( s' , r' ) \gets \mathrm{sqrtRem}( A ) \cr
 /// 4 & \quad ( q , u ) \gets \mathrm{divMod}( r' \beta + a_1 , 2 s' ) \cr
 /// 5 & \quad s \gets s' \beta + q;\ r \gets u \beta + a_0 - q^2 \cr
 /// 6 & \quad \textbf{if}\ r < 0\ \textbf{then}\ r \gets r + 2 s - 1;\ s
 ///     \gets s - 1 \cr
 /// 7 & \quad \textbf{return}\ ( s , r )
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \ge 0 \f$
 /// \post \f$ s^2 + r = a \f$, \f$ 0 \le r \le 2 s \f$, \f$ s \f$ equal to
 ///       `sqrtNewton( a )` (audits)
 static std::pair< BigInteger , BigInteger > sqrtRem( const BigInteger & a );
 /// the root \f$ \lfloor \sqrt{a} \rfloor \f$ of `sqrtRem()`, with no
 /// remainder at the top level and, most often, no product \f$ q^2 \f$
 /// \pre \f$ a \ge 0 \f$
 /// \post the result equals `sqrtNewton( a )` (audit)
 static BigInteger sqrt( const BigInteger & a );

 /// the integer square root by Newton's method from above
 ///
 /// In pseudocode (Newton's method on the integers, beyond the book), the
 /// iterates decreasing to \f$ \lfloor \sqrt{a} \rfloor \f$:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{sqrtNewton}( a ) \cr
 /// 2 & \quad \textbf{if}\ a = 0\ \textbf{then return}\ 0 \cr
 /// 3 & \quad x \gets 2^{\lceil \ell / 2 \rceil}\quad \triangleright\ \ell
 ///     \text{ the bits of } a \cr
 /// 4 & \quad \textbf{loop} \cr
 /// 5 & \qquad y \gets \lfloor ( x + \lfloor a / x \rfloor ) / 2 \rfloor \cr
 /// 6 & \qquad \textbf{if}\ y \ge x\ \textbf{then return}\ x \cr
 /// 7 & \qquad x \gets y
 /// \end{array}}
 /// \f]
 /// \pre \f$ a \ge 0 \f$
 static BigInteger sqrtNewton( const BigInteger & a );

 /// the greatest common divisor of \f$ |a| \f$ and \f$ |b| \f$, by Lehmer
 ///
 /// In pseudocode (Algorithm 4.5.2L of Knuth, beyond the book), the
 /// quotients of the leading bits applied to the whole numbers:
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{gcd}( a , b )\quad \triangleright\ |a| \ge
 ///     |b| \cr
 /// 2 & \quad \textbf{if}\ b \text{ has one digit}\ \textbf{then return}
 ///     \ \text{the binary gcd of it and } a \bmod b \cr
 /// 3 & \quad \textbf{while}\ b \text{ has two digits or more}\ \textbf{do}
 ///     \cr
 /// 4 & \qquad \textbf{if}\ b \text{ is long}\ \textbf{then}\ \text{reduce }
 ///     \text{the top third by the half gcd} \cr
 /// 5 & \qquad M \gets \text{the steps of Euclid on the leading 128 bits}
 ///     \text{ that are sure} \cr
 /// 6 & \qquad \textbf{if}\ M = I\ \textbf{then}\ ( a , b ) \gets ( b , a
 ///     \bmod b ) \cr
 /// 7 & \qquad \textbf{else}\ ( a , b ) \gets M^{-1} ( a , b ) \cr
 /// 8 & \quad \textbf{return}\ \text{the binary gcd of the last digits}
 /// \end{array}}
 /// \f]
 /// \post it equals `gcdEuclid( a , b )` (audit)
 static BigInteger gcd( const BigInteger & a , const BigInteger & b );

 /// the greatest common divisor of \f$ |a| \f$ and \f$ |b| \f$, by Euclid
 ///
 /// In pseudocode (the solution of exercise 17.2 of the book):
 /// \f[
 /// \class{pseudocode}{\begin{array}{r@{\quad}l}
 /// 1 & \textbf{function}\ \mathrm{gcdEuclid}( a , b ) \cr
 /// 2 & \quad a \gets |a|;\ b \gets |b| \cr
 /// 3 & \quad \textbf{while}\ b \ne 0\ \textbf{do}\ ( a , b ) \gets ( b , a
 ///     \bmod b ) \cr
 /// 4 & \quad \textbf{return}\ a
 /// \end{array}}
 /// \f]
 static BigInteger gcdEuclid( BigInteger a , BigInteger b );

/*--------------------------------------------------------------------------*/
 private:

 using Mag = detail::Digits;

 /// the longest product of the NTT, \f$ 2^{25} \f$ digits, \f$ 2^{26} \f$
 /// pieces of 32 bits
 static constexpr std::size_t kNTTMax = std::size_t( 1 ) << 25;

 BigInteger( bool neg , Mag d );

 static void trim( Mag & d );
 static int compareMag( const Mag & a , const Mag & b );

 /// the comparison of \f$ a \f$ and \f$ b \f$, \f$ n \f$ digits each
 static int cmpN( const Limb * a , const Limb * b , std::size_t n );

 /// \f$ r = a + b \f$, \f$ n_a \ge n_b \f$, \f$ r \f$ of \f$ n_a \f$ digits;
 /// the carry
 static Limb addN( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb );

 /// \f$ r = a - b \f$, \f$ n_a \ge n_b \f$, \f$ r \f$ of \f$ n_a \f$ digits;
 /// the borrow
 static Limb subN( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb );

 /// \f$ r \gets r + x \f$ and \f$ r \gets r - x \f$, \f$ n_r \ge n_x \f$; the
 /// carry and the borrow
 static Limb addTo( Limb * r , std::size_t nr , const Limb * x ,
                    std::size_t nx );
 static Limb subFrom( Limb * r , std::size_t nr , const Limb * x ,
                      std::size_t nx );

 /// \f$ r = a m \f$, \f$ r \gets r + a m \f$ and \f$ r \gets r - a m \f$,
 /// \f$ n \f$ digits; the carry or the borrow
 static Limb mul1( Limb * r , const Limb * a , std::size_t n , Limb m );
 static Limb addMul1( Limb * r , const Limb * a , std::size_t n , Limb m );
 static Limb subMul1( Limb * r , const Limb * a , std::size_t n , Limb m );

 /// \f$ r \gets r + a ( m_0 + m_1 \beta ) \f$, \f$ n \f$ digits,
 /// \f$ r[ n ] \f$ and \f$ r[ n + 1 ] \f$ written
 static void addMul2( Limb * r , const Limb * a , std::size_t n , Limb m0 ,
                      Limb m1 );

 /// \f$ r = |a - b| \f$, \f$ n_a \ge n_b \f$, \f$ r \f$ of \f$ n_a \f$
 /// digits; true if \f$ a < b \f$
 static bool absDiff( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb );

 static Mag addMag( const Mag & a , const Mag & b );
 static Mag subMag( const Mag & a , const Mag & b );   ///< \f$ a \ge b \f$

 /// \f$ a + b \f$, or \f$ a - b \f$ if `bneg` differs from the sign of
 /// \f$ b \f$
 static BigInteger addSigned( const BigInteger & a , const BigInteger & b ,
                              bool bneg );

 /// \f$ r = a b \f$, \f$ r \f$ of \f$ n_a + n_b \f$ digits: Algorithm M, a
 /// square if \f$ a = b \f$
 static void mulSchool( Limb * r , const Limb * a , std::size_t na ,
                        const Limb * b , std::size_t nb );
 static void sqrSchool( Limb * r , const Limb * a , std::size_t n );

 /// \f$ r = a^2 \f$, \f$ r \f$ of \f$ 2 n \f$ digits, \f$ n \le 4 \f$
 static void sqrTile( Limb * r , const Limb * a , std::size_t n );

 /// \f$ r = a b \f$, \f$ n \f$ digits each, on the scratch \f$ w \f$ of
 /// `mulScratch( n )` digits: Algorithm M, from `kKaratsuba` digits
 /// Karatsuba, from `toom` digits Toom-3, from `fft` digits the NTT
 static void mulN( Limb * r , const Limb * a , const Limb * b ,
                   std::size_t n , Limb * w , std::size_t fft ,
                   std::size_t toom );
 static std::size_t mulScratch( std::size_t n );

 /// one step of Karatsuba and of Toom-3 on \f$ n \f$ digits, the products of
 /// the parts by `mulN()`
 static void karatsuba( Limb * r , const Limb * a , const Limb * b ,
                        std::size_t n , Limb * w , std::size_t fft ,
                        std::size_t toom );
 static void toom3( Limb * r , const Limb * a , const Limb * b ,
                    std::size_t n , Limb * w , std::size_t fft ,
                    std::size_t toom );

 static void toom4( Limb * r , const Limb * a , const Limb * b ,
                    std::size_t n , Limb * w , std::size_t fft ,
                    std::size_t toom );

 /// the products of \f$ a \f$ of \f$ n_a \f$ digits by \f$ b \f$ of
 /// \f$ n_b < n_a \f$, \f$ a \f$ in 3 pieces and \f$ b \f$ in 2,
 /// \f$ a \f$ in 4 and \f$ b \f$ in 2; false if the lengths do not fit
 static bool toom32( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb , Limb * w ,
                     std::size_t fft , std::size_t toom );
 static bool toom42( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb , Limb * w ,
                     std::size_t fft , std::size_t toom );

 /// \f$ a \gets a / 3 \f$ for a multiple of 3, by Hensel's division;
 /// \f$ a \gets \lfloor a / 2^s \rfloor \f$, \f$ 0 < s < 64 \f$,
 /// \f$ n > 0 \f$ digits
 static void divExact3( Limb * a , std::size_t n );
 static void rshN( Limb * a , std::size_t n , int s );

 /// \f$ a \gets a / d \f$ for a multiple of \f$ d \f$, \f$ d \f$ a divisor of
 /// \f$ \beta - 1 \f$, \f$ f = ( \beta - 1 ) / d \f$
 static void divExactB1( Limb * a , std::size_t n , Limb f );

 /// \f$ r = a + 2 b \f$ and \f$ r = a - 2 b \f$, \f$ n > 0 \f$ digits, the
 /// carry or the borrow
 static Limb addLsh1( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );
 static Limb subLsh1( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );

 /// \f$ r = ( a + b ) / 2 \f$ and \f$ r = ( a - b ) / 2 \f$, \f$ n > 0 \f$
 /// digits, the carry or the borrow the top bit
 static void rsh1Add( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );
 static void rsh1Sub( Limb * r , const Limb * a , const Limb * b ,
                      std::size_t n );

 /// \f$ r = a b \f$ by the NTT, \f$ n_a + n_b \le \f$ `kNTTMax`; by the
 /// transforms of \f$ n \f$ terms on the lanes of `O`
 static void mulNTT( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb );
 template< class O >
 static void mulNTT3( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb , std::size_t n );
 template< class O >
 static void mulFNTT( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb , std::size_t n ,
                      bool cyclic = false );

 /// \f$ r = a b \bmod ( \beta^n - 1 ) \f$, \f$ n \f$ digits, by the cyclic
 /// convolution of \f$ n \f$ terms: \f$ n \f$ a power of 2 from 16 or 3
 /// times one from 24, \f$ n \le 2^{22} \f$, \f$ n_a , n_b \le n \f$ and
 /// \f$ \min( n_a , n_b ) \le 2^{21} \f$
 static void mulCyclic( Limb * r , const Limb * a , std::size_t na ,
                        const Limb * b , std::size_t nb , std::size_t n );

 /// \f$ r = a b \f$, \f$ r \f$ of \f$ n_a + n_b \f$ digits, by the method of
 /// the lengths; the NTT from `fft` digits
 static void mulP( Limb * r , const Limb * a , std::size_t na ,
                   const Limb * b , std::size_t nb ,
                   std::size_t fft = kFFT , std::size_t toom = kToom3 );
 static Mag mulMag( const Mag & a , const Mag & b , std::size_t fft = kFFT ,
                    std::size_t toom = kToom3 );

 /// `mulP()` for \f$ n_a \ge n_b \ge \f$ `kKaratsuba` below the NTT, apart
 /// so that the short products do not set up its scratch on the stack
 static void mulRec( Limb * r , const Limb * a , std::size_t na ,
                     const Limb * b , std::size_t nb , std::size_t fft ,
                     std::size_t toom );

 /// \f$ a \gets \lfloor a / d \rfloor \f$, the remainder; the same for
 /// \f$ d = d_n / 2^s \f$, \f$ d_n \f$ with the top bit set and its
 /// reciprocal \f$ v \f$; the remainder of \f$ a \f$ by \f$ d \f$
 static Limb divSmall( Mag & a , Limb d );
 static Limb divSmall( Mag & a , Limb dn , Limb v , int s );
 static Limb modSmall( const Limb * a , std::size_t n , Limb d );

 /// \f$ a \gets a / d \f$, \f$ d \f$ a divisor of \f$ a \f$, by Hensel's
 /// division
 static void divExactSmall( Mag & a , Limb d );

 /// \f$ a \gets a m + c \f$
 static void mulAddSmall( Mag & a , Limb m , Limb c );

 /// the quotient of \f$ a \f$, \f$ n + m \f$ digits, by \f$ b \f$,
 /// \f$ n \ge 2 \f$ digits with the top bit set, \f$ a < 2 b \, 2^{64 m} \f$:
 /// its \f$ m \f$ low digits in \f$ q \f$ and the top one, 0 or 1, returned,
 /// the remainder in \f$ a[ 0 , n ) \f$; by Algorithm D, and recursively for
 /// \f$ m \le n \f$ with the scratch \f$ t \f$ of \f$ n \f$ digits; with
 /// `cut`, the digit \f$ j \f$ of the quotient by the \f$ j + 2 \f$ leading
 /// digits of \f$ b \f$ alone, which leaves a quotient one off at most and no
 /// remainder [see `divModMag()`], or 2 returned for a digit of \f$ b \f$ or
 /// more
 static Limb divBase( Limb * a , std::size_t m , const Limb * b ,
                      std::size_t n , Limb * q , bool cut = false );
 /// the digits of the quotient \f$ K \f$ at a time as `divBase()` takes
 /// them, while \f$ m \ge K \f$ and the divisor, cut or not, has `least`
 /// digits or more; false, or true if a cut divisor gives a digit too large
 /// [see `divBase()`]
 template< std::size_t K >
 static bool divBlocks( Limb * a , std::size_t & m , const Limb * b ,
                        std::size_t n , Limb * q , bool cut , Limb d1 ,
                        Limb d0 , Limb v , std::size_t least );
 static Limb divRec( Limb * a , std::size_t m , const Limb * b ,
                     std::size_t n , Limb * q , Limb * t );

 /// \f$ x \f$, \f$ n \f$ digits, such that \f$ X = \beta^n + x \f$ has
 /// \f$ a X < \beta^{2n} \le a ( X + 2 ) \f$, for \f$ a \f$ of \f$ n \f$
 /// digits with the top bit set: by Newton's method (Brent and Zimmermann,
 /// Algorithm 3.5), from `kRecip` digits
 static void recip( Limb * x , const Limb * a , std::size_t n );

 /// the quotient of \f$ u \f$ by \f$ v \f$, \f$ 2 \le |v| < \f$ `kDivShort`
 /// and \f$ |u| < 2 \f$ `kDivShort` digits, \f$ u \ge v \f$, and the
 /// remainder if `r`, by Algorithm D on the stack
 static void divShort( const Mag & u , const Mag & v , Mag & q , Mag * r );

 /// as `divBase()`, by blocks of quotient digits from the reciprocal of the
 /// top digits of \f$ b \f$, the products by \f$ b \f$ modulo
 /// \f$ \beta^N - 1 \f$ where the NTT gives them (Barrett; Brent and
 /// Zimmermann, section 2.4.1); the reciprocal is kept in `keep`, if given,
 /// for the next divisions by \f$ b \f$, whose top digits serve the shorter
 /// blocks; with `quot`, the last block skips the remainder when its
 /// quotient cannot reach its last digit, which leaves the digits above it
 /// exact
 static Limb divNewton( Limb * a , std::size_t m , const Limb * b ,
                        std::size_t n , Limb * q , Mag * keep = nullptr ,
                        bool quot = false );

 /// the quotient and the remainder of the magnitudes, recursive or by
 /// Algorithm D alone if `school`, the reciprocal kept in `keep` as by
 /// `divNewton()`, the remainder left empty unless `rem`
 static std::pair< Mag , Mag > divModMag( const Mag & u , const Mag & v ,
                                          bool school = false ,
                                          Mag * keep = nullptr ,
                                          bool rem = true );

 /// the matrix of a half gcd, \f$ ( a ; b ) = M ( \alpha ; \beta ) \f$ with
 /// \f$ \det M = 1 \f$: its entries \f$ u_{00} , u_{01} , u_{10} , u_{11} \f$
 /// in \f$ p[ 0 .. 3 ] \f$, \f$ n \f$ digits each, room for `alloc`, for the
 /// half gcd of `len` digits
 struct HMat {
  explicit HMat( std::size_t len );
  Mag d;
  Limb * p[ 4 ];
  std::size_t n , alloc;
  };

 /// the matrix of one digit per entry of `hgcd2()`,
 /// \f$ u_{00} , u_{01} , u_{10} , u_{11} \f$
 struct HMat1 { Limb u[ 4 ]; };

 /// the steps of Euclid on the leading two digits \f$ ( a_h , a_l ) \f$ and
 /// \f$ ( b_h , b_l ) \f$ of two numbers while their remainders keep more
 /// than 65 bits, in \f$ M \f$: false if none can be taken
 static bool hgcd2( Limb ah , Limb al , Limb bh , Limb bl , HMat1 & M );

 /// \f$ ( a ; b ) \gets M^{-1} ( a ; b ) \f$, \f$ n \f$ digits, in place; the
 /// new length
 static std::size_t hgcdInv1( const HMat1 & M , Limb * a , Limb * b ,
                              std::size_t n );

 /// \f$ M \gets M M_1 \f$, \f$ M_1 \f$ of one digit per entry;
 /// \f$ M \gets M M_1 \f$; the column \f$ c \f$ of \f$ M \f$ plus \f$ q \f$
 /// times the other, \f$ q \f$ of \f$ q_n \f$ digits
 static void hgcdMul1( HMat & M , const HMat1 & M1 );
 static void hgcdMul( HMat & M , const HMat & M1 );

 /// \f$ M \gets M M_1 \f$ by the 7 products of Winograd's form of
 /// Strassen's, from `kHgcdMul7` digits of both
 static constexpr std::size_t kHgcdMul7 = 32;
 static void hgcdMul7( HMat & M , const HMat & M1 );
 static void hgcdUpdate( HMat & M , const Limb * q , std::size_t qn ,
                         unsigned c );

 /// \f$ ( a ; b ) \f$ of \f$ n \f$ digits with their top \f$ n - p \f$
 /// already reduced by \f$ M \f$: their low \f$ p \f$ digits times
 /// \f$ M^{-1} \f$ added; the new length
 static std::size_t hgcdAdjust( const HMat & M , std::size_t n , Limb * a ,
                                Limb * b , std::size_t p );

 /// one subtraction and one division of \f$ ( a ; b ) \f$, \f$ n \f$ digits
 /// each, the larger by the smaller, none that leaves \f$ s \f$ digits or
 /// less; the new length, 0 if none, or with \f$ s = 0 \f$ when the gcd is
 /// found, in \f$ g \f$; the quotients into \f$ M \f$ if given
 static std::size_t gcdSubdiv( Limb * a , Limb * b , std::size_t n ,
                               std::size_t s , HMat * M , Mag * g );

 /// a step of `hgcd()`: `hgcd2()` on the leading digits, else `gcdSubdiv()`
 static std::size_t hgcdStep( std::size_t n , Limb * a , Limb * b ,
                              std::size_t s , HMat & M );

 /// the half gcd of \f$ ( a ; b ) \f$, \f$ n \f$ digits each (Moeller):
 /// reduced in place until \f$ |a - b| \f$ has \f$ \lfloor n / 2 \rfloor + 1
 /// \f$ digits, \f$ M \f$ the matrix; the new length, 0 if no step could be
 /// taken
 static std::size_t hgcd( Limb * a , Limb * b , std::size_t n , HMat & M );

 /// the greatest common divisor of two numbers of two digits, binary
 static Mag gcd22( Limb u1 , Limb u0 , Limb v1 , Limb v0 );

 /// the greatest common divisor of the magnitudes, by Lehmer with the
 /// leading two digits, from `kGcdDC` digits by the half gcd
 static Mag gcdMag( const Mag & x , const Mag & y );

 /// the figures of \f$ a \f$ in the base appended to `out`, at least `pad`
 /// of them, dividing by \f$ P \f$, then those of \f$ b \f$, if given, at
 /// least `padb`, divided with them
 static void figures( Mag a , std::size_t pad , int base ,
                      std::string & out , Mag * b = nullptr ,
                      std::size_t padb = 0 );

 /// the figures of \f$ x \f$ appended to `out`, at least `pad`, by halves
 /// from `kConvert` digits, \f$ \mathit{pw}[ i ] \beta^{\mathit{zs}[ i ]} =
 /// P^{2^i} \f$ with \f$ P \f$ holding \f$ k \f$ figures,
 /// \f$ \mathit{rc}[ i ] \f$ the reciprocal of \f$ \mathit{pw}[ i ] \f$ kept
 /// by the divisions [see `divNewton()`]; a remainder
 /// \f$ x < P^{2^j} \f$, \f$ j \f$ given, of up to `kConvertF` digits by
 /// fractions
 static void toFigures( const Mag & x , std::size_t pad , int base ,
                        const std::vector< Mag > & pw ,
                        const std::vector< std::size_t > & zs , int k ,
                        std::vector< Mag > & rc , std::vector< Mag > & rf ,
                        std::string & out , std::size_t j );

 /// the \f$ 2^j k \f$ figures of \f$ x < P^{2^j} \f$ appended to `out`, by
 /// the fraction \f$ x / P^{2^j} \f$, made by the reciprocal of
 /// \f$ P^{2^j} \f$ kept in \f$ \mathit{rf}[ j ] \f$ [see `fracTree()`]
 static void fracFigures( const Mag & x , std::size_t j , int base , int k ,
                          Limb P , const std::vector< Mag > & pw ,
                          const std::vector< std::size_t > & zs ,
                          std::vector< Mag > & rf , std::string & out );

 /// the \f$ C k \f$ figures of the fraction \f$ f \f$, of \f$ C = 2^j \f$
 /// chunks, to `out`: the high half from the top digits of \f$ f \f$, the
 /// low half from the fraction of \f$ f P^{C/2} \f$, both rounded up, by
 /// halves down to `kFracLeaf` chunks, which are the integer parts of
 /// \f$ f P \f$, \f$ f P^2 \f$ and so on
 static void fracTree( Mag & f , std::size_t j , int base , int k , Limb P ,
                       const std::vector< Mag > & pw ,
                       const std::vector< std::size_t > & zs , bool up ,
                       char * out );
 static void fracLeaf( Mag & f , std::size_t c , Limb P , int k , int base ,
                       bool up , char * out );

 /// the digits \f$ [ s , e ) \f$ of \f$ a b \f$, \f$ n_a > n_b \f$, short of
 /// the exact ones by 2 at most, from the products \f$ a_i b_j \f$ with
 /// \f$ s - 5 \le i + j < e + 3 \f$ only, in \f$ r \f$ from the place
 /// \f$ s \f$ on; false, \f$ r \f$ unchanged, if it does not apply
 static bool mulBand( Limb * r , const Limb * a , std::size_t na ,
                      const Limb * b , std::size_t nb , std::size_t s ,
                      std::size_t e );

 /// the sum of \f$ c[ j ] P^j \f$ for \f$ j < n \f$, by halves from
 /// `kConvert` digits, \f$ \mathit{pw}[ i ] = P^{2^i} \f$
 static Mag fromChunks( const Limb * c , std::size_t n ,
                        const std::vector< Mag > & pw , Limb P );

 /// the root \f$ s \f$ and the remainder \f$ r \f$ of \f$ h \beta + l \f$,
 /// \f$ h \ge 2^{62} \f$, by Newton's method; of \f$ m \f$ of \f$ 2 k \f$
 /// digits, its top one at least \f$ \beta / 4 \f$, recursively, with `root`
 /// the remainder left empty when it is not needed
 static void sqrt2( Limb h , Limb l , Mag & s , Mag & r );
 static void sqrtRec( const Limb * m , std::size_t k , Mag & s , Mag & r ,
                      bool root = false );

 /// the root of \f$ a \f$ and, if `rem`, the remainder; for the root alone,
 /// `sqrtRec()` may leave \f$ r \f$ empty
 static std::pair< Mag , Mag > sqrtMag( const Mag & a , bool rem );

 /// `sqrtRec()` on the stack for \f$ k \le \f$ `kSqrtSmall`: \f$ s \f$ of
 /// \f$ k \f$ digits, \f$ r \f$ of \f$ k + 1 \f$ written; the length of
 /// \f$ r \f$, 0 with `root` when it is not needed
 static constexpr std::size_t kSqrtSmall = 64;
 static std::size_t sqrtN( const Limb * m , std::size_t k , Limb * s ,
                           Limb * r , bool root = false );

 /// the greatest common divisor of two numbers below \f$ 2^{64} \f$, binary
 static std::uint64_t gcdWord( std::uint64_t a , std::uint64_t b );

 /// the greatest common divisor of \f$ a \f$, \f$ n > 0 \f$ digits, not 0,
 /// and \f$ d \ne 0 \f$
 static Limb gcdLimb( const Limb * a , std::size_t n , Limb d );

#if ADS_BIGINT_GMP
 /// \f$ z \f$ as the read-only view of the \f$ n \f$ digits \f$ d \f$, a
 /// magnitude
 static mpz_srcptr gmpView( mpz_t z , const Limb * d , std::size_t n );

 /// the digits of the magnitude of \f$ z \f$
 static Mag gmpDigits( mpz_srcptr z );

 /// true if the quotient of \f$ u \f$ digits by \f$ v \ge 2 \f$ digits goes
 /// to GMP, with its remainder if `rem`
 static bool gmpDivides( std::size_t u , std::size_t v , bool rem );
#endif

 [[nodiscard]] bool normal( void ) const;

 bool f_neg = false;
 Mag f_d;                   ///< the digits, the least significant first

 };  // end( class BigInteger )

/*--------------------------------------------------------------------------*/

}  // end( namespace ADS )

/*--------------------------------------------------------------------------*/
/*-------------------------- IMPLEMENTATION --------------------------------*/
/*--------------------------------------------------------------------------*/

#include "ADS/numeric/impl/BigInteger.tpp"

/*--------------------------------------------------------------------------*/

#endif  /* BigInteger.h included */

/*--------------------------------------------------------------------------*/
/*------------------------- End File BigInteger.h --------------------------*/
/*--------------------------------------------------------------------------*/
