# Copyright (C) 2016-2017 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

find_package(debug_assert)
if(debug_assert_FOUND)
    set(TYPE_SAFE_HAS_IMPORTED_TARGETS On)
else()
    set(TYPE_SAFE_HAS_IMPORTED_TARGETS Off)
    execute_process(COMMAND git submodule update --init -- external/debug_assert
                    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/external/debug_assert)
endif()