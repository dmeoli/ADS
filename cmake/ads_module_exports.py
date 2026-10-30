#!/usr/bin/env python3
###############################################################################
#                          ads_module_exports.py                              #
#                                                                             #
# Writes src/ADS.cppm, the interface of the C++20 module ads: it reads the    #
# AST of all the headers of include/ADS (dumped by clang) and exports, with   #
# a using-declaration, every name that they declare at the scope of the       #
# namespace ADS or of one nested in it, but for the namespaces detail:        #
#                                                                             #
#   python3 cmake/ads_module_exports.py -- -fopenmp -DADS_WITH_TBB \          #
#           -DADS_WITH_FASTFLOW -I<fastflow> -DADS_WITH_MPI -I<mpi>           #
#                                                                             #
# The headers of the optional executors are compiled only with their          #
# library [see include/ADS/parallel and include/ADS/distributed]: their       #
# names are exported inside the #if of their header, and they are read        #
# only if the flags define the macro. To be run whenever a header gains or    #
# loses a name.                                                               #
###############################################################################

import argparse
import collections
import json
import os
import subprocess
import sys
import tempfile
import textwrap

# the kinds of declaration that a using-declaration can export; the others
# (e.g., the out-of-class definitions of members, the deduction guides) come
# with their class
EXPORTED = { "CXXRecordDecl", "ClassTemplateDecl", "ConceptDecl",
             "EnumDecl", "FunctionDecl", "FunctionTemplateDecl",
             "TypedefDecl", "TypeAliasDecl", "TypeAliasTemplateDecl",
             "VarDecl", "VarTemplateDecl" }

# the headers compiled only with an optional library, and their macro
OPTIONAL = { "ADS/parallel/OpenMPExecutor.h": "_OPENMP",
             "ADS/parallel/TBBExecutor.h": "ADS_WITH_TBB",
             "ADS/parallel/FastFlowExecutor.h": "ADS_WITH_FASTFLOW",
             "ADS/distributed/DistributedSort.h": "ADS_WITH_MPI",
             "ADS/distributed/MPIExecutor.h": "ADS_WITH_MPI",
             "ADS/distributed/MPIMailbox.h": "ADS_WITH_MPI" }


def dump_ast( clang , inc , headers , flags ):
    """Returns the JSON AST of a translation unit that includes the given
    headers of inc."""
    with tempfile.NamedTemporaryFile( "w" , suffix = ".cpp" ,
                                      delete = False ) as tu:
        for h in headers:
            tu.write( f'#include "{h}"\n' )
    try:
        out = subprocess.run( [ clang , "-std=c++20" , "-fsyntax-only" ,
                                "-I" + inc , *flags , "-Xclang" ,
                                "-ast-dump=json" , tu.name ] ,
                              capture_output = True )
    finally:
        os.unlink( tu.name )
    if out.returncode:
        errors = [ l for l in out.stderr.decode().splitlines()
                   if "error" in l ]
        sys.exit( "the headers do not compile together:\n  " +
                  "\n  ".join( errors[ :10 ] ) )
    return json.loads( out.stdout )


