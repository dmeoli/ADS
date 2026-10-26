###############################################################################
#                                 conf.py                                     #
#                                                                             #
# The configuration of Sphinx for the documentation of ADS: the pages that    #
# docs/pages.py writes from the README and the CHANGELOG (Markdown, by        #
# MyST), and the API reference, whose pages docs/apiref.py writes from the    #
# XML that docs/Doxyfile writes into build-docs/xml and Breathe renders. The  #
# theme is that of Read the Docs, and the Doxygen site lies in its folder     #
# doxygen. The workflow .github/workflows/docs.yml builds it with             #
#                                                                             #
#   (cat docs/Doxyfile; echo "GENERATE_HTML = NO";                            #
#    echo "INPUT = include/ADS"; echo "USE_MDFILE_AS_MAINPAGE =") | doxygen - #
#   python3 docs/pages.py                                                     #
#   sphinx-build -b html docs build-docs/html                                 #
###############################################################################

import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

project = 'ADS'
author = 'Donato Meoli'
copyright = '2026, Donato Meoli'
with open(os.path.join(ROOT, 'CMakeLists.txt')) as f:
    release = re.search(r'VERSION\s+(\d+\.\d+\.\d+)', f.read()).group(1)
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

# a signature longer than this goes one parameter per line, instead of
# being cut at the margin or wrapped at random
maximum_signature_line_length = 110

# the prompts of the shells are not copied with the commands
copybutton_prompt_text = r'\$ |>>> '
copybutton_prompt_is_regexp = True

source_suffix = {'.rst': 'restructuredtext', '.md': 'markdown'}
myst_enable_extensions = ['dollarmath', 'colon_fence']
myst_heading_anchors = 3
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

breathe_projects = {'ADS': os.path.join(ROOT, 'build-docs', 'xml')}
breathe_default_project = 'ADS'
primary_domain = 'cpp'
highlight_language = 'cpp'



def mend_xml():
    """the XML of docs/Doxyfile made fit for Breathe, in place (it is a
    product of the build, which is read and never published): the index
    names each header by its file name alone, while two folders have an
    Anagrams.h each, and doxygenfile finds a header by the name in the
    index, so each header is named there by its path from include/, as
    docs/apiref.py names it in the pages"""
    xml = breathe_projects['ADS']
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

html_theme = 'sphinx_rtd_theme'
html_title = 'ADS: Algorithms and Data Structures'
# the content as wide as the formulae and the signatures need, the cells of
# the tables on more lines (see _static/ads.css)
html_static_path = ['_static']
html_css_files = ['ads.css']
# the sidebar stays in place while the page scrolls and scrolls on its own;
# it holds the areas and the branch of the page down to its classes
html_theme_options = {'navigation_depth': 5, 'collapse_navigation': True,
                      'sticky_navigation': True}
html_context = {
    'display_github': True,
    'github_user': 'dmeoli',
    'github_repo': 'ADS',
    'github_version': 'main',
    'conf_py_path': '/docs/',
}

###############################################################################
#                              End of conf.py                                 #
###############################################################################
