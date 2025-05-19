#pragma once

#if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4505)  // unreferenced local function
#elif defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "3rdparty/stb_image.h"

#if defined(_MSC_VER)
#    pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#    pragma GCC diagnostic pop
#endif
