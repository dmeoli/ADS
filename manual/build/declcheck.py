#!/usr/bin/env python3
"""Check the C++ blocks of the chapters of the manual against the headers.

Every fenced ``cpp`` block of ``manual/chapters/*.md`` is one of three kinds:

- an include of an extract, a block whose only line is
  ``--8<-- "generated/<file>"``, written by snarf.py from the headers and so
  aligned with them by construction (skipped);
- a declaration block, whose first line is the marker
  ``// declarations of <ADS/dir/File.h>`` (several headers may follow, each in
  angle brackets, also on the next comment lines when they do not fit in
  79 columns), and whose other lines are declarations copied from the
  named headers: class heads with the public members the chapter shows,
  function signatures, aliases, enumerations, concepts;
- an example of use, any other block: statements that call the library.

For a declaration block, each declaration must appear in one of the named
headers (or in the ``.tpp`` files they include), the members of a class
inside the body of that class.  The comparison is on tokens, after removing
comments, attributes ``[[...]]`` (and the macros of the headers that
expand to attributes, as ``ADS_FLAT_INLINE``) and the keyword ``inline``
from both sides;
everything else, the default arguments included, must be equal.  A body
``{ ... }`` in the block stands for any body of the header (the requirements
of a concept, say); a block may omit the base classes of a class, never part
of a signature, and may not show a member initializer list.  A member may
be one the class inherits from a base class of the same header, and a
nested class may be defined outside its class (``Outer< T >::Name``).

For an example, the checker writes one translation unit per chapter in a
temporary directory, each example inside a function of its own, preceded by
the headers named by the declaration blocks of the chapter and by the
``#include`` lines of the examples, and compiles it with
``g++ -std=c++20 -fsyntax-only``.  An example that needs a header of another
chapter names it in an ``#include`` line of its own.

An example whose first line is ``// with <library>``, a name of
``LIBRARIES`` (e.g., ``// with Boost``), needs that library: the examples of
a chapter with the same library are a translation unit of their own,
compiled with the flags of the library (and with those of the environment
variable ``DECLCHECK_<KEY>``, e.g., ``DECLCHECK_LEMON="-I<dir>"``) where its
header is found, and skipped with a message where it is not.  With
``--tests chapter.md:test.cpp[,test.cpp]`` the code of each example of the
chapter, but its ``#include`` lines and its marker, must also be in one of
the tests, the spaces, the ends of the lines and the statements
``CHECK( ... );`` of the tests aside, so that the examples that the manual
prints are those that the tests run.

Usage (from manual/build/):
    python3 declcheck.py ../../include ../chapters [--no-examples] [-j N]
                         [--tests chapter.md:test.cpp,...] [chapter.md ...]
Exit status 1 if any block does not match.
"""

import argparse
import bisect
import concurrent.futures
import os
import re
import shlex
import subprocess
import sys
import tempfile
import time

MARKER = re.compile(r'^\s*//\s*declarations of\s+((?:<[^>]+>\s*)+)$')
MARKER_MORE = re.compile(r'^\s*//\s*((?:<[^>]+>\s*)+)$')
FENCE = re.compile(r'^```cpp[^\n]*\n(.*?)^```', re.S | re.M)
SNIPPET = re.compile(r'^\s*--8<--\s+"[^"]+"\s*$')
WITH = re.compile(r'^\s*//\s*with\s+(\S.*?)\s*$')

