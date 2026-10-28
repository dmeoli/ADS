class Ads < Formula
  desc "Algorithms and data structures of Montresor's textbook, in C++20"
  homepage "https://github.com/dmeoli/ADS"
  url "https://github.com/dmeoli/ADS/releases/download/v0.1.0/ADS-0.1.0.tar.gz"
  sha256 "0000000000000000000000000000000000000000000000000000000000000000"
  license "LGPL-3.0-only"
  head "https://github.com/dmeoli/ADS.git", branch: "main"

  depends_on "cmake" => :build

  def install
    system "cmake", "-S", ".", "-B", "build", "-DADS_BUILD_TESTS=OFF",
           "-DADS_WITH_GMP=OFF", "-DADS_WITH_BLAS=OFF", *std_cmake_args
    system "cmake", "--install", "build"
  end

  test do
    (testpath/"CMakeLists.txt").write <<~CMAKE
      cmake_minimum_required(VERSION 3.21)
      project(use LANGUAGES CXX)
      find_package(ADS REQUIRED)
      add_executable(use use.cpp)
      target_link_libraries(use PRIVATE ADS::ADS)
    CMAKE
    (testpath/"use.cpp").write <<~CPP
      #include "ADS/sequence/List.h"
      #include <algorithm>
      int main() {
        ADS::List<int> L{3, 1, 2};
        std::ranges::reverse(L);
        int s = 0;
        for (int x : L) s = 10 * s + x;
        return s == 213 ? 0 : 1;
      }
    CPP
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    system "./build/use"
  end
end
