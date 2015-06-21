# cpp-lisp
Toy LISP implementation for c++0x.

## Object types
- Symbol
- Cons
- Int
- String
- Proc
- BuiltinProc
- Macro

## Special forms
- `if`
- `do` Evaluations all arguments sequentially and returns last evaluation value.
- `quote`
- `def` Creates a variable binding on global. e.g. `(def first (\ (list) (car list)))`
- `set!` Rebinds a variable to a value.
- `let` e.g. `(let (a 1 b 2) (+ a b))` => `3`
- `let*`
- `\` a.k.a. `lambda`.
- `macro` e.g. `(def set-nil (macro (a) (cons (quote set!) (cons a (cons nil ()))))) (set-nil foo) (println foo)` => `nil`

## Built-in functions
- `eq?`
- `nil?`
- `cons?`
- `list?`
- `symbol?`
- `int?`
- `string?`
- `proc?`
- `+`
- `-`
- `*`
- `/`
- `mod`
- `=` Compares some Int values. If these values are equivalent mutually, it returns symbol `t`.
- `<`
- `print` Prints argument objects. No newline.
- `println` Prints argument objects with newlines.
- `print-to-string`
- `car`
- `cdr`
- `cons`
- `gensym`
- `bound?`
- `get-time`
- `eval`
- `read` Reads S-expression from standard input.
- `load` Receives a file name as String and evaluates the lisp code in the file.
- `macroexpand-all`

## Standard functions and macros
Some useful functions and macros are available immediately on LISP start. These are defined in `core.lisp` file.


## Examples

### Fibonacci
```
(def fib
  (\ (n)
     (if (< 1 n)
         (+ (fib (- n 1)) (fib (- n 2)))
         n)))
(println (fib 10))
; 55
```

### Lexical scope and closure
```
(def count-up
	   (let (count 0) ; This 'count' is a lexical variable.
          (\ () (set! count (+ count 1)) count)))
(println (count-up) (count-up) (count-up))
; 1
; 2
; 3
(println (bound? (quote count)))
; nil
```

### Dynamic scope
```
(def count 10) ; This 'count' is a special variable.
(def count-up
     (\ () (set! count (+ count 1)) count))
(println (count-up) (count-up) (count-up))
; 11
; 12
; 13
(let (count 0)
		 (println (count-up) (count-up) (count-up)))
; 1
; 2
; 3
```

### Variadic function
```
(def my-list (\ xs xs))
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

### FizzBuzz
```
(set! fizz-buzz
  (\ (n)
    (when (< 0 n)
      (do (fizz-buzz (- n 1))
          (println (cond
                     ((= (mod n 15) 0) "FizzBuzz")
                     ((= (mod n 3)  0) "Fizz")
                     ((= (mod n 5)  0) "Buzz")
                     (t                n)))))))
```
