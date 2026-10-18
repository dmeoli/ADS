###############################################################################
#                                makefile                                     #
#                                                                             #
# The same as CMakeLists.txt without CMake: it builds one program per         #
# chapter in $(BUILD), runs the tests of test/tests.txt and checks that the   #
# list and the programs agree, and installs the headers.                      #
#                                                                             #
#   make             builds the tests                                         #
#   make test        runs them                                                #
#   make bench       builds the benchmarks in $(BUILD)                        #
#   bench/run.sh     runs them and writes the tables of the README            #
#   make install     copies include/ADS into $(PREFIX)/include                #
###############################################################################

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
BUILD    ?= build-make
PREFIX   ?= /usr/local

INC      = -Iinclude -Itest -DADS_CONTRACTS=3
HEADERS  = $(shell find include/ADS -name '*.h' -o -name '*.tpp') \
           test/ADSTest.h
PROGRAMS = $(shell awk '/^[a-z]/{print $$1}' test/tests.txt | sort -u)
BINS     = $(addprefix $(BUILD)/,$(PROGRAMS))

all: $(BINS)

$(BUILD)/%: test/%.cpp $(HEADERS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(INC) -o $@ $< -pthread

$(BUILD):
	mkdir -p $@

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
# GMP=<prefix> and FFTW=<prefix> add GMP to bignum and FFTW to fft
BENCHES   = sort dictionaries priority sequences bignum fft
comma    := ,
BENCHFLAGS = -std=c++20 -O3 -march=native -DNDEBUG -DADS_CONTRACTS=0 \
             -Iinclude
GMPFLAGS  = $(if $(GMP),-DADS_HAS_GMP -I$(GMP)/include -L$(GMP)/lib \
            -Wl$(comma)-rpath$(comma)$(GMP)/lib -lgmpxx -lgmp)
FFTWFLAGS = $(if $(FFTW),-DADS_HAS_FFTW -I$(FFTW)/include -L$(FFTW)/lib \
            -Wl$(comma)-rpath$(comma)$(FFTW)/lib -lfftw3)

bench: $(addprefix $(BUILD)/bench-,$(BENCHES))

$(BUILD)/bench-bignum: bench/bignum.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(GMPFLAGS)

$(BUILD)/bench-fft: bench/fft.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $< $(FFTWFLAGS)

$(BUILD)/bench-%: bench/%.cpp bench/Bench.h $(HEADERS) | $(BUILD)
	$(CXX) $(BENCHFLAGS) -o $@ $<

install:
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp -R include/ADS $(DESTDIR)$(PREFIX)/include/

clean:
	rm -rf $(BUILD)

.PHONY: all test bench install clean

###############################################################################
#                             End of makefile                                 #
###############################################################################
