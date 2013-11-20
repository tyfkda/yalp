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

;; Compiles lisp code into vm code.
;;   x : code to be compiled.
;;   e : current environment, ((local-vars ...) free-vars ...)
;;   s : sets variables, (sym1 sym2 ...)
;;   @result : compiled code (list)
(define compile
  (lambda (x e s next)
    (cond
     ((symbol? x)
      (compile-refer x e
                     (if (set-member? x s)
                         (list 'INDIRECT next)
                       next)))
     ((pair? x)
      (record-case x
                   (quote (obj) (list 'CONSTANT obj next))
                   (^ (vars . bodies)
                     (compile-lambda vars bodies e s next))
                   (if (test then . rest)
                       (let ((thenc (compile then e s next))
                             (elsec (cond ((null? rest)
                                           (compile-undef e s next))
                                          ((null? (cdr rest))
                                           (compile (car rest) e s next))
                                          (else
                                           (compile `(if ,@rest) e s next)))))
                         (compile test e s (list 'TEST thenc elsec))))
                   (set! (var x)
                         (compile-lookup var e
                                         (lambda (n)   (compile x e s (list 'ASSIGN-LOCAL n next)))
                                         (lambda (n)   (compile x e s (list 'ASSIGN-FREE n next)))
                                         (lambda (sym) (compile x e s (list 'ASSIGN-GLOBAL sym next)))))
                   (call/cc (x)
                            (let ((c (list 'CONTI
                                           (list 'ARGUMENT
                                                 (compile x e s
                                                          (if (tail? next)
                                                              (list 'SHIFT
                                                                    1
                                                                    '(APPLY 1))
                                                            '(APPLY 1)))))))
                              (if (tail? next)
                                  c
                                (list 'FRAME next c))))
                   (defmacro (name vars . bodies)
                     (register-macro name vars bodies)
                     (compile `(quote ,name) e s next))
                   (else
                    (let ((func (car x))
                          (args (cdr x)))
                      (if (macro? func)
                          (compile-apply-macro func args e s next)
                        (compile-apply func args e s next))))))
     (else (list 'CONSTANT x next)))))

(define (compile-undef e s next)
  (list 'UNDEF next))

(define (compile-apply func args e s next)
  (let ((argnum (length args)))
    (let loop ((args args)
               (c (compile func e s
                           (if (tail? next)
                               `(SHIFT ,argnum
                                       (APPLY ,argnum))
                             `(APPLY ,argnum)))))
      (if (null? args)
          (if (tail? next)
              c
            (list 'FRAME next c))
        (loop (cdr args)
              (compile (car args)
                       e
                       s
                       (list 'ARGUMENT c)))))))

(define (compile-lambda vars bodies e s next)
  (let ((proper-vars (dotted->proper vars)))
    (let ((free (set-intersect (set-union (car e)
                                          (cdr e))
                               (find-frees bodies '() proper-vars)))
          (sets (find-setses bodies (dotted->proper proper-vars)))
          (varnum (if (eq? vars proper-vars)
                      (list (length vars) (length vars))
                    (list (- (length proper-vars) 1)
                          -1))))
      (collect-free free e
                    (list 'CLOSE
                          varnum
                          (length free)
                          (make-boxes sets proper-vars
                                      (compile-lambda-bodies proper-vars bodies free sets s))
                          next)))))

(define (compile-lambda-bodies vars bodies free sets s)
  (let ((ee (cons vars free))
        (ss (set-union sets
                       (set-intersect s free)))
        (next (list 'RETURN)))
    (let loop ((p bodies))
      (if (null? p)
          next
        (compile (car p) ee ss
                 (loop (cdr p)))))))

(define (find-frees xs b vars)
  (let ((bb (set-union (dotted->proper vars) b)))
    (let loop ((v '())
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
     ((symbol? x) (if (set-member? x b) '() (list x)))
     ((pair? x)
      (record-case (expand-macro-if-so x)
                   (^ (vars . bodies)
                     (find-frees bodies b vars))
                   (quote (obj) '())
                   (if      all (find-frees all b '()))
                   (set!    all (find-frees all b '()))
                   (call/cc all (find-frees all b '()))
                   (defmacro (name vars . bodies) (find-frees bodies b vars))
                   (else        (find-frees x   b '()))))
     (else '()))))

(define collect-free
  (lambda (vars e next)
    (if (null? vars)
        next
      (collect-free (cdr vars) e
                    (compile-refer (car vars) e
                                   (list 'ARGUMENT next))))))

(define (find-setses xs v)
  (let loop ((b '())
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
        (record-case (expand-macro-if-so x)
                     (set! (var x)
                           (set-union (if (set-member? var v) (list var) '())
                                      (find-sets x v)))
                     (^ (vars . bodies)
                       (find-setses bodies (set-minus v (dotted->proper vars))))
                     (quote   all '())
                     (if      all (find-setses all v))
                     (call/cc all (find-setses all v))
                     (defmacro (name vars . bodies)  (find-setses bodies (set-minus v (dotted->proper vars))))
                     (else        (find-setses x v)))
      '())))

(define make-boxes
  (lambda (sets vars next)
    (let f ((vars vars) (n 0))
      (if (null? vars)
          next
        (if (set-member? (car vars) sets)
            (list 'BOX n (f (cdr vars) (+ n 1)))
          (f (cdr vars) (+ n 1)))))))

(define compile-refer
  (lambda (x e next)
    (compile-lookup x e
                    (lambda (n)   (list 'REFER-LOCAL n next))
                    (lambda (n)   (list 'REFER-FREE n next))
                    (lambda (sym) (list 'REFER-GLOBAL sym next)))))

(define (find-index x ls)
  (let loop ((ls ls)
             (idx 0))
    (cond ((null? ls) #f)
          ((eq? (car ls) x) idx)
          (else (loop (cdr ls) (+ idx 1))))))

(define compile-lookup
  (lambda (x e return-local return-free return-global)
    (let ((locals (car e))
          (free   (cdr e)))
      (cond ((find-index x locals) => return-local)
            ((find-index x free) => return-free)
            (else (return-global x))))))

(define tail?
  (lambda (next)
    (eq? (car next) 'RETURN)))

;;; Macro

;; Macro hash table, (symbol => closure)
(define *macro-table* (make-hash-table))

(define (register-macro name vars bodies)
  "Compile (defmacro name (vars ...) bodies) syntax."
  (let ((closure (evaluate `(^ ,vars ,@bodies))))
    (hash-table-put! *macro-table* name closure)))

(define (macro? name)
  "Whther the given name is macro."
  (hash-table-exists? *macro-table* name))

(define (compile-apply-macro name args e s next)
  "Expand macro and compile the result."
  (let ((result (expand-macro name args)))
    (compile result e s next)))

(define (expand-macro name args)
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
  (let ((closure (hash-table-get *macro-table* name)))
    (apply-macro (length args)
                 closure
                 (push-args args
                            (make-frame '(HALT) 0 '() 0)))))

(define (expand-macro-if-so x)
  "Expand macro all if the given parameter is macro expression,
   otherwise return itself."
  (if (and (pair? x)
           (macro? (car x)))
      (let ((expanded (expand-macro (car x) (cdr x))))
        (if (equal? expanded x)
            x
          (expand-macro-if-so expanded)))
    x))

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
  (let ((min (slot-ref f 'min-arg-num))
        (max (slot-ref f 'max-arg-num)))
    (cond ((< argnum min)
           (runtime-error #`"Too few arguments: `,(slot-ref f 'name)' requires atleast ,min\, but given ,argnum"))
          ((and (>= max 0) (> argnum max))
           (runtime-error #`"Too many arguments: `,(slot-ref f 'name)' accepts atmost ,max\, but given ,argnum"))
          (else
           (let ((fn (slot-ref f 'fn))
                 (args (map (lambda (i) (index s i))
                            (iota argnum))))
             (apply fn args))))))

;;;; VM

(define VM
  (lambda (a x f c s)
    ;(print `(run stack= ,s x= ,x))
    (record-case x
                 (HALT () a)
                 (UNDEF (x)
                        (VM 'nil x f c s))
                 (REFER-LOCAL (n x)
                              (VM (index f n) x f c s))
                 (REFER-FREE (n x)
                             (VM (index-closure c n) x f c s))
                 (REFER-GLOBAL (sym x)
                               (receive (v exist?) (refer-global sym)
                                        (unless exist?
                                                (runtime-error #`"unbound variable: `,sym`"))
                                        (VM v x f c s)))
                 (INDIRECT (x)
                           (VM (unbox a) x f c s))
                 (CONSTANT (obj x)
                           (VM obj x f c s))
                 (CLOSE (nparam nfree body x)
                        (let ((min (car nparam))
                              (max (cadr nparam)))
                          (VM (closure body nfree s min max) x f c (- s nfree))))
                 (BOX (n x)
                      (index-set! f n (box (index f n)))
                      (VM a x f c s))
                 (TEST (then else)
                       (VM a (if (true? a) then else) f c s))
                 (ASSIGN-LOCAL (n x)
                               (set-box! (index f n) a)
                               (VM a x f c s))
                 (ASSIGN-FREE (n x)
                              (set-box! (index-closure c n) a)
                              (VM a x f c s))
                 (ASSIGN-GLOBAL (sym x)
                                (assign-global! sym a)
                                (VM a x f c s))
                 (CONTI (x)
                        (VM (continuation s) x f c s))
                 (NUATE (stack x)
                        (VM a x f c (restore-stack stack)))
                 (FRAME (ret x)
                        (VM a x f c (make-frame ret f c s)))
                 (ARGUMENT (x)
                           (VM a x f c (push a s)))
                 (SHIFT (n x)
                        (let ((callee-argnum (index f -1)))
                          (VM a x f c (shift-args n callee-argnum s))))
                 (APPLY (argnum)
                        (do-apply argnum a s))
                 (RETURN ()
                         (let ((argnum (index s 0)))
                           (do-return a (- s 1) argnum)))
                 (else
                  (display #`"Unknown op ,x\n")
                  (exit 1)))))

(define (make-frame ret f c s)
  (push ret (push f (push c s))))

(define (true? x)
  (not (eq? x 'nil)))

(define (do-apply argnum f s)
  (cond ((native-function? f)
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

(define (assign-global! sym val)
  (hash-table-put! *global-variable-table* sym val))

(define (refer-global sym)
  (if (hash-table-exists? *global-variable-table* sym)
      (values (hash-table-get *global-variable-table* sym) #t)
    (values #f #f)))


(define continuation
  (lambda (s)
    (closure
     (list 'REFER-LOCAL 0 (list 'NUATE (save-stack s) '(RETURN)))
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
           (runtime-error #`"Too few arguments: requires atleast ,min\, but given ,argnum"))
          ((and (>= max 0) (> argnum max))
           (runtime-error #`"Too many arguments: accepts atmost ,max\, but given ,argnum"))
          (else
           (let ((ds (if (rest-params? min max)
                         (modify-rest-params argnum min s)
                       0)))
             (let ((s2 (+ s ds))
                   (argnum2 (+ argnum ds)))
               (VM c (closure-body c) s2 c (push argnum2 s2))))))))

(define (modify-rest-params argnum min-arg-num s)
  (define (create-rest-params)
    (let loop ((acc '())
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
    (let1 code (compile x '(()) '() '(HALT))
      (VM '() code 0 '() 0))))


;;; main

(use file.util)
(use gauche.parseopt)

(define (install-native-functions)
  (define (convert-result f)
    (lambda args
      (let ((result (apply f args)))
        (case result
          ((#t) 't)
          ((#f) 'nil)
          ((()) 'nil)
          (else result)))))
  (define (assign-native! sym func min-arg-num max-arg-num)
    (let ((f (make <NativeFunc>
               :name sym
               :fn (convert-result func)
               :min-arg-num min-arg-num
               :max-arg-num max-arg-num)))
      (assign-global! sym f)))

  (assign-global! 'nil 'nil)
  (assign-global! 't 't)

  (assign-native! 'cons cons 2 2)
  (assign-native! 'car car 1 1)
  (assign-native! 'cdr cdr 1 1)
  (assign-native! 'list list 0 -1)
  (assign-native! 'list* list* 0 -1)
  (assign-native! 'consp pair? 1 1)
  (assign-native! 'append append 0 -1)
  (assign-native! '+ + 0 -1)
  (assign-native! '- - 0 -1)
  (assign-native! '* * 0 -1)
  (assign-native! '/ quotient 0 -1)

  (assign-native! 'eq eq? 2 2)
  (assign-native! 'equal equal? 2 2)
  (assign-native! '= = 2 -1)
  (assign-native! '< < 2 -1)
  (assign-native! '> > 2 -1)
  (assign-native! '<= <= 2 -1)
  (assign-native! '>= >= 2 -1)

  (assign-native! 'print print 1 1)
  (assign-native! 'display display 1 1)
  (assign-native! 'write write 1 1)
  (assign-native! 'newline newline 0 0)
  )

(define (compile-all codes)
  (let loop ((codes codes))
    (unless (null? codes)
      (let* ((code (car codes))
             (compiled (compile (car codes) '(()) '() '(HALT))))
        (write/ss compiled)
        (newline)
        (when (and (pair? code)
                   (eq? (car code) 'set!))
          (VM '() compiled 0 '() 0))
        (loop (cdr codes))))))

(define (repl)
  (display "> ") (flush)
  (let ((s (read)))
    (unless (member s `(:q :quit :exit ,(eof-object)))
      (print (evaluate s))
      (repl))))

(define (main args)
  (install-native-functions)
  (let-args (cdr args)
            ((compile "c|compile-only")
             (bin     "b|run-binary")
             . restargs)
    (cond ((null? restargs) (repl))
          (compile (dolist (filename restargs)
                     (compile-all (file->sexp-list filename)))
                   (exit 0))
          (bin (let ((codes (file->sexp-list (car restargs))))
                 (dolist (code codes)
                   (VM '() code 0 '() 0))
                 (exit 0)))
          (else (dolist (filename restargs)
                  (dolist (code (file->sexp-list filename))
                    (evaluate code)))
                (exit 0)))))
