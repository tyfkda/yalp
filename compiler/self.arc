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
(def (dotted->proper ls)
  (if (or (no ls)
          (and (pair? ls)
               (no (cdr (last ls)))))
      ls
    ((afn (p acc)
          (if (pair? p)
              (self (cdr p)
                    (cons (car p) acc))
              (reverse! (cons p acc))))
     ls ())))

;;;; set

(def (set-member? x s)
  (if
   (no s) nil
   (is x (car s)) t
   (set-member? x (cdr s))))

(def (set-cons x s)
  (if (set-member? x s)
      s
    (cons x s)))

(def (set-union s1 s2)
  (if (no s1)
      s2
    (set-union (cdr s1) (set-cons (car s1) s2))))

(def (set-minus s1 s2)
  (if (no s1)
        ()
      (set-member? (car s1) s2)
        (set-minus (cdr s1) s2)
      (cons (car s1) (set-minus (cdr s1) s2))))

(def (set-intersect s1 s2)
  (if (no s1)
        ()
      (set-member? (car s1) s2)
        (cons (car s1) (set-intersect (cdr s1) s2))
      (set-intersect (cdr s1) s2)))

;;; Compiler

(def *exit-compile* nil)

(def (compile-error . args)
  (print args)
  (*exit-compile* nil))

(def (compile x)
  (call/cc
   (^(cc)
     (set! *exit-compile* cc)
     (compile-recur (macroexpand-all x ())
                    '(()) () '(HALT)))))

