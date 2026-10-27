###############################################################################
#                                makefile                                     #
#                                                                             #
# The same as CMakeLists.txt without CMake: it builds one program per         #
# chapter in $(BUILD), runs the tests of test/tests.txt and checks that the   #
# list and the programs agree, and installs the headers.                      #
#                                                                             #
#   make             builds the tests                                         #
#   make test        runs them                                                #
#   make test-mpi    the sort over the processes of MPI, by mpicxx and mpirun #
#   make test ADS_WITH_GMP=1 [GMP=<prefix>]                                   #
#                    the same with BigInteger on the backend of GMP           #
#   make bench       builds the benchmarks in $(BUILD)                        #
#   bench/run.sh     runs them and writes the tables of the README            #
#   make install     copies include/ADS into $(PREFIX)/include                #
###############################################################################

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
BUILD    ?= build-make
PREFIX   ?= /usr/local

INC      = -Iinclude -Itest -DADS_CONTRACTS=3

# ADS_WITH_GMP=1: BigInteger on GMP where it is faster [see BigInteger.h],
# with GMP=<prefix> if GMP is not where the compiler looks
comma    := ,
GMPLINK   = $(if $(GMP),-I$(GMP)/include -L$(GMP)/lib \
            -Wl$(comma)-rpath$(comma)$(GMP)/lib) -lgmp
ifeq ($(ADS_WITH_GMP),1)
INC      += -DADS_WITH_GMP
LIBS     += $(GMPLINK)
endif
HEADERS  = $(shell find include/ADS -name '*.h' -o -name '*.tpp') \
           test/ADSTest.h
PROGRAMS = $(shell awk '/^[a-z]/{print $$1}' test/tests.txt | sort -u)
BINS     = $(addprefix $(BUILD)/,$(PROGRAMS))

all: $(BINS)

$(BUILD)/%: test/%.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $< -pthread $(LIBS)

$(BUILD):
	mkdir -p $@

# the sort over the processes of MPI, not in tests.txt: make test-mpi
# [MPICXX=mpicxx] [MPIRUN=mpirun], on 1 to 4 processes (Open MPI is told
# that they may outnumber the cores)
MPICXX   ?= mpicxx
MPIRUN   ?= mpirun $(if $(shell $(MPICXX) --showme:version 2>/dev/null),\
            --oversubscribe)
test-mpi: | $(BUILD)
	$(MPICXX) $(CXXFLAGS) $(INC) -DADS_WITH_MPI -o $(BUILD)/engineered_mpi \
	 test/engineered_mpi.cpp -pthread
	for t in distributed-sort distributed-sort-counts; do \
	  for np in 1 2 3 4; do \
	    $(MPIRUN) -np $$np $(BUILD)/engineered_mpi $$t || exit 1; \
	  done; \
	done

# each line of tests.txt is a test, and each program must have the tests of
# the list, no more and no less
test: $(BINS)
	@failed=0; \
	for p in $(PROGRAMS); do \
	  listed=$$(awk -v p=$$p '$$1 == p {print $$2}' test/tests.txt | sort); \
	  found=$$($(BUILD)/$$p -l | sort); \
	  if [ "$$listed" != "$$found" ]; then \
	    echo "FAILED $$p.registry"; failed=$$((failed + 1)); \
	  else echo "PASSED $$p.registry"; fi; \
	  for t in $$listed; do \
	    $(BUILD)/$$p $$t || failed=$$((failed + 1)); \
	  done; \
	done; \
	echo "$$failed failed"; test $$failed -eq 0

