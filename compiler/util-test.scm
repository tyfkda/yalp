(use gauche.test)
(test-start "util")
(load "./util")

(test-section "dotted->proper")
(test* "proper" '(1 2 3) (dotted->proper '(1 2 3)))
(test* "dotted" '(1 2 3) (dotted->proper '(1 2 . 3)))

(test-section "set")
(test* "member-yes" #t (set-member? 'b '(a b c)))
(test* "member-no" #f (set-member? 'x '(a b c)))
(test* "cons-add" '(x a b c) (set-cons 'x '(a b c)))
(test* "cons-already" '(a b c) (set-cons 'b '(a b c)))
(test* "union" '(c a b d x) (set-union '(a b c) '(b d x)))
(test* "minus" '(a c) (set-minus '(a b c) '(b d x)))
(test* "intersect" '(b d) (set-intersect '(a b c d) '(b d x)))

(test-end :exit-on-failure #t)
(exit 0)
