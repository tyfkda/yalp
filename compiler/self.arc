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

(defn set-member? (x s)
  nil)

(defn compile (x e s next)
  (if (symbolp x)
        (compile-refer x e
                       (if (set-member? x s)
                           (liset 'INDIRECT next)
                           next))
      (consp x)
        (record-case x
                     (quote (obj) (list 'CONSTANT obj next))
                     (^ (vars . bodies)
                        (compile-lambda vars bodies e s next))
                     (if (test then . rest)
                         (with (thenc (compile then e s next)
                                elsec (if (no rest)
                                            (compile-undef e s next)
                                          (no (cdr rest))
                                            (compile (car rest) e s next)
                                          (compile `(if ,@rest) e s next)))
                           (compile test e s (list 'TEST thenc elsec))))
                     (set! (var x)
                           (compile-lookup var e
                                           (^(n)   (compile x e s (list 'ASSIGN-LOCAL n next)))
                                           (^(n)   (compile x e s (list 'ASSIGN-FREE n next)))
                                           (^(sym) (compile x e s (list 'ASSIGN-GLOBAL sym next)))))
                     (call/cc (x)
                              (let c (list 'CONTI
                                           (list 'ARGUMENT
                                                 (compile x e s
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
                       (compile `(quote ,name) e s next))
                     (else
                      (with (func (car x)
                             args (cdr x))
                        (if (macro? func)
                            (compile-apply-macro func args e s next)
                            (compile-apply func args e s next)))))
        (list 'CONSTANT x next)))

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
                    (compile (car args)
                             e
                             s
                             (list 'ARGUMENT c)))))
     args
     (compile func e s
              (if (tail? next)
                  `(SHIFT ,argnum
                          (APPLY ,argnum))
                  `(APPLY ,argnum))))))

(defn compile-refer (x e next)
  (compile-lookup x e
                  (^(n)   (list 'REFER-LOCAL n next))
                  (^(n)   (list 'REFER-FREE n next))
                  (^(sym) (list 'REFER-GLOBAL sym next))))

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
  (is (car next) 'RETURN))

;; Macro
(defn macro? (name)
  nil)

;;

(print (compile 123 '(()) '() '(HALT)))
(print (compile '(quote xyz) '(()) '() '(HALT)))
(print (compile '(if 1 2 3) '(()) '() '(HALT)))
(print (compile '(set! x 123) '(()) '() '(HALT)))
(print (compile '(foo a b c) '(()) '() '(HALT)))

;;
