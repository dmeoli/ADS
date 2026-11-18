###############################################################################
#                                 conf.py                                     #
#                                                                             #
# The configuration of Sphinx for the documentation of ADS: the pages that    #
# docs/pages.py writes from the README and the CHANGELOG (Markdown, by        #
# MyST), and the API reference, whose pages docs/apiref.py writes from the    #
# XML that docs/Doxyfile writes into build-docs/xml and Breathe renders. The  #
# theme is that of Read the Docs, and the Doxygen site lies in its folder     #
# doxygen. docs/build.sh builds the two sites, after the two runs of Doxygen  #
# that it needs, as the workflow .github/workflows/docs.yml does.             #
###############################################################################

import collections
import glob
import os
import re
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

project = 'ADS'
author = 'Donato Meoli'
copyright = '2026, Donato Meoli'


def derive_version():
    """The version as cmake/DeriveVersion.cmake finds it: the last tag of
    git, else VERSION.txt (filled by git archive), else MODULE.bazel."""
    try:
        tag = subprocess.run(['git', 'describe', '--tags', '--abbrev=0'],
                             cwd=ROOT, capture_output=True, text=True,
                             check=True).stdout.strip()
        m = re.match(r'v?(\d+\.\d+\.\d+)', tag)
        if m:
            return m.group(1)
    except (OSError, subprocess.CalledProcessError):
        pass
    for name, pattern in (('VERSION.txt', r'^v?(\d+\.\d+\.\d+)'),
                          ('MODULE.bazel',
                           r'^\s*version\s*=\s*"(\d+\.\d+\.\d+)"')):
        try:
            with open(os.path.join(ROOT, name)) as f:
                m = re.search(pattern, f.read(), re.M)
            if m:
                return m.group(1)
        except OSError:
            pass
    return '0.0.0'


release = derive_version()
version = release

extensions = [
    'breathe',              # the Doxygen XML in Sphinx
    'myst_parser',          # the pages in Markdown
    'sphinx.ext.mathjax',   # the formulae of the comments
    'sphinx_copybutton',    # a button that copies a block of code
    'sphinx_rtd_dark_mode', # a switch between the light and the dark theme
]

# the light theme first, the dark one on the switch, which the browser
# remembers
default_dark_mode = False

# the formulae typeset when they come into view, since a theory page has
# hundreds of them, as on the Doxygen site [see docs/doxygen/mathjax.js]
mathjax3_config = {'loader': {'load': ['ui/lazy']}}

# the menu lists the pages, not the classes and functions documented in
# them, which would show a class twice under its header, as its page and
# as ADS::Name [see setup() below, for those of Breathe]
toc_object_entries = False

# a signature longer than this goes one parameter per line, instead of
# being cut at the margin or wrapped at random
maximum_signature_line_length = 110

# the prompts of the shells are not copied with the commands
copybutton_prompt_text = r'\$ |>>> '
copybutton_prompt_is_regexp = True

source_suffix = {'.rst': 'restructuredtext', '.md': 'markdown'}
myst_enable_extensions = ['dollarmath', 'colon_fence']
myst_heading_anchors = 3
# STYLE.md is the guide of the documentation for its authors, not a page
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store', 'STYLE.md']

breathe_projects = {'ADS': os.path.join(ROOT, 'build-docs', 'xml')}
breathe_default_project = 'ADS'
primary_domain = 'cpp'
# a block with no language is not colored: the code of the comments comes
# with its language (\code, the fences of the README), while the verbatim
# blocks of a comment (an indented block of Markdown, \verbatim) are text,
# often pseudocode that the lexer of C++ would refuse
highlight_language = 'none'



def caption_text(xml):
    """the content of a caption of a table as text, which is all that a
    caption of Breathe holds: the code as its text, and a formula as its
    TeX without the dollars, the commands of the fonts and the braces, its
    relations as their characters (the Doxygen site keeps the formulae)"""
    def formula(m):
        t = m.group(1).strip().strip('$').strip()
        t = re.sub(r'\\math(?:it|rm|bf)\{([^{}]*)\}', r'\1', t)
        for tex, ch in (('\\le', '\u2264'), ('\\ge', '\u2265'),
                        ('\\ne', '\u2260'), ('\\cdot', '\u00b7')):
            t = t.replace(tex, ch)
        t = t.replace('^*', '*').replace('{', '').replace('}', '')
        return ' '.join(t.split())
    xml = re.sub(r'<formula id="\d+">(.*?)</formula>', formula, xml,
                 flags=re.S)
    return re.sub(r'</?(computeroutput|ref)\b[^>]*>', '', xml)


