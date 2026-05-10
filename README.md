# nacc - not a c compiler

A hand-written C subset compiler targeting AArch64 assembly, built from scratch in 3 months. Compiles a subset of C all the way down to a native binary on a Raspberry Pi 4B (or any AArch64 machine).

No AI code, compiler frameworks, or parser generators were used. I wrote every stage, every line, in pure C code. :)

---

## pipeline

```
prog.c -> lexer -> parser -> sema -> tac -> codegen_aarch64 -> prog.s -> gcc -> binary
```

| stage | what it does |
|---|---|
| lexer | tokenizes source |
| parser | LL(1) recursive descent, builds AST |
| sema | scope/type checking, annotates AST |
| tac | walks AST, emits three-address code |
| codegen_aarch64 | walks TAC, emits AArch64 assembly |

---

## what it supports

- types: `int`, `char`, `int*`, `char*`, `void`, `void*`
- arrays: `int[]`, `char[]`
- control flow: `if/else`, `while`, `for`
- functions: declaration, definition, recursion
- operators: arithmetic, comparison, logical, `++`/`--`, `+=` `-=` `*=` `/=`
- pointers: `&`, `*`, dereference assignment
- I/O: `printf`/`scanf` via libc
- global/local variables

## what it doesn't support

- structs, enums, unions
- multi-dimensional arrays
- `switch`, `do-while`
- preprocessor (`#include`, `#define`)
- floats

## known limitations
- **no optimization:** code generation is done through naive stack spilling, no register allocation or liveness analysis (I want to work on this soon as a new project)
- **no array bounds checking:** out of bounds access is undefined behavior
- **single file only:** no multi-file compilation or linking
- **first error only:** compilation stops at the first error encountered
- **AArch64 Linux only:** output targets Linux ELF AArch64

---

## build & run

```bash
make
./nacc prog.c        #outputs prog.s
gcc prog.s -o prog  #assemble + link
./prog
```

---

## resources & AI use

**Textbook:** Cooper, Keith D., and Linda Torczon. *Engineering a Compiler*. 3rd ed., Morgan Kaufmann, 2022. Used throughout my CS4713 Compiler Construction course at UTSA and as a reference in this project for lexer/parser/IR design.

**AI:** All code was written by me. I used Claude strictly as a design/brainstorming tool for design decisions and concept explanations.
