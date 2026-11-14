###############################################################################
#                                 bibcheck.py                                 #
#                                                                             #
# Checks that the bibliography of the manual (chapters/Z-bibliography.md and  #
# the fragments bib/*.md) holds every work that the library cites: each DOI   #
# of the \see lines of the headers and of the theory pages and of the README, #
# each work of a \see line without a DOI (found by the surname of its first   #
# author and its title) and each anchor cited by a chapter; it prints what is #
# missing and exits with 1 if anything is.                                    #
#                                                                             #
# Usage: python3 bibcheck.py <root of the repository>                         #
###############################################################################

import pathlib
import re
import sys
import unicodedata

DOI = re.compile(r"(?:doi:\s*|doi\.org/)(10\.\d{4,9}/(?:\([^\s()]*\)|"
                 r"<[^\s<>]*>|[^\s,()<>\]])+)", re.I)
YEAR = re.compile(r"\b(?:1[5-9]|20)\d\d\b")


def plain(text):
    """the text without the escapes of Doxygen and of the URLs"""
    for a, b in (("\\", ""), ("%3C", "<"), ("%3E", ">"), ("%23", "#")):
        text = text.replace(a, b)
    return text


def dois(text):
    """the DOIs of a text, lowercase, without a trailing period"""
    return {d.rstrip(".;").lower() for d in DOI.findall(plain(text))}


def norm(s):
    """lowercase words of letters and digits, umlauts as plain vowels"""
    s = unicodedata.normalize("NFKD", s.lower())
    s = re.sub(r"([aou])\u0308", r"\1", s)
    s = "".join(c for c in s if not unicodedata.combining(c))
    s = re.sub(r"([aou])e", r"\1", s)
    return " ".join(re.sub(r"[^a-z0-9]+", " ", s).split())


def entries(texts):
    """( surname of the first author , titles to look for ) of each entry"""
    out = []
    for text in texts:
        for m in re.finditer(r'id="ref-[\w-]+"></span>(.*?)(?:\n\s*\n|\Z)',
                             text, re.S):
            body = " ".join(m.group(1).split())
            t = re.search(r"\*(.+?)\*", body)
            if not t:
                continue
            first = body[:t.start()].split(",")[0].split()
            title = t.group(1).rstrip(".")
            keys = {norm(title), norm(re.split(r"[:,]", title)[0]),
                    norm(title.split(" and ")[0])}
            keys = {k for k in keys if len(k.split()) > 1 or len(k) >= 4}
            out.append((norm(first[-1]) if first else "", keys))
    return out


def sees(text):
    """the citations of the \\see lines of a header or of a theory page"""
    out, cur = [], None
    for line in text.splitlines() + [""]:
        s = re.sub(r"^\s*(?:\*|//[/!]?)?\s?", "", line).strip()
        if s.startswith("\\see "):
            if cur:
                out.append(cur)
            cur = s[5:]
        elif cur is not None and s and not re.match(r"\\[a-eg-z]|\*/", s):
            cur += " " + s
        else:
            if cur:
                out.append(cur)
            cur = None
    cites = []
    for block in out:
        last = 0
        for m in DOI.finditer(plain(block)):
            last = m.end()
        for c in re.split(r";\s+(?=[A-Z])", plain(block)[last:]):
            c = c.strip(" ,;.")
            if YEAR.search(c) and "," in c:
                cites.append(c)
    return cites


def main():
    root = pathlib.Path(sys.argv[1])
    texts = [(root / "manual/chapters/Z-bibliography.md").read_text()] + \
        [f.read_text() for f in sorted((root / "manual/bib").glob("*.md"))]
    bib = "\n\n".join(texts)
    have, anchors = dois(bib), set(re.findall(r'id="(ref-[\w-]+)"', bib))
    works = entries(texts)
    want, miss_work = {}, {}
    files = list((root / "include").rglob("*.h")) + \
        list((root / "include").rglob("*.tpp")) + \
        list((root / "docs/theory").glob("*.dox")) + [root / "README.md"]
    for f in files:
        text = f.read_text(encoding="utf-8")
        for d in dois(text):
            want.setdefault(d, f.relative_to(root))
        for c in sees(text):
            n = " " + norm(c) + " "
            if not any(a and " " + a + " " in n and
                       any(" " + k + " " in n for k in keys)
                       for a, keys in works):
                miss_work.setdefault(c[:72], f.relative_to(root))
    cited = set()
    for f in (root / "manual/chapters").glob("*.md"):
        cited |= set(re.findall(r"Z-bibliography\.md#(ref-[\w-]+)",
                                f.read_text()))
    miss_doi = sorted(set(want) - have)
    miss_anchor = sorted(cited - anchors)
    for d in miss_doi:
        print(f"missing doi {d} ({want[d]})")
    for c in sorted(miss_work):
        print(f"missing work {c} ({miss_work[c]})")
    for a in miss_anchor:
        print(f"missing anchor {a}")
    print(f"{len(want)} DOIs cited by the library, {len(miss_doi)} missing;"
          f" {len(miss_work)} works without a DOI missing;"
          f" {len(cited)} anchors cited by the chapters,"
          f" {len(miss_anchor)} missing")
    sys.exit(1 if miss_doi or miss_work or miss_anchor else 0)


if __name__ == "__main__":
    main()
