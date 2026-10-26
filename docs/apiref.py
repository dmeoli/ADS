#!/usr/bin/env python3
###############################################################################
#                                  apiref.py                                  #
#                                                                             #
# Writes the API reference of the Sphinx site into docs/api from the XML that #
# docs/Doxyfile writes into build-docs/xml, as a tree that mirrors the        #
# library: a page with the areas (the folders of include/ADS, in the order    #
# and with the lines of the Layout section of the README: the foundations,    #
# the abstract data types and the algorithms by technique or by problem), a   #
# page per area with its headers, a page per header with its concepts, its    #
# classes and its functions, and a page per class, each rendered by Breathe.  #
# Each entry has the one line of its description, and each page the link to   #
# the same entity on the Doxygen site, which lies in the folder doxygen of    #
# the Sphinx site.                                                            #
#                                                                             #
# The same tree goes to the Topics of the Doxygen site: apiref.py writes      #
# build-docs/topics.dox, a group per area and in it a group per header, which #
# holds the header, its concepts and its classes; the file has only external  #
# documentation blocks, so no header changes, and the second run of Doxygen   #
# reads it. docs/pages.py calls it, after the first run of Doxygen and before #
# Sphinx.                                                                     #
###############################################################################

import collections
import glob
import os
import re
import shutil
import xml.etree.ElementTree as ET

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
XML = os.path.join(ROOT, 'build-docs', 'xml')
OUT = os.path.join(ROOT, 'docs', 'api')
TOPICS = os.path.join(ROOT, 'build-docs', 'topics.dox')
NAMESPACE = 'ADS::'

# where the Doxygen site is, seen from the page of a header or of a class,
# which lies in docs/api/<area>
DOXYGEN = '../../doxygen/'

# the headers in include/ADS itself, which form an area of their own
TOP = 'ADS'

# the kinds of areas, in the order of the pages; the folders not listed
# here are algorithms
FOUNDATIONS = 'Foundations'
TYPES = 'Abstract data types'
ALGORITHMS = 'Algorithms by technique or by problem'
KINDS = [FOUNDATIONS, TYPES, ALGORITHMS]

# the one line of the headers in include/ADS itself, which the Layout
# section of the README names in a sentence
LINES = {
    TOP: 'The concepts of the abstract data types of the book, the '
         'contracts, the iterators and the portable intrinsics.',
}


# ----- the text of the comments ------------------------------------------- #

def pieces(node):
    """the text of a node of the XML as a list of pieces (kind, text), the
    kind being 'text', 'code' or 'math', on one line"""
    out = []

    def walk(n):
        if n.tag == 'formula':
            out.append(('math', (n.text or '').strip().strip('$').strip()))
        elif n.tag == 'computeroutput':
            out.append(('code', ''.join(n.itertext())))
        elif n.tag in ('simplesect', 'parameterlist', 'xrefsect'):
            pass
        else:
            if n.text:
                out.append(('text', n.text))
            for c in n:
                walk(c)
                if c.tail:
                    out.append(('text', c.tail))
    if node is not None:
        walk(node)
    joined = []
    for kind, t in out:
        t = ' '.join(t.split()) if kind != 'text' else re.sub(r'\s+', ' ', t)
        if joined and kind == 'text' and joined[-1][0] == 'text':
            joined[-1] = ('text', joined[-1][1] + t)
        elif t:
            joined.append((kind, t))
    if joined and joined[0][0] == 'text':
        joined[0] = ('text', joined[0][1].lstrip())
    if joined and joined[-1][0] == 'text':
        joined[-1] = ('text', joined[-1][1].rstrip())
    return [p for p in joined if p[1]]


def first_sentence(ps):
    """the pieces up to the end of the first sentence, a dot followed by a
    space or by the end of the text, and not after e.g., i.e. or et al."""
    out = []
    for kind, t in ps:
        if kind == 'text':
            m = re.search(r'(?<!\be\.g)(?<!\bi\.e)(?<!\bal)(?<!\.)\.(\s|$)',
                          t)
            if m:
                out.append((kind, t[:m.start() + 1]))
                return out
        out.append((kind, t))
    return out


