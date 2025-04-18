# PocScript

**PocScript** is a *work-in-progress* compiled programming language created for educational purposes. The goal of this project is **not** to implement advanced features like classes, structs, dictionaries, or complex types — the main objective is to understand and build the **core structure of a programming language** from scratch.

---

## 🧠 Project Structure

PocScript consists of three main components (excluding syntax analysis):


#### **Lexer ➝ Parser ➝ Assembly Generator**


### 🔤 Lexer
The **lexer** reads the input file and converts it into a linked list of tokens, each categorized (e.g., keyword, operator, identifier). This is done using **regex pattern matching**.

### 🌳 Parser
The **parser** takes the token list from the lexer and builds an **AST (Abstract Syntax Tree)**, representing the grammatical structure of the program.  

PocScript uses a **Top-Down Recursive Descent Parser**, where each grammar rule is implemented as a function. This approach makes the structure more readable and easier to debug during development.

### ⚙️ Assembly Generator
The final step takes the AST and converts it to **x86 assembly** (requires an assembler like NASM to run the generated code). This part is still under development.

---

## 📈 Development Progress

### ✅ Lexer
- [x] Operators  
- [x] Keywords  
- [x] Delimiters  
- [x] Assignment Operators  
- [x] Integers  
- [x] Identifiers  
- [ ] Character and String literals  
- [ ] Floating-point numbers  

### ✅ Parser
- [x] Variable declarations  
- [x] Basic arithmetic and logic expressions  
- [x] `if` statements  
- [x] `while` loops  
- [x] `for` loops  
- [x] Functions  
- [x] Primitive types  
- [ ] Variable scopes  
- [ ] `+=` operator  
- [ ] Arrays  
- [ ] Parentheses in expressions  

### 🚧 Assembly Generator
- [ ] Not started yet

---

## 📌 Notes
This project is intended for **learning purposes**, so the implementation favors simplicity and clarity over performance or completeness. Contributions, feedback, or suggestions are welcome!
