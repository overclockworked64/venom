# venom

An implementation of a minimal dynamic programming language. For now, the source code is tokenized, parsed into AST, and compiled to bytecode which is interpreted by a virtual machine. The plan is to have a mark-and-sweep garbage collector and to keep the VM RISC-like.

Status: WIP.

## Compiling

Clone the repository and run:

```
make
```

## Running the tests

Make a Python virtual environment, install `pytest`, and run:

```
pytest
```