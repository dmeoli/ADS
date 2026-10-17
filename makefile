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
#   make install     copies include/ADS and src/ADS into $(PREFIX)/include    #
###############################################################################

CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra
BUILD    ?= build-make
PREFIX   ?= /usr/local

INC      = -Iinclude -Isrc -Itest -DADS_CONTRACTS=3
HEADERS  = $(shell find include/ADS src/ADS -name '*.h' -o -name '*.tpp') \
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

# the benchmarks, with the contracts off and the Boost headers if found
bench: $(BUILD)/bench-sort

$(BUILD)/bench-sort: bench/sort.cpp $(HEADERS) | $(BUILD)
	$(CXX) -std=c++20 -O3 -DNDEBUG -DADS_CONTRACTS=0 -Iinclude -Isrc -o $@ $<

install:
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp -R include/ADS src/ADS $(DESTDIR)$(PREFIX)/include/

clean:
	rm -rf $(BUILD)

.PHONY: all test bench install clean

###############################################################################
#                             End of makefile                                 #
###############################################################################