# the benchmarks, with the contracts off and the Boost headers if found;
# GMP=<prefix> and FFTW=<prefix> add GMP to bignum and FFTW to fft (also
# in single precision, if FFTW has libfftw3f), and GMP=<prefix> or
# ADS_WITH_GMP=1 bignum-gmp, BigInteger on GMP; FLINT=<prefix> adds FLINT
# to bignum (with GMP and MPFR in the same prefix); POCKETFFT=<dir> and
# DUCC0=<dir> add to fft pocketfft (the directory of pocketfft_hdronly.h)
# and ducc0 (the directory src of its sources);
# HASHING=<dir> adds to hashing the libraries whose sources are in <dir>
# (unordered_dense, robin-map, libcuckoo, xxHash, wyhash, rapidhash,
# fastfilter_cpp, xor_singleheader, pthash, BBHash, sux), ABSL=<prefix>
# Abseil; LIBSAIS=<prefix> and DIVSUFSORT=<prefix> add libsais and
# libdivsufsort to suffix, EDLIB=<prefix> and RAPIDFUZZ=<prefix> edlib and
# rapidfuzz-cpp to edit; PRIORITY=<dir> adds to priority the libraries
# whose sources are in <dir> (tlx, radix-heap); MIMALLOC=<prefix> and
# JEMALLOC=<prefix> add to memory mimalloc (built without the override of
# malloc) and jemalloc (built with the prefix je_), MIMALLOC mimalloc to
# allocators too; OPT=<prefix> adds to
# succinct and compression the libraries installed there: the headers of
# sdsl-lite, sux, pasta (with tlx) and succinct with libsuccinct, those of
# FiniteStateEntropy in fse/ with libfse, and streamvbyte, lz4 and libsais;
# SORTING=<dir> adds to sort the sorts whose sources are in <dir> (ips4o,
# ips2ra, x86-simd-sort, ska_sort, fluxsort, crumsort), HWY=<prefix> vqsort
# of Highway; TLX=<dir> adds tlx::multiway_merge to external from the
# sources of tlx in <dir>; COMBI=<dir> adds to combinatorial and backtrack
# the competitors whose sources are in <dir> (combinations, libpopcnt,
# libmorton, and the programs of Knuth built in knuth); ABSL=<prefix> and
# TLX=<dir> add Abseil and tlx to ordered, MINISELECT=<dir>, SDSL=<dir>
# and PGM=<dir> (their sources) miniselect, sdsl-lite and the PGM-index to
# arrays, LEMIRE=<prefix> and ROARING=<prefix> the SIMD intersections of
# Lemire et al. and CRoaring to sets, ABSL=<prefix>, HTRIE=<dir> (its
# source) and LIBART=<prefix> Abseil, tsl::htrie_map and libart to tries;
# SDSL adds the range minima of sdsl-lite to lce, HYPERSCAN=<prefix>
# Vectorscan (or Hyperscan) to multi; SDSL=<dir> (or OPT=<prefix>) and
# PGM=<dir> add the sd_vector of sdsl-lite and the PGM-indices of
# Ferragina and Vinciguerra to learned, HTRIE=<dir> tsl::htrie_map to
# stringdicts
BENCHES   = sort dictionaries priority sequences memory bignum fft hashing \
            search suffix edit numeric queues concurrent geometry matrix \
            sparse succinct compression external graphs combinatorial \
            backtrack ordered arrays sets tries allocators positions \
            assembly fingerprints multi lce parallel joins dynamic greedy \
            learned stringdicts \
            $(if $(GMP)$(filter 1,$(ADS_WITH_GMP)),bignum-gmp)
# on x86 the branches are kept off the 32-byte boundaries, where the
# erratum JCC of Skylake slows a loop by the alignment alone (about 10%):
# the same for ADS and for the libraries it is compared with, so that a
# ratio does not depend on where the code falls
JCCFLAGS := $(shell echo 'int main(){}' | $(CXX) -x c++ -c -o /dev/null \
            -Wa,-mbranches-within-32B-boundaries - 2>/dev/null && \
            echo -Wa,-mbranches-within-32B-boundaries)
BENCHFLAGS = -std=c++20 -O3 -march=native -DNDEBUG -DADS_CONTRACTS=0 \
             -Iinclude $(JCCFLAGS)
