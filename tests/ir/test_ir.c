#include "../helpers/test_helpers.h"
#include "../helpers/test_support.h"

#include <unistd.h>

static size_t countUnconditionalBranchesToBlock(const IRFunction *function, unsigned int target_block_id) {
    size_t block_index;
    size_t instruction_index;
    size_t count = 0;

    if (function == NULL) {
        return 0;
    }

    for (block_index = 0; block_index < function->block_count; block_index++) {
        IRBasicBlock *block = function->blocks[block_index];
        for (instruction_index = 0; instruction_index < block->instruction_count; instruction_index++) {
            IRInstruction *instruction = block->instructions[instruction_index];
            if (instruction->kind == IR_INSTR_BR && instruction->data.br.target_block_id == target_block_id) {
                count++;
            }
        }
    }

    return count;
}

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

void test_ir_printer_preserves_sized_array_parameter_types_in_calls(void) {
    char *llvm = emitLlvmIrFromString(
        "func first(values::Array<int>[3]) -> int { ret values[1]; } "
        "func main() -> int { data::Array<int>[3] = {1, 2, 3}; ret first(data); }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "define i32 @first([3 x i32] %p1)") != NULL);
        EXPECT_TRUE(strstr(llvm, "call i32 @first([3 x i32]") != NULL);
        EXPECT_TRUE(strstr(llvm, "[0 x i32]") == NULL);
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

void test_ir_printer_emits_float_values_as_double(void) {
    char *llvm = emitLlvmIrFromString("ratio::float = 1.5; func main() -> float { ret ratio; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "@ratio = global double 1.500000") != NULL);
        EXPECT_TRUE(strstr(llvm, "define double @main()") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_bool_values_as_i1(void) {
    char *llvm = emitLlvmIrFromString("flag::bool = true; func main() -> bool { ret flag; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "@flag = global i1 1") != NULL);
        EXPECT_TRUE(strstr(llvm, "define i1 @main()") != NULL);
        EXPECT_TRUE(strstr(llvm, "ret i1") != NULL);
        free(llvm);
    }
}

void test_ir_printer_escapes_special_characters_in_string_literals(void) {
    char *llvm = emitLlvmIrFromString("func main() -> void { printString(\"line\\nquote\\\"tab\\t\\\\\"); ret; }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "line\\5Cnquote\\5C\\22tab\\5Ct\\5C\\5C") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_sub_mul_and_div_operations(void) {
    char *llvm = emitLlvmIrFromString(
        "func math(a::int, b::int, c::int) -> int { ret (a - b) * c / 2; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "sub i32") != NULL);
        EXPECT_TRUE(strstr(llvm, "mul i32") != NULL);
        EXPECT_TRUE(strstr(llvm, "sdiv i32") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_default_return_for_empty_function(void) {
    char *llvm = emitLlvmIrFromString("func zero(flag::bool) -> int { if (flag) { ret 1; } }");

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "define i32 @zero(i1 %p1)") != NULL);
        EXPECT_TRUE(strstr(llvm, "ret i32 0") != NULL);
        free(llvm);
    }
}

void test_ir_printer_writes_module_to_file(void) {
    IRModule *module = buildIrModuleFromString("func main() -> void { printInt(1); ret; }");
    char pathTemplate[] = "/tmp/pocscript-ir-XXXXXX.ll";
    int fd = mkstemps(pathTemplate, 3);
    char *written;
    bool ok;

    EXPECT_TRUE(fd >= 0);
    EXPECT_TRUE(module != NULL);
    if (fd < 0 || module == NULL) {
        if (fd >= 0) {
            close(fd);
            unlink(pathTemplate);
        }
        if (module != NULL) {
            irModuleFree(module);
        }
        return;
    }

    close(fd);
    ok = irPrintModuleToFile(module, pathTemplate);
    written = readFileToString(pathTemplate);

    EXPECT_TRUE(ok);
    EXPECT_TRUE(written != NULL);
    if (written != NULL) {
        EXPECT_TRUE(strstr(written, "call void @printInt") != NULL);
    }

    free(written);
    irModuleFree(module);
    unlink(pathTemplate);
}

void test_ir_printer_emits_missing_comparison_variants(void) {
    char *llvm = emitLlvmIrFromString(
        "func main(a::int, b::int) -> bool { if (a >= b) { ret a == b; } ret a != b; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "icmp sge") != NULL);
        EXPECT_TRUE(strstr(llvm, "icmp eq") != NULL);
        EXPECT_TRUE(strstr(llvm, "icmp ne") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_unary_negation_and_not(void) {
    char *llvm = emitLlvmIrFromString(
        "func main(flag::bool, value::int) -> int { if (!flag) { ret -value; } ret value; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "xor i1") != NULL);
        EXPECT_TRUE(strstr(llvm, "sub i32 0,") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_logical_and_or(void) {
    char *llvm = emitLlvmIrFromString(
        "func main(a::bool, b::bool, c::bool) -> bool { if (a && b) { ret a || c; } ret false; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "and i1") != NULL);
        EXPECT_TRUE(strstr(llvm, "or i1") != NULL);
        EXPECT_TRUE(strstr(llvm, "ret i1") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_multi_index_array_access(void) {
    char *llvm = emitLlvmIrFromString(
        "func main() -> int { grid::int[2][3] = {{1, 2, 3}, {4, 5, 6}}; ret grid[1][2]; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "getelementptr") != NULL);
        EXPECT_TRUE(strstr(llvm, "[3 x [2 x i32]]") != NULL);
        EXPECT_TRUE(strstr(llvm, ", i32 1, i32 2") != NULL);
        free(llvm);
    }
}

void test_ir_printer_initializes_nested_array_literals_element_by_element(void) {
    char *llvm = emitLlvmIrFromString(
        "func main() -> int { grid::int[2][2] = {{1, 2}, {3, 4}}; ret grid[1][0]; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "alloca [2 x [2 x i32]]") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i32 1") != NULL);
        EXPECT_TRUE(strstr(llvm, "store i32 4") != NULL);
        free(llvm);
    }
}

void test_ir_printer_emits_break_and_continue_in_while(void) {
    IRModule *module = buildIrModuleFromString(
        "func main() -> int { i::int = 0; while (i < 5) { i += 1; if (i == 2) { continue; } if (i == 4) { break; } } ret i; }"
    );

    EXPECT_TRUE(module != NULL);
    if (module != NULL) {
        IRFunction *main_function = module->functions[0];
        unsigned int cond_block_id = main_function->blocks[1]->id;
        unsigned int end_block_id = main_function->blocks[3]->id;

        EXPECT_TRUE(countUnconditionalBranchesToBlock(main_function, cond_block_id) >= 2);
        EXPECT_TRUE(countUnconditionalBranchesToBlock(main_function, end_block_id) >= 1);
        irModuleFree(module);
    }
}

void test_ir_printer_emits_break_and_continue_in_for(void) {
    IRModule *module = buildIrModuleFromString(
        "func main() -> int { total::int = 0; for (i::int = 0; i < 5; i += 1) { if (i == 1) { continue; } if (i == 4) { break; } total += i; } ret total; }"
    );

    EXPECT_TRUE(module != NULL);
    if (module != NULL) {
        IRFunction *main_function = module->functions[0];
        unsigned int update_block_id = main_function->blocks[3]->id;
        unsigned int end_block_id = main_function->blocks[4]->id;

        EXPECT_TRUE(countUnconditionalBranchesToBlock(main_function, update_block_id) >= 2);
        EXPECT_TRUE(countUnconditionalBranchesToBlock(main_function, end_block_id) >= 1);
        irModuleFree(module);
    }
}

void test_ir_printer_inferrs_unsized_array_length_from_literal(void) {
    char *llvm = emitLlvmIrFromString(
        "func main() -> int { arr::Array<int> = {1, 2, 4}; ret 0; }"
    );

    EXPECT_TRUE(llvm != NULL);
    if (llvm != NULL) {
        EXPECT_TRUE(strstr(llvm, "alloca [3 x i32]") != NULL);
        EXPECT_TRUE(strstr(llvm, "alloca [0 x i32]") == NULL);
        free(llvm);
    }
}
