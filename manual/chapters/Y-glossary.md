# Glossary {#glossary}

An alphabetical reference for the terms used in this manual: the abstract
data types and the structures that realize them, the properties and the
costs by which they are analyzed, the techniques of the algorithms, and the
terms of C++ and of the library that the chapters rely on. Each entry gives
a working definition and a pointer to the section where the term is defined
or treated at length; the classes of the library are described in their
chapters and, in full, in the Doxygen reference.

**2-4 tree.** A rooted tree whose leaves are all at the same depth (the
height property) and whose internal nodes have 2, 3 or 4 children (the
degree property); as a search tree, a node of $k$ children holds $k - 1$
keys ([§8.10.1](08-tree.md#sec-8-10-1)).

**A/A measure.** The run of the same structure twice in one benchmark,
whose relative difference gives the noise against which the ratios of a
table are read ([§B.3](B-benchmarks.md#sec-B-3)).

**ABA problem.** The failure of a compare-and-swap on a pointer that finds
the value it read, although the object was removed and another one put at
the same address in between ([§24.1](24-concurrent.md#sec-24-1)).

**Abstract data type.** A type defined by its domains, its operations and
the axioms or contracts that fix their meaning, independently of any
representation; formally, a specification $( D , F , A )$
([§1.2](01-introduction.md#sec-1-2)).

**Abstraction.** The requirement that a program using an abstract data type
does not depend on how the type is realized
([§1.2](01-introduction.md#sec-1-2)).

**Access.** The operation of a link-cut tree that makes the path from the
root of the tree of a node $v$ to $v$ preferred, with $v$ at the root of
its splay tree; every other operation is one or two accesses
([§8.13.2](08-tree.md#sec-8-13-2)).

**Acquire.** The order of an atomic load which, reading the value of a
store with release, makes the writes that preceded that store visible to
the loading thread ([§24.1](24-concurrent.md#sec-24-1)).

**Activity.** The score of a variable in a CDCL solver, increased at each
conflict for the variables met in its analysis and decayed over time; the
decisions take the free variable of largest activity
([§19.7](19-backtrack.md#sec-19-7)).

**Acyclic graph.** A graph with no cycle: a DAG if oriented, a forest if
not, and a free tree if it is also connected
([§14.1](14-graph.md#sec-14-1)).

**Adaptive radix tree.** The engineered ordered dictionary of strings of
Leis et al., a trie whose nodes change their layout with the number of
their children, and whose chains of single children are compressed
([§7.10.3](07-dictionary.md#sec-7-10-3)).

**Adjacency lists.** The representation of a graph that keeps, for each
node, the list of its adjacent nodes, in $\Theta( n + m )$ space
([§14.3](14-graph.md#sec-14-3)).

**Adjacency matrix.** The $n \times n$ matrix whose entry $( u , v )$ is
the weight of the edge $( u , v )$, or empty when there is none; it takes
$\Theta( n^2 )$ space and answers whether an edge exists in $O( 1 )$ time
([§14.2](14-graph.md#sec-14-2)).

**Aho and Corasick automaton.** The automaton on the trie of $k$ patterns,
completed by failure and output links, that finds all their occurrences in
one scan of the text ([§15.3](15-string.md#sec-15-3)).

**Algebraic specification.** The semantic part of a specification given as
equations among the operations, those that observe or remove being applied
to the terms built by those that construct (e.g., $\mathrm{top}(
\mathrm{push}( S , v ) ) = v$) ([§1.2](01-introduction.md#sec-1-2)).

**Algorithm X.** The backtrack of Knuth for the exact cover, which covers
at each level the primary item with fewest options and tries each of its
options in turn; ADS runs it on the dancing links
([§19.5](19-backtrack.md#sec-19-5)).

**Alias method.** The sampling of a discrete distribution in $O( 1 )$ time
per draw, after a preprocessing that splits the probabilities into $n$
columns of at most two outcomes each ([§17.9](17-numeric.md#sec-17-9)).

**Alignment.** A way of writing two strings one above the other with spaces
inserted, scored column by column; it is global, semiglobal or local
according to where the spaces are free and where the score is read
([§15.4.3](15-string.md#sec-15-4-3)).

**Alphabet.** A finite, totally ordered set of $\sigma$ symbols, e.g., the
bytes of a string or the integers of $[ 0 , K )$
([§15.1](15-string.md#sec-15-1)).

**Alternating order.** The order $A[ 0 ] \le A[ 1 ] \ge A[ 2 ] \le \cdots$
of a vector, obtained by one pass of $n - 1$ comparisons
([§2.17](02-sequence.md#sec-2-17)).

**Alternating path.** A path whose edges are in turn out of a matching and
in it; it is augmenting when it joins two free nodes
([§14.11.2](14-graph.md#sec-14-11-2)).

**Amortized bound.** A bound $f( n )$ that holds on average over every
sequence of operations: any $m$ operations from an empty structure take at
most $m f( n )$ time, although a single one may take longer
([§1.7.2](01-introduction.md#sec-1-7-2)).

**Amortized cost.** The share of an operation in the total cost of any
sequence of operations, which may be much less than its cost in the worst
case ([§13.2](13-algorithm.md#sec-13-2)).

**Anagrams.** Two words one of which is a permutation of the other; they
are grouped by a key that does not depend on the order of the letters
([§15.7.3](15-string.md#sec-15-7-3)).

**Anchor.** A string that a front-coded dictionary stores whole, from which
the strings after it are decoded ([§15.6.2](15-string.md#sec-15-6-2)).

**Approximation algorithm.** An algorithm that gives a feasible solution
whose cost is within a relative error $e$ of the optimum on every instance
([§20.7](20-complexity.md#sec-20-7)).

**Approximation scheme.** An algorithm that takes $\epsilon > 0$ in input
and gives a solution within relative error $\epsilon$, in a time polynomial
in the size of the instance (and, for a fully polynomial one, in $1 /
\epsilon$) ([§20.7](20-complexity.md#sec-20-7)).

**Arena.** An allocator that gives the objects of one type from large
blocks by the bump of a pointer and frees them all at once with the arena;
also called a region ([§5.4](05-memory.md#sec-5-4)).

**Arithmetic code.** The code of a whole text as one number in an interval
that each symbol narrows in proportion to its probability, which comes
within about 2 bits of the entropy of the text
([§21.4](21-compression.md#sec-21-4)).

**Arithmetic expression.** An expression on operands and the binary
operators, written in infix, prefix or postfix notation; a stack evaluates
the postfix one and translates the infix one into it
([§3.6](03-stack.md#sec-3-6)).

**Array container.** The form of a container of a compressed bitmap that
holds the sorted low 16 bits of its items, used when the chunk holds at
most 4096 items ([§10.5](10-set.md#sec-10-5)).

**Assignment problem.** The problem of matching $n$ rows to $n$ columns,
each pair with a cost, at the least total cost, i.e., the perfect matching
of least cost in a complete bipartite graph
([§14.11.3](14-graph.md#sec-14-11-3)).

**Asymmetric numeral systems.** The entropy coder of Duda, which replaces
the interval of the arithmetic code with a single integer state, changed by
each symbol according to its scaled frequency
([§21.5](21-compression.md#sec-21-5)).

**Asymptotic notation.** The notations $O$, $\Omega$, $\Theta$, $o$ and
$\omega$, which compare functions up to constant factors for large
arguments ([§1.5.3](01-introduction.md#sec-1-5-3)).

**Atom.** In the cell heap of a LISP, a field holding an integer rather
than nil or the address of a cell ([§5.10](05-memory.md#sec-5-10)).

**Atomic item.** In the external-memory model, an item that is moved and
compared whole, a pair or a triple of the algorithms counting as one item
([§23.1](23-external.md#sec-23-1)).

**Audit.** A contract whose check costs more than the operation it guards
(e.g., that a vector is sorted, before a binary search), stated by
`ADS_AUDIT` and evaluated from the level 3
([§1.7.1](01-introduction.md#sec-1-7-1)).

**Augmented map.** A map on a balanced tree whose nodes also keep the
value, under an associative operation, of a function of the keys of their
subtree ([§8.12.6](08-tree.md#sec-8-12-6)).

**Augmenting path.** An alternating path that joins two free nodes; its
symmetric difference with the matching is a matching with one edge more
([§14.11.2](14-graph.md#sec-14-11-2)).

**AVL tree.** A binary search tree in which the heights of the two subtrees
of each node differ by at most 1, which bounds its height by about $1.44
\log_2 n$ ([§8.7](08-tree.md#sec-8-7)).

**Axiom.** An equation among the operations of an abstract data type that
fixes their meaning, such as $\mathrm{pop}( \mathrm{push}( S , v ) ) = S$
for the STACK ([§1.2](01-introduction.md#sec-1-2)).

**Back.** The end of a queue at which the items are added ([Chapter
4](04-queue.md#ch-4)).

**Backtrack.** The enumeration of the solutions of a problem by a visit in
depth of the tree of the choices, which extends a partial solution one item
at a time and goes back when it cannot be extended
([§19.1](19-backtrack.md#sec-19-1)).

**Backward search.** The count of the occurrences of a pattern in the
FM-index, which reads the pattern from its end and narrows a range of rows
of the sorted rotations by two ranks per symbol
([§21.11](21-compression.md#sec-21-11)).

**Bag.** A collection that may hold equal items, whose `find( x )` gives
one item equal to $x$ and `findAll( x )` all of them
([§1.3.3](01-introduction.md#sec-1-3-3)).

**Balanced parentheses.** A string of as many opening as closing
parentheses, no prefix of which has more closing than opening ones;
succinct trees code their shape this way
([§18.7](18-combinatorial.md#sec-18-7)).

**Band of Ukkonen.** The restriction of the table of the edit distance to
the diagonals within the distance sought, which brings its cost down to $O(
k n )$ for distance $k$ ([§15.4.5](15-string.md#sec-15-4-5)).

**Base of a blossom.** The node of a blossom nearest to the root of the
alternating tree, the only node of the cycle that may be matched outside it
([§14.11.2](14-graph.md#sec-14-11-2)).

**Bellman, Ford and Moore algorithm.** The shortest paths with a queue as
the frontier of the prototype, which allows negative costs and takes $O( n
m )$ time ([§14.8.1](14-graph.md#sec-14-8-1)).

**Best fit.** The rule of an allocator that reads the whole free list and
takes the smallest block large enough for the request
([§5.2](05-memory.md#sec-5-2)).

**Big integer.** The integer of any length, kept as a sign and the vector
of its limbs in base $2^{64}$ ([§17.3](17-numeric.md#sec-17-3)).

**Binary counter.** The example of the amortized analysis: an increment of
a counter of $k$ bits flips $O( 1 )$ bits amortized, though one increment
may flip all $k$ ([§13.2](13-algorithm.md#sec-13-2)).

**Binary fuse filter.** A static filter whose cells are the solution of a
linear system over $\mathbb{F}_2$, with the three places of a key in
consecutive segments (spatial coupling), which lowers the space to about
$1.125 n$ cells ([§10.10](10-set.md#sec-10-10)).

**Binary gcd.** The greatest common divisor computed by shifts and
subtractions instead of divisions ([§17.8](17-numeric.md#sec-17-8)).

**Binary heap.** A complete binary tree, laid out in a vector, whose nodes
are heap-ordered; it inserts and removes the minimum in $O( \log n )$ time
([§9.3](09-priority.md#sec-9-3)).

**Binary packing.** The code of a block of integers with the same number of
bits for each, the bits of its largest one, decoded by a fixed sequence of
shifts and masks ([§21.8](21-compression.md#sec-21-8)).

**Binary search.** The search of a sorted vector that compares the item
sought with the middle item of the part still to examine and halves it, in
$O( \log n )$ comparisons ([§6.2](06-search.md#sec-6-2)).

**Binary search tree.** A binary tree whose nodes hold keys so that, for
every node, the keys of its left subtree are smaller and those of its right
subtree larger than its own (the binary search tree property)
([§8.5](08-tree.md#sec-8-5)).

**Binary tree.** An ordered tree whose nodes have at most two children,
told apart as the left and the right child even when only one of them is
there ([§8.1](08-tree.md#sec-8-1)).

**Binary trie.** The trie of integers of $w$ bits, a binary tree whose
leaves are all at depth $w$, each integer being the path from the root to
its leaf ([§7.11.1](07-dictionary.md#sec-7-11-1)).

**Bit vector.** A vector of bits packed in words of 64 bits, the base of
the succinct structures, with rank and select added by a directory
([§22.2](22-succinct.md#sec-22-2)).

**Bit vectors of Myers.** The computation of a column of the table of the
edit distance in $O( 1 )$ operations on words, by the vertical and
horizontal differences of adjacent entries packed as bits
([§15.4.5](15-string.md#sec-15-4-5)).

**Bitmap container.** The form of a container of a compressed bitmap that
holds the $2^{16}$ bits of its chunk in 1024 words, used when the chunk
holds more than 4096 items ([§10.5](10-set.md#sec-10-5)).

**Bitmap trie.** The trie of Morin's exercise 13.3, which extends the
binary trie from strings of $w$ bits to strings of bytes of any length
([§7.11.4](07-dictionary.md#sec-7-11-4)).

**Bitonic sequence.** A sequence that first increases and then decreases,
or a rotation of one; the bitonic sorter of Batcher merges such sequences
by half-cleaners ([§12.12](12-sort.md#sec-12-12)).

**Black height.** The number of black nodes on the paths from a node down
to a leaf of a red-black tree, the same along all of them
([§8.5.5](08-tree.md#sec-8-5-5)).

**Black token.** The extra unit of black that the removal from a Red-Black
tree moves up the path until a red node absorbs it or a rotation disposes
of it ([§8.5.5](08-tree.md#sec-8-5-5)).

**Blind search.** The search of a pattern in a Patricia trie that follows
the branching characters only, reads a single string at the leaf reached,
and then finds the lexicographic position from the first mismatch
([§7.10.2](07-dictionary.md#sec-7-10-2)).

**Block (allocation).** A run of contiguous words that an allocator gives
for a request and that no other block overlaps
([§5.1](05-memory.md#sec-5-1)).

**Block store.** A collection of blocks identified by their indices, read
and written whole, which hides the devices of the external memory
([§23.2](23-external.md#sec-23-2)).

**Blocked filter of Bloom.** A filter of Bloom whose $k$ bits of a key lie
in one line of cache, so that a query misses the cache once, at the price
of a somewhat larger false positive rate ([§10.8](10-set.md#sec-10-8)).

**Blocking.** A concurrent implementation in which a thread stopped in the
middle of an operation may keep the other threads from completing theirs,
as a thread holding a lock does ([§24.1](24-concurrent.md#sec-24-1)).

**Bloom filter.** The filter that sets $k$ bits, chosen by $k$ hash
functions, of a vector of $m$ bits for each key, and answers that a key may
be in the set if all its $k$ bits are set ([§10.7](10-set.md#sec-10-7)).

**Blossom.** An odd cycle closed by an edge between two even nodes of an
alternating tree, which the algorithm of Edmonds shrinks to a single node
([§14.11.2](14-graph.md#sec-14-11-2)).

**Boolean vector set.** The SET of the integers of $[ 0 , N )$ kept as a
vector of $N$ booleans, one per integer of the universe
([§10.2](10-set.md#sec-10-2)).

**Border.** A string that is both a proper prefix and a proper suffix of a
given string ([§15.1](15-string.md#sec-15-1)).

**Borrowing.** The repair of a node of a B-tree (or of a 2-4 tree) with too
few keys or children, which takes one from a sibling through the key of the
parent between them ([§8.11.1](08-tree.md#sec-8-11-1)).

**Boundary tag.** The size of a block written at both its ends, with its
sign telling whether the block is free, so that a freed block finds and
merges its free neighbors in $O( 1 )$ time ([§5.2](05-memory.md#sec-5-2)).

**Boyer and Moore search.** The search of a pattern that compares it with
the text from its end and shifts it by the occurrence of the mismatched
character (the rule of Horspool) ([§15.2.1](15-string.md#sec-15-2-1)).

**Broadword computation.** The treatment of the 64 bits of a word as a
vector of small fields, processed together by the arithmetic and logical
instructions of the processor ([§18.2](18-combinatorial.md#sec-18-2)).

**B-tree.** A search tree whose nodes hold several keys each, so that the
tree is shallow and a search reads few nodes; in the main memory a node
fills a few lines of cache, on the disk a block
([§8.11](08-tree.md#sec-8-11)).

**B+-tree.** The B-tree of order $m$ that keeps the values in the leaves
only, its internal nodes holding separators, with the leaves linked in a
list in both directions ([§8.11.2](08-tree.md#sec-8-11-2)).

**B-tree of minimum degree $t$.** The B-tree in which each node but the
root holds between $t - 1$ and $2 t - 1$ keys, and an internal node one
child more than its keys ([§8.11.1](08-tree.md#sec-8-11-1)).

**B-tree on the blocks.** The B-tree of the external memory, whose node
fills a block of the disk, so that a search costs $O( \log_B n )$ I/Os
([§23.3.1](23-external.md#sec-23-3-1)).

**Bucket queue.** The monotone priority queue of integer keys that applies
the scan of Dial to two keys, the second breaking the ties toward the last
item pushed, as the search A* wants ([§9.9](09-priority.md#sec-9-9)).

**Buddy system.** An allocator that gives only blocks of $2^k$ words
aligned to their size, splitting a larger block into two buddies and
merging two free buddies back ([§5.3](05-memory.md#sec-5-3)).

**Bulk operation.** An operation on a whole set of keys at once (a union,
an intersection, a difference, a batch of insertions), realized on the
join-based trees by divide and conquer ([§8.12](08-tree.md#sec-8-12)
and [§8.12.4](08-tree.md#sec-8-12-4)).

**Burrows and Wheeler transform.** The permutation of a text that writes
the last column of the sorted rotations of the text with a terminator,
which groups the symbols of equal contexts and can be inverted
([§21.10](21-compression.md#sec-21-10)).

**Butterfly.** The step of the fast Fourier transform that combines the
transforms $E_k$ and $O_k$ of the even and odd halves into the outputs $E_k
\pm \omega^k O_k$ ([§17.3](17-numeric.md#sec-17-3)).

**Cache-oblivious.** Said of an algorithm that is efficient in the
external-memory model without knowing the size $B$ of a block and $M$ of
the memory ([§1.6](01-introduction.md#sec-1-6)).

**Canonical code.** A prefix-free code that gives the symbols of each
length consecutive codewords in the order of the symbols, and therefore is
determined by the lengths alone ([§21.2](21-compression.md#sec-21-2)).

**Carry-save adder.** The circuit, simulated on words, that adds three
vectors of bits place by place into a vector of sums and one of carries; it
speeds up the population count of an array
([§18.3](18-combinatorial.md#sec-18-3)).

**Cartesian tree.** The binary tree of an array whose root is the leftmost
minimum and whose subtrees are the Cartesian trees of the two parts around
it; it reduces the range minima to the lowest common ancestors
([§6.9](06-search.md#sec-6-9)).

**Cascading cut.** In a Fibonacci heap, the cut of a parent that loses its
second child since it became a child, repeated up the tree
([§9.7](09-priority.md#sec-9-7)).

**Catalan numbers.** The numbers $C_n = \binom{2 n}{n} / ( n + 1 )$, which
count the binary trees of $n$ nodes and the strings of $n$ pairs of
balanced parentheses ([§17.5](17-numeric.md#sec-17-5)).

**Categories of iterators.** The classes of iterators by the moves they
allow in constant time: forward, bidirectional, random access and
contiguous ([§1.8](01-introduction.md#sec-1-8)).

**Cell (benchmark).** One structure measured on one case at one size, the
unit of a table of benchmark results ([§B.2](B-benchmarks.md#sec-B-2)).

**Cellar.** The places beyond the $m$ addressed by the hash function in a
table with coalesced chaining, taken first by the overflowing keys
([§7.5](07-dictionary.md#sec-7-5)).

**Certificate (NP).** A string of polynomial length that, checked by a
polynomial algorithm, proves that an instance of a decision problem has the
answer YES ([§20.1](20-complexity.md#sec-20-1)).

**Certificate of optimality.** Data that prove a solution on a graph
optimal by conditions each of which touches a single edge, so that a change
of the graph can be tested against them
([§14.13.2](14-graph.md#sec-14-13-2)).

**Chaining.** The resolution of the collisions of a hash table by a list of
overflow per place, holding the keys that hash there
([§7.4](07-dictionary.md#sec-7-4)).

**Changelog.** The file `CHANGELOG.md` of the repository, in the form of
Keep a Changelog, where each change enters under the section of the release
to come ([§A.9](A-writing-structure.md#sec-A-9)).

**Channel.** The link through which the workers of a parallel computation
exchange batches of messages, each worker draining its own inbox
([§25.13](25-parallel.md#sec-25-13)).

**Chunk.** One of the parts of $2^{16}$ integers into which a compressed
bitmap cuts the universe, by the top 16 bits ([§10.5](10-set.md#sec-10-5)).

**Circular vector.** A vector of capacity $c$ whose items occupy the places
from a head on, modulo $c$, so that items are added and removed at both
ends in $O( 1 )$ time without moving the others
([§2.3](02-sequence.md#sec-2-3)).

**Circulation.** A flow conserved at every node, with a lower and an upper
bound on each edge; its existence reduces to a maximum flow
([§14.10.4](14-graph.md#sec-14-10-4)).

**Clause.** A disjunction of literals; a formula in conjunctive normal form
is a set of clauses ([§19.7](19-backtrack.md#sec-19-7)).

**Closest pair.** A pair of points at the least Euclidean distance among
the $n$ points of the input, found in $O( n \log n )$ time by divide and
conquer ([§16.3](16-geometry.md#sec-16-3)).

**Coalesced chaining.** The open addressing in which the overflowing keys
of a place are linked in lists that may merge, the first places taken being
those of a cellar ([§7.5](07-dictionary.md#sec-7-5)).

**Coalescing.** The merge of adjacent free blocks into one, which fights
the external fragmentation ([§5.1](05-memory.md#sec-5-1)).

**Cocktail shaker sort.** The bubble sort that alternates the passes to the
right and to the left, so that a turtle reaches the front in one pass
([§12.3](12-sort.md#sec-12-3)).

**Code.** A map of each symbol to a binary codeword, and of a text to the
concatenation of the codewords of its symbols
([§21.1](21-compression.md#sec-21-1)).

**Collective operation.** An operation that all the processes of a
communicator call, each with its part, such as a gather, a broadcast or a
reduction ([§26.1](26-distributed.md#sec-26-1)).

**Collision.** Two distinct keys that a hash function maps to the same
place ([§7.1](07-dictionary.md#sec-7-1)).

**Communicator.** The group of the processes of a distributed computation,
numbered by their ranks, within which they send messages and call the
collective operations ([§26.1](26-distributed.md#sec-26-1)).

**Compact representation.** A representation of an object of a class of $N$
elements in $O( \log_2 N )$ bits, i.e., within a constant factor of the
information-theoretic lower bound ([§22.1](22-succinct.md#sec-22-1)).

**Compact trie.** A trie whose chains of nodes with a single child are
contracted into single edges labelled by strings; a Patricia trie keeps
only the first character of each edge
([§7.10.1](07-dictionary.md#sec-7-10-1)).

**Comparison.** The evaluation of the order on two items, which measures
the cost of a search or of a sort by comparisons
([§6.1](06-search.md#sec-6-1)).

**Comparison tree.** The binary tree of the comparisons that a sort by
comparisons makes on $n$ distinct items, whose $n!$ leaves give its lower
bound of $\log_2 n! = \Omega( n \log n )$ comparisons; also called the
decision tree ([§12.2](12-sort.md#sec-12-2)).

**Complete binary tree.** A binary tree whose levels are full but for the
last, whose nodes are as far to the left as possible; it is laid out in a
vector without pointers ([§9.3](09-priority.md#sec-9-3)).

**Compressed bitmap.** A set of integers of 32 bits kept as containers of
the low 16 bits of the items of each chunk, each an array or a bitmap
according to its density ([§10.5](10-set.md#sec-10-5)).

**Compressed index.** An index of the substrings of a text in space close
to that of the compressed text, such as the FM-index on the Burrows and
Wheeler transform ([§21.11](21-compression.md#sec-21-11)).

**Compressed sparse rows.** The layout of a sparse matrix in three vectors,
the nonzero entries by rows, their columns and the start of each row
([§17.7](17-numeric.md#sec-17-7)).

**Compression (union-find).** The heuristic of a find in a forest of
parents that makes each node of the path climbed a child of the root
([§11.3](11-mfset.md#sec-11-3)).

**Concept.** In C++20, a named predicate on types checked at compile time,
which requires some expressions to be valid and to have some types; ADS
writes the syntactic specification of each abstract data type as a concept
([§1.2](01-introduction.md#sec-1-2)).

**Conflict-driven clause learning.** The solver of satisfiability that, at
each conflict, derives a new clause from the implications (up to the first
unique implication point), adds it to the formula and backjumps
([§19.7](19-backtrack.md#sec-19-7)).

**Conflicting clause.** A clause all of whose literals are false under the
current partial assignment ([§19.7](19-backtrack.md#sec-19-7)).

**Conjunctive normal form.** A formula written as a conjunction of clauses,
each a disjunction of literals ([§19.7](19-backtrack.md#sec-19-7)).

**Connected components.** The classes of the nodes of a graph that are
reachable from one another with the orientation of the edges ignored
([§14.1](14-graph.md#sec-14-1)).

**Consolidation.** The step of the removal of the minimum of a Fibonacci
heap that links two roots of equal degree, the larger under the smaller,
until all the degrees differ ([§9.7](09-priority.md#sec-9-7)).

**Constructive specification.** The semantic part of a specification given
as the precondition and the postcondition of each operation, in terms of a
type already known ([§1.2](01-introduction.md#sec-1-2)).

**Constructor.** An operation that makes a value of an abstract data type,
such as `create()` or `push()` ([§1.2](01-introduction.md#sec-1-2)).

**Container (bitmap).** The part of a compressed bitmap that holds the low
16 bits of the items of one chunk, as an array or as a bitmap
([§10.5](10-set.md#sec-10-5)).

**Contract.** A pair of a precondition, which the caller ensures, and a
postcondition, which the operation ensures under it; ADS checks its
contracts at run time by the macros of `ADS/Contract.h`
([§1.7.1](01-introduction.md#sec-1-7-1)).

**Contract level.** The value of the macro `ADS_CONTRACTS`, from 0 to 3,
that chooses which contracts are evaluated: preconditions from 1,
postconditions from 2, invariants and audits from 3
([§1.7.1](01-introduction.md#sec-1-7-1)).

**Convex hull.** The smallest convex set that contains a set of points,
i.e., a convex polygon whose vertices are some of the points
([§16.2](16-geometry.md#sec-16-2)).

**Coordinated scan.** A sorted file of requests read together with the file
that answers them, which costs only the scans of the two files
([§23.1](23-external.md#sec-23-1)).

**Correctness.** The property of a realization that it realizes its
interface, checked in ADS by the contracts and the tests
([§1.7](01-introduction.md#sec-1-7)).

**Counting sort.** The sort by digits of keys in a small range that counts
the occurrences of each key and places the items by the prefix sums of the
counts, stable in $O( n + k )$ time ([§12.10](12-sort.md#sec-12-10)).

**Critical factorization.** A cut $P = u v$ of a pattern whose local period
equals the period of $P$, on which the Two-Way search rests
([§15.2.1](15-string.md#sec-15-2-1)).

**Cuckoo filter.** A filter of fingerprints kept by partial-key cuckoo
hashing in buckets of a few entries, which also supports removals
([§10.9](10-set.md#sec-10-9)).

**Cuckoo graph.** The graph whose vertices are the places of a cuckoo table
and whose edges join the two places of each key; a placement of the keys is
an orientation of the edges in which each vertex receives at most one edge
([§7.7](07-dictionary.md#sec-7-7)).

**Cuckoo hashing.** The hashing that gives each key two places and keeps it
in one of them, evicting the key that occupies it to its other place; a
lookup reads at most two places ([§7.7](07-dictionary.md#sec-7-7)).

**Cursor.** A link given as the index of a place of a vector rather than as
a pointer, so that the cells of linked structures lie in one block of
memory ([§2.11](02-sequence.md#sec-2-11)).

**Cycle.** A path of at least one edge (three for an unoriented graph)
whose two ends coincide ([§14.1](14-graph.md#sec-14-1)).

**Dancing cells.** See sparse set ([§19.6](19-backtrack.md#sec-19-6)).

**Dancing links.** The trick of Knuth by which a node removed from a doubly
linked list keeps its own links, so that it is put back in $O( 1 )$ time
when the backtrack returns ([§19.5](19-backtrack.md#sec-19-5)).

**Data race.** A pair of accesses of two threads to one location, one of
them a write, neither atomic and neither ordered before the other; a
program with a data race has no defined behavior
([§24.1](24-concurrent.md#sec-24-1)).

**Deal.** The move of half the items of one side of a deque on two stacks
to its empty side, when a removal finds it empty
([§4.8](04-queue.md#sec-4-8)).

**Decision problem.** A set of instances, each with the answer YES or NO
([§20.1](20-complexity.md#sec-20-1)).

**Decision tree.** A binary tree whose internal nodes are comparisons and
whose leaves are the answers, the model of a search or a sort by
comparisons ([§6.1](06-search.md#sec-6-1)).

**Degree property.** The property of a 2-4 tree that each internal node has
2, 3 or 4 children ([§8.10.1](08-tree.md#sec-8-10-1)).

**Delta-stepping.** The parallel shortest paths that relax at each step the
active nodes whose distance lies in the current bucket of width $\Delta$
([§25.12](25-parallel.md#sec-25-12)).

**Dense matrix.** A matrix that keeps all its $m n$ entries, in one vector
by rows ([§17.6](17-numeric.md#sec-17-6)).

**Depth (node).** The number of edges from the root of a tree to a node
([§8.1](08-tree.md#sec-8-1)).

**Depth (parallel).** The time of a longest path of the graph of the tasks
of a parallel computation, i.e., its time on infinitely many threads; also
called the span ([§25.1](25-parallel.md#sec-25-1)).

**Deque.** A sequence changed at both ends, by insertions and removals at
its front and at its back; it realizes both the stack and the queue
([Chapter 4](04-queue.md#ch-4)).

**Descendant.** A node of the subtree of a given node, each node being a
descendant (and an ancestor) of itself ([§8.1](08-tree.md#sec-8-1)).

**Descent.** A place $i$ of a sequence with $s_{i+1} < s_i$; a sequence of
$r$ runs has $r - 1$ descents ([§2.16](02-sequence.md#sec-2-16)).

**Design by contract.** The method of Meyer in which a violated
precondition is a defect of the caller and a violated postcondition or
invariant a defect of the operation
([§1.7.1](01-introduction.md#sec-1-7-1)).

**Deterministic reservations.** The scheme of Blelloch et al. that runs a
greedy algorithm in parallel by rounds, in which the items reserve what
they need with their priority and commit if they won all their
reservations, with the result of the sequential order
([§25.10](25-parallel.md#sec-25-10)).

**d-gaps.** The first item of a sorted list followed by the differences of
its consecutive items, small and skewed numbers that the codes of the
integers compress well ([§21.7](21-compression.md#sec-21-7)).

**Dial queue.** The monotone priority queue of Dial, an array of buckets
indexed by the key within a band of values above the last minimum extracted
([§9.9](09-priority.md#sec-9-9)).

**Dictionary.** A univocal relation from keys to values, with `lookup(k)`,
`insert(k, v)` and `remove(k)` ([§7.1](07-dictionary.md#sec-7-1)).

**Dictionary of strings.** A sorted array of strings searched by prefix, as
in the autocompletion of a query ([§15.6](15-string.md#sec-15-6)).

**Direct access table.** The dictionary of the keys of a small interval of
integers, a vector with a place per key of the universe
([§7.2](07-dictionary.md#sec-7-2)).

**Directly addressable codes.** The code of a sequence of integers in
chunks of a few bits, one level of chunks per vector, with a vector of bits
per level telling which integers go on, so that the $i$-th integer is
reached by ranks ([§21.8](21-compression.md#sec-21-8)).

**Distinguishing prefix.** The shortest prefix of a string that
distinguishes it from all the others of a set; the sorts of strings read
each character of these prefixes a bounded number of times
([§12.11](12-sort.md#sec-12-11)).

**Distribution sort.** The external sort that distributes the items by $k -
1$ pivots drawn from a sample into $k$ buckets, each sorted recursively
([§23.4.3](23-external.md#sec-23-4-3)).

**Divide and conquer.** The scheme (divide et impera) that splits an
instance into smaller instances of the same problem, solves them
recursively and combines their solutions
([§13.4](13-algorithm.md#sec-13-4)).

**Division hashing.** The hash function $H( k ) = \mathrm{int}(
\mathrm{bin}( k ) ) \bmod m$, which reads the characters of the key as the
digits of a number and takes the remainder at each step of the rule of
Horner, with $m$ best a prime far from a power of 2
([§7.3.1](07-dictionary.md#sec-7-3-1)).

**Double black.** The temporary colour of the removal from a left-leaning
red-black tree, for a node that counts as two black ones until the repair
([§8.10.2](08-tree.md#sec-8-10-2)).

**Double ended priority queue.** A priority queue that gives and removes
both its best and its worst item in logarithmic time
([§9.10](09-priority.md#sec-9-10)).

**Double hashing.** The open addressing whose inspection steps by a second
hash of the key, $H( k , i ) = ( H_0( k ) + i H_\delta( k ) ) \bmod m$,
which comes close to the uniform hashing
([§7.5](07-dictionary.md#sec-7-5)).

**Doubling vector.** A vector that doubles its capacity when it is full
(and halves it when it is too empty), so that an insertion or a removal at
its end takes $O( 1 )$ amortized time ([§2.2](02-sequence.md#sec-2-2)).

**Doubly linked list.** A list whose cells keep the addresses of the next
and of the previous cell, so that an insertion and a removal at a cell take
$O( 1 )$ time; ADS makes it circular with a sentinel
([§2.10](02-sequence.md#sec-2-10)).

**Dual array deque.** The deque of Morin made of two stacks on vectors
placed back to back, rebalanced when one is much larger than the other
([§2.4](02-sequence.md#sec-2-4)).

**Dyck word.** A sequence of $+1$ and $-1$ whose prefix sums are never
negative, i.e., a sequence of pushes and pops that never pops an empty
stack ([§1.3.1](01-introduction.md#sec-1-3-1)).

**Dynamic filter.** A filter that takes both insertions and removals
([§10.1](10-set.md#sec-10-1)).

**Dynamic PGM-index.** The ordered map of arithmetic keys built from static
PGM-indexes by the logarithmic method, with insertions and removals by
tombstones ([§6.6](06-search.md#sec-6-6)).

**Dynamic programming.** The method that solves a problem with optimal
substructure by keeping the optimal value of each subproblem in a table,
filled in an order in which each entry depends on entries already computed
([§13.7](13-algorithm.md#sec-13-7)).

**Dynamic schedule.** The schedule in which the threads claim the blocks of
a loop one at a time from a shared counter, so that a thread is idle only
when no block is left ([§25.1](25-parallel.md#sec-25-1)).

**Dynamic storage allocation.** The management of a memory from which a
program takes blocks of the sizes it needs, when it needs them, and to
which it gives them back in any order ([Chapter 5](05-memory.md#ch-5)).

**Dynamic tree.** A forest of rooted trees that changes by links and cuts
and answers queries on the paths, such as the root of a node, in $O( \log n
)$ amortized time by the link-cut trees ([§8.13](08-tree.md#sec-8-13)).

**Edit distance.** The least number of substitutions, insertions and
deletions of single symbols that turn one string into another
([§15.1](15-string.md#sec-15-1)).

**Elias and Fano representation.** The code of a non-decreasing sequence of
$n$ integers of $[ 0 , u )$ by their low bits written plainly and their
high bits in unary, in about $n ( 2 + \log_2( u / n ) )$ bits, with access
and successor search ([§22.6](22-succinct.md#sec-22-6)).

**Elias codes.** The codes $\gamma$ and $\delta$ of the positive integers,
which write the length of the binary representation of an integer (in
unary, or by $\gamma$) before its bits
([§21.7](21-compression.md#sec-21-7)).

**Engineered level.** The level of ADS that realizes the variants of the
literature on algorithm engineering, tuned for the memory hierarchy and the
processor and measured by the benchmarks
([§1.9](01-introduction.md#sec-1-9)).

**Entropy of order 0.** The quantity $H_0 = \sum_c P[ c ] \log_2( 1 / P[ c
] )$ of a distribution on the symbols, a lower bound on the average length
of a code of the symbols one by one; it is empirical when $P[ c ]$ is the
frequency of $c$ in the text ([§21.1](21-compression.md#sec-21-1)).

**Entropy of order $k$.** The empirical entropy of a text when each symbol
is predicted from the $k$ symbols before it, the measure of the compressors
that use contexts ([§21.1](21-compression.md#sec-21-1)).

**Enumeration.** The problem of listing all the feasible solutions of an
instance, finding one of them or counting them ([Chapter
19](19-backtrack.md#ch-19)).

**Escape.** In the prediction by partial matching, the event coded when a
context does not predict the next symbol, after which the shorter context
is tried ([§21.6](21-compression.md#sec-21-6)).

**Eulerian cycle.** A closed trail, i.e., a cycle that uses each edge of
the graph exactly once; it exists in a connected graph if and only if every
node has as many edges leaving as entering it (an even degree, if
unoriented) ([§14.12.1](14-graph.md#sec-14-12-1)).

**Event count.** The primitive of Reed and Kanodia by which a thread sleeps
until another thread signals that the condition it waits for may hold,
without missing a signal sent in between
([§24.10](24-concurrent.md#sec-24-10)).

**Event (graph).** A change of a graph (an edge added, removed or
reweighted) that an observable graph sends, as a value of a variant, to the
observers that want its kind ([§14.13.1](14-graph.md#sec-14-13-1)).

**Evert.** The operation of a link-cut tree that makes a node the root of
its tree, by reversing the path from the root to it
([§8.13.3](08-tree.md#sec-8-13-3)).

**Exact cover.** A set of options that contains each primary item exactly
once and each secondary item at most once
([§19.5](19-backtrack.md#sec-19-5)).

**Exact cover with colors.** The exact cover in which an option may give a
color to a secondary item, and several options may share the item if they
give it the same color (Knuth's Algorithm C)
([§19.6](19-backtrack.md#sec-19-6)).

**Exchange argument.** The proof that a greedy choice is safe, which turns
an optimal solution without that choice into another optimal solution with
it by swapping one component ([§13.11](13-algorithm.md#sec-13-11)).

**Exclusion.** The principle of the prediction by partial matching by which
a context that escapes tells the decoder that the symbol is none of those
it predicts, which the shorter contexts then leave out
([§21.6](21-compression.md#sec-21-6)).

**Executor.** An object that runs a batch of calls on its threads and
returns when all have returned; every parallel algorithm of ADS takes one
as its first argument ([§25.2](25-parallel.md#sec-25-2)).

**Expected bound.** A bound on the expected value of the running time, a
random variable over the random choices of the structure, never over the
inputs ([§1.7.2](01-introduction.md#sec-1-7-2)).

**Expected value.** The quantity $\mathrm{E}[ X ] = \sum_x x \Pr\{ X = x
\}$ of a discrete random variable $X$
([§1.5.4](01-introduction.md#sec-1-5-4)).

**Expression tree.** The binary tree of an arithmetic expression, whose
leaves are the operands and whose internal nodes are the operators; its
visits give the prefix, infix and postfix forms
([§8.3.5](08-tree.md#sec-8-3-5)).

**External fragmentation.** The free memory of an allocator that is in
pieces too small for a request, although the free words would be enough
([§5.1](05-memory.md#sec-5-1)).

**External nodes.** The nodes added to complete a binary tree, one for each
missing child; in the representations with pointers they are the null
pointers ([§8.1](08-tree.md#sec-8-1)).

**External sort.** The sort of a file larger than the memory, which forms
sorted runs and merges them, $k$ at a time, in $O( ( n / B ) \log_{M / B} (
n / B ) )$ I/Os ([§23.4.2](23-external.md#sec-23-4-2)).

**External-memory model.** The model of Aggarwal and Vitter, which counts
the transfers of blocks of $B$ items between a memory of $M$ items and a
disk ([§23.1](23-external.md#sec-23-1)).

**Eytzinger layout.** The layout of a sorted vector in the order of a
complete binary tree by levels, so that a search reads the places of its
first levels close together ([§6.3](06-search.md#sec-6-3)).

**Failure link.** In the automaton of Aho and Corasick, the link from a
state $u$ to the state of the longest proper suffix of $u$ that is in the
trie of the patterns ([§15.3](15-string.md#sec-15-3)).

**False negative.** An answer no of a filter for a key of its set, which a
filter never gives ([§10.1](10-set.md#sec-10-1)).

**False positive.** An answer yes of a filter for a key that is not in its
set ([§10.1](10-set.md#sec-10-1)).

**False positive rate.** The probability $\varepsilon$, over the choice of
the hash functions, that a filter answers yes for a key not in its set
([§10.1](10-set.md#sec-10-1)).

**False sharing.** Two words written by two threads on one line of cache,
which makes the line travel between the cores at each write
([§24.1](24-concurrent.md#sec-24-1)).

**Farm with feedback.** A parallel computation in which a master emits
tasks to $w$ workers, whose results go back to the master, which may emit
more tasks from them ([§25.14](25-parallel.md#sec-25-14)).

**Fast Fourier transform.** The evaluation of a polynomial of degree less
than $N$ at the $N$ roots of unity in $\Theta( N \log N )$ operations, by
splitting its coefficients into the even and the odd ones; it multiplies
polynomials and integers ([§17.3](17-numeric.md#sec-17-3)).

**Fibonacci heap.** The heap of Fredman and Tarjan, a list of heap-ordered
trees with cuts, cascading cuts and consolidation, which decreases a
priority in $O( 1 )$ and removes the minimum in $O( \log n )$ amortized
time ([§9.7](09-priority.md#sec-9-7)).

**Fibonacci tree.** The AVL tree of height $h$ with the fewest nodes, whose
root has a Fibonacci tree of height $h - 1$ on one side and one of height
$h - 2$ on the other; it is also the worst case of the removal
([§8.7](08-tree.md#sec-8-7)).

**FIFO.** The discipline of a queue, first in, first out: the item removed
is the first one added ([Chapter 4](04-queue.md#ch-4)).

**Fifty-percent rule.** The observation of Knuth that, under some
assumptions of equilibrium, the free blocks of an allocator are about half
as many as the blocks in use, whatever the fit
([§5.2](05-memory.md#sec-5-2)).

**Filter.** A set that answers a query on a key with no false negative and
with false positives of a given rate, in a space that does not depend on
the size of the keys ([§10.1](10-set.md#sec-10-1)).

**Finger.** A pointer to a place of a structure (a node of a treap, the
nodes of a search path of a skip list) from which a search that ends near
it costs according to the distance rather than to the size
([§7.9.4](07-dictionary.md#sec-7-9-4)).

**Fingerprint.** The few bits of the hash of a key that a filter of
fingerprints keeps in place of the key; a key out of the set is a false
positive when its fingerprint matches ([§10.9](10-set.md#sec-10-9)).

**First fit.** The rule of an allocator that takes the first block of the
free list large enough for the request ([§5.2](05-memory.md#sec-5-2)).

**First unique implication point.** The literal of the last decision level
at which the analysis of a conflict stops, a single literal of that level
being left in the clause it learns ([§19.7](19-backtrack.md#sec-19-7)).

**Fisher and Yates shuffle.** The random permutation of a vector that swaps
each place, from the last, with a place drawn uniformly at random among
those before it ([§17.9](17-numeric.md#sec-17-9)).

**Flow.** An assignment to the edges of a network of values within their
capacities, conserved at every node but the source and the sink
([§14.10.1](14-graph.md#sec-14-10-1)).

**Floyd and Rivest selection.** The selection that takes its pivot from a
recursive sample around the rank $k$, with $n + min( k , n - k )$
comparisons in expectation up to lower-order terms
([§6.8](06-search.md#sec-6-8)).

**Floyd and Warshall algorithm.** The shortest paths between all the pairs,
computed by allowing the nodes as intermediate one at a time, in $\Theta(
n^3 )$ time ([§14.8.2](14-graph.md#sec-14-8-2)).

**FM-index.** The compressed index on the Burrows and Wheeler transform
that counts the occurrences of a pattern by the backward search, with ranks
on the transformed text ([§21.11](21-compression.md#sec-21-11)).

**Focus pointers.** The pointers of the loopless generation of the tuples
in reflected Gray order, which give the place to change in $O( 1 )$ time
instead of searching for it ([§18.5](18-combinatorial.md#sec-18-5)).

**Folded product.** The 128-bit product of two words whose two halves are
combined by an exclusive or, the mixing step of the fast hashes of ADS
([§7.3.6](07-dictionary.md#sec-7-3-6)).

**Folklore table.** The open addressing with linear probing on a vector of
atomic words, which never moves a word once written
([§24.8](24-concurrent.md#sec-24-8)).

**Forced move.** In the exact cover, an item with a single option left,
taken at the next level without trying alternatives
([§19.6](19-backtrack.md#sec-19-6)).

**Foreach.** The visit of all the pairs of a dictionary (or the items of a
set) in the order of the structure, by a function called on each
([§8.5.1](08-tree.md#sec-8-5-1)).

**Forest of parents.** The representation of disjoint sets as trees in a
vector of parents, the roots being the representatives
([§11.3](11-mfset.md#sec-11-3)).

**Fork and join.** The pair of the start of two calls in parallel and of
the wait for both, one of the two ways in which the parallel algorithms
build their graph of tasks ([§25.1](25-parallel.md#sec-25-1)).

**Fragmentation.** The memory that an allocator holds and does not give for
use, internal when a block is larger than its request and external when the
free memory is in pieces too small ([§5.1](05-memory.md#sec-5-1)).

**Free list.** The list of the free blocks or cells of a memory, linked
through the free blocks themselves ([§5.1](05-memory.md#sec-5-1)).

**Front.** The end of a queue from which the items are removed ([Chapter
4](04-queue.md#ch-4)).

**Front coding.** The code of a sorted dictionary that writes each string
as the length of the prefix it shares with the previous string followed by
the rest, with some anchors stored whole
([§15.6.2](15-string.md#sec-15-6-2)).

**Frontier.** In the prototype of the shortest paths, the structure that
holds the nodes whose edges have to be checked again; a priority queue, a
queue, a stack or a deque in its place gives the algorithms of Dijkstra, of
Bellman, Ford and Moore, and of Pape and D'Esopo
([§14.8.1](14-graph.md#sec-14-8-1)).

**Galloping.** An exponential search followed by a binary one, by which a
merge skips the items of one run that come before the next item of the
other ([§12.5](12-sort.md#sec-12-5)).

**Garbage.** The cells of a heap that are not reachable from the roots,
which a garbage collector reclaims ([§5.10](05-memory.md#sec-5-10)).

**Garbage collection.** The reclamation of the cells that the program can
no longer reach, by marking those reachable from the roots and sweeping the
others into the free list ([§5.10](05-memory.md#sec-5-10)).

**Generator.** A function that visits all the combinatorial objects of a
kind, each once, in a fixed order; it is loopless when the work between two
consecutive objects is bounded by a constant
([§18.4](18-combinatorial.md#sec-18-4)).

**Generic visit.** The visit of a graph that keeps the set of the open
nodes and, at each step, reaches a new node from one of them or closes it;
its frontier decides whether it is in breadth or in depth
([§14.5.1](14-graph.md#sec-14-5-1)).

**Global rebuilding.** The technique of Overmars that rebuilds a structure
in the background, a few steps per operation, so that an amortized bound
becomes a worst-case one ([§4.9](04-queue.md#sec-4-9)).

**Graham scan.** The convex hull that sorts the points by angle around the
lowest one and scans them with a stack, popping the points that make a turn
to the right, in $O( n \log n )$ time
([§16.2.1](16-geometry.md#sec-16-2-1)).

**Grammar of the matched strings.** The definition by which the empty
string is matched and, with $u$ and $v$ matched, so are $u$ between a pair
of brackets of one kind and $u v$; a stack decides it in $O( n )$ time
([§3.8](03-stack.md#sec-3-8)).

**Graph.** A set of nodes together with a set of pairs of nodes, the edges,
oriented or not ([§14.1](14-graph.md#sec-14-1)).

**Gray code.** The order of the binary words of $n$ bits in which two
consecutive words differ in one bit, the word of rank $k$ being $k \oplus (
k \gg 1 )$ ([§18.2](18-combinatorial.md#sec-18-2)).

**Greedy algorithm.** An algorithm that builds a solution by stages, fixing
at each one the component that looks best, with no going back; it is
correct under the greedy choice property and optimal substructure
([§13.11](13-algorithm.md#sec-13-11)).

**Greedy schedule.** A schedule that never leaves a thread idle while a
task is ready to run, which takes at most $W / p + D$ time on $p$ threads
([§25.1](25-parallel.md#sec-25-1)).

**Grid.** The implicit graph whose nodes are the free cells of a map of $R
\times C$ cells and whose edges are the moves between them
([§14.6.1](14-graph.md#sec-14-6-1)).

**Group (y-fast trie).** The keys between two consecutive representatives
of a y-fast trie, kept in a balanced tree that is the value of its
representative ([§7.11.3](07-dictionary.md#sec-7-11-3)).

**Half-cleaner.** The stage of a bitonic sorter of $k$ items that compares
each item of the first half with the item $k / 2$ places after it
([§12.12](12-sort.md#sec-12-12)).

**Halving.** The variant of the compression of a union-find in which each
node on the path climbed is made a child of its grandparent, in one pass
with no stack ([§11.4](11-mfset.md#sec-11-4)).

**Handle.** What an insertion into a priority queue returns to find the
item again (a cell, a node, an index), needed by `decrease()` and by the
removal of an arbitrary item ([§9.1](09-priority.md#sec-9-1)).

**Happens before.** The order between the writes of a thread before a
release store and the reads of another thread after an acquire load that
reads it ([§24.1](24-concurrent.md#sec-24-1)).

**Harmonic numbers.** The numbers $H_k = 1 + 1 / 2 + \cdots + 1 / k$, with
$\ln k < H_k \le 1 + \ln k$, which bound the expected depths of the random
search trees ([§8.6.1](08-tree.md#sec-8-6-1)).

**Hash code.** The integer of $w$ bits to which a key that is not an
integer (a string, an array, an object of several parts) is mapped before
it is hashed ([§7.3.5](07-dictionary.md#sec-7-3-5)).

**Hash function.** A function $H : U \rightarrow \{ 0 , \ldots , m - 1 \}$
from the universe of the keys to the places of a table
([§7.1](07-dictionary.md#sec-7-1)).

**Hash table.** A dictionary that keeps its keys in a vector of $m$ places
chosen by a hash function, resolving the collisions by chaining or by open
addressing ([§7.4](07-dictionary.md#sec-7-4)).

**Hashing by multiplication.** The hash function $H( k ) = \lfloor m \,
\mathrm{frac}( C i ) \rfloor$ of a key read as the integer $i$, which works
with any $m$ and, for $C = ( \sqrt{5} - 1 ) / 2$, spreads the consecutive
keys most evenly ([§7.3.1](07-dictionary.md#sec-7-3-1)).

**Header (allocation).** Some words of a block that the allocator reads,
such as its size; a sized deallocation needs none
([§5.1](05-memory.md#sec-5-1)).

**Heap.** A tree whose nodes are heap-ordered, so that its root holds a
minimum ([§9.3](09-priority.md#sec-9-3)).

**Heap (allocator).** In the pool shared by threads, the pages that a
thread owns, with the batches of the slots it frees for other heaps and the
returns that other threads free into it ([§5.8](05-memory.md#sec-5-8)).

**Heap-ordered.** Said of a tree in which no node has a priority smaller
than that of its parent ([§9.3](09-priority.md#sec-9-3)).

**Heap's method.** The generation of the permutations by one swap per
permutation, with the least bookkeeping
([§18.6](18-combinatorial.md#sec-18-6)).

**Heapsort.** The sort that builds a max-heap on the vector and then moves
its maximum to the end $n - 1$ times, in place in $O( n \log n )$ time
([§12.9](12-sort.md#sec-12-9)).

**Height (node).** The number of edges of the longest path from a node down
to a leaf; the height of a tree is that of its root, and $-1$ for the empty
tree ([§8.1](08-tree.md#sec-8-1)).

**Height property.** The property of a 2-4 tree that all its leaves are at
the same depth ([§8.10.1](08-tree.md#sec-8-10-1)).

**Hierholzer algorithm.** The construction of an Eulerian cycle by a walk
kept on a stack, a node with no unused edge being popped and written
([§14.12.2](14-graph.md#sec-14-12-2)).

**Huffman code.** The prefix-free code of least average length for given
frequencies of the symbols, built by merging the two least frequent trees
until one is left ([§21.2](21-compression.md#sec-21-2)).

**Huge pages.** The pages of 2 MB (or more) of the virtual memory, which
cut the misses of the translation lookaside buffer of a structure whose
nodes span many pages ([§5.9](05-memory.md#sec-5-9)).

**Hungarian tree.** An alternating tree that no augmenting path can use,
whose nodes are left out of the later searches of a maximum matching
([§14.11.2](14-graph.md#sec-14-11-2)).

**Implementation.** A realization of an interface, i.e., how a structure
does what its interface states ([§1.3](01-introduction.md#sec-1-3)).

**In place.** Said of a sort that uses $O( 1 )$ extra items, or $O( \log n
)$ extra words for a stack ([§12.1](12-sort.md#sec-12-1)).

**Inbox.** The queue of a worker into which the others push batches of
messages and which only its owner drains
([§25.13](25-parallel.md#sec-25-13)).

**Incremental filter.** A filter that takes insertions one at a time but no
removal ([§10.1](10-set.md#sec-10-1)).

**Independent set (list ranking).** A set of items of a list none of which
is the successor of another or the tail, removed together at a step of the
recursive list ranking ([§23.5.2](23-external.md#sec-23-5-2)).

**Indexed heap.** A $d$-ary heap of the ids $0 , \ldots , n - 1$ of the
nodes of a graph, with the place of each id kept in a vector, so that a
decrease finds its item in $O( 1 )$ time ([§9.5](09-priority.md#sec-9-5)).

**Indicator variable.** A random variable equal to 1 when an event happens
and 0 otherwise, whose expected value is the probability of the event
([§1.5.4](01-introduction.md#sec-1-5-4)).

**Information-theoretic lower bound.** The $\lceil \log_2 N \rceil$ bits
that a representation of the elements of a class of $N$ needs for some of
them ([§22.1](22-succinct.md#sec-22-1)).

**Inorder visit.** The symmetric visit of a binary tree, which examines a
node after its left subtree and before its right one, and gives the keys of
a binary search tree in increasing order ([§8.1](08-tree.md#sec-8-1)).

**Insertion sort.** The sort that inserts each item into the sorted prefix
before it, in $O( n + I )$ time for $I$ inversions
([§12.3](12-sort.md#sec-12-3)).

**Inspection.** The sequence of places $H( k , 0 ) , H( k , 1 ) , \ldots$
that an open-addressing table probes for a key
([§7.5](07-dictionary.md#sec-7-5)).

**Interface.** An abstract data type as a program sees it, which states
what a structure does but not how ([§1.3](01-introduction.md#sec-1-3)).

**Interleaved directory.** A directory of rank and select whose counts are
stored next to the bits they describe, so that a query reads one line of
cache ([§22.4](22-succinct.md#sec-22-4)).

**Internal fragmentation.** The memory lost because a block is larger than
its request, by the rounding to a size class or because of a header
([§5.1](05-memory.md#sec-5-1)).

**Interpolation search.** The search of a sorted vector of numbers that
probes the place estimated by linear interpolation between the ends, in $O(
\log \log n )$ expected comparisons on uniform keys
([§6.2](06-search.md#sec-6-2)).

**Interpolative code.** The code of a strictly increasing sequence that
writes the middle item within the range its neighbors allow and then the
two halves recursively ([§21.7](21-compression.md#sec-21-7)).

**Interval tree.** A balanced tree of intervals, ordered by their left
ends, whose nodes keep the largest right end of their subtree, and which
finds the intervals that contain a point
([§16.4.1](16-geometry.md#sec-16-4-1)).

**Invariant.** A property of the representation of a structure that each
operation may assume at its start and must restore at its end; ADS checks
it by `ADS_INVARIANT` at the level 3
([§1.7.1](01-introduction.md#sec-1-7-1)).

**Inversion.** A pair of items of a sequence in the wrong order; the
elementary sorts take time proportional to their number
([§12.3](12-sort.md#sec-12-3)).

**Invisit.** The visit of an ordered tree that examines a node after the
subtrees of its first $i$ children; with $i = 1$ on a binary tree, the
symmetric visit ([§8.1](08-tree.md#sec-8-1)).

**I/O.** The transfer of one block between the disk and the memory, the
unit of cost of the external-memory model
([§23.1](23-external.md#sec-23-1)).

**Iterator.** A position in a container that can be read and moved, of the
category its moves allow in constant time
([§1.8](01-introduction.md#sec-1-8)).

**Iterators (operations).** The operations of an abstract data type that
visit its items ([§1.2](01-introduction.md#sec-1-2)).

**Jacobson directory.** The two-level directory of counts, by superblocks
and blocks, that gives the rank of a vector of bits in $O( 1 )$ time with
$o( m )$ extra bits ([§22.3](22-succinct.md#sec-22-3)).

**Join.** The operation that, from two balanced trees whose keys are
smaller and larger than a key $k$, returns a balanced tree of all their
keys and $k$; all the other operations of a join-based tree are written on
it ([§8.12](08-tree.md#sec-8-12)).

**Join-based tree.** A balanced search tree whose insertions, removals and
set operations are all written on the join of its balancing scheme (AVL,
red-black, weight balance or treap) ([§8.12](08-tree.md#sec-8-12)).

**Jump pointer.** In a binary trie, the pointer of an internal node with
one child to the smallest or the largest leaf below it
([§7.11.1](07-dictionary.md#sec-7-11-1)).

**Karatsuba multiplication.** The product of two numbers of $n$ digits by
three products of halves instead of four, in $\Theta( n^{\log_2 3} )$ time
([§17.2](17-numeric.md#sec-17-2)).

**Knapsack.** The problem of choosing among items of given weights and
values those of largest value within a capacity, solved by dynamic
programming in pseudopolynomial time ([§13.7](13-algorithm.md#sec-13-7)).

**Knuth, Morris and Pratt search.** The search of a pattern that, after a
mismatch, shifts it by its longest border, never reading a character of the
text twice, in $O( n + m )$ time ([§15.2.1](15-string.md#sec-15-2-1)).

**Kraft inequality.** The bound $\sum_c 2^{-\ell( c )} \le 1$ on the
lengths of a prefix-free code ([§21.2](21-compression.md#sec-21-2)).

**k-sorted.** Said of a vector in which each item is at most $k$ places
away from its place in the sorted order ([§2.17](02-sequence.md#sec-2-17)).

**Lane.** In the queue of many producers and one consumer, the queue of one
producer, taken in turn by the consumer
([§24.3](24-concurrent.md#sec-24-3)).

**Lazy deletion.** The handling of a decrease in a priority queue without
handles, which pushes the item again with its new priority and skips the
stale copy when it comes out ([§9.1](09-priority.md#sec-9-1)).

**LCP array.** The array of the lengths of the longest common prefixes of
the suffixes consecutive in the suffix array, computed in linear time by
the method of Kasai et al ([§15.5.1](15-string.md#sec-15-5-1)).

**Leading dimension.** The distance between the starts of two rows of a
dense matrix, by which a block of it is addressed without a copy
([§17.6](17-numeric.md#sec-17-6)).

**Leaf.** A node of a tree with no child; the others are internal
([§8.1](08-tree.md#sec-8-1)).

**Left-leaning red-black tree.** The red-black tree in which a node with
one red child has it on the left, so that each node of a 2-4 tree has
exactly one representation ([§8.10.2](08-tree.md#sec-8-10-2)).

**Level (tree).** The set of the nodes of a tree at a given depth
([§8.1](08-tree.md#sec-8-1)).

**Lexicographic position.** The number of the strings of a sorted
dictionary that are smaller than a pattern; the strings prefixed by the
pattern follow it consecutively ([§15.6](15-string.md#sec-15-6)).

**LIFO.** The discipline of a stack, last in, first out: the item removed
is the last one added ([Chapter 3](03-stack.md#ch-3)).

**Light edge.** An edge from a node to its parent that has at most half of
the descendants of the parent; a path from a root crosses at most $\log_2
n$ of them ([§8.13.4](08-tree.md#sec-8-13-4)).

**Limb.** A digit of a `BigInteger`, i.e., a word of 64 bits read as a
digit in the base $2^{64}$ ([§17.1](17-numeric.md#sec-17-1)).

**Linear congruential generator.** The generator of pseudorandom numbers
$x_{i+1} = ( a x_i + c ) \bmod m$ ([§17.9](17-numeric.md#sec-17-9)).

**Linear probing.** The open addressing whose inspection tries the places
after $H_0( k )$ one by one, which suffers from the primary clustering but
scans contiguous memory ([§7.5](07-dictionary.md#sec-7-5)).

**Linearity of expectation.** The identity $\mathrm{E}[ X_1 + \cdots + X_k
] = \mathrm{E}[ X_1 ] + \cdots + \mathrm{E}[ X_k ]$, which holds for any
random variables, independent or not
([§1.5.4](01-introduction.md#sec-1-5-4)).

**Linearizable.** Said of a run of a shared object in which each operation
seems to take effect at one instant between its invocation and its response
(its linearization point), with the responses of the sequential object
([§24.1](24-concurrent.md#sec-24-1)).

**Linearization point.** The instant between the invocation and the
response of an operation of a shared object at which it seems to take
effect; a run is linearizable when the responses are those of the
sequential object on the operations taken in the order of their points
([§24.1](24-concurrent.md#sec-24-1)).

**Link by index.** The rule of a union-find that hangs the root of smaller
index under the other, so that the paths increase strictly to their root,
with no rank and no size ([§11.4](11-mfset.md#sec-11-4)).

**Link-cut trees.** The dynamic trees of Sleator and Tarjan, which keep the
preferred paths of a forest in splay trees and do all the operations in $O(
\log n )$ amortized time ([§8.13](08-tree.md#sec-8-13)).

**Linked list.** A sequence whose items are in cells of their own, each
holding the address of the next cell (and of the previous one, if doubly
linked) ([§2.9](02-sequence.md#sec-2-9)).

**List ranking.** The computation of the distance of each item of a linked
list from its tail, by pointer jumping or, in the external memory, by
recursion on independent sets ([§23.5](23-external.md#sec-23-5)).

**Lists of children.** The representation of an ordered tree that keeps,
for each node, the list of its children in their order
([§8.2.3](08-tree.md#sec-8-2-3)).

**Literal.** A variable or its negation
([§19.7](19-backtrack.md#sec-19-7)).

**Load factor.** The ratio $\alpha = n / m$ of the keys of a hash table to
its places ([§7.1](07-dictionary.md#sec-7-1)).

**Local optimum.** A solution with no better solution in its neighborhood,
at which a local search stops ([§20.9](20-complexity.md#sec-20-9)).

**Local search.** The heuristic that starts from a solution and moves to a
better solution of its neighborhood while there is one
([§20.9](20-complexity.md#sec-20-9)).

**Lock-free.** A concurrent implementation in which, whatever the threads
that stop, some running thread completes an operation in a finite number of
its steps ([§24.1](24-concurrent.md#sec-24-1)).

**Logarithmic method.** The technique of Bentley and Saxe that makes a
static structure dynamic by keeping static structures of geometrically
growing sizes and merging them as a binary counter does
([§6.6](06-search.md#sec-6-6)).

**Longest common extension.** The length $\mathrm{lce}( i , j )$ of the
longest common prefix of the suffixes $i$ and $j$ of a text, a range
minimum of the LCP array ([§15.5.6](15-string.md#sec-15-5-6)).

**Longest common subsequence.** The longest sequence that is a subsequence
of both of two strings, computed by a table on their prefixes
([§15.4.2](15-string.md#sec-15-4-2)).

**Loopless.** Said of a generator whose work between two consecutive
objects is bounded by a constant in the worst case
([§18.4](18-combinatorial.md#sec-18-4)).

**Loser tree.** The tournament tree of $k$ players whose internal nodes
keep the loser of the match played there, which merges $k$ sorted sequences
with about $\log_2 k$ comparisons per item
([§9.11](09-priority.md#sec-9-11)).

**LOUDS.** The level-order unary degree sequence, the succinct code of a
tree in about $2 n$ bits that writes the degree of each node in unary, by
levels ([§22.8](22-succinct.md#sec-22-8)).

**Lowest common ancestor.** The deepest node that is an ancestor of two
given nodes of a tree ([§8.4.6](08-tree.md#sec-8-4-6)).

**Lyndon word.** A nonempty string strictly smaller than all its proper
rotations; every string factors uniquely into a non-increasing sequence of
Lyndon words ([§15.7.2](15-string.md#sec-15-7-2)).

**LZ77.** The compressor of Lempel and Ziv that parses the text into
phrases, each the longest prefix of the rest that starts in a window of the
text already seen, coded as a copy ([§21.9](21-compression.md#sec-21-9)).

**LZ78.** The compressor of Lempel and Ziv whose phrases are a phrase
already seen extended by one symbol, kept in a trie
([§21.9](21-compression.md#sec-21-9)).

**Mailbox.** The layer of a parallel or distributed computation through
which the workers or the processes send batches of messages to the owners
of the data, with a detection of the termination
([§25.15](25-parallel.md#sec-25-15)).

**Manacher algorithm.** The computation of the longest palindrome centred
at each place of a string in linear time
([§15.7.1](15-string.md#sec-15-7-1)).

**Master.** In a farm with feedback, the thread that emits the tasks and
receives the results ([§25.14](25-parallel.md#sec-25-14)).

**Matched string.** A string of the brackets `()`, `[]` and `{}` in which
the brackets pair up properly, i.e., a Dyck word of sum 0 in which each
closed bracket meets an open one of its kind
([§1.3.1](01-introduction.md#sec-1-3-1)).

**Matching.** A set of edges of a graph no two of which share an end; a
$b$-matching lets each node $x$ be in at most $b( x )$ of them
([§14.11.1](14-graph.md#sec-14-11-1)).

**Matrix of incidence.** The $n \times m$ matrix of an oriented graph with
a column per edge, $-1$ at its source and $+1$ at its target
([§14.4.3](14-graph.md#sec-14-4-3)).

**Max-heap.** A heap with the order reversed, whose root holds a maximum,
as heapsort uses ([§12.9](12-sort.md#sec-12-9)).

**Maximum flow.** A flow of largest value from the source to the sink of a
network, found by augmenting paths or by the push-relabel
([§14.10.1](14-graph.md#sec-14-10-1)).

**Measured search tree.** The binary search tree whose nodes keep the size
of their subtree, their depth and the height of their subtree, all exact
after each change ([§8.5.6](08-tree.md#sec-8-5-6)).

**Merge (B-tree).** The repair of a node of a B-tree with too few keys that
joins it, its sibling and the key of their parent between them into one
node ([§8.11.1](08-tree.md#sec-8-11-1)).

**Merge sort.** The sort by divide and conquer that sorts the two halves
recursively and merges them, in $O( n \log n )$ time, stable, with a buffer
of $n$ items ([§12.4](12-sort.md#sec-12-4)).

**Merging sorted sequences.** The merge of $k$ sorted sequences, which
extracts at each step the least of their first items, by a heap or a loser
tree ([§9.11](09-priority.md#sec-9-11)).

**Middle square.** The hash function that takes the bits in the middle of
the square of the key ([§7.3.1](07-dictionary.md#sec-7-3-1)).

**Miller and Rabin test.** The randomized test of primality that tries $K$
bases drawn at random, a composite number passing with probability at most
$4^{-K}$ ([§17.8](17-numeric.md#sec-17-8)).

**Minimal perfect hash function.** A perfect hash function on $n$ keys
whose range is $\{ 0 , \ldots , n - 1 \}$
([§7.8](07-dictionary.md#sec-7-8)).

**Minimum spanning tree.** A spanning tree of an unoriented connected graph
of least total weight, found by the algorithms of Kruskal and Prim
([§14.9](14-graph.md#sec-14-9)).

**Min-max heap.** A double ended priority queue on a complete binary tree
whose levels alternate between minimum and maximum order
([§9.10](09-priority.md#sec-9-10)).

**MinQueue.** The QUEUE with `min()` built on two `MinStack`s, the back and
the front, the front refilled from the back by reversing it when empty
([§4.8](04-queue.md#sec-4-8)).

**MinStack.** The STACK that also gives its least item by `min()` in $O( 1
)$ time, keeping beside its items the stack of the minima, one per record
low from the bottom ([§3.5](03-stack.md#sec-3-5)).

**Min-wise independence.** The property of a hash function that each of $k$
keys has the least hash with probability $O( 1 / k )$, enough for a treap
whose priorities are hashes ([§8.6.4](08-tree.md#sec-8-6-4)).

**Model of computation.** The machine on which the costs are counted, the
word-RAM with words of $w$ bits in ADS, with the external-memory and the
cache-oblivious models for the memory hierarchy
([§1.6](01-introduction.md#sec-1-6)).

**Model (satisfiability).** An assignment of truth values that makes every
clause of a formula true ([§19.7](19-backtrack.md#sec-19-7)).

**Modifier.** An operation that changes a value of an abstract data type
([§1.2](01-introduction.md#sec-1-2)).

**Module.** The C++20 module `ads`, which exports all the names that the
headers declare in the namespace `ADS` and in those nested in it, but for
the namespaces `detail` ([§A.8](A-writing-structure.md#sec-A-8)).

**Monotone priority queue.** A priority queue into which no priority
smaller than the last one extracted is pushed, as in the algorithm of
Dijkstra with nonnegative costs; integer keys then allow the radix heap and
the bucket queue ([§9.1](09-priority.md#sec-9-1)).

**Monte Carlo algorithm.** A randomized algorithm that always ends within
its time bound and may answer wrongly with a probability bounded over its
random choices ([§13.12](13-algorithm.md#sec-13-12)).

**Morton code.** The interleaving of the bits of the two coordinates of a
point in one word, whose order visits the plane by a recursive Z-shaped
curve ([§18.2](18-combinatorial.md#sec-18-2)).

**Move-to-front.** The transform that replaces each symbol with its place
in a list of the symbols and then moves it to the front, which turns the
runs of the Burrows and Wheeler transform into small numbers
([§21.10](21-compression.md#sec-21-10)).

**Multikey quicksort.** The sort of strings that partitions them into three
parts by one character of a pivot, and recurs on the next character only in
the middle part ([§12.11](12-sort.md#sec-12-11)).

**Multiplicative hashing.** The hash $( z x \bmod 2^w ) \operatorname{div}
2^{w - d}$ of a hash code $x$ into $2^d$ places, $z$ a random odd integer
([§7.3.3](07-dictionary.md#sec-7-3-3)).

**MultiQueue.** The relaxed priority queue made of several queues under
locks, in which a pop takes the better of the tops of two queues drawn at
random ([§24.9](24-concurrent.md#sec-24-9)).

**Multiset.** A set that may hold an item more than once, the number of its
occurrences being its multiplicity ([§10.1](10-set.md#sec-10-1)).

**MultiStack.** The realization of $k$ stacks in one vector of linked cells
with a free list, which shares the space among the stacks and grows by a
constant factor ([§3.4](03-stack.md#sec-3-4)).

**Multiway merge.** See merging sorted sequences
([§9.11](09-priority.md#sec-9-11)).

**n queens.** The problem of placing $n$ queens on an $n \times n$ board
with no two on a row, a column or a diagonal, solved by backtracking
([§19.3](19-backtrack.md#sec-19-3)).

**Natural merge sort.** A merge sort that merges the runs that the input
already has, in an order that decides its cost
([§12.5](12-sort.md#sec-12-5)).

**Next fit.** The rule of an allocator that starts the search of the free
list where the last one stopped ([§5.2](05-memory.md#sec-5-2)).

**Nondeterministic algorithm.** An algorithm that may call `choice( k )` to
pick one of $k$ alternatives and answers YES if some sequence of choices
leads to success ([§20.2](20-complexity.md#sec-20-2)).

**No-red-edge property.** The property of a red-black tree that no red node
has a red parent ([§8.10.2](08-tree.md#sec-8-10-2)).

**NP.** The class of the decision problems whose instances with answer YES
have a certificate of polynomial length checked in polynomial time
([§20.1](20-complexity.md#sec-20-1)).

**NP-complete.** Said of a problem of NP to which every problem of NP
reduces in polynomial time; it is strongly NP-complete if it stays so when
its numbers are bounded by a polynomial in its size, and weakly otherwise
([§20.4](20-complexity.md#sec-20-4)).

**NP-hard.** Said of a problem to which every problem of NP reduces in
polynomial time ([§20.4](20-complexity.md#sec-20-4)).

**Number theoretic transform.** The fast Fourier transform modulo a prime
with roots of unity of large order, which multiplies polynomials with
integer coefficients exactly ([§17.3](17-numeric.md#sec-17-3)).

**Object pool.** An arena that also frees single objects, putting their
slots on a free list that the next allocations reuse
([§5.5](05-memory.md#sec-5-5)).

**Observable graph.** A graph that turns each of its changes into an event
and sends it to the observers that want its kind
([§14.13.1](14-graph.md#sec-14-13-1)).

**Observer (operation).** An operation that returns a property of a value
of an abstract data type, such as `empty()` or `top()`
([§1.2](01-introduction.md#sec-1-2)).

**Occurrence.** A place of a text at which a pattern starts; the problem of
the search is to find all of them ([§15.2](15-string.md#sec-15-2)).

**Octagon.** The polygon of the eight extreme points of a set in the
directions of the axes and of the diagonals, whose interior points the
filter of Akl and Toussaint discards before the convex hull
([§16.2.2](16-geometry.md#sec-16-2-2)).

**Offer.** In the priority queues that are stolen, the atomic pointer
through which an owner publishes a heap for a thief to claim
([§24.11](24-concurrent.md#sec-24-11)).

**Open addressing.** The resolution of the collisions of a hash table by
further places of the table itself, along the inspection of the key
([§7.5](07-dictionary.md#sec-7-5)).

**Optimal substructure.** The property of a problem that an optimal
solution is made of a choice and of optimal solutions of the subproblems
that the choice leaves ([§13.7](13-algorithm.md#sec-13-7)).

**Order (B+-tree).** The largest number $m$ of children of an internal node
of a B+-tree, each node but the root having at least $\lceil m / 2 \rceil$
([§8.11.2](08-tree.md#sec-8-11-2)).

**Order maintenance.** The problem of a list in which an element is
inserted after another, removed, and two elements are compared by their
order, all "fast" ([§2.15.1](02-sequence.md#sec-2-15-1)).

**Order preserving hash function.** A perfect hash function that maps the
keys in their order ([§7.8](07-dictionary.md#sec-7-8)).

**Ordered tree.** A tree whose children of each node form a sequence, the
first being the leftmost ([§8.1](08-tree.md#sec-8-1)).

**Orientation.** The sign of the cross product of $b - a$ and $c - a$,
which tells whether three points turn counterclockwise, clockwise or are
collinear ([§16.1](16-geometry.md#sec-16-1)).

**Orthogonal lists.** The representation of a sparse matrix in which each
nonzero entry is in the list of its row and in that of its column
([§17.7](17-numeric.md#sec-17-7)).

**Output link.** In the automaton of Aho and Corasick, the link from a
state to the nearest proper suffix of it that is a whole pattern
([§15.3](15-string.md#sec-15-3)).

**Overhead.** The space that a structure adds to its items (pointers,
indices, capacities, free places) ([§1.7.3](01-introduction.md#sec-1-7-3)).

**P.** The class of the decision problems decided in time polynomial in the
size of the instance ([§20.1](20-complexity.md#sec-20-1)).

**Packed array.** An array of $n$ values below $2^B$ in fields of $B$ bits
packed in words, $n B$ bits in all ([§22.7](22-succinct.md#sec-22-7)).

**Page heap.** The allocator of size classes whose free lists are sharded
by page, so that the objects freed together are given out again from few
pages ([§5.7](05-memory.md#sec-5-7)).

**Pairing heap.** A heap-ordered tree with any number of children, whose
removal of the minimum links the subtrees of the root in pairs; the
simplest of the heaps with an efficient `decrease()`
([§9.8](09-priority.md#sec-9-8)).

**Palindrome.** A sequence that reads the same in both directions
([§15.7.1](15-string.md#sec-15-7-1)).

**Parallel step.** A round $A[ a_i ] \gets \mathrm{op}( A[ a_i ] , A[ b_i ]
)$ in which all the values of $A$ are read before any change, simulated in
the external memory by sorting ([§23.5.1](23-external.md#sec-23-5-1)).

**Parallelism.** The ratio $W / D$ of the work to the depth of a parallel
computation, the largest number of threads that can be kept busy
([§25.1](25-parallel.md#sec-25-1)).

**Parent.** The node of which a node is a child; each node of a tree but
the root has exactly one ([§8.1](08-tree.md#sec-8-1)).

**Parse tree.** The tree of an arithmetic expression, whose leaves are the
operands and whose internal nodes the operators
([§8.3.5](08-tree.md#sec-8-3-5)).

**Partial rebuilding.** The technique of Overmars that rebuilds a subtree
that went out of balance, the cost being paid, in the amortized sense, by
the operations that unbalanced it ([§8.9](08-tree.md#sec-8-9)).

**Partial solution.** A prefix of a solution under construction, a node of
the tree of the choices ([§19.1](19-backtrack.md#sec-19-1)).

**Partial-key cuckoo hashing.** The cuckoo hashing of the cuckoo filter, in
which the other bucket of a fingerprint is computed from the fingerprint
and either bucket ([§10.9](10-set.md#sec-10-9)).

**Path.** A sequence of nodes each joined to the next by an edge; it is
simple if its nodes are distinct ([§14.1](14-graph.md#sec-14-1)).

**Path compression (trie).** The contraction of the chains of nodes with a
single child of a trie into the prefix of the first node that branches
([§7.10.3](07-dictionary.md#sec-7-10-3)).

**Path-parent pointer.** In a link-cut tree, the pointer from the root of
the splay tree of a preferred path to the parent of the top of the path
([§8.13.2](08-tree.md#sec-8-13-2)).

**Patricia trie.** The compact trie that keeps on each edge only its first
character and in each node the length of its prefix, searched by the blind
search ([§7.10.2](07-dictionary.md#sec-7-10-2)).

**Peak.** An item of a vector not smaller than its neighbors, found after
$\lceil \log_2 n \rceil$ comparisons by a binary search that keeps a part
holding a peak ([§6.7](06-search.md#sec-6-7)).

**Peeling.** The repeated removal of a vertex of degree 1 with its edge
from a hypergraph, whose reverse order solves the linear system of some
perfect hash functions and filters ([§7.8.2](07-dictionary.md#sec-7-8-2)).

**Perfect hash function.** A hash function with no collision on a given set
of keys, which gives a table that searches in $O( 1 )$ time in the worst
case ([§7.8](07-dictionary.md#sec-7-8)).

**Perfectly balanced.** Said of a binary search tree in which the root of
each subtree of $s$ nodes is the node of rank $\lfloor ( s - 1 ) / 2
\rfloor$ in symmetric order ([§8.9.1](08-tree.md#sec-8-9-1)).

**Period.** An integer $p > 0$ such that $P[ i ] = P[ i + p ]$ wherever
both sides are defined ([§15.1](15-string.md#sec-15-1)).

**Permuted buckets.** The intersection of sorted lists of Sanders and
Transier, which maps the universe by the invertible permutation $( a x + c )
\bmod 2^w$, $a$ odd, and splits each list into buckets of about $L$ items
by the top bits of the images ([§10.6](10-set.md#sec-10-6)).

**PGM-index.** The learned index of Ferragina and Vinciguerra, which covers
the points $( A[ i ] , i )$ of a sorted vector by segments within a given
error and searches the segments recursively ([§6.4](06-search.md#sec-6-4)).

**Phase saving.** The rule of a CDCL solver that a decided variable takes
the value it had last ([§19.7](19-backtrack.md#sec-19-7)).

**Phrase.** A piece of the parse of a text by a compressor of Lempel and
Ziv, coded as a reference to an earlier occurrence
([§21.9](21-compression.md#sec-21-9)).

**Plain changes.** The order of the permutations of Steinhaus, Johnson and
Trotter in which two consecutive permutations differ by a swap of adjacent
items ([§18.6](18-combinatorial.md#sec-18-6)).

**Pointer jumping.** The list ranking by rounds in which each item replaces
its successor by the successor of its successor, in $O( \log n )$ rounds
([§23.5.1](23-external.md#sec-23-5-1)).

**Polynomial reduction.** A function computable in polynomial time that
maps the instances of a problem $A$ to those of a problem $B$ with the same
answers, written $A \propto B$ ([§20.4](20-complexity.md#sec-20-4)).

**Position.** The handle of an item of a SEQUENCE, of a type chosen by the
realization, through which the items are read, written, inserted and
removed ([§2.1](02-sequence.md#sec-2-1)).

**Postcondition.** The part of a contract that the operation ensures at its
end, stated by `ADS_POST` and evaluated from the level 2
([§1.7.1](01-introduction.md#sec-1-7-1)).

**Postfix notation.** The notation that writes each operator after its two
operands, which needs neither parentheses nor precedences (the reverse
Polish notation of Łukasiewicz) ([§3.6](03-stack.md#sec-3-6)).

**Postvisit.** The visit of a tree that examines each node after the
subtrees of its children ([§8.1](08-tree.md#sec-8-1)).

**Potential.** A function $\Phi \ge 0$ of the state of a structure, 0 on
the empty one, whose increase pays for the expensive operations in an
amortized bound ([§1.7.2](01-introduction.md#sec-1-7-2)).

**Powersort.** The natural merge sort that merges adjacent runs in the
order of the powers of their boundaries, which bounds its cost by the
entropy of the run lengths ([§12.5](12-sort.md#sec-12-5)).

**Precondition.** The part of a contract that the caller ensures before an
operation, stated by `ADS_PRE` and evaluated from the level 1
([§1.7.1](01-introduction.md#sec-1-7-1)).

**Prediction by partial matching.** The compressor of Cleary and Witten
that codes each symbol by the counts of the longest context that predicts
it, escaping to shorter contexts ([§21.6](21-compression.md#sec-21-6)).

**Preferred path.** A path of a forest that goes down along preferred
children, kept in a splay tree by the link-cut trees
([§8.13.2](08-tree.md#sec-8-13-2)).

**Prefix.** A substring that starts at the first symbol of a string
([§15.1](15-string.md#sec-15-1)).

**Prefix code.** A code in which no codeword is a prefix of another, so
that a text coded by it is decoded from left to right with no separator;
also called prefix-free ([§21.1](21-compression.md#sec-21-1)).

**Prefix filter.** The filter of fingerprints of Even et al. whose first
level is a table of bins holding the smallest fingerprints that came to
them and whose second level, the spare, takes the others
([§10.9](10-set.md#sec-10-9)).

**Prefix sums.** The sums of all the prefixes of a sequence, computed in
parallel in linear work and logarithmic depth
([§25.6](25-parallel.md#sec-25-6)).

**Preflow.** An assignment of flow to the edges of a network in which each
node but the source may have an excess of entering over leaving flow, as
the push-relabel keeps ([§14.10.2](14-graph.md#sec-14-10-2)).

**Previsit.** The visit of a tree that examines each node before the
subtrees of its children ([§8.1](08-tree.md#sec-8-1)).

**Prim algorithm.** The minimum spanning tree grown from one node by adding
at each step the lightest edge that leaves the tree, with a priority queue
([§14.9](14-graph.md#sec-14-9)).

**Primary clustering.** The gathering of the occupied places of a table
with linear probing in runs, which a key falling anywhere in a run
lengthens ([§7.5](07-dictionary.md#sec-7-5)).

**Priority queue.** A multiset of items, each with a priority, from which
the item of least priority is read and removed while new items are inserted
([Chapter 9](09-priority.md#ch-9)).

**Protection.** The requirement that only the operations of an abstract
data type act on its values ([§1.2](01-introduction.md#sec-1-2)).

**Prune.** To leave a subtree of the tree of the choices unvisited because
a test on its root proves that it holds no feasible solution
([§19.1](19-backtrack.md#sec-19-1)).

**Pseudopolynomial.** Said of a bound that is polynomial in the values of
the numbers of the input, but exponential in their number of bits (e.g.,
$\Theta( n C )$ for the knapsack) ([§13.7](13-algorithm.md#sec-13-7)).

**Push-relabel.** The algorithm of Goldberg and Tarjan for the maximum
flow, which keeps a preflow and labels on the nodes and pushes excesses
toward the sink along admissible edges
([§14.10.2](14-graph.md#sec-14-10-2)).

**Queue.** A sequence to which items are added at one end, its back, and
from which they are removed at the other, its front, in the order of their
arrival (FIFO) ([Chapter 4](04-queue.md#ch-4)).

**Queueing discipline.** The rule by which a collection decides which item
is removed next: FIFO for a queue, LIFO for a stack, the least priority for
a priority queue ([§1.3.1](01-introduction.md#sec-1-3-1)).

**Quicksort.** The sort by divide and conquer that partitions the items
around a pivot and sorts the two parts recursively, in $O( n \log n )$
expected time with a random pivot ([§12.6](12-sort.md#sec-12-6)).

**Radix heap.** The monotone priority queue of integer keys that keeps its
items in buckets of exponentially growing ranges above the last minimum
extracted, with no comparison of keys ([§9.9](09-priority.md#sec-9-9)).

**Radix sort.** The sort by digits that sorts the keys digit by digit, by a
stable counting sort per digit from the least significant one or by
distribution from the most significant one
([§12.10](12-sort.md#sec-12-10)).

**Random binary search tree.** The binary search tree built by inserting
the keys of a uniformly random permutation into an empty tree, whose
expected depth of a node is $O( \log n )$ ([§8.6.1](08-tree.md#sec-8-6-1)).

**Random queue.** A bag from which `pop()` removes an item chosen uniformly
at random, by a draw with no bias, in $O( 1 )$ amortized time for `push()`
and $O( 1 )$ expected time for `pop()` ([§4.7](04-queue.md#sec-4-7)).

**Randomized binary search tree.** The search tree of Martínez and Roura,
which keeps the distribution of a random binary search tree under any
sequence of operations by inserting at the root with the right probability
([§8.6.6](08-tree.md#sec-8-6-6)).

**Randomized meldable heap.** The heap-ordered binary tree of Gambin and
Malinowski whose merge descends a random path, so that every operation
takes $O( \log n )$ expected time ([§9.6](09-priority.md#sec-9-6)).

**Randomized structure.** A structure that makes random choices depending
neither on its items nor on its operations, so that its expected bounds
hold for every input ([§1.5.4](01-introduction.md#sec-1-5-4)).

**Range filter.** A filter that answers whether a range of keys may hold a
key of its set, with no false negative and a bounded rate of false
positives ([§10.11](10-set.md#sec-10-11)).

**Range minimum query.** The query $\mathrm{RMQ}( i , j )$ of the place of
a minimum of $A[ i .. j ]$, answered in $O( 1 )$ time after a preprocessing
of $A$ ([§6.9](06-search.md#sec-6-9)).

**Range tree.** A balanced tree of points ordered by $x$ whose nodes keep
the points of their subtree by $y$, which counts the points of a rectangle
([§16.4.2](16-geometry.md#sec-16-4-2)).

**Rank.** The index $0 , \ldots , n - 1$ of an item of a sequence, by which
a LIST reaches its items ([§2.1](02-sequence.md#sec-2-1)).

**Rank (bit vector).** The number $\mathrm{rank}_b( i )$ of the bits equal
to $b$ before the place $i$ of a vector of bits
([§22.1](22-succinct.md#sec-22-1)).

**Rank error.** The number of the items smaller than the one that a relaxed
priority queue returns, 0 for an exact queue
([§24.9](24-concurrent.md#sec-24-9)).

**Rank (union-find).** An upper bound on the height of a tree of a forest
of parents, by which the union by rank hangs the lower tree under the
higher ([§11.3](11-mfset.md#sec-11-3)).

**Real-time queue.** A queue whose every operation takes $O( 1 )$ time in
the worst case, by a rotation spread over the operations (Hood and
Melville) ([§4.9](04-queue.md#sec-4-9)).

**Rear coding.** The code of a sorted dictionary that writes, in place of
the length of the common prefix, the number of symbols to take off the end
of the previous string ([§15.6.3](15-string.md#sec-15-6-3)).

**Record low.** An item of a stack that is not larger than any item below
it; the record lows of a stack, from the bottom up, are the minima that
`MinStack` keeps ([§3.5](03-stack.md#sec-3-5)).

**Red-Black tree.** A binary search tree whose nodes are red or black, with
no red node under a red one and the same number of black nodes on every
path to a leaf, which bounds its height by $2 \log_2( n + 1 )$
([§8.5.5](08-tree.md#sec-8-5-5)).

**Reduced costs.** The costs $c( i , j ) - u_i - v_j$ of an assignment
problem with respect to the dual variables $u$ and $v$, nonnegative at a
dual feasible solution ([§14.11.3](14-graph.md#sec-14-11-3)).

**Reference list.** In the codes of Boldi and Vigna for the graphs of the
Web, the list, among the $W$ previous ones, that shares the most successors
with the list being coded, which is then written as a copy list of the
reference (in copy blocks), the extra nodes as intervals and the residuals
([§22.10](22-succinct.md#sec-22-10)).

**Reflected Gray order.** The order of the tuples of a mixed radix in which
two consecutive tuples differ by $\pm 1$ in one place
([§18.5](18-combinatorial.md#sec-18-5)).

**Region.** Another name of an arena; in the concurrent cuckoo table, a set
of buckets under one lock ([§5.4](05-memory.md#sec-5-4)).

**Rehash.** The rebuilding of a hash table with new hash functions, e.g.,
after an insertion into a cuckoo table that gives up
([§7.7](07-dictionary.md#sec-7-7)).

**Relaxed (memory order).** The order of an atomic access that is atomic
and nothing more, with no ordering of the other accesses
([§24.1](24-concurrent.md#sec-24-1)).

**Relaxed priority queue.** A priority queue shared by threads that may
return an item which is not the least, in exchange for threads that do not
wait for one another ([§24.9](24-concurrent.md#sec-24-9)).

**Release.** The order of an atomic store which, read by a load with
acquire, makes the writes that preceded it visible to the loading thread
([§24.1](24-concurrent.md#sec-24-1)).

**Reoptimization.** The repair of a solution on a graph after a change,
tested against the certificate of the previous solution: harmless,
repairable or fatal ([§14.13.2](14-graph.md#sec-14-13-2)).

**Replacement selection.** The formation of the runs of an external sort
with a heap of $M$ items, which gives runs of about $2 M$ items on random
input ([§12.13](12-sort.md#sec-12-13)).

**Representative.** The element of a component of disjoint sets that
`find()` returns, the same for all its elements until the component changes
([§11.1](11-mfset.md#sec-11-1)).

**Reservoir sampling.** The uniform sample of $n$ items from a stream of
unknown length, in one pass and $O( n )$ space
([§17.10](17-numeric.md#sec-17-10)).

**Residual network.** The network of the residual capacities of a flow,
where the reverse of an edge with flow stands for the flow that can be sent
back ([§14.10.1](14-graph.md#sec-14-10-1)).

**Restricted growth string.** The code $a_0 a_1 \cdots a_{n-1}$ of a
partition of a set, in which $a_j$ is the block of $j$ and each block is
numbered at most one above the largest before it
([§18.7](18-combinatorial.md#sec-18-7)).

**Reversal of a stack by a queue.** The reversal of the order of the items
of a stack that has only `push()` and `pop()`, popping them all into a
queue and pushing them back in the order in which they leave it
([§3.7](03-stack.md#sec-3-7)).

**Reverse search path.** The path of a skip list from a node of the bottom
list up to the head, the search path read backwards, by whose length the
cost of a search is bounded ([§7.9.3](07-dictionary.md#sec-7-9-3)).

**Revolving door.** The order of the combinations in which two consecutive
combinations differ by one item in and one item out
([§18.7](18-combinatorial.md#sec-18-7)).

**Ribbon filter.** A static filter whose cells solve by Gauss elimination
on the fly a linear system over $\mathbb{F}_2$ whose equations have their
coefficients in a narrow band ([§10.10](10-set.md#sec-10-10)).

**Rollback union-find.** The union-find that undoes its merges in the
reverse of their order, with union by rank and no compression, the base of
the offline connectivity ([§11.5](11-mfset.md#sec-11-5)).

**Rooted tree.** A finite set of nodes, either empty or with a root and a
partition of the other nodes into subtrees whose roots are the children of
the root ([§8.1](08-tree.md#sec-8-1)).

**Rootish array stack.** The LIST of Brodnik et al. that keeps its items in
blocks of sizes $1 , 2 , 3 , \ldots$, wasting only $O( \sqrt{n} )$ places
([§2.6](02-sequence.md#sec-2-6)).

**Roots (garbage collection).** The cells that the program holds directly,
from which the reachable cells are marked ([§5.10](05-memory.md#sec-5-10)).

**Rotation (tree).** The local change of the shape of a binary search tree
that makes a child the parent of its parent and keeps the order of the
keys, in $O( 1 )$ time ([§8.5.4](08-tree.md#sec-8-5-4)).

**RRR vector.** The compressed vector of bits of Raman, Raman and Rao,
which writes each block of $b$ bits as its class (the number of 1s) and its
offset among the blocks of that class ([§22.5](22-succinct.md#sec-22-5)).

**Ruler function.** The place $\rho( x )$ of the lowest 1 of a nonzero
word, i.e., the number of its trailing zeros
([§18.1](18-combinatorial.md#sec-18-1)).

**Run.** A maximal stretch of a sequence that does not decrease (or
strictly decreases); the natural merge sorts merge the runs of the input
([§2.10.1](02-sequence.md#sec-2-10-1)).

**Samplesort.** The sort that distributes the items into $k$ buckets by
$k - 1$ splitters drawn from a sample, and sorts the buckets recursively
([§12.8](12-sort.md#sec-12-8)).

**Satisfiability.** The problem whether a formula in conjunctive normal
form has a model, the first problem proved NP-complete
([§19.7](19-backtrack.md#sec-19-7)).

**Satisfied clause.** A clause one of whose literals is true under the
current partial assignment ([§19.7](19-backtrack.md#sec-19-7)).

**Saturated array.** An array of packed fields that stores a value above
the largest that fits as that largest value, and counts the values clamped
([§22.7](22-succinct.md#sec-22-7)).

**Scapegoat.** The ancestor of a deep new node whose subtree a scapegoat
tree rebuilds perfectly balanced ([§8.9.2](08-tree.md#sec-8-9-2)).

**Scapegoat tree.** The binary search tree of Galperin and Rivest, which
keeps no balance information in the nodes and rebuilds the subtree of a
scapegoat when an insertion makes the tree too deep
([§8.9.2](08-tree.md#sec-8-9-2)).

**Search path.** The sequence of the nodes that a search visits from the
head of a skip list (or the root of a tree) down to the place of the key
([§7.9.1](07-dictionary.md#sec-7-9-1)).

**Search space.** A set of objects, with a function of admissibility and
one of answer, among which a technique looks for the solutions of an
instance ([§13.4](13-algorithm.md#sec-13-4)).

**Secondary clustering.** The sharing of a whole inspection by the keys
that have the same first place, as in the quadratic inspection
([§7.5](07-dictionary.md#sec-7-5)).

**Segment.** A run $D[ b .. s ]$ of consecutive items of an array
([§13.5](13-algorithm.md#sec-13-5)).

**Selection.** The search of the item of rank $k$ of an unsorted vector, by
a partition around a pivot as in quicksort but recurring on one side only,
in $O( n )$ expected time ([§6.8](06-search.md#sec-6-8)).

**Semantic specification.** The part $A$ of a specification that fixes the
meaning of the operations, algebraically by axioms or constructively by
contracts ([§1.2](01-introduction.md#sec-1-2)).

**Sentinel.** A cell with no item that stands before the first and after
the last cell of a list (or below the leaves of a tree), so that no
operation needs a special case at the ends
([§2.10](02-sequence.md#sec-2-10)).

**Separator.** A key of an internal node of a B+-tree, which separates the
keys of two children and carries no value
([§8.11.2](08-tree.md#sec-8-11-2)).

**Seqlock.** A lock under which the readers take no lock but read a version
before and after their read, and read again if it changed or was odd
([§24.7](24-concurrent.md#sec-24-7)).

**Sequence.** A finite list $s_0 , \ldots , s_{n-1}$ of items, possibly
repeated, in which the order matters, reached through positions or ranks
([Chapter 2](02-sequence.md#ch-2)).

**Sequence heap.** The priority queue of Sanders for the cache, which
inserts into a small heap and moves the full heaps into sorted runs merged
by loser trees ([§9.12](09-priority.md#sec-9-12)).

**Sequentially consistent.** The order of an atomic access that is acquire
and release and also takes a place in a single total order of all such
accesses ([§24.1](24-concurrent.md#sec-24-1)).

**Shared object.** A structure whose operations are called by several
threads at once ([§24.1](24-concurrent.md#sec-24-1)).

**Shellsort.** The insertion sort on the items $h$ places apart for a
decreasing sequence of gaps $h$, the last being 1
([§12.3](12-sort.md#sec-12-3)).

**Shift-Or.** The search of a pattern of at most $w$ symbols that keeps in
one word the state of all the prefixes of the pattern that match the text,
updated by a shift and an or per symbol
([§15.2.1](15-string.md#sec-15-2-1)).

**Shunting-yard algorithm.** The algorithm of Dijkstra that translates an
infix expression with precedences into postfix notation by a stack of the
operators whose right operand is not complete yet
([§3.6](03-stack.md#sec-3-6)).

**Sideways sum.** The number $\nu( x )$ of the 1 bits of a word, also
called its population count ([§18.1](18-combinatorial.md#sec-18-1)).

**Sieve of Eratosthenes.** The computation of the primes up to $n$ by
crossing out the multiples of each prime ([§17.8](17-numeric.md#sec-17-8)).

**Simple tabulation.** The hash function that cuts a key into characters of
$r$ bits and XORs the random words that a table per character gives them
([§7.3.4](07-dictionary.md#sec-7-3-4)).

**Simple uniformity.** The hypothesis on a hash function that each place
receives the same share of the keys, weighted by their probabilities
([§7.1](07-dictionary.md#sec-7-1)).

**Singly linked list.** A list whose cells keep only the address of the
next cell; it inserts after a cell and at its head in $O( 1 )$ time
([§2.9](02-sequence.md#sec-2-9)).

**Size class.** One of the few sizes to which an allocator rounds the
requests, which removes the external fragmentation within a class at the
price of an internal one ([§5.6](05-memory.md#sec-5-6)).

**Size-balanced.** Said of a binary tree whose two subtrees of each node
differ by at most one node ([§8.4.4](08-tree.md#sec-8-4-4)).

**Skip list.** A sorted linked list with express lanes, the lists $L_0
\supseteq L_1 \supseteq \cdots$ obtained by tossing a coin for each key,
which searches in $O( \log n )$ expected time
([§7.9](07-dictionary.md#sec-7-9)).

**Slack.** The nonnegative quantity of an edge in the dual of the weighted
matching, which is zero on the edges that may enter the matching
([§14.11.2](14-graph.md#sec-14-11-2)).

**Small vector.** A vector that keeps its first few items inside the object
itself and allocates only beyond them ([§2.2.1](02-sequence.md#sec-2-2-1)).

**Sort by comparisons.** A sort that learns about the items only through
the order, and is therefore bound by $\Omega( n \log n )$ comparisons in
the worst case ([§12.1](12-sort.md#sec-12-1)).

**Sort by digits.** A sort that reads the keys as integers or strings and
uses their parts as indices, and is therefore not bound by the lower bound
of the comparisons ([§12.1](12-sort.md#sec-12-1)).

**Sort by passes.** The bubble sort on a stack, made with the operations of
the stack and the recursion only: each pass carries a largest item to the
bottom, $n - 1$ passes sort the stack, and the $n$ passes of `sortStack()`
take $\Theta( n^2 )$ operations on every input
([§3.7](03-stack.md#sec-3-7)).

**Sorting by regular sampling.** The distributed sort of Shi and Schaeffer
in which each process sorts its part, sends regular samples to choose $P -
1$ splitters, and exchanges its items in one all-to-all
([§26.2](26-distributed.md#sec-26-2)).

**Sorting network.** A fixed sequence of comparators, each putting the
smaller of two places first whatever the input, that sorts every input
([§12.12](12-sort.md#sec-12-12)).

**Space.** The memory that a structure takes, its items and its overhead,
counted in words ([§1.7.3](01-introduction.md#sec-1-7-3)).

**Spare.** The second level of a prefix filter, a blocked filter of Bloom
that takes the fingerprints for which the bins have no room
([§10.9](10-set.md#sec-10-9)).

**Sparse graph.** A graph whose number of edges is of the order of the
number of its nodes; a dense graph has the order of their square
([§14.1](14-graph.md#sec-14-1)).

**Sparse matrix.** A matrix most of whose entries are 0, of which only the
nonzero entries are stored, with their places
([§17.7](17-numeric.md#sec-17-7)).

**Sparse set.** The representation of a subset of $\{ 0 , \ldots , n - 1
\}$ by a dense vector of its items and a vector of their places, with
insertion, removal and membership in $O( 1 )$ time; the "dancing cells" of
Knuth ([§19.6](19-backtrack.md#sec-19-6)).

**Sparse table.** The structure that answers the range minima in $O( 1 )$
time by storing the minimum of each range of length $2^L$, in $O( n \log n
)$ space ([§6.9](06-search.md#sec-6-9)).

**Spatial coupling.** The placement of the cells of a key of a static
filter in consecutive segments, which lets the linear system be solved with
fewer cells ([§10.10](10-set.md#sec-10-10)).

**Specification.** A triple $( D , F , A )$ of the domains, the operations
and the axioms (or contracts) of an abstract data type
([§1.2](01-introduction.md#sec-1-2)).

**Spectral test.** The test of a linear congruential generator that looks
at its structure rather than at its output: its pairs $( X_k , X_{k+1} )$
lie on parallel lines at the distance $1 / \nu_2$, where $\nu_2$ is the
length of the shortest nonzero vector of a lattice
([§17.9](17-numeric.md#sec-17-9)).

**Speedup.** The ratio $T_1 / T_p$ of the time on one thread to the time on
$p$ threads, at most $p$; it is linear when it is $\Theta( p )$
([§25.1](25-parallel.md#sec-25-1)).

**Splay.** The sequence of zig, zig-zig and zig-zag steps that brings an
accessed node of a splay tree to the root ([§8.8](08-tree.md#sec-8-8)).

**Splay tree.** The binary search tree of Sleator and Tarjan, which keeps
no balance information and splays each node accessed to the root, in $O(
\log n )$ amortized time per operation ([§8.8](08-tree.md#sec-8-8)).

**Splice.** The removal of a node with one child from a binary search tree,
by which the child takes its place under its parent
([§8.5.3](08-tree.md#sec-8-5-3)).

**Splicing.** The step of the algorithm of Rem for union and find that,
while a merge climbs the two paths together, gives the node with the
smaller parent the parent of the other node, splicing it into the other
path ([§11.4](11-mfset.md#sec-11-4)).

**Split (B-tree).** The repair of a full node of a B-tree (or of a 2-4 tree
with five children) that cuts it into two and moves the middle key up to
the parent ([§8.10.1](08-tree.md#sec-8-10-1)).

**Splitter.** One of the $k - 1$ keys drawn from a sample by which
samplesort distributes the items into $k$ buckets
([§12.8](12-sort.md#sec-12-8)).

**Stabbing query.** The search of the intervals of a set that contain a
point $x$, which an interval tree answers
([§16.4.1](16-geometry.md#sec-16-4-1)).

**Stable sort.** A sort in which equal items keep the order that they had
in the input ([§12.1](12-sort.md#sec-12-1)).

**Stack.** A sequence to which items are added, and from which they are
removed, at one end only, its top (LIFO) ([Chapter 3](03-stack.md#ch-3)).

**Stash.** A few places of a cuckoo table that keep the keys whose
insertion failed, read by every search ([§7.7](07-dictionary.md#sec-7-7)).

**Static filter.** A filter built once from its set, with no later
insertion ([§10.1](10-set.md#sec-10-1)).

**Strassen multiplication.** The product of two matrices by seven products
of blocks instead of eight, in $O( n^{\log_2 7} )$ time
([§17.6](17-numeric.md#sec-17-6)).

**Stream VByte.** The variable-byte code with the lengths of the integers
in separate control bytes, decoded by one shuffle of bytes per group
([§21.8](21-compression.md#sec-21-8)).

**Strongly connected components.** The classes of the nodes of an oriented
graph that reach one another along the orientation of the edges
([§14.5.5](14-graph.md#sec-14-5-5)).

**Subsequence.** A sequence obtained from another by keeping some of its
symbols in their order, not necessarily contiguous
([§15.1](15-string.md#sec-15-1)).

**Substring.** A string of consecutive symbols of a string, $T[ i .. j ]$
([§15.1](15-string.md#sec-15-1)).

**Subtree.** The tree rooted at a node that holds all its descendants
([§8.1](08-tree.md#sec-8-1)).

**Successor search.** The search of the least key $y \ge x$ of a sorted
set, meaningful even when no key equals $x$; `lowerBound( x )` in ADS
([§1.3.3](01-introduction.md#sec-1-3-3)).

**Succinct representation.** A representation of an object of a class of
$N$ elements in $\lceil \log_2 N \rceil + o( \log_2 N )$ bits, answering
its queries in about the time of the plain one
([§22.1](22-succinct.md#sec-22-1)).

**Sufficiently complete.** Said of the axioms of an abstract data type that
decide the value of each observer on each value of the type
([§1.2](01-introduction.md#sec-1-2)).

**Suffix.** A substring that ends at the last symbol of a string
([§15.1](15-string.md#sec-15-1)).

**Suffix array.** The permutation of the starting places of the suffixes of
a text that lists them in lexicographic order
([§15.5.1](15-string.md#sec-15-5-1)).

**Suffix automaton.** The smallest deterministic automaton that accepts the
suffixes of a string (the directed acyclic word graph), built online in
linear time ([§15.5.4](15-string.md#sec-15-5-4)).

**Suffix link.** The pointer from the node of a suffix tree (or the state
of a suffix automaton) spelling $c \alpha$ to the one spelling $\alpha$
([§15.5.3](15-string.md#sec-15-5-3)).

**Suffix sorting.** The construction of the suffix array, e.g., by the
induced sorting of SA-IS in linear time
([§15.5.2](15-string.md#sec-15-5-2)).

**Suffix tree.** The compact trie of the suffixes of a text with a
terminator, each suffix ending in a leaf of its own
([§15.5.3](15-string.md#sec-15-5-3)).

**Swiss table.** The layout of an open-addressing table in groups of
places, each with a control byte holding a fingerprint of 8 bits of the
hash, compared all at once by one SIMD instruction
([§7.6](07-dictionary.md#sec-7-6)).

**Symmetric visit.** The visit of a binary tree that examines each node
between its left and its right subtree ([§8.1](08-tree.md#sec-8-1)).

**Syntactic specification.** The pair $( D , F )$ of the domains and the
operations of an abstract data type with their types, which ADS writes as a
concept ([§1.2](01-introduction.md#sec-1-2)).

**Tabulation hashing.** See simple tabulation
([§7.3.4](07-dictionary.md#sec-7-3-4)).

**Terminated.** Said of a computation by messages in which all the workers
are passive and no message is on its way, a state that lasts once reached
([§25.15](25-parallel.md#sec-25-15)).

**Terminator.** A symbol smaller than all the others appended to a text, so
that no suffix is a prefix of another ([§15.1](15-string.md#sec-15-1)).

**Textbook level.** The level of ADS that realizes each structure in the
form of a textbook, with its pseudocode, contracts and exercises, written
to be read ([§1.9](01-introduction.md#sec-1-9)).

**Theory page.** The page `docs/theory/<area>.dox` of the Doxygen
documentation that collects the definitions, the invariants and the
arguments behind the costs of an area
([§A.5](A-writing-structure.md#sec-A-5)).

**Thread pool.** The executor of ADS on `std::thread`, whose workers take
their tasks from deques of work stealing
([§25.3](25-parallel.md#sec-25-3)).

**Threaded tree.** A binary tree whose missing children are replaced by
links (threads) to the predecessor and the successor in symmetric order, so
that the visit needs no stack ([§8.3.4](08-tree.md#sec-8-3-4)).

**Tiered vector.** The LIST of Goodrich and Kloss that keeps its items in
circular blocks of $b$ places, $b$ a power of 2, so that an insertion in
the middle shifts $O( \sqrt{n} )$ items and the access by index stays $O( 1
)$ ([§2.7](02-sequence.md#sec-2-7)).

**Time.** The cost of an operation in the model of computation, bounded in
the worst case, amortized or in expectation
([§1.7.2](01-introduction.md#sec-1-7-2)).

**Tombstone.** A mark left in place of a removed key (a pair of a dynamic
PGM-index, a place of an open-addressing table) so that the searches go on
past it ([§6.6](06-search.md#sec-6-6)).

**Top.** The end of a stack at which the items are added and removed
([Chapter 3](03-stack.md#ch-3)).

**Topological order.** An order of the nodes of an oriented graph in which
every edge goes from an earlier to a later node; it exists if and only if
the graph is acyclic ([§14.5.4](14-graph.md#sec-14-5-4)).

**Towers of Hanoi.** The example of the elimination of the recursion, in
which $2^n - 1$ moves carry $n$ disks between three pegs
([§13.3](13-algorithm.md#sec-13-3)).

**Trail.** A path that uses each edge at most once; an Eulerian trail uses
each edge exactly once ([§14.12.1](14-graph.md#sec-14-12-1)).

**Transdichotomous.** Said of a model of computation in which the length
$w$ of a word grows with the size $n$ of the input, at least as $\log_2 n$
([§1.6](01-introduction.md#sec-1-6)).

**Treap.** The binary search tree of Seidel and Aragon whose nodes also
carry random priorities in heap order, so that its shape is that of a
random binary search tree under any sequence of operations
([§8.6.2](08-tree.md#sec-8-6-2)).

**Tree of losers.** See loser tree ([§9.11](09-priority.md#sec-9-11)).

**Tree of shortest paths.** A tree rooted at the source, spanning the nodes
reachable from it, in which no path from the source costs less than the
path in the tree ([§14.8.1](14-graph.md#sec-14-8-1)).

**Tree of the choices.** The tree of the partial solutions of a backtrack,
rooted at the empty vector, each node having one child per choice of its
next item ([§19.1](19-backtrack.md#sec-19-1)).

**Treque.** The LIST of Morin whose insertions and removals are fast near
either end and near the middle, made of two deques
([§2.5](02-sequence.md#sec-2-5)).

**Trie.** A tree that stands for a set of strings, the node at depth $d$
being a prefix of $d$ characters and the children indexed by the next
character ([§7.10](07-dictionary.md#sec-7-10)).

**Trit.** A value of three states; five of them fit in one byte as a
number in base 3, at most $3^5 - 1 = 242$
([§22.7](22-succinct.md#sec-22-7)).

**Turtle.** A small item near the end of a vector, which a bubble sort
moves one place to the left per pass ([§12.3](12-sort.md#sec-12-3)).

**Two-level memory.** The model with an internal memory of $M$ items and an
unbounded disk of blocks of $B$ items, in which the cost is the number of
I/Os ([§23.1](23-external.md#sec-23-1)).

**Two-level perfect hashing.** The scheme of Fredman, Komlós and Szemerédi,
which hashes $n$ keys into $n$ lists and turns each list of $n_j$ keys into
a table of $n_j^2$ places with no collision, in $O( n )$ space
([§7.8.1](07-dictionary.md#sec-7-8-1)).

**Two-level storage.** The intersection of sorted sequences that keeps the
first item of each block of $L$ items apart, merges these with the shorter
sequence, and searches only the blocks that may hold an item
([§10.6](10-set.md#sec-10-6)).

**Two-Way search.** The search of a pattern of Crochemore and Perrin at a
critical factorization, in linear time and $O( 1 )$ extra space
([§15.2.1](15-string.md#sec-15-2-1)).

**Uniform hashing.** The model of the analysis of open addressing, in which
the inspection of each key is a random permutation of the places,
independent of the others ([§7.5](07-dictionary.md#sec-7-5)).

**Union by rank.** The rule of a union-find that hangs the root of lower
rank under the other, the rank growing by 1 only on a tie
([§11.3](11-mfset.md#sec-11-3)).

**Union by size.** The rule of a union-find that hangs the tree with fewer
nodes under the other ([§11.3](11-mfset.md#sec-11-3)).

**Union-find.** The structure of disjoint sets, with `find()` of the
representative and `merge()` of two components, in nearly constant
amortized time with union by rank and compression ([Chapter
11](11-mfset.md#ch-11)).

**Unit clause.** A clause all of whose literals are false but one, which is
free and must therefore be true in every model that extends the assignment
([§19.7](19-backtrack.md#sec-19-7)).

**Universal class.** A class of hash functions under which two distinct
keys collide with probability at most $1 / m$ over the choice of the
function ([§7.3.2](07-dictionary.md#sec-7-3-2)).

**Universal sink.** A node of an oriented graph that is the target of $n -
1$ edges and the source of none ([§14.7.2](14-graph.md#sec-14-7-2)).

**Universe.** The set from which the keys of a dictionary are drawn
([§7.1](07-dictionary.md#sec-7-1)).

**Unrolled list.** A doubly linked list of blocks, each holding several
items, which saves pointers and scans contiguous memory
([§2.12](02-sequence.md#sec-2-12)).

**Variable-byte code.** The code of an integer in groups of 7 bits, one per
byte, the high bit of each byte telling whether another follows
([§21.7](21-compression.md#sec-21-7)).

**Vector of parents.** The representation of a tree by the parent of each
node, $-1$ for the root, which answers only the parent in $O( 1 )$ time
([§8.2.4](08-tree.md#sec-8-2-4)).

**Vectors of adjacency.** The representation of a static graph that packs
all its lists of adjacency into one vector, with the start of the list of
each node in another ([§14.4.1](14-graph.md#sec-14-4-1)).

**Virtual tree.** The splay trees of the preferred paths of a link-cut tree
together with the path-parent pointers, which represent the forest
([§8.13.2](08-tree.md#sec-8-13-2)).

**Visit (graph).** The search from a node of all the nodes reachable from
it, each once, along the edges; in breadth with a queue, in depth with a
stack or by recursion ([§14.5.1](14-graph.md#sec-14-5-1)).

**Visit in breadth.** The visit of a graph whose frontier is a queue, which
reaches the nodes by increasing distance in edges from the source
([§14.5.2](14-graph.md#sec-14-5-2)).

**Visit in depth.** The visit of a graph whose frontier is a stack, i.e.,
the order of a recursion, whose times of entry and exit classify the edges
([§14.5.3](14-graph.md#sec-14-5-3)).

**Visit (tree).** The examination of each node of a tree once, as a
previsit, a postvisit, an invisit or by levels
([§8.1](08-tree.md#sec-8-1)).

**Wait-free.** A concurrent implementation in which each running thread
completes each of its operations in a finite number of its own steps
([§24.1](24-concurrent.md#sec-24-1)).

**Warnsdorff rule.** The heuristic of the tour of the knight that moves to
the square with fewest onward moves ([§19.4](19-backtrack.md#sec-19-4)).

**Watched literals.** The two literals of each clause that a CDCL solver
watches, so that a clause is visited only when one of them becomes false
([§19.7](19-backtrack.md#sec-19-7)).

**Wave.** A reduction that does not block, by which the processes sum their
counts of the messages sent and received to detect the termination
([§26.5](26-distributed.md#sec-26-5)).

**Wavelet matrix.** The succinct representation of a sequence of integers
of $w$ bits as $w$ vectors of bits with rank and select, which answers
access, rank and select on the symbols in $O( w )$ time
([§22.9](22-succinct.md#sec-22-9)).

**Width.** The number of the items of the bottom list that a link of a skip
list jumps over, by which the skip list reaches the items by rank
([§2.14](02-sequence.md#sec-2-14)).

**With high probability.** Said of a bound that holds with probability at
least $1 - n^{-c}$ for a constant $c > 0$
([§1.5.4](01-introduction.md#sec-1-5-4)).

**Witness.** A base that shows an odd number $n$ composite, since it
violates the condition that every base satisfies when $n$ is prime, or
shares a factor with $n$; the test of Miller and Rabin looks for one among
$K$ random bases ([§17.8](17-numeric.md#sec-17-8)).

**Word.** An unsigned integer of 64 bits treated as a set of the places of
its 1 bits, with the bit $x_k$ in the place $k$
([§18.1](18-combinatorial.md#sec-18-1)).

**Word-RAM.** The random access machine whose memory is a vector of words
of $w$ bits, with the arithmetic and logical operations on words in
constant time ([§1.6](01-introduction.md#sec-1-6)).

**Work.** The total time $W$ of the tasks of a parallel computation, i.e.,
its time on one thread ([§25.1](25-parallel.md#sec-25-1)).

**Work stealing.** The scheduling in which each worker takes its tasks from
the bottom of its own deque and, when it is empty, steals from the top of
the deque of another worker ([§24.5](24-concurrent.md#sec-24-5)).

**Work-efficient.** Said of a parallel algorithm whose work is, up to a
constant factor, the time of the best sequential algorithm
([§25.1](25-parallel.md#sec-25-1)).

**Worker.** One of the threads of a pool (or of the processes of a
distributed computation) that run the tasks
([§25.3](25-parallel.md#sec-25-3)).

**Working set theorem.** The bound of Sleator and Tarjan by which an access
to a key of a splay tree that was accessed $k$ accesses before costs $O(
\log k )$ amortized time ([§8.8](08-tree.md#sec-8-8)).

**Worst-case bound.** A bound $f( n )$ on the time of each single operation
([§1.7.2](01-introduction.md#sec-1-7-2)).

**X-fast trie.** The binary trie that keeps a hash table of the prefixes of
each level, so that the deepest node of the path of a key is found by a
binary search on the depth in $O( \log w )$ time
([§7.11.2](07-dictionary.md#sec-7-11-2)).

**XOR list.** A doubly linked list whose cells keep one word of links, the
exclusive or of the addresses of their two neighbors
([§2.13](02-sequence.md#sec-2-13)).

**Xoshiro.** The generators of pseudorandom words of Blackman and Vigna on
a state of a few words changed by shifts, rotations and exclusive ors
([§17.9](17-numeric.md#sec-17-9)).

**Y-fast trie.** The structure of Willard that keeps a random sample of the
keys, the representatives, in an x-fast trie and the groups between them in
balanced trees, with $O( \log w )$ expected time per operation in $O( n )$
space ([§7.11.3](07-dictionary.md#sec-7-11-3)).

**Z-function.** The array whose entry $i$ is the length of the longest
common prefix of a string and of its suffix $i$, computed in linear time
([§15.2.1](15-string.md#sec-15-2-1)).

**Zig, zig-zig, zig-zag.** The three steps of a splay: a single rotation at
the root, two rotations in the same direction, and a double rotation
([§8.8](08-tree.md#sec-8-8)).

**Ziggurat.** The method that draws normal and exponential variates by
covering the density with layers of equal area, with one comparison on most
draws ([§17.9](17-numeric.md#sec-17-9)).
