add_library(MiniaudioCpp STATIC)
add_library(JPL::MiniaudioCpp ALIAS MiniaudioCpp)

jpl_enable_ipo()

# ---- MiniaudioCpp static library
file(GLOB_RECURSE MINIAUDIOCPP_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/**.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/miniaudio/miniaudio.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/dr_libs/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/c89atomic/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/choc/**.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/miniaudio/extras/stb_vorbis.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/miniaudio/miniuaudio.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/c89atomic.h"
)
# Exclude files that clash with JPLSpatial library
# if (TARGET JPLSpatial)
    # list(FILTER MINIAUDIOCPP_SOURCES EXCLUDE REGEX ".*Core.h$|.*ErrorReporting.h$|.*ErrorReporting.cpp$")
# endif()

target_sources(MiniaudioCpp PRIVATE ${MINIAUDIOCPP_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${MINIAUDIOCPP_SOURCES})

# C++ version
target_compile_features(MiniaudioCpp PUBLIC cxx_std_20)

target_include_directories(MiniaudioCpp PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/MiniaudioCpp"
)

target_include_directories(MiniaudioCpp PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/c89atomic"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/MiniAudioInterface"
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/choc"
)

target_compile_definitions(MiniaudioCpp
  PRIVATE
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    $<$<BOOL:${BUILD_TESTING}>:JPL_TESTS>
)

# Linux PIC
if(UNIX AND NOT APPLE)
  set_target_properties(MiniaudioCpp PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# x86/AVX2 vs x86/SSE2, skip on non-x86
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
  if(MSVC)
    target_compile_options(MiniaudioCpp PRIVATE /arch:AVX2)
  else()
    target_compile_options(MiniaudioCpp PRIVATE -mavx2 -mfma)
  endif()
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86|i[3-6]86)$")
  if(MSVC)
    target_compile_options(MiniaudioCpp PRIVATE /arch:SSE2)
  else()
    target_compile_options(MiniaudioCpp PRIVATE -msse2)
  endif()
endif()

# Set output directories
jpl_set_output_dirs(MiniaudioCpp)
