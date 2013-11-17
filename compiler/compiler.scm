;;;; Compiler
;;;; Based on 3imp.

(add-load-path ".") 
(load "util.scm")

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
                   (lambda (vars . bodies)
                     (compile-lambda vars bodies e s next))
                   (if (test then . rest)
                       (let ((thenc (compile then e s next))
                             (elsec (cond ((null? rest)
                                           (compile-undef e s next))
                                          ((null? (cdr rest))
                                           (compile (car rest) e s next))
                                          (else (error "malformed if")))))
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
                                                                    (cadr next)
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
    (recur loop ((args args)
                 (c (compile func e s
                             (if (tail? next)
                                 `(SHIFT ,argnum ,(cadr next)
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
  (let ((free (set-intersect (set-union (car e)
                                        (cdr e))
                             (find-frees bodies vars)))
        (sets (find-setses bodies vars)))
    (collect-free free e
                  (list 'CLOSE
                        (length free)
                        (make-boxes sets vars
                                    (compile-lambda-bodies vars bodies free sets s))
                        next))))

(define (compile-lambda-bodies vars bodies free sets s)
  (let ((ee (cons vars free))
        (ss (set-union sets
                       (set-intersect s free)))
        (next (list 'RETURN (length vars))))
    (recur loop ((p bodies))
           (if (null? p)
               next
             (compile (car p) ee ss
                      (loop (cdr p)))))))

(define (find-frees xs b)
  (recur loop ((v '())
               (p xs))
         (if (null? p)
             v
           (loop (set-union v (find-free (car p) b))
                 (cdr p)))))

(define find-free
  (lambda (x b)
    (cond
     ((symbol? x) (if (set-member? x b) '() (list x)))
     ((pair? x)
      (record-case (expand-macro-if-so x)
                   (lambda (vars . bodies)
                     (find-frees bodies (set-union vars b)))
                   (quote (obj) '())
                   (if      all (find-frees all b))
                   (set!    all (find-frees all b))
                   (call/cc all (find-frees all b))
                   (defmacro all '())
                   (else        (find-frees x   b))))
     (else '()))))

(define collect-free
  (lambda (vars e next)
    (if (null? vars)
        next
      (collect-free (cdr vars) e
                    (compile-refer (car vars) e
                                   (list 'ARGUMENT next))))))

(define (find-setses xs v)
  (recur loop ((b '())
               (p xs))
         (if (null? p)
             b
           (loop (set-union b (find-sets (car p) v))
                 (cdr p)))))

(define find-sets
  (lambda (x v)
    (if (pair? x)
        (record-case (expand-macro-if-so x)
                     (set! (var x)
                           (set-union (if (set-member? var v) (list var) '())
                                      (find-sets x v)))
                     (lambda (vars . bodies)
                       (find-setses bodies (set-minus v vars)))
                     (quote   all '())
                     (if      all (find-setses all v))
                     (call/cc all (find-setses all v))
                     (defmacro all '())
                     (else        (find-setses x v)))
      '())))

(define make-boxes
  (lambda (sets vars next)
    (recur f ((vars vars) (n 0))
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
  (let ((closure (evaluate `(lambda ,vars ,@bodies))))
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
  (define (apply-macro closure s)
    (do-apply s closure s))

  "Expand macro."
  (let ((closure (hash-table-get *macro-table* name)))
    (apply-macro closure
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
          (expand-macro-if-so x)))
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


;;;; VM

(define VM
  (lambda (a x f c s)
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
                                                (error #`"unbound variable: `,sym`"))
                                        (VM v x f c s)))
                 (INDIRECT (x)
                           (VM (unbox a) x f c s))
                 (CONSTANT (obj x)
                           (VM obj x f c s))
                 (CLOSE (n body x)
                        (VM (closure body n s) x f c (- s n)))
                 (BOX (n x)
                      (index-set! s n (box (index s n)))
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
                 (SHIFT (n m x)
                        (VM a x f c (shift-args n m s)))
                 (APPLY (argnum)
                        (do-apply argnum a s))
                 (RETURN (n)
                         (do-return a s n))
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
         (VM f (closure-body f) s f s))
        (else
         (error "invalid application:" f))))

(define (do-return a s argnum)
  (let ((s (- s argnum)))
    (VM a (index s 0) (index s 1) (index s 2) (- s 3))))

(define (native-function? f)
  (is-a? f <procedure>))

(define (call-native-function f s argnum)
  (let ((args (map (lambda (i) (index s i))
                   (iota argnum))))
    (apply f args)))



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
     (list 'REFER-LOCAL 0 (list 'NUATE (save-stack s) '(RETURN 0)))
     s
     s)))

(define closure
  (lambda (body n s)
    (let ((v (make-vector (+ n 1))))
      (vector-set! v 0 body)
      (recur f ((i 0))
             (unless (= i n)
               (vector-set! v (+ i 1) (index s i))
               (f (+ i 1))))
      v)))

(define (closure? f)
  (vector? f))

(define closure-body
  (lambda (c)
    (vector-ref c 0)))

(define index-closure
  (lambda (c n)
    (vector-ref c (+ n 1))))

(define save-stack
  (lambda (s)
    (let ((v (make-vector s)))
      (recur copy ((i 0))
             (unless (= i s)
               (vector-set! v i (vector-ref *stack* i))
               (copy (+ i 1))))
      v)))

(define restore-stack
  (lambda (v)
    (let ((s (vector-length v)))
      (recur copy ((i 0))
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
      (error "is not box"))
    (cdr bx)))

(define set-box!
  (lambda (x v)
    (set-cdr! x v)))

(define shift-args
  (lambda (n m s)
    (recur nxtarg ((i (- n 1)))
           (unless (< i 0)
             (index-set! s (+ i m) (index s i))
             (nxtarg (- i 1))))
    (- s m)))

(define evaluate
  (lambda (x)
    (let1 code (compile x '(()) '() '(HALT))
      (VM '() code 0 '() 0))))


;;; main

(use file.util)
(use gauche.parseopt)

(define (install-native-functions)
  (define (convert-result sym f)
    (lambda args
      (let ((result (apply f args)))
        ;(print `(,sym ,args result ,result))
        (case result
          ((#t) 't)
          ((#f) 'nil)
          ((()) 'nil)
          (else result)))))
  (define (assign-native! sym func)
    (assign-global! sym (convert-result sym func)))

  (assign-global! 'nil 'nil)
  (assign-global! 't 't)

  (assign-native! 'cons cons)
  (assign-native! 'car car)
  (assign-native! 'cdr cdr)
  (assign-native! 'list list)
  (assign-native! 'list* list*)
  (assign-native! '+ +)
  (assign-native! '- -)
  (assign-native! '* *)
  (assign-native! '/ quotient)

  (assign-native! 'eq eq?)
  (assign-native! 'equal equal?)
  (assign-native! '= =)
  (assign-native! '< <)
  (assign-native! '> >)
  (assign-native! '<= <=)
  (assign-native! '>= >=)

  (assign-native! 'print print)
  (assign-native! 'display display)
  (assign-native! 'write write)
  (assign-native! 'newline newline)
  )

(define (compile-all codes)
  (let recur ((codes codes))
    (unless (null? codes)
            (write/ss (compile (car codes) '(()) '() '(HALT)))
            (display "\n")
            (recur (cdr codes)))))

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
          (compile (let ((codes (file->sexp-list (car restargs))))
                     (compile-all codes)
                     (exit 0)))
          (bin (let ((codes (file->sexp-list (car restargs))))
                 (dolist (code codes)
                   (VM '() code 0 '() 0))
                 (exit 0)))
          (else (let ((codes (file->sexp-list (car restargs))))
                  (dolist (code codes)
                    (evaluate code))
                  (exit 0))))))
