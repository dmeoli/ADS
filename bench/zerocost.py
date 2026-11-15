#!/usr/bin/env python3
###############################################################################
#                               zerocost.py                                   #
#                                                                             #
# The check that a structure which tells its changes costs nothing when no    #
# one listens, by counts and by code, never by times: it compiles             #
# bench/zerocost.cpp and bench/zerocost_asm.cpp with each compiler (g++ and   #
# clang++ when found) at -O2, counts by callgrind the instructions of one     #
# change of each variant (those of n and 2 n changes, the difference over n), #
# twice for the plain variant (the two counts must be equal), disassembles    #
# the functions of zerocost_asm.cpp by objdump and compares each with its     #
# reference byte by byte, relocations included, and prints the table in       #
# Markdown. The counts do not depend on the load of the machine. With --base  #
# REV it also builds the two files on the headers of the revision REV of git  #
# (or of a directory holding its include/) and on the present ones, both with #
# ZC_BASE (so that the two translation units hold the same code), and         #
# compares the counts of the workloads of JoinTree with no observer and the   #
# code of every function of the tree, the names of the policy left out.       #
#                                                                             #
#   python3 bench/zerocost.py [ --cxx g++ clang++ ] [ -n 100000 ]             #
#                             [ --out build-zerocost ] [ --base REV ]         #
###############################################################################

import argparse
import os
import platform
import re
import shutil
import subprocess
import sys

ROOT = os.path.dirname( os.path.dirname( os.path.abspath( __file__ ) ) )
WORKLOADS = [ "array-weight" , "list-weight" , "list-edge" , "key" ,
              "tree-insert" , "tree-overwrite" , "tree-assign" ]
LEGACY = [ "plain" , "observable" , "observable-excl" ]
POLICIES = [ "none" , "static-excl" , "static-count" , "dynamic" ]
# the variants of the array of keys, which has no Observable
KEY = [ "plain" , "none" , "static-excl" , "static-count" , "dynamic" ]
# the variants of JoinTree, and its workloads of an older revision
TREE = [ "none" , "static-excl" , "static-count" , "dynamic" ]
TREE_BASE = [ "tree-insert" , "tree-overwrite" ]
# the operations of JoinTree in zerocost_asm.cpp, compared by policy
TREE_OPS = [ "insert" , "remove" , "lookup" , "lower_bound" , "clear" ,
             "range" , "union" , "multi_insert" , "multi_remove" , "assign" ]
TREE_TAGS = [ "static_excl" , "static_count" , "dynamic" ]
# the functions of zerocost_asm.cpp by workload: ( function , reference )
ASM = {
    "array-weight": [
        ( "zc_none_array_weight" , "zc_plain_array_weight" ) ,
        ( "zc_static_excl_array_weight_ref" , "zc_none_array_weight_ref" ) ,
        ( "zc_static_count_array_weight_ref" , "zc_none_array_weight_ref" ) ,
        ( "zc_dynamic_array_weight_ref" , "zc_none_array_weight_ref" ) ,
        ( "zc_observable_array_weight_ref" , "zc_none_array_weight_ref" ) ] ,
    "list-edge": [
        ( "zc_none_list_edge" , "zc_plain_list_edge" ) ,
        ( "zc_static_excl_list_edge_ref" , "zc_none_list_edge_ref" ) ,
        ( "zc_static_count_list_edge_ref" , "zc_none_list_edge_ref" ) ,
        ( "zc_dynamic_list_edge_ref" , "zc_none_list_edge_ref" ) ,
        ( "zc_observable_list_edge_ref" , "zc_none_list_edge_ref" ) ] ,
    "key": [
        ( "zc_none_key_ref" , "zc_plain_key_ref" ) ,
        ( "zc_static_excl_key_ref" , "zc_plain_key_ref" ) ,
        ( "zc_static_count_key_ref" , "zc_plain_key_ref" ) ,
        ( "zc_dynamic_key_ref" , "zc_plain_key_ref" ) ] }


def run( cmd , **kw ):
    """Runs cmd, stops on failure."""
    r = subprocess.run( cmd , capture_output = True , text = True , **kw )
    if r.returncode != 0:
        sys.exit( "failed: " + " ".join( cmd ) + "\n" + r.stdout + r.stderr )
    return( r )


def build( cxx , out , policies , include = None , suffix = "" ,
           extra = () ):
    """The program and the object file compiled by cxx, in out, on the
    headers of include (those of the tree by default)."""
    tag = os.path.basename( cxx ) + suffix
    flags = [ "-std=c++20" , "-O2" , "-DNDEBUG" , "-DADS_CONTRACTS=0" ] + \
            list( extra ) + \
            [ "-I" + ( include or os.path.join( ROOT , "include" ) ) ,
              "-I" + os.path.join( ROOT , "bench" ) ]
    exe = os.path.join( out , "zerocost-" + tag )
    obj = os.path.join( out , "zerocost_asm-" + tag + ".o" )
    run( [ cxx ] + flags + [ os.path.join( ROOT , "bench" , "zerocost.cpp" ) ,
                              "-o" , exe ] )
    # the stack protector, on by default in some distributions, is chosen
    # before the optimization from the locals of a function, and the
    # wrapper built in a function of zerocost_asm.cpp would get a canary
    # that a wrapper living outside the function does not
    run( [ cxx ] + flags + [ "-fno-stack-protector" , "-c" ,
                              os.path.join( ROOT , "bench" ,
                                            "zerocost_asm.cpp" ) ,
                              "-o" , obj ] )
    return( exe , obj )


