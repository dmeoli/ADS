#!/usr/bin/env python3
###############################################################################
#                          ads_module_exports.py                              #
#                                                                             #
# Writes the interfaces of the C++20 modules of ADS: ads.core, ads.base and   #
# ads.advanced (src/ADSCore.cppm, src/ADSBase.cppm, src/ADSAdvanced.cppm),    #
# one per set of headers [see cmake/ads_sets.txt], and ads (src/ADS.cppm),    #
# which exports the three. It reads the AST of all the headers of             #
# include/ADS (dumped by clang), and each module exports, with a              #
# using-declaration, every name that the headers of its set declare at the    #
# scope of the namespace ADS or of one nested in it, but for the namespaces   #
# detail (the names of a header of both from ads.base and ads.advanced):      #
#                                                                             #
#   python3 cmake/ads_module_exports.py -- -fopenmp -DADS_WITH_TBB \          #
#           -DADS_WITH_FASTFLOW -I<fastflow> -DADS_WITH_MPI -I<mpi> \         #
#           -DADS_WITH_BOOST                                                  #
#                                                                             #
# The headers of the optional executors and of the interoperation with Boost  #
# are compiled only with their library [see include/ADS/parallel,             #
# include/ADS/distributed and include/ADS/interop/boost]: their names are     #
# exported inside the #if of their macro, given by ads_sets.txt, and they are #
# read only if the flags define the macro. To be run whenever a header gains  #
# or loses a name or changes its set.                                         #
###############################################################################

import argparse
import collections
import json
import re
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

sys.path.insert( 0 , os.path.dirname( os.path.abspath( __file__ ) ) )
import ads_sets  # noqa: E402

SETS = ads_sets.Sets()
# the headers compiled only with an optional library, and their macro
OPTIONAL = { "ADS/" + h : m for h , m in SETS.macro.items() }


FILE = re.compile( r'"file": "((?:[^"\\]|\\.)*)"' )


def indent( line ):
    return len( line ) - len( line.lstrip( " " ) )


def last_file( line , prev , last ):
    """The file of a location on line, if it is one and not the file of an
    includedFrom (on the line prev), else last."""
    if '"file"' in line and ( "includedFrom" not in prev ):
        m = FILE.search( line )
        if m:
            return m.group( 1 )
    return last


def elements( it , depth , top ):
    """Yields the declarations of the array of the JSON AST whose items
    begin with a line "{" indented by depth, read from the lines it, until
    the line that closes the array. A namespace (at the top only ADS or an
    anonymous one) and a linkage specification come with their inner ones
    as a generator, read in turn; any other declaration comes without what
    is nested more than two levels in it (e.g., the bodies of the
    functions, the members of the classes), followed by the last file it
    names, as { "kind" : "File" , "file" : ... }, since the dump writes the
    file of a location only when it changes."""
    pad = " " * depth
    for line in it:
        if indent( line ) < depth:          # the end of the array
            return
        assert line == pad + "{\n"
        head , end , prev , last = [ line ] , None , line , None
        field = pad + '  "'
        for line in it:
            last = last_file( line , prev , last )
            prev = line
            if line in ( pad + "}\n" , pad + "},\n" ):
                end = line
                break
            head.append( line )
            if line.startswith( field + 'inner": [' ):
                break
        text = "".join( head )
        kind = re.search( r'"kind": "(\w+)"' , text ).group( 1 )
        name = re.search( r'\n' + pad + r'  "name": "([^"]*)"' , text )
        nest = ( kind == "LinkageSpecDecl" ) or \
               ( ( kind == "NamespaceDecl" ) and
                 ( ( not top ) or ( not name ) or
                   ( name.group( 1 ) == "ADS" ) ) )
        if nest and ( end is None ):
            node = json.loads( "".join( head[ :-1 ] ) +
                               pad + '  "inner": []\n' + pad + "}" )
            node[ "inner" ] = elements( it , depth + 4 , False )
            yield node
            for _ in node[ "inner" ]:       # what the walk left
                pass
            next( it )                      # the "}" of the declaration
            continue
        keep = nest or ( not top )
        while end is None:
            line = next( it )
            last = last_file( line , prev , last )
            prev = line
            if line in ( pad + "}\n" , pad + "},\n" ):
                end = line
            elif keep and ( indent( line ) <= depth + 8 ):
                head.append( line )
        if keep:
            text = "".join( head ) + pad + "}"
            yield json.loads( re.sub( r",(\s*[}\]])" , r"\1" , text ) )
        yield { "kind" : "File" ,
                "file" : json.loads( '"' + last + '"' ) if last else None }


