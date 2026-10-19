#!/usr/bin/env bash
###############################################################################
#                           packaging/bump.sh                                 #
#                                                                             #
# Writes the version of a tag and the checksums of its source tarball into    #
# every recipe of packaging/ and into the snippets of the README; flake.nix   #
# needs nothing, since it builds the checkout itself. The entry of the Bazel  #
# Central Registry takes the MODULE.bazel of the tarball, which must already  #
# have the version of the tag.                                                #
#                                                                             #
#   packaging/bump.sh v0.1.0                 downloads ADS-0.1.0.tar.gz from  #
#                                            the GitHub release of the tag    #
#   packaging/bump.sh v0.1.0 <tarball>       uses that tarball instead        #
#                                                                             #
# A recipe that keeps all the versions (Conan, Spack, xmake) gets the new     #
# one added, or its checksum replaced if it is already there; the others      #
# move to the new version, with their release number back to 1. The           #
# Debian changelog gets an entry when the version is new.                     #
###############################################################################

set -euo pipefail

tag=${1:?usage: bump.sh vX.Y.Z [tarball]}
version=${tag#v}
here=$(cd "$(dirname "$0")" && pwd)
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

if [ $# -ge 2 ]; then
    tarball=$(realpath "$2")
else
    tarball=$tmp/ADS-$version.tar.gz
    base=https://github.com/dmeoli/ADS/releases/download
    curl -fsSL -o "$tarball" "$base/$tag/ADS-$version.tar.gz"
fi

sha256=$(sha256sum "$tarball" | cut -d' ' -f1)
sha512=$(sha512sum "$tarball" | cut -d' ' -f1)
sri="sha256-$(openssl dgst -sha256 -binary "$tarball" | base64)"
echo "ADS $version"
echo "  sha256 $sha256"
echo "  sha512 $sha512"
echo "  sri    $sri"

# the MODULE.bazel of the tarball, which the Bazel Central Registry keeps,
# must have the new version, else nothing is written
module=$tmp/MODULE.bazel
tar -xzOf "$tarball" "ADS-$version/MODULE.bazel" > "$module"
if ! grep -q "^    version = \"$version\",\$" "$module"; then
    echo "bump.sh: the MODULE.bazel of the tarball is not at $version" >&2
    exit 1
fi

python3 - "$here" "$version" "$sha256" "$sha512" "$sri" "$module" <<'EOF'
import email.utils, json, re, shutil, sys, time
from pathlib import Path

root, v, sha256, sha512, sri, module = sys.argv[1:]
root = Path(root)
author = "Donato Meoli <donato.meoli.95@gmail.com>"


def edit(rel, *subs):
    p = root / rel
    s = p.read_text()
    for pat, rep in subs:
        s, n = re.subn(pat, rep, s, flags=re.M)
        if n == 0:
            sys.exit(f"bump.sh: {rel}: no match for {pat!r}")
    p.write_text(s)
    print(f"  {rel}")


# vcpkg: the version in vcpkg.json, the SHA512 of the tarball in portfile
p = root / "vcpkg/ports/ads/vcpkg.json"
j = json.loads(p.read_text())
j["version"] = v
j.pop("port-version", None)
p.write_text(json.dumps(j, indent=2) + "\n")
print("  vcpkg/ports/ads/vcpkg.json")
edit("vcpkg/ports/ads/portfile.cmake", (r"SHA512 \S+", f"SHA512 {sha512}"))

# conda-forge
old = re.search(r'^  version: "([^"]+)"',
                (root / "conda/recipe/recipe.yaml").read_text(), re.M)[1]
subs = [(r'^  version: ".*"', f'  version: "{v}"'),
        (r"^  sha256: \S+", f"  sha256: {sha256}")]
if old != v:
    subs.append((r"^  number: \d+", "  number: 0"))
edit("conda/recipe/recipe.yaml", *subs)

# Homebrew
edit("homebrew/Formula/ads.rb",
     (r"download/v[^/]+/ADS-[^\"]+\.tar\.gz",
      f"download/v{v}/ADS-{v}.tar.gz"),
     (r'^  sha256 "\w+"', f'  sha256 "{sha256}"'))

# Nix
edit("nix/package.nix",
     (r'^  version = ".*";', f'  version = "{v}";'),
     (r'hash = "sha256-[^"]*";', f'hash = "{sri}";'))

# AUR and MSYS2
for pkg in ("aur/PKGBUILD", "msys2/PKGBUILD"):
    edit(pkg, (r"^pkgver=.*", f"pkgver={v}"), (r"^pkgrel=.*", "pkgrel=1"),
         (r"^sha256sums=\('\w+'\)", f"sha256sums=('{sha256}')"))
edit("aur/.SRCINFO",
     (r"pkgver = .*", f"pkgver = {v}"), (r"pkgrel = .*", "pkgrel = 1"),
     (r"source = .*",
      f"source = ADS-{v}.tar.gz::https://github.com/dmeoli/ADS/releases/"
      f"download/v{v}/ADS-{v}.tar.gz"),
     (r"sha256sums = \w+", f"sha256sums = {sha256}"))

# Conan: one entry per version in config.yml and conandata.yml
p = root / "conan/config.yml"
s = p.read_text()
if f'"{v}":' not in s:
    s = s.replace("versions:\n",
                  f'versions:\n  "{v}":\n    folder: all\n', 1)
    p.write_text(s)
print("  conan/config.yml")
p = root / "conan/all/conandata.yml"
s = p.read_text()
entry = (f'  "{v}":\n    url: "https://github.com/dmeoli/ADS/releases/'
         f'download/v{v}/ADS-{v}.tar.gz"\n    sha256: "{sha256}"\n')
s, n = re.subn(rf'^  "{re.escape(v)}":\n(    .*\n){{2}}', entry, s,
               flags=re.M)
if n == 0:
    s = s.replace("sources:\n", "sources:\n" + entry, 1)
p.write_text(s)
print("  conan/all/conandata.yml")

# Spack and xmake: one line per version
line = f'    version("{v}", sha256="{sha256}")'
p = root / "spack/packages/ads/package.py"
s = p.read_text()
s, n = re.subn(rf'^    version\("{re.escape(v)}", sha256=.*\)$', line, s,
               flags=re.M)
if n == 0:
    s = re.sub(r'^(    version\("main".*\)\n)', rf"\1{line}\n", s,
               flags=re.M)
s = re.sub(r"download/v[^/]+/ADS-[^\"]+\.tar\.gz",
           f"download/v{v}/ADS-{v}.tar.gz", s)
p.write_text(s)
print("  spack/packages/ads/package.py")
line = f'    add_versions("{v}", "{sha256}")'
p = root / "xmake/packages/a/ads/xmake.lua"
s = p.read_text()
s, n = re.subn(rf'^    add_versions\("{re.escape(v)}", ".*"\)$', line, s,
               flags=re.M)
if n == 0:
    s = re.sub(r"^(    add_versions\()", line.replace("\\", "\\\\") +
               r"\n\1", s, count=1, flags=re.M)
p.write_text(s)
print("  xmake/packages/a/ads/xmake.lua")

# the snippets of FetchContent, CPM.cmake and Bazel in the README
p = root.parent / "README.md"
s = p.read_text()
s = re.sub(r"download/v[0-9.]+/ADS-[0-9.]+\.tar\.gz",
           f"download/v{v}/ADS-{v}.tar.gz", s)
s = re.sub(r"FIND_PACKAGE_ARGS [0-9.]+\)",
           f"FIND_PACKAGE_ARGS {'.'.join(v.split('.')[:2])})", s)
s = re.sub(r'"gh:dmeoli/ADS@[0-9.]+"', f'"gh:dmeoli/ADS@{v}"', s)
s = re.sub(r'bazel_dep\(name = "ads", version = "[0-9.]+"\)',
           f'bazel_dep(name = "ads", version = "{v}")', s)
p.write_text(s)
print("  ../README.md")

# Bazel Central Registry: a directory per version, with the MODULE.bazel of
# the tarball, the source and the presubmit of the latest version before it
bcr = root / "bcr/modules/ads"
p = bcr / "metadata.json"
j = json.loads(p.read_text())
if v not in j["versions"]:
    last = bcr / j["versions"][-1]
    j["versions"].append(v)
    p.write_text(json.dumps(j, indent=4) + "\n")
    (bcr / v).mkdir(exist_ok=True)
    shutil.copy(last / "presubmit.yml", bcr / v / "presubmit.yml")
print("  bcr/modules/ads/metadata.json")
shutil.copy(module, bcr / v / "MODULE.bazel")
(bcr / v / "source.json").write_text(json.dumps({
    "url": f"https://github.com/dmeoli/ADS/releases/download/v{v}/"
           f"ADS-{v}.tar.gz",
    "integrity": sri,
    "strip_prefix": f"ADS-{v}"}, indent=4) + "\n")
print(f"  bcr/modules/ads/{v}/")

# WrapDB: the wrap and the version of the port, and a release of it, whose
# revision goes back to 1 with a new version
edit("wrapdb/subprojects/ads.wrap",
     (r"^directory = .*", f"directory = ADS-{v}"),
     (r"^source_url = .*",
      f"source_url = https://github.com/dmeoli/ADS/releases/download/"
      f"v{v}/ADS-{v}.tar.gz"),
     (r"^source_filename = .*", f"source_filename = ADS-{v}.tar.gz"),
     (r"^source_hash = .*", f"source_hash = {sha256}"))
edit("wrapdb/subprojects/packagefiles/ads/meson.build",
     (r"^  version: '.*',", f"  version: '{v}',"))
p = root / "wrapdb/releases.json"
j = json.loads(p.read_text())
if not any(r.rsplit("-", 1)[0] == v for r in j["ads"]["versions"]):
    j["ads"]["versions"].insert(0, f"{v}-1")
p.write_text(json.dumps(j, indent=2) + "\n")
print("  wrapdb/releases.json")

# Debian: an entry of the changelog for a new version
p = root / "debian/changelog"
s = p.read_text()
if not s.startswith(f"ads ({v}-"):
    date = email.utils.formatdate(time.time(), localtime=True)
    s = (f"ads ({v}-1) unstable; urgency=medium\n\n"
         f"  * New upstream release.\n\n -- {author}  {date}\n\n" + s)
    p.write_text(s)
print("  debian/changelog")

EOF

###############################################################################
#                          End of packaging/bump.sh                           #
###############################################################################
