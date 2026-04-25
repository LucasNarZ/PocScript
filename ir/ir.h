#ifndef IR_H
#define IR_H

#include <stdbool.h>
#include <stddef.h>

#include "../parser/ast.h"
#include "../semantic/semantic.h"

typedef struct IRType IRType;
typedef struct IRLiteral IRLiteral;
typedef struct IROperand IROperand;
typedef struct IRInstruction IRInstruction;
typedef struct IRBasicBlock IRBasicBlock;
typedef struct IRFunction IRFunction;
typedef struct IRGlobal IRGlobal;
typedef struct IRSymbol IRSymbol;
typedef struct IRSymbolTable IRSymbolTable;
typedef struct IRScope IRScope;
typedef struct IRModule IRModule;
typedef struct IRBuilder IRBuilder;

typedef struct {
    IRBasicBlock *break_target;
    IRBasicBlock *continue_target;
} IRLoopTarget;

typedef struct {
    unsigned int local_id;
    char *name;
    IRType *type;
} IRParameter;

typedef enum {
    IR_TYPE_VOID,
    IR_TYPE_INT,
    IR_TYPE_FLOAT,
    IR_TYPE_CHAR,
    IR_TYPE_BOOL,
    IR_TYPE_STRING,
    IR_TYPE_ARRAY,
    IR_TYPE_POINTER
} IRTypeKind;

typedef enum {
    IR_LITERAL_INT,
    IR_LITERAL_FLOAT,
    IR_LITERAL_STRING,
    IR_LITERAL_BOOL
} IRLiteralKind;

typedef enum {
    IR_INSTR_CONST,
    IR_INSTR_GLOBAL_ADDR,
    IR_INSTR_ALLOCA,
    IR_INSTR_LOAD,
    IR_INSTR_STORE,
    IR_INSTR_GEP,
    IR_INSTR_ADD,
    IR_INSTR_SUB,
    IR_INSTR_MUL,
    IR_INSTR_DIV,
    IR_INSTR_NEG,
    IR_INSTR_CMP_EQ,
    IR_INSTR_CMP_NE,
    IR_INSTR_CMP_LT,
    IR_INSTR_CMP_LE,
    IR_INSTR_CMP_GT,
    IR_INSTR_CMP_GE,
    IR_INSTR_NOT,
    IR_INSTR_BR,
    IR_INSTR_COND_BR,
    IR_INSTR_RET,
    IR_INSTR_CALL,
    IR_INSTR_CAST
} IRInstructionKind;

typedef enum {
    IR_OPERAND_LOCAL,
    IR_OPERAND_PARAM,
    IR_OPERAND_GLOBAL,
    IR_OPERAND_LITERAL,
    IR_OPERAND_BLOCK
} IROperandKind;

typedef enum {
    IR_SYMBOL_GLOBAL,
    IR_SYMBOL_LOCAL,
    IR_SYMBOL_FUNCTION
} IRSymbolKind;

struct IRType {
    IRTypeKind kind;
    IRType *element_type;
    size_t array_size;
    bool has_array_size;
};

struct IRLiteral {
    IRLiteralKind kind;
    union {
        long int_value;
        double float_value;
        char *string_value;
        bool bool_value;
    } data;
};

struct IROperand {
    IRType *type;
    IROperandKind kind;
    union {
        unsigned int local_id;
        unsigned int param_id;
        unsigned int global_id;
        IRLiteral *literal;
        unsigned int block_id;
    } data;
};

typedef struct {
    IROperand value;
    IROperand address;
    IRType *type;
    bool has_value;
    bool has_address;
} IRValue;

typedef struct {
    IROperand value;
} IRConstInstr;

typedef struct {
    IROperand operand;
} IRUnaryInstr;

typedef struct {
    IROperand left;
    IROperand right;
} IRBinaryInstr;

typedef struct {
    IROperand address;
} IRLoadInstr;

typedef struct {
    IROperand address;
    IROperand value;
} IRStoreInstr;

typedef struct {
    IRType *allocated_type;
} IRAllocaInstr;

typedef struct {
    IROperand base;
    IROperand *indices;
    size_t index_count;
} IRGepInstr;

typedef struct {
    unsigned int global_id;
} IRGlobalAddrInstr;

typedef struct {
    unsigned int target_block_id;
} IRBrInstr;

typedef struct {
    IROperand condition;
    unsigned int then_block_id;
    unsigned int else_block_id;
} IRCondBrInstr;

typedef struct {
    bool has_value;
    IROperand value;
} IRRetInstr;

typedef struct {
    char *callee;
    IROperand *args;
    size_t arg_count;
} IRCallInstr;

typedef struct {
    IROperand value;
    IRType *target_type;
} IRCastInstr;

typedef struct {
    IRType *return_type;
    IRType **param_types;
    size_t param_count;
} IRSymbolFunctionData;

struct IRSymbol {
    char *name;
    IRSymbolKind kind;
    IRType *type;
    union {
        unsigned int global_id;
        unsigned int local_id;
        IRSymbolFunctionData function;
    } data;
    IRSymbol *next;
};

struct IRSymbolTable {
    IRSymbol **buckets;
    size_t bucket_count;
};

struct IRScope {
    IRScope *parent;
    IRSymbolTable *symbols;
};

