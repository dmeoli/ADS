#!/bin/bash
###############################################################################
#                                  run.sh                                     #
#                                                                             #
# Runs all the benchmarks on one core and writes their lines, with a header   #
# that records the machine, the compiler, the load and the date, to           #
# results/<host>-<date>.tsv; then bench/tables.py turns the file into the     #
# tables of the README. With ISOLATE=1, the default, each structure of each   #
# benchmark runs in a process of its own (the binary lists its labels under   #
# ADS_BENCH_LIST=1, then runs once per label under ADS_BENCH_WHO), so that    #
# no cell depends on the structures run before it in the same process (the    #
# state of the heap, the caches, the layout of the code); ISOLATE=0 runs each #
# benchmark in one process. The core comes from CORE (default 0), the second  #
# one of the benchmark of two threads from CORE2 (default CORE + 1), the runs #
# per cell from RUNS (default 11), the build directory from BUILD, the text   #
# of the compressors from ADS_BENCH_TEXT (a file, repeated up to each size),  #
# the cores of the parallel algorithms from PCORES (default CORE).            #
#                                                                             #
#   bench/run.sh            after: make bench [GMP=<prefix>] [FFTW=<prefix>]  #
#                           [HASHING=<dir>] [ABSL=<prefix>]                   #
#                           [LIBSAIS=<prefix>] [DIVSUFSORT=<prefix>]          #
#                           [EDLIB=<prefix>] [RAPIDFUZZ=<prefix>]             #
#                           (with GMP also bignum-gmp, BigInteger on GMP)     #
#                           [FLINT=<prefix>]                                  #
#                           [PRIORITY=<dir>] [MIMALLOC=<prefix>]              #
#                           [JEMALLOC=<prefix>] [OPT=<prefix>]                #
#                           [SORTING=<dir>] [HWY=<prefix>] [TLX=<dir>]        #
#                           [COMBI=<dir>] [MINISELECT=<dir>] [SDSL=<dir>]     #
#                           [PGM=<dir>] [LEMIRE=<prefix>] [ROARING=<prefix>]  #
#                           [HTRIE=<dir>] [LIBART=<prefix>]                   #
#                           [HYPERSCAN=<prefix>]                              #
#                           [POCKETFFT=<dir>] [DUCC0=<dir>]                   #
#                           [TBB=1] [FASTFLOW=<dir>] [PARLAY=<dir>]           #
#                           [GAPBS=<dir>] [EIGEN=<dir>] [OPENBLAS=<prefix>]   #
###############################################################################

set -e
BUILD=${BUILD:-build-make}
CORE=${CORE:-0}
RUNS=${RUNS:-11}
ISOLATE=${ISOLATE:-1}
mkdir -p results
out=results/$(hostname -s)-$(date +%Y%m%d-%H%M).tsv
{
 echo "# machine	$(lscpu | sed -n 's/^Model name: *//p')"
 echo "# cores	$(nproc)"
 echo "# compiler	$(${CXX:-g++} --version | head -1)"
 echo "# load	$(cut -d' ' -f1-3 /proc/loadavg)"
 echo "# date	$(date -Iseconds)"
 echo "# core	$CORE"
 echo "# runs	$RUNS"
 echo "# processes	$([ "$ISOLATE" = 1 ] && echo one per structure ||
                       echo one per benchmark)"
 echo "# text	${ADS_BENCH_TEXT:-words of a Zipf law}"
} > "$out"
# the lines of the benchmark $2 on the cores $1, its arguments after them,
# with ISOLATE a process per structure
cells() {
 local cores=$1 b=$2 whos who
 shift 2
 if [ "$ISOLATE" != 1 ]; then
  taskset -c "$cores" "$BUILD/bench-$b" -r "$RUNS" "$@"
  return
 fi
 whos=$(ADS_BENCH_LIST=1 taskset -c "$cores" "$BUILD/bench-$b" -r "$RUNS" \
        "$@")
 while IFS= read -r who; do
  [ -n "$who" ] || continue
  ADS_BENCH_WHO=$who taskset -c "$cores" "$BUILD/bench-$b" -r "$RUNS" "$@" \
   < /dev/null
 done <<< "$whos"
}
run() { cells "$CORE" "$@" >> "$out"; }
# the sorts print a header and a table per type: "sort <type>" is the
# benchmark of each type, the pattern its case
cells "$CORE" sort 100000 1000000 |
 awk -F'\t' -v OFS='\t' '$1 != "type" || $2 != "pattern" {
  print "sort " $1 , $2 , $3 , $4 , $5 }' >> "$out"
run dictionaries 100000 1000000
run priority 100000 1000000
run sequences 100000 10000000
run memory 100000 1000000
run bignum 16 256 1024 4096 32768
[ -x "$BUILD/bench-bignum-gmp" ] && run bignum-gmp 16 256 1024 4096 32768
run fft 64 1000 1024 16384 100000 131072 1048576
run hashing 100000 1000000
run search 100000 1000000 10000000
run multi 1000000 10000000
run suffix 100000 1000000 10000000
run lce 100000 1000000 10000000
run edit 100 1000 10000 100000
run numeric 100000 1000000
run queues 100000 1000000
run geometry 100000 1000000
run matrix 64 256 1024 2048 4096
run sparse 100000 1000000
run graphs 100000 1000000
# the queues between threads on the core and the next one allowed
cells "$CORE,${CORE2:-$((CORE + 1))}" concurrent 1000000 10000000 >> "$out"
run succinct 100000 1000000 10000000 100000000
run compression 100000 1000000 10000000
run external merge 1000000 10000000
run external book 1000000 10000000
run external sampling 1000 100000 1000000
run combinatorial 100000 1000000
run backtrack
run ordered 100000 1000000
run arrays 100000 1000000 10000000
run sets 100000 1000000
run tries 100000 1000000
# the allocators, whose case cross-thread frees in a second thread
cells "$CORE,${CORE2:-$((CORE + 1))}" allocators 100000 1000000 >> "$out"
run positions 100000 1000000
run assembly 100000 1000000
run fingerprints 100000 1000000
# the parallel algorithms on the cores PCORES (default the core CORE
# alone), on 1, 2, 4, ... threads up to their number; the GAP benchmark
# suite prints lines of its own, left out
PCORES=${PCORES:-$CORE}
np=$(taskset -c "$PCORES" nproc)
for t in 1 2 4 8 16 32 64 128; do
 [ "$t" -le "$np" ] || break
 cells "$PCORES" parallel -t "$t" 1000000 10000000 | grep '^parallel' \
  >> "$out" || true
done
echo "# load at the end	$(cut -d' ' -f1-3 /proc/loadavg)" >> "$out"
echo "$out"
###############################################################################
#                              End of run.sh                                  #
###############################################################################