def constraints(text):
    """the requires-clause of each member function that another one of the
    same compound has the same name, the same parameters and the same
    template parameters as, appended to its argsstring as a trailing one:
    Breathe leaves the clause out, and Sphinx would take the two for one
    declaration (e.g., the copy constructors of a vector for a plain and
    for any allocator)"""
    def strip(t):
        return ' '.join(re.sub(r'<[^>]+>', '', t).split())

    def key(m):
        body = m.group(0)
        tparams = re.search(r'<templateparamlist>.*?</templateparamlist>',
                            body, re.S)
        if tparams:
            body = body.replace(tparams.group(0), '')
        return (re.search(r'<name>(.*?)</name>', body).group(1),
                tuple(strip(t) for t in re.findall(
                    r'<param>\s*<type>(.*?)</type>', body, re.S)),
                strip(tparams.group(0)) if tparams else '')
    members = list(re.finditer(r'<memberdef kind="function".*?</memberdef>',
                               text, re.S))
    counts = collections.Counter(key(m) for m in members)

    def fix(m):
        clause = re.search(r'<requiresclause>(.*?)</requiresclause>',
                           m.group(0), re.S)
        args = re.search(r'<argsstring>(.*?)</argsstring>', m.group(0), re.S)
        if (counts[key(m)] < 2 or not clause or not args
                or ' requires ' in args.group(1)):
            return m.group(0)
        return re.sub(r'<argsstring>(.*?)</argsstring>',
                      lambda a: '<argsstring>%s requires %s</argsstring>'
                      % (a.group(1), strip(clause.group(1))), m.group(0),
                      count=1, flags=re.S)
    return re.sub(r'<memberdef kind="function".*?</memberdef>', fix, text,
                  flags=re.S)


def mend_xml():
    """the XML of docs/Doxyfile made fit for Breathe, in place (it is a
    product of the build, which is read and never published): the index
    names each header by its file name alone, while two folders have an
    Anagrams.h each, and doxygenfile finds a header by the name in the
    index, so each header is named there by its path from include/, as
    docs/apiref.py names it in the pages; the parts of the concepts, which
    Doxygen 1.17 writes and Breathe does not know, are dropped (Breathe
    shows the concepts from their initializer); the captions of the tables
    get the id that Breathe requires, and their formulae and code as text,
    since a caption of Breathe holds text only [see caption_text()]; the
    attribute thread_local of the variables, which Doxygen 1.17 writes and
    Breathe does not know, is dropped; the overloads told apart by their
    requires-clauses alone get them [see constraints()]; the space that
    Doxygen puts before an inline formula is dropped after an opening
    bracket, where the text of the comment has none, as in
    "(\\f$ 2 m \\f$ for ...)" """
    xml = breathe_projects['ADS']
    for path in glob.glob(os.path.join(xml, '*.xml')):
        with open(path) as f:
            text = f.read()
        new = re.sub(r'([(\[]) (<formula id="\d+">)', r'\1\2', text)
        new = re.sub(r' thread_local="(yes|no)"', '', new)
        new = constraints(new)
        if os.path.basename(path).startswith('concept'):
            new = re.sub(r'\s*<conceptparts>.*?</conceptparts>', '', new,
                         flags=re.S)
        if new != text:
            with open(path, 'w') as f:
                f.write(new)
    index = os.path.join(xml, 'index.xml')
    if not os.path.isfile(index):
        return
    with open(index) as f:
        text = f.read()

    def path(m):
        with open(os.path.join(xml, m.group(1) + '.xml')) as f:
            loc = re.search(r'<location file="([^"]+)"', f.read())
        return m.group(0) if loc is None else (
            m.group(0)[:m.start(2) - m.start(0)] + loc.group(1) +
            m.group(0)[m.end(2) - m.start(0):])
    new = re.sub(r'<compound refid="([^"]+)" kind="file"><name>([^<]+)'
                 r'</name>', path, text)
    if new != text:
        with open(index, 'w') as f:
            f.write(new)
    # the caption of a table comes with no id, which the schema of Breathe
    # requires, and gets one
    for path in glob.glob(os.path.join(xml, '*.xml')):
        with open(path) as f:
            text = f.read()
        if '<caption>' not in text:
            continue
        name = os.path.basename(path)[:-4]
        parts = text.split('<caption>')
        text = parts[0] + ''.join('<caption id="%s_caption%d">%s'
                                  % (name, k, part)
                                  for k, part in enumerate(parts[1:]))
        text = re.sub(r'(<caption [^>]*>)(.*?)(</caption>)',
                      lambda m: m.group(1) + caption_text(m.group(2)) +
                      m.group(3), text, flags=re.S)
        with open(path, 'w') as f:
            f.write(text)


mend_xml()