struct IRInstruction {
    IRInstructionKind kind;
    IRType *result_type;
    bool has_result;
    unsigned int result_id;
    union {
        IRConstInstr const_instr;
        IRUnaryInstr unary;
        IRBinaryInstr binary;
        IRLoadInstr load;
        IRStoreInstr store;
        IRAllocaInstr alloca;
        IRGepInstr gep;
        IRGlobalAddrInstr global_addr;
        IRBrInstr br;
        IRCondBrInstr cond_br;
        IRRetInstr ret;
        IRCallInstr call;
        IRCastInstr cast;
    } data;
};

struct IRBasicBlock {
    unsigned int id;
    IRInstruction **instructions;
    size_t instruction_count;
    size_t instruction_capacity;
};

struct IRFunction {
    char *name;
    IRType *return_type;
    IRScope *scope;
    IRParameter *params;
    size_t param_count;
    IRBasicBlock **blocks;
    size_t block_count;
    size_t block_capacity;
    unsigned int next_local_id;
    unsigned int next_block_id;
};

struct IRGlobal {
    char *name;
    unsigned int id;
    IRType *type;
    IRLiteral *initializer;
    bool is_constant;
    bool has_storage_global;
    unsigned int storage_global_id;
};

struct IRModule {
    IRScope *global_scope;
    IRGlobal **global_items;
    size_t global_count;
    size_t global_capacity;
    IRFunction **functions;
    size_t function_count;
    size_t function_capacity;
};

struct IRBuilder {
    AstNode *program;
    const SemanticResult *semantic;
    IRModule *module;
    IRFunction *current_function;
    IRBasicBlock *current_block;
    IRScope *current_scope;
};

IRType *irTypeCreate(IRTypeKind kind);
IRType *irTypeCreateArray(IRType *element_type);
IRType *irTypeCreateSizedArray(IRType *element_type, size_t array_size);
IRType *irTypeCreatePointer(IRType *element_type);
IRType *irTypeClone(const IRType *type);
void irTypeFree(IRType *type);

IRLiteral *irLiteralCreateInt(long value);
IRLiteral *irLiteralCreateFloat(double value);
IRLiteral *irLiteralCreateString(const char *value);
IRLiteral *irLiteralCreateBool(bool value);
void irLiteralFree(IRLiteral *literal);

IROperand irOperandCreateLocal(IRType *type, unsigned int local_id);
IROperand irOperandCreateParam(IRType *type, unsigned int param_id);
IROperand irOperandCreateGlobal(IRType *type, unsigned int global_id);
IROperand irOperandCreateLiteral(IRType *type, IRLiteral *literal);
IROperand irOperandCreateBlock(unsigned int block_id);
IRValue irValueEmpty(void);
void irOperandFree(IROperand *operand);

IRInstruction *irInstructionCreate(IRInstructionKind kind);
void irInstructionFree(IRInstruction *instruction);

IRBasicBlock *irBasicBlockCreate(unsigned int id);
bool irBasicBlockAppendInstruction(IRBasicBlock *block, IRInstruction *instruction);
void irBasicBlockFree(IRBasicBlock *block);

IRFunction *irFunctionCreate(const char *name, IRType *return_type, IRScope *parent_scope);
unsigned int irFunctionReserveLocalId(IRFunction *function);
unsigned int irFunctionReserveBlockId(IRFunction *function);
IRBasicBlock *irFunctionCreateBlock(IRFunction *function);
bool irFunctionAppendBlock(IRFunction *function, IRBasicBlock *block);
void irFunctionFree(IRFunction *function);

IRGlobal *irGlobalCreate(const char *name, unsigned int id, IRType *type, IRLiteral *initializer);
void irGlobalFree(IRGlobal *global);

IRModule *irModuleCreate(void);
bool irModuleAppendGlobal(IRModule *module, IRGlobal *global);
bool irModuleAppendFunction(IRModule *module, IRFunction *function);
IRModule *irBuildModule(AstNode *program, const SemanticResult *semantic);
bool irPrintModuleToFile(const IRModule *module, const char *path);
char *irPrintModuleToString(const IRModule *module);
void irModuleFree(IRModule *module);

IRSymbol *irSymbolCreateGlobal(const char *name, IRType *type, unsigned int global_id);
IRSymbol *irSymbolCreateLocal(const char *name, IRType *type, unsigned int local_id);
IRSymbol *irSymbolCreateFunction(const char *name, IRType *return_type, IRType **param_types, size_t param_count);
void irSymbolFree(IRSymbol *symbol);

IRSymbolTable *irSymbolTableCreate(size_t bucket_count);
void irSymbolTableFree(IRSymbolTable *table);
IRSymbol *irSymbolTableLookupCurrent(IRSymbolTable *table, const char *name);
bool irSymbolTableDeclare(IRSymbolTable *table, IRSymbol *symbol);

IRScope *irScopeCreate(IRScope *parent);
void irScopeFree(IRScope *scope);
IRSymbol *irScopeLookupCurrent(IRScope *scope, const char *name);
IRSymbol *irScopeLookup(IRScope *scope, const char *name);
bool irScopeDeclare(IRScope *scope, IRSymbol *symbol);

IRBuilder *irBuilderCreate(AstNode *program, const SemanticResult *semantic);
void irBuilderFree(IRBuilder *builder);

#endif