def collect( ast , inc ):
    """Returns { namespace : { name : header } } for the names declared at
    namespace scope in inc, each with the header that defines it, and the
    list of those that cannot be exported."""
    names = collections.defaultdict( dict )
    internal = []
    declared , defined = set() , set()
    # the dump writes the file of a location only when it changes, in the
    # order of the dump: the last one is followed through every node
    state = { "file" : "" }

    def bare( loc ):
        if "file" in loc:
            state[ "file" ] = loc[ "file" ]

    def at( loc ):
        """Follows a location, and returns the file of its expansion."""
        if "spellingLoc" in loc or "expansionLoc" in loc:
            bare( loc.get( "spellingLoc" , {} ) )
            bare( loc.get( "expansionLoc" , {} ) )
        else:
            bare( loc )
        return state[ "file" ]

    def follow( node ):
        """Follows the locations of a node and of all those inside it."""
        at( node.get( "loc" , {} ) )
        rng = node.get( "range" , {} )
        at( rng.get( "begin" , {} ) )
        at( rng.get( "end" , {} ) )
        for n in node.get( "inner" , [] ):
            follow( n )

    def walk( nodes , ns ):
        for node in nodes:
            kind = node.get( "kind" )
            path = at( node.get( "loc" , {} ) )
            rng = node.get( "range" , {} )
            at( rng.get( "begin" , {} ) )
            at( rng.get( "end" , {} ) )
            if kind == "NamespaceDecl":
                walk( node.get( "inner" , [] ) ,
                      ns + [ node.get( "name" , "" ) ] )
                continue
            if kind == "LinkageSpecDecl":
                walk( node.get( "inner" , [] ) , ns )
                continue
            for n in node.get( "inner" , [] ):
                follow( n )
            if ( not path.startswith( inc + os.sep ) ) or \
               ( kind not in EXPORTED ) or \
               node.get( "isImplicit" ) or ( not node.get( "name" ) ):
                continue
            # a definition of a name declared elsewhere (e.g., that of
            # detail::f, or of the class C::D) comes with its declaration
            if "parentDeclContextId" in node:
                continue
            if ( not ns ) or ( ns[ 0 ] != "ADS" ) or ( "" in ns ):
                if ns and ( "" in ns ):
                    internal.append( f"{'::'.join( ns )}::{node[ 'name' ]}"
                                     f" ({os.path.relpath( path , inc )})" )
                continue
            if "detail" in ns:
                continue
            header = os.path.relpath( path , inc )
            # the definitions in impl/ go under the header that includes them
            if "/impl/" in header:
                h = os.path.splitext( header.replace( "/impl/" , "/" ) )[ 0 ]
                if os.path.exists( os.path.join( inc , h + ".h" ) ):
                    header = h + ".h"
            key = "::".join( ns )
            # a function template keeps its storage class in its pattern,
            # and if that is a method it is the out-of-class definition of a
            # member, which comes with its class
            decl = node
            if kind == "FunctionTemplateDecl":
                decl = next( ( n for n in node.get( "inner" , [] )
                               if n.get( "kind" ) in
                               ( "FunctionDecl" , "CXXMethodDecl" ,
                                 "CXXConstructorDecl" ,
                                 "CXXConversionDecl" ,
                                 "CXXDestructorDecl" ) ) , {} )
                if decl.get( "kind" ) != "FunctionDecl":
                    continue
            if decl.get( "storageClass" ) == "static":
                internal.append( f"{key}::{node[ 'name' ]} ({header})" )
                continue
            # a name goes under the header that defines it, not under the
            # first that declares it in advance
            record = node
            if kind == "ClassTemplateDecl":
                record = next( ( n for n in node.get( "inner" , [] )
                                 if n.get( "kind" ) == "CXXRecordDecl" ) ,
                               {} )
            if kind in ( "CXXRecordDecl" , "ClassTemplateDecl" ):
                declared.add( ( key , node[ "name" ] ) )
            if record.get( "completeDefinition" ):
                defined.add( ( key , node[ "name" ] ) )
                names[ key ][ node[ "name" ] ] = header
            else:
                names[ key ].setdefault( node[ "name" ] , header )
            # the enumerators of an unscoped enum live in the namespace
            if ( kind == "EnumDecl" ) and not node.get( "scopedEnumTag" ):
                for e in node.get( "inner" , [] ):
                    if e.get( "kind" ) == "EnumConstantDecl":
                        names[ key ].setdefault( e[ "name" ] , header )

    walk( ast.get( "inner" , [] ) , [] )
    for key , name in declared - defined:
        names[ key ].pop( name , None )
    return names , internal


BANNER = "/*" + "-" * 74 + "*/"


def title( text ):
    text = f" {text} "
    left = ( 74 - len( text ) ) // 2
    return "/*" + "-" * left + text + "-" * ( 74 - left - len( text ) ) + "*/"


def para( text ):
    return [ " * " + l for l in textwrap.wrap( text , 75 ,
                                                 break_on_hyphens = False ) ]


