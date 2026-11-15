# --------------------------------------------------------------------------- #
#                                 hooks.py                                    #
#                                                                             #
# The hooks of MkDocs for the web site of the manual. Before the Markdown of  #
# a page is converted, (i) each displayed formula between two lines $$ gets  #
# a blank line before and after, so that pymdownx.arithmatex takes it as a   #
# block and hands it to KaTeX untouched (Pandoc, which builds the PDF, also  #
# takes a display inside a paragraph, and the Markdown of MkDocs would read  #
# \{, * and _ in it), and (ii) the name of a file or folder of the           #
# repository written as code, e.g., `include/ADS/tree/BTree.h`,              #
# `tree/BTree.h` or `BTree.h`, becomes a link to it on GitHub, as            #
# build/source-links.lua does in the PDF with the map that                   #
# `python3 hooks.py --tsv` prints. A name is taken as a path from the root,  #
# from include/, include/ADS/ or manual/, then as the end of a single path   #
# of the repository; when more paths end with it, the one in the folder of   #
# the area of the chapter wins, else the table AMBIGUOUS decides.            #
# --------------------------------------------------------------------------- #

import os
import re
import subprocess
import sys

REPO = 'https://github.com/dmeoli/ADS'  # the repository
BRANCH = 'main'                         # the branch published
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__))))
CHAPTERS = os.path.join(ROOT, 'manual', 'chapters')
SPAN = re.compile(r'(?<![\[`])`([A-Za-z0-9_.][A-Za-z0-9_./-]*)`(?![`\]])')
FILE = re.compile(r'\.(h|tpp|cpp|cppm|py|sh|tex|dox|md|txt|tsv|yml|lua|'
                  r'cmake|bazel|nix)$')
PREFIXES = ('', 'include/', 'include/ADS/', 'manual/')

# the names that more paths end with and that the area of the chapter does
# not settle: (chapter, name) -> path
AMBIGUOUS = {
    ('A-writing-structure', 'hashing.cpp'): 'test/hashing.cpp',
    ('A-writing-structure', 'ordered.cpp'): 'test/ordered.cpp',
    ('A-writing-structure', 'CMakeLists.txt'): 'CMakeLists.txt',
}

# the names written as code that stand for a kind of file or folder, not
# for one of them
GENERIC = {'impl/'}

_tracked = None


def tracked():
    """the files and the folders of the repository, and the paths that end
    with each name"""
    global _tracked
    if _tracked is None:
        files = subprocess.run(['git', '-C', ROOT, 'ls-files'],
                               capture_output=True, text=True,
                               check=True).stdout.split('\n')
        files = [f for f in files if f]
        paths = set(files)
        for f in files:
            p = f.split('/')
            for i in range(1, len(p)):
                paths.add('/'.join(p[:i]) + '/')
        ends = {}
        for q in paths:
            p = q.rstrip('/').split('/')
            for i in range(len(p)):
                e = '/'.join(p[i:]) + ('/' if q.endswith('/') else '')
                ends.setdefault(e, set()).add(q)
        _tracked = (paths, ends)
    return _tracked


def area(chapter):
    """the folder of include/ADS of a chapter, e.g., string for 15-string"""
    return re.sub(r'^\d+-', '', chapter)


def resolve(name, chapter):
    """the path of the repository that a name written as code stands for in
    a chapter, or None"""
    if '..' in name or '*' in name or name in GENERIC:
        return None
    if not FILE.search(name) and '/' not in name:
        if chapter == 'B-benchmarks':  # a program of bench/
            return resolve('bench/' + name + '.cpp', chapter)
        return None
    paths, ends = tracked()
    if (chapter, name) in AMBIGUOUS:
        return AMBIGUOUS[(chapter, name)]
    for pre in PREFIXES:
        for q in (pre + name, pre + name + '/'):
            if q in paths:
                return q
    if '/' not in name.rstrip('/') and name.endswith('/'):
        return None  # a bare folder, e.g., `impl/`, names no single place
    cand = ends.get(name, set()) | ends.get(name + '/', set())
    if len(cand) == 1:
        return next(iter(cand))
    inarea = [q for q in cand
              if q.startswith(f'include/ADS/{area(chapter)}/')]
    if len(inarea) == 1:
        return inarea[0]
    return None


def source_url(path):
    kind = 'tree' if path.endswith('/') else 'blob'
    return f'{REPO}/{kind}/{BRANCH}/{path.rstrip("/")}'


def link_sources(line, chapter):
    def repl(m):
        path = resolve(m.group(1), chapter)
        if not path:
            return m.group(0)
        return f'[`{m.group(1)}`]({source_url(path)})'
    return SPAN.sub(repl, line)


def lines_out_of_code(markdown):
    """the lines of a page with, for each, whether it is out of the fenced
    blocks and of the displays"""
    fence, disp = None, False
    for line in markdown.split('\n'):
        s = line.strip()
        m = re.match(r'(`{3,}|~{3,})', s)
        if fence:
            if m and s.startswith(fence):
                fence = None
            yield line, False
            continue
        if m:
            fence = m.group(1)
            yield line, False
            continue
        if not disp and len(s) > 4 and s.startswith('$$') and \
           s.endswith('$$'):
            yield line, False
            continue
        if s == '$$':
            disp = not disp
            yield line, False
            continue
        yield line, not disp


def on_page_markdown(markdown, page=None, **kwargs):
    chapter = ''
    if page is not None:
        chapter = os.path.splitext(os.path.basename(page.file.src_path))[0]
    out, disp = [], False
    for line, free in lines_out_of_code(markdown):
        s = line.strip()
        ind = line[:len(line) - len(line.lstrip())]
        if not free and len(s) > 4 and s.startswith('$$') and \
           s.endswith('$$') and not disp:
            if out and out[-1].strip():
                out.append(ind)
            out += [line, ind]
            continue
        if not free and s == '$$':
            if not disp:
                if out and out[-1].strip():
                    out.append(ind)
                out.append(line)
            else:
                out.append(line)
                out.append(ind)
            disp = not disp
            continue
        out.append(link_sources(line, chapter) if free else line)
    return '\n'.join(out)


def tsv():
    """the map for build/source-links.lua: a line H <id> <chapter> for the
    first heading of each chapter, and a line <chapter> <name> <url> for
    each name written as code that is a link"""
    for md in sorted(os.listdir(CHAPTERS)) + ['../index.md']:
        if not md.endswith('.md'):
            continue
        chapter = os.path.splitext(os.path.basename(md))[0]
        text = open(os.path.join(CHAPTERS, md), encoding='utf-8').read()
        h = re.search(r'^# .*\{#([^}\s]+)', text, re.M)
        if h:
            print(f'H\t{h.group(1)}\t{chapter}')
        seen = set()
        for line, free in lines_out_of_code(text):
            if not free:
                continue
            for m in SPAN.finditer(line):
                path = resolve(m.group(1), chapter)
                if path and m.group(1) not in seen:
                    seen.add(m.group(1))
                    print(f'{chapter}\t{m.group(1)}\t{source_url(path)}')


if __name__ == '__main__' and sys.argv[1:] == ['--tsv']:
    tsv()
