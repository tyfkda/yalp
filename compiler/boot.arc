(def no (^(x) (if x nil t)))

(def any?
  (^(f ls)
    (if (pair? ls)
        (if (f (car ls))
            ls
            (any? f (cdr ls)))
      nil)))

(def map1-loop
  (^(f ls acc)
    (if (pair? ls)
        (map1-loop f (cdr ls)
                   (cons (f (car ls)) acc))
      (reverse! acc))))

(def mapn-loop
  (^(f lss acc)
    (if (any? no lss)
        (reverse! acc)
      (mapn-loop f (map1-loop cdr lss '())
                 (cons (apply f (map1-loop car lss '()))
                       acc)))))

(def map
  (^(f ls . rest)
    (if rest
        (mapn-loop f (cons ls rest) '())
      (map1-loop f ls '()))))

;; Make pair from a list. (a b c d e) -> ((a b) (c d) (e))
(def pair
    (^(xs)
      (if (no xs)
            nil
          (no (cdr xs))
            (list (list (car xs)))
          (cons (list (car xs) (cadr xs))
                (pair (cddr xs))))))

(def cadr [car (cdr _)])
(def cddr [cdr (cdr _)])

(def qq-expand
    (^(x)
      (if (pair? x)
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
      (if (pair? x)
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

(defmacro do body
  `((^() ,@body)))

(defmacro when (test . body)
  `(if ,test (do ,@body)))

(defmacro unless (test . body)
  `(if (not ,test) (do ,@body)))

(defmacro def (name . body)
  (if (pair? name)
      `(def ,(car name)
         (^ ,(cdr name) ,@body))
    `(def ,name ,@body)))

(defmacro set! (var val . rest)
  (if rest
      `(do ,@(map (^(vv) `(set! ,(car vv) ,(cadr vv)))
                  (cons (list var val)
                        (pair rest))))
    `(set! ,var ,val)))

(defmacro let (var val . body)
  `((^(,var) ,@body) ,val))

(defmacro with (parms . body)
  `((^ ,(map car (pair parms))
       ,@body)
    ,@(map cadr (pair parms))))

(defmacro with* (parms . body)
  (if parms
      `((^ (,(car parms))
           ,@(if (cddr parms)
                `((with* ,(cddr parms) ,@body))
              body))
        ,(cadr parms))
    `(do ,@body)))

;; Anapholic-with macro.
;; Like with macro, but captures "loop" variable to make loop syntax.
;; This is similar to named-let syntax in Scheme.
(defmacro awith (parms . body)
  `((let loop nil
      (set! loop (^ ,(map car (pair parms))
                    ,@body)))
    ,@(map cadr (pair parms))))

(defmacro aif (expr . body)
  (if (no body)
      expr
    `(let it ,expr
       (if it
           ,@(if (cdr body)
                 `(,(car body) (aif ,@(cdr body)))
               body)))))

(defmacro w/uniq (names . body)
  (if (pair? names)
      ; (w/uniq (a b c) ...) => (with (a (uniq) b (uniq) c (uniq) ...)
      `(with ,(apply append (map [list _ '(uniq)]
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

(defmacro caselet (var expr . args)
  `(let ,var ,expr
     ,(awith (args args)
        (if (no args)
              '()
            (no (cdr args))
              (car args)
            `(if (is ,var ',(car args))
                 ,(cadr args)
               ,(loop (cddr args)))))))

(defmacro case (expr . args)
  `(caselet ,(uniq) ,expr ,@args))

(def (isnt x y)
  (no (is x y)))

(def (len x)
  (awith (x x
          n 0)
    (if (pair? x)
        (loop (cdr x) (+ n 1))
      n)))

;; Returns last pair
(def (last ls)
  (if (pair? (cdr ls))
      (last (cdr ls))
    ls))

(def (member-if f ls)
  (when (pair? ls)
    (if (f (car ls))
        ls
      (member-if f (cdr ls)))))

(def (member x ls)
  (member-if [is x _] ls))

(def (print x . rest)
  (let stream (if rest (car rest)
                  *stdout*)
    (display x stream)
    (display "\n" stream))
  x)

;; Write shared structure.
(def (write/ss s . rest)
  (with (stream (if rest (car rest)
                    *stdout*)
         h (make-hash-table))
    (hash-table-put! h 'index 0)
    (write/ss-print s (write/ss-loop s h) stream)))

(def (write/ss-loop s h)
  (if (pair? s)
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

(def (write/ss-print s h stream)
  (if (pair? s)
      (let index (hash-table-get h s)
        (if (and index (< index 0))
            (do (display "#" stream)
                (display (- -1 index) stream)
                (display "#" stream))
          (do (when index
                (do (display "#" stream)
                    (display index stream)
                    (display "=" stream)
                    (hash-table-put! h s (- -1 index))))
              (awith (c "("
                      s s)
                (if (no s)
                      (display ")" stream)
                    (do (display c stream)
                        (write/ss-print (car s) h stream)
                        (if (or (and (pair? (cdr s))
                                     (no (hash-table-get h (cdr s))))
                                (no (cdr s)))
                            (loop " " (cdr s))
                          (do (display " . " stream)
                              (write/ss-print (cdr s) h stream)
                              (display ")" stream)))))))))
    (write s)))
