;(load-binary "../../boot.bin")
;(load "./test.arc")

(test-section "dotted->proper")
(test "proper" '(1 2 3) (dotted->proper '(1 2 3)))
(test "dotted" '(1 2 3) (dotted->proper '(1 2 . 3)))
(test "symbol" '(abc)   (dotted->proper 'abc))

(test-section "set")
(test "minus" '(a c) (set-minus '(a b c) '(b d x)))

(test-complete)