GMPFLAGS  = $(if $(GMP),-DADS_HAS_GMP -I$(GMP)/include -L$(GMP)/lib \
            -Wl$(comma)-rpath$(comma)$(GMP)/lib -lgmpxx -lgmp)
FLINTFLAGS = $(if $(FLINT),-DADS_HAS_FLINT -I$(FLINT)/include \
             -L$(FLINT)/lib -Wl$(comma)-rpath$(comma)$(FLINT)/lib -lflint \
             -lmpfr -lgmp)
FFTWFLAGS = $(if $(FFTW),-DADS_HAS_FFTW -I$(FFTW)/include -L$(FFTW)/lib \
            -Wl$(comma)-rpath$(comma)$(FFTW)/lib -lfftw3 \
            $(if $(wildcard $(FFTW)/lib/libfftw3f.*),-DADS_HAS_FFTWF \
            -lfftw3f)) \
            $(if $(POCKETFFT),-DADS_HAS_POCKETFFT -isystem $(POCKETFFT)) \
            $(if $(DUCC0),-DADS_HAS_DUCC0 -isystem $(DUCC0)) -pthread
SAISFLAGS = $(if $(LIBSAIS),-DADS_HAS_LIBSAIS -I$(LIBSAIS)/include \
            -L$(LIBSAIS)/lib -lsais) \
            $(if $(DIVSUFSORT),-DADS_HAS_DIVSUFSORT -I$(DIVSUFSORT)/include \
            -L$(DIVSUFSORT)/lib -ldivsufsort)
LCEFLAGS  = $(if $(SDSL),-DADS_HAS_SDSL -isystem $(SDSL)/include)
MULTIFLAGS = $(if $(HYPERSCAN),-DADS_HAS_HYPERSCAN -I$(HYPERSCAN)/include \
             -L$(HYPERSCAN)/lib -lhs)
EDITFLAGS = $(if $(EDLIB),-DADS_HAS_EDLIB -I$(EDLIB)/include \
            -L$(EDLIB)/lib -ledlib) \
            $(if $(RAPIDFUZZ),-DADS_HAS_RAPIDFUZZ -I$(RAPIDFUZZ)/include)

HASHINC   = unordered_dense/include robin-map/include libcuckoo \
            xxHash wyhash rapidhash fastfilter_cpp/src \
            xor_singleheader/include pthash/include pthash/external/bits \
            pthash/external/bits/include \
            pthash/external/bits/external/essentials/include \
            pthash/external/mm_file/include BBHash sux
HASHFLAGS = $(if $(HASHING),$(addprefix -isystem $(HASHING)/,$(HASHINC))) \
            $(if $(ABSL),-DADS_HAS_ABSL -isystem $(ABSL)/include \
            $(shell PKG_CONFIG_PATH=$(ABSL)/lib/pkgconfig pkg-config \
             --libs absl_flat_hash_map absl_hash 2>/dev/null)) -pthread
# the libraries of the other benchmarks, each from its prefix or source
# directory if given: OPENBLAS=<prefix> for matrix, EIGEN=<include dir>
# for matrix and sparse, CGAL=<include dir> (with GMP) for geometry,
# HURCHALLA="<its include dirs>" and PCG=<include dir> for numeric,
# RIGTORP=, MOODYCAMEL= and FOLLY=<include dirs> for concurrent
OPENBLASFLAGS = $(if $(OPENBLAS),-DADS_HAS_OPENBLAS \
                -I$(OPENBLAS)/include -L$(OPENBLAS)/lib \
                -Wl$(comma)-rpath$(comma)$(OPENBLAS)/lib -lopenblas)
EIGENFLAGS = $(if $(EIGEN),-I$(EIGEN))
NUMERICFLAGS = $(if $(HURCHALLA),-DADS_HAS_HURCHALLA \
               $(addprefix -I,$(HURCHALLA))) \
               $(if $(PCG),-DADS_HAS_PCG -I$(PCG))
