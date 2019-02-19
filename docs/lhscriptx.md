# LHScriptX

LHScriptX is a simple interpreted scripting language used by Black & White.

## Language

#### Commands

Commands are set at runtime and consist of a list of commands and their parameters, for instance: `VERSION` has a `F` parameter for float.

`VERSION(2.3000000)`

Commands do **NOT** return any value.

#### Variables

Variables can be defined by putting the type and variable name on a line like so: `SLONG HELP_TEXT_NARRATOR_NONE`

And then assigned to using the equals operator: `HELP_TEXT_NARRATOR_NONE = 0`

Variables can be used directly throughout the current script context and will be directly substituted with their
value. e.g:

`ADD_TEXT( 0, HELP_TEXT_NARRATOR_NONE, "HELP_TEXT_NONE", "Invalid text string.")`

#### Whitespace

All whitespace is ignored, some files have extra spaces and `\r` characters in lines
that need to be specifically ignored whilst lexing.

#### Comments

Comments can be defined in two ways:

* `REM` can prefix a line (in any case).
* `****` can wrap a line.

## Lexing

#### Tokens

`VERSION(2.3000000)` -> `VERSION`, `(`, `2.3000000`, `)`

`ADD_TEXT( 0, HELP_TEXT_NARRATOR_NONE, "HELP_TEXT_NONE", "Invalid text string.")`

->

`ADD_TEXT`, `(`, `0`, `,`, `HELP_TEXT_NARRATOR_NONE`, `,`, `"HELP_TEXT_NONE"`, `,`, `"Invalid text string."`, `)`