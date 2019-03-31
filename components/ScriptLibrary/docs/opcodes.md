# Opcodes

The virtual machine consists of a total of 31 different opcodes, each instruction also
has two additional enum values:  `access type` is either `1`: use stack or `2`: use variable.
And `data type` which can can be any of the following:
* `0x01`: Integer Value
* `0x02`: Float Value
* `0x03`: Vector Value (Tends to be 3 floats pushed and popped on the stack equally)
* `0x04`: ?
* `0x05`: ?
* `0x06`: Boolean value (`0 = false`, `1 = true`)

Below is a list of defined behaviours combinations of opcodes and data types should use,
this should be a comprehensive list of all needed to implement a fully working LHVM,
however it is unknown if compiled bytecodes have any undefined behaviours.

| Opcode | Data Type | Mnemonic     | Description                                                                                                              |
|:------:| --------- | ------------ | ------------------------------------------------------------------------------------------------------------------------ |
| `0x00` |           | `END`        | Stops the execution of the current script task.                                                                          |
| `0x01` |           | `WAIT`       | Waits at this instruction until a value can be popped from the stack.                                                    |
| `0x02` |           | `PUSH`       | Pushes a literal value onto the stack, `Value` can either be literal or a reference to a variable depending on `Access`. |
| `0x03` |           | `POP`        | Pops a value from the stack either into null or into a referenced variable.                                              |
| `0x04` |  `0x01`   | `ADD`        | Pops `2 ints`, adds them, pushes `1 int`.                                                                                |
| `0x04` |  `0x02`   | `ADDF`       | Pops `2 floats`, adds them, pushes `1 float`.                                                                            |
| `0x04` |  `0x03`   | `ADDV`       | Pops `6 vfloats`, adds them, pushes `3 vfloats`.                                                                         |
| `0x05` |           | `CALL`       | Calls a *"native"* function using the `Value` as the function identifer. (0 -> 464)                                      |
| `0x06` |  `0x01`   | `MINUS`      | Pops `2 ints`, minuses them, pushes `1 int`.                                                                             |
| `0x06` |  `0x02`   | `MINUSF`     | Pops `2 floats`, minuses them, pushes `1 float`.                                                                         |
| `0x06` |  `0x03`   | `MINUSV`     | Pops `6 vfloats`, minuses them, pushes `3 vfloats`.                                                                      |
| `0x07` |  `0x01`   | `UMINUS`     | Pops `1 int` and pushes the negated `1 int`.                                                                             |
| `0x07` |  `0x02`   | `UMINUSF`    | Pops `1 float` and pushes the negated `1 float`.                                                                         |
| `0x07` |  `0x03`   | `UMINUSV`    | Pops `3 vfloat` and pushes the negated `3 vfloat`.                                                                       |
| `0x08` |  `0x01`   | `TIMES`      | Pops `2 ints`, multiplies them, pushes `1 int`.                                                                          |
| `0x08` |  `0x02`   | `TIMESF`     | Pops `2 floats`, multiplies them, pushes `1 float`.                                                                      |
| `0x08` |  `0x03`   | `TIMESV`     | Pops `6 vfloats`, multiplies them, pushes `3 vfloats`.                                                                   |
| `0x09` |  `0x01`   | `DIVIDE`     | Pops `2 ints`, divides them, pushes `1 int`.                                                                             |
| `0x09` |  `0x02`   | `DIVIDEF`    | Pops `2 floats`, divides them, pushes `1 float`.                                                                         |
| `0x09` |  `0x03`   | `DIVIDEV`    | Pops `6 vfloats`, divides them, pushes `3 vfloats`.                                                                      |
| `0x0A` |  `0x01`   | `MODULUS`    | Pops `2 ints`, modulus them, pushes `1 int`.                                                                             |
| `0x0A` |  `0x02`   | `MODULUSF`   | Pops `2 floats`, modulus them, pushes `1 float`.                                                                         |
| `0x0B` |           | `NOT`        | Pops `1` value, checks if it's equal to zero, pushes `1 boolean`.
| `0x0C` |           | `AND`        | Pops `2` values, `val1 && val2`, pushes `1 boolean`.
| `0x0D` |           | `OR`         | Pops `2` values, `val1 || val2`, pushes `1 boolean`.
| `0x0E` |  `0x01`   | `EQ`         | Pops 2 `integer` values, checks equality, pushes 1 `boolean` value.
| `0x0E` |  `0x02`   | `EQ`         | Pops 2 `float` values, checks equality, pushes 1 `boolean` value.
| `0x0E` |  `0x0*`   | `EQ`         | Pops **zero** values, pushes 1 `false boolean` value.
| `0x0E` |  `0x06`   | `EQ`         | Pops 2 `boolean` values, checks equality, pushes 1 `boolean` value.
| `0x0F` |  `0x01`   | `NEQ`        | Pops 2 `integer` values, checks equality, pushes 1 `boolean` value.
| `0x0F` |  `0x02`   | `NEQ`        | Pops 2 `float` values, checks equality, pushes 1 `boolean` value.
| `0x0F` |  `0x0*`   | `NEQ`        | Pops **zero** values, pushes 1 `false boolean` value.
| `0x0F` |  `0x06`   | `NEQ`        | Pops 2 `boolean` values, checks equality, pushes 1 `boolean` value.
| `0x10` |  `0x01`   | `GEQ`        | Pops 2 `integer` values, checks greater than or equal to, pushes 1 `boolean` value.
| `0x10` |  `0x02`   | `GEQ`        | Pops 2 `float` values, checks greater than or equal to, pushes 1 `boolean` value.
| `0x10` |  `0x0*`   | `GEQ`        | Pops **zero**, pushes 1 `false boolean` value.
| `0x11` |  `0x01`   | `LEQ`        | Pops 2 `integer` values, checks less than or equal to, pushes 1 `boolean` value.
| `0x11` |  `0x02`   | `LEQ`        | Pops 2 `float` values, checks less than or equal to, pushes 1 `boolean` value.
| `0x11` |  `0x0*`   | `LEQ`        | Pops **zero**, pushes 1 `false boolean` value.
| `0x12` |  `0x01`   | `GT`         | Pops 2 `integer` values, checks greater than, pushes 1 `boolean` value.
| `0x12` |  `0x02`   | `GT`         | Pops 2 `float` values, checks greater than, pushes 1 `boolean` value.
| `0x12` |  `0x0*`   | `GT`         | Pops **zero**, pushes 1 `false boolean` value.
| `0x13` |  `0x01`   | `LT`         | Pops 2 `integer` values, checks less than, pushes 1 `boolean` value.
| `0x13` |  `0x02`   | `LT`         | Pops 2 `float` values, checks less than, pushes 1 `boolean` value.
| `0x13` |  `0x0*`   | `LT`         | Pops **zero**, pushes 1 `false boolean` value.
| `0x14` |           | `JUMP`       | Sets the current instruction address to the `value`.
| `0x15` |           | `SLEEP`      | Pops `1 float` from the stack, sleeps for that time, if it is interrupted it pushes a `bool value of true` onto the stack, otherwise it pushes a `bool value of false`.
| `0x16` |           | `EXCEPT`     | todo...
| `0x17` |           | `INTCAST`    | Pops `1 value`, casts it to an integer and pushes `1 integer` value.
| `0x18` |           | `RUN`        | Calls another script task. `Value` is script id.
| `0x19` |           | `ENDEXCEPT`  | todo...
| `0x1A` |           | `RETEXCEPT`  | todo...
| `0x1B` |           | `FAILEXCEPT` | todo...
| `0x1C` |           | `BRKEXCEPT`  | todo...
| `0x1D` |  `0x01`   | `SWAP`       | Pops `2 values` off the top and pushes them back in the opposite order.
| `0x1D` |           | `SWAP`       | todo...
| `0x1E` |           | `LINE`       | Nothing, just specifies the source code file line.

## Native Functions

todo: auto generate these from IDA