#!/usr/bin/env python3
###############################################################################
#                                 autobrief.py                                #
#                                                                             #
# The input filter of Doxygen [see INPUT_FILTER in docs/Doxyfile], which      #
# prints the header it is given with the dots of its comments that end no     #
# sentence escaped: with JAVADOC_AUTOBRIEF the brief description of a comment #
# ends at its first dot followed by a space, which cuts the ranges, as        #
# A[ i .. j ], the ellipses, and the e.g. and the et al. in the middle of a   #
# sentence. The dots of the formulae, of the code and of the verbatim blocks  #
# stay as they are, and so does the code out of the comments.                 #
###############################################################################

import re
import sys

# the regions of a comment where the text is printed as it is
OPEN = re.compile(r'\\f\$|\\f\[|\\f\{|\\code\b|\\verbatim\b|`')
CLOSE = {'\\f$': '\\f$', '\\f[': '\\f]', '\\f{': '\\f}',
         '\\code': '\\endcode', '\\verbatim': '\\endverbatim', '`': '`'}


def escape(text):
    """the dots of a piece of the text of a comment"""
    # the dots of a range or of an ellipsis, each escaped, since a dot
    # followed by an escaped one ends the brief too
    text = re.sub(r'\.{2,}', lambda m: '\\.' * len(m.group(0)), text)
    # the dot of an abbreviation, followed by a backslash, which is not
    # printed, and by the space; et al. ends a sentence before a capital
    text = re.sub(r'\bet al\. (?=[a-z(])', r'et al.\\ ', text)
    return re.sub(r'\b(e\.g|i\.e|cf|vs)\.( |$)', r'\1.\\ ', text)


def comment(text, state):
    """the text of a comment, outside the regions that are printed as they
    are; state holds the region open at the end of the previous line"""
    out, pos = [], 0
    while pos < len(text):
        if state[0]:
            end = text.find(CLOSE[state[0]], pos)
            if end < 0 or (state[0] == '`' and end < 0):
                out.append(text[pos:])
                if state[0] == '`':   # a code span ends with its line
                    state[0] = None
                return ''.join(out)
            end += len(CLOSE[state[0]])
            out.append(text[pos:end])
            pos, state[0] = end, None
            continue
        m = OPEN.search(text, pos)
        stop = m.start() if m else len(text)
        out.append(escape(text[pos:stop]))
        if not m:
            break
        out.append(m.group(0))
        pos, state[0] = m.end(), m.group(0)
    return ''.join(out)


def main():
    with open(sys.argv[1], encoding='utf-8') as f:
        lines = f.read().split('\n')
    block, state = False, [None]
    for k, line in enumerate(lines):
        if block:
            end = line.find('*/')
            if end < 0:
                lines[k] = comment(line, state)
                continue
            lines[k] = comment(line[:end], state) + line[end:]
            block, state = False, [None]
            continue
        m = re.search(r'/\*[*!]|//[/!]', line)
        if not m or line[:m.start()].count('"') % 2:
            continue
        start = m.end()
        if m.group(0).startswith('//'):
            lines[k] = line[:start] + comment(line[start:], [None])
            continue
        end = line.find('*/', start)
        if end >= 0:
            lines[k] = (line[:start] + comment(line[start:end], [None])
                        + line[end:])
        else:
            lines[k] = line[:start] + comment(line[start:], state)
            block = True
    sys.stdout.write('\n'.join(lines))


if __name__ == '__main__':
    main()

###############################################################################
#                            End of autobrief.py                              #
###############################################################################
