;; Show opcode

;(load "code-walker.arc")

(def (print-opcodes code)
  (vm-walker code
    (^(c recur)
      (let operands (table-get *opcode-table* (car c))
        (write (take (+ 1 (len operands)) c)) (display "\n")))))

(def (main args)
  (files-or-stdin args
                  (^(stream)
                    (awhile (read stream)
                      (print-opcodes it)))))
