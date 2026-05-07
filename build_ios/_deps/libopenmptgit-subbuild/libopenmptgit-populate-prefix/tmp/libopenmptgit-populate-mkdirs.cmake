# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-src")
  file(MAKE_DIRECTORY "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-src")
endif()
file(MAKE_DIRECTORY
  "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-build"
  "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-subbuild/libopenmptgit-populate-prefix"
  "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-subbuild/libopenmptgit-populate-prefix/tmp"
  "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-subbuild/libopenmptgit-populate-prefix/src/libopenmptgit-populate-stamp"
  "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-subbuild/libopenmptgit-populate-prefix/src"
  "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-subbuild/libopenmptgit-populate-prefix/src/libopenmptgit-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-subbuild/libopenmptgit-populate-prefix/src/libopenmptgit-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/tomtomjack/Documents/jassy/jazz2-native-master-2/build_ios/_deps/libopenmptgit-subbuild/libopenmptgit-populate-prefix/src/libopenmptgit-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
