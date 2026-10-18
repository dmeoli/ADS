###############################################################################
#                                 conf.py                                     #
#                                                                             #
# The configuration of Sphinx for the documentation of ADS: the pages that    #
# docs/pages.py writes from the README and the CHANGELOG (Markdown, by        #
# MyST), and the API reference, which Exhale builds from the XML that         #
# Doxygen extracts from the headers and Breathe renders; the theme is that    #
# of Read the Docs. The workflow .github/workflows/docs.yml builds it with    #
#                                                                             #
#   python3 docs/pages.py && sphinx-build -b html docs build-docs/html        #
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
    'exhale',               # a page per class, function and file
    'myst_parser',          # the pages in Markdown
    'sphinx.ext.mathjax',   # the formulae of the comments
]

source_suffix = {'.rst': 'restructuredtext', '.md': 'markdown'}
myst_enable_extensions = ['dollarmath', 'colon_fence']
myst_heading_anchors = 3
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

breathe_projects = {'ADS': os.path.join(ROOT, 'build-docs', 'xml')}
breathe_default_project = 'ADS'
primary_domain = 'cpp'
highlight_language = 'cpp'

exhale_args = {
    'containmentFolder': './api',
    'rootFileName': 'library_root.rst',
    'doxygenStripFromPath': os.path.join(ROOT, 'include'),
    'rootFileTitle': 'API Reference',
    'createTreeView': True,
    'exhaleExecutesDoxygen': True,
    'exhaleDoxygenStdin': '\n'.join([
        'INPUT = "' + os.path.join(ROOT, 'include', 'ADS') + '"',
        'FILE_PATTERNS = *.h',
        'RECURSIVE = YES',
        'EXCLUDE_PATTERNS = */impl/*',
        'EXCLUDE_SYMBOLS = detail::* ADS::detail::*',
        'EXTRACT_ALL = YES',
        'JAVADOC_AUTOBRIEF = YES',
        'BUILTIN_STL_SUPPORT = YES',
        'GENERATE_HTML = NO',
        'GENERATE_LATEX = NO',
        'QUIET = YES',
        'WARNINGS = NO',
    ]),
}

html_theme = 'sphinx_rtd_theme'
html_title = 'ADS: Algorithms and Data Structures'
html_theme_options = {'navigation_depth': 3, 'collapse_navigation': True}
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
