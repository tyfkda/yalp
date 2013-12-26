(def instructions
    '((HALT ())
      (VOID $next)
      (CONST (v . $next))
      (LREF (n . $next))
      (FREF (n . $next))
      (GREF (sym . $next))
      (LSET (n . $next))
      (FSET (n . $next))
      (GSET (sym . $next))
      (DEF (sym . $next))
      (PUSH $next)
      (TEST ($then . $else))
      (CLOSE (nparam nfree $body . $next))
      (FRAME ($cont . $ret))
      (APPLY (n))
      (RET ())
      (TAPPLY (n))
      (BOX (n . $next))
      (UNBOX $next)
      (CONTI (tail . $next))
      (MACRO (name nparam nfree $body . $next))
      (EXPND (n . $next))
      (SHRNK (n . $next))
      (VALS (n . $next))
      (RECV (n . $next))
      (NIL $next)
      ))

(def *opcode-table* (table))
(dolist (inst instructions)
  (table-put! *opcode-table* (car inst)
              (map [is (char-at (string _) 0) #\$]
                   (cadr inst))))

(def (vm-walker code f)
  (let1 h (write/ss-construct code)
    (nwith recur (code code)
      (let1 index (table-get h code)
        (when (or (no index) (>= index 0))
          (table-put! h code -1)
          (let1 operands (table-get *opcode-table* (car code))
            (when (f code recur)
              (awith (p operands
                      c (cdr code))
                (if (pair? p)
                    (do (when (car p)
                          (recur (car c)))
                        (loop (cdr p) (cdr c)))
                  (when p
                    (recur c)))))))))))

(def (files-or-stdin args f)
  (if args
      (dolist (filename args)
        (let1 ss (open filename)
          (f ss)
          (close ss)))
    (f *stdin*)))
