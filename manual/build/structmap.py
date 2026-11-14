#!/usr/bin/env python3
"""Write the map of the structures of ADS: every public class of the headers.

usage: structmap.py INCLUDE_DIR OUT_MD
       structmap.py INCLUDE_DIR --check CHAPTER_MD

A class is public when it is defined (not only declared) at namespace scope
in `ADS` or in a namespace nested in it other than `detail`, outside the
`\\cond` sections; an alias at namespace scope whose right-hand side names
such a class (e.g., `UnionFind = DisjointSets<>`) is listed with it. The
chapter of a header is that of its area (`OUTLINE.md`), but for the headers
of `EXCEPT`, which are described elsewhere. OUT_MD gets two Markdown tables
for chapter 1: one row per header with classes, and one row per chapter
with its headers of functions only; with --check, every header and class
of the two tables must appear in CHAPTER_MD. The counts go on stderr.
"""
import os
import re
import sys

INC, OUT = sys.argv[1], sys.argv[2]

# the chapter of each area, as in OUTLINE.md
CHAPTER = {
    '': 1, 'sequence': 2, 'stack': 3, 'queue': 4, 'memory': 5, 'search': 6,
    'dictionary': 7, 'tree': 8, 'priority': 9, 'set': 10, 'mfset': 11,
    'sort': 12, 'algorithm': 13, 'optimization': 13, 'graph': 14,
    'string': 15, 'geometry': 16, 'numeric': 17, 'combinatorial': 18,
    'backtrack': 19, 'complexity': 20, 'intractable': 20, 'compression': 21,
    'succinct': 22, 'external': 23, 'concurrent': 24, 'parallel': 25,
    'distributed': 26,
}
# the file of each chapter
FILE = {
    1: '01-introduction', 2: '02-sequence', 3: '03-stack', 4: '04-queue',
    5: '05-memory', 6: '06-search', 7: '07-dictionary', 8: '08-tree',
    9: '09-priority', 10: '10-set', 11: '11-mfset', 12: '12-sort',
    13: '13-algorithm', 14: '14-graph', 15: '15-string', 16: '16-geometry',
    17: '17-numeric', 18: '18-combinatorial', 19: '19-backtrack',
    20: '20-complexity', 21: '21-compression', 22: '22-succinct',
    23: '23-external', 24: '24-concurrent', 25: '25-parallel',
    26: '26-distributed',
}
# the headers described in a chapter other than that of their area
EXCEPT = {'set/SetInterfaces.h': 1, 'set/Bag.h': 1}


def strip(src):
    """the source without comments, strings, characters and directives,
    but for the markers of the \\cond sections, kept as tokens"""
    src = re.sub(r'///\s*\\cond\b', ' @COND@ ', src)
    src = re.sub(r'///\s*\\endcond\b', ' @ENDCOND@ ', src)
    out, i, n = [], 0, len(src)
    while i < n:
        c = src[i]
        if src.startswith('//', i):
            j = src.find('\n', i)
            i = n if j < 0 else j
        elif src.startswith('/*', i):
            j = src.find('*/', i + 2)
            i = n if j < 0 else j + 2
        elif c in '"\'':
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == '\\' else 1
            out.append(' ')
            i = j + 1
        elif c == '#' and (i == 0 or src[i - 1] == '\n'):
            j = i
            while True:                       # a directive, with its
                k = src.find('\n', j)         # continuation lines
                if k < 0:
                    j = n
                    break
                if src[k - 1] != '\\':
                    j = k
                    break
                j = k + 1
            i = j
        else:
            out.append(c)
            i += 1
    return ''.join(out)


TOKEN = re.compile(r'@COND@|@ENDCOND@|[A-Za-z_]\w*|::|[{}();=<>,:]|\S')


