(use gauche.test)
(test-start "compiler")
(load "./compiler")

(test-section "find-free")
(test* "symbol" '(symbol) (find-free 'symbol '()))
(test* "lambda" '(a c) (find-free '(^(b) a b c) '()))
(test* "nested" '(d a) (find-free '(^(b) d (^(c) a b c)) '()))
(test* "if" '(b a c) (find-free '(if a b c) '()))
(test* "set!" '(x) (find-free '(set! x 123) '()))
(test* "call/cc" '(x) (find-free '(call/cc (^(cc) (cc x))) '()))
(test* "defmacro" '(y) (find-free '(defmacro foo (x) x y) '()))
(test* "other" '(y) (find-free '((^(x) x) y) '()))

(test-section "find-frees")
(test* "dotted" '(c) (find-frees '(a b c) '() '(a . b)))

(test-section "find-sets")
(test* "set!" '(x) (find-sets '(set! x 123) '(x)))
(test* "symbol" '() (find-sets 'symbol '(symbol)))
(test* "lambda" '() (find-sets '(^(x) (set! x 123)) '(x)))
(test* "lambda" '(y) (find-sets '(^(x) (set! y 123)) '(x y)))
(test* "quote" '() (find-sets '(quote x) '(x)))
(test* "if" '(x y) (find-sets '(if a (set! x 1) (set! y 2)) '(x y)))
(test* "call/cc" '(x) (find-sets '(call/cc (^(cc) (set! x 123))) '(x)))
(test* "defmacro" '(y) (find-sets '(defmacro foo (x) (set! x 1) (set! y 2)) '(x y)))
(test* "other" '(y) (find-sets '((^(x) (set! x 1)) (set! y 2)) '(x y)))
(test* "dotted-params" '() (find-sets '(^(a . x) (set! x 123)) '(x)))

(test-end :exit-on-failure #t)
(exit 0)

;;