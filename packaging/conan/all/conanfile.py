import os

from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.files import copy, get
from conan.tools.layout import basic_layout

required_conan_version = ">=2.0"


class AdsConan(ConanFile):
    name = "ads"
    description = "Header-only C++20 algorithms and data structures"
    license = "LGPL-3.0-only"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/dmeoli/ADS"
    topics = ("algorithms", "data-structures", "header-only")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"
    no_copy_source = True

    def layout(self):
        basic_layout(self, src_folder="src")

    def package_id(self):
        self.info.clear()

    def validate(self):
        check_min_cppstd(self, 20)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def build(self):
        pass

    def package(self):
        copy(self, "LICENSE", self.source_folder,
             os.path.join(self.package_folder, "licenses"))
        copy(self, "*", os.path.join(self.source_folder, "include"),
             os.path.join(self.package_folder, "include"))

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.set_property("cmake_file_name", "ADS")
        self.cpp_info.set_property("cmake_target_name", "ADS::ADS")
        self.cpp_info.set_property("pkg_config_name", "ads")
