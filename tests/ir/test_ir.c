#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"

void test_ir_builder_creates_module_for_empty_program(void) {
    IRModule *module = buildIrModuleFromString("");

    EXPECT_TRUE(module != NULL);
    if (module != NULL) {
        EXPECT_TRUE(module->global_count == 0);
        EXPECT_TRUE(module->function_count == 0);
        irModuleFree(module);
    }
}

void test_ir_builder_predeclares_global_and_function_symbols(void) {
    IRModule *module = buildIrModuleFromString(
        "greeting::string = \"hi\";\n"
        "func main() -> int { ret 1; }"
    );

    EXPECT_TRUE(module != NULL);
    if (module != NULL) {
        EXPECT_TRUE(module->global_count >= 1);
        EXPECT_TRUE(module->function_count == 1);
        EXPECT_TRUE(irScopeLookup(module->global_scope, "main") != NULL);
        irModuleFree(module);
    }
}

void test_ir_printer_emits_private_string_global(void) {
    char *llvm = emitLlvmIrFromString("func main() -> void { msg::string = \"hi\"; ret; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "private unnamed_addr constant [3 x i8] c\"hi\\00\"") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_add_and_return(void) {
    char *llvm = emitLlvmIrFromString("func sum(a::int, b::int) -> int { ret a + b; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "define i32 @sum(") != NULL);
        EXPECT_TRUE(strstr(llvm, "add i32") != NULL);
        EXPECT_TRUE(strstr(llvm, "ret i32") != NULL);
        free(llvm);
    }
}

void test_ir_printer_lowers_compound_assignment_through_load_compute_store(void) {
    char *llvm = emitLlvmIrFromString("func main() -> void { x::int = 1; x += 2; x -= 1; ret; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "load i32") != NULL);
        EXPECT_TRUE(strstr(llvm, "add i32") != NULL);
        EXPECT_TRUE(strstr(llvm, "sub i32") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i32") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_conditional_branches_and_labels(void) {
    char *llvm = emitLlvmIrFromString(
        "func main() -> int { x::int = 1; if (x > 0) { ret x; } else { ret 0; } }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "br i1") != NULL);
        EXPECT_TRUE(strstr(llvm, "bb") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_gep_for_array_access(void) {
    char *llvm = emitLlvmIrFromString(
        "func main() -> int { arr::int[3] = {1, 2, 3}; ret arr[1]; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "getelementptr") != NULL);
        EXPECT_TRUE(strstr(llvm, "[3 x i32]") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_function_call(void) {
    char *llvm = emitLlvmIrFromString(
        "func add(a::int, b::int) -> int { ret a + b; } "
        "func main() -> int { ret add(1, 2); }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "call i32 @add") != NULL);
        free(llvm);
    }
}

void test_ir_builder_predeclares_builtin_runtime_functions(void) {
    IRModule *module = buildIrModuleFromString("func main() -> void { printInt(1); ret; }");

    EXPECT_TRUE(module != NULL);
    if (module != NULL) {
        EXPECT_TRUE(irScopeLookup(module->global_scope, "printInt") != NULL);
        EXPECT_TRUE(irScopeLookup(module->global_scope, "printString") != NULL);
        irModuleFree(module);
    }
}

void test_ir_printer_emits_runtime_function_declarations(void) {
    char *llvm = emitLlvmIrFromString("func main() -> void { printString(\"hi\"); printInt(1); ret; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "declare void @printString(i8*)") != NULL);
        EXPECT_TRUE(strstr(llvm, "declare void @printInt(i32)") != NULL);
        EXPECT_TRUE(strstr(llvm, "call void @printString") != NULL);
        EXPECT_TRUE(strstr(llvm, "call void @printInt") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_loop_comparisons_and_branches(void) {
    char *llvm = emitLlvmIrFromString(
        "func main() -> int { i::int = 0; while(i <= 2) { i += 1; } for(j::int = 0; j < 2; j += 1) { i += j; } ret i; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "icmp sle") != NULL);
        EXPECT_TRUE(strstr(llvm, "icmp slt") != NULL);
        EXPECT_TRUE(strstr(llvm, "br label %bb") != NULL);
        free(llvm);
    }
}

void test_ir_printer_reads_and_writes_global_variables(void) {
    char *llvm = emitLlvmIrFromString(
        "x::int = 1; flag::bool = true; "
        "func main() -> int { x += 2; if (flag) { ret x; } ret 0; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "@x = global i32 1") != NULL);
        EXPECT_TRUE(strstr(llvm, "load i32, i32* @x") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i32") != NULL);
        EXPECT_TRUE(strstr(llvm, "load i1, i1* @flag") != NULL);
        free(llvm);
    }
}