# the libraries of the examples marked "// with <library>": the flags of
# the compiler and the header whose presence tells that it is installed
# (MPI and Boost.MPI take the flags of mpicxx besides)
LIBRARIES = {
    'C++23': (['-std=c++23'], 'flat_map'),
    'Boost': (['-DADS_WITH_BOOST'], 'boost/version.hpp'),
    'LEMON': (['-DADS_WITH_LEMON', '-DLEMON_ONLY_TEMPLATES'],
              'lemon/list_graph.h'),
    'GMP': (['-DADS_WITH_GMP'], 'gmpxx.h'),
    'BLAS': (['-DADS_WITH_BLAS'], 'cblas.h'),
    'oneTBB': (['-DADS_WITH_TBB'], 'tbb/task_arena.h'),
    'OpenMP': (['-fopenmp'], 'omp.h'),
    'std::execution': (['-DADS_WITH_STDPAR'], 'execution'),
    'stdexec': (['-DADS_WITH_STDEXEC'], 'stdexec/execution.hpp'),
    'Taskflow': (['-DADS_WITH_TASKFLOW'], 'taskflow/taskflow.hpp'),
    'Kokkos': (['-DADS_WITH_KOKKOS'], 'Kokkos_Core.hpp'),
    'MPI': (['-DADS_WITH_MPI'], 'mpi.h'),
    'Boost.MPI': (['-DADS_WITH_MPI', '-DADS_WITH_BOOST_MPI'],
                  'boost/mpi/communicator.hpp'),
    'Eigen': (['-DADS_WITH_EIGEN', '-I/usr/include/eigen3'],
              'Eigen/SparseCore'),
    'METIS': (['-DADS_WITH_METIS'], 'metis.h'),
    'KaHIP': (['-DADS_WITH_KAHIP'], 'kaHIP_interface.h'),
    'GraphBLAS': (['-DADS_WITH_GRAPHBLAS', '-I/usr/include/suitesparse'],
                  'GraphBLAS.h'),
    'NetworKit': (['-DADS_WITH_NETWORKIT', '-fopenmp'],
                  'networkit/graph/Graph.hpp'),
    'CRoaring': (['-DADS_WITH_CROARING'], 'roaring/roaring.hh'),
    'sdsl-lite': (['-DADS_WITH_SDSL'], 'sdsl/cereal.hpp'),
}

# ----------------------------------------------------------------------------
# tokens

TOKEN = re.compile(r'''
    (?P<str>"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')
  | (?P<num>\.?\d(?:[\w.']|[eEpP][+-])*)
  | (?P<id>[A-Za-z_]\w*)
  | (?P<op>::|->\*?|\.\.\.|&&|\|\||==|!=|<=>|<=|>=|<<=|<<|\+\+|--|[-+*/%&|^]=
          |[{}()\[\];,<>=:.?~!&|^*/%+-])
  ''', re.X)


LEXICAL = re.compile(r'''
    (?P<str>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
  | (?P<line>//[^\n]*)
  | (?P<block>/\*.*?\*/)
  | (?P<pre>^[ \t]*\#(?:[^\n]*\\\n)*[^\n]*)
  ''', re.S | re.M | re.X)


def strip_comments(text):
    """Remove the comments and the preprocessor lines, keep the strings."""
    def repl(m):
        if m.group('str'):
            return m.group(0)
        return '\n' if m.group('pre') else ' '
    return LEXICAL.sub(repl, text)


def tokenize(text):
    toks = []
    for m in TOKEN.finditer(strip_comments(text)):
        t = m.group(0)
        if t == '>>':
            toks += ['>', '>']
        else:
            toks.append(t)
    return normalize(toks)


# the macros of the headers that stand for attributes (Portable.h and the
# like): ADS_FLAT_INLINE, ADS_NO_UNIQUE_ADDRESS, ADS_COLD_PATH, ...
ATTRIBUTE_MACRO = re.compile(r'^ADS_\w*(INLINE|NO_UNIQUE_ADDRESS|COLD_PATH|'
                             r'_KERNEL)$')


def normalize(toks):
    """Drop the attributes [[...]], their macros and the keyword inline."""
    out = []
    i = 0
    while i < len(toks):
        if toks[i] == '[' and i + 1 < len(toks) and toks[i + 1] == '[':
            depth, j = 0, i
            while j < len(toks):
                if toks[j] == '[':
                    depth += 1
                elif toks[j] == ']':
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            i = j + 1
            continue
        if toks[i] == 'inline' or ATTRIBUTE_MACRO.match(toks[i]):
            i += 1
            continue
        out.append(toks[i])
        i += 1
    return out


