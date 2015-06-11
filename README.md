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
- `print` Prints argument objects. No newline.
- `println` Prints argument objects with newlines.
- `print-to-string`
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
(println (fib 10))
; 55
```

### Tower of Hanoi
```
(set! hanoi
  (\ (n a b c)
     (if (< 0 n)
         (do (hanoi (- n 1) a b c)
             (print "move " n " from " a " to " c "\n")
             (hanoi (- n 1) b c a)))))
(hanoi 3 "towerA" "towerB" "towerC")
;move 1 from towerA to towerC
;move 2 from towerA to towerC
;move 1 from towerB to towerA
;move 3 from towerA to towerC
;move 1 from towerB to towerA
;move 2 from towerB to towerA
;move 1 from towerC to towerB
```