CONCURRENTFLAGS = $(if $(RIGTORP),-DADS_HAS_RIGTORP -I$(RIGTORP)) \
                  $(if $(MOODYCAMEL),-DADS_HAS_MOODYCAMEL -I$(MOODYCAMEL)) \
                  $(if $(FOLLY),-DADS_HAS_FOLLY -I$(FOLLY))
CGALFLAGS = $(if $(CGAL),-DADS_HAS_CGAL -I$(CGAL) $(if $(GMP),\
            -I$(GMP)/include -L$(GMP)/lib) -lgmp)

PRIOFLAGS = $(if $(PRIORITY),-isystem $(PRIORITY)/tlx \
            -isystem $(PRIORITY)/radix-heap $(PRIORITY)/tlx/tlx/die/core.cpp)

MEMFLAGS  = $(if $(MIMALLOC),-DADS_HAS_MIMALLOC \
            -I$(MIMALLOC)/include/mimalloc-2.1 \
            $(MIMALLOC)/lib/mimalloc-2.1/libmimalloc.a) \
            $(if $(JEMALLOC),-DADS_HAS_JEMALLOC -I$(JEMALLOC)/include \
            $(JEMALLOC)/lib/libjemalloc.a -ldl) -pthread
OPTFLAGS  = $(if $(OPT),-I$(OPT)/include -L$(OPT)/lib)
OPTLIBS   = $(foreach l,$(1),$(if $(wildcard $(OPT)/lib/lib$(l).a),-l$(l)))

SORTINC   = ips4o/include ips2ra/include x86-simd-sort/src ska_sort \
            fluxsort/src crumsort/src
SORTFLAGS = $(if $(SORTING),$(addprefix -isystem $(SORTING)/,$(SORTINC))) \
            $(if $(HWY),-DADS_HAS_VQSORT -isystem $(HWY)/include \
            -L$(HWY)/lib -lhwy_contrib -lhwy)

COMBIFLAGS = $(if $(COMBI),-isystem $(COMBI)/combinations \
             -isystem $(COMBI)/libpopcnt -isystem $(COMBI)/libmorton/include)
