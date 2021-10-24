include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        e2239ee6043f73722e7aa812a459f54a28552929  # release-1.11.0
)
FetchContent_MakeAvailable(googletest)

find_package(Qt6 COMPONENTS Core Quick REQUIRED)

add_executable(quick-tile-view-tests
  flextilelayouter_test.cpp
  main.cpp
)

target_link_libraries(quick-tile-view-tests PRIVATE
  Qt6::Core
  Qt6::Quick
  gtest
  quick-tiler
)

gtest_discover_tests(quick-tile-view-tests)