def dump_ast( clang , inc , headers , flags ):
    """Yields the declarations at the scope of the translation unit that
    includes the given headers of inc [see elements()], read from the JSON
    AST that clang writes as it writes it, since the whole AST takes some
    gigabytes."""
    with tempfile.NamedTemporaryFile( "w" , suffix = ".cpp" ,
                                      delete = False ) as tu:
        for h in headers:
            tu.write( f'#include "{h}"\n' )
    err = tempfile.TemporaryFile()
    try:
        proc = subprocess.Popen( [ clang , "-std=c++20" , "-fsyntax-only" ,
                                   "-I" + inc , *flags , "-Xclang" ,
                                   "-ast-dump=json" , tu.name ] ,
                                 stdout = subprocess.PIPE , stderr = err ,
                                 text = True )
        it = iter( proc.stdout )
        for line in it:
            if line == '  "inner": [\n':
                yield from elements( it , 4 , True )
                break
        for _ in it:
            pass
        proc.wait()
    finally:
        os.unlink( tu.name )
    if proc.returncode:
        err.seek( 0 )
        errors = [ l for l in err.read().decode().splitlines()
                   if "error" in l ]
        sys.exit( "the headers do not compile together:\n  " +
                  "\n  ".join( errors[ :10 ] ) )


def collect( nodes , inc ):
    """Returns { namespace : { name : headers } } for the names declared at
    namespace scope in inc, a class with the header that defines it and any
    other name with all those that declare it (e.g., the overloads of a
    function in several headers), and the list of those that cannot be
    exported."""
    names = collections.defaultdict( lambda : collections.defaultdict( set ) )
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
            if kind == "File":
                if node[ "file" ]:
                    state[ "file" ] = node[ "file" ]
                continue
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
            # a class template declared only to be specialized (a
            # customization point, e.g., MPICodec) is defined by its
            # complete specializations
            if ( kind == "ClassTemplateSpecializationDecl" ) and \
               node.get( "completeDefinition" ) and ns and \
               ( ns[ 0 ] == "ADS" ) and ( "detail" not in ns ):
                defined.add( ( "::".join( ns ) , node.get( "name" ) ) )
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
                names[ key ][ node[ "name" ] ] = { header }
            elif ( key , node[ "name" ] ) not in defined:
                names[ key ][ node[ "name" ] ].add( header )
            # the enumerators of an unscoped enum live in the namespace
            if ( kind == "EnumDecl" ) and not node.get( "scopedEnumTag" ):
                for e in node.get( "inner" , [] ):
                    if e.get( "kind" ) == "EnumConstantDecl":
                        names[ key ][ e[ "name" ] ].add( header )

    walk( nodes , [] )
    for key , name in declared - defined:
        names[ key ].pop( name , None )
    return names , internal , defined


BANNER = "/*" + "-" * 74 + "*/"


def title( text ):
    text = f" {text} "
    left = ( 74 - len( text ) ) // 2
    return "/*" + "-" * left + text + "-" * ( 74 - left - len( text ) ) + "*/"


def para( text ):
    return [ " * " + l for l in textwrap.wrap( text , 75 ,
                                                 break_on_hyphens = False ) ]


def head( base , *about ):
    """The comment at the top of the interface base, with the paragraphs
    about (a line of code if it begins with spaces)."""
    out = [ BANNER , title( f"File {base}" ) , BANNER , "/** @file" ]
    for a in about:
        out += [ " * " + a ] if a.startswith( " " ) else para( a )
        out += [ " *" ]
    out += para( "This file is written by cmake/ads_module_exports.py, which "
                 "reads the names from the AST of the headers and their sets "
                 "from cmake/ads_sets.txt: it is not to be edited by hand." )
    out += [ " *" , " * \\author Donato Meoli \\n" ,
             " *         Dipartimento di Informatica \\n" ,
             " *         Università di Pisa \\n" , " *" ,
             " * \\copyright &copy; by Donato Meoli" , " */" ]
    return out


def write( out_file , out ):
    os.makedirs( os.path.dirname( out_file ) , exist_ok = True )
    with open( out_file , "w" ) as f:
        f.write( "\n".join( out ) + "\n" )


ABOUT = {
    "core" : "Interface of the C++20 module ads.core, which exports the names "
             "that the headers of the core of ADS [see ADS/core.h] declare in "
             "the namespace ADS and in those nested in it, but for the "
             "namespaces detail; the modules ads.base and ads.advanced "
             "export it in turn.",
    "base" : "Interface of the C++20 module ads.base, which exports ads.core "
             "and the names that the headers of the base set of ADS [see "
             "ADS/base.h] declare in the namespace ADS and in those nested in "
             "it, but for the namespaces detail; a translation unit that "
             "writes import ads.base; compiles neither the advanced headers "
             "but those that the base ones use inside, whose names are not "
             "exported, nor the executors and the headers of MPI.",
    "advanced" : "Interface of the C++20 module ads.advanced, which exports "
                 "ads.core and the names that the headers of the advanced "
                 "set of ADS [see ADS/advanced.h] declare in the namespace "
                 "ADS and in those nested in it, but for the namespaces "
                 "detail; the names of the base headers that these use are "
                 "not exported. The names of the executors of OpenMP, oneTBB "
                 "and FastFlow and of the sort on MPI are exported only if "
                 "the module is compiled with their library, as their "
                 "headers are.",
}