def write_module( out_file , headers , names ):
    base = os.path.basename( out_file )
    out = [ BANNER , title( f"File {base}" ) , BANNER , "/** @file" ]
    out += para( "Interface of the C++20 module ads, which exports every "
                 "name that the headers of include/ADS declare in the "
                 "namespace ADS and in those nested in it, but for the "
                 "namespaces detail; a translation unit can then write" )
    out += [ " *" , " *     import ads;" , " *" ]
    out += para( "in place of the #include of the headers. The macros (e.g., "
                 "ADS_PRE of Contract.h) are not exported by a module, so a "
                 "file that uses them still includes the header that "
                 "defines them, and those that configure the headers (e.g., "
                 "ADS_CONTRACTS, ADS_WITH_GMP) take the value they have "
                 "where the module is compiled. The names of the executors "
                 "of OpenMP, oneTBB and FastFlow and of the sort on MPI are "
                 "exported only if the module is compiled with their "
                 "library, as their headers are." )
    out += [ " *" ]
    out += para( "This file is written by cmake/ads_module_exports.py, which "
                 "reads the names from the AST of the headers: it is not to "
                 "be edited by hand." )
    out += [ " *" , " * \\author Donato Meoli \\n" ,
             " *         Dipartimento di Informatica \\n" ,
             " *         Universita' di Pisa \\n" , " *" ,
             " * \\copyright &copy; by Donato Meoli" , " */" ,
             BANNER , title( "GLOBAL MODULE FRAGMENT" ) , BANNER , "" ,
             "module;" , "" ]
    for h in headers:
        if h in OPTIONAL:
            out += [ f"#if defined( {OPTIONAL[ h ]} )" , f' #include "{h}"' ,
                     "#endif" ]
        else:
            out.append( f'#include "{h}"' )
    out += [ "" , BANNER , title( "MODULE PURVIEW" ) , BANNER , "" ,
             "export module ads;" , "" ]
    for ns in sorted( names ):
        by_header = collections.defaultdict( list )
        for n , header in names[ ns ].items():
            by_header[ header ].append( n )
        parts = ns.split( "::" )
        out.append( "export namespace " + parts[ 0 ] + " {" )
        for depth , p in enumerate( parts[ 1: ] , 1 ):
            out.append( " " * depth + "namespace " + p + " {" )
        pad = " " * len( parts )
        for header in sorted( by_header ):
            out.append( "" )
            out.append( pad + "// " + header )
            if header in OPTIONAL:
                out.append( f"#if defined( {OPTIONAL[ header ]} )" )
            for n in sorted( by_header[ header ] ):
                out.append( f"{pad}using {ns}::{n};" )
            if header in OPTIONAL:
                out.append( "#endif" )
        for depth in range( len( parts ) - 1 , -1 , -1 ):
            out.append( " " * depth + "}" )
        out.append( "" )
    out += [ BANNER , title( f"End File {base}" ) , BANNER ]
    os.makedirs( os.path.dirname( out_file ) , exist_ok = True )
    with open( out_file , "w" ) as f:
        f.write( "\n".join( out ) + "\n" )


def main():
    ap = argparse.ArgumentParser(
        description = "Writes the interface of the C++20 module ads." )
    ap.add_argument( "--clang" , default = "clang++" )
    ap.add_argument( "flags" , nargs = "*" )
    args = ap.parse_args()

    root = os.path.dirname( os.path.dirname( os.path.abspath( __file__ ) ) )
    inc = os.path.join( root , "include" )
    headers = sorted( os.path.relpath( os.path.join( d , f ) , inc )
                      for d , _ , fs in os.walk( os.path.join( inc , "ADS" ) )
                      for f in fs if f.endswith( ".h" ) and
                      os.path.basename( d ) != "impl" )
    # an optional header whose macro the flags do not define is not there
    defined = { f[ 2: ].split( "=" )[ 0 ] for f in args.flags
                if f.startswith( "-D" ) }
    if "-fopenmp" in args.flags:
        defined.add( "_OPENMP" )
    missing = sorted( m for m in OPTIONAL.values() if m not in defined )
    if missing:
        sys.exit( "the names of the optional headers are read only with "
                  "their macros, and these are not defined: " +
                  ", ".join( missing ) )
    names , internal = collect( dump_ast( args.clang , inc , headers ,
                                          args.flags ) , inc )
    if internal:
        sys.exit( "names with internal linkage, which a module cannot "
                  "export:\n  " + "\n  ".join( sorted( set( internal ) ) ) )
    write_module( os.path.join( root , "src" , "ADS.cppm" ) , headers ,
                  names )


if __name__ == "__main__":
    main()

###############################################################################
#                       End File ads_module_exports.py                        #
###############################################################################