OPEN = {'(': ')', '[': ']', '{': '}'}


def match_close(toks, i):
    """Index of the bracket closing the one at toks[i]."""
    o, c = toks[i], OPEN[toks[i]]
    depth = 0
    for j in range(i, len(toks)):
        if toks[j] == o:
            depth += 1
        elif toks[j] == c:
            depth -= 1
            if depth == 0:
                return j
    return len(toks) - 1


def is_elision(toks, i):
    return toks[i:i + 3] == ['{', '...', '}']


# ----------------------------------------------------------------------------
# the items of a declaration block

ACCESS = {'public', 'protected', 'private'}


def split_items(toks, access='public'):
    """Split a sequence of member or namespace declarations into items.

    An item is ('decl', tokens, access), the tokens of a declaration without
    its ';', or ('class', head, body, access) for a class with its body;
    access is the access the block gives to the item."""
    items = []
    i, n = 0, len(toks)
    start = 0
    while i < n:
        t = toks[i]
        if t in ACCESS and i + 1 < n and toks[i + 1] == ':' and i == start:
            access = t
            i += 2
            start = i
            continue
        if t in '([':
            i = match_close(toks, i) + 1
            continue
        if t == '{':
            if is_elision(toks, i):
                i += 3
                continue
            j = match_close(toks, i)
            head = toks[start:i]
            if class_name(head) is not None:
                items.append(('class', head, toks[i + 1:j], access))
                k = j + 1
                if k < n and toks[k] == ';':
                    k += 1
                i = start = k
                continue
            i = j + 1
            continue
        if t == ';':
            if i > start:
                items.append(('decl', toks[start:i], access))
            i = start = i + 1
            continue
        i += 1
    if start < n:
        items.append(('decl', toks[start:n], access))
    return items


def class_name(head):
    """The name of the class a head defines, None if it is no class head."""
    depth = 0
    pos = None
    k = 0
    while k < len(head):
        t = head[k]
        if t in '([':
            k = match_close(head, k) + 1
            continue
        if t == '<':
            depth += 1
        elif t == '>':
            depth -= 1
        elif depth == 0 and t in ('class', 'struct', 'union'):
            if k > 0 and head[k - 1] == 'enum':
                return None
            pos = k
            break
        k += 1
    if pos is None or pos + 1 >= len(head):
        return None
    return pos


def head_core(head):
    """A class head without its base clause and without final."""
    pos = class_name(head)
    core = head[:pos + 2]
    rest = head[pos + 2:]
    has_base = ':' in rest
    return core, rest, has_base


# ----------------------------------------------------------------------------
# matching against the header

BOUNDARY_BEFORE = {';', '{', '}', ':'}
FOLLOW = {';', '{', '=', 'try'}


def strip_defaults(toks):
    """Remove the default arguments of the parameter lists; return the
    remaining tokens and, for each one, its index in toks."""
    out, where = [], []
    i, n = 0, len(toks)
    stack = []
    while i < n:
        t = toks[i]
        if t in '(<[{':
            stack.append(t)
        elif t in ')>]}':
            if stack:
                stack.pop()
        elif t == '=' and stack and stack[-1] in '(<':
            # skip to the ',' or the closing bracket of this list
            depth = 0
            while i < n:
                u = toks[i]
                if u in '([{<':
                    depth += 1
                elif u in ')]}>':
                    if depth == 0:
                        break
                    depth -= 1
                elif u == ',' and depth == 0:
                    break
                i += 1
            continue
        out.append(t)
        where.append(i)
        i += 1
    return out, where


_pos_cache = {}


