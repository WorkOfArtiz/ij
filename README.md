# IJ Compiler

A small compiler written in C++ to compile ij code to Tannenbaum IJVM.

## ij compiler

```
Usage: ij [options] in.ij
   -o, --output   - output file (stdout by default)
   -S, --assembly - generates jas assembly, compiled otherwise
   -v, --verbose  - prints verbose info
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
    y = 0 - y;
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
    result = 0 - result;

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