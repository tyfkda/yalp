;; Macro hash table, (symbol => closure)
(def *macro-table* (make-hash-table))

;; Compile (defmacro name (vars ...) bodies) syntax.
(def register-macro
    (^(name closure)
      (hash-table-put! *macro-table* name closure)))

;; Whether the given name is macro.
(def macro?
    (^(name)
      (hash-table-exists? *macro-table* name)))

(def no (^(x) (if x nil t)))

(def map
    (^(f ls)
      (if ls
          (cons (f (car ls))
                (map f (cdr ls)))
          ())))

;; Make pair from a list. (a b c d e) -> ((a b) (c d) (e))
(def pair
    (^(xs)
      (if (no xs)
            nil
          (no (cdr xs))
            (list (list (car xs)))
          (cons (list (car xs) (cadr xs))
                (pair (cddr xs))))))

(def cadr (^(x) (car (cdr x))))
(def cddr (^(x) (cdr (cdr x))))
(def caadr (^(x) (car (cadr x))))

(def qq-expand
    (^(x)
      (if (consp x)
          ((^(m)
             (if (is m 'unquote)
                 (cadr x)
                 (is m 'unquote-splicing)
                 (error "Illegal")
                 (is m 'quasiquote)
                 (qq-expand
                  (qq-expand (cadr x)))
                 (list 'append
                       (qq-expand-list (car x))
                       (qq-expand (cdr x)))))
           (car x))
        (list 'quote x))))

(def qq-expand-list
    (^(x)
      (if (consp x)
          ((^(m)
             (if (is m 'unquote)
                 (list 'list (cadr x))
                 (is m 'unquote-splicing)
                 (cadr x)
                 (is m 'quasiquote)
                 (qq-expand-list
                  (qq-expand (cadr x)))
                 (list 'list
                       (list 'append
                             (qq-expand-list (car x))
                             (qq-expand (cdr x))))))
           (car x))
        (list 'quote (list x)))))

(defmacro quasiquote (x)
  (qq-expand x))

(defmacro def (name . body)
  (if (consp name)
      `(def ,(car name)
           (^ ,(cdr name) ,@body))
    `(def ,name ,@body)))

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
  (if (no body)
      expr
    `(let it ,expr
       (if it
           ,@(if (cdr body)
                 `(,(car body) (aif ,@(cdr body)))
               body)))))

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



(def (isnt x y)
  (no (is x y)))

(def (len x)
  ((afn (x n)
        (if (consp x)
            (self (cdr x) (+ n 1))
            n))
   x 0))

;; Returns last pair
(def (last ls)
  (if (consp (cdr ls))
      (last (cdr ls))
    ls))

(def (reverse! ls)
  (if (consp ls)
      ((afn (c p)
            (let d (cdr c)
              (set-cdr! c p)
              (if (consp d)
                  (self d c)
                  c)))
       ls ())
    ls))

(def (newline)
  (display "\n"))

(def (print x)
  (display x)
  (newline)
  x)

;; Write shared structure.
(def (write/ss s)
  (let h (make-hash-table)
    (hash-table-put! h 'index 0)
    (write/ss-print s (write/ss-loop s h))))

(def (write/ss-loop s h)
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

(def (write/ss-print s h)
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
