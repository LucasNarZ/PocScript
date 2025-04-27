#include "generator.h"

char *operators[10] = {"-", "+", "*", "/", "=", "if", "else", "else if", "while", "for"};
char *argsRegisters[4] = {"rdi", "rsi", "rcx", "rdx"};
char *variableTypes[3] = {"NUMBER", "BOOL", "CHAR"};
char *skip = "skip";
char *strValue = "strValue";
int stringIndex = 0;
int skipIndex = 0;
int functionIndex = 0;

void writeAtLine(const char *text, char **lines, LineIndices *lineIndices, int lineIndex){
    lines[lineIndex] = malloc(MAX_LINE_LEN);
    snprintf(lines[lineIndex], MAX_LINE_LEN, "%s", text);
    lineIndices->currentLine++;
}

void writeBaseFile(char **lines, LineIndices *lineIndices){
    writeAtLine("; Assembly Program", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    writeAtLine("section .data", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    writeAtLine("section .bss", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    intBuffer resb 20", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    writeAtLine("section .text", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    global _start", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    // end label
    writeAtLine("end:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rax, 60", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    xor rdi, rdi", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    syscall", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    // strlen function
    writeAtLine("strlen:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    xor rcx, rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(".next:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    cmp byte [rsi + rcx], 0", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    je .done", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    inc rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    jmp .next", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(".done:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rdx, rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    ret", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    // print function
    writeAtLine("printString:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    push rax", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    push rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rsi, rdi", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rax, 1", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rdi, 1", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    call strlen", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    syscall", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    pop rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    pop rax", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    ret", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    writeAtLine("printInt:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rcx, intBuffer + 19", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rax, rdi", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rbx, 10", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    cmp rax, 0", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    jne .convert", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov byte [rcx], '0'", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    dec rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    jmp .done", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(".convert:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(".loop:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    xor rdx, rdx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    div rbx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    add dl, '0'", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    dec rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov [rcx], dl", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    cmp rax, 0", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    jne .loop", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(".done:", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rdi, 1", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rax, 1", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rsi, rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rdx, intBuffer + 19", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    sub rdx, rcx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    syscall", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    ret", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("", lines, lineIndices, lineIndices->currentLine);

    // _start
    writeAtLine("_start:", lines, lineIndices, lineIndices->currentLine);
}

void writeGlobalVariable(Node *node, const char *value, char **lines, LineIndices *lineIndices){
    char *buffer = malloc(MAX_LINE_LEN);
    snprintf(buffer, MAX_LINE_LEN, "    %s %s", node->value, value);

    for (int i = lineIndices->currentLine; i > lineIndices->globalVariblesLine; i--) {
        lines[i] = lines[i - 1];
    }

    writeAtLine("", lines, lineIndices, lineIndices->globalVariblesLine);
    writeAtLine(buffer, lines, lineIndices, lineIndices->globalVariblesLine);
    lineIndices->globalVariblesLine++;
    lineIndices->currentLine--;

    free(buffer);

}

void writeComparison(const char *label, char **lines, LineIndices *lineIndices){
    char *buffer1 = malloc(MAX_LINE_LEN);
    char *buffer2 = malloc(MAX_LINE_LEN);
    char *buffer3 = malloc(MAX_LINE_LEN);
    char *buffer4 = malloc(MAX_LINE_LEN);
    skipIndex++;
    snprintf(buffer1, MAX_LINE_LEN, "skip%d:", skipIndex);
    snprintf(buffer2, MAX_LINE_LEN, "    %s skip%d",label, skipIndex);
    skipIndex++;
    snprintf(buffer3, MAX_LINE_LEN, "skip%d:", skipIndex);
    snprintf(buffer4, MAX_LINE_LEN, "    jmp skip%d", skipIndex);
    
    writeAtLine("    pop rbx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    pop rax", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    cmp rax, rbx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(buffer2, lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    mov rax, 1", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(buffer4, lines, lineIndices, lineIndices->currentLine);
    writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    xor rax, rax", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(buffer3, lines, lineIndices, lineIndices->currentLine);
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(buffer4);
}

void writeOperator(const char *operator, char **lines, LineIndices *lineIndices){
    char *buffer = malloc(MAX_LINE_LEN);
    snprintf(buffer, MAX_LINE_LEN, "    %s rax, rbx", operator);
    writeAtLine("    pop rbx", lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    pop rax", lines, lineIndices, lineIndices->currentLine);
    writeAtLine(buffer, lines, lineIndices, lineIndices->currentLine);
    writeAtLine("    push rax", lines, lineIndices, lineIndices->currentLine);
    free(buffer);
}

void writeAssignString(char *name, char *value, int size, char **lines, LineIndices *lineIndices){
    char *buffer = malloc(MAX_LINE_LEN);

    writeAtLine("    mov rcx, TAMANHO_MAX_STRING", lines, lineIndices, lineIndices->currentLine++);
    writeAtLine("    mov rdi, addr_string", lines, lineIndices, lineIndices->currentLine++);
    writeAtLine("    xor rax, rax", lines, lineIndices, lineIndices->currentLine++);
    writeAtLine("    rep stosb", lines, lineIndices, lineIndices->currentLine++);

    for(int i = 0; i < size;i++){
        snprintf(buffer, MAX_LINE_LEN, "    mov [%s + %d], '%c'", name, i, value[i]);
        writeAtLine(buffer, lines, lineIndices, lineIndices->currentLine);
    }
}

void writeAssignGlobalVaribleInt(const char *name, const char *value, char **lines, LineIndices *lineIndices){
    char *buffer = malloc(MAX_LINE_LEN);

    snprintf(buffer, MAX_LINE_LEN, "    mov qword [%s], %s", name, value);
    writeAtLine(buffer, lines, lineIndices, lineIndices->currentLine);
    free(buffer);
}

void writeFile(const char *filename, char **lines, LineIndices *lineIndices, char **functionLines, LineIndices *functionLineIndices){
    FILE *file = fopen(filename, "w");
    if(file == NULL){
        fprintf(stderr, "Error opening file %s\n", filename);
        return;
    }

    for(int i = 1; i < lineIndices->currentLine; i++){
        fprintf(file, "%s\n", lines[i]);
        free(lines[i]);
    }
    fprintf(file, "%s\n", "");
    for(int i = 1; i < functionLineIndices->currentLine; i++){
        fprintf(file, "%s\n", functionLines[i]);
        free(functionLines[i]);
    }
    fclose(file);
}

void writeExpression(Node *node, char **lines, LineIndices *lineIndices){
    
    for(int i = 0; i < node->numChildren; i++){
        writeExpression(node->children[i], lines, lineIndices);
    }

    if(arrayContains(variableTypes, 3, node->description)){
        char *buffer = malloc(MAX_LINE_LEN);
        snprintf(buffer, MAX_LINE_LEN, "    push %s", node->value);
        writeAtLine(buffer, lines, lineIndices, lineIndices->currentLine);
        free(buffer);
    }else if(strcmp(node->description, "IDENTIFIER") == 0){
        char *buffer = malloc(MAX_LINE_LEN);
        snprintf(buffer, MAX_LINE_LEN, "    push qword [%s]", node->value);
        writeAtLine(buffer, lines, lineIndices, lineIndices->currentLine);
        free(buffer);
    }else if(strcmp(node->value, "+") == 0){
        writeOperator("add", lines, lineIndices);
    }else if(strcmp(node->value, "-") == 0){
        writeOperator("sub", lines, lineIndices);
    }else if(strcmp(node->value, "*") == 0){
        writeOperator("imul", lines, lineIndices);
    }else if(strcmp(node->value, "/") == 0){
        writeOperator("div", lines, lineIndices);
    }else if(strcmp(node->value, "&&") == 0){
        writeOperator("and", lines, lineIndices);
    }else if(strcmp(node->value, "||") == 0){
        writeOperator("or", lines, lineIndices);
    }else if(strcmp(node->value, "==") == 0){
        writeComparison("jne", lines, lineIndices);
    }else if(strcmp(node->value, "!=") == 0){
        writeComparison("je", lines, lineIndices);
    }else if(strcmp(node->value, ">=") == 0){
        writeComparison("jl", lines, lineIndices);
    }else if(strcmp(node->value, "<=") == 0){
        writeComparison("jg", lines, lineIndices);
    }else if(strcmp(node->value, "<") == 0){
        writeComparison("jge", lines, lineIndices);
    }else if(strcmp(node->value, ">") == 0){
        writeComparison("jle", lines, lineIndices);
    }
}

void walkTree(Node *node, char **lines, LineIndices *lineIndices, char **functionLines, LineIndices *functionLineIndices){
    if(node == NULL) return;

    if(strcmp(node->value, "=") == 0){
        char *buffer = malloc(MAX_LINE_LEN);
        if(node->children[0]->numChildren != 0){
            if(strcmp(node->children[0]->children[0]->value, "char") == 0){
                snprintf(buffer, MAX_LINE_LEN, "db %s, 0x0A, 0", node->children[1]->value);
                writeGlobalVariable(node->children[0], buffer, lines, lineIndices);
            }else{
                snprintf(buffer, MAX_LINE_LEN, "dq %s", node->children[1]->value);
                writeExpression(node->children[1], lines, lineIndices);
                writeAtLine("    pop rax", lines, lineIndices, lineIndices->currentLine);
                writeAssignGlobalVaribleInt(node->children[0]->value, "rax", lines, lineIndices);
                writeGlobalVariable(node->children[0], "dq 0", lines, lineIndices);
            }
            Node *type = NULL;

            type = createNode(node->children[0]->children[0]->value, node->children[0]->value);
            node = allocNode(node, type);
            
            defineVariables(node->children[0]->value, type, &stack, &scopesStack);
        }else{
            if(strcmp(getVarType(node->children[0]->value, &stack), "char") == 0){
                writeAssignString(node->children[0]->value, node->children[1]->value, sizeof(node->children[0]->value), lines, lineIndices);
            }else{
                writeExpression(node->children[1], lines, lineIndices);
                writeAtLine("    pop rax", lines, lineIndices, lineIndices->currentLine);
                writeAssignGlobalVaribleInt(node->children[0]->value, "rax", lines, lineIndices);
            }
        }
        free(buffer);
    }else if(node->children != NULL && strcmp(node->children[0]->value, "CALL_ARGS") == 0){
        char *buffer = malloc(MAX_LINE_LEN);
        for(int i = 0;i < node->children[0]->numChildren;i++){
            Node *arg = node->children[0]->children[i];
            if(arrayContains(variableTypes, 3, arg->description) || strcmp(getVarType(arg->value, &stack), "char") == 0){
                snprintf(buffer, MAX_LINE_LEN, "    mov %s, %s", argsRegisters[i], arg->value);
            }else if(arrayContains(variableTypes, 3, arg->description) || strcmp(getVarType(arg->value, &stack), "char") == 0){
                snprintf(buffer, MAX_LINE_LEN, "    mov %s, %s", argsRegisters[i], arg->value);
            }else{
                snprintf(buffer, MAX_LINE_LEN, "    mov %s, [%s]", argsRegisters[i], arg->value);
            }
            writeAtLine(buffer, lines, lineIndices, lineIndices->currentLine);
        }
        snprintf(buffer, MAX_LINE_LEN, "    call %s", node->value);
        writeAtLine(buffer, lines, lineIndices, lineIndices->currentLine);
        free(buffer);
    }else if(strcmp(node->value, "if") == 0){
        writeExpression(node->children[0], lines, lineIndices);
        char *buffer1 = malloc(MAX_LINE_LEN);
        char *buffer2 = malloc(MAX_LINE_LEN);

        skipIndex++;
        int lastIndex = skipIndex;
        snprintf(buffer2, MAX_LINE_LEN, "    jne skip%d", lastIndex);
        writeAtLine("    cmp rax, 1", lines, lineIndices, lineIndices->currentLine);
        writeAtLine(buffer2, lines, lineIndices, lineIndices->currentLine);
        writeAtLine("    push rax", lines, lineIndices, lineIndices->currentLine);
        
        int i = 1;
        for(i; i < node->numChildren; i++){
            if(strcmp(node->children[i]->value, "else") == 0){
                writeAtLine("    pop rax", lines, lineIndices, lineIndices->currentLine);
                snprintf(buffer1, MAX_LINE_LEN, "skip%d:", lastIndex);
                writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
            }
            walkTree(node->children[i], lines, lineIndices, functionLines, functionLineIndices);
        }

        if(strcmp(node->children[i - 1]->value, "else") != 0){
            writeAtLine("    pop rax", lines, lineIndices, lineIndices->currentLine);
            snprintf(buffer1, MAX_LINE_LEN, "skip%d:", lastIndex);
            writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
        }

        
        
        free(buffer1);
        free(buffer2);
    }else if(strcmp(node->value, "else") == 0){
        char *buffer1 = malloc(MAX_LINE_LEN);
        char *buffer2 = malloc(MAX_LINE_LEN);

        skipIndex++;
        int lastIndex = skipIndex;
        

        snprintf(buffer2, MAX_LINE_LEN, "    jne skip%d", lastIndex);
        writeAtLine("    cmp rax, 0", lines, lineIndices, lineIndices->currentLine);
        writeAtLine(buffer2, lines, lineIndices, lineIndices->currentLine);
        
        int i = 0;
        for(i; i < node->numChildren; i++){
            walkTree(node->children[i], lines, lineIndices, functionLines, functionLineIndices);
        }
        
        snprintf(buffer2, MAX_LINE_LEN, "skip%d:", lastIndex);
        writeAtLine(buffer2, lines, lineIndices, lineIndices->currentLine);

        free(buffer1);
        free(buffer2);

    }else if(strcmp(node->value, "while") == 0){
        char *buffer1 = malloc(MAX_LINE_LEN);

        skipIndex++;
        int lastIndex1 = skipIndex;
        int lastIndex2 = skipIndex + 1;
        skipIndex++;

        writeExpression(node->children[0], lines, lineIndices);
        writeAtLine("    cmp rax, 1", lines, lineIndices, lineIndices->currentLine);
        snprintf(buffer1, MAX_LINE_LEN, "    jne skip%d", lastIndex2);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);

        snprintf(buffer1, MAX_LINE_LEN, "loop%d:", lastIndex1);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
        writeAtLine("", lines, lineIndices, lineIndices->currentLine);
        int i = 1;
        for(i; i < node->numChildren; i++){
            walkTree(node->children[i], lines, lineIndices, functionLines, functionLineIndices);
        }
        writeAtLine("", lines, lineIndices, lineIndices->currentLine);
        writeExpression(node->children[0], lines, lineIndices);

        
        writeAtLine("    cmp rax, 0", lines, lineIndices, lineIndices->currentLine);
        snprintf(buffer1, MAX_LINE_LEN, "    jne loop%d", lastIndex1);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
        snprintf(buffer1, MAX_LINE_LEN, "skip%d:", lastIndex2);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
    }else if(strcmp(node->value, "for") == 0){
        char *buffer1 = malloc(MAX_LINE_LEN);

        skipIndex++;
        int lastIndex1 = skipIndex;
        int lastIndex2 = skipIndex + 1;
        skipIndex++;

        walkTree(node->children[0], lines, lineIndices, functionLines, functionLineIndices);

        writeExpression(node->children[2], lines, lineIndices);
        writeAtLine("    cmp rax, 1", lines, lineIndices, lineIndices->currentLine);
        snprintf(buffer1, MAX_LINE_LEN, "    jne skip%d", lastIndex2);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);

        snprintf(buffer1, MAX_LINE_LEN, "loop%d:", lastIndex1);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
        writeAtLine("", lines, lineIndices, lineIndices->currentLine);
        int i = 3;
        for(i; i < node->numChildren; i++){
            walkTree(node->children[i], lines, lineIndices, functionLines, functionLineIndices);
        }
        walkTree(node->children[2], lines, lineIndices, functionLines, functionLineIndices);   
        writeAtLine("", lines, lineIndices, lineIndices->currentLine);   
        
        writeAtLine("", lines, lineIndices, lineIndices->currentLine);
        writeExpression(node->children[1], lines, lineIndices);
        writeAtLine("", lines, lineIndices, lineIndices->currentLine);

          
        writeAtLine("    cmp rax, 0", lines, lineIndices, lineIndices->currentLine);
        snprintf(buffer1, MAX_LINE_LEN, "    jne loop%d", lastIndex1);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
        snprintf(buffer1, MAX_LINE_LEN, "skip%d:", lastIndex2);
        writeAtLine(buffer1, lines, lineIndices, lineIndices->currentLine);
    }else if(strcmp(node->description, "DECLARATION") == 0 && strcmp(node->children[0]->value, "ARGS") == 0){
        char *buffer1 = malloc(MAX_LINE_LEN);
        functionIndex++;
        snprintf(buffer1, MAX_LINE_LEN, "%s:", node->value);
        writeAtLine(buffer1, functionLines, functionLineIndices, functionLineIndices->currentLine);
        writeAtLine("   push rbp", functionLines, functionLineIndices, functionLineIndices->currentLine);
        writeAtLine("   mov rbp, rsp", functionLines, functionLineIndices, functionLineIndices->currentLine);

        int i = 0;
        int offset = 4;
        for(i; i < node->children[0]->numChildren; i++){
            snprintf(buffer1, MAX_LINE_LEN, "   mov DWORD [rbp-%d], edi", offset);
            writeAtLine(buffer1, functionLines, functionLineIndices, functionLineIndices->currentLine);
            offset += 4;
        }
        
        writeAtLine("   pop rbp", functionLines, functionLineIndices, functionLineIndices->currentLine);
        writeAtLine("   ret", functionLines, functionLineIndices, functionLineIndices->currentLine);

        free(buffer1);
        printf("asdasd");
    }
    for(int i = 0; i < node->numChildren; i++){
        if(!arrayContains(operators, 10, node->value) && strcmp(node->children[0]->value ,"ARGS") != 0){
            walkTree(node->children[i], lines, lineIndices, functionLines, functionLineIndices);
        }
    }
}

void generateAssembly(Node *root, FILE *outputFile, LineIndices *lineIndices, LineIndices *functionLineIndices){
    if(root == NULL) return;

    char **lines = malloc(sizeof(char*) * MAX_LINES);
    char **functionLines = malloc(sizeof(char*) * MAX_LINES);

    outputFile = fopen("output.asm", "w");
    if(outputFile == NULL){
        fprintf(stderr, "Error opening outputFile \n");
        return;
    }

    writeBaseFile(lines, lineIndices);

    walkTree(root, lines, lineIndices, functionLines, functionLineIndices);

    writeAtLine("    call end", lines, lineIndices, lineIndices->currentLine);

    writeFile("output.asm", lines, lineIndices, functionLines, functionLineIndices);
    free(lines);
    free(functionLines);

    printf("Assembly code generated successfully.\n");
}