def sentence_case():
    """the titles of the sections of the members that Breathe writes
    (Public Types, Public Functions, ...) in sentence case, as all the
    other titles of the site"""
    from breathe.renderer.sphinxrenderer import SphinxRenderer
    sections = getattr(SphinxRenderer, 'sections', None)
    if sections:
        SphinxRenderer.sections = [(kind, title[:1] + title[1:].lower())
                                   for kind, title in sections]
        SphinxRenderer.section_titles = dict(SphinxRenderer.sections)


sentence_case()


def table_captions():
    """the caption of a table of a comment over the table on the Sphinx
    site too, as its title: Breathe reads it and then leaves it out"""
    from docutils import nodes
    from breathe import parser
    from breathe.renderer.sphinxrenderer import SphinxRenderer
    visit = SphinxRenderer.node_handlers[parser.Node_docTableType]

    def visit_doctable(self, node):
        out = visit(self, node)
        caption = ''.join(node.caption) if node.caption else ''
        if caption.strip() and out and isinstance(out[0], nodes.table):
            out[0].insert(0, nodes.title(caption, caption))
        return out
    # the handler is looked up by the type of the node, in the table that
    # Breathe builds when the class is defined
    SphinxRenderer.node_handlers[parser.Node_docTableType] = visit_doctable


table_captions()


def overloads():
    """the overload of a function that a directive names by its parameters
    found also when its return type has parentheses, e.g., decltype(auto):
    Breathe takes the parameters of a candidate from the first parenthesis
    of its signature, and so matches none, while those of its argsstring
    are the right ones"""
    from sphinx.domains.cpp import DefinitionError
    from breathe.directives.function import DoxygenFunctionDirective
    resolve = DoxygenFunctionDirective._resolve_function

    def same(self, entry, args):
        try:
            return self._parse_args(entry[0].value.argsstring or '()') == args
        except DefinitionError:
            return False

    def by_parameters(self, matches, args, project_info):
        if args is not None:
            found = [entry for entry in matches if same(self, entry, args)]
            if found:
                matches, args = found, None
        return resolve(self, matches, args, project_info)
    DoxygenFunctionDirective._resolve_function = by_parameters


overloads()


def group_scopes():
    """the members of a namespace that Breathe finds in a group, as it does
    those of a header [see docs/doxygen/autobrief.py], declared in their
    namespace, as those it finds in the namespace are: Breathe takes no
    scope from a group, and would declare the functions of two namespaces
    with the same signature as one (partition() of METIS and of KaHIP)"""
    from breathe import parser
    from breathe.renderer.sphinxrenderer import SphinxRenderer
    qualification = SphinxRenderer.get_qualification

    def with_namespace(self):
        names = qualification(self)
        if self.nesting_level > 0:
            return names
        # the member, then the compound that holds it, past its section
        stack = self.qualification_stack
        member = next((n for n in stack
                       if isinstance(n, parser.Node_memberdefType)), None)
        up = next((n for n in stack
                   if isinstance(n, parser.Node_compounddefType)), None)
        if (member is not None and member.qualifiedname and up is not None
                and up.kind == parser.DoxCompoundKind.group):
            return member.qualifiedname.split('::')[:-1] + names
        return names
    SphinxRenderer.get_qualification = with_namespace


group_scopes()

html_theme = 'sphinx_rtd_theme'
html_title = 'ADS: Algorithms and Data Structures'
# the content as wide as the formulae and the signatures need, the cells of
# the tables on more lines, and the look of the PDF of the manual, whose
# fonts are copied next to it (see _static/ads.css)
html_static_path = ['_static'] + sorted(
    glob.glob(os.path.join(ROOT, 'manual', 'build', 'fonts', '*.otf')))
html_css_files = ['ads.css']
# the sidebar stays in place while the page scrolls and scrolls on its own;
# it holds the areas and the branch of the page down to its classes
html_theme_options = {'navigation_depth': 5, 'collapse_navigation': True,
                      'sticky_navigation': True,
                      'style_nav_header_background': '#ffffff'}
html_context = {
    'display_github': True,
    'github_user': 'dmeoli',
    'github_repo': 'ADS',
    'github_version': 'main',
    'conf_py_path': '/docs/',
}



def no_toc_entries(app, doctree):
    """the signatures of Breathe out of the menu: Breathe names each of
    them for the menu by itself, whatever toc_object_entries says; this
    handler runs before the menu is collected"""
    from sphinx import addnodes
    for sig in doctree.findall(addnodes.desc_signature):
        sig['_toc_name'] = ''
        sig['_toc_parts'] = ()


def setup(app):
    app.connect('doctree-read', no_toc_entries, priority=400)

###############################################################################
#                              End of conf.py                                 #
###############################################################################