def positions(H):
    """The positions of each token of H."""
    key = id(H)
    if key not in _pos_cache:
        d = {}
        for k, t in enumerate(H):
            d.setdefault(t, []).append(k)
        _pos_cache[key] = (H, d)
    return _pos_cache[key][1]


def find_seq(H, lo, hi, pat):
    """Positions k in [lo, hi) where H[k:...] matches pat; '{ ... }' in pat
    matches any balanced brace group of H."""
    res = []
    hi = min(hi, len(H))
    pos = positions(H).get(pat[0], [])
    for k in pos[bisect.bisect_left(pos, lo):bisect.bisect_left(pos, hi)]:
        i, j = 0, k
        ok = True
        while i < len(pat):
            if j >= hi:
                ok = False
                break
            if is_elision(pat, i):
                if H[j] != '{':
                    ok = False
                    break
                j = match_close(H, j) + 1
                i += 3
                continue
            if pat[i] != H[j]:
                ok = False
                break
            i += 1
            j += 1
        if ok:
            res.append((k, j))
    return res


def find_decl(H, lo, hi, pat):
    """The positions where the declaration pat begins in H[lo:hi]."""
    res = []
    for k, j in find_seq(H, lo, hi, pat):
        if k > lo and H[k - 1] not in BOUNDARY_BEFORE:
            continue
        if j < len(H) and H[j] not in FOLLOW and pat[-1] != '}':
            # a constructor whose initializer list the block omits
            if not (H[j] == ':' and pat[-1] == ')' or
                    H[j] == ':' and pat[-1] == 'noexcept'):
                continue
        res.append(k)
    return res


def access_at(H, lo, k, default):
    """The access of the member at H[k] of the class body H[lo:...]."""
    access = default
    i = lo
    while i < k:
        t = H[i]
        if t == '{':
            i = match_close(H, i) + 1
            continue
        if t in ACCESS and i + 1 < len(H) and H[i + 1] == ':':
            access = t
        i += 1
    return access


def find_class(H, lo, hi, head):
    """The body ranges of the definitions of the class of the block head
    inside H[lo:hi] (more than one where the preprocessor chooses)."""
    res = []
    core, rest, has_base = head_core(head)
    for k, j in find_seq(H, lo, hi, core):
        if k > lo and H[k - 1] not in BOUNDARY_BEFORE:
            continue
        # j is past the class name: the header continues with the base
        # clause, final, or the body
        b = j
        while b < hi and H[b] not in ('{', ';'):
            if H[b] == '(':
                b = match_close(H, b)
            b += 1
        if b >= hi or H[b] != '{':
            continue
        hrest = H[j:b]
        if has_base or rest:
            if hrest != rest:
                continue
        elif hrest and hrest[0] not in (':', 'final'):
            continue
        res.append((b + 1, match_close(H, b),
                    keyword_access(core[class_name(core)])))
        res += base_ranges(H, hrest, 0)
    if not res:
        res = qualified_ranges(H, core[-1])
    return res


def base_names(hrest):
    """The names of the classes of a base clause ': public A< T > , B'."""
    if not hrest or hrest[0] != ':':
        return []
    names, depth, last = [], 0, None
    for t in hrest[1:] + [',']:
        if t in '<(':
            depth += 1
        elif t in '>)':
            depth -= 1
        elif depth == 0 and t == ',':
            if last:
                names.append(last)
            last = None
        elif depth == 0 and re.match(r'[A-Za-z_]\w*$', t) and \
                t not in ('public', 'protected', 'private', 'virtual'):
            last = t
    return names


def base_ranges(H, hrest, level):
    """The body ranges of the base classes defined in the same header, so
    that a block may show an inherited member as a member of the class."""
    res = []
    if level > 3:
        return res
    for name in base_names(hrest):
        for k in range(len(H) - 1):
            if H[k] in ('class', 'struct') and H[k + 1] == name:
                b = k + 2
                while b < len(H) and H[b] not in ('{', ';'):
                    b += 1
                if b < len(H) and H[b] == '{':
                    res.append((b + 1, match_close(H, b),
                                keyword_access(H[k])))
                    res += base_ranges(H, H[k + 2:b], level + 1)
    return res


