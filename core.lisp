(def not
  (\ (b) (nil? b)))

(def first
  (\ (xs) (car xs)))

(def second
  (\ (xs) (car (cdr xs))))

(def nth
  (\ (n xs)
    (if (< 0 n)
        (nth (- n 1) (cdr xs))
        (car xs))))

(def length
  (\ (xs)
    (if (cons? xs)
        (+ 1 (length (cdr xs)))
        0)))

(def last
  (\ (xs)
    (if (cons? xs)
        (if (cons? (cdr xs))
            (last (cdr xs))
            (car xs))
        nil)))

(def butlast
  (\ (xs)
    (if (cons? xs)
        (if (cons? (cdr xs))
            (cons (car xs) (butlast (cdr xs)))
            ())
        nil)))

(def list (\ xs xs))

; list*
(let (f ())
  (set! f
    (\ (xs)
      (if (cons? xs)
          (if (cons? (cdr xs))
              (cons (car xs) (f (cdr xs)))
              (car xs))
          nil)))
  (def list*
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
  (def append
    (\ xs (f xs ()))))

; reverse
(let (rev ())
  (set! rev
        (\ (xs ys)
           (if (nil? xs)
               ys
               (rev (cdr xs) (cons (car xs) ys)))))
  (def reverse
        (\ (xs) (rev xs ()))))

(def map-single
    (\ (func xs)
       (if (cons? xs)
           (cons (func (car xs)) (map-single func (cdr xs)))
           xs)))

(def tree-eq?
    (\ (obj-1 obj-2)
       (if (eq? obj-1 obj-2)
           t
           (if (cons? obj-1)
               (if (cons? obj-2)
                   (if (tree-eq? (car obj-1) (car obj-2))
                       (tree-eq? (cdr obj-1) (cdr obj-2))))))))


;;; macros
(def when
  (macro (condition . body)
    (cons (quote if)
          (cons condition
                (cons (cons (quote do)
                            body)
                      ())))))
(def unless
  (macro (condition . body)
    (cons (quote if)
          (cons condition
                (cons ()
                      (cons (cons (quote do)
                                  body)
                            ()))))))

(def cond
  (macro clauses
    (if (cons? clauses)
        (list (quote if)
              (car (car clauses))
              (cons (quote do) (cdr (car clauses)))
              (list* (quote cond) (cdr clauses)))
        ())))

(def and
  (macro forms
    (if (cons? forms)
        (if (cons? (cdr forms))
            (list (quote if)
                  (car forms)
                  (list* (quote and) (cdr forms))
                  nil)
            (car forms))
        t)))

; or
(let* (f
       (\ (forms sym)
         (if (cons? forms)
             (list (quote if)
                   (list (quote set!) sym (car forms))
                   sym
                   (f (cdr forms) sym))
             nil)))

  (def or
      (macro forms
             (let (sym (gensym))
               (list (quote let)
                     (list sym
                           ())
                     (f forms sym))))))


; quasiquote
; e.g.
;   in Common Lisp : `(a ,b ,@c)
;   in cpp-lisp    : (qquote (a (unq b) (unqs c)))
(let* (qqe
       (\ (form)
         (cond
           ((and (cons? form) (eq? (car form) (quote unq)))
            (car (cdr form)))
           ((quotable? form)
            (list (quote quote) form))
           ((listable? form)
            (list* (quote list) (map-single qqe form)))
           (t
            (list* (quote append)
                   (append-elts form ())))))
       append-elts
       (\ (forms list-forms)
         (if (cons? forms)
             (if (and (cons? (car forms))
                      (eq? (car (car forms)) (quote unqs)))
                 (if (cons? list-forms)
                     (cons (qqe (reverse list-forms))
                           (cons (car (cdr (car forms)))
                                 (append-elts (cdr forms) ())))
                     (cons (car (cdr (car forms)))
                           (append-elts (cdr forms) ())))
                 (append-elts (cdr forms) (cons (car forms) list-forms)))
             (if (cons? list-forms)
                 (list (qqe (reverse list-forms)))
                 ())))
       quotable?-aux
       (\ (form)
         (if (cons? form)
             (and (quotable? (car form))
                  (quotable?-aux (cdr form)))
             t))
       quotable?
       (\ (form)
         (if (cons? form)
             (if (or (eq? (car form) (quote unq))
                     (eq? (car form) (quote unqs)))
                 nil
                 (and (quotable? (car form))
                      (quotable?-aux (cdr form))))
             t))
       listable?
       (\ (form)
         (if (cons? form)
             (and (if (cons? (car form))
                      (not (eq? (car (car form)) (quote unqs)))
                      t)
                  (listable? (cdr form)))
             t)))

  (def qquote
      (macro (form)
             (qqe form))))

(def defn
    (macro (name lambda-list . body)
           (qquote (def (unq name)
                       (\ (unq lambda-list)
                          (unqs body))))))

(def defm
    (macro (name lambda-list . body)
           (qquote (def (unq name)
                       (macro (unq lambda-list)
                              (unqs body))))))

(defm push! (value xs)
  (qquote (set! (unq xs) (cons (unq value) (unq xs)))))

(defm pop! (xs)
  (let (s (gensym))
    (qquote (let ((unq s) (car (unq xs)))
              (set! (unq xs) (cdr (unq xs)))
              (unq s)))))
