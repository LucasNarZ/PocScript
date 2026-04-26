#include "ast.h"
#include "ir.h"
#include "lexer.h"
#include "parser.h"
#include "semantic.h"
#include <stdio.h>

int main() {
  AstNode *root;
  Parser parser;
  SemanticResult semantic;
  Token *tokens = tokenizeFile("input.ps");
  IRModule *module = NULL;
  size_t i;

  if (tokens == NULL) {
    fprintf(stderr, "Error opening input.ps\n");
    return 1;
  }

  parserInit(&parser, tokens);
  root = parserParseProgram(&parser);
  semantic = semanticAnalyze(root);

  if (semantic.errors.count > 0) {
    printf("\n");
    for (i = 0; i < semantic.errors.count; i++) {
      SemanticError *error = &semantic.errors.items[i];

      if (error->line > 0 && error->column > 0) {
        printf("%s at line %d, column %d: %s\n",
               semanticErrorKindName(error->kind), error->line, error->column,
               error->message);
      } else {
        printf("%s: %s\n", semanticErrorKindName(error->kind), error->message);
      }
    }
  } else {
    module = irBuildModule(root, &semantic);
    if (module == NULL || !irPrintModuleToFile(module, "build/ir/IR.ll")) {
      remove("build/ir/IR.ll");
      fprintf(stderr, "IR generation failed\n");
      irModuleFree(module);
      semanticResultFree(&semantic);
      astFree(root);
      freeTokens(tokens);
      return 1;
    }
  }

  irModuleFree(module);
  semanticResultFree(&semantic);
  astFree(root);
  freeTokens(tokens);

  return 0;
}
