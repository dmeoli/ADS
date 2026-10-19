# Copyright Spack Project Developers. See COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)

from spack_repo.builtin.build_systems.cmake import CMakePackage

from spack.package import *


class Ads(CMakePackage):
    """The algorithms and data structures of Montresor's textbook,
    Algoritmi e Strutture Dati, as a header-only C++20 library."""

    homepage = "https://github.com/dmeoli/ADS"
    url = "https://github.com/dmeoli/ADS/releases/download/v0.1.0/ADS-0.1.0.tar.gz"
    git = "https://github.com/dmeoli/ADS.git"

    maintainers("dmeoli")

    license("LGPL-3.0-only", checked_by="dmeoli")

    version("main", branch="main")
    version("0.1.0", sha256="0000000000000000000000000000000000000000000000000000000000000000")

    depends_on("cxx", type="build")
    depends_on("cmake@3.21:", type="build")

    def cmake_args(self):
        return [
            self.define("ADS_BUILD_TESTS", self.run_tests),
            self.define("BUILD_TESTING", self.run_tests),
        ]
