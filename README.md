## Overview
This loader is designed to load programs from object files and execute loaded programs in user-space memory. Since all programs are located and executed in user-space memory, we are able to avoid the overhead of process creation and destruction overhead. Programs are cached for better performance. 

Current implementation supports compiled C-programs without dynamic linking and compiled output of `wasm2c`. The current implementation is designed to run on single core and programs are executed sequentially. The current implementation runs on x86-64. 

## Loading and Executing Program
Each program supplied to the loader has to have a function named `main`, which takes in two argument `argc` and `argv`. Some example programs are provided in [code example](code_example/). To execute a program with the loader, do
```
./trial add-new.o main.o
```

The loader will allocate memory for code and data sections of the object files and update the code sections based on relocation records of the object file. Parameters are loaded into register `$esi` and `$edi` following calling conventions of x86-64. Then, the loader jumps to `main` throw inline assembly.

##Performance
