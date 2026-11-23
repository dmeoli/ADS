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
#                      by mpirun (ADS_WITH_MPI=auto or 1)                     #
#     make test-boost-mpi                                                     #
#                      the same in a program of Boost.MPI                     #
#     make test ADS_WITH_GMP=0 ADS_WITH_BLAS=0 ADS_WITH_BOOST=0               #
#               ADS_WITH_HIGHWAY=0 ADS_WITH_LEMON=0                           #
#                      the same without the backends of GMP, of a CBLAS, of   #
#                      Boost, of Highway and of LEMON, on by default where    #
#                      they are found [GMP=<prefix>] [BLAS=<prefix>]          #
#                      [BOOST=<prefix>] [HWY=<prefix>] [LEMON=<prefix>]       #
#     make test ADSMOD=1                                                      #
#                      also the C++20 module ads, those of the sets and the   #
#                      tests that import them                                 #
#     make sets        writes the entry points of the sets of the headers     #
#                      [see cmake/ads_sets.txt], as each make does            #
#     make test-sets   compiles each alone, which make test also does         #
#     make backends    tells whether GMP, a CBLAS, Boost, Highway, LEMON, MPI #
#                      and the runtimes of a program were found               #
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
#           optional: $(ADS_TEST_CONTRACTS) = the level of the contracts of   #
#                                   the tests [see Contract.h], 3 (all of     #
#                                   them) if not given; below 3 the checks of #
#                                   the contracts are not run [see ADSTest.h] #
#           optional: the macros of the backends, ADS_WITH_GMP,               #
#                     ADS_WITH_BLAS, ADS_WITH_BOOST, ADS_WITH_HIGHWAY,        #
#                     ADS_WITH_LEMON,                                         #
#                     ADS_WITH_STDPAR, ADS_WITH_STDEXEC, ADS_WITH_TASKFLOW,   #
#                     ADS_WITH_CEREAL, ADS_WITH_CROARING, ADS_WITH_SDSL,      #
#                     ADS_WITH_NANOARROW, ADS_WITH_EIGEN, ADS_WITH_METIS,     #
#                     ADS_WITH_KAHIP, ADS_WITH_GRAPHBLAS and                  #
#                     ADS_WITH_NETWORKIT                                      #
#                     (auto by default, 0 or 1), ADS_WITH_MPI (0 by default,  #
#                     auto or 1), ADS_WITH_OPENMP=1, ADS_WITH_TBB=1,          #
#                     ADS_WITH_FASTFLOW=1, and their *_ROOT [see              #
#                     lib/makefile-c]; ADSMOD=1 [see lib/makefile-mod]        #
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
# all the contracts checked (or those of the level ADS_TEST_CONTRACTS), and
# the stack of Linux, 8 MB, for the recursive solutions of the exercises on
# chains of 10^4 nodes (2 MB by default with MinGW)

STACK    = $(eval STACK := $$(shell echo 'int main(){}' | $$(CXX) -x c++ - \
           -Wl$$(ADScomma)--stack$$(ADScomma)8388608 -o $$(ADSTRY) \
           >$$(ADSNULL) 2>&1 && \
           echo -Wl$$(ADScomma)--stack$$(ADScomma)8388608))$(STACK)
ADS_TEST_CONTRACTS ?= 3
INC      = $(ADSINC) $(ADSLEMONINC) -Itest \
           -DADS_CONTRACTS=$(ADS_TEST_CONTRACTS)
LIBS     = $(ADSLIB) $(STACK)
HEADERS  = $(ADSH) $(ADSLEMONH) test/ADSTest.h
PROGRAMS = $(shell awk '/^[a-z]/{print $$1}' test/tests.txt | sort -u)
BINS     = $(addprefix $(BUILD)/,$(addsuffix $(ADSEXE),$(PROGRAMS)))

# with the module, $(BUILD)/module, the test that imports it in place of the
# headers [see test/module.cpp], and $(BUILD)/module_<set>, those that
# import the modules of the sets, each linked only with its own, which
# exports the names of ads.core too [see test/module_sets.cpp]
.DEFAULT_GOAL := all
ifneq ($(ADSOBJ),)
MODBIN   = $(BUILD)/module$(ADSEXE) $(foreach s,core base advanced,\
           $(BUILD)/module_$(s)$(ADSEXE))
$(BUILD)/module$(ADSEXE): test/module.cpp $(ADSOBJ) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(ADSMODSW) -o $@ $< $(ADSOBJ) $(LIBS)
MODSET_core     = 1
MODSET_base     = 2
MODSET_advanced = 3
define MODSETrule
$(BUILD)/module_$(1)$(ADSEXE): test/module_sets.cpp $(ADSMODOBJ_$(1)) \
                               | $(BUILD)
	$$(CXX) $$(CXXFLAGS) $$(ADSMODSW) -DADS_MODULE_SET=$(MODSET_$(1)) \
	-o $$@ $$< $(ADSMODOBJ_$(1)) $$(LIBS)
