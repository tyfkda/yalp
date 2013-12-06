;;; Ambiguous calculation.
(def (fail)
  (error 'no-solution))

(def (in-range a b)
  (call/cc
   (^(cont)
     (enumerate a b cont))))

(def (enumerate a b cont)
  (if (> a b)
      (fail)
    (let save fail
      (set! fail
            (^()
              (set! fail save)
              (enumerate (+ a 1) b cont)))
      (cont a))))

(print
 (with (x (in-range 2 9)
        y (in-range 2 9)
        z (in-range 2 9))
   (if (is (+ (* x x) (* y y))
           (* z z))
       (list x y z)
     (fail))))
