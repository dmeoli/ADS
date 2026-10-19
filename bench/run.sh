#!/bin/bash
###############################################################################
#                                  run.sh                                     #
#                                                                             #
# Runs all the benchmarks on one core and writes their lines, with a header   #
# that records the machine, the compiler, the load and the date, to           #
# results/<host>-<date>.tsv; then bench/tables.py turns the file into the     #
# tables of the README. The core comes from CORE (default 0), the runs per    #
# cell from RUNS (default 11), the build directory from BUILD.                #
#                                                                             #
#   bench/run.sh            after: make bench [GMP=<prefix>] [FFTW=<prefix>]  #
#                           [HASHING=<dir>] [ABSL=<prefix>]                   #
#                           [LIBSAIS=<prefix>] [DIVSUFSORT=<prefix>]          #
#                           [EDLIB=<prefix>] [RAPIDFUZZ=<prefix>]             #
#                           (with GMP also bignum-gmp, BigInteger on GMP)     #
###############################################################################

set -e
BUILD=${BUILD:-build-make}
CORE=${CORE:-0}
RUNS=${RUNS:-11}
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
} > "$out"
run() { taskset -c "$CORE" "$BUILD/bench-$1" -r "$RUNS" "${@:2}" >> "$out"; }
# the sorts print a header and a table per type: "sort <type>" is the
# benchmark of each type, the pattern its case
taskset -c "$CORE" "$BUILD/bench-sort" -r "$RUNS" 100000 1000000 |
 awk -F'\t' -v OFS='\t' 'NR > 1 { print "sort " $1 , $2 , $3 , $4 , $5 }' \
 >> "$out"
run dictionaries 100000 1000000
run priority 100000 1000000
run sequences 100000 10000000
run bignum 16 256 4096 32768
[ -x "$BUILD/bench-bignum-gmp" ] && run bignum-gmp 16 256 4096 32768
run fft 1024 65536 1048576
run hashing 100000 1000000
run search 100000 1000000 10000000
run suffix 100000 1000000 10000000
run edit 100 1000 10000 100000
echo "# load at the end	$(cut -d' ' -f1-3 /proc/loadavg)" >> "$out"
echo "$out"
###############################################################################
#                              End of run.sh                                  #
###############################################################################
