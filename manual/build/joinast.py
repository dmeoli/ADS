###############################################################################
#                                 joinast.py                                  #
#                                                                             #
# Reads each input file of pandoc.yaml by itself into the AST of Pandoc and   #
# joins the ASTs in reading order, since the Markdown reader of Pandoc 2      #
# takes on the files read together memory that grows faster than their size  #
# (more than 9 GB on the whole manual, about 600 MB on its largest chapter).  #
# Writes:                                                                     #
#                                                                             #
# - <out>/manual.json, the joined AST, the metadata of the first file that    #
#   has any and the identifiers of the headings made unique as Pandoc does    #
#   on files read together (the second "x" is "x-1", the third "x-2");        #
# - <out>/pandoc.yaml, pandoc.yaml with manual.json as its only input, read   #
#   as JSON, for the filters and the writer.                                  #
#                                                                             #
# Usage, from manual/build/: python3 joinast.py pandoc.yaml ../generated      #
###############################################################################

import json
import os
import subprocess
import sys


def defaults( path ):
    """The reader, the input files and the other lines of a defaults file
    of Pandoc, as pandoc.yaml writes them."""
    reader , inputs , rest , listing = None , [] , [] , False
    for line in open( path ):
        if line.startswith( "from:" ):
            reader = line.split( ":" , 1 )[ 1 ].strip()
            continue
        if line.startswith( "input-files:" ):
            listing = True
            continue
        if listing:
            if line.startswith( "  - " ):
                inputs.append( line[ 4: ].strip() )
                continue
            listing = False
        rest.append( line )
    return reader , inputs , rest


def unique( blocks , seen ):
    """The identifiers of the headings of blocks, and of those inside
    divisions, made unique against seen."""
    for b in blocks:
        if b[ "t" ] == "Header":
            attr = b[ "c" ][ 1 ]
            ident = attr[ 0 ]
            if ident:
                new , k = ident , 0
                while new in seen:
                    k += 1
                    new = "%s-%d" % ( ident , k )
                attr[ 0 ] = new
                seen.add( new )
        elif b[ "t" ] == "Div":
            unique( b[ "c" ][ 1 ] , seen )


def main( yaml , out ):
    reader , inputs , rest = defaults( yaml )
    doc , seen = None , set()
    for f in inputs:
        ast = json.loads( subprocess.run(
            [ "pandoc" , "-f" , reader , "-t" , "json" , f ] ,
            check = True , capture_output = True ).stdout )
        unique( ast[ "blocks" ] , seen )
        if doc is None:
            doc = ast
        else:
            doc[ "blocks" ] += ast[ "blocks" ]
            if not doc[ "meta" ]:
                doc[ "meta" ] = ast[ "meta" ]
    os.makedirs( out , exist_ok = True )
    with open( os.path.join( out , "manual.json" ) , "w" ) as g:
        json.dump( doc , g )
    with open( os.path.join( out , "pandoc.yaml" ) , "w" ) as g:
        g.write( "from: json\n" )
        g.write( "input-files:\n  - %s\n" % os.path.join( out , "manual.json" ) )
        g.writelines( rest )


if __name__ == "__main__":
    main( sys.argv[ 1 ] , sys.argv[ 2 ] )