def qualified_ranges(H, name):
    """The body ranges of a nested class defined outside its class, as in
    'template< class T > class Outer< T >::Name { ... };'."""
    res = []
    for k in range(1, len(H) - 1):
        if H[k] == name and H[k - 1] == '::' and H[k + 1] in ('{', ':'):
            a = k - 1
            while a > 0 and H[a] not in ('class', 'struct', ';', '}', '{'):
                a -= 1
            if H[a] not in ('class', 'struct'):
                continue
            b = k + 1
            while b < len(H) and H[b] not in ('{', ';'):
                b += 1
            if b < len(H) and H[b] == '{':
                res.append((b + 1, match_close(H, b), keyword_access(H[a])))
    return res


def keyword_access(keyword):
    """The default access of the members of a class or a struct."""
    return 'private' if keyword == 'class' else 'public'


def show(toks, limit=110):
    s = ' '.join(toks)
    s = re.sub(r' ([,;)\]>]) ?', r' \1 ', s)
    return s if len(s) <= limit else s[:limit - 3] + '...'


def check_items(H, lo, hi, items, where, errors, default=None):
    """Check the items of a block against H[lo:hi], the body of a class
    whose members have the access default (None outside a class)."""
    for it in items:
        if it[0] == 'class':
            _, head, body, _ = it
            name = head[class_name(head) + 1]
            rngs = find_class(H, lo, hi, head)
            if not rngs:
                errors.append('%s: class head not in the header: %s'
                              % (where, show(head)))
                continue
            inner = 'private' if head[class_name(head)] == 'class' \
                else 'public'
            for member in split_items(body, inner):
                best = None
                for r in rngs:
                    e = []
                    check_items(H, r[0], r[1], [member],
                                where + ' (class %s)' % name, e, r[2])
                    if best is None or len(e) < len(best):
                        best = e
                    if not e:
                        break
                errors += best
            continue
        pat = it[1]
        if not pat:
            continue
        if init_list(pat):
            errors.append('%s: member initializer list in a declaration: %s'
                          % (where, show(pat)))
            continue
        found = find_decl(H, lo, hi, pat)
        if found:
            if default is None or it[2] != 'public' or \
               any(access_at(H, lo, k, default) == 'public' for k in found):
                continue
            errors.append('%s: not public in the header: %s'
                          % (where, show(pat)))
            continue
        sp = strip_defaults(pat)[0]
        SH, where_ = strip_defaults_cached(H)[:2]
        slo = bisect.bisect_left(where_, lo)
        shi = bisect.bisect_left(where_, hi)
        if find_decl(SH, slo, shi, sp):
            errors.append('%s: default arguments differ from the header: %s'
                          % (where, show(pat)))
        else:
            errors.append('%s: not in the header: %s' % (where, show(pat)))


def init_list(pat):
    for k in range(1, len(pat) - 2):
        if pat[k] == ':' and pat[k - 1] in (')', 'noexcept') and \
           re.match(r'[A-Za-z_]', pat[k + 1]) and pat[k + 2] in ('(', '{'):
            return True
    return False


_sd_cache = {}


def strip_defaults_cached(H):
    key = id(H)
    if key not in _sd_cache:
        _sd_cache[key] = strip_defaults(H) + (H,)
    return _sd_cache[key]


_hdr_cache = {}
_main_len = {}


def file_text(inc, name):
    """The text of a header and of the files of impl/ it includes."""
    text = open(os.path.join(inc, name), encoding='utf-8').read()
    for m in re.finditer(r'^\s*#\s*include\s+"([^"]+)"', text, re.M):
        sub = m.group(1)
        if '/impl/' in sub or sub.endswith('.tpp'):
            p = os.path.join(inc, sub)
            if os.path.isfile(p):
                text += '\n' + open(p, encoding='utf-8').read()
    return text


