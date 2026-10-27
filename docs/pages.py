#!/usr/bin/env python3
###############################################################################
#                                 pages.py                                    #
#                                                                             #
# Writes the pages of the site of the documentation into docs/generated,      #
# from the README, which stays the one source of the text: each section of    #
# the README becomes a page, the links to the files of the repository point   #
# to GitHub, the tables in HTML get the classes of the tables of the theme,   #
# and the CHANGELOG becomes the release notes. It also writes the main page   #
# of the Doxygen site into build-docs/mainpage.md, from the head of the       #
# README, and then apiref.py writes the API reference into docs/api and the   #
# Topics of the Doxygen site. docs/build.sh runs it after the first run of    #
# Doxygen, on the headers and the theory pages, and before the second one.    #
###############################################################################

import os
import re
import shutil

import apiref

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, 'docs', 'generated')
MAINPAGE = os.path.join(ROOT, 'build-docs', 'mainpage.md')
BLOB = 'https://github.com/dmeoli/ADS/blob/main/'
TREE = 'https://github.com/dmeoli/ADS/tree/main/'

# the sections of the README and the page each one goes to; the sections
# not listed here go to the home page
PAGES = {
    'Performance': 'user-guide/performance.md',
    'Layout': 'user-guide/layout.md',
    'Where each thing comes from': 'user-guide/book.md',
    'Beyond the book': 'user-guide/beyond.md',
    'From The Art of Computer Programming': 'user-guide/taocp.md',
    "From Ferragina's algorithm engineering": 'user-guide/ferragina.md',
    'The exercises and the exams of the course': 'user-guide/exams.md',
    'Engineered structures': 'user-guide/engineered.md',
    'Sorting': 'user-guide/sorting.md',
    'Parallelism': 'user-guide/parallelism.md',
    'Design by contract': 'user-guide/contracts.md',
    'The standard interface': 'user-guide/standard-interface.md',
    'Further reading': 'user-guide/further-reading.md',
    'Build and test': 'getting-started/installation.md',
    'Use': 'getting-started/quick-start.md',
    'License': 'project/license.md',
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


def tables(text):
    """the tables in HTML of the README with the classes of the tables of
    the theme, which MyST passes as they are: without them the cells have
    no padding and no borders; the first row, of headings, goes in a thead,
    so that it is styled as the headings of the other tables"""
    def fix(m):
        rows = m.group(1).strip('\n').split('\n')
        head = ''
        if rows and '<th>' in rows[0]:
            head, rows = '<thead>%s</thead>\n' % rows[0], rows[1:]
        return ('<div class="wy-table-responsive"><table class="docutils '
                'align-default">\n%s<tbody>\n%s\n</tbody></table></div>'
                % (head, '\n'.join(rows)))
    return re.sub(r'<table>\n(.*?)</table>', fix, text, flags=re.S)


def write(path, text):
    full = os.path.join(OUT, path)
    os.makedirs(os.path.dirname(full), exist_ok=True)
    with open(full, 'w') as f:
        f.write(tables(links(text)))


def mainpage(head):
    """the main page of the Doxygen site: the head of the README and the
    link to the Sphinx site, which lies one folder up"""
    # the name of the library, which Doxygen would link to its namespace
    text = re.sub(r'^ADS ', '%ADS ', links(head).rstrip('\n'), flags=re.M)
    text += ('\n\nThe guide to the library, from the layout of the headers'
             ' to the contracts and the benchmarks, is on the '
             '<a href="../index.html">Sphinx site</a>, which also has the '
             'same reference; here, the Topics hold the headers by area '
             'and, in each one, its classes.\n')
    os.makedirs(os.path.dirname(MAINPAGE), exist_ok=True)
    with open(MAINPAGE, 'w') as f:
        f.write(text)


def main():
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
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
    write('index.md', ''.join(home))
    for page, text in pages.items():
        write(page, text)
    log = open(os.path.join(ROOT, 'CHANGELOG.md')).read()
    write('project/release_notes.md', log.replace('# Changelog',
                                                  '# Release notes', 1))
    mainpage(parts[0])
    apiref.main()


if __name__ == '__main__':
    main()

###############################################################################
#                              End of pages.py                                #
###############################################################################
