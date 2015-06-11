# cpp-lisp
Toy LISP implementation for c++0x.

## Object types
- Symbol
- Cons
- Int
- String
- Func
- BuiltinFunc

## Special forms
- `if`
- `quote`
- `set!`
- `\` a.k.a. `lambda`.

## Built-in functions
- `do` Evaluations all arguments sequentially and returns last evaluation value.
- `+`
- `-`
- `*`
- `=` Compares some Int values. If these values are equivalent mutually, it returns symbol `t`.
- `<`
- `print`
- `car`
- `cdr`
- `cons`
- `eval`
- `read`

## Samples

### Fibonacci
```
(set! fib
  (\ (n)
     (if (< 1 n)
         (+ (fib (- n 1)) (fib (- n 2)))
         n)))
(print (fib 10))
; 55
```
