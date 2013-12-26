(defmacro test (name expected actual)
  (w/uniq (v result)
    `(let1 ,v ,expected
       (format *stdout* "Testing %@ ... " ',name)
       (let1 ,result ,actual
         (if (iso ,v ,result)
             (print "ok")
           (do
             (format *stdout* "FAILED: %@ expected, but %@\n" ,v ,result)
             (exit 1)))))))

(def (test-section name)
  (format *stdout* "==== %@\n" name))

(def (test-complete)
  (display "\x1b[1;32mSUCCESS\x1b[0;39m\n"))
