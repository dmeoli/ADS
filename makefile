###############################################################################
################################## makefile ###################################
###############################################################################
#                                                                             #
#   makefile of the tests and of the benchmarks of ADS                        #
#                                                                             #
#   The same as CMakeLists.txt without CMake: it builds one program per       #
#   chapter in $(BUILD), runs the tests of test/tests.txt and checks that the #
#   list and the programs agree, and installs the headers. ADS itself, its    #
#   headers and the external libraries of its backends, comes from            #
#   lib/makefile-c, which is what the makefile of another program using ADS   #
#   includes [see lib/makefile-c].                                            #
#                                                                             #
#     make             builds the tests                                       #
#     make test        runs them (make -j<n> test builds and runs n programs  #
#                      at a time)                                             #
#     make test-mpi    the sort and the runtime over the processes of MPI,    #
#                      by mpirun                                              #
#     make test ADS_WITH_GMP=0 ADS_WITH_BLAS=0                                #
#                      the same without the backends of GMP and of a CBLAS,   #
#                      on by default where they are found [GMP=<prefix>]      #
#                      [BLAS=<prefix>]                                        #
#     make test ADSMOD=1                                                      #
#                      also the C++20 module ads and the test that imports it #
#     make backends    tells whether GMP, a CBLAS and MPI were found          #
#     make bench       builds the benchmarks in $(BUILD)                      #
#     bench/run.sh     runs them and writes the tables of the README          #
#     make install     copies include/ADS into $(PREFIX)/include, with        #
#                      ADS/ADSConfig.h, the version                           #
#                                                                             #
#   Input:  optional: $(CXX)      = the compiler command (g++ if not given)   #
#           optional: $(CXXFLAGS) = the compiler options (-std=c++20 -O2      #
#                                   -Wall -Wextra if not given)               #
#           optional: $(BUILD)    = the directory of the programs (build-make #
#                                   if not given)                             #
#           optional: $(PREFIX)   = where make install copies the headers     #
#                                   (/usr/local if not given)                 #
#           optional: the macros of the backends, ADS_WITH_GMP,               #
#                     ADS_WITH_BLAS and ADS_WITH_MPI (auto by default, 0 or   #
#                     1),                                                     #
#                     ADS_WITH_OPENMP=1, ADS_WITH_TBB=1, ADS_WITH_FASTFLOW=1, #
#                     and their *_ROOT [see lib/makefile-c]; ADSMOD=1 [see    #
#                     lib/makefile-mod]                                       #
#                                                                             #
###############################################################################

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
BUILD    ?= build-make
PREFIX   ?= /usr/local

# define & include the necessary modules- - - - - - - - - - - - - - - - - - -
# each module outputs some macros to be used here:
# *OBJ is the final object(s) / library
# *LIB is the external libraries + -L< libdirs >
# *H   is the list of all include files
# *INC is the -I< include directories >

# ADS (complete with the external libraries of the backends turned on); the
# module, if built, with the contracts of the tests
ADSSDR = .
ADSODR = $(BUILD)
ADSMODFLAGS = -Itest -DADS_CONTRACTS=3
include $(ADSSDR)/lib/makefile-c

# the tests - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# all the contracts checked, and the stack of Linux, 8 MB, for the recursive
# solutions of the exercises on chains of 10^4 nodes (2 MB by default with
# MinGW)

INC      = $(ADSINC) -Itest -DADS_CONTRACTS=3
LIBS     = $(ADSLIB) $(if $(filter Windows,$(UNAME_S)),\
           -Wl$(ADScomma)--stack$(ADScomma)8388608)
HEADERS  = $(ADSH) test/ADSTest.h
PROGRAMS = $(shell awk '/^[a-z]/{print $$1}' test/tests.txt | sort -u)
BINS     = $(addprefix $(BUILD)/,$(addsuffix $(ADSEXE),$(PROGRAMS)))

# with the module, $(BUILD)/module, the test that imports it in place of the
# headers [see test/module.cpp]
.DEFAULT_GOAL := all
ifneq ($(ADSOBJ),)
MODBIN   = $(BUILD)/module$(ADSEXE)
$(MODBIN): test/module.cpp $(ADSOBJ) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(ADSMODSW) -o $@ $< $(ADSOBJ) $(LIBS)
endif

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

all: $(BINS) $(MODBIN)

$(BUILD)/%$(ADSEXE): test/%.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $< $(LIBS)

$(BUILD):
	mkdir -p $@

# the sort and the runtime over the processes of MPI, not in tests.txt:
# make test-mpi [MPICXX=mpicxx] [MPIRUN=mpirun], on 1 to 4 processes, with
# the options of MPI that MPICXX tells (Open MPI is told that the processes
# may outnumber the cores)
MPICXX   ?= mpicxx
MPIRUN   ?= mpirun $(if $(shell $(MPICXX) --showme:version 2>/dev/null),\
            --oversubscribe)
