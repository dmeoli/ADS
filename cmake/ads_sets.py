#!/usr/bin/env python3
###############################################################################
#                                ads_sets.py                                  #
#                                                                             #
# Reads cmake/ads_sets.txt, the set of each public header of include/ADS      #
# (core, base, advanced or interop), checks it against the headers and their  #
# includes, and writes from it the entry points ADS/core.h, ADS/base.h,       #
# ADS/advanced.h and ADS/ADS.h and the table of the sets in the README:       #
#                                                                             #
#   python3 cmake/ads_sets.py            writes the files that changed        #
#   python3 cmake/ads_sets.py --check    only tells which are out of date     #
#   python3 cmake/ads_sets.py --test base --cxx g++ -- <flags>                #
#                                        compiles ADS/base.h alone, and       #
#                                        checks the headers that it pulls     #
#                                                                             #
# A header with no set, a set given to a header that is not there, and an     #
# include from core to another set, from base to advanced, or from any other  #
# set to interop, that no line "uses" allows, are errors; so is, for --check, #
# a module of src/ that exports the names of a header of another set [see     #
# ads_module_exports.py, which imports this file]. The headers of interop     #
# are in no entry point and no module. CMake runs it at configure time and    #
# the makefile at each make, and the CI with --check.                         #
###############################################################################

import argparse
import collections
import os
import re
import subprocess
import sys
import tempfile
import textwrap

SETS = ( "core" , "base" , "advanced" , "interop" )

# the entry point of each set, and the modules [see ads_module_exports.py]
ENTRY = { "core" : "ADS/core.h" , "base" : "ADS/base.h" ,
          "advanced" : "ADS/advanced.h" }
ALL = "ADS/ADS.h"
MODULE = { "core" : ( "ads.core" , "src/ADSCore.cppm" ) ,
           "base" : ( "ads.base" , "src/ADSBase.cppm" ) ,
           "advanced" : ( "ads.advanced" , "src/ADSAdvanced.cppm" ) }

# the sets of the headers that an entry point includes
MEMBERS = { "core" : ( "core" , ) , "base" : ( "base" , ) ,
            "advanced" : ( "advanced" , ) }

# the sets whose names each module exports: ads.base and ads.advanced also
# those of the core, since they do not import ads.core [see
# ads_module_exports.py]
EXPORTS = { "core" : ( "core" , ) , "base" : ( "base" , "core" ) ,
            "advanced" : ( "advanced" , "core" ) }

ROOT = os.path.dirname( os.path.dirname( os.path.abspath( __file__ ) ) )
INC = os.path.join( ROOT , "include" )

README_BEGIN = "<!-- sets: written by cmake/ads_sets.py -->"
README_END = "<!-- end of sets -->"


class Sets:
    """The contents of ads_sets.txt: the set, the macro and the reason of
    each header, and the includes across the sets that it allows."""

    def __init__( self , path = os.path.join( ROOT , "cmake" ,
                                              "ads_sets.txt" ) ):
        self.set , self.macro , self.reason = {} , {} , {}
        self.uses = {}
        self.errors = []
        last = None
        with open( path , encoding = "utf-8" ) as f:
            for n , line in enumerate( f , 1 ):
                if ( not line.strip() ) or line.startswith( "#" ):
                    last = None
                    continue
                if line[ 0 ].isspace():
                    if last is None:
                        self.errors.append( f"ads_sets.txt:{n}: an indented "
                                            "line after no header" )
                    else:
                        last.append( line.strip() )
                    continue
                w = line.split()
                if ( len( w ) >= 3 ) and ( w[ 1 ] == "uses" ):
                    key = ( w[ 0 ] , w[ 2 ] )
                    if key in self.uses:
                        self.errors.append( f"ads_sets.txt:{n}: {w[ 0 ]} "
                                            f"uses {w[ 2 ]} twice" )
                    self.uses[ key ] = last = w[ 3: ]
                    continue
                if ( len( w ) < 2 ) or ( w[ 1 ] not in SETS ):
                    self.errors.append( f"ads_sets.txt:{n}: not a header "
                                        "and one of " + ", ".join( SETS ) )
                    last = None
                    continue
                if w[ 0 ] in self.set:
                    self.errors.append( f"ads_sets.txt:{n}: {w[ 0 ]} has "
                                        "two sets" )
                self.set[ w[ 0 ] ] = w[ 1 ]
                rest = w[ 2: ]
                if rest and rest[ 0 ].startswith( "?" ):
                    self.macro[ w[ 0 ] ] = rest.pop( 0 )[ 1: ]
                self.reason[ w[ 0 ] ] = last = rest
        self.reason = { h : " ".join( r ) for h , r in self.reason.items() }
        self.uses = { k : " ".join( r ) for k , r in self.uses.items() }

    def of( self , *sets ):
        """The headers of the given sets, sorted."""
        return sorted( h for h , s in self.set.items() if s in sets )


