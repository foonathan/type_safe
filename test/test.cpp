// Copyright (C) 2016-2020 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#if defined(TYPE_SAFE_IMPORT_STD_MODULE)
import std;

// Catch header when CATCH_CONFIG_MAIN uses a lot of types from the global namespace
#include "float.h"
#include "math.h"
#include "stdio.h"
#include "stdint.h"
#include "time.h"
#endif



#define CATCH_CONFIG_MAIN
#include <catch.hpp>