test-mpi: | $(BUILD)
	$(if $(filter 1,$(ADSMPION)),,$(error MPI was not found [see \
	 extlib/makefile-libMPI]))
	$(CXX) $(CXXFLAGS) $(INC) -o $(BUILD)/engineered_mpi \
	 test/engineered_mpi.cpp $(LIBS)
	for t in distributed-sort distributed-sort-counts mpi-executor \
	         mpi-mailbox mpi-termination mpi-min mpi-bytes mpi-codec \
	         mpi-stop; do \
	  for np in 1 2 3 4; do \
	    $(MPIRUN) -np $$np $(BUILD)/engineered_mpi $$t || exit 1; \
	  done; \
	done

# test target - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# each line of tests.txt is a test, and each program must have the tests of
# the list, no more and no less: test-<program> runs those of a program,
# writes its output in $(BUILD)/<program>.log, printed in one piece when it
# ends, and the number of its failed tests in $(BUILD)/<program>.failed,
# which test sums (the end of the lines written by a program of Windows is
# dropped before the list is compared)

test-%: $(BUILD)/%$(ADSEXE)
	@listed=$$(awk -v p=$* '$$1 == p {print $$2}' test/tests.txt | sort); \
	found=$$($< -l | tr -d '\r' | sort); \
	failed=0; \
	{ if [ "$$listed" != "$$found" ]; then \
	    echo "FAILED $*.registry"; failed=$$((failed + 1)); \
	  else echo "PASSED $*.registry"; fi; \
	  for t in $$listed; do \
	    $< $$t || failed=$$((failed + 1)); \
	  done; \
	  echo $$failed > $(BUILD)/$*.failed; \
	} > $(BUILD)/$*.log 2>&1; \
	cat $(BUILD)/$*.log

# the order of the includes of FastFlowExecutor.h, a test of the compiler
# [see test/fastflow_order.cpp]: a program that includes a header of
# FastFlow first must not compile, with the error of the header, and with
# FastFlow one that defines the macros of the header itself must
test-fastflow-order: | $(BUILD)
	@if $(CXX) $(CXXFLAGS) $(INC) -fsyntax-only test/fastflow_order.cpp \
	    > $(BUILD)/fastflow_order.log 2>&1 || ! grep -q \
	    'included after a header of FastFlow' $(BUILD)/fastflow_order.log; \
	then echo "FAILED fastflow-order"; cat $(BUILD)/fastflow_order.log; \
	     exit 1; fi
	$(if $(filter 1,$(ADS_WITH_FASTFLOW)),@$(CXX) $(CXXFLAGS) $(INC) \
	 -DADS_FASTFLOW_DEFINED_FIRST -fsyntax-only test/fastflow_order.cpp)
	@echo "PASSED fastflow-order"

test: $(addprefix test-,$(PROGRAMS)) $(MODBIN) test-fastflow-order
	@failed=$$(cat $(addprefix $(BUILD)/,$(addsuffix .failed,$(PROGRAMS))) \
	| awk '{s += $$1} END {print s + 0}'); \
	$(if $(MODBIN),$(MODBIN) || failed=$$((failed + 1));) \
	echo "$$failed failed"; test $$failed -eq 0

# the backends found, as the tests are built [see lib/makefile-c] - - - - - -

backends:
	@echo "ADS: GMP $(if $(filter 1,$(ADSGMPON)),ON,OFF)," \
	      "CBLAS $(if $(filter 1,$(ADSBLASON)),ON,OFF)"
	@echo "ADS: MPI $(if $(filter 1,$(ADSMPION)),ON,OFF)"

# bench target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

comma    := ,
# the backends as the tests have them [see lib/makefile-c]
GMPON     = $(if $(filter 1,$(ADSGMPON)),-DADS_WITH_GMP $(libGMPINC) \
            $(libGMPLIB))
BLASON    = $(if $(filter 1,$(ADSBLASON)),-DADS_WITH_BLAS $(libBLASINC) \
            $(libBLASLIB))

# the benchmarks, with the contracts off and the Boost headers if found;
# bignum, matrix and parallel on the backends found as the tests are, and
# with them on, bignum-own and matrix-own on the code of ADS alone;
# GMP=<prefix> adds GMP to bignum; FLINT=<prefix> adds FLINT to bignum
# (with GMP and MPFR in the same prefix);
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
# Ferragina and Vinciguerra to learned, LAVECTOR=<dir> (its source, with
# its sdsl-lite) the la_vector of Boffa et al.; HTRIE=<dir> adds
# tsl::htrie_map to stringdicts, MARISA=<prefix> marisa-trie, TX=<prefix>
# the tx-trie (tx/tx.hpp and libtx.a) and PDT=<dir> (its source, with its
# succinct) the path-decomposed tries of Grossi and Ottaviano
BENCHES   = sort dictionaries priority sequences memory bignum hashing \
            search suffix edit numeric queues concurrent geometry matrix \
            sparse succinct compression external graphs combinatorial \
            backtrack ordered arrays sets tries allocators positions \
            assembly fingerprints multi lce parallel joins dynamic greedy \
            learned stringdicts flows assignment runtime bestfirst \
            $(if $(GMPON),bignum-own) $(if $(BLASON),matrix-own)