BACKFLAGS  = $(if $(COMBI),-DADS_KNUTH=\"$(COMBI)/knuth\")

ABSLFLAGS = $(if $(ABSL),-I$(ABSL)/include -L$(ABSL)/lib \
            -Wl$(comma)-rpath$(comma)$(ABSL)/lib -labsl_throw_delegate \
            -labsl_raw_logging_internal -labsl_log_severity -labsl_base \
            -labsl_spinlock_wait)
ARRSFLAGS = $(if $(MINISELECT),-I$(MINISELECT)/include) \
            $(if $(SDSL),-I$(SDSL)/include) $(if $(PGM),-I$(PGM)/include)
SETSFLAGS = $(if $(LEMIRE),-I$(LEMIRE)/include -L$(LEMIRE)/lib \
            -Wl$(comma)-rpath$(comma)$(LEMIRE)/lib \
            -lSIMDCompressionAndIntersection) \
            $(if $(ROARING),-I$(ROARING)/include -L$(ROARING)/lib \
            -Wl$(comma)-rpath$(comma)$(ROARING)/lib -lroaring)
TRIEFLAGS = $(ABSLFLAGS) $(if $(HTRIE),-I$(HTRIE)/include) \
            $(if $(LIBART),-I$(LIBART)/include -L$(LIBART)/lib \
            -Wl$(comma)-rpath$(comma)$(LIBART)/lib -lart)

bench: $(addprefix $(BUILD)/bench-,$(BENCHES))

$(BUILD)/bench-bignum: bench/bignum.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(GMPFLAGS) $(FLINTFLAGS)

$(BUILD)/bench-bignum-gmp: bench/bignum.cpp bench/Bench.h $(HEADERS) \
                          | $(BUILD)
	$(CXX) $(BENCHFLAGS) -DADS_WITH_GMP -o $@ $< $(GMPLINK)

$(BUILD)/bench-fft: bench/fft.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(FFTWFLAGS)

$(BUILD)/bench-hashing: bench/hashing.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(HASHFLAGS)
$(BUILD)/bench-search: bench/search.cpp bench/Bench.h bench/Texts.h \
                      $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $<

$(BUILD)/bench-suffix: bench/suffix.cpp bench/Bench.h bench/Texts.h \
                      $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(SAISFLAGS)

$(BUILD)/bench-multi: bench/multi.cpp bench/Bench.h bench/Texts.h \
                     $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(MULTIFLAGS)

$(BUILD)/bench-lce: bench/lce.cpp bench/Bench.h bench/Texts.h \
                   $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(LCEFLAGS)

$(BUILD)/bench-edit: bench/edit.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(EDITFLAGS)
$(BUILD)/bench-numeric: bench/numeric.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(NUMERICFLAGS) $(GMPFLAGS)

$(BUILD)/bench-concurrent: bench/concurrent.cpp bench/Bench.h $(HEADERS) \
                           | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(CONCURRENTFLAGS) -pthread

$(BUILD)/bench-geometry: bench/geometry.cpp bench/Bench.h $(HEADERS) \
                         | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(CGALFLAGS)

$(BUILD)/bench-matrix: bench/matrix.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(EIGENFLAGS) $(OPENBLASFLAGS)

$(BUILD)/bench-sparse: bench/sparse.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(EIGENFLAGS)

$(BUILD)/bench-priority: bench/priority.cpp bench/Bench.h $(HEADERS) | \
                         $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(PRIOFLAGS)

$(BUILD)/bench-memory: bench/memory.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(MEMFLAGS)

$(BUILD)/bench-allocators: bench/allocators.cpp bench/Bench.h $(HEADERS) \
                           | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(MEMFLAGS)

$(BUILD)/bench-assembly: bench/assembly.cpp bench/Bench.h $(HEADERS) \
                         | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(EIGENFLAGS)
$(BUILD)/bench-succinct: bench/succinct.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(OPTFLAGS) $(call OPTLIBS,succinct)

$(BUILD)/bench-compression: bench/compression.cpp bench/Bench.h $(HEADERS) \
                            | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(OPTFLAGS) \
	       $(if $(wildcard $(OPT)/src/r-index/internal),\
	       -I$(OPT)/src/r-index/internal) \
	       $(call OPTLIBS,fse streamvbyte lz4 sais FastPFOR divsufsort \
	       divsufsort64)

$(BUILD)/bench-sort: bench/sort.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(SORTFLAGS)

$(BUILD)/bench-external: bench/external.cpp bench/Bench.h $(HEADERS) | \
                         $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(if $(TLX),-DADS_HAS_TLX -isystem $(TLX))

$(BUILD)/bench-combinatorial: bench/combinatorial.cpp bench/Bench.h \
                             $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(COMBIFLAGS)

$(BUILD)/bench-backtrack: bench/backtrack.cpp bench/Bench.h $(HEADERS) \
                         | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BACKFLAGS)

$(BUILD)/bench-ordered: bench/ordered.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(ABSLFLAGS) $(if $(TLX),-I$(TLX))

$(BUILD)/bench-arrays: bench/arrays.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(ARRSFLAGS)

$(BUILD)/bench-sets: bench/sets.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(SETSFLAGS)

$(BUILD)/bench-tries: bench/tries.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(TRIEFLAGS)

$(BUILD)/bench-learned: bench/learned.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(if $(SDSL),-isystem $(SDSL)/include) \
	       $(if $(PGM),-isystem $(PGM)/include) $(OPTFLAGS)

