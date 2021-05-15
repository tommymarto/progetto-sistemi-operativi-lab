#pragma once

#include <data-structures.h>

vector_request* parseCommandLineArguments(int argc, char *argv[]);
void expandRequests(vector_request* old);