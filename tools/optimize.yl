;; Optimize VM code

;(load "code-walker.arc")

(defun optimize! (code)
  (vm-walker code
    (^(c recur)
      (record-case c
                   (CONST (v . _)
                          (when (is v 'nil)
                            (set-car! c 'NIL)
                            (set-cdr! c (cddr c)))
                          t)
                   (GREF (sym . _)
                         (when (is sym 'nil)
                           (set-car! c 'NIL)
                           (set-cdr! c (cddr c)))
                         t)
                   (else t)))))

(defun main (args)
  (files-or-stdin args
                  (^(stream)
                    (awhile (read stream)
                            (optimize! it)
                            (write/ss it)
                            (display "\n")))))