_file_cache = {}


# the macros that open and close the inline namespace of the configuration
# of the layouts [see Abi.h], read as the braces they expand to
ABI_MACROS = {'ADS_ABI_BEGIN': '{', 'ADS_ABI_END': '}'}


def file_tokens(inc, name):
    if name not in _file_cache:
        _file_cache[name] = [ABI_MACROS.get(t, t)
                             for t in tokenize(file_text(inc, name))]
    return _file_cache[name]


def header_tokens(inc, name):
    """The tokens of a header with its impl/ files, followed by those of
    the headers of the library it includes; the latter only serve to find
    the base classes (main_len() tells where they start)."""
    if name in _hdr_cache:
        return _hdr_cache[name]
    path = os.path.join(inc, name)
    if not os.path.isfile(path):
        _hdr_cache[name] = None
        return None
    text = file_text(inc, name)
    toks = list(file_tokens(inc, name))
    _main_len[id(toks)] = len(toks)
    for m in re.finditer(r'^\s*#\s*include\s+"(ADS/[^"]+\.h)"', text, re.M):
        sub = m.group(1)
        if sub != name and os.path.isfile(os.path.join(inc, sub)):
            toks += file_tokens(inc, sub)
    _hdr_cache[name] = toks
    return toks


def main_len(H):
    return _main_len.get(id(H), len(H))


# ----------------------------------------------------------------------------
# the blocks of a chapter

def blocks(path):
    text = open(path, encoding='utf-8').read()
    for m in FENCE.finditer(text):
        line = text.count('\n', 0, m.start()) + 1
        yield line, m.group(1)


def kind(code):
    lines = [l for l in code.split('\n') if l.strip()]
    if lines and all(SNIPPET.match(l) for l in lines):
        return 'snippet', None
    if lines:
        m = MARKER.match(lines[0])
        if m:
            names = m.group(1)
            for l in lines[1:]:
                more = MARKER_MORE.match(l)
                if not more:
                    break
                names += more.group(1)
            return 'decl', re.findall(r'<([^>]+)>', names)
    if lines:
        m = WITH.match(lines[0])
        if m:
            return 'example', m.group(1)
    return 'example', None


def library_key(name):
    """The suffix of the variable DECLCHECK_<KEY> of a library."""
    return re.sub(r'\W', '', name.replace('+', 'X')).upper()


def library_flags(name):
    """The flags of the library name, None if it is not found."""
    flags, probe = LIBRARIES[name]
    flags = list(flags) + shlex.split(
        os.environ.get('DECLCHECK_' + library_key(name), ''))
    if name in ('MPI', 'Boost.MPI'):
        mpi = mpi_flags()
        if mpi is None:
            return None
        flags += mpi
    r = subprocess.run(['g++', '-std=c++20', '-fsyntax-only', '-x', 'c++',
                        '-'] + flags, input='#include <%s>\n' % probe,
                       capture_output=True, text=True)
    return flags if r.returncode == 0 else None


def squeeze(text):
    """The text with each run of spaces and ends of lines as one space."""
    return ' '.join(text.split())


def without_checks(text):
    """The text of a test without its statements CHECK( ... );, which may
    stand between the lines of an example."""
    out, i = [], 0
    for m in re.finditer(r'\bCHECK\s*\(', text):
        if m.start() < i:
            continue
        depth, j = 1, m.end()
        while depth and j < len(text):
            depth += {'(': 1, ')': -1}.get(text[j], 0)
            j += 1
        k = re.match(r'\s*;', text[j:])
        out.append(text[i:m.start()])
        i = j + (k.end() if k else 0)
    out.append(text[i:])
    return ''.join(out)