def collected( exe , work , var , n ):
    """The instructions of a run, by callgrind."""
    r = subprocess.run( [ "valgrind" , "--tool=callgrind" ,
                          "--callgrind-out-file=/dev/null" , exe , work ,
                          var , str( n ) ] , capture_output = True ,
                        text = True )
    if r.returncode != 0:
        return( None )
    m = re.search( r"Collected\s*:\s*(\d+)" , r.stderr )
    return( int( m.group( 1 ) ) if m else None )


def per_change( exe , work , var , n ):
    """The instructions of one change: those of 2 n changes less those of
    n, over n."""
    a = collected( exe , work , var , n )
    b = collected( exe , work , var , 2 * n )
    if a is None or b is None:
        return( None )
    return( ( b - a ) / n )


def functions( obj , demangle = False ):
    """The code of each function of obj: its instructions as bytes and its
    relocations, without the addresses; with demangle, by the demangled
    names, normalized by plain()."""
    r = run( [ "objdump" , "-d" , "-r" , "-w" ] +
             ( [ "-C" ] if demangle else [] ) + [ obj ] )
    code , name = {} , None
    for line in r.stdout.splitlines():
        m = re.match( r"^[0-9a-f]+ <(.+)>:$" if demangle else
                      r"^[0-9a-f]+ <([^>]+)>:$" , line )
        if m:
            name = plain( m.group( 1 ) ) if demangle else m.group( 1 )
            code[ name ] = { "bytes": [] , "relocs": [] , "count": 0 }
            continue
        if name is None:
            continue
        m = re.match( r"^\s*[0-9a-f]+:\s+(R_\S+)\s+(.+)$" if demangle else
                      r"^\s*[0-9a-f]+:\s+(R_\S+)\s+(\S+)" , line )
        if m:
            code[ name ][ "relocs" ].append( ( m.group( 1 ) ,
                plain( m.group( 2 ).strip() ) if demangle else
                m.group( 2 ) ) )
            continue
        m = re.match( r"^\s*[0-9a-f]+:\t((?:[0-9a-f]{2} )+)\s*(.*)$" , line )
        if m:
            code[ name ][ "bytes" ].append( m.group( 1 ).strip() )
            if m.group( 2 ).strip():
                code[ name ][ "count" ] += 1
    return( code )


def plain( name ):
    """A demangled name with the subject policy of JoinTree left out, as in
    a revision with no policy, and with the closing brackets written alike
    by the two forms of the demangler."""
    name = re.sub( r", ADS::(NoObservers|Static<zc::\w+>|Dynamic<void>)" ,
                   "" , name )
    while "> >" in name:
        name = name.replace( "> >" , ">>" )
    return( name )