def public_headers():
    """The public headers of include/ADS, but for the entry points."""
    entries = set( ENTRY.values() ) | { ALL }
    return sorted( h for h in
                   ( os.path.relpath( os.path.join( d , f ) , INC )
                     for d , _ , fs in os.walk( os.path.join( INC , "ADS" ) )
                     for f in fs if f.endswith( ".h" ) and
                     os.path.basename( d ) != "impl" )
                   if h not in entries )


INCLUDE = re.compile( r'^\s*#\s*include\s+"(ADS/[^"]+)"' , re.M )


def includes():
    """{ file : the files of ADS that it includes } for every header and
    every file of impl/, by their path relative to include/."""
    out = {}
    for d , _ , fs in os.walk( os.path.join( INC , "ADS" ) ):
        for f in fs:
            p = os.path.join( d , f )
            with open( p , encoding = "utf-8" ) as src:
                out[ os.path.relpath( p , INC ) ] = \
                    INCLUDE.findall( src.read() )
    return out


def direct( graph , public ):
    """{ header : the public headers that it includes } (without the ADS/
    prefix), an include through the files of impl/ counting as one of the
    header."""
    out = {}
    for h in public:
        found , seen , todo = set() , { h } , [ h ]
        while todo:
            for g in graph.get( todo.pop() , [] ):
                if g in seen:
                    continue
                seen.add( g )
                if g in public:
                    found.add( g[ 4: ] )
                else:
                    todo.append( g )
        out[ h[ 4: ] ] = found
    return out


def crossing( src , dst ):
    """Whether an include from the set src to the set dst must be allowed
    by a line uses."""
    if src == "core":
        return dst != "core"
    if dst == "interop":
        return src != "interop"
    return ( src == "base" ) and ( dst == "advanced" )


def check( sets , deps ):
    """The errors of ads_sets.txt against the headers."""
    errors = list( sets.errors )
    headers = set( deps )
    for h in sorted( headers - set( sets.set ) ):
        errors.append( f"{h} has no set: add it to cmake/ads_sets.txt" )
    for h in sorted( set( sets.set ) - headers ):
        errors.append( f"{h} is in cmake/ads_sets.txt but not in "
                       "include/ADS" )
    for ( a , b ) in sorted( sets.uses ):
        if b not in deps.get( a , () ):
            errors.append( f"{a} uses {b} in cmake/ads_sets.txt, but it "
                           "does not include it" )
        elif not crossing( sets.set.get( a ) , sets.set.get( b ) ):
            errors.append( f"{a} uses {b} in cmake/ads_sets.txt, which "
                           "needs no line uses" )
    for a in sorted( deps ):
        for b in sorted( deps[ a ] ):
            if ( a in sets.set ) and ( b in sets.set ) and \
               crossing( sets.set[ a ] , sets.set[ b ] ) and \
               ( ( a , b ) not in sets.uses ):
                errors.append( f"{a} ({sets.set[ a ]}) includes {b} "
                               f"({sets.set[ b ]}): allow it with a line "
                               f"\"{a} uses {b}\" in cmake/ads_sets.txt, or "
                               "move one of the two" )
    for h , m in sorted( sets.macro.items() ):
        if not re.fullmatch( r"[A-Za-z_][A-Za-z0-9_]*" , m ):
            errors.append( f"{h}: ?{m} is not a macro" )
    return errors