# on x86 the branches are kept off the 32-byte boundaries, where the
# erratum JCC of Skylake slows a loop by the alignment alone (about 10%):
# the same for ADS and for the libraries it is compared with, so that a
# ratio does not depend on where the code falls
JCCFLAGS := $(shell echo 'int main(){}' | $(CXX) -x c++ -c -o /dev/null \
            -Wa,-mbranches-within-32B-boundaries - 2>/dev/null && \
            echo -Wa,-mbranches-within-32B-boundaries)
BENCHFLAGS = -std=c++20 -O3 -march=native -DNDEBUG -DADS_CONTRACTS=0 \
             -I$(ADSSDR)/include $(JCCFLAGS)
GMPFLAGS  = $(if $(GMP),-DADS_HAS_GMP -I$(GMP)/include -L$(GMP)/lib \
            -Wl$(comma)-rpath$(comma)$(GMP)/lib -lgmpxx -lgmp)
FLINTFLAGS = $(if $(FLINT),-DADS_HAS_FLINT -I$(FLINT)/include \
             -L$(FLINT)/lib -Wl$(comma)-rpath$(comma)$(FLINT)/lib -lflint \
             -lmpfr -lgmp)
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
	$(CXX) $(BENCHFLAGS) -o $@ $< $(GMPON) $(GMPFLAGS) $(FLINTFLAGS)

$(BUILD)/bench-bignum-own: bench/bignum.cpp bench/Bench.h $(HEADERS) \
                          | $(BUILD)
	$(CXX) $(BENCHFLAGS) -DADS_BENCH_OWN -o $@ $<

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
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BLASON) $(EIGENFLAGS) $(OPENBLASFLAGS)

$(BUILD)/bench-matrix-own: bench/matrix.cpp bench/Bench.h $(HEADERS) \
                          | $(BUILD)
	$(CXX) $(BENCHFLAGS) -DADS_BENCH_OWN -o $@ $<

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
	$(CXX) $(BENCHFLAGS) -o $@ $< \
	       $(if $(LAVECTOR),-isystem $(LAVECTOR)/include \
	       -isystem $(LAVECTOR)/lib/sdsl-lite/include) \
	       $(if $(SDSL),-isystem $(SDSL)/include) \
	       $(if $(PGM),-isystem $(PGM)/include) $(OPTFLAGS)

$(BUILD)/bench-stringdicts: bench/stringdicts.cpp bench/Bench.h $(HEADERS) \
                           | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(if $(HTRIE),-I$(HTRIE)/include) \
	       $(if $(MARISA),-isystem $(MARISA)/include -L$(MARISA)/lib \
	       -Wl$(comma)-rpath$(comma)$(MARISA)/lib -lmarisa) \
	       $(if $(TX),-isystem $(TX)/include $(TX)/lib/libtx.a) \
	       $(if $(PDT),-isystem $(PDT) $(PDT)/succinct/bp_vector.cpp \
	       $(PDT)/succinct/rs_bit_vector.cpp)

# the parallel benchmark: OpenMP always (the executor of OpenMP and the
# parallel mode of libstdc++), TBB=1 (or TBB=<prefix>) oneTBB as executor
# and competitor, FASTFLOW=<dir> FastFlow, PARLAY=<dir> ParlayLib (with
# its examples), SORTING=<dir> the parallel IPS4o and IPS2Ra, TLX=<dir>
# the parallel merge of tlx, GAPBS=<dir> the GAP benchmark suite, EIGEN=
# <dir> Eigen, OPENBLAS=<prefix> OpenBLAS, HASHING=<dir> libcuckoo,
# MULTIQUEUE=<dir> the MultiQueue of Williams et al., KLSM=<dir> the k-LSM
# (its source of the threads compiled in), GROWT=<dir> the tables of growt,
# SDSL=<dir> (its sources) the builds of sdsl-lite, SPRAYLIST=<lib> the
# SprayList built with the wrapper of the k-LSM (with KLSM=<dir>, and
# GSL=<prefix> of its GSL)
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
            $(if $(HASHING),-isystem $(HASHING)/libcuckoo) \
            $(if $(MULTIQUEUE),-isystem $(MULTIQUEUE)/include \
            -isystem $(MULTIQUEUE)/include/multiqueue/third_party/pcg) \
            $(if $(KLSM),-DADS_HAS_KLSM -isystem $(KLSM)/src \
            $(KLSM)/src/util/thread_local_ptr.cpp) \
            $(if $(GROWT),-isystem $(GROWT)) \
            $(if $(SDSL),-DADS_HAS_SDSL -isystem $(SDSL)/include) \
            $(if $(SPRAYLIST),-DADS_HAS_SPRAYLIST \
            -isystem $(KLSM)/src/bench/pqs $(SPRAYLIST) \
            $(if $(GSL),-L$(GSL)/lib) -lgsl -lgslcblas)

