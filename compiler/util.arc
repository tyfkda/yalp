;;;; Utility functions for compiler

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
  (if (and ls
           (or (no (pair? ls))
               (cdr (last ls))))
      ;; Symbol or dotted list: convert it to proper list.
      (awith (p ls
              acc '())
        (if (pair? p)
            (loop (cdr p)
                  (cons (car p) acc))
          (reverse! (cons p acc))))
    ;; nil or proper list: return as is.
    ls))

;;;; set

(def (set-cons x s)
  (if (member x s)
      s
    (cons x s)))

(def (set-union s1 s2)
  (if s1
      (set-union (cdr s1) (set-cons (car s1) s2))
    s2))

(def (set-minus s1 s2)
  (if s1
      (if (member (car s1) s2)
          (set-minus (cdr s1) s2)
        (cons (car s1) (set-minus (cdr s1) s2)))
    '()))

(def (set-intersect s1 s2)
  (if s1
      (if (member (car s1) s2)
          (cons (car s1) (set-intersect (cdr s1) s2))
        (set-intersect (cdr s1) s2))
    '()))