def head(ps):
    """the one line of an entry: its first sentence up to the first colon
    out of brackets, without the references in square brackets [see ...]
    and with no dot at the end"""
    ps = first_sentence(ps)
    out, depth = [], 0
    for kind, t in ps:
        if kind != 'text':
            out.append((kind, t))
            continue
        cut = None
        for k, ch in enumerate(t):
            depth += ch in '([{'
            depth -= ch in ')]}'
            if ch == ':' and depth == 0:
                cut = k
                break
        # a colon after a word or two names what follows, e.g.,
        # "perno(): the partition of ...", and the line goes on
        before = ''.join(x for _, x in out) + t[:cut or 0]
        if cut is not None and len(before.split()) >= 3:
            out.append((kind, t[:cut]))
            break
        out.append((kind, t))
    text = []
    for kind, t in out:
        if kind == 'text':
            t = re.sub(r'\s*\[see [^\]]*\]', '', t)
        text.append((kind, t))
    if text and text[-1][0] == 'text':
        text[-1] = ('text', text[-1][1].rstrip(' ,;.'))
    # the first letter capital, but that of the name of a function
    if text and text[0][0] == 'text':
        t = text[0][1].lstrip()
        if not re.match(r'\w+\(', t):
            t = t[:1].upper() + t[1:]
        text[0] = ('text', t)
    return [p for p in text if p[1]]


def escape(line):
    return re.sub(r'([*_|`\\\[\]<>])', r'\\\1', line)


def rst(ps):
    """the pieces in reStructuredText"""
    out = []
    for kind, t in ps:
        if kind == 'math':
            out.append(':math:`%s`' % t)
        elif kind == 'code':
            out.append('``%s``' % t)
        else:
            out.append(escape(t))
    text = ''.join(out)
    # an inline role needs no letter right before or after it
    return re.sub(r'(?<=\w)(:math:|``)', r'\\ \1', text)


def dox(ps):
    """the pieces in a comment of Doxygen, its commands escaped"""
    out = []
    for kind, t in ps:
        if kind == 'math':
            out.append('\\f$ %s \\f$' % t)
        elif kind == 'code':
            out.append('<tt>%s</tt>' % re.sub(r'([\\@&$#<>%])', r'\\\1', t))
        else:
            out.append(re.sub(r'([\\@&$#<>%])', r'\\\1', t))
    return ''.join(out)


def describe(cd):
    """the one line of a compound or a member: the head of its brief
    description, else of its detailed one"""
    for tag in ('briefdescription', 'detaileddescription'):
        ps = head(pieces(cd.find(tag)))
        if ps:
            return ps
    return []


# ----- reading the XML ----------------------------------------------------- #

class Compound:
    def __init__(self, cd):
        self.refid = cd.get('id')
        self.kind = cd.get('kind')
        self.name = cd.findtext('compoundname')
        self.short = (self.name[len(NAMESPACE):]
                      if self.name.startswith(NAMESPACE) else self.name)
        loc = cd.find('location')
        self.file = loc.get('file') if loc is not None else ''
        self.line = int(loc.get('line') or 0) if loc is not None else 0
        self.line_of = describe(cd)
        self.bases = [b.get('refid') for b in cd.findall('basecompoundref')
                      if b.get('refid')]
        self.derived = [b.get('refid')
                        for b in cd.findall('derivedcompoundref')
                        if b.get('refid')]
        self.inner = [i.get('refid') for i in cd.findall('innerclass')]


def read_xml():
    """the compounds, and the members of the namespaces, which are those
    outside the classes; the macros come from the files"""
    compounds, members = {}, {}
    for path in glob.glob(os.path.join(XML, '*.xml')):
        if os.path.basename(path) in ('index.xml', 'Doxyfile.xml'):
            continue
        for cd in ET.parse(path).getroot().findall('compounddef'):
            c = Compound(cd)
            compounds[c.refid] = c
            if c.kind not in ('namespace', 'file'):
                continue
            for m in cd.iter('memberdef'):
                loc = m.find('location')
                kind = m.get('kind')
                if loc is None or m.get('id') in members:
                    continue
                if c.kind == 'file' and kind != 'define':
                    continue
                if kind not in ('function', 'typedef', 'enum', 'variable',
                                'define'):
                    continue
                members[m.get('id')] = {
                    'kind': kind, 'file': loc.get('file'),
                    'line': int(loc.get('line') or 0),
                    'name': m.findtext('name'),
                    'qualified': m.findtext('qualifiedname')
                    or m.findtext('name'),
                    'args': m.findtext('argsstring') or '',
                    'template': m.find('templateparamlist') is not None,
                    'line_of': describe(m)}
    return compounds, members


# ----- the areas and the headers ------------------------------------------- #

