Implementation note
===================

## Runtime stack usage
### states
* c   ... closure
* s   ... stack pointer
* f   ... frame pointer (stack index)
* ret ... next code

### Initial condition
1. f = s = 0

### Function call
1. Use FRAME opcode to create new call frame on stack:
  - `f[c][f][ret]s`
2. Push function arguments to stack in reverse order:
  - `f[c][f][ret][a3][a2][a1]s`
3. Use APPLY opcode to apply function,
   then argument number is pushed to stack:
  - `[c][f][ret][a3][a2][a1]f[argnum]s`

### Return
1. Get called argument number from stack:
  - `[c][f][ret][a3][a2][a1]f`
2. Put back previous frame:
  - f = s = 0

### Shift (tail call)
1. Put next function arguments onto stack:
  - `[c][f][ret][a3][a2][a1]f[argnum][b2][b1]s`
2. Use SHIFT opcode to remove previous function arguments:
  - `[c][f][ret][b2][b1]s`
3. Use APPLY opcode to apply function,
   then argument number is pushed to stack:
  - `[c][f][ret][b2][b1]f[argnum]s`

### Local variables
1. Initial state:
  - `...[a3][a2][a1]f[argnum]s`
2. Use ADDSP opcode to create space:
  - `...[a3][a2][a1]f[argnum][nil][nil]s`
3. Use LOCAL opcode to store initial value:
  - `...[a3][a2][a1]f[argnum][l-2][l-3]s`
4. Use LREF opcode to refer local variable:
  - `...[a3][a2][a1]f[argnum][l-2][l-3]s`
