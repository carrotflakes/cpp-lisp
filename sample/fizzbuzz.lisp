;;;; Fizz Buzz

(def fizz-buzz
    (\ (n)
       (when (< 0 n)
         (do (fizz-buzz (- n 1))
             (println (cond
                        ((= (mod n 15) 0) "FizzBuzz")
                        ((= (mod n 3)  0) "Fizz")
                        ((= (mod n 5)  0) "Buzz")
                        (t                n)))))))

; (fizz-buzz 20)
