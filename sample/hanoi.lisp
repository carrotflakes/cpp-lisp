;;;; tower of hanoi

(def hanoi
    (\ (n a b c)
       (if (< 0 n)
           (do (hanoi (- n 1) a b c)
               (print "move " n " from " a " to " c "\n")
               (hanoi (- n 1) b c a)))))

; (hanoi 3 "towerA" "towerB" "towerC")
