#pragma once

#include <data-types.h>

// basic vector types
#define GENERIC_TYPE float
#include <vector.h>
#define GENERIC_TYPE int
#include <vector.h>
#define GENERIC_TYPE char
#include <vector.h>

// vector_string with custom deallocator
#define GENERIC_TYPE string
#define CUSTOM_TYPE_DESTRUCTOR free_string
#include <vector.h>

// vector_request with custom deallocator
#define GENERIC_TYPE request
#define CUSTOM_TYPE_DESTRUCTOR free_request
#include <vector.h>