def allowed( sets , deps , s ):
    """The headers that the entry point of the set s may pull: those of its
    sets, of core, and those that these reach by the includes that the
    rules allow (a header of advanced includes base freely)."""
    start = set( sets.of( *MEMBERS[ s ] ) ) | set( sets.of( "core" ) )
    found , todo = set( start ) , list( start )
    while todo:
        a = todo.pop()
        for b in deps.get( a , () ):
            if ( b not in found ) and \
               ( ( not crossing( sets.set[ a ] , sets.set[ b ] ) ) or
                 ( ( a , b ) in sets.uses ) ):
                found.add( b )
                todo.append( b )
    return found


# --------------------------------------------------------------------------- #
# the entry points

BANNER = "/*" + "-" * 74 + "*/"
DEFINITIONS = "/*" + "-" * 29 + " DEFINITIONS " + "-" * 32 + "*/"
INCLUDES = "/*" + "-" * 30 + " INCLUDES " + "-" * 34 + "*/"


def title( text ):
    text = f" {text} "
    left = ( 74 - len( text ) ) // 2
    return "/*" + "-" * left + text + "-" * ( 74 - left - len( text ) ) + "*/"


def wrap( text ):
    return [ " * " + l for l in textwrap.wrap( text , 75 ,
                                                 break_on_hyphens = False ) ]


ABOUT = {
    "core" : "The core of ADS, included by the two sets of the library [see "
             "`base.h` and `advanced.h`]: the infrastructure that their "
             "headers share, i.e., the contracts, the iterators, the "
             "primitives of the processor, the capabilities, the concept of "
             "the executors, the pool of the nodes and its huge pages, the "
             "random hashing, the checks of the sorts, the bits of the "
             "coders and the files of records. The module ads.core exports "
             "its names.",
    "base" : "The base set of ADS: the structures and the algorithms that "
             "follow a textbook or an academic source (Montresor, The Art of "
             "Computer Programming, Ferragina's book and notes, Morin's Open "
             "Data Structures), written to be read and checked against the "
             "text, even when they are fast, with the headers of `core.h`. "
             "A header of this set "
             "may use one of the advanced set inside (e.g., `SuffixArray.h` "
             "sorts the ranks by `RadixSort.h`), which this file then "
             "includes without the module ads.base exporting its names; "
             "the uses allowed are the lines \"uses\" of "
             "`cmake/ads_sets.txt`.",
    "advanced" : "The advanced set of ADS: the structures and the algorithms "
                 "written for speed beyond the textbooks, the concurrent and "
                 "the parallel ones, the executors, the distributed layer, "
                 "the typed modifications and the observers, with the "
                 "headers of `core.h`. A header of this set extends those of "
                 "the base set by "
                 "composition, and includes those it needs, without the "
                 "module ads.advanced exporting their names. The executors "
                 "of OpenMP, oneTBB and FastFlow and the headers of MPI are "
                 "included only with their macros, as in the module.",
    "all" : "All the headers of ADS, i.e., those of the two sets and of "
            "their core [see `core.h`, `base.h` and `advanced.h`], as the "
            "module ads exports all their names, but for those of the "
            "interoperation with other libraries (`ADS/interop/`), which a "
            "program includes one by one next to the headers of the other "
            "library.",
}


