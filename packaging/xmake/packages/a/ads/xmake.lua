package("ads")
    set_kind("library", {headeronly = true})
    set_homepage("https://github.com/dmeoli/ADS")
    set_description("Header-only C++20 algorithms and data structures")
    set_license("BSL-1.0")

    add_urls("https://github.com/dmeoli/ADS/releases/download/v$(version)/ADS-$(version).tar.gz",
             "https://github.com/dmeoli/ADS.git")
    add_versions("0.1.0", "3a95aa39198f4ef81d891cdee34e4310d6d9c0ea7e752e58a12a522c9ac43b87")

    add_deps("cmake")

    on_install(function (package)
        import("package.tools.cmake").install(package,
            {"-DADS_BUILD_TESTS=OFF", "-DADS_WITH_GMP=OFF",
             "-DADS_WITH_BLAS=OFF", "-DADS_WITH_BOOST=OFF",
             "-DADS_WITH_HIGHWAY=OFF",
             "-DADS_WITH_LEMON=OFF",
             "-DADS_WITH_MPI=OFF", "-DADS_WITH_STDPAR=OFF",
             "-DADS_WITH_STDEXEC=OFF", "-DADS_WITH_TASKFLOW=OFF",
             "-DADS_WITH_KOKKOS=OFF", "-DADS_WITH_BOOST_MPI=OFF",
             "-DADS_WITH_HWLOC=OFF", "-DADS_WITH_CEREAL=OFF",
             "-DADS_WITH_EIGEN=OFF", "-DADS_WITH_METIS=OFF",
             "-DADS_WITH_KAHIP=OFF", "-DADS_WITH_GRAPHBLAS=OFF",
             "-DADS_WITH_NETWORKIT=OFF", "-DADS_WITH_CROARING=OFF",
             "-DADS_WITH_SDSL=OFF", "-DADS_WITH_PYTHON=OFF"})
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