def example_body(code):
    """The code of an example but its #include lines and its marker."""
    lines = code.split('\n')
    if lines and WITH.match(lines[0]):
        lines = lines[1:]
    return squeeze('\n'.join(l for l in lines
                              if not re.match(r'^\s*#\s*include\b', l)))


def check_decl(inc, headers, code, where):
    errors = []
    toks = tokenize(code)
    for name in headers:
        if header_tokens(inc, name) is None:
            errors.append('%s: no header %s' % (where, name))
    hs = [h for h in headers if header_tokens(inc, h) is not None]
    if not hs:
        return errors
    items = split_items(toks)
    for it in items:
        best = None
        for h in hs:
            e = []
            H = header_tokens(inc, h)
            check_items(H, 0, main_len(H), [it], where, e)
            if not e:
                best = []
                break
            if best is None or len(e) < len(best):
                best = e
        errors += best
    return errors


PRELUDE = ''.join('#include <%s>\n' % h for h in (
    'algorithm', 'array', 'atomic', 'cassert', 'cstdint', 'functional',
    'iostream', 'list', 'map', 'memory', 'numeric', 'optional', 'random',
    'span', 'sstream', 'string', 'string_view', 'thread', 'tuple', 'utility',
    'vector'))

MAIN = re.compile(r'^\s*int\s+main\s*\(', re.M)


def example_unit(chapter, examples, headers):
    """The translation unit of the examples of a chapter: each one in a
    function of its own, or, if it is a whole program with its main(), in a
    namespace of its own."""
    incs = set('#include "%s"' % h for h in headers)
    body = []
    for line, code in examples:
        rest = []
        for l in code.split('\n'):
            if re.match(r'^\s*#\s*include\b', l):
                incs.add(l.strip())
            else:
                rest.append(l)
        if MAIN.search(code):
            body.append('namespace example_%d {\n' % line)
        else:
            body.append('[[maybe_unused]] static void example_%d( void ) {\n'
                        % line)
        body.append('#line %d "%s"\n' % (line + 1, chapter))
        body.append('\n'.join(rest))
        body.append('\n}\n\n')
    return PRELUDE + '\n'.join(sorted(incs)) + '\n\n' + ''.join(body)


def mpi_flags():
    """The flags of the MPI compiler wrapper, None if there is none."""
    try:
        r = subprocess.run(['mpicxx', '--showme:compile'],
                           capture_output=True, text=True)
    except OSError:
        return None
    return r.stdout.split() if r.returncode == 0 else None


