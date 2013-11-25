;; Macro hash table, (symbol => closure)
(set! *macro-table* (make-hash-table))

;; Compile (defmacro name (vars ...) bodies) syntax.
(set! register-macro
      (^(name closure)
        (hash-table-put! *macro-table* name closure)))

;; Whether the given name is macro.
(set! macro?
      (^(name)
        (hash-table-exists? *macro-table* name)))


(set! null
      (^(x)
        (if x
            nil
          t)))
(set! no
      (^(x)
        (if x
            nil
          t)))

(set! map
      (^(f ls)
        (if ls
            (cons (f (car ls))
                  (map f (cdr ls)))
            ())))

;; Make pair from a list. (a b c d e) -> ((a b) (c d) (e))
(set! pair
      (^(xs)
        (if (no xs)
            nil
          (no (cdr xs))
          (list (list (car xs)))
          (cons (list (car xs) (cadr xs))
                (pair (cddr xs))))))

(set! cadr (^(x) (car (cdr x))))
(set! cddr (^(x) (cdr (cdr x))))
(set! caadr (^(x) (car (cadr x))))

(defmacro quasiquote (x)
  ((^(loop)
     (set! loop (^(x)
                  (if (no (consp x))  (list 'quote (list x))
                    (is (car x) 'unquote)  (list 'list (cadr x))
                    (is (car x) 'unquote-splicing)  (cadr x)
                    (list 'list (cons 'append (map loop x))))))
     ((^(res)
        (if (is (car res) 'list) (cadr res)
          (if (is (car res) 'quote) (list 'quote (caadr res))
            (error "unexpected"))))
      (loop x)))
   nil))

(defmacro def (name value)
  `(set! ,name ,value))

(defmacro defn (name vars . body)
  `(set! ,name (^ ,vars ,@body)))

(defmacro with (parms . body)
  `((^ ,(map car (pair parms))
     ,@body)
    ,@(map cadr (pair parms))))

(defmacro let (var val . body)
  `((^(,var) ,@body) ,val))

(defmacro do body
  `((^() ,@body)))

(defmacro when (test . body)
  `(if ,test (do ,@body)))

(defmacro unless (test . body)
  `(if (not ,test) (do ,@body)))

(defmacro aif (expr . body)
  `(let it ,expr
     (if it
         ,@(if (cddr body)
               `(,(car body) (aif ,@(cdr body)))
               body))))

(defmacro w/uniq (names . body)
  (if (consp names)
      ; (w/uniq (a b c) ...) => (with (a (uniq) b (uniq) c (uniq) ...)
      `(with ,(apply + '() (map (^(n) (list n '(uniq)))
                                names))
         ,@body)
    ; (w/uniq a ...) => (let a (uniq) ...)
    `(let ,names (uniq) ,@body)))

(defmacro and args
  (if args
      (if (cdr args)
          `(if ,(car args) (and ,@(cdr args))
               )  ; Eliminating else-value relies on UNDEF = false
        (car args))
    't))  ; (and) = true

(defmacro or args
  (and args
       (w/uniq g
         `(let ,g ,(car args)
            (if ,g ,g (or ,@(cdr args)))))))

(defmacro afn (parms . body)
  `(let self nil
     (set! self (^ ,parms ,@body))))

(defmacro caselet (var expr . args)
  (let ex (afn (args)
            (if (no args)
                  ()
                (no (cdr args))
                  (car args)
                `(if (is ,var ',(car args))
                     ,(cadr args)
                   ,(self (cddr args)))))
    `(let ,var ,expr ,(ex args))))

(defmacro case (expr . args)
  `(caselet ,(uniq) ,expr ,@args))



(defn isnt (x y)
  (no (is x y)))

(defn len (x)
  ((afn (x n)
        (if (consp x)
            (self (cdr x) (+ n 1))
            n))
   x 0))

(defn last-pair (ls)
  (if (consp (cdr ls))
      (last-pair (cdr ls))
      ls))

(defn reverse! (ls)
  (if (consp ls)
      ((afn (c p)
            (let d (cdr c)
              (rplacd c p)
              (if (consp d)
                  (self d c)
                  c)))
       ls ())
    ls))


;; Write shared structure.
(defn write/ss (s)
  (let h (make-hash-table)
    (hash-table-put! h 'index 0)
    (write/ss-print s (write/ss-loop s h))))

(defn write/ss-loop (s h)
  (if (consp s)
      (if (no (hash-table-exists? h s))
          ;; Put nil for the first appeared object.
          (do (hash-table-put! h s nil)
              ;; And check children recursively.
              (write/ss-loop (car s) (write/ss-loop (cdr s) h)))
        (do (when (no (hash-table-get h s))
              ;; Assign index for the object appeared more than 1 time.
              (let i (hash-table-get h 'index)
                (hash-table-put! h s i)
                (hash-table-put! h 'index (+ 1 i))))
            h))
    h))

(defn write/ss-print (s h)
  (if (consp s)
      (let index (hash-table-get h s)
        (if (and index (< index 0))
            (do (display "#")
                (display (- -1 index))
                (display "#"))
          (do (when index
                (do (display "#")
                    (display index)
                    (display "=")
                    (hash-table-put! h s (- -1 index))))
              ((afn (c s)
                    (if (no s)
                        (display ")")
                      (do (display c)
                          (write/ss-print (car s) h)
                          (if (or (and (consp (cdr s))
                                       (no (hash-table-get h (cdr s))))
                                  (no (cdr s)))
                              (self " " (cdr s))
                            (do (display " . ")
                                (write/ss-print (cdr s) h)
                                (display ")"))))))
               "(" s))))
    (write s)))
