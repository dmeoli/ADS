# The rewrites of rewrites.txt for one header, the same that the CMake
# build applies [see ADSLemonShim.cmake]:
#   awk -v hdr=<header> -v dir=<shim dir> -f rewrite.awk rewrites.txt <file>
# writes the rewritten header on the standard output, and fails if an anchor
# is not in it exactly once.

BEGIN { FS = "|" ; n = 0 }

# the rules of the header
FNR == NR {
  if( $0 ~ /^#/ || $1 != hdr )
    next
  anchor[ ++n ] = $2
  what[ n ] = substr( $0 , length( $1 ) + length( $2 ) + 3 )
  found[ n ] = 0
  next
}

{
  line = $0
  for( i = 1 ; i <= n ; ++i ) {
    if( $0 != anchor[ i ] )
      continue
    ++found[ i ]
    op = substr( what[ i ] , 1 , 1 )
    arg = substr( what[ i ] , 2 )
    if( op == "<" ) {
      file = dir "/" arg
      while( ( getline l < file ) > 0 )
        print l
      close( file )
    }
    else if( op == "+" )
      line = line arg
    else if( op == "=" )
      line = arg
  }
  print line
}

END {
  for( i = 1 ; i <= n ; ++i )
    if( found[ i ] != 1 ) {
      print "ADS: lemon/" hdr " has " found[ i ] " lines \"" \
            anchor[ i ] "\" instead of one [see shim/rewrites.txt]" \
            > "/dev/stderr"
      bad = 1
    }
  exit bad
}
