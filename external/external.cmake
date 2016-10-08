# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

execute_process(COMMAND git submodule update --init -- external/debug_assert
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
add_subdirectory(${CMAKE_SOURCE_DIR}/external/debug_assert)
