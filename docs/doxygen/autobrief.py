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


def ingroup(text, group):
    """the text of a header with "\\ingroup group" at the end of each block
    of documentation at the scope of a namespace (the functions, the
    types, the classes), so that the second run of Doxygen puts them in the
    group of the header in the Topics, whose page then documents them in
    full; it goes at the end of the last line of the block, so that the
    lines keep their numbers"""
    ends, stack, code = [], [], []
    k, n, line = 0, len(text), 0
    while k < n:
        c = text[k]
        if text.startswith('//', k):
            e = text.find('\n', k)
            e = n if e < 0 else e
            doc = (text.startswith(('///', '//!'), k)
                   and not text.startswith(('///<', '//!<', '////'), k))
            if doc and 'other' not in stack:
                ends.append(('line', e))
            k = e
            continue
        if text.startswith('/*', k):
            e = text.find('*/', k + 2)
            e = n if e < 0 else e
            doc = (text.startswith(('/**', '/*!'), k)
                   and not text.startswith(('/**<', '/*!<', '/**/'), k))
            if doc and 'other' not in stack:
                ends.append(('block', e))
            k = e + 2
            continue
        if c in '"\'':
            e = k + 1
            while e < n and text[e] != c:
                e += 2 if text[e] == '\\' else 1
            k = e + 1
            continue
        if c == '{':
            head = ''.join(code[-200:])
            head = re.split(r'[;{}]', head)[-1]
            stack.append('ns' if re.search(r'\bnamespace\b|extern\s*"C"',
                                           head) else 'other')
        elif c == '}' and stack:
            stack.pop()
        code.append(c)
        k += 1
    # a run of /// lines is one block, marked at the end of its last line
    marks = []
    for kind, e in ends:
        if kind == 'line':
            nxt = text.find('\n', e + 1)
            following = text[e + 1:nxt if nxt >= 0 else n].lstrip()
            if following.startswith(('///', '//!')) and \
                    not following.startswith(('///<', '//!<')):
                continue
        marks.append(e)
    for e in sorted(marks, reverse=True):
        text = text[:e] + ' \\ingroup %s ' % group + text[e:]
    return text


def main():
    group = sys.argv[1] if len(sys.argv) > 2 else None
    with open(sys.argv[-1], encoding='utf-8') as f:
        text = f.read()
    if group:
        text = ingroup(text, group)
    lines = text.split('\n')
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
            state = [None]
            continue
        start = m.end()
        if m.group(0).startswith('//'):
            # a formula opened on a line of /// goes on in the next ones
            if line[:m.start()].strip():
                state = [None]
            lines[k] = line[:start] + comment(line[start:], state)
            continue
        end = line.find('*/', start)
        state = [None]
        if end >= 0:
            lines[k] = (line[:start] + comment(line[start:end], state)
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
