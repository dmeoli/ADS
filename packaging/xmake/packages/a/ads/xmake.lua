package("ads")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/dmeoli/ADS")
    set_description("Header-only C++20 algorithms and data structures")
    set_license("LGPL-3.0")

    add_urls("https://github.com/dmeoli/ADS/releases/download/v$(version)/ADS-$(version).tar.gz",
             "https://github.com/dmeoli/ADS.git")
    add_versions("0.1.0", "0000000000000000000000000000000000000000000000000000000000000000")

    add_deps("cmake")

    on_install(function (package)
        import("package.tools.cmake").install(package,
            {"-DADS_BUILD_TESTS=OFF", "-DADS_WITH_GMP=OFF",
             "-DADS_WITH_BLAS=OFF"})
    end)

    on_test(function (package)
        assert(package:check_cxxsnippets({test = [[
            #include <algorithm>
            void test() {
                ADS::List<int> L{3, 1, 2};
                std::ranges::reverse(L);
            }
        ]]}, {configs = {languages = "c++20"},
              includes = "ADS/sequence/List.h"}))
    end)