endef
$(foreach s,core base advanced,$(eval $(call MODSETrule,$(s))))
endif

# the entry points of the sets [see cmake/ads_sets.txt], written again by
# each make where python3 is found [see cmake/ads_sets.py]; test-sets
# compiles each alone and checks the headers of ADS that it pulls
PYTHON  ?= python3
HASPY    = $(shell command -v $(PYTHON) 2>/dev/null)

sets:
	$(if $(HASPY),@$(PYTHON) cmake/ads_sets.py,@echo "$(PYTHON) not found: \
	the entry points of the sets are not written again")

test-sets:
	$(if $(HASPY),,$(error test-sets needs $(PYTHON)))
	@for s in core base advanced; do \
	  $(PYTHON) cmake/ads_sets.py --test $$s --cxx "$(CXX)" || exit 1; \
	done
	@$(PYTHON) cmake/ads_sets.py --check && echo "PASSED sets.check"

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

all: sets $(BINS) $(MODBIN)

$(BUILD)/%$(ADSEXE): test/%.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $< $(LIBS)

$(BUILD):
	mkdir -p $@

# the executors on the runtimes of a program found, under the parallel
# algorithms, in their own tests [see test/InteropExecutors.h] and in the
# examples of appendix C of the manual [see test/interop_examples.cpp], the
# last in C++23 where the compiler takes it, for std::flat_map
INTEROPBINS = $(addprefix $(BUILD)/,$(addsuffix $(ADSEXE),\
              engineered_parallel engineered_speculative interop_parallel \
              interop_examples))
$(INTEROPBINS): INC += $(ADSINTEROPINC)
$(INTEROPBINS): LIBS += $(ADSINTEROPLIB)
CXX23 = $(shell $(CXX) -std=c++23 -x c++ -fsyntax-only /dev/null \
        2>/dev/null && echo -std=c++23)
$(BUILD)/interop_examples$(ADSEXE): CXXFLAGS += $(CXX23)

# the archives of cereal under the persistence, where they are found
$(BUILD)/engineered_persistence$(ADSEXE): INC += $(ADSCEREALINC) \
	$(if $(ADSCEREALINC),$(ADSSDSLINC))

# the columns of Arrow, the bitmaps of CRoaring and the vectors of sdsl-lite,
# with the archives of cereal, where they are found [see
# test/interop_columnar.cpp], also in the examples of appendix C
COLUMNARBINS = $(addprefix $(BUILD)/,$(addsuffix $(ADSEXE),\
               interop_columnar interop_examples))
$(COLUMNARBINS): INC += $(ADSCEREALINC) $(ADSCOLUMNARINC)
$(COLUMNARBINS): LIBS += $(ADSCOLUMNARLIB)
# nanoarrow, where found, reads the columns that ADS exports and writes
# those that it imports
$(BUILD)/interop_columnar$(ADSEXE): INC += $(ADSNANOARROWINC)
$(BUILD)/interop_columnar$(ADSEXE): LIBS += $(ADSNANOARROWLIB)

# the libraries of the graphs and of the matrices, Eigen, METIS, KaHIP,
# GraphBLAS and NetworKit, where they are found, under their adapters [see
# test/interop_graphs.cpp] and in the examples of appendix C
GRAPHSBINS = $(addprefix $(BUILD)/,$(addsuffix $(ADSEXE),\
             interop_graphs interop_examples))
$(GRAPHSBINS): INC += $(ADSGRAPHSINC)
$(GRAPHSBINS): LIBS += $(ADSGRAPHSLIB)

# the configurations of the layouts, not in tests.txt [see ADS/Abi.h and
# test/abi_units.cpp]: one program from two units of test/abi_unit.cpp, the
# first without the vector matching and the compare-and-swap of 16 bytes,
# the second with the defaults of the target and -mcx16 where the compiler
# takes it
ABIMCX16 = $(shell $(CXX) -mcx16 -x c++ -fsyntax-only /dev/null \
           2>/dev/null && echo -mcx16)
test-abi: | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) -DADS_ABI_UNIT=A -DADS_FLAT_SIMD=0 \
	 -DADS_FLAT_CAS16=0 -c -o $(BUILD)/abi_unit_a.o test/abi_unit.cpp
	$(CXX) $(CXXFLAGS) $(INC) -DADS_ABI_UNIT=B $(ABIMCX16) \
	 -c -o $(BUILD)/abi_unit_b.o test/abi_unit.cpp
	$(CXX) $(CXXFLAGS) -o $(BUILD)/abi_units$(ADSEXE) test/abi_units.cpp \
	 $(BUILD)/abi_unit_a.o $(BUILD)/abi_unit_b.o $(LIBS)
	$(BUILD)/abi_units$(ADSEXE)