def write_module( root , names ):
    """Writes the interfaces of the three modules of the sets, and that of
    ads."""
    lost = sorted( { h for ns in names.values() for hs in ns.values()
                     for h in hs if h[ 4: ] not in SETS.set } )
    if lost:
        sys.exit( "names declared by headers with no set: " +
                  ", ".join( lost ) )
    for s , ( module , path ) in ads_sets.MODULE.items():
        base = os.path.basename( path )
        out = head( base , ABOUT[ s ] )
        out += [ BANNER , title( "GLOBAL MODULE FRAGMENT" ) , BANNER , "" ,
                 "module;" , "" , f'#include "{ads_sets.ENTRY[ s ]}"' , "" ,
                 BANNER , title( "MODULE PURVIEW" ) , BANNER , "" ,
                 f"export module {module};" , "" ]
        if s != "core":
            out += [ "export import ads.core;" , "" ]
        members = ads_sets.MEMBERS[ s ]
        for ns in sorted( names ):
            by_header = collections.defaultdict( list )
            for n , headers in names[ ns ].items():
                mine = sorted( h for h in headers
                               if SETS.set[ h[ 4: ] ] in members )
                if mine:
                    by_header[ mine[ 0 ] ].append( n )
            if not by_header:
                continue
            parts = ns.split( "::" )
            out += [ "export namespace " + parts[ 0 ] , "{" ]
            for p in parts[ 1: ]:
                out += [ "namespace " + p , "{" ]
            for header in sorted( by_header ):
                out.append( "" )
                out.append( "// " + header[ 4: ] )
                if header in OPTIONAL:
                    out.append( f"#if defined( {OPTIONAL[ header ]} )" )
                for n in sorted( by_header[ header ] ):
                    out.append( f"using {ns}::{n};" )
                if header in OPTIONAL:
                    out.append( "#endif" )
            out += [ "}" ] * len( parts )
            out.append( "" )
        out += [ BANNER , title( f"End File {base}" ) , BANNER ]
        write( os.path.join( root , path ) , out )

    out = head( "ADS.cppm" ,
                "Interface of the C++20 module ads, which exports the "
                "modules ads.core, ads.base and ads.advanced, i.e., every "
                "name that the headers of include/ADS declare in the "
                "namespace ADS and in those nested in it, but for the "
                "namespaces detail; a translation unit can then write" ,
                "    import ads;" ,
                "in place of the #include of the headers, or import one of "
                "the three for a part of the library [see "
                "cmake/ads_sets.txt]. The macros (e.g., ADS_PRE of "
                "Contract.h) are not exported by a module, so a file that "
                "uses them still includes the header that defines them, and "
                "those that configure the headers (e.g., ADS_CONTRACTS, "
                "ADS_WITH_GMP) take the value they have where the module is "
                "compiled." )
    out += [ BANNER , title( "MODULE PURVIEW" ) , BANNER , "" ,
             "export module ads;" , "" ,
             "export import ads.core;" , "export import ads.base;" ,
             "export import ads.advanced;" , "" ,
             BANNER , title( "End File ADS.cppm" ) , BANNER ]
    write( os.path.join( root , "src" , "ADS.cppm" ) , out )


def main():
    ap = argparse.ArgumentParser(
        description = "Writes the interfaces of the C++20 modules of ADS." )
    ap.add_argument( "--clang" , default = "clang++" )
    ap.add_argument( "flags" , nargs = "*" )
    args = ap.parse_args()

    root = os.path.dirname( os.path.dirname( os.path.abspath( __file__ ) ) )
    inc = os.path.join( root , "include" )
    # the headers, each with its set
    public = ads_sets.public_headers()
    errors = ads_sets.check( SETS , ads_sets.direct( ads_sets.includes() ,
                                                     set( public ) ) )
    if errors:
        sys.exit( "ads_sets.py:\n  " + "\n  ".join( errors ) )
    headers = public
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
    names , internal , _ = collect( dump_ast( args.clang , inc , headers ,
                                             args.flags ) , inc )
    if internal:
        sys.exit( "names with internal linkage, which a module cannot "
                  "export:\n  " + "\n  ".join( sorted( set( internal ) ) ) )
    write_module( root , names )


if __name__ == "__main__":
    main()

###############################################################################
#                       End File ads_module_exports.py                        #
###############################################################################
