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
- [x] Floating-point numbers  
- [x] Character and String literals  
- [x] Primitive Types
- [x] Comments

### ✅ Parser
- [x] Variable declarations  
- [x] Basic arithmetic and logic expressions  
- [x] `if` statements 
- [x] `else` statements
- [x] `else if` statements
- [x] `while` loops  
- [x] `for` loops  
- [x] Function declaration
- [x] Primitive types  
- [x] `+=` operator  
- [x] Variable scopes  
- [x] Vectors
- [x] Matrices 
- [x] Manage Array scope 
- [x] Parentheses in expressions  
- [x] Functions calls
- [x] `!` operator
- [x] `true` and `false` keywords
- [ ] Negative Numbers
- [x] Access array elements
- [x] Functions `ret` statement
- [ ] n-dimention Array(maybe)

### ✅ Assembly Generator
- [x] Generate `.asm` output file
- [x] Add `.data`, `.text`, and `_start` sections
- [ ] Declare scalar variables in `.data` (`int`, `float`, etc.)
- [x] Support variable initialization with literals
- [x] Simple assignment (`=`)
- [x] Generate code for number literals
- [x] Basic arithmetic operations (`+`, `-`, `*`, `/`)
- [x] Parentheses and precedence in expressions
- [x] Comparison operators (`==`, `!=`, `<`, `<=`, `>`, `>=`)
- [x] Logical operators (`&&`, `||`, `!`)
- [x] Default `print` function
- [x] Function Call
- [ ] Compound assignment (`+=`, `-=`, etc.)
- [ ] If statements (`if`)
- [ ] Else and else-if chaining (`else`, `else if`)
- [ ] While loops
- [ ] For loops
- [ ] Local Variable declaration
- [ ] Function declaration
- [ ] Support 1D arrays (declaration and access)
- [ ] Support 2D matrices (declaration and access)

---

## 🛠️ How to Build and Run

### 📦 Dependencies
To compile and run PocScript, make sure you have the following tools installed:

- `gcc` (GNU Compiler Collection)
- `make` (Build automation tool)
- `nasm` (The Netwide Assembler — for x86 assembly)

You can install them using your package manager. For example, on Debian/Ubuntu-based systems:

```bash
sudo apt update
sudo apt install build-essential nasm
```
### ⚙️ Building the Project

Clone the repository and run make in the project root:
```bash
git clone https://github.com/LucasNarZ/PocScript.git
cd PocScript
make
```

### 🧪 Running the Compiler

1. Create an input file with the extension .ps. For example:
```bash
echo "a::int = 2 + 3;" > input.ps
```
2. Run the compiler:
```bash
./compiler
```
3. (In future versions) The output will be an .asm file to be compiled with NASM.
## 📌 Notes
This project is intended for **learning purposes**, so the implementation favors simplicity and clarity over performance or completeness. Contributions, feedback, or suggestions are welcome!