# the sort and the runtime over the processes of MPI, and the examples of
# MPI of appendix C of the manual [see test/interop_examples_mpi.cpp], not in
# tests.txt: make test-mpi [MPICXX=mpicxx] [MPIRUN=mpirun], on 1 to 4
# processes, with the options of MPI that MPICXX tells (Open MPI is told that
# the processes may outnumber the cores)
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
	         mpi-stop mpi-edges; do \
	  for np in 1 2 3 4; do \
	    $(MPIRUN) -np $$np $(BUILD)/engineered_mpi $$t || exit 1; \
	  done; \
	done
	$(CXX) $(CXXFLAGS) $(INC) -o $(BUILD)/interop_examples_mpi \
	 test/interop_examples_mpi.cpp $(LIBS)
	for np in 1 2 3 4; do \
	  for t in mpi-sort boost-mpi-sort; do \
	    $(MPIRUN) -np $$np $(BUILD)/interop_examples_mpi $$t || exit 1; \
	  done; \
	done

# the same in a program of Boost.MPI [see test/engineered_boost_mpi.cpp]:
# make test-boost-mpi, with MPI on and Boost.MPI and Boost.Serialization
# where $(MPICXX) finds them
test-boost-mpi: | $(BUILD)
	$(if $(filter 1,$(ADSMPION)),,$(error MPI was not found [see \
	 extlib/makefile-libMPI]))
	$(CXX) $(CXXFLAGS) $(INC) -o $(BUILD)/engineered_boost_mpi \
	 test/engineered_boost_mpi.cpp $(LIBS) -lboost_mpi -lboost_serialization
	for t in boost-mpi-sort boost-mpi-codec; do \
	  for np in 1 2 3 4; do \
	    $(MPIRUN) -np $$np $(BUILD)/engineered_boost_mpi $$t || exit 1; \
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

# the wrong attachments of an observer, a test of the compiler [see
# test/modification_attach.cpp]: each case must fail with the error of the
# subject
test-modification-attach: | $(BUILD)
	@for c in 1 2 3 4; do \
	  case $$c in \
	   1) e='has an on() for none of the events' ;; \
	   2) e='must be noexcept' ;; \
	   3) e='requires an event that the structure' ;; \
	   4) e='precedes one it depends on' ;; \
	  esac; \
	  if $(CXX) $(CXXFLAGS) $(INC) -DADS_ATTACH_CASE=$$c -fsyntax-only \
	      test/modification_attach.cpp > $(BUILD)/attach_$$c.log 2>&1 || \
	     ! grep -qF "$$e" $(BUILD)/attach_$$c.log; \
	  then echo "FAILED modification-attach.$$c"; \
	       cat $(BUILD)/attach_$$c.log; exit 1; fi; \
	done
	@echo "PASSED modification-attach"

test: $(addprefix test-,$(PROGRAMS)) $(MODBIN) test-fastflow-order \
      test-modification-attach test-abi $(if $(HASPY),test-sets)
	@failed=$$(cat $(addprefix $(BUILD)/,$(addsuffix .failed,$(PROGRAMS))) \
	| awk '{s += $$1} END {print s + 0}'); \
	$(foreach m,$(MODBIN),$(m) || failed=$$((failed + 1));) \
	echo "$$failed failed"; test $$failed -eq 0

# the backends found, as the tests are built [see lib/makefile-c] - - - - - -

backends:
	@echo "ADS: GMP $(if $(filter 1,$(ADSGMPON)),ON,OFF)," \
	      "CBLAS $(if $(filter 1,$(ADSBLASON)),ON,OFF)"
	@echo "ADS: Boost $(if $(filter 1,$(ADSBOOSTON)),ON,OFF)"
	@echo "ADS: hwloc $(if $(filter 1,$(ADSHWLOCON)),ON,OFF)"
	@echo "ADS: Highway $(if $(filter 1,$(ADSHWYON)),ON,OFF)"
	@echo "ADS: LEMON $(if $(filter 1,$(ADSLEMONON)),ON,OFF)"
	@echo "ADS: MPI $(if $(filter 1,$(ADSMPION)),ON,OFF)"
	@echo "ADS: StdPar $(if $(filter 1,$(ADSSTDPARON)),ON,OFF)," \
	      "stdexec $(if $(filter 1,$(ADSSTDEXECON)),ON,OFF)," \
	      "Taskflow $(if $(filter 1,$(ADSTASKFLOWON)),ON,OFF)," \
	      "cereal $(if $(filter 1,$(ADSCEREALON)),ON,OFF)," \
	      "CRoaring $(if $(filter 1,$(ADSCROARINGON)),ON,OFF)," \
	      "sdsl $(if $(filter 1,$(ADSSDSLON)),ON,OFF)," \
	      "nanoarrow $(if $(filter 1,$(ADSNANOARROWON)),ON,OFF)"
	@echo "ADS: Eigen $(if $(filter 1,$(ADSEIGENON)),ON,OFF)," \
	      "METIS $(if $(filter 1,$(ADSMETISON)),ON,OFF)," \
	      "KaHIP $(if $(filter 1,$(ADSKAHIPON)),ON,OFF)," \
	      "GraphBLAS $(if $(filter 1,$(ADSGRAPHBLASON)),ON,OFF)," \
	      "NetworKit $(if $(filter 1,$(ADSNETWORKITON)),ON,OFF)"