def compare( code , f , ref ):
    """The cell of the comparison of the function f with ref."""
    if f not in code or ref not in code:
        return( "-" )
    a , b = code[ f ] , code[ ref ]
    if a[ "bytes" ] == b[ "bytes" ] and a[ "relocs" ] == b[ "relocs" ]:
        return( "identical (%d)" % a[ "count" ] )
    return( "%d vs %d" % ( a[ "count" ] , b[ "count" ] ) )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument( "--cxx" , nargs = "+" ,
                     default = [ c for c in ( "g++" , "clang++" )
                                 if shutil.which( c ) ] )
    ap.add_argument( "-n" , type = int , default = 100000 )
    ap.add_argument( "--out" , default = os.path.join( ROOT ,
                                                       "build-zerocost" ) )
    ap.add_argument( "--base" , default = None )
    a = ap.parse_args()
    os.makedirs( a.out , exist_ok = True )
    policies = os.path.exists( os.path.join( ROOT , "include" , "ADS" ,
                                             "Modification.h" ) )
    built = { c: build( c , a.out , policies ) for c in a.cxx }
    tags = [ os.path.basename( c ) for c in a.cxx ]

    model = platform.machine()
    if os.path.exists( "/proc/cpuinfo" ):
        for line in open( "/proc/cpuinfo" ):
            if line.startswith( "model name" ):
                model = line.split( ":" , 1 )[ 1 ].strip()
                break
    print( "- machine: %s" % model )
    for c in a.cxx:
        print( "- %s: %s" % ( os.path.basename( c ) ,
                              run( [ c , "--version" ] ).stdout.splitlines()[
                                  0 ] ) )
    print( "- %s" % run( [ "valgrind" , "--version" ] ).stdout.strip() )
    print( "- instructions of one change, (I(2n) - I(n)) / n, n = %d" % a.n )
    print()

    # the instructions of one change
    print( "| workload | variant | " +
           " | ".join( "Ir/change " + t for t in tags ) + " |" )
    print( "|---|---|" + "---|" * len( tags ) )
    for work in WORKLOADS:
        if work == "key" or work.startswith( "tree-" ):
            if not policies:
                continue
            variants = KEY if work == "key" else TREE
        else:
            variants = LEGACY + ( POLICIES if policies else [] )
        for var in variants:
            cells = []
            for c in a.cxx:
                x = per_change( built[ c ][ 0 ] , work , var , a.n )
                cells.append( "-" if x is None else "%.2f" % x )
            print( "| %s | %s | %s |" % ( work , var , " | ".join( cells ) ) )
        # the A/A row: the plain variant counted twice, equal counts
        cells = []
        first = variants[ 0 ]
        for c in a.cxx:
            x = collected( built[ c ][ 0 ] , work , first , a.n )
            y = collected( built[ c ][ 0 ] , work , first , a.n )
            cells.append( "equal" if x == y else "%d vs %d" % ( x , y ) )
        print( "| %s | A/A (%s twice) | %s |" % ( work , first ,
                                                  " | ".join( cells ) ) )
    print()

    # the code of the functions against their references
    print( "| workload | function | reference | " +
           " | ".join( "code " + t for t in tags ) + " |" )
    print( "|---|---|---|" + "---|" * len( tags ) )
    codes = { c: functions( built[ c ][ 1 ] ) for c in a.cxx }
    for work , pairs in ASM.items():
        for f , ref in pairs:
            if not policies and f != "zc_observable_array_weight_ref" \
               and f != "zc_observable_list_edge_ref":
                continue
            cells = [ compare( codes[ c ] , f , ref ) for c in a.cxx ]
            print( "| %s | %s | %s | %s |" % ( work , f , ref ,
                                               " | ".join( cells ) ) )
    if policies:
        # JoinTree by policy, the names of the policies left out
        trees = { c: functions( built[ c ][ 1 ] , True ) for c in a.cxx }
        for t in TREE_TAGS:
            for op in TREE_OPS:
                f , ref = "zc_%s_tree_%s" % ( t , op ) , \
                          "zc_none_tree_%s" % op
                cells = [ compare( trees[ c ] , f , ref ) for c in a.cxx ]
                print( "| tree | %s | %s | %s |" % ( f , ref ,
                                                     " | ".join( cells ) ) )

    if a.base:
        against( a , tags )


def against( a , tags ):
    """The counts and the code of JoinTree with no observer on the headers
    of the revision a.base and on the present ones."""
    if os.path.isdir( os.path.join( a.base , "include" ) ):
        base = a.base                   # a copy of the tree of the revision
    else:
        base = os.path.join( a.out , "base" )
        shutil.rmtree( base , ignore_errors = True )
        os.makedirs( base )
        tar = subprocess.run( [ "git" , "-C" , ROOT , "archive" , a.base ,
                                "include" ] , capture_output = True )
        if tar.returncode != 0:
            sys.exit( "no revision " + a.base )
        subprocess.run( [ "tar" , "-x" , "-C" , base ] , input = tar.stdout ,
                        check = True )
    old = { c: build( c , a.out , True , os.path.join( base , "include" ) ,
                      "-base" , [ "-DZC_BASE" ] ) for c in a.cxx }
    new = { c: build( c , a.out , True , None , "-now" , [ "-DZC_BASE" ] )
            for c in a.cxx }
    print()
    print( "| workload | %s | " % a.base +
           " | ".join( "Ir/change %s, %s / now" % ( t , a.base )
                       for t in tags ) + " |" )
    print( "|---|---|" + "---|" * len( tags ) )
    for work in TREE_BASE:
        cells = []
        for c in a.cxx:
            x = per_change( old[ c ][ 0 ] , work , "none" , a.n )
            y = per_change( new[ c ][ 0 ] , work , "none" , a.n )
            cells.append( "-" if x is None or y is None else
                          "%.2f / %.2f" % ( x , y ) )
        print( "| %s | none | %s |" % ( work , " | ".join( cells ) ) )
    print()
    print( "| compiler | functions of the tree | identical | differing |" )
    print( "|---|---|---|---|" )
    for c in a.cxx:
        x = functions( old[ c ][ 1 ] , True )
        y = functions( new[ c ][ 1 ] , True )
        both = sorted( n for n in x if n in y and ( "JoinTree" in n or
                       n.startswith( "zc_none_tree_" ) ) )
        diff = [ "%s (%d now, %d before)" % ( n , y[ n ][ "count" ] ,
                                              x[ n ][ "count" ] )
                 for n in both if ( x[ n ][ "bytes" ] , x[ n ][ "relocs" ] )
                 != ( y[ n ][ "bytes" ] , y[ n ][ "relocs" ] ) ]
        print( "| %s | %d | %d | %s |" % ( os.path.basename( c ) ,
                                           len( both ) ,
                                           len( both ) - len( diff ) ,
                                           ", ".join( diff ) or "none" ) )


if __name__ == "__main__":
    main()
