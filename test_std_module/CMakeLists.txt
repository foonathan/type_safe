# Copyright (C) 2016-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

add_executable(type_safe_test_std_module test.cpp)
target_link_libraries(type_safe_test_std_module PUBLIC type_safe)
target_include_directories(type_safe_test_std_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Hopefully MS will provide a better/automatic/IJW way to use the standard library module
if( MSVC )
    target_sources(type_safe_test_std_module PUBLIC "C:/Program Files/Microsoft Visual Studio/2022/Preview/VC/Tools/MSVC/14.35.32019/modules/std.ixx")
endif()