# bench target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

comma    := ,
# the backends as the tests have them [see lib/makefile-c]
GMPON     = $(if $(filter 1,$(ADSGMPON)),-DADS_WITH_GMP $(libGMPINC) \
            $(libGMPLIB))
BLASON    = $(if $(filter 1,$(ADSBLASON)),-DADS_WITH_BLAS $(libBLASINC) \
            $(libBLASLIB))
HWLOCON   = $(if $(filter 1,$(ADSHWLOCON)),-DADS_WITH_HWLOC $(libHwlocINC) \
            $(libHwlocLIB))

# the benchmarks, with the contracts off and the Boost headers if found;
# bignum, matrix and parallel on the backends found as the tests are, and
# with them on, bignum-own and matrix-own on the code of ADS alone;
# GMP=<prefix> adds GMP to bignum; FLINT=<prefix> adds FLINT to bignum
# (with GMP and MPFR in the same prefix);
# HASHING=<dir> adds to hashing the libraries whose sources are in <dir>
# (unordered_dense, robin-map, libcuckoo, xxHash, wyhash, rapidhash,
# fastfilter_cpp, xor_singleheader, pthash, BBHash, sux,
# datasketches-cpp, ConsensusRecSplit, cqf, vqf, Memento_Filter),
# ABSL=<prefix> Abseil; LIBSAIS=<prefix> and DIVSUFSORT=<prefix> add libsais
# and libdivsufsort to suffix, EDLIB=<prefix> and RAPIDFUZZ=<prefix> edlib and
# rapidfuzz-cpp to edit; PRIORITY=<dir> adds to priority the libraries
# whose sources are in <dir> (tlx, radix-heap); MIMALLOC=<prefix> and
# JEMALLOC=<prefix> add to memory mimalloc (built without the override of
# malloc) and jemalloc (built with the prefix je_), MIMALLOC mimalloc to
# allocators too; OPT=<prefix> adds to
# succinct and compression the libraries installed there: the headers of
# sdsl-lite, sux, pasta (with tlx) and succinct with libsuccinct, those of
# FiniteStateEntropy in fse/ with libfse, and streamvbyte, lz4, zstd,
# libsais, fsst (in fsst/), the ALP of its authors (in alp-cwida/, with the
# Chimp128 of competitors/) and the kernels of FastLanes (in
# fastlanes-cwida/);
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
# succinct) the path-decomposed tries of Grossi and Ottaviano; ODS=<dir>
# (the C++ sources of Open Data Structures) adds the sets of Morin to ssets;
# BLOSSOM5=<dir> (the sources of the Blossom V of Kolmogorov, under its
# licence for research) adds it to the perfect matchings of graphs
BENCHES   = sort dictionaries priority sequences memory bignum hashing \
            search suffix edit numeric queues concurrent geometry matrix \
            sparse succinct compression external graphs combinatorial \
            backtrack ordered arrays sets tries allocators positions \
            assembly fingerprints multi lce parallel joins dynamic greedy \
            learned stringdicts flows assignment runtime bestfirst \
            hashattack ssets \
            $(if $(GMPON),bignum-own) $(if $(BLASON),matrix-own)
# on x86 the branches are kept off the 32-byte boundaries, where the
# erratum JCC of Skylake slows a loop by the alignment alone (about 10%):
# the same for ADS and for the libraries it is compared with, so that a
# ratio does not depend on where the code falls
JCCFLAGS := $(shell echo 'int main(){}' | $(CXX) -x c++ -c -o /dev/null \
            -Wa,-mbranches-within-32B-boundaries - 2>/dev/null && \
            echo -Wa,-mbranches-within-32B-boundaries)
