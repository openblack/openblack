## Black & White Challenge File

The game uses the `chl` format to represent a script of the game logic such as
God AI, objectives, picking creatures, cinematics, everything.

These files are internally referred to as challenge files, they are compiled
from a proprietary scripting language into bytecode, this bytecode is then
interpreted using the LHVM.

### File Format

#### Header
The file has a header identifying the file format and version: ASCII 4 bytes
`LHVM` followed by a 4 byte integer representing the version, Black & White 1
is `7`. (Creature Isle is `8`, Black & White 2 is `12`).

#### Global Variables

A `uint32_t` at the start of this section represents the number of global variables
to read. Each global variable is just a null terminated string representing the
global variable's name. You can create a dictionary mapping like so:

```cpp
map <uint32_t, string> global_names;
for (uint32_t i = 0; i < length; i++)
  global_names[i] = read_null_terminated_string()
```

#### Code

Todo ...