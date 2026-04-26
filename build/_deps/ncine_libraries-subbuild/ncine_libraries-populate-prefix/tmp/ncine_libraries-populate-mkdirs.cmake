# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "F:/gitt/jazz2-native/build/_deps/ncine_libraries-src")
  file(MAKE_DIRECTORY "F:/gitt/jazz2-native/build/_deps/ncine_libraries-src")
endif()
file(MAKE_DIRECTORY
  "F:/gitt/jazz2-native/build/_deps/ncine_libraries-build"
  "F:/gitt/jazz2-native/build/_deps/ncine_libraries-subbuild/ncine_libraries-populate-prefix"
  "F:/gitt/jazz2-native/build/_deps/ncine_libraries-subbuild/ncine_libraries-populate-prefix/tmp"
  "F:/gitt/jazz2-native/build/_deps/ncine_libraries-subbuild/ncine_libraries-populate-prefix/src/ncine_libraries-populate-stamp"
  "F:/gitt/jazz2-native/build/_deps/ncine_libraries-subbuild/ncine_libraries-populate-prefix/src"
  "F:/gitt/jazz2-native/build/_deps/ncine_libraries-subbuild/ncine_libraries-populate-prefix/src/ncine_libraries-populate-stamp"
)

set(configSubDirs Debug)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "F:/gitt/jazz2-native/build/_deps/ncine_libraries-subbuild/ncine_libraries-populate-prefix/src/ncine_libraries-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "F:/gitt/jazz2-native/build/_deps/ncine_libraries-subbuild/ncine_libraries-populate-prefix/src/ncine_libraries-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