# the benchmarks are built as a program that uses ADS is, i.e., on Boost,
# Highway and hwloc where they are found, but for the code of ADS kept
# where a structure would delegate to Boost or to LEMON, since the
# benchmarks compare it with them (the same as the targets of CMake)
BOOSTON   = $(if $(filter 1,$(ADSBOOSTON)),-DADS_WITH_BOOST $(libBoostINC))
HWYON     = $(if $(filter 1,$(ADSHWYON)),-DADS_WITH_HIGHWAY $(libHighwayINC))
BENCHFLAGS = -std=c++20 -O3 -march=native -DNDEBUG -DADS_CONTRACTS=0 \
             -I$(ADSSDR)/include $(JCCFLAGS) $(BOOSTON) $(HWYON) \
             $(if $(filter 1,$(ADSHWLOCON)),-DADS_WITH_HWLOC $(libHwlocINC)) \
             -DADS_POINTERTABLE_BOOST=0 -DADS_FLATHASH_BOOST=0 \
             -DADS_SMALLVECTOR_BOOST=0 -DADS_RANDOM_BOOST=0 \
             -DADS_MATCHING_LEMON=0 -DADS_ASSIGNMENT_LEMON=0
# the libraries of Highway and hwloc, after the source of a benchmark
BENCHLIBS = $(if $(filter 1,$(ADSHWYON)),$(libHighwayLIB)) \
            $(if $(filter 1,$(ADSHWLOCON)),$(libHwlocLIB))
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
            pthash/external/mm_file/include BBHash sux \
            datasketches-cpp/common/include datasketches-cpp/hll/include \
            datasketches-cpp/kll/include datasketches-cpp/count/include \
            datasketches-cpp/theta/include \
            $(CONSENSUS)/include $(CONSENSUS)/extlib/util/include \
            $(CONSENSUS)/extlib/ips2ra/include $(CONSENSUS)/extlib/tlx \
            $(CONSENSUS)/extlib/fips/include \
            $(PASTA)/pasta-bit-vector/include \
            $(CONSENSUS)/extlib/pasta-utils/include
# ConsensusRecSplit, with the bit vectors of pasta in a submodule of a
# submodule and the utilities of pasta, which its build fetches, cloned
# next to its submodules
CONSENSUS = ConsensusRecSplit
PASTA     = $(CONSENSUS)/extlib/fips/extlib/util/extern
# 1 if a unit that includes the header $(1) compiles at the flags of the
# benchmarks with the options $(2): the competitors of AVX2 (fastfilter_cpp,
# x86-simd-sort) are taken where it does [see bench/hashing.cpp and sort.cpp];
# the # of the #include is the octal escape of printf, since make 4.3 and later
# keep the backslash of a \# in a function and 4.2 needs it; the standard
# headers come first, as in the benchmarks, since simd-block.h uses memset()
# and std::runtime_error without including them
COMPILES  = $(shell { printf '\043include <%s>\n' cstdint cstdlib cstring \
            stdexcept $(1) ; \
            echo 'int main() { }' ; } | \
            $(CXX) $(BENCHFLAGS) $(2) -fsyntax-only -x c++ - >/dev/null \
            2>&1 && echo 1)
HASHFLAGS = $(if $(HASHING),$(addprefix -isystem $(HASHING)/,$(HASHINC)) \
            $(if $(call COMPILES,bloom/simd-block.h,\
            -isystem $(HASHING)/fastfilter_cpp/src),-DADS_HAS_FASTFILTER)) \
            $(if $(ABSL),-DADS_HAS_ABSL -isystem $(ABSL)/include \
            $(shell PKG_CONFIG_PATH=$(ABSL)/lib/pkgconfig pkg-config \
             --libs absl_flat_hash_map absl_hash 2>/dev/null)) -pthread
# the quotient filters of their authors from the sources in $(HASHING)
# (cqf, vqf and Memento_Filter), each in a unit of its own [see
# bench/QuotientCompetitors.h], taken where the unit compiles (once, at the
# first use)
QFCQF     = -I$(HASHING)/cqf/include -I$(HASHING)/cqf/src -D__SSE4_2_
QFVQF     = -I$(HASHING)/vqf/include -I$(HASHING)/vqf/src
QFMEM     = -I$(HASHING)/Memento_Filter/include \
            -I$(HASHING)/Memento_Filter/src
QFCC      = -O3 -march=native -DNDEBUG -Ibench -w
UNITOK    = $(shell $(1) $(QFCC) $(2) -fsyntax-only $(3) >/dev/null 2>&1 && \
            echo 1)
QFOBJS    = $(eval QFOBJS := $$(if $$(HASHING),\
            $$(if $$(call UNITOK,$$(CC),$$(QFCQF),bench/qf_cqf.c),\
            $$(BUILD)/qf_cqf.o) \
            $$(if $$(call UNITOK,$$(CXX) -std=c++20,$$(QFVQF),\
            bench/qf_vqf.cpp),$$(BUILD)/qf_vqf.o) \
            $$(if $$(call UNITOK,$$(CC),$$(QFMEM),bench/qf_memento.c),\
            $$(BUILD)/qf_memento.o)))$(QFOBJS)