def compile_unit(args):
    inc, tmp, chapter, src, extra, lib = args
    base = os.path.splitext(os.path.basename(chapter))[0]
    if lib is not None:
        base += '-' + library_key(lib).lower()
    path = os.path.join(tmp, base + '.cpp')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(src)
    t0 = time.time()
    r = subprocess.run(['g++', '-std=c++20', '-fsyntax-only',
                        '-Wno-unused-variable', '-Wno-unused-but-set-variable',
                        '-I' + inc] + extra + [path],
                       capture_output=True, text=True)
    what = chapter if lib is None else '%s with %s' % (chapter, lib)
    return what, r.returncode, r.stderr, time.time() - t0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('include')
    ap.add_argument('chapters')
    ap.add_argument('files', nargs='*', help='only these chapters')
    ap.add_argument('--no-examples', action='store_true',
                    help='check the declaration blocks only')
    ap.add_argument('--examples-only', action='store_true',
                    help='compile the examples only')
    ap.add_argument('-j', type=int, default=os.cpu_count() or 1)
    ap.add_argument('--keep', help='write the translation units here')
    ap.add_argument('--tests', action='append', default=[],
                    metavar='CHAPTER:TEST[,TEST]',
                    help='the tests that hold the marked examples of a '
                         'chapter')
    a = ap.parse_args()
    tests = {}
    for t in a.tests:
        chapter, files = t.split(':', 1)
        tests[os.path.basename(chapter)] = files.split(',')

    inc = os.path.abspath(a.include)
    files = a.files or sorted(
        os.path.join(a.chapters, f) for f in os.listdir(a.chapters)
        if f.endswith('.md'))
    nerr = 0
    count = {'decl': 0, 'example': 0, 'snippet': 0}
    units = []
    t0 = time.time()
    for path in files:
        rel = os.path.relpath(path)
        headers, examples, marked = [], [], {}
        for line, code in blocks(path):
            k, hs = kind(code)
            count[k] += 1
            if k == 'decl':
                headers += [h for h in hs if h not in headers]
                if not a.examples_only:
                    for e in check_decl(inc, hs, code, '%s:%d' % (rel, line)):
                        print(e)
                        nerr += 1
            elif hs is not None:
                if hs not in LIBRARIES:
                    print('%s:%d: no library %s' % (rel, line, hs))
                    nerr += 1
                else:
                    marked.setdefault(hs, []).append((line, code))
            elif k == 'example':
                examples.append((line, code))
        if os.path.basename(path) in tests:
            text = ''
            for t in tests[os.path.basename(path)]:
                text += squeeze(without_checks(
                    open(t, encoding='utf-8').read())) + ' '
            for lib in [None] + list(marked):
                for line, code in (examples if lib is None
                                   else marked[lib]):
                    if example_body(code) not in text:
                        print('%s:%d: the example is not in %s' %
                              (rel, line, ', '.join(
                                  tests[os.path.basename(path)])))
                        nerr += 1
        if examples and not a.no_examples:
            units.append((inc, None, path,
                          example_unit(os.path.abspath(path), examples,
                                       headers)))
        if not a.no_examples:
            for lib in sorted(marked):
                units.append((inc, lib, path,
                              example_unit(os.path.abspath(path),
                                           marked[lib], headers)))
    t1 = time.time()
    print('declcheck: %d declaration blocks, %d examples, %d extracts; '
          'declarations checked in %.1f s' % (count['decl'], count['example'],
                                              count['snippet'], t1 - t0))
    if units:
        tmp = a.keep or tempfile.mkdtemp(prefix='declcheck-')
        os.makedirs(tmp, exist_ok=True)
        jobs = []
        mpi = None
        found = {}
        for u in units:
            extra = []
            if u[1] is not None:
                if u[1] not in found:
                    found[u[1]] = library_flags(u[1])
                if found[u[1]] is None:
                    print('declcheck: examples of %s with %s skipped, %s not '
                          'found' % (os.path.relpath(u[2]), u[1], u[1]))
                    continue
                jobs.append((u[0], tmp, u[2], u[3], found[u[1]], u[1]))
                continue
            if 'MPI' in u[3]:
                if mpi is None:
                    mpi = mpi_flags() or []
                if not mpi:
                    print('declcheck: examples of %s skipped, no mpicxx'
                          % os.path.relpath(u[2]))
                    continue
                extra = ['-DADS_WITH_MPI'] + mpi
            jobs.append((u[0], tmp, u[2], u[3], extra, None))
        with concurrent.futures.ThreadPoolExecutor(a.j) as ex:
            for chapter, rc, err, dt in ex.map(compile_unit, jobs):
                rel = os.path.relpath(chapter)
                status = 'ok' if rc == 0 else 'FAILED'
                print('declcheck: examples of %s %s in %.1f s'
                      % (rel, status, dt))
                if rc:
                    nerr += 1
                    print(err)
        if not a.keep:
            for f in os.listdir(tmp):
                os.remove(os.path.join(tmp, f))
            os.rmdir(tmp)
        print('declcheck: examples compiled in %.1f s' % (time.time() - t1))
    if nerr:
        print('declcheck: %d errors' % nerr)
    return 1 if nerr else 0


if __name__ == '__main__':
    sys.exit(main())
