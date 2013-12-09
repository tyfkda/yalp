(defmacro test (name expected actual)
  (w/uniq (v result)
    `(let ,v ,expected
       (display "Testing ")
       (display ',name)
       (display " ... ")
       (let ,result ,actual
         (if (iso ,v ,result)
             (print "ok")
           (do
             (display "FAILED: ")
             (write ,v)
             (display " expected, but ")
             (write ,result)
             (newline)
             (exit 1)))))))

(def (test-section name)
  (display "==== ")
  (print name))