QFFLAGS   = $(if $(filter %/qf_cqf.o,$(QFOBJS)),-DADS_HAS_CQF) \
            $(if $(filter %/qf_vqf.o,$(QFOBJS)),-DADS_HAS_VQF) \
            $(if $(filter %/qf_memento.o,$(QFOBJS)),-DADS_HAS_MEMENTO)
# the libraries of the other benchmarks, each from its prefix or source
# directory if given: OPENBLAS=<prefix> for matrix, EIGEN=<include dir>
# for matrix and sparse, CGAL=<include dir> (with GMP) for geometry,
# HURCHALLA="<its include dirs>" and PCG=<include dir> for numeric,
# RIGTORP=, MOODYCAMEL=, FOLLY= and XENIUM=<include dirs> for concurrent
OPENBLASFLAGS = $(if $(OPENBLAS),-DADS_HAS_OPENBLAS \
                -I$(OPENBLAS)/include -L$(OPENBLAS)/lib \
                -Wl$(comma)-rpath$(comma)$(OPENBLAS)/lib -lopenblas)
EIGENFLAGS = $(if $(EIGEN),-I$(EIGEN))
NUMERICFLAGS = $(if $(HURCHALLA),-DADS_HAS_HURCHALLA \
               $(addprefix -I,$(HURCHALLA))) \
               $(if $(PCG),-DADS_HAS_PCG -I$(PCG))
CONCURRENTFLAGS = $(if $(RIGTORP),-DADS_HAS_RIGTORP -I$(RIGTORP)) \
                  $(if $(MOODYCAMEL),-DADS_HAS_MOODYCAMEL -I$(MOODYCAMEL)) \
                  $(if $(FOLLY),-DADS_HAS_FOLLY -I$(FOLLY)) \
                  $(if $(XENIUM),-DADS_HAS_XENIUM -I$(XENIUM))
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
SORTFLAGS = $(if $(SORTING),$(addprefix -isystem $(SORTING)/,$(SORTINC)) \
            $(if $(call COMPILES,x86simdsort-static-incl.h,\
            -isystem $(SORTING)/x86-simd-sort/src),-DADS_HAS_XSS) \
            $(if $(wildcard $(SORTING)/fluxsort/src/fluxsort.h),\
            -fno-strict-aliasing)) \
            $(if $(HWY),-DADS_HAS_VQSORT -isystem $(HWY)/include \
            -L$(HWY)/lib -Wl$(comma)-rpath$(comma)$(HWY)/lib \
            -lhwy_contrib -lhwy)

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
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(GMPON) $(GMPFLAGS) $(FLINTFLAGS)

$(BUILD)/bench-bignum-own: bench/bignum.cpp bench/Bench.h $(HEADERS) \
                          | $(BUILD)
	$(CXX) $(BENCHFLAGS) -DADS_BENCH_OWN -o $@ $< $(BENCHLIBS)

$(BUILD)/bench-hashing: bench/hashing.cpp bench/Bench.h \
                       bench/QuotientCompetitors.h $(HEADERS) $(QFOBJS) \
                       | $(BUILD)
	$(CXX) $(BENCHFLAGS) $(QFFLAGS) -o $@ $< $(QFOBJS) $(BENCHLIBS) \
	       $(HASHFLAGS)
$(BUILD)/qf_cqf.o: bench/qf_cqf.c bench/QuotientCompetitors.h | $(BUILD)
	$(CC) $(QFCC) $(QFCQF) -c -o $@ $<
$(BUILD)/qf_vqf.o: bench/qf_vqf.cpp bench/QuotientCompetitors.h | $(BUILD)
	$(CXX) -std=c++20 $(QFCC) $(QFVQF) -c -o $@ $<
$(BUILD)/qf_memento.o: bench/qf_memento.c bench/QuotientCompetitors.h \
                       | $(BUILD)
	$(CC) $(QFCC) $(QFMEM) -c -o $@ $<
$(BUILD)/bench-search: bench/search.cpp bench/Bench.h bench/Texts.h \
                      $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS)

$(BUILD)/bench-suffix: bench/suffix.cpp bench/Bench.h bench/Texts.h \
                      $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(SAISFLAGS)

$(BUILD)/bench-multi: bench/multi.cpp bench/Bench.h bench/Texts.h \
                     $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(MULTIFLAGS)

$(BUILD)/bench-lce: bench/lce.cpp bench/Bench.h bench/Texts.h \
                   $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(LCEFLAGS)

$(BUILD)/bench-edit: bench/edit.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(EDITFLAGS)
$(BUILD)/bench-numeric: bench/numeric.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(NUMERICFLAGS) $(GMPFLAGS)

