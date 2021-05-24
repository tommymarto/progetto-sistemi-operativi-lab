#pragma once

#include <data-structures.h>

vector_string* char_array_split(char* char_arr, int size, const char* delim);
vector_string* string_split(string* self, const char* delim);

// converts str to unsigned integers. returns true if successful, false otherwise
bool strToInt(const char* src, long* result);