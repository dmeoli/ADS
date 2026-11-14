###############################################################################
#                                 bibcheck.py                                 #
#                                                                             #
# Checks that the bibliography of the manual (chapters/Z-bibliography.md)     #
# holds every work that the library cites: each DOI of the \see lines of the  #
# headers and of the theory pages and of the README, and each anchor cited   #
# by a chapter; it prints what is missing and exits with 1 if anything is.    #
#                                                                             #
# Usage: python3 bibcheck.py <root of the repository>                         #
###############################################################################

import pathlib
import re
import sys

DOI = re.compile(r"(?:doi:\s*|doi\.org/)(10\.\d{4,9}/[^\s,;)\]>]+)", re.I)


def dois(text):
    """the DOIs of a text, lowercase, without a trailing period"""
    return {d.rstrip(".").lower() for d in DOI.findall(text)}


def main():
    root = pathlib.Path(sys.argv[1])
    bib = (root / "manual/chapters/Z-bibliography.md").read_text()
    have, anchors = dois(bib), set(re.findall(r'id="(ref-[\w-]+)"', bib))
    want = {}
    files = list((root / "include").rglob("*.h")) + \
        list((root / "include").rglob("*.tpp")) + \
        list((root / "docs/theory").glob("*.dox")) + [root / "README.md"]
    for f in files:
        for d in dois(f.read_text(encoding="utf-8")):
            want.setdefault(d, f.relative_to(root))
    cited = set()
    for f in (root / "manual/chapters").glob("*.md"):
        cited |= set(re.findall(r"Z-bibliography\.md#(ref-[\w-]+)",
                                f.read_text()))
    miss_doi = sorted(set(want) - have)
    miss_anchor = sorted(cited - anchors)
    for d in miss_doi:
        print(f"missing doi {d} ({want[d]})")
    for a in miss_anchor:
        print(f"missing anchor {a}")
    print(f"{len(want)} DOIs cited by the library, {len(miss_doi)} missing;"
          f" {len(cited)} anchors cited by the chapters,"
          f" {len(miss_anchor)} missing")
    sys.exit(1 if miss_doi or miss_anchor else 0)


if __name__ == "__main__":
    main()
