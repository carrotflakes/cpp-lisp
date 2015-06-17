;;;; Benchmark
;;;; measure execution times.
;;;;
;;;; Usage:
;;;;   > (load "sample/bench.lisp")
;;;;   t
;;;;   > (bench-all)
;;;;
;;;;   Evaluation took 840 ms of real time
;;;;
;;;;   Evaluation took 950 ms of real time
;;;;
;;;;   Evaluation took 5480 ms of real time
;;;;   nil


(defn fibonacci (n)
  (if (< 1 n)
      (+ (fibonacci (- n 1)) (fibonacci (- n 2)))
      n))

(defn bench-fib ()
    (time (fibonacci 20)))


(defn ackermann (m n)
  (cond
    ((= m 0) (+ n 1))
    ((= n 0) (ackermann (- m 1) 1))
    (t (ackermann (- m 1) (ackermann m (- n 1))))))

(defn bench-ack ()
  (time (ackermann 3 4)))


(defn tarai (x y z)
  (if (or (< x y) (= x y))
      y
      (tarai (tarai (- x 1) y z)
             (tarai (- y 1) z x)
             (tarai (- z 1) x y))))

(defn bench-tarai ()
  (time (tarai 9 5 0)))


(defn bench-all ()
  (bench-fib)
  (bench-ack)
  (bench-tarai))
