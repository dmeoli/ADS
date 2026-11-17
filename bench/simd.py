#!/usr/bin/env python3
###############################################################################
#                                  simd.py                                    #
#                                                                             #
# The old against the new vector kernels [see bench/simd.cpp]: it builds      #
# bench/simd.cpp on the headers of the revision REV of git, or of a           #
# directory holding its include/ (the kernels on                              #
# the intrinsics, with -march=x86-64-v3 for the counts, which valgrind runs,  #
# and -march=native for the times) and on the present ones with Highway (no   #
# flag of the machine), and prints one row per kernel. --counts gives the     #
# instructions of one call by callgrind, those of 3 R calls less those of R   #
# over 2 R, which the load of the machine does not change, with the row A/A   #
# (the new build counted again) and the new build also at the flags of the    #
# old one (callgrind runs no AVX-512, so the old paths of AVX-512 are not     #
# counted); --times gives the                                                 #
# nanoseconds of one call, the median of 11 runs of each build taken in       #
# turn, with the row A/A (the new build against a second run of itself) for   #
# the noise, and refuses to run unless the machine is idle (the load of the   #
# last minute below 1 per 32 processors), as the rule of the measures asks.   #
#                                                                             #
#   python3 bench/simd.py --counts [ --before REV ] [ --hwy PREFIX ]          #
#   python3 bench/simd.py --times  [ --before REV ] [ --hwy PREFIX ]          #
###############################################################################

import argparse
import os
import re
import statistics
import subprocess
import sys

ROOT = os.path.dirname( os.path.dirname( os.path.abspath( __file__ ) ) )


def run( cmd , **kw ):
    return( subprocess.run( cmd , check = True , capture_output = True ,
                            text = True , **kw ) )


def build( include , out , flags , hwy ):
    cmd = [ os.environ.get( "CXX" , "g++" ) , "-std=c++20" , "-O2" ,
            "-DNDEBUG" , "-DADS_CONTRACTS=0" , "-I" + include ] + flags
    cmd += [ os.path.join( ROOT , "bench" , "simd.cpp" ) , "-o" , out ,
             "-pthread" ]
    if hwy:
        cmd[ 5:5 ] = [ "-DADS_WITH_HIGHWAY" , "-I" + hwy + "/include" ]
        cmd += [ "-L" + hwy + "/lib" , "-Wl,-rpath," + hwy + "/lib" ,
                 "-lhwy_contrib" , "-lhwy" ]
    run( cmd )


def instructions( prog , kernel , reps ):
    r = subprocess.run( [ "valgrind" , "--tool=callgrind" ,
                          "--callgrind-out-file=/dev/null" , prog , kernel ,
                          str( reps ) ] , capture_output = True , text = True )
    m = re.search( r"Collected : (\d+)" , r.stderr )
    if not m:
        sys.exit( "simd.py: no count for " + kernel + "\n" + r.stderr )
    return( int( m.group( 1 ) ) )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument( "--counts" , action = "store_true" )
    ap.add_argument( "--times" , action = "store_true" )
    ap.add_argument( "--before" , default = "origin/feature/manual" )
    ap.add_argument( "--hwy" , default = "" )
    ap.add_argument( "--out" , default = "build-simd" )
    ap.add_argument( "-r" , type = int , default = 4 )
    a = ap.parse_args()
    if a.times and os.getloadavg()[ 0 ] > os.cpu_count() / 32:
        sys.exit( "simd.py: the machine is not idle, no times" )
    os.makedirs( a.out , exist_ok = True )
    old = os.path.abspath( os.path.join( a.out , "before" ) )
    if os.path.isdir( os.path.join( a.before , "include" ) ):
        old = os.path.abspath( a.before )  # a tree of the old headers
    elif not os.path.isdir( old ):
        run( [ "git" , "-C" , ROOT , "worktree" , "add" , "--detach" , old ,
               a.before ] )
    march = "-march=native" if a.times else "-march=x86-64-v3"
    po , pn = os.path.join( a.out , "simd-old" ) , os.path.join( a.out ,
                                                                 "simd-new" )
    build( os.path.join( old , "include" ) , po , [ march ] , "" )
    build( os.path.join( ROOT , "include" ) , pn , [] , a.hwy )
    names = run( [ pn , "--list" ] ).stdout.split()
    if a.counts:
        # the new build also with the flags of the old one, whose inline
        # kernels then take the same set of instructions
        p3 = os.path.join( a.out , "simd-new-v3" )
        build( os.path.join( ROOT , "include" ) , p3 , [ march ] , a.hwy )
        print( "| kernel | old | new | new / old | new A/A | new " + march +
               " | / old |" )
        print( "|---|---:|---:|---:|---:|---:|---:|" )
        for k in names:
            c = []
            for p in ( po , pn , pn , p3 ):  # 3 R calls less R, the first
                c.append( max( 1 , instructions( p , k , 3 * a.r ) -  # warm
                               instructions( p , k , a.r ) ) / ( 2 * a.r ) )
            print( f"| {k} | {c[ 0 ]:.0f} | {c[ 1 ]:.0f} |"
                   f" {c[ 1 ] / c[ 0 ]:.2f} | {c[ 2 ]:.0f} |"
                   f" {c[ 3 ]:.0f} | {c[ 3 ] / c[ 0 ]:.2f} |" )
    if a.times:
        print( "| kernel | old ns | new ns | new / old | A/A |" )
        print( "|---|---:|---:|---:|---:|" )
        for k in names:
            t = { "o" : [] , "n" : [] , "m" : [] }
            for _ in range( 5 ):  # the builds in turn
                for key , p in ( ( "o" , po ) , ( "n" , pn ) , ( "m" , pn ) ):
                    out = run( [ p , "--time" , k ] ).stdout.split()
                    t[ key ].append( float( out[ 1 ] ) )
            o , n , m = ( statistics.median( t[ x ] ) for x in "onm" )
            print( f"| {k} | {o:.0f} | {n:.0f} | {n / o:.2f} |"
                   f" {m / n:.2f} |" )


if __name__ == "__main__":
    main()
