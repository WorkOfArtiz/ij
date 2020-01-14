# IJ Compiler

A small compiler written in C++ to compile ij code to Tannenbaum IJVM (both assembly and machine code) and x86_64.

## ij compiler

The current mode has both compile and run options, in which compiles compiles ij code to 
jas, ijvm or raw amd64 instructions (mostly for debug purposes as ELF support isn't here yet).

```
Usage: ij {compile,run} [options] in.ij
```

To compile run `ij compile`:

```
Usage: ij compile [options] in.ij
       ij c       [options] in.ij
          compiles the sources to jas/ijvm, options:

          -o, --output   - output file (stdout by default)
          -f, --format {jas, ijvm, x64}
                         - which output format, default=jas
          -v, --verbose  - prints verbose info
          -d, --debug    - prints debug info
```

To JIT-compile and run:

```
Usage: ij run [options] in.ij
       ij r   [options] in.ij
    jit compiles the sources to x64 and executes them, options:

    -i, --input    - IN reads from file instead of stdin
    -o, --output   - OUT writes to file instead of stdout
    -v, --verbose  - prints verbose info
    -d, --debug    - prints debug info
```

## ij format

ij has constants through the following syntax:

```
constant x = 3;
```

As for functions, the compiler as of right now supports 2 kinds: full ij functions
and ij jas functions (the IJVM assembler format).

### ij pure functions

This is the main target, meant for making complex functions.

```
function <name>(<arg_list>)
{
  <stmt*>
}

with:
  name     := [a-zA-Z_]\w*
  arg_list := [<name> [',' <name>]*]
  stmt     := 
    'return' <expr> ';'
    'var' <name> ['=' <expr>] ';'
    'for' '(' [<expr>] ';' [<logic_expr>] ';' [<expr>] ')' '{' <stmt>* '}'
    'if' '(' <logic_expr> ')' '{' <stmt>* '}' ['else' '{' <stmt>* '}']
  
  logic_op  := '==' | '!=' | '<' | '>' | '<=' | '>='
  logic_expr:= <expr> <logic_op> <expr> | <expr> 
  op        := '+' | '-' | '&' | '|' | '+=' | '-=' | '&=' | '|='
  expr      := \d+ | 0x[\da-fA-F]+ | <func_call> | <name> | '(' <expr> ')' |
               <expr> <op> <expr> (with right precedence ofc)
```

An example is given in `tests/mul.ij`

```
function mul(x, y)
{
  var i;
  var result = 0;
  var bit = 1;
  var sign = 0;

  if (y < 0)
  {
    y = -y;
    sign = 1;
  }

  for (i = 0; i < top_bit; i += 1)
  {
    if (bit & y)
      result += x;

    x += x;
    bit += bit;
  }

  if (sign)
    result = -result;

  return result;
}
```

### IJ jas functions

IJ jas functions have a `"jas"` token after the declaration to signify the jas
function.

These functions support var declarations and JAS instructions, both of which 
have to be closed with a semi-colon. (labels unsupported for now)

```
function getc() jas 
{
  IN;
  IRETURN;
}
```

# Optimization

A lot has to be done here still, as of now, the code output isn't that efficient... 
Just readable. 

Originally I had planned to compile to IR, optimize IR, and compile to another
backend. (roughly the way LLVM does it)

Also note, the reason this isn't just an LLVM backend is that it's not fairly 
difficult outputting LLVM to a stack based architecture, especially one as limited
as IJVM.
