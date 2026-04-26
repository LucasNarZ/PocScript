#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "errors.h"
#include "scope.h"
#include "types.h"
#include "ast.h"

typedef struct {
    Scope *global_scope;
    SemanticErrorList errors;
} SemanticResult;

SemanticResult semanticAnalyze(AstNode *program);
void semanticResultFree(SemanticResult *result);

#endif
