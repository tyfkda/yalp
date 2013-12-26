;(load-binary "../../boot.bin")
;(load "./test.arc")

(test-section "functions")
;; no
(test "no-nil" t (no nil))
(test "no-t" nil (no t))

;; map
(test "map 1" '(1 4 9) (map (^(x) (* x x)) '(1 2 3)))
(test "map 3" '(111 222 333) (map + '(1 2 3) '(10 20 30) '(100 200 300)))

;; pair
(test "pair even" '((1 2) (3 4) (5 6)) (pair '(1 2 3 4 5 6)))
(test "pair odd" '((1 2) (3 4) (5)) (pair '(1 2 3 4 5)))

;; cxxr
(test "cadr" 2 (cadr '(1 2 3 4)))
(test "cddr" '(3 4) (cddr '(1 2 3 4)))

;; do
(test "do" 3 (do 1 2 3))

;; when
(test "when t" 3 (when 1 2 3))
(test "when nil" nil (when nil 2 3))

;; unless
(test "unless t" nil (unless 1 2 3))
(test "unless nil" 3 (unless nil 2 3))

;; set!
(test "set!" 2 ((^(x) (set! x 2) x) 1))
(test "set! result" 2 ((^(x) (set! x 2)) 1))
(test "multi set! result" '(3 4) ((^(x y)
                                    (set! x 3
                                          y 4)
                                    (list x y))
                                  1 2))

;; let1
(test "let1" 123 (let1 x 123 x))

;; let
(test "let" 3 (let ((x 1) (y 2)) (+ x y)))

;; with*
(test "with*" 12321 (with* (x 111 x (* x x)) x))

;; bracket
(test "[...]" '(1 4 9) (map [* _ _] '(1 2 3)))

;; awith
(test "awith" 55 (awith (i 10
                         acc 0)
                   (if (< i 0)
                       acc
                     (loop (- i 1) (+ acc i)))))

;; aif
(test "aif true" 1 (aif 1 it 2 it))
(test "aif false" nil (aif nil it nil it))

;; awhile
(test "awhile" 3 (with (x '(1 2 nil 4 5)
                        acc 0)
                     (awhile (car x)
                       (set! acc (+ acc it))
                       (set! x (cdr x)))
                     acc))

;; and
(test "and" 3 (and 1 2 3))
(test "and" nil (and 1 nil 3))

;; or
(test "or" 2 (or nil 2 3))
(test "or" nil (or nil nil nil))

;; isnt
(test "isnt" nil (isnt 1 1))
(test "isnt" t (isnt 1 2))

;; len
(test "len" 3 (len '(1 2 3)))
(test "len dotted" 3 (len '(1 2 3 . 4)))

;; last
(test "last" '(3) (last '(1 2 3)))
(test "last dotted" '(3 . 4) (last '(1 2 3 . 4)))

;; reverse
(test "reverse" '(3 2 1) (reverse '(1 2 3)))
(test "reverse dotted" '(3 2 1) (reverse '(1 2 3 . 4)))

;; member-if
(test "member-if" '(x 3) (member-if symbol? '(1 x 3)))
(test "member-if fail" nil (member-if symbol? '(1 2 3)))

;; member
(test "member" '(y z) (member 'y '(x y z)))
(test "member fail" nil (member 'a '(x y z)))

;; union
(test "union" '(c a b d x) (union '(a b c) '(b d x)))

;; intersection
(test "intersect" '(b d) (intersection '(a b c d) '(b d x)))

(test-complete)
