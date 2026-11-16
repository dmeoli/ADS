#!/usr/bin/env python3
###############################################################################
#                               portable.py                                   #
#                                                                             #
# The operations whose code changed when the branches on the processor, the   #
# compiler and the system were replaced by portable code, before and after:   #
# it builds bench/portable.cpp on the headers of the revision REV of git (or  #
# of a directory holding its include/) as "old" and on the present ones as    #
# "new", with each compiler (g++ and clang++ when found), at -O2 with the     #
# flags of the old fast paths (-march=x86-64-v3 -mcx16 on x86-64, which give  #
# BMI2, AVX2 and the CAS of 16 bytes) and with Boost, and prints the          #
# instructions of one operation of each case by callgrind, (I(2n) - I(n)) /   #
# n, twice for the new build (the two counts must be equal), in Markdown.     #
# The counts do not depend on the load of the machine. With --time it runs    #
# the timings of the two builds instead, the new one twice (the second time   #
# as "A/A", the noise of the machine), in the lines of Bench.h: they are to   #
# be taken on an idle machine only.                                           #
#                                                                             #
#   python3 bench/portable.py [ --before REV ] [ --cxx g++ clang++ ]          #
#                             [ -n 20000 ] [ --out build-portable ]           #
#                             [ --flags ... ] [ --time ]                      #
###############################################################################

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname( os.path.dirname( os.path.abspath( __file__ ) ) )
CASES = [ "select" , "bselect" , "prefix" , "recsplit" , "hash" , "mulmod" ,
          "montgomery" , "bigmul" , "bigdiv" , "merge" , "cas16" , "notify" ,
          "wait" , "suffix" ]


def run( cmd , **kw ):
    """Runs cmd, stops on failure."""
    r = subprocess.run( cmd , capture_output = True , text = True , **kw )
    if r.returncode != 0:
        sys.exit( "failed: " + " ".join( cmd ) + "\n" + r.stdout + r.stderr )
    return( r )


def before( rev , out ):
    """The headers of the revision rev, extracted in out, or those of the
    tree in the directory rev, if it is one."""
    if os.path.isdir( os.path.join( rev , "include" ) ):
        return( os.path.join( os.path.abspath( rev ) , "include" ) )
    d = os.path.join( out , "before-" + re.sub( r"[^A-Za-z0-9]" , "_" , rev ) )
    if not os.path.isdir( d ):
        os.makedirs( d )
        p = subprocess.Popen( [ "git" , "-C" , ROOT , "archive" , rev ,
                                "include" ] , stdout = subprocess.PIPE )
        subprocess.run( [ "tar" , "-x" , "-C" , d ] , stdin = p.stdout ,
                        check = True )
        p.wait()
    return( os.path.join( d , "include" ) )


def build( cxx , out , include , tag , flags ):
    """The program compiled by cxx in out on the headers of include."""
    exe = os.path.join( out , "portable-%s-%s" % ( tag ,
                                                   os.path.basename( cxx ) ) )
    run( [ cxx , "-std=c++20" , "-O2" , "-DNDEBUG" , "-DADS_CONTRACTS=0" ,
           "-DADS_WITH_BOOST" , '-DPORTABLE_TAG="%s"' % tag ,
           "-I" + include , "-I" + os.path.join( ROOT , "bench" ) ] + flags +
         [ os.path.join( ROOT , "bench" , "portable.cpp" ) , "-o" , exe ,
           "-pthread" ] )
    return( exe )


def collected( exe , case , n ):
    """The instructions of a run, by callgrind."""
    r = subprocess.run( [ "valgrind" , "--tool=callgrind" ,
                          "--callgrind-out-file=/dev/null" , exe , case ,
                          str( n ) ] , capture_output = True , text = True )
    m = re.search( r"Collected\s*:\s*(\d+)" , r.stderr )
    return( int( m.group( 1 ) ) if ( r.returncode == 0 ) and m else None )


def per_op( exe , case , n ):
    """The instructions of one operation, (I(2 n) - I(n)) / n."""
    a , b = collected( exe , case , n ) , collected( exe , case , 2 * n )
    return( None if a is None or b is None else ( b - a ) / n )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument( "--before" , default = "origin/feature/manual" )
    ap.add_argument( "--cxx" , nargs = "+" ,
                     default = [ c for c in ( "g++" , "clang++" )
                                 if shutil.which( c ) ] )
    ap.add_argument( "-n" , type = int , default = 20000 )
    ap.add_argument( "--out" , default = os.path.join( ROOT ,
                                                       "build-portable" ) )
    ap.add_argument( "--flags" , nargs = "*" , default = None )
    ap.add_argument( "--time" , action = "store_true" )
    a = ap.parse_args()
    os.makedirs( a.out , exist_ok = True )
    flags = a.flags
    if flags is None:
        flags = [ "-march=x86-64-v3" , "-mcx16" ] \
            if platform.machine() in ( "x86_64" , "AMD64" ) else []
    old = before( a.before , a.out )
    new = os.path.join( ROOT , "include" )
    exes = { c: ( build( c , a.out , old , "old" , flags ) ,
                  build( c , a.out , new , "new" , flags ) ) for c in a.cxx }
    if a.time:
        for c in a.cxx:
            for exe in exes[ c ]:
                sys.stdout.write( run( [ exe , "time" ] ).stdout )
        return
    print( "- before: %s; flags: %s" % ( a.before , " ".join( flags ) ) )
    for c in a.cxx:
        print( "- %s: %s" % ( os.path.basename( c ) ,
                              run( [ c , "--version" ] ).stdout.splitlines()[
                                  0 ] ) )
    print( "- instructions of one operation, (I(2n) - I(n)) / n, n = %d" %
           a.n )
    print()
    print( "| case | " + " | ".join( "%s old | %s new | %s new A/A" %
                                     ( ( os.path.basename( c ) , ) * 3 )
                                     for c in a.cxx ) + " |" )
    print( "|---|" + "---:|" * ( 3 * len( a.cxx ) ) )
    for case in CASES:
        cells = []
        for c in a.cxx:
            o , w = exes[ c ]
            for exe in ( o , w , w ):
                v = per_op( exe , case , a.n )
                cells.append( "n/a" if v is None else "%.1f" % v )
        print( "| %s | %s |" % ( case , " | ".join( cells ) ) )


if __name__ == "__main__":
    main()
