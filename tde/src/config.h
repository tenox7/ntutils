/*
 * This file contains the external declarations for the configuration and
 * syntax highlighting strings.
 */


/*
 * The number of configuration functions. This is more than NUM_FUNCS to
 * allow for aliases. Note this value is the index of the last item, not
 * the actual count of items.
 */
#define CFG_FUNCS (NUM_FUNCS+4)


/*
 * Display an error message with line number.
 * errmsg is the message to display;
 * line_str is the buffer to store the line number.
 * Expects line_out, line_no and prompt_line to be defined externally.
 */
#define ERRORLINE(errmsg, line_str) \
combine_strings( line_out, errmsg, config0, my_ltoa( line_no, line_str, 10 ) );\
error( WARNING, prompt_line, line_out )


/*
 * The maximum length of a key string - largest key is 15 characters, which can
 * occur twice (two-keys are so-called for a reason) plus the space between
 * them and the terminating null.
 */
#define KEY_STR_MAX        32


#define NUM_MODES          42
#define NUM_MACRO_MODES    15

/*
 * mode indexes
 * jmh 020911: renamed to be the same as the config string
 */
#define InsertMode          0
#define IndentMode          1
#define PTabSize            2
#define LTabSize            3
#define SmartTabMode        4
#define InflateTabs         5
#define ControlZ            6
#define EndOfLineStyle      7
#define TrimTrailingBlanks  8
#define DisplayEndOfLine    9
#define WordWrapMode       10
#define LeftMargin         11
#define ParagraphMargin    12
#define RightMargin        13
#define CursorStyle        14
#define Backups            15
#define Ruler              16
#define TimeStamp          17
#define InitialCaseMode    18
#define CaseMatch          19
#define CaseIgnore         20
#define JustifyRightMargin 21
#define CursorCross        22
#define DirSort            23
#define CaseConvert        24
#define CharDef            25
#define FrameStyle         26
#define FrameSpace         27
#define Shadow             28
#define LineNumbers        29
#define UndoGroup          30
#define UndoMove           31
#define KeyName            32
#define Scancode           33
#define AutoSaveWorkspace  34
#define TrackPath          35
#define DisplayCWD         36
#define UserMenu           37
#define HelpFile           38
#define HelpTopic          39
#define Menu               40
#define QuickEdit          41


extern const CONFIG_DEFS  valid_keys[AVAIL_KEYS];
extern const char * const cfg_key[MAX_KEYS];
extern const CONFIG_DEFS  valid_func[CFG_FUNCS+1];
extern const char * const func_str[NUM_FUNCS];
extern const CONFIG_DEFS valid_modes[NUM_MODES];
extern const CONFIG_DEFS valid_syntax[SHL_NUM_FEATURES];
extern const CONFIG_DEFS valid_colors[NUM_COLORS*2+1];
extern const CONFIG_DEFS valid_macro_modes[NUM_MACRO_MODES];
extern const CONFIG_DEFS off_on[2];
extern const CONFIG_DEFS case_modes[2];
extern const CONFIG_DEFS valid_z[2];
extern const CONFIG_DEFS valid_cursor[3];
extern const CONFIG_DEFS valid_crlf[4];
extern const CONFIG_DEFS valid_wraps[3];
extern const CONFIG_DEFS valid_tabs[3];
extern const CONFIG_DEFS valid_eol[3];
extern const CONFIG_DEFS valid_string[2];
extern const CONFIG_DEFS valid_dir_sort[2];
extern const CONFIG_DEFS valid_pairs[8];

#if defined( __UNIX__ )
extern const CONFIG_DEFS valid_curse[8];
#endif

extern const CONFIG_DEFS valid_color[21];
extern const CONFIG_DEFS valid_frame[4];

#define MENU_CLEAR      0
#define MENU_HEADER     1
#define MENU_ITEM       2
#define MENU_POPOUT     3
#define MENU_POPITEM    4
#define VALID_MENU_DEFS 4
extern const CONFIG_DEFS valid_menu[VALID_MENU_DEFS+1];
