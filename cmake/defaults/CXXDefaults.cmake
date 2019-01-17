
include(CXXHelpers)

if (CMAKE_COMPILER_IS_GNUCXX)
    #include(gccdefaults)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    #include(clangdefaults)
elseif(MSVC)
    # Disable warning C4996 regarding fopen(), strcpy(), etc.
    _add_define("_CRT_SECURE_NO_WARNINGS")

    # Disable warning C4996 regarding unchecked iterators for std::transform,
    # std::copy, std::equal, et al.
    _add_define("_SCL_SECURE_NO_WARNINGS")

    # Make sure WinDef.h does not define min and max macros which
    # will conflict with std::min() and std::max().
    _add_define("NOMINMAX")
endif()
