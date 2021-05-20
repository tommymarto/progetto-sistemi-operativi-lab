#pragma once

#include <data-structures.h>
#include <request.h>

// vector_request with custom deallocator
#define GENERIC_TYPE request
#define CUSTOM_TYPE_DESTRUCTOR free_request
#include <vector.h>