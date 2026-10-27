#!/bin/bash
###############################################################################
#                                  build.sh                                   #
#                                                                             #
# Builds the two sites of the documentation into build-docs/html, the site of #
# Sphinx with the Doxygen site in its folder doxygen, from the root of the    #
# repository; the workflow .github/workflows/docs.yml runs it, and so does    #
# whoever builds them locally, so that the steps are the same everywhere:     #
#                                                                             #
#   pip install -r docs/requirements.txt && bash docs/build.sh                #
#                                                                             #
# Doxygen must be 1.17, that of the workflow (DOXYGEN names another binary,   #
# SPHINX another sphinx-build). A first run of Doxygen writes the XML of the  #
# headers and of the theory pages, which the headers may \ref, from which     #
# docs/pages.py writes the pages of Sphinx, the main page of the Doxygen      #
# site, the groups of its Topics and the input filters that put each header   #
# in its group; a second run with them writes the HTML and the XML that       #
# Breathe reads. It prints the warnings of the three steps, and fails if one  #
# of them gives a warning.                                                    #
###############################################################################

set -eu -o pipefail

DOXYGEN=${DOXYGEN:-doxygen}
SPHINX=${SPHINX:-sphinx-build}

cd "$( dirname "$0" )/.."

# no comment ends a line with a backslash: in a comment // it joins the
# next line to the comment (g++ -Wcomment), and the rows of a pseudocode
# end with \cr instead of \\ [see docs/STYLE.md]
if grep -rnE --include='*.h' --include='*.tpp' --include='*.cpp' \
        '^\s*(//|\*|/\*).*\\\s*$' include test bench ||
   grep -rnE '\\\s*$' docs/theory
then
 echo "build.sh: comment lines that end with a backslash" >&2
 exit 1
fi
rm -rf build-docs/xml build-docs/doxygen build-docs/html build-docs/doctrees
mkdir -p build-docs

"$DOXYGEN" --version
( cat docs/Doxyfile; echo "GENERATE_HTML = NO"
  echo "INPUT = include/ADS docs/theory"
  echo "USE_MDFILE_AS_MAINPAGE =" ) |
    "$DOXYGEN" - 2> build-docs/doxygen1.log
python3 docs/pages.py
cat docs/Doxyfile build-docs/pass2.cfg |
    "$DOXYGEN" - 2> build-docs/doxygen2.log

# the outline of a page in navtree.js turns its names into entities and then
# sets them as text, which shows "&#x2F;" for "/"
sed -i 's/textContent = escapeHtml(\(text\|name\))/textContent = \1/' \
    build-docs/doxygen/navtree.js
test "$( grep -c 'textContent = escapeHtml' build-docs/doxygen/navtree.js )" \
     = 0
# the space that Doxygen puts before an inline formula dropped after an
# opening bracket, where the comment has none, as the Sphinx site does on
# the XML [see mend_xml() in docs/conf.py]
sed -i 's/\([([]\) \\(/\1\\(/g' build-docs/doxygen/*.html
# the overloads in the outline with the types of their parameters
cat docs/doxygen/outline-overloads.js >> build-docs/doxygen/navtree.js

"$SPHINX" -b html -d build-docs/doctrees docs build-docs/html \
    2> build-docs/sphinx.log
mv build-docs/doxygen build-docs/html/doxygen
touch build-docs/html/.nojekyll

# the warnings of Doxygen, but "is not documented", which EXTRACT_ALL
# silences, and those of Sphinx, the ParseWarning of Breathe included: a
# part of the XML that Breathe does not know is left out of the Sphinx site
n=0
for log in build-docs/doxygen1.log build-docs/doxygen2.log \
           build-docs/sphinx.log; do
 case "$log" in
  *sphinx*) w='WARNING:\|: ParseWarning:' ;;
  *)        w='warning:' ;;
 esac
 k=$( grep -- "$w" "$log" | grep -vc 'is not documented' || true )
 echo "$log: $k warnings"
 grep -- "$w" "$log" | grep -v 'is not documented' || true
 n=$(( n + k ))
done
test "$n" = 0

###############################################################################
#                              End of build.sh                                #
###############################################################################
