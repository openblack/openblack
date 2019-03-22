# Black & White Challenge File

The game uses the `chl` format to represent a script of the game logic such as
God AI, objectives, picking creatures, cinematics, everything.

These files are internally referred to as challenge files, they are compiled
from a proprietary scripting language into bytecode, this bytecode is then
interpreted using the LHVM.

## File Format

## Header
The file has a header identifying the file format and version: ASCII 4 bytes
`LHVM` followed by a 4 byte integer representing the version, Black & White 1
is `7`. (Creature Isle is `8`, Black & White 2 is `12`).

## Global Variables

A `uint32_t` at the start of this section represents the number of global variables
to read. Each global variable is just a null terminated string representing the
global variable's name. You can create a dictionary mapping like so:

```cpp
std::vector<uint32_t, std::string> global_names;
for (uint32_t i = 0; i < length; i++)
  global_names[i] = read_null_terminated_string()
```

## Code

A `uint32_t` prefixes this section with the amount of instructions to read.

|             | Type     | Description                                                                                                                                                                                                                                                                                                                                                                                                                                               |
|-------------|----------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| OPCode      | uint32_t | Opcodes can be seen below.                                                                                                                                                                                                                          |
| Access      | uint32_t | Unclear what this does, possibly to push and pop variables onto different stacks.                                                                                                                                                                   |
| Data Type   | uint32_t | Usually used to specify what kind of data the instruction is going to be,working with. This is not a reliable way of determining operand data,types however. It can be useful for trying to figure out unknown,instructions.                        |
| Parameter   | uint32_t | Data used by instruction, if applicable. Most instructions use the,stack as their source of data. Though, obviously, the data has to get,onto the stack some how, this is where this field comes into play.                                         |
| Line Number | uint32_t | This field holds the line number that was responsible for,generating this instruction. On occasion this field will be null due to,auto generated instructions that do not link directly to any one line in,the code. These, luckily, are very few.  |

## Auto Start Scripts

A `uint32_t` at the start of this section represents the number of scripts that
need to be auto started. Each script is represented as a `uint32_t` of the
script's id read in the next section.

## Scripts

A `uint32_t` at the start of this section represents the number of scripts that
need to be read.

### Script Format
|                     | Type            | Description                                                                                     |
|---------------------|-----------------|-------------------------------------------------------------------------------------------------|
| Name                | string          | The name of the script.                                                                      |
| Source Filename     | string          | The filename of the source file this was compiled from.                                         |
| ?                   | uint32_t        | Unknown.                                                                                        |
| ?                   | uint32_t        | Unknown.                                                                                        |
| Local Variables     | uint32_t + data | Names of the local variables, read the integer and then that amount of null terminated strings. |
| Instruction Address | uint32_t        | The location of the instruction to start at in the instruction array.                           |
| Parameter Count     | uint32_t        | How many local variables are parameters.                                                        |
| Script ID        | uint32_t        | Unique ID of this script, used by instructions to call it.                                   |

## Data

A `uint32_t` prefixes the number of bytes of data that follow. This array of data is used for strings mainly and would be pointed to with a pointer.

## Opcodes

### Black & White 1

| ID | Opcode       |
|:--:| ------------ |
| 0  | `END`        |
| 1  | `WAIT`       |
| 2  | `PUSH`       |
| 3  | `POP`        |
| 4  | `ADD`        |
| 5  | `CALL`       |
| 6  | `MINUS`      |
| 7  | `UMINUS`     |
| 8  | `TIMES`      |
| 9  | `DIVIDE`     |
| 10 | `MODULUS`    |
| 11 | `NOT`        |
| 12 | `AND`        |
| 13 | `OR`         |
| 14 | `EQ`         |
| 15 | `NEQ`        |
| 16 | `GEQ`        |
| 17 | `LEQ`        |
| 18 | `GT`         |
| 19 | `LT`         |
| 20 | `JUMP`       |
| 21 | `SLEEP`      |
| 22 | `EXCEPT`     |
| 23 | `INTCAST`    |
| 24 | `RUN`        |
| 25 | `ENDEXCEPT`  |
| 26 | `RETEXCEPT`  |
| 27 | `FAILEXCEPT` |
| 28 | `BRKEXCEPT`  |
| 29 | `SWAP`       |
| 30 | `LINE`       |