def classes(path):
    """the public classes and the aliases of the header `path`"""
    toks = TOKEN.findall(strip(open(path, encoding='utf-8').read()))
    scope = []            # 'ns:<name>', 'class', 'block'
    cond = 0
    found, aliases = [], []
    i = 0
    pending = None        # the scope that the next '{' opens
    while i < len(toks):
        t = toks[i]
        public = (cond == 0 and scope and
                  all(s.startswith('ns:') for s in scope) and
                  'ns:detail' not in scope and scope[0] == 'ns:ADS')
        if t == '@COND@':
            cond += 1
        elif t == '@ENDCOND@':
            cond -= 1
        elif t == 'namespace':
            j = i + 1
            name = []
            while toks[j] not in '{;=':
                if toks[j] != '::':
                    name.append(toks[j])
                j += 1
            if toks[j] == '{':
                pending = ['ns:' + x for x in name] or ['ns:']
                scope.extend(pending[:-1])
                pending = pending[-1:]
            i = j - 1
        elif t in ('class', 'struct') and (i == 0 or toks[i - 1] not in
                                          ('enum', '<', ',', 'friend')):
            j = i + 1
            while toks[j] in ('alignas', '(', ')') or toks[j].isdigit() or \
                    toks[j].startswith('['):
                j += 1
            name = toks[j]
            k = j + 1               # a definition reaches '{' before ';'
            depth = 0
            while k < len(toks) and not (depth == 0 and toks[k] in '{;'):
                depth += toks[k] == '('
                depth -= toks[k] == ')'
                k += 1
            if k < len(toks) and toks[k] == '{':
                if public and re.match(r'[A-Za-z]', name) and \
                        name not in ('final',):
                    found.append(name)
                pending = ['class']
                i = k - 1
        elif t == 'using' and public and i + 2 < len(toks) and \
                toks[i + 2] == '=' and toks[i + 1][0].isupper():
            j = i + 3
            rhs = []
            while toks[j] != ';':
                rhs.append(toks[j])
                j += 1
            aliases.append((toks[i + 1], rhs))
            i = j
        elif t == '{':
            scope.append(pending[0] if pending else 'block')
            pending = None
        elif t == '}':
            if scope:
                scope.pop()
        elif t == ';':
            pending = None
        i += 1
    return found, aliases


rows, every = [], set()
for root, _, files in os.walk(os.path.join(INC, 'ADS')):
    if '/impl' in root or root.endswith('impl'):
        continue
    for f in sorted(files):
        if not f.endswith('.h'):
            continue
        p = os.path.join(root, f)
        rel = os.path.relpath(p, os.path.join(INC, 'ADS'))
        area = os.path.dirname(rel)
        found, aliases = classes(p)
        rows.append([rel, area, found, aliases])
        every.update(found)
for r in rows:                      # the aliases of a class of the library
    r[3] = [a for a, rhs in r[3] if any(x in every for x in rhs[:1])]

rows.sort(key=lambda r: (EXCEPT.get(r[0], CHAPTER[r[1]]), r[0]))


def link(ch):
    return f'[{ch}]({FILE[ch]}.md#ch-{ch})'


out, n, funcs = [], 0, {}
# the lengths of the dashes give the widths of the columns in the PDF
out.append('| Header | Classes | Chapter |\n'
           '|-----------------|-------------------------------|-----|\n')
for rel, area, found, aliases in rows:
    ch = EXCEPT.get(rel, CHAPTER[area])
    # a class with partial specializations, once
    names = list(dict.fromkeys(found + aliases))
    if not names:
        funcs.setdefault(ch, []).append(rel)
        continue
    n += len(names)
    out.append(f'| `{rel}` | ' + ', '.join(f'`{x}`' for x in names) +
               f' | {link(ch)} |\n')
out.append('\n| Chapter | Headers of functions |\n'
           '|-----|-----------------------------------------------|\n')
for ch in sorted(funcs):
    out.append(f'| {link(ch)} | ' +
               ', '.join(f'`{h}`' for h in funcs[ch]) + ' |\n')
text = ''.join(out)
print(f'{n} classes and aliases in {len(rows) - sum(map(len, funcs.values()))}'
      f' headers, {sum(map(len, funcs.values()))} headers of functions',
      file=sys.stderr)
if OUT == '--check':                 # every name of the map in the chapter
    chapter = open(sys.argv[3], encoding='utf-8').read()
    missing = [x for x in re.findall(r'`([^`]+)`', text)
               if f'`{x}`' not in chapter]
    for x in missing:
        print('missing:', x, file=sys.stderr)
    sys.exit(1 if missing else 0)
open(OUT, 'w', encoding='utf-8').write(text)
