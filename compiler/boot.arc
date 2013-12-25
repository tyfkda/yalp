(proclaim
 (inline no cadr cddr isnt member))

(set-macro-character #\'
                     (^(stream ch)
                       (list (quote quote) (read stream))))

(set-macro-character #\`
                     (^(stream ch)
                       (list 'quasiquote (read stream))))

(set-macro-character #\,
                     (^(stream ch)
                       ((^(c)
                          (if (is c #\@)
                              (list 'unquote-splicing (read stream))
                            (list 'unquote (read (unread-char c stream)))))
                        (read-char stream))))

(def no (^(x) (if x nil t)))
(def fixnum? (^(x) (is (type x) 'fixnum)))
(def pair? (^(x) (is (type x) 'pair)))
(def symbol? (^(x) (is (type x) 'symbol)))
(def string? (^(x) (is (type x) 'string)))
(def flonum? (^(x) (is (type x) 'flonum)))
(def procedure? (^(x) (let tt (type x)
                        (or (is tt 'closure)
                            (is tt 'subr)
                            (is tt 'continuation)))))

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
      (let result (reverse! acc)
        (if ls
            (do (set-cdr! acc (f ls))
                result)
          result)))))

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

(def cadr (^(x) (car (cdr x))))
(def cddr (^(x) (cdr (cdr x))))

;; Make pair from a list. (a b c d e) -> ((a b) (c d) (e))
(def pair
    (^(xs)
      (if (no xs)
            nil
          (no (cdr xs))
            (list (list (car xs)))
          (cons (list (car xs) (cadr xs))
                (pair (cddr xs))))))

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
  `(if ,test (do) (do ,@body)))

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

(set-macro-character #\[
                     (^(stream ch)
                       `(^ (_) ,(read-delimited-list #\] stream))))

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

(defmacro awhen (expr . body)
  `(aif ,expr
     (do ,@body)))

(defmacro awhile (expr . body)
  `(awith ()
     (awhen ,expr
       ,@body
       (loop))))

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
          `(if ,(car args)
               (and ,@(cdr args))
             'nil)
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

(defmacro dolist (vars . body)
  (with (x (car vars)
         ls (cadr vars))
    (w/uniq p
      `(awith (,p ,ls)
         (when (pair? ,p)
           (let ,x (car ,p)
             ,@body
             (loop (cdr ,p))))))))

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

(def (reverse ls)
  (awith (ls ls
          acc '())
    (if (pair? ls)
        (loop (cdr ls) (cons (car ls) acc))
      acc)))

(def (member-if f ls)
  (if (pair? ls)
      (if (f (car ls))
          ls
        (member-if f (cdr ls)))
    nil))

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
         h (table))
    (table-put! h 'index 0)
    (write/ss-construct s h)
    (write/ss-print s h stream)))

;; Put cell appear idnex for more than 2 times into table.
(def (write/ss-construct s h)
  (when (pair? s)
    (if (table-exists? h s)
        (do (unless (table-get h s)
              ;; Assign index for the object appeared more than 1 time.
              (let i (table-get h 'index)
                (table-put! h s i)
                (table-put! h 'index (+ 1 i)))))
      ;; Put nil for the first appeared object.
      (do (table-put! h s nil)
          ;; And check children recursively.
          (write/ss-construct (car s) h)
          (write/ss-construct (cdr s) h)))))

(def (write/ss-print s h stream)
  (if (pair? s)
      (let index (table-get h s)
        (if (and index (< index 0))
            ;; Print structure after second time or later.
            (format stream "#%@#" (- -1 index))
          ;; Print structure at first time.
          (do (when index
                (format stream "#%@=" index)
                (table-put! h s (- -1 index)))
              (awith (c "("
                      s s)
                (if s
                    (do (display c stream)
                        (write/ss-print (car s) h stream)
                        (if (or (and (pair? (cdr s))
                                     (no (table-get h (cdr s))))
                                (no (cdr s)))
                            (loop " " (cdr s))
                          (do (display " . " stream)
                              (write/ss-print (cdr s) h stream)
                              (display ")" stream))))
                  (display ")" stream))
                s))))
    (write s)))

;; Take first n elements from the list.
(def (take n ls)
  (awith (n n
          acc '()
          ls ls)
    (if (and ls (> n 0))
        (loop (- n 1) (cons (car ls) acc) (cdr ls))
      (reverse! acc))))

;; Drop first n elements from the list (nthcdr).
(def (drop n ls)
  (if ls
      (if (> n 0)
          (drop (- n 1) (cdr ls))
        ls)))
