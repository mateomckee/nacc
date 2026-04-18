# nacc — not a c compiler

A source-to-executable compiler for a subset of C, targeting the AArch64 instruction set.
Built from scratch for CS4713 Compiler Construction at UTSA.

Every stage of the pipeline is hand-written, no parser generators, no compiler frameworks.

```
source.c → [lexer] → [parser] → [sema] → [irgen] → [codegen] → output.s → gcc → binary
```

---

## pipeline

| stage | input | output |
|---|---|---|
| lexer | source file | token stream |
| parser | token stream | AST |
| semantic analysis | AST | annotated AST |
| IR generation | annotated AST | three-address code |
| code generation | TAC | AArch64 assembly |

---

## supported C subset

**types:** `int`, `char`, `int*`, `char*`, `void`

**control flow:** `if`/`else`, `while`, `for`

**functions:** declaration, definition, recursion

**expressions:** arithmetic, comparison, logical, assignment, `++`/`--`, `+=`, `-=`, `*=`, `/=`

**variables:** local and global

**I/O:** `printf` and `scanf` via libc linking

**not supported:** structs, unions, enums, `switch`, `do-while`, preprocessor directives (`#include`, `#define`), floating point

---

## dependencies

- `gcc` — used to assemble and link the output `.s` file
- standard C library (libc) — linked automatically by gcc

No other dependencies. nacc itself is written in C and builds with Make.

### installing gcc (if not already installed)

**macOS:**
```bash
xcode-select --install
```

**Ubuntu/Debian:**
```bash
sudo apt install gcc
```

**Raspberry Pi (Raspberry Pi OS):**
```bash
sudo apt install gcc
```

---

## building nacc

```bash
git clone https://github.com/mateomckee/nacc.git
cd nacc
make
```

This produces the `nacc` binary in the project root.

---

## usage

**compile a C file:**
```bash
./nacc input.c
```

This emits `output.s` in the current directory.

**assemble and link with gcc:**
```bash
gcc output.s -o program
```

**run:**
```bash
./program
```

---

## example

```bash
./nacc tests/fibonacci.c
gcc output.s -o fib
./fib
```

---

## project structure

```
nacc/
├── src/
│   ├── main.c
│   ├── lexer.h / lexer.c       — hand-written scanner, produces token stream
│   ├── parser.h / parser.c     — LL(1) recursive descent parser, produces AST
│   ├── sema.h / sema.c         — semantic analysis, scope-stack symbol table
│   ├── ir.h / ir.c             — three-address code IR generation
│   ├── codegen.h / codegen.c   — AArch64 assembly emission
│   └── util.h / util.c         — error reporting, memory helpers
├── tests/
│   ├── fibonacci.c
│   ├── factorial.c
│   ├── calculator.c
│   └── ...
└── Makefile
```

---

## design

nacc is an ahead-of-time compiler, it translates source to AArch64 assembly at compile
time, producing a native binary via GCC's assembler and linker.

**lexer** — hand-written character-by-character scanner. O(n), no regex engine.

**parser** — hand-written LL(1) recursive descent. One token of lookahead, O(n).
Each production rule in the grammar corresponds directly to a function in parser.c.

**semantic analysis** — AST walk with a scope-stack symbol table. Catches undeclared
variables, redeclarations, and basic type errors.

**IR** — three-address code. Each instruction has at most one operator and three operands.
Temporaries generated sequentially (t0, t1, ...), labels for control flow (L0, L1, ...).

**code generation** — walks the TAC instruction list and emits AArch64 assembly.
Naive stack-based allocation — all locals and temporaries spill to the stack.
libc linked automatically by gcc, enabling printf/scanf without custom implementation.

---

## grammar (LL(1))

```
program      -> function*
function     -> type IDENT '(' params ')' block
params       -> (param (',' param)*)?
param        -> type IDENT
block        -> '{' stmt* '}'
stmt         -> if_stmt | while_stmt | for_stmt | return_stmt | decl_stmt | expr_stmt
if_stmt      -> 'if' '(' expr ')' block ('else' block)?
while_stmt   -> 'while' '(' expr ')' block
for_stmt     -> 'for' '(' for_init expr ';' expr ')' block
return_stmt  -> 'return' expr? ';'
decl_stmt    -> type IDENT ('=' expr)? ';'
expr_stmt    -> expr ';'
type         -> ('int' | 'char' | 'void') '*'?
expr         -> assign
assign       -> IDENT '=' assign | or
or           -> and ('||' and)*
and          -> equality ('&&' equality)*
equality     -> compare (('==' | '!=') compare)*
compare      -> add (('<' | '>' | '<=' | '>=') add)*
add          -> mul (('+' | '-') mul)*
mul          -> unary (('*' | '/') unary)*
unary        -> ('-' | '!' | '*' | '&') unary | primary
primary      -> INT_LIT | CHAR_LIT | STRING_LIT | IDENT '(' args ')' | IDENT | '(' expr ')'
args         -> (expr (',' expr)*)?
```

---

## test programs

```bash
./nacc tests/fibonacci.c && gcc output.s -o fib && ./fib
./nacc tests/factorial.c && gcc output.s -o fact && ./fact
./nacc tests/calculator.c && gcc output.s -o calc && ./calc
```

---

## known limitations

- no preprocessor — `#include` and `#define` not supported
- no floating point — `float` and `double` not supported
- no structs, unions, or enums
- no arrays (pointer arithmetic supported)
- no standard library beyond `printf` and `scanf`
- naive register allocation — all variables spill to stack

---

## a note on AI assistance

This project was built as a learning exercise in compiler construction. All implementation
was written by hand, every line of C code is my own. I used AI (Claude) as a design and
brainstorming tool to discuss architecture decisions, understand concepts, and think through
edge cases, similar to how one might use a textbook or a colleague.
No code was generated by AI. This README.md was generated by AI, using my hand-written design document.

---

*Mateo McKee · CS4713 Compiler Construction · UTSA*  
*aah279 · mateo.mckee@my.utsa.edu*
