#pragma once

int readFromFile(const char* pathname, char** buf);
void writeResultsToFile(const char* dirname, char** content, int* sizes, int dim);