;; Compiles lisp code into vm code.
;;   x : code to be compiled.
;;   e : current environment, ((local-vars ...) free-vars ...)
;;   s : sets variables, (sym1 sym2 ...)
;;   @result : compiled code (list)
(def (compile-recur x e s next)
  (if (symbol? x)
        (compile-refer x e
                       (if (set-member? x s)
                           (list 'UNBOX next)
                           next))
      (pair? x)
        (record-case x
                     (quote (obj) (list 'CONST obj next))
                     (^ (vars . body)
                        (compile-lambda vars body e s next))
                     (if (test then . rest)
                         (with (thenc (compile-recur then e s next)
                                      elsec (if (no rest)
                                                (compile-undef next)
                                                (no (cdr rest))
                                                (compile-recur (car rest) e s next)
                                                (compile-recur `(if ,@rest) e s next)))
                           (compile-recur test e s (list 'TEST thenc elsec))))
                     (set! (var x)
                           (compile-lookup var e
                                           (^(n) (compile-recur x e s (list 'LSET n next)))
                                           (^(n) (compile-recur x e s (list 'FSET n next)))
                                           (^()  (compile-recur x e s (list 'GSET var next)))))
                     (def (var x)
                         (compile-recur x e s (list 'DEF var next)))
                     (call/cc (x)
                              (let c (list 'CONTI (if (tail? next) 't 'nil)
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
                     (defmacro (name vars . body)
                       (compile-defmacro name vars body next))
                     (else
                      (with (func (car x)
                                  args (cdr x))
                        (compile-apply func args e s next))))
    (list 'CONST x next)))

(def (compile-undef next)
  (list 'UNDEF next))

(def (compile-apply func args e s next)
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

(def (compile-lambda vars body e s next)
  (let proper-vars (dotted->proper vars)
    (aif (member-if (^(x) (no (symbol? x))) proper-vars)
         (compile-error "Function parameter must be symbol")
      (with (free (set-intersect (set-union (car e)
                                            (cdr e))
                                 (find-frees body () proper-vars))
                  sets (find-setses body (dotted->proper proper-vars))
                  varnum (if (is vars proper-vars)
                             (list (len vars) (len vars))
                           (list (- (len proper-vars) 1)
                                  -1)))
        (collect-free free e
                      (list 'CLOSE
                            varnum
                            (len free)
                            (make-boxes sets proper-vars
                                        (compile-lambda-body proper-vars body free sets s))
                            next))))))

(def (compile-lambda-body vars body free sets s)
  (with (ee (cons vars free)
         ss (set-union sets
                       (set-intersect s free))
         next (list 'RET))
    (if (no body)
        (compile-undef next)
      ((afn (p)
            (if (no p)
                next
                (compile-recur (car p) ee ss
                               (self (cdr p)))))
       body))))

(def (find-frees xs b vars)
  (let bb (set-union (dotted->proper vars) b)
    ((afn (v p)
      (if (no p)
          v
        (self (set-union v (find-free (car p) bb))
              (cdr p))))
     () xs)))

;; Find free variables.
;; This does not consider upper scope, so every symbol except under scope
;; are listed up.
(def (find-free x b)
  (if (symbol? x)
        (if (set-member? x b) () (list x))
      (pair? x)
        (record-case x
                     (^ (vars . body)
                        (find-frees body b vars))
                     (quote (obj) ())
                     (if      all (find-frees all b ()))
                     (set! (var exp)
                           (set-union (if (set-member? var b) '() (list var))
                                      (find-free exp b)))
                     (def (var exp)
                         (set-union (if (set-member? var b) () (list var))
                                    (find-free exp b)))
                     (call/cc all (find-frees all b ()))
                     (defmacro (name vars . body) (find-frees body b vars))
                     (else        (find-frees x b ())))
    ()))

(def (collect-free vars e next)
  (if (no vars)
      next
    (collect-free (cdr vars) e
                  (compile-refer (car vars) e
                                 (list 'PUSH next)))))

(def (find-setses xs v)
  ((afn (b p)
        (if (no p)
            b
            (self (set-union b (find-sets (car p) v))
                  (cdr p))))
   () xs))

;; Find assignment expression for local variables to make them boxing.
;; Boxing is needed to keep a value for continuation.
(def (find-sets x v)
  (if (pair? x)
      (record-case x
                   (set! (var val)
                         (set-union (if (set-member? var v) (list var) ())
                                    (find-sets val v)))
                   (def (var val)
                       (find-sets val v))
                   (^ (vars . body)
                      (find-setses body (set-minus v (dotted->proper vars))))
                   (quote   all ())
                   (if      all (find-setses all v))
                   (call/cc all (find-setses all v))
                   (defmacro (name vars . body)  (find-setses body (set-minus v (dotted->proper vars))))
                   (else        (find-setses x   v)))
    ()))

(def (make-boxes sets vars next)
  ((afn (vars n)
        (if (no vars)
              next
            (set-member? (car vars) sets)
              (list 'BOX n (self (cdr vars) (+ n 1)))
            (self (cdr vars) (+ n 1))))
   vars 0))

(def (compile-refer var e next)
  (compile-lookup var e
                  (^(n) (list 'LREF n next))
                  (^(n) (list 'FREF n next))
                  (^()  (list 'GREF var next))))

(def (find-index x ls)
  ((afn (ls idx)
        (if (no ls) nil
            (is (car ls) x) idx
            (self (cdr ls) (+ idx 1))))
   ls 0))

(def (compile-lookup var e return-local return-free return-global)
  (with (locals (car e)
         free   (cdr e))
    (aif (find-index var locals)  (return-local it)
         (find-index var free)    (return-free it)
         (return-global))))

(def (tail? next)
  (is (car next) 'RET))

;;; Macro

(def (compile-defmacro name vars body next)
  (let proper-vars (dotted->proper vars)
    (with (min (if (is vars proper-vars) (len vars) (- (len proper-vars) 1))
           max (if (is vars proper-vars) (len vars) -1)
           body-code (compile-lambda-body proper-vars body (list proper-vars) () ()))
      ;; Macro registeration will be done in other place.
      ;(register-macro name (closure body-code 0 %running-stack-pointer min max))
      (list 'MACRO
            name
            (list min max)
            body-code
            next))))

(def (macroexpand-all exp scope-vars)
  (if (pair? exp)
      (if (no (member (car exp) scope-vars))
          (let expanded (macroexpand-1 exp)
            (if (iso expanded exp)
                (macroexpand-all-sub exp scope-vars)
              (macroexpand-all expanded scope-vars)))
        (macroexpand-all-sub exp scope-vars))
    exp))

(def (map-macroexpand-all ls svars)
  (map (^(e) (macroexpand-all e svars))
       ls))

(def (macroexpand-all-sub exp scope-vars)
  (record-case exp
               (quote (obj) `(quote ,obj))
               (^ (vars . body)
                  (let new-scope-vars (append (dotted->proper vars) scope-vars)
                    `(^ ,vars ,@(map-macroexpand-all body new-scope-vars))))
               (if all
                   `(if ,@(map-macroexpand-all all scope-vars)))
               (set! (var x)
                     `(set! ,var ,(macroexpand-all x scope-vars)))
               (def (var x)
                   `(def ,var ,(macroexpand-all x scope-vars)))
               (call/cc (x)
                        `(call/cc ,(macroexpand-all x scope-vars)))
               (defmacro (name vars . body)
                 (let new-scope-vars (append (dotted->proper vars) scope-vars)
                   `(defmacro ,name ,vars ,@(map-macroexpand-all body new-scope-vars))))
               (else (map-macroexpand-all exp scope-vars))))

(def (macroexpand exp)
  (let expanded (macroexpand-1 exp)
    (if (iso expanded exp)
        exp
      (macroexpand expanded))))

;;

(def (eval x)
  (run-binary (compile x)))
