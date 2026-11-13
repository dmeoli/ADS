###############################################################################
#                                  snarf.py                                   #
#                                                                             #
# Extracts from the headers of ADS what the chapters of the manual include    #
# with --8<--, so that the code and the pseudocode are written once, in the   #
# headers, and the manual, the Doxygen site and the library agree:            #
#                                                                             #
# - <out>/<File>-<name>.cpp, the code between the markers //! [name] of a .h  #
#   or .tpp (the markers of \snippet of Doxygen), dedented, without the       #
#   contracts (ADS_PRE, ADS_POST, ADS_INVARIANT) and the [[maybe_unused]]     #
#   copies that only they read, since the text states them;                   #
# - <out>/<File>-<name>.pseudo.md, each pseudocode of the comments (the      #
#   \f[ \class{pseudocode}{...} \f] blocks of the conventions of             #
#   docs/STYLE.md) as a display formula of the Markdown, named after its     #
#   first procedure or function (or else after the function declared after   #
#   the comment), after <Class>- for a member function of a class not named  #
#   as the file, with -2, -3, ... for the same name in the same header; the   #
#   columns r@{\quad}l become rl, since KaTeX does not know @{}.              #
#                                                                             #
# Usage: python3 snarf.py <include folder> <out folder>                       #
###############################################################################

import pathlib
import re
import sys

MARK = re.compile(r"^\s*//! \[([A-Za-z0-9_-]+)\]\s*$")
CONTRACT = re.compile(
    r"^\s*(ADS_PRE|ADS_POST|ADS_INVARIANT)\s*\(|^\s*\[\[maybe_unused\]\]")
DOC = re.compile(r"^\s*///\s?(.*)$")
NAME = re.compile(r"(operator\s*\S+?|~?[A-Za-z_]\w*)\s*\(")
CLASS = re.compile(r"^(?:class|struct)\s+(\w+)[^;]*$")
FNAME = re.compile(
    r"\\textbf\{(?:function|procedure)\}\\?\s*\\mathrm\{(\w+)\}")


def code_snippets(path):
    """the code snippets of a file, as a dictionary name -> lines"""
    out, open_ = {}, {}
    for line in path.read_text(encoding="utf-8").splitlines():
        m = MARK.match(line)
        if m:
            name = m.group(1)
            if name in open_:
                out[name] = open_.pop(name)
            else:
                open_[name] = []
            continue
        for body in open_.values():
            body.append(line)
    if open_:
        sys.exit(f"{path}: snippet {sorted(open_)} not closed")
    return out


def clean(lines):
    """the lines without the contracts, dedented"""
    kept, depth = [], 0
    for line in lines:
        if depth > 0 or CONTRACT.match(line):
            depth = max(depth + line.count("(") - line.count(")"), 0)
            continue
        kept.append(line.rstrip())
    while kept and not kept[0]:
        kept.pop(0)
    while kept and not kept[-1]:
        kept.pop()
    ind = min((len(l) - len(l.lstrip()) for l in kept if l), default=0)
    return [l[ind:] for l in kept]


def pseudocodes(block):
    """the formulae of the pseudocodes in the lines of a comment"""
    text, out, i = "\n".join(block), [], 0
    while True:
        i = text.find(r"\class{pseudocode}{", i)
        if i < 0:
            return out
        j = text.find(r"\f]", i)
        if j < 0:
            return out
        body = text[i + len(r"\class{pseudocode}{"):j].rstrip()
        if body.endswith("}"):
            out.append(body[:-1].rstrip())
        i = j


def pseudo_snippets(path):
    """the pseudocode of the functions of a header, name -> formula; the
    name is that of the first procedure or function of the pseudocode, or
    else that of the function declared after the comment, preceded by the
    class of a member function when the class is not named as the file"""
    out, block, pending, member, cls = {}, [], [], False, None
    for line in path.read_text(encoding="utf-8").splitlines():
        c = CLASS.match(line)
        if c:
            cls = c.group(1)
        m = DOC.match(line)
        if m:
            if not block:
                member = line.startswith(" ")
            block.append(m.group(1))
            continue
        if block and line.strip():
            pending, block = pending + pseudocodes(block), []
        if pending and line.strip():
            n = NAME.search(line)
            if not n and not line.rstrip().endswith(";"):
                continue              # the declaration goes on
            decl = re.sub(r"\W", "", n.group(1)) if n else "op"
            for formula in pending:
                f = FNAME.search(formula)
                name = f.group(1) if f else decl
                if member and cls and cls != path.stem:
                    name = f"{cls}-{name}"
                key, k = name, 2
                while key in out:
                    key, k = f"{name}-{k}", k + 1
                out[key] = formula
            pending = []
    return out


def main():
    src, dst = pathlib.Path(sys.argv[1]), pathlib.Path(sys.argv[2])
    dst.mkdir(parents=True, exist_ok=True)
    for path in sorted(src.rglob("*")):
        if path.suffix not in (".h", ".tpp"):
            continue
        for name, lines in code_snippets(path).items():
            out = dst / f"{path.stem}-{name}.cpp"
            out.write_text("\n".join(clean(lines)) + "\n", encoding="utf-8")
        if path.suffix != ".h":
            continue
        for name, formula in pseudo_snippets(path).items():
            out = dst / f"{path.stem}-{name}.pseudo.md"
            formula = formula.replace(r"{r@{\quad}l}", "{rl}")  # KaTeX
            out.write_text("$$\n" + formula + "\n$$\n", encoding="utf-8")


if __name__ == "__main__":
    main()
