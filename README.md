# cpp-lisp
Toy LISP implementation for c++0x.

## Object types
- Symbol
- Cons
- Int
- String
- Proc
- BuiltinProc

## Special forms
- `if`
- `quote`
- `set!`
- `let` e.g. `(let (a 1 b 2) (+ a b))` => `3`
- `\` a.k.a. `lambda`.

## Built-in functions
- `nil?`
- `list?`
- `symbol?`
- `int?`
- `string?`
- `proc?`
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
; move 1 from towerA to towerC
; move 2 from towerA to towerC
; move 1 from towerB to towerA
; move 3 from towerA to towerC
; move 1 from towerB to towerA
; move 2 from towerB to towerA
; move 1 from towerC to towerB
```

### Lexical scope
```
(set! count 10)
(set! count-up
	(let (count 0)
     (\ () (set! count (+ count 1)) count)))
(println (count-up) (count-up) (count-up))
; 1
; 2
; 3
(println count)
; 10
```
