(set! first
  (\ (xs) (car xs)))

(set! second
  (\ (xs) (car (cdr xs))))

(set! nth
  (\ (n xs)
    (if (< 0 n)
        (nth (- n 1) (cdr xs))
        (car xs))))

(set! length
  (\ (xs)
    (if (cons? xs)
        (+ 1 (length (cdr xs)))
        0)))

(set! last
  (\ (xs)
    (if (cons? xs)
        (if (cons? (cdr xs))
            (last (cdr xs))
            (car xs))
        nil)))

(set! butlast
  (\ (xs)
    (if (cons? xs)
        (if (cons? (cdr xs))
            (cons (car xs) (butlast (cdr xs)))
            ())
        nil)))

(set! list (\ xs xs))

; list*
(let (f ())
  (set! f
    (\ (xs)
      (if (cons? xs)
          (if (cons? (cdr xs))
              (cons (car xs) (f (cdr xs)))
              (car xs))
          nil)))
  (set! list*
    (\ xs (f xs))))

; append
(let (f ())
  (set! f
    (\ (xs ys)
      (if (cons? ys)
          (cons (car ys) (f xs (cdr ys)))
          (if (cons? xs)
              (f (cdr xs) (car xs))
              ()))))
  (set! append
    (\ xs (f xs ()))))

; reverse
(let (rev ())
  (set! rev
        (\ (xs ys)
           (if (nil? xs)
               ys
               (rev (cdr xs) (cons (car xs) ys)))))
  (set! reverse
        (\ (xs) (rev xs ()))))

(set! map-single
      (\ (func xs)
         (if (cons? xs)
             (cons (func (car xs)) (map-single func (cdr xs)))
             xs)))


;;; macros
(set! when
      (macro (condition . body)
             (cons (quote if)
                   (cons condition
                         (cons (cons (quote do)
                                     body)
                               ())))))
(set! unless
      (macro (condition . body)
             (cons (quote if)
                   (cons condition
                         (cons ()
                               (cons (cons (quote do)
                                           body)
                                     ()))))))
; cond case and or
