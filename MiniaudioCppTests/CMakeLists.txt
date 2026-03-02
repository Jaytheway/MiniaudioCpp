add_executable(MiniaudioCppTests)

# Sources
file(GLOB_RECURSE TEST_SRC
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/vendor/**.h"
)
target_sources(MiniaudioCppTests PRIVATE ${TEST_SRC})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${TEST_SRC})

# C++ version
target_compile_features(MiniaudioCppTests PRIVATE cxx_std_20)

# Includes
target_include_directories(MiniaudioCppTests PRIVATE
  "${CMAKE_SOURCE_DIR}/MiniaudioCpp/include"
  "${CMAKE_SOURCE_DIR}/MiniaudioCpp/vendor"
)


# Place the project in a folder in IDEs
set_target_properties(MiniaudioCppTests PROPERTIES FOLDER "Tests")

set_target_properties(MiniaudioCppTests PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:MiniaudioCppTests>"
)

# Fetch GoogleTest
if(NOT TARGET GTest::gtest)
    CPMFindPackage(
        NAME              GTest
        GITHUB_REPOSITORY google/googletest
        GIT_TAG           v1.14.0
        EXCLUDE_FROM_ALL  YES
        OPTIONS           "INSTALL_GTEST OFF"
    )
endif()
    
# Auto-discover gtest cases in the built binary
include(GoogleTest)
gtest_discover_tests(MiniaudioCppTests
    WORKING_DIRECTORY $<TARGET_FILE_DIR:MiniaudioCppTests>
    DISCOVERY_TIMEOUT 60
    DISCOVERY_MODE PRE_TEST
)

# Link libraries
target_link_libraries(MiniaudioCppTests PRIVATE
    JPL::MiniaudioCpp
    GTest::gtest
)

set_target_properties(gtest PROPERTIES FOLDER "Dependencies")

# Per-config defines
target_compile_definitions(MiniaudioCppTests PRIVATE
  JPL_TEST
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

# Ensure we build a console app
set_target_properties(MiniaudioCppTests PROPERTIES WIN32_EXECUTABLE OFF)

# Set output directories
jpl_set_output_dirs(MiniaudioCppTests)