def area_of(path):
    """the area of a header: its folder in include/ADS, else TOP"""
    m = re.match(r'ADS/(?:([^/]+)/)?[^/]+$', path)
    if not m:
        return ''
    return m.group(1) or TOP


def readme_lines():
    """the areas in the order of the Layout section of the README, each with
    its one line, the folders of the types (those of its first list) apart
    from the others"""
    with open(os.path.join(ROOT, 'README.md')) as f:
        readme = f.read()
    part = readme.split('\n## Layout', 1)[-1].split('\n## ', 1)[0]
    lists = re.findall(r'(?:^- .*\n(?:  .*\n)*)+', part, re.M)
    types, lines = [], {}
    for k, items in enumerate(lists):
        for m in re.finditer(r'^- `(\w+)/`, (.*\n(?:  .*\n)*)', items,
                             re.M):
            line = ' '.join(m.group(2).split()).rstrip(';.')
            lines[m.group(1)] = line[:1].upper() + line[1:] + '.'
            if k == 0:
                types.append(m.group(1))
    return types, lines


class Header:
    def __init__(self, f):
        self.file = f.file
        self.name = os.path.basename(f.file)
        self.stem = os.path.splitext(self.name)[0]
        self.refid = f.refid
        self.area = area_of(f.file)
        self.line_of = f.line_of
        self.classes = []
        self.concepts = []
        self.members = []
        self.page = re.sub(r'\W', '_', self.stem)


def headers_of(compounds, members):
    """the headers of each area, with their concepts, their classes, but
    the nested ones, and their members"""
    nested = {i for c in compounds.values()
              if c.kind in ('class', 'struct', 'union') for i in c.inner}
    areas = collections.defaultdict(list)
    by_file = {}
    for f in compounds.values():
        if f.kind == 'file' and area_of(f.file):
            h = Header(f)
            by_file[f.file] = h
            areas[h.area].append(h)
    for c in sorted(compounds.values(), key=lambda c: c.line):
        h = by_file.get(c.file)
        if h is None or c.refid in nested:
            continue
        if c.kind in ('class', 'struct', 'union'):
            h.classes.append(c)
        elif c.kind == 'concept':
            h.concepts.append(c)
    for m in sorted(members.values(), key=lambda m: m['line']):
        h = by_file.get(m['file'])
        if h is None or '<' in m['qualified']:
            continue
        # the macros that configure the code, which nobody documented
        if m['kind'] == 'define' and not m['line_of']:
            continue
        h.members.append(m)
    for hs in areas.values():
        hs.sort(key=lambda h: h.name.lower())
    return areas


def class_page_name(c):
    return 'class_' + re.sub(r'\W+', '_', c.short).strip('_')


def html_page(refid):
    """the page of the Doxygen site of a compound"""
    return refid + '.html'


# ----- the pages ----------------------------------------------------------- #

def title(line, char='='):
    return '%s\n%s\n\n' % (line, char * len(line))


def rubric(line):
    # a heading that stays out of the table of contents, where the pages
    # below a page go and its sections would come between them
    return '.. rubric:: %s\n\n' % line


def toctree(entries, hidden=True, maxdepth=1):
    out = '.. toctree::\n   :maxdepth: %d\n' % maxdepth
    if hidden:
        out += '   :hidden:\n'
    return out + '\n' + ''.join('   %s\n' % e for e in entries) + '\n'


def table(rows, header):
    if not rows:
        return ''
    out = '.. list-table::\n   :header-rows: 1\n   :widths: 30 70\n\n'
    for row in [header] + rows:
        out += '   * - %s\n     - %s\n' % (row[0], row[1] or ' ')
    return out + '\n'


def write(page, content):
    full = os.path.join(OUT, page + '.rst')
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w') as f:
        f.write(content)


def class_page(c, h):
    kind = 'struct' if c.kind == 'struct' else 'class'
    out = title(escape(c.short))
    out += ('Declared in :doc:`%s <%s>` (area :doc:`%s <index>`); the same %s'
            ' on the `Doxygen site <%s%s>`__.\n\n'
            % (escape(h.name), h.page, h.area, kind, DOXYGEN,
               html_page(c.refid)))
    if c.bases or c.derived:
        # the diagram of the hierarchy that the Doxygen site draws for the
        # class, which leads to its page there
        out += ('.. raw:: html\n\n   <p><a href="%s%s"><img alt="inheritance'
                ' diagram of %s" src="%s%s.png"></a></p>\n\n'
                % (DOXYGEN, html_page(c.refid), escape(c.short), DOXYGEN,
                   c.refid))
    out += ('.. doxygen%s:: %s\n   :members:\n   :protected-members:\n'
            '   :undoc-members:\n' % (kind, c.name))
    return out