$(BUILD)/bench-parallel: bench/parallel.cpp bench/Bench.h $(HEADERS) \
                        | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BLASON) $(PARFLAGS)

# the bulk operations of the trees based on join: PAM=<dir> (its sources)
# with PARLAY=<dir> (those of ParlayLib) adds pam_map
JOINFLAGS = $(if $(PAM),-DADS_HAS_PAM -isystem $(PAM)/include \
            -isystem $(PARLAY)/include) -pthread
$(BUILD)/bench-joins: bench/joins.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(JOINFLAGS)

# the runtime of the workers: OpenMP always, TBB=1 (or TBB=<prefix>)
# oneTBB, FASTFLOW=<dir> FastFlow and the mesh of its queues
$(BUILD)/bench-runtime: bench/runtime.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< -fopenmp -pthread $(TBBFLAGS) \
	       $(if $(FASTFLOW),-DADS_WITH_FASTFLOW -isystem $(FASTFLOW))

# the parallel greedy algorithms: PARLAY=<dir> the examples of ParlayLib
$(BUILD)/bench-greedy: bench/greedy.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< \
	       $(if $(PARLAY),-isystem $(PARLAY)/include -isystem $(PARLAY)) \
	       -pthread

# the maximum flows and the assignment: LEMON=1 (or LEMON=<prefix>) the
# Preflow and the minimum cost flows of LEMON; the push-relabel and the
# weighted matching of Boost when their headers are found
LEMONFLAGS = $(if $(LEMON),-DADS_HAS_LEMON $(if $(filter-out 1,$(LEMON)),\
             -I$(LEMON)/include -L$(LEMON)/lib) -llemon)
$(BUILD)/bench-flows: bench/flows.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(LEMONFLAGS)

$(BUILD)/bench-assignment: bench/assignment.cpp bench/Bench.h $(HEADERS) \
                          | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(LEMONFLAGS)

# the sort and the mailbox over the processes of MPI, by mpicxx (not among
# BENCHES): make $(BUILD)/bench-mpi, then mpirun -np P $(BUILD)/bench-mpi
# [-m]
$(BUILD)/bench-mpi: bench/mpi.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(MPICXX) $(BENCHFLAGS) -DADS_WITH_MPI -o $@ $< -pthread

$(BUILD)/bench-%: bench/%.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $<

# install target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
# the headers, and ADS/ADSConfig.h from cmake/ADSConfig.h.in with the version
# of git as cmake/DeriveVersion.cmake derives it: the tag, if the directory
# is the top of a clone of ADS with the tags, else the one written by git
# archive in VERSION.txt, else the one of MODULE.bazel

install:
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp -R $(ADSSDR)/include/ADS $(DESTDIR)$(PREFIX)/include/
	@cd $(ADSSDR); v=; \
	if c=$$(git rev-parse --show-cdup 2>/dev/null) && [ -z "$$c" ]; then \
	  v=$$(git describe --tags --abbrev=0 2>/dev/null); fi; \
	echo "$$v" | grep -q '^v\{0,1\}[0-9]' || v=$$(cat VERSION.txt); \
	echo "$$v" | grep -q '^v\{0,1\}[0-9]' || v=$$(sed -n \
	  's/^ *version *= *"\([0-9][^"]*\)".*/\1/p' MODULE.bazel | head -n 1); \
	set -- $$(echo "$$v" | sed 's/^v//' | \
	  awk -F. '{print $$1 + 0, $$2 + 0, $$3 + 0}'); \
	echo "ADS $$1.$$2.$$3"; \
	sed -e "s/@PROJECT_VERSION@/$$1.$$2.$$3/" \
	    -e "s/@PROJECT_VERSION_MAJOR@/$$1/" \
	    -e "s/@PROJECT_VERSION_MINOR@/$$2/" \
	    -e "s/@PROJECT_VERSION_PATCH@/$$3/" cmake/ADSConfig.h.in \
	  > $(DESTDIR)$(PREFIX)/include/ADS/ADSConfig.h

# clean target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -rf $(BUILD)

.PHONY: all test test-mpi bench install clean backends

############################### End of makefile ###############################