$(BUILD)/bench-stringdicts: bench/stringdicts.cpp bench/Bench.h $(HEADERS) \
                           | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(if $(HTRIE),-I$(HTRIE)/include)

# the parallel benchmark: OpenMP always (the executor of OpenMP and the
# parallel mode of libstdc++), TBB=1 (or TBB=<prefix>) oneTBB as executor
# and competitor, FASTFLOW=<dir> FastFlow, PARLAY=<dir> ParlayLib (with
# its examples), SORTING=<dir> the parallel IPS4o and IPS2Ra, TLX=<dir>
# the parallel merge of tlx, GAPBS=<dir> the GAP benchmark suite, EIGEN=
# <dir> Eigen, OPENBLAS=<prefix> OpenBLAS, FFTW=<prefix> FFTW with its
# threads, HASHING=<dir> libcuckoo
TBBFLAGS  = $(if $(TBB),-DADS_HAS_TBB -DADS_WITH_TBB \
            $(if $(filter-out 1,$(TBB)),-I$(TBB)/include -L$(TBB)/lib \
            -Wl$(comma)-rpath$(comma)$(TBB)/lib) -ltbb)
PARFLAGS  = -fopenmp -pthread $(TBBFLAGS) \
            $(if $(FASTFLOW),-DADS_WITH_FASTFLOW -isystem $(FASTFLOW)) \
            $(if $(PARLAY),-isystem $(PARLAY)/include -isystem $(PARLAY)) \
            $(if $(SORTING),-isystem $(SORTING)/ips4o/include \
            -isystem $(SORTING)/ips2ra/include -mcx16 -latomic) \
            $(if $(TLX),-isystem $(TLX) \
            $(TLX)/tlx/algorithm/parallel_multiway_merge.cpp) \
            $(if $(GAPBS),-isystem $(GAPBS)/src) \
            $(if $(EIGEN),-isystem $(EIGEN)) \
            $(if $(OPENBLAS),-DADS_HAS_OPENBLAS -I$(OPENBLAS)/include \
            -L$(OPENBLAS)/lib -Wl$(comma)-rpath$(comma)$(OPENBLAS)/lib \
            -lopenblas) \
            $(if $(FFTW),-DADS_HAS_FFTW -I$(FFTW)/include -L$(FFTW)/lib \
            -Wl$(comma)-rpath$(comma)$(FFTW)/lib -lfftw3_threads -lfftw3) \
            $(if $(HASHING),-isystem $(HASHING)/libcuckoo)

$(BUILD)/bench-parallel: bench/parallel.cpp bench/Bench.h $(HEADERS) \
                        | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(PARFLAGS)

# the bulk operations of the trees based on join: PAM=<dir> (its sources)
# with PARLAY=<dir> (those of ParlayLib) adds pam_map
JOINFLAGS = $(if $(PAM),-DADS_HAS_PAM -isystem $(PAM)/include \
            -isystem $(PARLAY)/include) -pthread
$(BUILD)/bench-joins: bench/joins.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(JOINFLAGS)

# the parallel greedy algorithms: PARLAY=<dir> the examples of ParlayLib
$(BUILD)/bench-greedy: bench/greedy.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< \
	       $(if $(PARLAY),-isystem $(PARLAY)/include -isystem $(PARLAY)) \
	       -pthread

# the sort over the processes of MPI, by mpicxx (not among BENCHES): make
# $(BUILD)/bench-mpi, then mpirun -np P $(BUILD)/bench-mpi
$(BUILD)/bench-mpi: bench/mpi.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(MPICXX) $(BENCHFLAGS) -DADS_WITH_MPI -o $@ $< -pthread

$(BUILD)/bench-%: bench/%.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $<

install:
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp -R include/ADS $(DESTDIR)$(PREFIX)/include/

clean:
	rm -rf $(BUILD)

.PHONY: all test test-mpi bench install clean

###############################################################################
#                             End of makefile                                 #
###############################################################################
