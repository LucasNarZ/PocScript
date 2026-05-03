#include "compiler_driver.h"

#include <stdio.h>

int main(int argc, char **argv) {
  const char *input_path = "input.ps";
  const char *output_path = "build/ir/IR.ll";

  if (argc >= 2) {
    input_path = argv[1];
  }

  if (argc >= 3) {
    output_path = argv[2];
  }

  if (argc > 3) {
    fprintf(stderr, "usage: %s [input.ps] [output.ll]\n", argv[0]);
    return 1;
  }

  return compileSourceFileToLlvmIr(input_path, output_path) ? 0 : 1;
}
