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

(add-load-path ".")
(load "util.scm")

(define *exit-compile* #f)

(define (compile-error . args)
  (print args)
  (*exit-compile* #f))

(define (compile x)
  (call/cc
    (lambda (cc)
      (set! *exit-compile* cc)
      (compile-recur (macroexpand-all x ())
                     '(()) () '(HALT)))))

(define (member-if f ls)
  (cond ((null? ls) #f)
        ((f (car ls)) ls)
        (else (member-if f (cdr ls)))))

;; Compiles lisp code into vm code.
;;   x : code to be compiled.
;;   e : current environment, ((local-vars ...) free-vars ...)
;;   s : sets variables, (sym1 sym2 ...)
;;   @result : compiled code (list)
(define compile-recur
  (lambda (x e s next)
    (cond
     ((symbol? x)
      (compile-refer x e
                     (if (set-member? x s)
                         (list 'UNBOX next)
                       next)))
     ((pair? x)
      (record-case x
                   (quote (obj) (list 'CONST obj next))
                   (^ (vars . body)
                     (compile-lambda vars body e s next))
                   (if (test then . rest)
                       (let ((thenc (compile-recur then e s next))
                             (elsec (cond ((null? rest)
                                           (compile-undef next))
                                          ((null? (cdr rest))
                                           (compile-recur (car rest) e s next))
                                          (else
                                           (compile-recur `(if ,@rest) e s next)))))
                         (compile-recur test e s (list 'TEST thenc elsec))))
                   (set! (var x)
                         (compile-lookup var e
                                         (lambda (n) (compile-recur x e s (list 'LSET n next)))
                                         (lambda (n) (compile-recur x e s (list 'FSET n next)))
                                         (lambda ()  (compile-recur x e s (list 'GSET var next)))))
                   (def (var x)
                        (compile-recur x e s (list 'DEF var next)))
                   (call/cc (x)
                            (let ((c (list 'CONTI (if (tail? next) 't 'nil)
                                           (list 'PUSH
                                                 (compile-recur x e s
                                                                (if (tail? next)
                                                                    (list 'SHIFT
                                                                          1
                                                                          '(APPLY 1))
                                                                  '(APPLY 1)))))))
                              (if (tail? next)
                                  c
                                (list 'FRAME next c))))
                   (defmacro (name vars . body)
                     (compile-defmacro name vars body next))
                   (else
                    (let ((func (car x))
                          (args (cdr x)))
                      (compile-apply func args e s next)))))
     (else (list 'CONST x next)))))

(define (compile-undef next)
  (list 'UNDEF next))

(define (compile-apply func args e s next)
  (let ((argnum (length args)))
    (let loop ((args args)
               (c (compile-recur func e s
                                 (if (tail? next)
                                     `(SHIFT ,argnum
                                             (APPLY ,argnum))
                                   `(APPLY ,argnum)))))
      (if (null? args)
          (if (tail? next)
              c
            (list 'FRAME next c))
        (loop (cdr args)
              (compile-recur (car args)
                             e
                             s
                             (list 'PUSH c)))))))

(define (compile-lambda vars body e s next)
  (let ((proper-vars (dotted->proper vars)))
    (if (member-if (lambda (x) (not (symbol? x))) proper-vars)
        (compile-error "Function parameter must be symbol")
      (let ((free (set-intersect (set-union (car e)
                                            (cdr e))
                                 (find-frees body () proper-vars)))
            (sets (find-setses body (dotted->proper proper-vars)))
            (varnum (if (eq? vars proper-vars)
                        (list (length vars) (length vars))
                      (list (- (length proper-vars) 1)
                            -1))))
        (collect-free free e
                      (list 'CLOSE
                            varnum
                            (length free)
                            (make-boxes sets proper-vars
                                        (compile-lambda-body proper-vars body free sets s))
                            next))))))

(define (compile-lambda-body vars body free sets s)
  (let ((ee (cons vars free))
        (ss (set-union sets
                       (set-intersect s free)))
        (next (list 'RET)))
    (if (null? body)
        (compile-undef next)
      (let loop ((p body))
        (if (null? p)
            next
          (compile-recur (car p) ee ss
                         (loop (cdr p))))))))

(define (find-frees xs b vars)
  (let ((bb (set-union (dotted->proper vars) b)))
    (let loop ((v ())
               (p xs))
      (if (null? p)
          v
        (loop (set-union v (find-free (car p) bb))
              (cdr p))))))

;; Find free variables.
;; This does not consider upper scope, so every symbol except under scope
;; are listed up.
(define find-free
  (lambda (x b)
    (cond
     ((symbol? x) (if (set-member? x b) () (list x)))
     ((pair? x)
      (record-case x
                   (^ (vars . body)
                     (find-frees body b vars))
                   (quote (obj) ())
                   (if      all (find-frees all b ()))
                   (set! (var exp)
                         (set-union (if (set-member? var b) () (list var))
                                    (find-free exp b)))
                   (def (var exp)
                        (set-union (if (set-member? var b) () (list var))
                                   (find-free exp b)))
                   (call/cc all (find-frees all b ()))
                   (defmacro (name vars . body) (find-frees body b vars))
                   (else        (find-frees x   b ()))))
     (else ()))))

(define collect-free
  (lambda (vars e next)
    (if (null? vars)
        next
      (collect-free (cdr vars) e
                    (compile-refer (car vars) e
                                   (list 'PUSH next))))))

(define (find-setses xs v)
  (let loop ((b ())
             (p xs))
    (if (null? p)
        b
      (loop (set-union b (find-sets (car p) v))
            (cdr p)))))

;; Find assignment expression for local variables to make them boxing.
;; Boxing is needed to keep a value for continuation.
(define find-sets
  (lambda (x v)
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
      ())))

(define make-boxes
  (lambda (sets vars next)
    (let f ((vars vars) (n 0))
      (if (null? vars)
          next
        (if (set-member? (car vars) sets)
            (list 'BOX n (f (cdr vars) (+ n 1)))
          (f (cdr vars) (+ n 1)))))))

(define compile-refer
  (lambda (var e next)
    (compile-lookup var e
                    (lambda (n) (list 'LREF n next))
                    (lambda (n) (list 'FREF n next))
                    (lambda ()  (list 'GREF var next)))))

(define (find-index x ls)
  (let loop ((ls ls)
             (idx 0))
    (cond ((null? ls) #f)
          ((eq? (car ls) x) idx)
          (else (loop (cdr ls) (+ idx 1))))))

(define compile-lookup
  (lambda (var e return-local return-free return-global)
    (let ((locals (car e))
          (free   (cdr e)))
      (cond ((find-index var locals) => return-local)
            ((find-index var free) => return-free)
            (else (return-global))))))

(define tail?
  (lambda (next)
    (eq? (car next) 'RET)))

;;; Macro

(define (compile-defmacro name vars body next)
  (let ((proper-vars (dotted->proper vars)))
    (let ((min (if (eq? vars proper-vars) (length vars) (- (length proper-vars) 1)))
          (max (if (eq? vars proper-vars) (length vars) -1))
          (body-code (compile-lambda-body proper-vars body (list proper-vars) () ())))
      ;; Macro registeration will be done in other place.
      ;(register-macro name (closure body-code 0 %running-stack-pointer min max))
      (list 'MACRO
            name
            (list min max)
            body-code
            next))))

;; Macro hash table, (symbol => closure)
(define *macro-table* (make-hash-table))

(define (register-macro name closure)
  "Compile (defmacro name (vars ...) body) syntax."
  (hash-table-put! *macro-table* name closure))

(define (macro? name)
  "Whether the given name is macro."
  (hash-table-exists? *macro-table* name))

;; Expand macro if the given expression is macro expression,
;; otherwise return itself.
(define (my-macroexpand-1 exp)
  (define (push-args args s)
    (let loop ((rargs (reverse args))
               (s s))
      (if (null? rargs)
          s
        (loop (cdr rargs)
              (push (car rargs) s)))))
  (define (apply-macro argnum closure s)
    (do-apply argnum closure s))

  "Expand macro."
  (if (and (pair? exp)
           (macro? (car exp)))
      (let ((name (car exp))
            (args (cdr exp)))
        (let ((closure (hash-table-get *macro-table* name)))
          (apply-macro (length args)
                       closure
                       (push-args args
                                  (make-frame '(HALT) 0 () %running-stack-pointer)))))
    exp))

(define (macroexpand-all exp scope-vars)
  (define (map-macroexpand-all ls svars)
    (map (lambda (e) (macroexpand-all e svars))
         ls))
  (define (macroexpand-all-sub xp scope-vars)
    (record-case exp
                 (quote (obj) `(quote ,obj))
                 (^ (vars . body)
                   (let ((new-scope-vars (append (dotted->proper vars) scope-vars)))
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
                   (let ((new-scope-vars (append (dotted->proper vars) scope-vars)))
                     `(defmacro ,name ,vars ,@(map-macroexpand-all body new-scope-vars))))
                 (else (map-macroexpand-all exp scope-vars))))

  (if (pair? exp)
      (if (not (member (car exp) scope-vars))
          (let ((expanded (my-macroexpand-1 exp)))
            (if (equal? expanded exp)
                (macroexpand-all-sub exp scope-vars)
              (macroexpand-all expanded scope-vars)))
        (macroexpand-all-sub exp scope-vars))
    exp))

(define (my-macroexpand exp)
  (let ((expanded (my-macroexpand-1 exp)))
    (if (equal? expanded exp)
        exp
      (my-macroexpand expanded))))

;;;; runtime

(define *stack* (make-vector 1000))

(define push
  (lambda (x s)
    (vector-set! *stack* s x)
    (+ s 1)))

(define index
  (lambda (s i)
    (vector-ref *stack* (- (- s i) 1))))

(define index-set!
  (lambda (s i v)
    (vector-set! *stack* (- (- s i) 1) v)))


;;; Native function

(define-class <NativeFunc> ()
  ((name :init-keyword :name)
   (fn :init-keyword :fn)
   (min-arg-num :init-keyword :min-arg-num)
   (max-arg-num :init-keyword :max-arg-num)
   ))

(define (native-function? f)
  (is-a? f <NativeFunc>))

(define-method call-native-function ((f <NativeFunc>) s argnum)
  (let ((args (map (lambda (i) (index s i))
                   (iota argnum))))
    (call-native-function f args)))

(define-method call-native-function ((f <NativeFunc>) (args <list>))
  (let ((min (slot-ref f 'min-arg-num))
        (max (slot-ref f 'max-arg-num))
        (argnum (length args)))
    (cond ((< argnum min)
           (runtime-error #`"Too few arguments: `,(slot-ref f 'name)' requires at least ,min\, but given ,argnum"))
          ((and (>= max 0) (> argnum max))
           (runtime-error #`"Too many arguments: `,(slot-ref f 'name)' accepts at most ,max\, but given ,argnum"))
          (else
           (let ((fn (slot-ref f 'fn)))
             (apply fn args))))))

;;;; VM

(define %running-stack-pointer 0)

(define VM
  (lambda (a x f c s)
    ;(print `(run stack= ,s x= ,x))
    (record-case x
                 (HALT ()
                       (set! %running-stack-pointer s)
                       a)
                 (UNDEF (x)
                        (VM () x f c s))
                 (CONST (obj x)
                        (VM obj x f c s))
                 (LREF (n x)
                       (VM (index f n) x f c s))
                 (FREF (n x)
                       (VM (index-closure c n) x f c s))
                 (GREF (sym x)
                       (receive (v exist?) (refer-global sym)
                         (unless exist?
                           (runtime-error #`"unbound variable: `,sym`"))
                         (VM v x f c s)))
                 (LSET (n x)
                       (set-box! (index f n) a)
                       (VM a x f c s))
                 (FSET (n x)
                       (set-box! (index-closure c n) a)
                       (VM a x f c s))
                 (GSET (sym x)
                       (when (not (assign-global! sym a))
                         (runtime-error #`"Global variable `,sym` not defined"))
                       (VM a x f c s))
                 (DEF (sym x)
                      (define-global! sym a)
                      (VM a x f c s))
                 (PUSH (x)
                       (VM a x f c (push a s)))
                 (TEST (then else)
                       (VM a (if (true? a) then else) f c s))
                 (CLOSE (nparam nfree body x)
                        (let ((min (car nparam))
                              (max (cadr nparam)))
                          (VM (closure body nfree s min max) x f c (- s nfree))))
                 (FRAME (ret x)
                        (VM a x f c (make-frame ret f c s)))
                 (APPLY (argnum)
                        (do-apply argnum a s))
                 (RET ()
                      (let ((argnum (index s 0)))
                        (do-return a (- s 1) argnum)))
                 (SHIFT (n x)
                        (let ((callee-argnum (index f -1)))
                          (VM a x f c (shift-args n callee-argnum s))))
                 (BOX (n x)
                      (index-set! f n (box (index f n)))
                      (VM a x f c s))
                 (UNBOX (x)
                        (VM (unbox a) x f c s))
                 (CONTI (tail x)
                        (VM (continuation s tail) x f c s))
                 (NUATE (tail stack)
                        (let* ((argnum (index f -1))
                               (a (if (eq? argnum 0) 'nil (index f 0))))
                          (let* ((s (restore-stack stack))
                                 (s (if (true? tail)
                                        (let ((callee-argnum (index s 0)))
                                          (- s callee-argnum 1))
                                      s)))
                            (do-return a s 0))))
                 (MACRO (name nparam body x)
                        (let ((min (car nparam))
                              (max (cadr nparam)))
                          (register-macro name (closure body 0 s min max))
                          (VM a x f c s)))
                 (else
                  (display #`"Unknown op ,x\n")
                  (exit 1)))))

(define (make-frame ret f c s)
  (push ret (push f (push c s))))

(define (true? x)
  (not (or (eq? x ())
           (eq? x 'nil))))

(define (do-apply argnum f s)
  (cond ((native-function? f)
         (set! %running-stack-pointer s)
         (let ((res (call-native-function f s argnum)))
           (do-return res s argnum)))
        ((closure? f)
         (call-closure f s argnum))
        (else
         (runtime-error #`"invalid application: ,f"))))

(define (do-return a s argnum)
  (let ((s (- s argnum)))
    (VM a (index s 0) (index s 1) (index s 2) (- s 3))))

(define (runtime-error msg)
  (display msg (standard-error-port))
  (exit 1))


;;; Global variable table


(define *global-variable-table* (make-hash-table))

(define (define-global! sym val)
  (hash-table-put! *global-variable-table* sym val))

(define (assign-global! sym val)
  (if (hash-table-exists? *global-variable-table* sym)
      (begin
        (hash-table-put! *global-variable-table* sym val)
        #t)
    #f))

(define (refer-global sym)
  (if (hash-table-exists? *global-variable-table* sym)
      (values (hash-table-get *global-variable-table* sym) #t)
    (values #f #f)))


(define continuation
  (lambda (s tail)
    (closure
     (list 'NUATE tail (save-stack s))
     s
     s
     0 1)))

(define-class <Closure> ()
  ((body :init-keyword :body)
   (free-variables :init-keyword :free-variables)
   (min-arg-num :init-keyword :min-arg-num)
   (max-arg-num :init-keyword :max-arg-num)
   ))

(define closure
  (lambda (body nfree s min-arg-num max-arg-num)
    (let* ((free-variables (make-vector nfree))
           (c (make <Closure>
                :min-arg-num min-arg-num
                :max-arg-num max-arg-num
                :body body
                :free-variables free-variables)))
      (let loop ((i 0))
        (unless (= i nfree)
          (vector-set! free-variables i (index s i))
          (loop (+ i 1))))
      c)))

(define-method call-closure ((c <Closure>) s argnum)
  (define (rest-params? min max)
    (< max 0))
  (let ((min (slot-ref c 'min-arg-num))
        (max (slot-ref c 'max-arg-num)))
    (cond ((< argnum min)
           (runtime-error #`"Too few arguments: requires at least ,min\, but given ,argnum"))
          ((and (>= max 0) (> argnum max))
           (runtime-error #`"Too many arguments: accepts at most ,max\, but given ,argnum"))
          (else
           (let ((ds (if (rest-params? min max)
                         (modify-rest-params argnum min s)
                       0)))
             (let ((s2 (+ s ds))
                   (argnum2 (+ argnum ds)))
               (VM c (closure-body c) s2 c (push argnum2 s2))))))))

(define (modify-rest-params argnum min-arg-num s)
  (define (create-rest-params)
    (let loop ((acc ())
               (i (- argnum 1)))
      (if (>= i min-arg-num)
          (loop (cons (index s i) acc)
                (- i 1))
        acc)))
  (define (unshift-args argnum s)
    (let loop ((i 0))
      (when (< i argnum)
        (index-set! s (- i 1) (index s i))
        (loop (+ i 1)))))

  (let ((rest (create-rest-params))
        (ds (if (<= argnum min-arg-num)
                (begin (unshift-args min-arg-num s)
                       1)
              0)))
      (index-set! (+ s ds) min-arg-num rest)
      ds))

(define (closure? c)
  (is-a? c <Closure>))

(define closure-body
  (lambda (c)
    (slot-ref c 'body)))

(define index-closure
  (lambda (c n)
    (vector-ref (slot-ref c 'free-variables) n)))

(define save-stack
  (lambda (s)
    (let ((v (make-vector s)))
      (let copy ((i 0))
        (unless (= i s)
          (vector-set! v i (vector-ref *stack* i))
          (copy (+ i 1))))
      v)))

(define restore-stack
  (lambda (v)
    (let ((s (vector-length v)))
      (let copy ((i 0))
        (unless (= i s)
          (vector-set! *stack* i (vector-ref v i))
          (copy (+ i 1))))
      s)))

(define box
  (lambda (x)
    (cons '=box= x)))

(define unbox
  (lambda (bx)
    (unless (eq? (car bx) '=box=)
      (runtime-error #`"box expected, but `,bx`"))
    (cdr bx)))

(define set-box!
  (lambda (x v)
    (set-cdr! x v)))

(define shift-args
  (lambda (n m s)
    (let nxtarg ((i (- n 1)))
      (unless (< i 0)
        (index-set! s (+ i m 1) (index s i))
        (nxtarg (- i 1))))
    (- s m 1)))

(define evaluate
  (lambda (x)
    (let1 code (compile x)
      (if code
          (run-binary code)
        #f))))

(define (run-binary code)
  (VM () code 0 () %running-stack-pointer))


;;; main

(use file.util)
(use gauche.parseopt)

(define (my-apply f . params)
  (let ((args (apply list* params)))
    (cond ((native-function? f)
           (call-native-function f args))
          ((closure? f)
           (let* ((s (make-frame '(HALT) 0 () %running-stack-pointer))
                  (s2 (let loop ((p args))
                        (if (null? p)
                            s
                          (push (car p) (loop (cdr p)))))))
             (call-closure f s2 (length args))))
          (else
           (runtime-error #`"invalid application: ,f")))))

(define (install-native-functions)
  (define (all f ls)
    (cond ((null? ls) #t)
          ((f (car ls)) (all f (cdr ls)))
          (else #f)))
  (define (convert x)
    (case x
      ((#t) 't)
      ((#f) ())
      ((nil) ())
      ;((()) 'nil)
      (else x)))
  (define (convert-result f)
    (lambda args
      (convert (apply f args))))
  (define (convert-inputs f)
    (lambda args
      (convert (apply f (map convert args)))))
  (define (define-native! sym func min-arg-num max-arg-num)
    (let ((f (make <NativeFunc>
               :name sym
               :fn (convert-result func)
               :min-arg-num min-arg-num
               :max-arg-num max-arg-num)))
      (define-global! sym f)))

  (define-global! 'nil ())
  (define-global! 't 't)

  (define-native! 'cons cons 2 2)
  (define-native! 'car car 1 1)
  (define-native! 'cdr cdr 1 1)
  (define-native! 'set-car! (lambda (x v) (set-car! x v) v) 2 2)
  (define-native! 'set-cdr! (lambda (x v) (set-cdr! x v) v) 2 2)
  (define-native! 'list list 0 -1)
  (define-native! 'list* list* 0 -1)
  (define-native! 'pair? pair? 1 1)
  (define-native! 'symbol? (lambda (x)
                             (or (symbol? x)
                                 (eq? x ()))) 1 1)
  (define-native! 'append append 0 -1)
  (define-native! '+ + 0 -1)
  (define-native! '- - 0 -1)
  (define-native! '* * 0 -1)
  (define-native! '/ (lambda args
                       (if (all (cut is-a? <> <integer>) args)
                           (apply quotient args)
                         (exact->inexact (apply / args)))) 0 -1)

  (define-native! 'is (convert-inputs eq?) 2 2)
  (define-native! 'iso (convert-inputs equal?) 2 2)
  (define-native! '= = 2 -1)
  (define-native! '< < 2 -1)
  (define-native! '> > 2 -1)
  (define-native! '<= <= 2 -1)
  (define-native! '>= >= 2 -1)

  (define-native! 'print print 1 1)
  (define-native! 'display display 1 1)
  (define-native! 'write (lambda (x)
                           (write (if (eq? x ()) 'nil x))) 1 1)

  (define-native! 'uniq gensym 0 0)
  (define-native! 'macroexpand-1 my-macroexpand-1 1 1)
  (define-native! 'macroexpand my-macroexpand 1 1)
  (define-native! 'apply my-apply 2 -1)
  (define-native! 'read read 0 0)
  (define-native! 'compile compile 1 1)
  (define-native! 'run-binary run-binary 1 1)
  (define-native! 'eval evaluate 1 1)

  (define-native! 'make-hash-table make-hash-table 0 0)
  (define-native! 'hash-table-get (lambda (h k)
                                    (if (hash-table-exists? h k)
                                        (hash-table-get h k)
                                      'nil)) 2 2)
  (define-native! 'hash-table-put! (lambda (h k v)
                                     (hash-table-put! h k v)
                                     v) 3 3)
  (define-native! 'hash-table-exists? hash-table-exists? 2 2)
  (define-native! 'hash-table-delete! hash-table-delete! 2 2)
  )

(define (compile-all out-port codes)
  (let loop ((codes codes))
    (unless (null? codes)
      (let* ((code (car codes))
             (compiled (compile (car codes))))
        (write/ss compiled out-port)
        (newline out-port)
        (run-binary compiled)
        (loop (cdr codes))))))

(define (repl tty?)
  (when tty?
    (display "type ':q' to quit\n"))
  (let loop ()
    (when tty?
      (display "> ") (flush))
    (let ((s (read)))
      (unless (member s `(:q :quit :exit ,(eof-object)))
        (let ((result (evaluate s)))
          (cond (tty? (display "=> ") (print result))
                ((eq? result #f) (exit 1))))
        (loop))))
  (when tty? (print "bye\n")))

(define (read-all cmdlineargs)
  "Read all S-expression from files specified with command line arguments. \\
   if no arguments specified, read from standard input port."
  (if (null? cmdlineargs)
      (let loop ((acc ())
                 (s (read)))
        (if (eof-object? s)
            (reverse! acc)
          (loop (cons s acc) (read))))
    (let loop ((acc ())
               (p cmdlineargs))
      (if (null? p)
          acc
        (let ((filename (car p)))
          (loop (append acc (file->sexp-list filename))
                (cdr p)))))))

(define (main args)
  (define (run-main)
    (let ((main (refer-global 'main)))
      (when main
        (evaluate '(main)))))

  (install-native-functions)
  (let-args (cdr args)
      ((is-compile "c|compile-only")
       (bin        "b|run-binary")
       (lib        "l|library=s" => (lambda (n) (dolist (exp (file->sexp-list n))
                                                  (evaluate exp))))
       (binlib     "L|binary-library=s" => (lambda (n) (dolist (exp (file->sexp-list n))
                                                         (run-binary exp))))
       . restargs)
    (cond (is-compile (let ((out-port (standard-output-port)))
                        (with-output-to-port (open-output-string)  ; Ignores normal output.
                          (lambda ()
                            (compile-all out-port (read-all restargs))))
                        (exit 0)))
          (bin (dolist (code (read-all restargs))
                 (run-binary code))
               (run-main))
          ((null? restargs)
           (let ((tty? (sys-isatty (standard-input-port))))
             (repl tty?)
             (when (not tty?)
               (run-main))))
          (else (dolist (code (read-all restargs))
                  (when (eq? (evaluate code) #f)
                    (exit 1)))
                (run-main))))
  (exit 0))
