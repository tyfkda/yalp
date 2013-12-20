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
             (display "\n")
             (exit 1)))))))

(def (test-section name)
  (display "==== ")
  (print name))

(def (test-complete)
  (display "\x1b[1;32mSUCCESS\x1b[0;39m\n"))
