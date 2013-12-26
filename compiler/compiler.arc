;;;; Yalp compiler
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

;(load "./boot.arc")
;(load "./util.arc")

;;; Compiler

(def *exit-compile* nil)

(def (compile-error . args)
  (apply format *stderr* args)
  (display "\n")
  (*exit-compile* nil))

(def (compile x)
  (call/cc
   (^(cc)
     (set! *exit-compile* cc)
     (compile-recur (macroexpand-all x '())
                    '(()) '() '(HALT)))))

(defmacro proclaim defs
  `(do
     ,@(map (^(sentence)
              (when (pair? sentence)
                (case (car sentence)
                  inline (do (dolist (sym (cdr sentence))
                                     (proclaim-inline sym))
                             '(values))
                  )))
            defs)))

(def *inline-functions* (table))
(def (proclaim-inline  sym)
  (table-put! *inline-functions* sym t))

(def (inline-function? sym)
  (and (symbol? sym)
       (table-exists? *inline-functions* sym)))

(def (lambda-expression? exp)
  (and (pair? exp)
       (is (car exp) '^)))

(def (register-inline-function sym func)
  (table-put! *inline-functions* sym func))

(def (get-inline-function-body sym)
  (table-get *inline-functions* sym))

;; Compiles lisp code into vm code.
;;   x : code to be compiled.
;;   e : current environment, ((local-vars ...) free-vars ...)
;;   s : sets variables, (sym1 sym2 ...)
;;   @result : compiled code (list)
(def (compile-recur x e s next)
  (cond ((symbol? x)
         (compile-refer x e
                        (if (member x s)
                            (list* 'UNBOX next)
                          next)))
        ((pair? x)
         (record-case x
                      (quote (obj) (list* 'CONST obj next))
                      (^ (vars . body)
                         (compile-lambda vars body e s next))
                      (if (test then . rest)
                          (let ((thenc (compile-recur then e s next))
                                (elsec (if rest
                                           (do (when (cdr rest)
                                                 (compile-error "malformed if"))
                                               (compile-recur (car rest) e s next))
                                         (compile-void next))))
                            (compile-recur test e s (list* 'TEST thenc elsec))))
                      (set! (var val)
                            (unless (symbol? var)
                                    (compile-error "1st argument for `set!` must be symbol, but `%@`" var))
                            (compile-lookup var e
                                            (^(n) (compile-recur val e s (list* 'LSET n next)))
                                            (^(n) (compile-recur val e s (list* 'FSET n next)))
                                            (^()  (compile-recur val e s (list* 'GSET var next)))))
                      (def (var val)
                          (unless (symbol? var)
                            (compile-error "1st argument for `def` must be symbol, but `%@`" var))
                        (when (and (inline-function? var)
                                   (lambda-expression? val))
                          (register-inline-function var val))
                        (compile-recur val e s (list* 'DEF var next)))
                      (call/cc (func)
                               (let1 c (list* 'CONTI (if (tail? next) 't 'nil)
                                             'PUSH
                                             (compile-recur func e s
                                                            (if (tail? next)
                                                                '(TAPPLY 1)  ;(SHIFT 1 APPLY 1)
                                                              '(APPLY 1))))
                                 (if (tail? next)
                                     c
                                   (list* 'FRAME c next))))
                      (defmacro (name vars . body)
                        (compile-defmacro name vars body e s next))
                      (values args
                              (compile-values args e s next))
                      (receive (vars vals . body)
                               (compile-receive vars vals body e s next))
                      (else
                       (let ((func (car x))
                             (args (cdr x)))
                         (cond ((direct-invoke? func)
                                (compile-apply-direct (cadr func) (cddr func) args e s next))
                               ((inline-function? func)
                                (compile-inline-apply func args e s next))
                               (else (compile-apply func args e s next)))))))
        (else (list* 'CONST x next))))

(def (compile-void next)
  (list* 'VOID next))

(def (direct-invoke? func)
  (and (pair? func)
       (is '^ (car func))))

(def (compile-apply func args e s next)
  (let* ((argnum (len args))
         (c (compile-recur func e s
                           (if (tail? next)
                               (list 'TAPPLY argnum)  ;(list 'SHIFT argnum 'APPLY argnum)
                             (list 'APPLY argnum))))
         (bc (compile-apply-args args e s c)))
    (if (tail? next)
        bc
      (list* 'FRAME bc next))))

(def (compile-inline-apply sym args e s next)
  (let1 lambda (get-inline-function-body sym)
    (compile-recur `(,lambda ,@args)
                   e s next)))

(def (compile-apply-args args e s next)
  (awith (args args
          c next)
    (if args
        (loop (cdr args)
              (compile-recur (car args) e s
                             (list* 'PUSH c)))
      c)))

;; Compiles ((^(vars) body) args) form
(def (compile-apply-direct vars body args e s next)
  (let ((proper-vars (check-parameters vars))
        (argnum (len args))
        (varnum (len vars)))
    (if (isnt vars proper-vars)  ;; Has rest param.
        ;; Tweak arguments.
        (cond ((< argnum varnum)  ;; Error handled in the below.
               (compile-apply-direct proper-vars body args e s next))
              ;; ((^(x y . z) ...) 1 2 3 4) => ((^(x y z) ...) 1 2 '(3 4))
              ((> argnum varnum)
               (compile-recur `((^ ,proper-vars ,@body)
                                ,@(take varnum args)
                                (list ,@(drop varnum args)))
                              e s next))
              ;; ((^(x y . z) ...) 1 2) => ((^(x y z) ...) 1 2 '())
              (else (compile-apply-direct proper-vars body (append args '(()))
                                          e s next)))
      (do
        (unless (is argnum varnum)
          (let1 which (if (< argnum varnum) "few" "many")
            (compile-error "Too %@ arguments, %@ for %@" which argnum varnum)))
        (let1 ext-vars (append proper-vars (car e))
          (let ((free (cdr e))  ;(intersection (union (car e)
                                ;                     (cdr e))
                                ;              (find-frees body '() proper-vars))
                (sets (union s (find-setses body ext-vars)))) ;(find-setses body proper-vars))
            (compile-apply-args args e s
                                (if (is argnum 0)
                                    (compile-body proper-vars ext-vars body free sets s
                                                  next)
                                  (list* 'EXPND argnum
                                         (compile-body proper-vars ext-vars body free sets s
                                                       (if (tail? next)
                                                           next
                                                         (list* 'SHRNK argnum next))))))))))))

(def (compile-lambda vars body e s next)
  (let1 proper-vars (check-parameters vars)
    (let ((free (intersection (union (car e)
                                     (cdr e))
                              (find-frees body '() proper-vars)))
          (sets (find-setses body proper-vars))
          (varnum (if (is vars proper-vars)
                      (len vars)
                    (list (- (len proper-vars) 1)
                          -1))))
      (collect-free free e
                    (list* 'CLOSE varnum (len free)
                           (compile-body proper-vars proper-vars body free sets s '(RET))
                           next)))))

(def (collect-free vars e next)
  (if vars
      (collect-free (cdr vars) e
                    (compile-refer (car vars) e
                                   (list* 'PUSH next)))
    next))

;; Check function parameters are valid and returns proper vars.
(def (check-parameters vars)
  (let1 proper-vars (dotted->proper vars)
    (aif (member-if [no (symbol? _)] vars)
      (compile-error "parameter must be symbol, but `%@`" (car it)))
    (awith (p vars)
      (when p
        (when (member (car p) (cdr p))
          (compile-error "Duplicated parameter `%@`" (car p)))
        (loop (cdr p))))
    proper-vars))

(def (compile-body set-vars vars body free sets s next)
  (make-boxes sets set-vars
              (let ((ee (cons vars free))
                    (ss (union sets
                               (intersection s free))))
                (if body
                    (awith (p body)
                      (if p
                          (compile-recur (car p) ee ss
                                         (loop (cdr p)))
                        next))
                  (compile-void next)))))

(def (make-boxes sets vars next)
  (awith (vars vars
          n 0)
    (if vars
        (if (member (car vars) sets)
            (list* 'BOX n (loop (cdr vars) (+ n 1)))
          (loop (cdr vars) (+ n 1)))
      next)))

(def (compile-values args e s next)
  (let1 argnum (len args)
    (if (is argnum 0)
        (compile-void next)
      (compile-apply-args args e s (list* 'VALS argnum next)))))

(def (compile-receive vars vals body e s next)
  (let* ((proper-vars (check-parameters vars))
         (ext-vars (append proper-vars (car e))))
    (let ((free (cdr e))  ;(intersection (union (car e)
                          ;                     (cdr e))
                          ;              (find-frees body '() proper-vars))
          (sets (union s (find-setses body ext-vars))) ;(find-setses body proper-vars))
          (varnum (if (is vars proper-vars)
                      (len vars)
                    (list (- (len proper-vars) 1)
                          -1))))
      (compile-recur vals e s
                     (list* 'RECV varnum
                            (compile-body ext-vars ext-vars body free sets s
                                          (list* 'SHRNK (len proper-vars) next)))))))

(def (find-frees xs b vars)
  (let1 bb (union (dotted->proper vars) b)
    (awith (v '()
            p xs)
      (if p
          (loop (union v (find-free (car p) bb))
                (cdr p))
        v))))

;; Find free variables.
;; This does not consider upper scope, so every symbol except under scope
;; are listed up.
(def (find-free x b)
  (cond ((symbol? x)
         (if (member x b) '() (list x)))
        ((pair? x)
         (record-case x
                      (^ (vars . body)
                         (find-frees body b vars))
                      (quote (obj) '())
                      (if      all (find-frees all b '()))
                      (set! (var exp)
                            (union (if (member var b) '() (list var))
                                   (find-free exp b)))
                      (def (var exp)
                          (union (if (member var b) '() (list var))
                                 (find-free exp b)))
                      (call/cc all (find-frees all b '()))
                      (defmacro (name vars . body) (find-frees body b vars))
                      (values  all (find-frees all b '()))
                      (receive (vars vals . body)
                               (union (find-free vals b)
                                      (find-frees body b vars)))
                      (else        (find-frees x b '()))))
        (else '())))

(def (find-setses xs v)
  (awith (b '()
          p xs)
    (if p
        (loop (union b (find-sets (car p) v))
              (cdr p))
      b)))

;; Find assignment expression for local variables to make them boxing.
;; Boxing is needed to keep a value for continuation.
(def (find-sets x v)
  (if (pair? x)
      (record-case x
                   (set! (var val)
                         (union (if (member var v) (list var) '())
                                (find-sets val v)))
                   (def (var val)
                       (find-sets val v))
                   (^ (vars . body)
                      (find-setses body (set-minus v (dotted->proper vars))))
                   (quote   all '())
                   (if      all (find-setses all v))
                   (call/cc all (find-setses all v))
                   (defmacro (name vars . body)  (find-setses body (set-minus v (dotted->proper vars))))
                   (values  all (find-setses all v))
                   (receive (vars vals . body)
                            (union (find-sets vals v)
                                   (find-setses body (set-minus v (dotted->proper vars)))))
                   (else        (find-setses x   v)))
    '()))

(def (compile-refer var e next)
  (compile-lookup var e
                  (^(n) (list* 'LREF n next))
                  (^(n) (list* 'FREF n next))
                  (^()  (list* 'GREF var next))))

(def (find-index x ls)
  (awith (ls ls
          idx 0)
    (if ls
        (if (is (car ls) x)
            idx
          (loop (cdr ls) (+ idx 1)))
      nil)))

(def (compile-lookup var e return-local return-free return-global)
  (let ((locals (car e))
        (free   (cdr e)))
    (acond ((find-index var locals)  (return-local it))
           ((find-index var free)    (return-free it))
           (else (return-global)))))

(def (tail? next)
  (is (car next) 'RET))

;;; Macro

(def (compile-defmacro name vars body e s next)
  (let1 proper-vars (check-parameters vars)
    (let ((free (intersection (union (car e)
                                     (cdr e))
                              (find-frees body '() proper-vars)))
          (sets (find-setses body proper-vars))
          (varnum (if (is vars proper-vars)
                      (len vars)
                    (list (- (len proper-vars) 1)
                          -1))))
      ;(print `(COMPILE-DEFMACRO free= ,free sets= ,sets))
      ;; Macro registeration will be done in other place.
      ;;(register-macro name (closure body-code 0 %running-stack-pointer min max))
      (collect-free free e
                    (list* 'MACRO name varnum (len free)
                           (compile-body proper-vars proper-vars body free sets s '(RET))
                           next)))))

(def (macroexpand-all exp scope-vars)
  (if (pair? exp)
      (if (member (car exp) scope-vars)
          (macroexpand-all-sub exp scope-vars)
        (let1 expanded (macroexpand-1 exp)
          (if (iso expanded exp)
              (macroexpand-all-sub exp scope-vars)
            (macroexpand-all expanded scope-vars))))
    exp))

(def (map-macroexpand-all ls svars)
  (map [macroexpand-all _ svars] ls))

(def (macroexpand-all-sub exp scope-vars)
  (record-case exp
               (quote (obj) `(quote ,obj))
               (^ (vars . body)
                  (let1 new-scope-vars (append (dotted->proper vars) scope-vars)
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
                 (let1 new-scope-vars (append (dotted->proper vars) scope-vars)
                   `(defmacro ,name ,vars ,@(map-macroexpand-all body new-scope-vars))))
               (values all
                       `(values ,@(map-macroexpand-all all scope-vars)))
               (receive (vars vals . body)
                        (let1 new-scope-vars (append (dotted->proper vars) scope-vars)
                          `(receive ,vars ,(macroexpand-all vals scope-vars)
                             ,@(map-macroexpand-all body new-scope-vars))))
               (else (map-macroexpand-all exp scope-vars))))

(def (macroexpand exp)
  (let1 expanded (macroexpand-1 exp)
    (if (iso expanded exp)
        exp
      (macroexpand expanded))))

;;

(def (eval x)
  (run-binary (compile x)))
