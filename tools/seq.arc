;; Show opcode sequence, 2 gram

;; Usage:
;;   $ cat ../boot.bin | ../yalp -L ../boot.bin code-walker.arc seq.arc | sort | uniq -c | sort -nr

;(load "code-walker.arc")

(def *next-table* (table))
(dolist (inst instructions)
  (table-put! *next-table* (car inst) (cadr inst)))

(def (get-next-code code)
  (awith (p (table-get *next-table* (car code))
          c (cdr code))
    (if (pair? p)
        (if (is (car p) '$next)
            (car c)
          (loop (cdr p) (cdr c)))
      (if (is p '$next)
          c
        nil))))

(def (print-opcode-pairs code)
  (vm-walker code
    (^(c recur)
      (aif (get-next-code c)
        (print (list (car c) (car it))))
      t)))

(def (main args)
  (files-or-stdin args
                  (^(stream)
                    (awhile (read stream)
                      (print-opcode-pairs it)))))
