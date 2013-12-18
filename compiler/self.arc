;;;; Compiler
;;;; Based on 3imp.

; Stack usage:
;    c   ... closure
;    s   ... stack pointer
;    f   ... frame pointer (stack index)
;    ret ... next code
;
; * Initial
;   f = s = 0
;
; * Function call
;   1. Use FRAME opcode to create new frame on stack:
;     f[c][f][ret]s
;   2. Push function arguments to stack on reverse order:
;     f[c][f][ret][a3][a2][a1]s
;   3. Use APPLY opcode to apply function,
;      then argument number is pushed to stack:
;     [c][f][ret][a3][a2][a1]f[argnum]s
;
; * Return
;   1. Get called argument number from stack:
;     [c][f][ret][a3][a2][a1]f
;   2. Put back previous frame:
;     f = s = 0
; * Shift
;   1. Put next function arguments:
;     [c][f][ret][a3][a2][a1]f[argnum][b1]s
;   2. Use SHIFT opcode to remove previous function arguments:
;     [c][f][ret][b1]s
;   4. Use APPLY opcode to apply function,
;      then argument number is pushed to stack:
;     [c][f][ret][b1]f[argnum]s

(defmacro record (args parm . exprs)
  `(apply (^ ,parm
            ,@exprs)
          ,args))

(defmacro record-case (x . args)
  (w/uniq g1
    `(let ,g1 ,x
       (case (car ,g1)
         ,@(awith (p args)
              (when p
                (let e (car p)
                  (with (key (car e)
                         vars (cadr e)
                         exprs (cddr e))
                    (if (is key 'else)
                          (cdr e)
                        `(,key
                          (record (cdr ,g1) ,vars ,@exprs)
                          ,@(loop (cdr p))))))))))))

;;; dotted pair -> proper list
(def (dotted->proper ls)
  (if (or (no ls)
          (and (pair? ls)
               (no (cdr (last ls)))))
      ls
    (awith (p ls
            acc '())
      (if (pair? p)
          (loop (cdr p)
                (cons (car p) acc))
        (reverse! (cons p acc))))))

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
        '()
      (set-member? (car s1) s2)
        (set-minus (cdr s1) s2)
      (cons (car s1) (set-minus (cdr s1) s2))))

(def (set-intersect s1 s2)
  (if (no s1)
        '()
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
     (compile-recur (macroexpand-all x '())
                    '(()) '() '(HALT)))))

;; Compiles lisp code into vm code.
;;   x : code to be compiled.
;;   e : current environment, ((local-vars ...) free-vars ...)
;;   s : sets variables, (sym1 sym2 ...)
;;   @result : compiled code (list)
(def (compile-recur x e s next)
  (if (symbol? x)
        (compile-refer x e
                       (if (set-member? x s)
                           (list* 'UNBOX next)
                         next))
      (pair? x)
        (record-case x
                     (quote (obj) (list* 'CONST obj next))
                     (^ (vars . body)
                        (compile-lambda vars body e s next))
                     (if (test then . rest)
                         (with (thenc (compile-recur then e s next)
                                elsec (if (no rest)
                                            (compile-undef next)
                                          (no (cdr rest))
                                            (compile-recur (car rest) e s next)
                                          (compile-recur `(if ,@rest) e s next)))
                           (compile-recur test e s (list* 'TEST thenc elsec))))
                     (set! (var x)
                           (compile-lookup var e
                                           (^(n) (compile-recur x e s (list* 'LSET n next)))
                                           (^(n) (compile-recur x e s (list* 'FSET n next)))
                                           (^()  (compile-recur x e s (list* 'GSET var next)))))
                     (def (var x)
                         (compile-recur x e s (list* 'DEF var next)))
                     (call/cc (x)
                              (let c (list* 'CONTI (if (tail? next) 't 'nil)
                                            'PUSH
                                            (compile-recur x e s
                                                           (if (tail? next)
                                                               '(SHIFT 1
                                                                 APPLY 1)
                                                             '(APPLY 1))))
                                (if (tail? next)
                                    c
                                  (list* 'FRAME c next))))
                     (defmacro (name vars . body)
                       (compile-defmacro name vars body next))
                     (else
                      (with (func (car x)
                             args (cdr x))
                        (if (direct-invoke? func)
                            (compile-apply-direct (cadr func) (cddr func) args e s next)
                          (compile-apply func args e s next)))))
    (list* 'CONST x next)))

(def (compile-undef next)
  (list* 'UNDEF next))

(def (direct-invoke? func)
  (and (pair? func)
       (is '^ (car func))))

(def (compile-apply func args e s next)
  (with* (argnum (len args)
          c (compile-recur func e s
                           (if (tail? next)
                               (list 'SHIFT argnum
                                     'APPLY argnum)
                             (list 'APPLY argnum)))
          bc (compile-apply-args args c e s))
    (if (tail? next)
        bc
      (list* 'FRAME bc next))))

(def (compile-apply-args args next e s)
  (awith (args args
          c next)
    (if (no args)
        c
      (loop (cdr args)
            (compile-recur (car args) e s
                           (list* 'PUSH c))))))

;; Compiles ((^(vars) body) args) form
(def (compile-apply-direct vars body args e s next)
  (with* (proper-vars (check-parameters vars)
          ext-vars (append proper-vars (car e)))
    (with (free (cdr e)  ;(set-intersect (set-union (car e)
                         ;                 (cdr e))
                         ;      (find-frees body '() proper-vars))
           sets (set-union s (find-setses body ext-vars))) ;(find-setses body proper-vars))
      (with* (argnum (len args)
              c (if (is argnum 0)
                    (compile-lambda-body ext-vars body free sets s
                                         next)
                  (list* 'EXPND argnum
                         (make-boxes sets proper-vars
                                     (compile-lambda-body ext-vars body free sets s
                                                          (if (tail? next)
                                                              next
                                                            (list* 'SHRNK argnum next)))))))
        (compile-apply-args args c e s)))))

(def (compile-lambda vars body e s next)
  (let proper-vars (check-parameters vars)
    (with (free (set-intersect (set-union (car e)
                                          (cdr e))
                               (find-frees body '() proper-vars))
           sets (find-setses body proper-vars)
           varnum (if (is vars proper-vars)
                      (len vars)
                    (list (- (len proper-vars) 1)
                          -1)))
      (collect-free free e
                    (list* 'CLOSE varnum (len free)
                           (make-boxes sets proper-vars
                                       (compile-lambda-body proper-vars body free sets s '(RET)))
                           next)))))

;; Check function parameters are valid and returns proper vars.
(def (check-parameters vars)
  (let proper-vars (dotted->proper vars)
    (when (member-if (^(var) (no (symbol? var))) vars)
      (compile-error "parameter must be symbol"))
    proper-vars))

(def (compile-lambda-body vars body free sets s next)
  (with (ee (cons vars free)
         ss (set-union sets
                       (set-intersect s free)))
    (if (no body)
        (compile-undef next)
      (awith (p body)
        (if (no p)
            next
          (compile-recur (car p) ee ss
                         (loop (cdr p))))))))

(def (find-frees xs b vars)
  (let bb (set-union (dotted->proper vars) b)
    (awith (v '()
            p xs)
      (if (no p)
          v
        (loop (set-union v (find-free (car p) bb))
              (cdr p))))))

;; Find free variables.
;; This does not consider upper scope, so every symbol except under scope
;; are listed up.
(def (find-free x b)
  (if (symbol? x)
        (if (set-member? x b) '() (list x))
      (pair? x)
        (record-case x
                     (^ (vars . body)
                        (find-frees body b vars))
                     (quote (obj) '())
                     (if      all (find-frees all b '()))
                     (set! (var exp)
                           (set-union (if (set-member? var b) '() (list var))
                                      (find-free exp b)))
                     (def (var exp)
                         (set-union (if (set-member? var b) '() (list var))
                                    (find-free exp b)))
                     (call/cc all (find-frees all b '()))
                     (defmacro (name vars . body) (find-frees body b vars))
                     (else        (find-frees x b '())))
    '()))

(def (collect-free vars e next)
  (if (no vars)
      next
    (collect-free (cdr vars) e
                  (compile-refer (car vars) e
                                 (list* 'PUSH next)))))

(def (find-setses xs v)
  (awith (b '()
          p xs)
    (if (no p)
        b
      (loop (set-union b (find-sets (car p) v))
            (cdr p)))))

;; Find assignment expression for local variables to make them boxing.
;; Boxing is needed to keep a value for continuation.
(def (find-sets x v)
  (if (pair? x)
      (record-case x
                   (set! (var val)
                         (set-union (if (set-member? var v) (list var) '())
                                    (find-sets val v)))
                   (def (var val)
                       (find-sets val v))
                   (^ (vars . body)
                      (find-setses body (set-minus v (dotted->proper vars))))
                   (quote   all '())
                   (if      all (find-setses all v))
                   (call/cc all (find-setses all v))
                   (defmacro (name vars . body)  (find-setses body (set-minus v (dotted->proper vars))))
                   (else        (find-setses x   v)))
    '()))

(def (make-boxes sets vars next)
  (awith (vars vars
          n 0)
    (if (no vars)
          next
        (set-member? (car vars) sets)
          (list* 'BOX n (loop (cdr vars) (+ n 1)))
        (loop (cdr vars) (+ n 1)))))

(def (compile-refer var e next)
  (compile-lookup var e
                  (^(n) (list* 'LREF n next))
                  (^(n) (list* 'FREF n next))
                  (^()  (list* 'GREF var next))))

(def (find-index x ls)
  (awith (ls ls
          idx 0)
    (if (no ls) nil
        (is (car ls) x) idx
        (loop (cdr ls) (+ idx 1)))))

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
  (let proper-vars (check-parameters vars)
    (with (varnum (if (is vars proper-vars)
                      (len vars)
                    (list (- (len proper-vars) 1)
                          -1))
           body-code (compile-lambda-body proper-vars body (list proper-vars) '() '() '(RET)))
      ;; Macro registeration will be done in other place.
      ;(register-macro name (closure body-code 0 %running-stack-pointer min max))
      (list* 'MACRO name varnum body-code
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
