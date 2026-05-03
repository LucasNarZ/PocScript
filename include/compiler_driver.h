#ifndef COMPILER_DRIVER_H
#define COMPILER_DRIVER_H

#include <stdbool.h>

bool compileSourceFileToLlvmIr(const char *input_path, const char *output_path);
bool compileSourceStringToLlvmIr(const char *input, const char *output_path);

#endif
