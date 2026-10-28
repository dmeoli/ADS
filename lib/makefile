###############################################################################
################################## makefile ###################################
###############################################################################
#                                                                             #
#   makefile of ADS                                                           #
#                                                                             #
#   ADS is made of headers only: the makefile gives them, and the -I          #
#   directive of the library, to some other "main" makefile that uses ADS,    #
#   setting the target and doing the finishing. It takes in input the -I      #
#   and -D directives of the external libraries of the backends turned on,    #
#   if any, and *adds* to them that of the library; the external libraries    #
#   and the corresponding -L< libdirs > are not dealt with here, but in       #
#   makefile-c. The only object is that of the C++20 module ads, when it is   #
#   asked for [see makefile-mod].                                             #
#                                                                             #
#   Input:  $(CXX)      = compiler command                                    #
#           $(CXXFLAGS) = compiler options                                    #
#           $(ADSSDR)   = the directory where the ADS source is               #
#           $(ADSODR)   = the directory where the objects have to be put      #
#           $(ADSINC)   = the -I and -D for the backends of ADS, if any:      #
#                         - GMP, for BigInteger                               #
#                         - OpenMP, oneTBB and FastFlow, for the executors    #
#                         - MPI, for the distributed sort                     #
#                                                                             #
#   Output: $(ADSOBJ)   = the object of the module ads, to be linked where    #
#                         it is imported (empty if it is not built)           #
#           $(ADSH)     = the .h and .tpp files of ADS                        #
#           $(ADSINC)   = all the -I$( include dirs ) of ADS, the one of      #
#                         ADS first                                           #
#                                                                             #
###############################################################################

# macros to be exported - - - - - - - - - - - - - - - - - - - - - - - - - - -

ADSH := $(shell find $(ADSSDR)/include/ADS -name '*.h' -o -name '*.tpp')

# the headers of ADS before those of the external libraries, which might
# hold another ADS (e.g., an installed one)
ADSINC := -I$(ADSSDR)/include $(ADSINC)

# the C++20 module ads [see makefile-mod]- - - - - - - - - - - - - - - - - -

include $(ADSSDR)/lib/makefile-mod

ADSOBJ = $(ADSMODOBJ)

############################### End of makefile ###############################