def entry( sets , s ):
    """The text of the entry point of the set s ("all" for ADS.h)."""
    path = ALL if s == "all" else ENTRY[ s ]
    base = os.path.basename( path )
    guard = "__ADS_" + os.path.splitext( base )[ 0 ]
    out = [ BANNER , title( f"File {base}" ) , BANNER , "/** @file" ]
    out += wrap( ABOUT[ s ] )
    out += [ " *" ]
    out += wrap( "This file is written by `cmake/ads_sets.py` from "
                 "`cmake/ads_sets.txt`: it is not to be edited by hand." )
    out += [ " *" , " * \\author Donato Meoli \\n" ,
             " *         Dipartimento di Informatica \\n" ,
             " *         Università di Pisa \\n" , " *" ,
             " * \\copyright &copy; by Donato Meoli" , " */" , BANNER ,
             DEFINITIONS , BANNER , "" , f"#ifndef {guard}" ,
             f" #define {guard}" ,
             "/* self-identification: #endif constant */" , "" , BANNER ,
             INCLUDES , BANNER , "" ]
    if s == "all":
        out += [ f'#include "{ENTRY[ x ]}"' for x in
                 ( "advanced" , "base" , "core" ) ]
    else:
        if s != "core":
            out += [ f'#include "{ENTRY[ "core" ]}"' , "" ]
        for h in sets.of( *MEMBERS[ s ] ):
            if h in sets.macro:
                out += [ f"#if defined( {sets.macro[ h ]} )" ,
                         f' #include "ADS/{h}"' , "#endif" ]
            else:
                out.append( f'#include "ADS/{h}"' )
    out += [ "" , BANNER , "" , f"#endif /* {base} included */" , "" ,
             BANNER , title( f"End File {base}" ) , BANNER ]
    return "\n".join( out ) + "\n"


def readme( sets , text ):
    """The README with the table of the sets between its markers."""
    rows = collections.defaultdict( lambda : collections.defaultdict( list ) )
    for h , s in sets.set.items():
        folder = os.path.dirname( h )
        rows[ folder ][ s ].append( os.path.splitext(
            os.path.basename( h ) )[ 0 ] )
    count = collections.Counter( sets.set.values() )
    cols = ( "base" , "advanced" , "core" , "interop" )
    told = [ f"{count[ s ]} {s}" for s in cols ]
    out = [ README_BEGIN , "" ,
            f"{len( sets.set )} headers: " + ", ".join( told[ :-1 ] ) +
            f" and {told[ -1 ]}." , "" ,
            "| Folder | " + " | ".join( s.capitalize() for s in cols ) +
            " |" , "|---" * ( len( cols ) + 1 ) + "|" ]
    for folder in sorted( rows ):
        cells = [ ", ".join( f"`{x}`" for x in sorted( rows[ folder ][ s ] ) )
                  for s in cols ]
        name = f"`{folder}/`" if folder else "`ADS/`"
        out.append( f"| {name} | " + " | ".join( cells ) + " |" )
    out += [ "" , README_END ]
    i , j = text.find( README_BEGIN ) , text.find( README_END )
    if ( i < 0 ) or ( j < i ):
        sys.exit( f"README.md has not the markers {README_BEGIN} and "
                  f"{README_END}" )
    return text[ :i ] + "\n".join( out ) + text[ j + len( README_END ): ]


def module_errors( sets ):
    """The headers whose names a module of src/ exports, if it is not the
    module of their set."""
    errors = []
    for s , ( name , path ) in MODULE.items():
        p = os.path.join( ROOT , path )
        if not os.path.exists( p ):
            errors.append( f"{path} is not there: run "
                           "cmake/ads_module_exports.py" )
            continue
        with open( p , encoding = "utf-8" ) as f:
            for h in re.findall( r"^// (\S+\.h)$" , f.read() , re.M ):
                if sets.set.get( h ) not in EXPORTS[ s ]:
                    errors.append( f"{path} exports the names of {h}, of "
                                   f"the set {sets.set.get( h )}: run "
                                   "cmake/ads_module_exports.py" )
    return errors