def member_directive(m, overloaded):
    name = m['qualified']
    # a function without overloads by its name alone, which Breathe finds
    # also when it cannot match the parameters of a template to those of
    # the XML
    if m['kind'] == 'function' and name not in overloaded:
        return '.. doxygenfunction:: %s\n' % name
    if m['kind'] == 'function':
        args = re.match(r'\(.*\)', m['args'])
        return '.. doxygenfunction:: %s%s\n' % (name,
                                                 args.group(0) if args else '')
    kind = {'typedef': 'typedef', 'enum': 'enum', 'variable': 'variable',
            'define': 'define'}[m['kind']]
    return '.. doxygen%s:: %s\n' % (kind, name)


def functions_table(h):
    """a row per name of a function of the header, with the one line of its
    first overload that has one, which links to the first overload"""
    rows, seen = [], collections.OrderedDict()
    for m in h.members:
        if m['kind'] == 'function':
            seen.setdefault(m['name'], []).append(m)
    for name, ms in seen.items():
        line = next((m['line_of'] for m in ms if m['line_of']), [])
        if name.startswith('operator'):
            entry = '``%s``' % name
        else:
            entry = ':cpp:func:`%s() <%s>`' % (escape(name),
                                               ms[0]['qualified'])
        rows.append((entry, rst(line)))
    return rows


def header_page(h, pages, overloaded, defined):
    out = title(escape(h.name))
    out += ('Header ``#include "%s"`` of the area :doc:`%s <index>`; the same'
            ' header on the `Doxygen site <%s%s>`__.\n\n'
            % (h.file, h.area, DOXYGEN, html_page(h.refid)))
    # the comment of the file, which explains what the header holds
    out += ('.. doxygenfile:: %s\n   :sections: briefdescription '
            'detaileddescription\n\n' % h.file)
    if h.concepts:
        out += rubric('Concepts')
        out += table([(':cpp:concept:`%s <%s>`' % (c.short, c.name),
                       rst(c.line_of))
                      for c in h.concepts], ('Concept', 'Description'))
    if h.classes:
        out += rubric('Classes')
        out += table([(':doc:`%s <%s>`' % (escape(c.short), pages[c.refid]),
                       rst(c.line_of)) for c in h.classes],
                     ('Class', 'Description'))
        out += toctree([pages[c.refid] for c in h.classes])
    rows = functions_table(h)
    if rows:
        out += rubric('Functions')
        out += table(rows, ('Function', 'Description'))
    if h.concepts:
        out += rubric('Definitions of the concepts')
        out += ''.join('.. doxygenconcept:: %s\n\n' % c.name
                       for c in h.concepts)
    kinds = [('function', 'Definitions of the functions'),
             ('typedef', 'Types'), ('enum', 'Enumerations'),
             ('variable', 'Variables'), ('define', 'Macros')]
    for kind, name in kinds:
        ms = [m for m in h.members if m['kind'] == kind
              and (kind != 'define' or m['file'] == defined[m['name']])]
        if ms:
            out += rubric(name)
            out += ''.join(member_directive(m, overloaded) + '\n'
                           for m in ms)
    return out


def kind_of(area, types):
    if area == TOP:
        return FOUNDATIONS
    return TYPES if area in types else ALGORITHMS


def ordered(areas, types, lines):
    """the areas in the order of the README, those it does not name last"""
    named = [TOP] + list(lines)
    known = [a for a in named if a in areas]
    return known + sorted(a for a in areas if a not in known)


def line_of_area(area, lines):
    return LINES.get(area) or lines.get(area, '')


def md_inline(line):
    """a line of the README in reStructuredText: the code in double
    quotes, the rest escaped"""
    parts = line.split('`')
    return ''.join('``%s``' % p if k % 2 else escape(p)
                   for k, p in enumerate(parts))


# ----- the Topics of the Doxygen site -------------------------------------- #

def block(*lines):
    """a documentation block of Doxygen with the given lines"""
    return '/** %s\n%s */\n' % (lines[0], ''.join(
        (' *  ' + line).rstrip() + '\n' for line in lines[1:]))


