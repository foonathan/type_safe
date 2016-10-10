# Copyright (C) 2016 Jonathan Müller <jonathanmueller.dev@gmail.com>
# This file is subject to the license terms in the LICENSE file
# found in the top-level directory of this distribution.

cmake_minimum_required(VERSION 3.1)
project(TYPE_SAFE)

include(external/external.cmake)

# options
option(TYPE_SAFE_ENABLE_ASSERTIONS "whether or not to enable assertions for the type_safe library" ON)
if(${TYPE_SAFE_ENABLE_ASSERTIONS})
    set(_type_safe_enable_assertions 1)
else()
    set(_type_safe_enable_assertions 0)
endif()

option(TYPE_SAFE_ENABLE_WRAPPER "whether or not the wrappers in types.hpp are used" OFF)
if(${TYPE_SAFE_ENABLE_WRAPPER})
    set(_type_safe_enable_wrapper 1)
else()
    set(_type_safe_enable_wrapper 0)
endif()

# interface target
set(detail_header_files
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/assert.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/detail/force_inline.hpp)
set(header_files
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/boolean.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/constrained_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/floating_point.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/integer.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/narrow_cast.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/optional.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/strong_typedef.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/type_safe/types.hpp)

add_library(type_safe INTERFACE)
target_sources(type_safe INTERFACE ${detail_header_files} ${header_files})
target_include_directories(type_safe INTERFACE include/)
target_compile_definitions(type_safe INTERFACE
                                     TYPE_SAFE_ENABLE_ASSERTIONS=${_type_safe_enable_assertions}
                                     TYPE_SAFE_ENABLE_WRAPPER=${_type_safe_enable_wrapper})
target_link_libraries(type_safe INTERFACE debug_assert)

# other subdirectories
# only add if not inside add_subdirectory()
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    add_subdirectory(example/)
    add_subdirectory(test/)
endif()
