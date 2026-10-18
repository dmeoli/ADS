#!/usr/bin/env python3
###############################################################################
#                                 pages.py                                    #
#                                                                             #
# Writes the pages of the site of the documentation into build-docs/pages,    #
# from the README, which stays the one source of the text: each section of    #
# the README becomes a page of the user guide, the links to the files of the  #
# repository point to GitHub, and the CHANGELOG becomes the release notes.    #
# The workflow .github/workflows/docs.yml runs it before mkdocs.              #
#                                                                             #
#   python3 docs/pages.py && mkdocs build                                     #
###############################################################################

import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'build-docs', 'pages')
BLOB = 'https://github.com/dmeoli/ADS/blob/main/'
TREE = 'https://github.com/dmeoli/ADS/tree/main/'

# the sections of the README and the page each one goes to; the sections
# not listed here go to the home page
PAGES = {
    'Layout': 'user-guide/layout.md',
    'Where each thing comes from': 'user-guide/book.md',
    'Beyond the book': 'user-guide/beyond.md',
    'From The Art of Computer Programming': 'user-guide/taocp.md',
    "From Ferragina's algorithm engineering": 'user-guide/ferragina.md',
    'The exercises and the exams of the course': 'user-guide/exams.md',
    'Engineered structures': 'user-guide/engineered.md',
    'Sorting': 'user-guide/sorting.md',
    'Design by contract': 'user-guide/contracts.md',
    'The standard interface': 'user-guide/standard-interface.md',
    'Further reading': 'user-guide/further-reading.md',
    'Build and test': 'getting-started/installation.md',
    'Use': 'getting-started/quick-start.md',
}


def links(text):
    """the relative links of the README made absolute on GitHub"""
    def fix(m):
        target = m.group(2)
        if re.match(r'[a-z]+:', target) or target.startswith('#'):
            return m.group(0)
        base = TREE if target.endswith('/') else BLOB
        return '%s(%s%s)' % (m.group(1), base, target)
    return re.sub(r'(\]|\]\s)\(([^)]+)\)', fix, text)


def write(path, text):
    full = os.path.join(OUT, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w') as f:
        f.write(text)


def main():
    readme = open(os.path.join(ROOT, 'README.md')).read()
    parts = re.split(r'^## ', readme, flags=re.M)
    home, pages = [parts[0]], {}
    for part in parts[1:]:
        title, _, body = part.partition('\n')
        page = PAGES.get(title.strip())
        text = '# %s\n%s' % (title.strip(), body)
        if page:
            pages[page] = pages.get(page, '') + text
        else:
            home.append('## ' + part)
    write('index.md', links(''.join(home)))
    for page, text in pages.items():
        write(page, links(text))
    log = open(os.path.join(ROOT, 'CHANGELOG.md')).read()
    write('release_notes.md', links(log.replace('# Changelog',
                                                '# Release Notes', 1)))


if __name__ == '__main__':
    main()

###############################################################################
#                              End of pages.py                                #
###############################################################################
