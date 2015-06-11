# cpp-lisp
Toy LISP implementation for c++0x.

## Special forms
- `if`
- `quote`
- `setq`
- `\` a.k.a. lambda.

## Built-in functions
- `do` Evaluations all arguments sequentially and returns last evaluation value.
- `+`
- `-`
- `*`
- `=`
- `<`
- `print`
- `car`
- `cdr`
- `cons`
- `eval`

## Samples

### Fibonacci
```
(setq fib
  (\ (n)
     (if (< 1 n)
         (+ (fib (- n 1)) (fib (- n 2)))
         n)))
(print (fib 10))
; 55
```