$(BUILD)/bench-concurrent: bench/concurrent.cpp bench/Bench.h $(HEADERS) \
                           | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(CONCURRENTFLAGS) $(HWLOCON) -pthread

$(BUILD)/bench-geometry: bench/geometry.cpp bench/Bench.h $(HEADERS) \
                         | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(CGALFLAGS)

$(BUILD)/bench-matrix: bench/matrix.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(BLASON) $(EIGENFLAGS) $(OPENBLASFLAGS)

$(BUILD)/bench-matrix-own: bench/matrix.cpp bench/Bench.h $(HEADERS) \
                          | $(BUILD)
	$(CXX) $(BENCHFLAGS) -DADS_BENCH_OWN -o $@ $< $(BENCHLIBS)

$(BUILD)/bench-sparse: bench/sparse.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(EIGENFLAGS)

$(BUILD)/bench-priority: bench/priority.cpp bench/Bench.h $(HEADERS) | \
                         $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(PRIOFLAGS)

$(BUILD)/bench-memory: bench/memory.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(MEMFLAGS)

$(BUILD)/bench-allocators: bench/allocators.cpp bench/Bench.h $(HEADERS) \
                           | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(MEMFLAGS)

$(BUILD)/bench-assembly: bench/assembly.cpp bench/Bench.h $(HEADERS) \
                         | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(EIGENFLAGS)
$(BUILD)/bench-succinct: bench/succinct.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(OPTFLAGS) $(call OPTLIBS,succinct)

$(BUILD)/bench-compression: bench/compression.cpp bench/Bench.h $(HEADERS) \
                            | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(OPTFLAGS) \
	       $(if $(wildcard $(OPT)/src/r-index/internal),\
	       -I$(OPT)/src/r-index/internal) \
	       $(if $(wildcard $(OPT)/include/alp-cwida/alp.hpp),\
	       -I$(OPT)/include/alp-cwida \
	       -I$(OPT)/include/alp-cwida/competitors) \
	       $(if $(wildcard $(OPT)/include/fastlanes-cwida),\
	       -I$(OPT)/include/fastlanes-cwida) \
	       $(call OPTLIBS,fse streamvbyte lz4 sais FastPFOR divsufsort \
	       divsufsort64 fsst fastlanes-cwida alp-cwida zstd) \
	       $(if $(wildcard $(OPT)/lib/libzstd.a),-DADS_BENCH_ZSTD)

$(BUILD)/bench-sort: bench/sort.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(SORTFLAGS)

$(BUILD)/bench-external: bench/external.cpp bench/Bench.h $(HEADERS) | \
                         $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(if $(TLX),-DADS_HAS_TLX -isystem $(TLX))

$(BUILD)/bench-combinatorial: bench/combinatorial.cpp bench/Bench.h \
                             $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(COMBIFLAGS)

$(BUILD)/bench-backtrack: bench/backtrack.cpp bench/Bench.h $(HEADERS) \
                         | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(BACKFLAGS)

$(BUILD)/bench-ordered: bench/ordered.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(ABSLFLAGS) $(if $(TLX),-I$(TLX))

$(BUILD)/bench-arrays: bench/arrays.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(ARRSFLAGS)

$(BUILD)/bench-sets: bench/sets.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(SETSFLAGS)

$(BUILD)/bench-tries: bench/tries.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(TRIEFLAGS)

$(BUILD)/bench-learned: bench/learned.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(if $(LAVECTOR),-isystem $(LAVECTOR)/include \
	       -isystem $(LAVECTOR)/lib/sdsl-lite/include) \
	       $(if $(SDSL),-isystem $(SDSL)/include) \
	       $(if $(PGM),-isystem $(PGM)/include) $(OPTFLAGS)

$(BUILD)/bench-stringdicts: bench/stringdicts.cpp bench/Bench.h $(HEADERS) \
                           | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(if $(HTRIE),-I$(HTRIE)/include) \
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
# BTREEOLC=<dir> the B+-tree of Leis et al. (BTreeOLC.h of index-microbench),
# SDSL=<dir> (its sources) the builds of sdsl-lite, SPRAYLIST=<lib> the
# SprayList built with the wrapper of the k-LSM (with KLSM=<dir>, and
# GSL=<prefix> of its GSL), LIBSAIS_OMP=<prefix> libsais built with OpenMP
TBBFLAGS  = $(if $(TBB),-DADS_HAS_TBB -DADS_WITH_TBB \
            $(if $(filter-out 1,$(TBB)),-I$(TBB)/include -L$(TBB)/lib \
            -Wl$(comma)-rpath$(comma)$(TBB)/lib) -ltbb)
