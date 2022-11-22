# Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

add_executable(type_safe_test_std_module test.cpp ${CMAKE_SOURCE_DIR}/cmake/type_safe_standard_library_module.cmake)
target_link_libraries(type_safe_test_std_module PUBLIC type_safe)
target_include_directories(type_safe_test_std_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

type_safe_add_std_module(TARGET type_safe_test_std_module)