def outputs( sets ):
    """{ path : text } of the files that the sets write."""
    out = { os.path.join( INC , ENTRY[ s ] ) : entry( sets , s )
            for s in ENTRY }
    out[ os.path.join( INC , ALL ) ] = entry( sets , "all" )
    p = os.path.join( ROOT , "README.md" )
    with open( p , encoding = "utf-8" ) as f:
        out[ p ] = readme( sets , f.read() )
    return out


def test( sets , deps , s , cxx , flags ):
    """Compiles the entry point of the set s alone, and checks that the
    headers of ADS that it pulls are among those allowed."""
    with tempfile.TemporaryDirectory() as d:
        tu = os.path.join( d , "sets.cpp" )
        with open( tu , "w" ) as f:
            f.write( f'#include "{ENTRY[ s ]}"\nint main() {{ return 0; }}\n' )
        out = subprocess.run( [ cxx , "-std=c++20" , "-fsyntax-only" , "-H" ,
                                "-I" + INC , *flags , tu ] ,
                              capture_output = True , text = True )
    if out.returncode:
        print( out.stderr[ -4000: ] , file = sys.stderr )
        return [ f"{ENTRY[ s ]} does not compile alone" ]
    pulled = set()
    prefix = os.path.realpath( INC ) + os.sep
    for line in out.stderr.splitlines():
        m = re.match( r"^\.+ (.+)$" , line )
        if m:
            p = os.path.realpath( m.group( 1 ) )
            if p.startswith( prefix + "ADS" + os.sep ):
                h = p[ len( prefix ) + 4: ]
                if h in sets.set:
                    pulled.add( h )
    ok = allowed( sets , deps , s )
    errors = [ f"{ENTRY[ s ]} pulls {h} ({sets.set[ h ]}), which no line "
               "uses of cmake/ads_sets.txt allows" for h in
               sorted( pulled - ok ) ]
    if not errors:
        count = collections.Counter( sets.set[ h ] for h in pulled )
        print( f"PASSED sets.{s}: {ENTRY[ s ]} pulls " +
               ", ".join( f"{count[ x ]} {x}" for x in SETS if count[ x ] ) )
    return errors


def main():
    ap = argparse.ArgumentParser(
        description = "Writes the entry points of the sets of ADS." )
    ap.add_argument( "--check" , action = "store_true" ,
                     help = "write nothing, fail if a file is out of date" )
    ap.add_argument( "--test" , choices = sorted( ENTRY ) ,
                     help = "compile the entry point of a set alone" )
    ap.add_argument( "--cxx" , default = "c++" )
    ap.add_argument( "flags" , nargs = "*" )
    args = ap.parse_args()

    sets = Sets()
    public = public_headers()
    deps = direct( includes() , set( public ) )
    errors = check( sets , deps )
    if ( not errors ) and args.test:
        errors = test( sets , deps , args.test , args.cxx , args.flags )
    elif not errors:
        for path , text in outputs( sets ).items():
            old = None
            if os.path.exists( path ):
                with open( path , encoding = "utf-8" ) as f:
                    old = f.read()
            if old == text:
                continue
            rel = os.path.relpath( path , ROOT )
            if args.check:
                errors.append( f"{rel} is out of date: run "
                               "python3 cmake/ads_sets.py" )
            else:
                with open( path , "w" , encoding = "utf-8" ) as f:
                    f.write( text )
                print( f"ads_sets.py: written {rel}" )
    # a module out of date stops only the check, which the CI runs, since
    # writing it again needs clang [see ads_module_exports.py]
    if ( not errors ) and args.check:
        errors = module_errors( sets )
    elif ( not errors ) and ( not args.test ):
        for e in module_errors( sets ):
            print( f"ads_sets.py: warning: {e}" , file = sys.stderr )
    if errors:
        sys.exit( "ads_sets.py:\n  " + "\n  ".join( errors ) )


if __name__ == "__main__":
    main()

###############################################################################
#                            End File ads_sets.py                             #
###############################################################################
