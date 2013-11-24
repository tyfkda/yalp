(defmacro record (args parm . exprs)
  `(apply (^ ,parm
            ,@exprs)
          ,args))

(defmacro record-case (x . args)
  (w/uniq g1
    `(let ,g1 ,x
       (case (car ,g1)
         ,@((afn (p)
                 (when p
                   (let e (car p)
                     (with (key (car e)
                            vars (cadr e)
                            exprs (cddr e))
                       (if (is key 'else)
                           (cdr e)
                         `(,key
                           (record (cdr ,g1) ,vars ,@exprs)
                           ,@(self (cdr p))))))))
            args)))))

;;; dotted pair -> proper list
(defn dotted->proper (ls)
  (if (or (no ls)
          (no (cdr (last-pair ls))))
      ls
    ((afn (p acc)
          (if (consp p)
              (self (cdr p)
                    (cons (car p) acc))
              (reverse! (cons p acc))))
     ls nil)))

;;;; set

(defn set-member? (x s)
  (if
   (no s) nil
   (is x (car s)) t
   (set-member? x (cdr s))))

(defn set-cons (x s)
  (if (set-member? x s)
      s
    (cons x s)))

(defn set-union (s1 s2)
  (if (no s1)
      s2
    (set-union (cdr s1) (set-cons (car s1) s2))))

(defn set-minus (s1 s2)
  (if (no s1)
        '()
      (set-member? (car s1) s2)
        (set-minus (cdr s1) s2)
      (cons (car s1) (set-minus (cdr s1) s2))))

(defn set-intersect (s1 s2)
  (if (no s1)
        '()
      (set-member? (car s1) s2)
        (cons (car s1) (set-intersect (cdr s1) s2))
      (set-intersect (cdr s1) s2)))

;;; Compiler

(defn compile (x)
  (compile-recur x '(()) '() '(HALT)))

;; Compiles lisp code into vm code.
;;   x : code to be compiled.
;;   e : current environment, ((local-vars ...) free-vars ...)
;;   s : sets variables, (sym1 sym2 ...)
;;   @result : compiled code (list)
(defn compile-recur (x e s next)
  (if (symbolp x)
        (compile-refer x e
                       (if (set-member? x s)
                           (liset 'UNBOX next)
                           next))
      (consp x)
        (record-case x
                     (quote (obj) (list 'CONST obj next))
                     (^ (vars . bodies)
                        (compile-lambda vars bodies e s next))
                     (if (test then . rest)
                         (with (thenc (compile-recur then e s next)
                                elsec (if (no rest)
                                            (compile-undef e s next)
                                          (no (cdr rest))
                                            (compile-recur (car rest) e s next)
                                          (compile-recur `(if ,@rest) e s next)))
                           (compile-recur test e s (list 'TEST thenc elsec))))
                     (set! (var x)
                           (compile-lookup var e
                                           (^(n)   (compile-recur x e s (list 'LSET n next)))
                                           (^(n)   (compile-recur x e s (list 'FSET n next)))
                                           (^(sym) (compile-recur x e s (list 'GSET sym next)))))
                     (call/cc (x)
                              (let c (list 'CONTI
                                           (list 'PUSH
                                                 (compile-recur x e s
                                                                (if (tail? next)
                                                                    (list 'SHIFT
                                                                          1
                                                                          '(APPLY 1))
                                                                  '(APPLY 1)))))
                                (if (tail? next)
                                    c
                                    (list 'FRAME next c))))
                     (defmacro (name vars . bodies)
                       (register-macro name vars bodies)
                       (compile-recur `(quote ,name) e s next))
                     (else
                      (with (func (car x)
                             args (cdr x))
                        (if (macro? func)
                            (compile-apply-macro func args e s next)
                            (compile-apply func args e s next)))))
        (list 'CONST x next)))

(defn compile-undef (e s next)
  (list 'UNDEF next))

(defn compile-apply (func args e s next)
  (let argnum (len args)
    ((afn (args c)
          (if (no args)
              (if (tail? next)
                  c
                  (list 'FRAME next c))
              (self (cdr args)
                    (compile-recur (car args)
                                   e
                                   s
                                   (list 'PUSH c)))))
     args
     (compile-recur func e s
                    (if (tail? next)
                        `(SHIFT ,argnum
                                (APPLY ,argnum))
                      `(APPLY ,argnum))))))

(defn compile-lambda (vars bodies e s next)
  (let proper-vars (dotted->proper vars)
    (with (free (set-intersect (set-union (car e)
                                          (cdr e))
                               (find-frees bodies '() proper-vars))
           sets (find-setses bodies (dotted->proper proper-vars))
           varnum (if (is vars proper-vars)
                      (list (len vars) (len vars))
                    (list (- (len proper-vars) 1)
                          -1)))
      (collect-free free e
                    (list 'CLOSE
                          varnum
                          (len free)
                          (make-boxes sets proper-vars
                                      (compile-lambda-bodies proper-vars bodies free sets s))
                          next)))))

(defn compile-lambda-bodies (vars bodies free sets s)
  (with (ee (cons vars free)
         ss (set-union sets
                       (set-intersect s free))
         next (list 'RET))
    ((afn (p)
          (if (no p)
              next
              (compile-recur (car p) ee ss
                             (self (cdr p)))))
     bodies)))

(defn find-frees (xs b vars)
  (let bb (set-union (dotted->proper vars) b)
    ((afn (v p)
      (if (no p)
          v
        (self (set-union v (find-free (car p) bb))
              (cdr p))))
     '() xs)))

;; Find free variables.
;; This does not consider upper scope, so every symbol except under scope
;; are listed up.
(defn find-free (x b)
  (if (symbolp x)
        (if (set-member? x b) '() (list x))
      (consp x)
        (let expanded (expand-macro-if-so x)
          (if (is expanded x)
                (record-case expanded
                             (^ (vars . bodies)
                                (find-frees bodies b vars))
                             (quote (obj) '())
                             (if      all (find-frees all b '()))
                             (set!    all (find-frees all b '()))
                             (call/cc all (find-frees all b '()))
                             (defmacro (name vars . bodies) (find-frees bodies b vars))
                             (else        (find-frees expanded b '())))
              (find-free expanded b)))
      '()))

(defn collect-free (vars e next)
  (if (no vars)
      next
    (collect-free (cdr vars) e
                  (compile-refer (car vars) e
                                 (list 'PUSH next)))))

(defn find-setses (xs v)
  ((afn (b p)
        (if (no p)
            b
            (self (set-union b (find-sets (car p) v))
                  (cdr p))))
   '() xs))

;; Find assignment expression for local variables to make them boxing.
;; Boxing is needed to keep a value for continuation.
(defn find-sets (x v)
  (if (consp x)
      (let expanded (expand-macro-if-so x)
        (if (isnt expanded x)
            (find-sets expanded v)
          (record-case x
                       (set! (var val)
                             (set-union (if (set-member? var v) (list var) '())
                                        (find-sets val v)))
                       (^ (vars . bodies)
                          (find-setses bodies (set-minus v (dotted->proper vars))))
                       (quote   all '())
                       (if      all (find-setses all v))
                       (call/cc all (find-setses all v))
                       (defmacro (name vars . bodies)  (find-setses bodies (set-minus v (dotted->proper vars))))
                       (else        (find-setses x   v)))))
      '()))

(defn make-boxes (sets vars next)
  ((afn (vars n)
        (if (no vars)
              next
            (set-member? (car vars) sets)
              (list 'BOX n (self (cdr vars) (+ n 1)))
            (self (cdr vars) (+ n 1))))
   vars 0))

(defn compile-refer (x e next)
  (compile-lookup x e
                  (^(n)   (list 'LREF n next))
                  (^(n)   (list 'FREF n next))
                  (^(sym) (list 'GREF sym next))))

(defn find-index (x ls)
  ((afn (ls idx)
        (if (no ls) nil
            (is (car ls) x) idx
            (self (cdr ls) (+ idx 1))))
   ls 0))

(defn compile-lookup (x e return-local return-free return-global)
  (with (locals (car e)
         free   (cdr e))
    (aif (find-index x locals)  (return-local it)
         (find-index x free)    (return-free it)
         (return-global x))))

(defn tail? (next)
  (is (car next) 'RET))

;; Macro
(defn register-macro (name vars bodies)
  )

(defn macro? (name)
  nil)

(defn expand-macro-if-so (x)
  x)

;;

(defn eval (x)
  (run-binary (compile x)))

;;