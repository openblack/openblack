# ----------------------------------
# Options affecting listfile parsing
# ----------------------------------
with section("parse"):

  # Specify structure for custom cmake functions
  additional_commands = {
    'vcpkg_cmake_configure': {
      'kwargs': {
        'SOURCE_PATH': '1',
        'LOG_FILE_BASE': '1',
        'NO_CHARSET_FLAG': '0',
        'PREFER_NINJA': '0',
        'WINDOWS_USE_MSBUILD': '0',
        'OPTIONS': '+',
        'OPTIONS_RELEASE': '+',
        'OPTIONS_DEBUG': '+',
        'OPTIONS_MAYBE_UNUSED_VARIABLES': '+',
      },
    },
    'vcpkg_from_github': {
      'kwargs': {
        'OUT_SOURCE_PATH': '1',
        'REPO': '1',
        'REF': '1',
        'SHA512': '1',
        'HEAD_REF': '1',
        'PATCHES': '+',
        'GITHUB_HOST': '1',
        'AUTHORIZATION_TOKEN': '1',
        'FILE_DISAMBIGUATOR': '1',
      },
    },
    'vcpkg_check_features': {
      'kwargs': {
        'OUT_FEATURE_OPTION': '1',
        'PREFIX': '1',
        'FEATURES': '2+',
        'INVERTED_FEATURES': '2+',
      },
    },
    'vcpkg_copy_tools': {
      'kwargs': {
        'TOOL_NAMES': '+',
        'SEARCH_DIR': '1',
        'DESTINATION': '1',
        'AUTO_CLEAN': '0',
      },
    },
    'get_profile_ext': {'pargs': {'nargs': 2}},
    'mark_shaders_for_compilation': {
      'kwargs': {
        'OUTPUT_DIR': 1,
        'SHADERS': '+',
        'TYPE': 1,
        'VARYING_DEF': 1
      },
      'pargs': {'flags': [], 'nargs': '*'}
    },
    'shaderc_parse': {
      'kwargs': {
        'BIN2C': 1,
        'DEFINES': '+',
        'FILE': 1,
        'INCLUDES': '+',
        'O': 1,
        'OUTPUT': 1,
        'PROFILE': 1,
        'VARYINGDEF': 1,
      },
      'pargs': {
        'flags': [
          'DEPENDS',
          'ANDROID',
          'ASM_JS',
          'IOS',
          'LINUX',
          'NACL',
          'OSX',
          'WINDOWS',
          'PREPROCESS',
          'RAW',
          'FRAGMENT',
          'VERTEX',
          'COMPUTE',
          'VERBOSE',
          'DEBUG',
          'DISASM',
          'WERROR',
        ],
        'nargs': '1+',
      },
    },
  }

# -----------------------------
# Options affecting formatting.
# -----------------------------
with section("format"):

  # How wide to allow formatted cmake files
  line_width = 80

  # How many spaces to tab for indent
  tab_size = 2

  # If true, lines are indented using tab characters (utf-8 0x09) instead of
  # <tab_size> space characters (utf-8 0x20). In cases where the layout would
  # require a fractional tab character, the behavior of the  fractional
  # indentation is governed by <fractional_tab_policy>
  use_tabchars = False

  # If <use_tabchars> is True, then the value of this variable indicates how
  # fractional indentions are handled during whitespace replacement. If set to
  # 'use-space', fractional indentation is left as spaces (utf-8 0x20). If set
  # to `round-up` fractional indentation is replaced with a single tab character
  # (utf-8 0x09) effectively shifting the column to the next tabstop
  fractional_tab_policy = 'use-space'

  # If an argument group contains more than this many sub-groups (parg or kwarg
  # groups) then force it to a vertical layout.
  max_subgroups_hwrap = 2

  # If a positional argument group contains more than this many arguments, then
  # force it to a vertical layout.
  max_pargs_hwrap = 6

  # If true, separate flow control names from their parentheses with a space
  separate_ctrl_name_with_space = True

  # If a statement is wrapped to more than one line, than dangle the closing
  # parenthesis on its own line.
  dangle_parens = True

  # If the trailing parenthesis must be 'dangled' on its on line, then align it
  # to this reference: `prefix`: the start of the statement,  `prefix-indent`:
  # the start of the statement, plus one indentation  level, `child`: align to
  # the column of the arguments
  dangle_align = 'prefix'

  # What style line endings to use in the output.
  line_ending = 'unix'

  # Format command names consistently as 'lower' or 'upper' case
  command_case = 'canonical'

  # Format keywords consistently as 'lower' or 'upper' case
  keyword_case = 'upper'

  # A list of command names which should always be wrapped
  always_wrap = []

  # If true, the argument lists which are known to be sortable will be sorted
  # lexicographicall
  enable_sort = True

  # If true, the parsers may infer whether or not an argument list is sortable
  # (without annotation).
  autosort = False

  # By default, if cmake-format cannot successfully fit everything into the
  # desired linewidth it will apply the last, most agressive attempt that it
  # made. If this flag is True, however, cmake-format will print error, exit
  # with non-zero status code, and write-out nothing
  require_valid_layout = False

  # A dictionary mapping layout nodes to a list of wrap decisions. See the
  # documentation for more information.
  layout_passes = {}

# ------------------------------------------------
# Options affecting comment reflow and formatting.
# ------------------------------------------------
with section("markup"):

  # enable comment markup parsing and reflow
  enable_markup = False