PARFLAGS  = -fopenmp -pthread $(TBBFLAGS) \
            $(if $(FASTFLOW),-DADS_WITH_FASTFLOW -isystem $(FASTFLOW)) \
            $(if $(PARLAY),-isystem $(PARLAY)/include -isystem $(PARLAY)) \
            $(if $(SORTING),-isystem $(SORTING)/ips4o/include \
            -isystem $(SORTING)/ips2ra/include $(ABIMCX16) -latomic) \
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
            $(if $(BTREEOLC),-DADS_HAS_BTREEOLC -isystem $(BTREEOLC)) \
            $(if $(SDSL),-DADS_HAS_SDSL -isystem $(SDSL)/include) \
            $(if $(SPRAYLIST),-DADS_HAS_SPRAYLIST \
            -isystem $(KLSM)/src/bench/pqs $(SPRAYLIST) \
            $(if $(GSL),-L$(GSL)/lib) -lgsl -lgslcblas) \
            $(if $(LIBSAIS_OMP),-DADS_HAS_LIBSAIS_OMP \
            -I$(LIBSAIS_OMP)/include $(LIBSAIS_OMP)/lib/libsais.a)

$(BUILD)/bench-parallel: bench/parallel.cpp bench/Bench.h $(HEADERS) \
                        | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(BLASON) $(PARFLAGS)

# the bulk operations of the trees based on join: PAM=<dir> (its sources)
# with PARLAY=<dir> (those of ParlayLib) adds pam_map
JOINFLAGS = $(if $(PAM),-DADS_HAS_PAM -isystem $(PAM)/include \
            -isystem $(PARLAY)/include) -pthread
$(BUILD)/bench-joins: bench/joins.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(JOINFLAGS)

# the runtime of the workers: OpenMP always, TBB=1 (or TBB=<prefix>)
# oneTBB, FASTFLOW=<dir> FastFlow and the mesh of its queues
$(BUILD)/bench-runtime: bench/runtime.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       -fopenmp -pthread $(TBBFLAGS) \
	       $(if $(FASTFLOW),-DADS_WITH_FASTFLOW -isystem $(FASTFLOW))

# the parallel greedy algorithms: PARLAY=<dir> the examples of ParlayLib
$(BUILD)/bench-greedy: bench/greedy.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(if $(PARLAY),-isystem $(PARLAY)/include -isystem $(PARLAY)) \
	       -pthread

# the maximum flows, the assignment and the matchings: LEMON=1 (or
# LEMON=<prefix>) the Preflow, the MinCostAssignment (abLEMON), the minimum
# cost flows and the matchings of LEMON, the first competitors; the
# push-relabel and the weighted matching of Boost when their headers are
# found
LEMONFLAGS = $(if $(LEMON),-DADS_HAS_LEMON $(if $(filter-out 1,$(LEMON)),\
             -I$(LEMON)/include -L$(LEMON)/lib) -llemon)
$(BUILD)/bench-flows: bench/flows.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(LEMONFLAGS)

$(BUILD)/bench-assignment: bench/assignment.cpp bench/Bench.h $(HEADERS) \
                          | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(LEMONFLAGS)

# the Blossom V of Kolmogorov from BLOSSOM5=<dir>, its sources compiled
# apart with their own flags and no warning, for the comparison only
BLOSSOM5SRC = misc PMduals PMexpand PMinit PMinterface PMmain PMrepair \
              PMshrink MinCost/MinCost
BLOSSOM5OBJ = $(if $(BLOSSOM5),$(BLOSSOM5SRC:%=$(BUILD)/blossom5/%.o))
$(BUILD)/blossom5/%.o: $(BLOSSOM5)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -O3 -DNDEBUG -w -c -o $@ $<

$(BUILD)/bench-graphs: bench/graphs.cpp bench/Bench.h $(HEADERS) \
                       $(BLOSSOM5OBJ) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) $(LEMONFLAGS) \
	       $(if $(BLOSSOM5),-DADS_HAS_BLOSSOM5 -I$(BLOSSOM5) $(BLOSSOM5OBJ))

# the sort and the mailbox over the processes of MPI, by mpicxx (not among
# BENCHES): make $(BUILD)/bench-mpi, then mpirun -np P $(BUILD)/bench-mpi
# [-m]
$(BUILD)/bench-mpi: bench/mpi.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(MPICXX) $(BENCHFLAGS) -DADS_WITH_MPI -o $@ $< $(BENCHLIBS) -pthread

$(BUILD)/bench-ssets: bench/ssets.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS) \
	       $(if $(ODS),-DADS_HAS_ODS -I$(ODS))

$(BUILD)/bench-%: bench/%.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(BENCHLIBS)

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

.PHONY: all test test-mpi test-boost-mpi test-abi bench install clean \
        backends sets test-sets

############################### End of makefile ###############################