def ident(*words):
    """the name of a group of Doxygen, which takes only letters, digits
    and underscores"""
    return '_'.join(re.sub(r'\W+', '_', w).strip('_') for w in words)


def topics(order, areas, lines):
    """the external documentation blocks that put the tree of the pages in
    the Topics of the Doxygen site: a group per area, a group per header in
    it, with the header, its concepts and its classes"""
    out = ['/* written by docs/apiref.py from the XML of Doxygen */\n']
    for area in order:
        grp = ident('area', area)
        out.append(block('\\defgroup %s %s' % (grp, area),
                         '\\brief %s' % dox([('text', line_of_area(
                             area, lines).replace('`', ''))])))
        for h in areas[area]:
            hdr = ident('hdr', area, h.page)
            lines_ = ['\\defgroup %s %s' % (hdr, h.name),
                      '\\ingroup %s' % grp]
            if h.line_of:
                lines_.append('\\brief %s' % dox(h.line_of))
            out.append(block(*lines_))
            out.append(block('\\file %s' % h.file, '\\ingroup %s' % hdr))
            for c in h.concepts:
                out.append(block('\\concept %s' % c.name,
                                 '\\ingroup %s' % hdr))
            for c in h.classes:
                # Doxygen cuts the name of a specialization on a template,
                # and makes a class of its own of it: those stay on the page
                # of their header only
                if '<' in c.name:
                    continue
                out.append(block('\\%s %s' % (
                    'struct' if c.kind == 'struct' else 'class', c.name),
                    '\\ingroup %s' % hdr))
    with open(TOPICS, 'w') as f:
        f.write('\n'.join(out))


# ----- main ---------------------------------------------------------------- #

def main():
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(OUT)
    if not os.path.isdir(XML):
        write('index', title('API reference') + 'Doxygen has not run.\n')
        return
    compounds, members = read_xml()
    areas = headers_of(compounds, members)
    types, lines = readme_lines()
    order = ordered(areas, types, lines)
    # the names of more functions, and the header of the first definition
    # of each macro, which a few headers define alike
    counts = collections.Counter(m['qualified'] for m in members.values()
                                 if m['kind'] == 'function')
    overloaded = {n for n, k in counts.items() if k > 1}
    defined = {}
    for h in sorted((h for hs in areas.values() for h in hs),
                    key=lambda h: h.file):
        for m in h.members:
            if m['kind'] == 'define':
                defined.setdefault(m['name'], h.file)
    pages = {c.refid: class_page_name(c)
             for hs in areas.values() for h in hs for c in h.classes}

    index = title('API reference')
    index += ('The headers of ADS by area, i.e., by the folder of '
              '``include/ADS`` that holds them, as the :doc:`layout '
              '<../generated/user-guide/layout>` of the library describes '
              'them: each area has its headers, and each header its '
              'concepts, its classes and its functions. The '
              ':ref:`index <genindex>` lists all the classes and the '
              'functions, and the same reference, with the same tree under '
              'its Topics and with the lists of all the classes and of all '
              'the files, is on the `Doxygen site <%s>`__.\n\n'
              % DOXYGEN[3:])
    for kind in KINDS:
        rows = [(':doc:`%s <%s/index>`' % (a, a),
                 md_inline(line_of_area(a, lines)))
                for a in order if kind_of(a, types) == kind]
        if rows:
            index += rubric(kind) + table(rows, ('Area', 'Description'))
    index += toctree(['%s/index' % a for a in order])
    write('index', index)
    topics(order, areas, lines)

    for area in order:
        hs = areas[area]
        out = title(area)
        out += md_inline(line_of_area(area, lines)) + '\n\n'
        where = ('include/ADS' if area == TOP else 'include/ADS/%s' % area)
        out += ('The headers are in `%s <https://github.com/dmeoli/ADS/tree/'
                'main/%s>`__.\n\n' % (where, where))
        out += table([(':doc:`%s <%s>`' % (escape(h.name), h.page),
                       rst(h.line_of)) for h in hs],
                     ('Header', 'Description'))
        out += toctree([h.page for h in hs])
        write('%s/index' % area, out)
        for h in hs:
            write('%s/%s' % (area, h.page),
                  header_page(h, pages, overloaded, defined))
            for c in h.classes:
                write('%s/%s' % (area, pages[c.refid]), class_page(c, h))


if __name__ == '__main__':
    main()

###############################################################################
#                             End of apiref.py                                #
###############################################################################
