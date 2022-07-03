# venom

An implementation of a minimal dynamic programming language. For now, the source code is tokenized, parsed into AST using a recursive-descent parser, and compiled to bytecode which is interpreted by a virtual machine. The plan is to have a mark-and-sweep garbage collector and to keep the VM RISC-like.

Status: almost usable.

## Examples

Fibonacci:

```rust
fn fib(n) { 
    if (n == 0) {
        return 0;
    }
    if (n == 1) {
        return 1;
    }
    return fib(n-1) + fib(n-2);
}

print fib(20);
```

Fizzbuzz:

```rust
fn fizzbuzz() {
    let i = 0;
    while (i < 100) {
        if (i % 15 == 0) {
            print "fizzbuzz";
        } else if (i % 5 == 0) {
            print "buzz";
        } else if (i % 3 == 0) {
            print "fizz";
        } else {
            print i;
        }
        i = i + 1;
    }
}

fizzbuzz();
```

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