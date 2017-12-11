
include(CXXHelpers)

if (CMAKE_COMPILER_IS_GNUCXX)
    include(gccdefaults)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    include(clangdefaults)
elseif(MSVC)
    include(msvcdefaults)
endif()
