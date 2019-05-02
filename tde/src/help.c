/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1992
 *
 * This file contains the credit screen, main help screen and the help
 * displays for functions where help would be useful.
 *
 * jmh 991022: changed all the frames to double-line.
 * jmh 010528: added command line help screen.
 */

/*
#include "tdestr.h"
#include "define.h"
*/
#include "letters.h"
#define NULL 0

#define VERSION  "   Version 5.1v   "   /* This width must not change! */
#define DATE     "   May 1, 2007    "   /* This width must not change! */
#define PVERSION "5.1v"
#define PDATE    "May 1, 2007"


const char * const tde_help =
"\
TDE, the Thomson-Davis Editor, version "PVERSION" ("PDATE").\n\
Jason Hood <jadoxa@yahoo.com.au>.  http://tde.adoxa.cjb.net/\n\
\n\
tde [-v] [-i config] [options] [filename(s)] [options] ...\n\
tde [-v] [-i config] [options] -f|F|g|G pattern filename(s)\n\
tde [-v] [-i config] -w [workspace [...]]\n\
\n\
-v is viewer mode (everything is loaded read-only).\n\
-i will load the specified configuration file.\n\
-w will load files from, or save to, the given workspace.\n\
\n\
Filenames (and directories) can include wildcards - use '-?\?' for help.\n\
\n\
'F' is a text-based search; 'G' is a regular expression search.\n\
Lowercase letter ignores case; uppercase letter matches case.\n\
Use '-G?' for help on regular expression syntax.  Add an equal\n\
sign after the letter to create a window containing the matching\n\
lines from all files.\n\
\n\
Options can be:\n\
   a              Load all files immediately\n\
   b[n]           Binary (default n is 64; 0 will force text; negative will\n\
                   keep text files; ! will ignore binary files)\n\
   c <title>      Name the window \"title\" (\".\" to use filename)\n\
   e <macro>      Execute \"macro\" after loading each file\n\
   l[lang]        Disable syntax highlighting, or use language \"lang\"\n\
   n              Create a new (scratch) window\n\
   r              Read-only\n\
   s              Scratch\n\
   t[n]           Use tab size n (default is 8; 0 will deflate tabs)\n\
   [line][:col]   Start at the specified position (negative line number\n\
                   will be taken from the end of the file, negative column\n\
                   from the end of the line)\n\
   +offset        Move to the specified offset (prefix with 0x for hex)\n\
\n\
Options prefixed with '-' will apply to all subsequent files (except title and\n\
movement); use a trailing '-' to restore default behavior.  Options prefixed\n\
with '+' will only apply to one file.  If the option is uppercase, it will\n\
apply to the previous file.  Example:\n\
\n\
   tde -b file1 -b- file2\n\
   tde file1 +B file2\n\
\n\
will both load \"file1\" as binary and auto-detect the type of \"file2\".\n\
\n\
TDE can also be used as a filter (eg: dir | tde >dir.lst).\
";


#if STAT_COUNT > STATS_COUNT
char *stat_screen[STAT_COUNT+1];  /* used for both status and statistics */
#else
char *stat_screen[STATS_COUNT+1];
#endif

int  help_dim[2][2] = { { 25, 80 }, { 25, 80 } };


#if !defined( __UNIX__ ) || defined( PC_CHARS )

const char * const credit_screen[] = {
"ษออออออออออออออออออออออออออออออออออออออออออออออออออออป",
"บ                                                    บ",
"บ           TDE, the Thomson-Davis Editor            บ",
"บ                                                    บ",
"บ                 "    VERSION     "                 บ",
"บ                                                    บ",
"บ                    Frank Davis                     บ",
"บ                        and                         บ",
"บ                     Jason Hood                     บ",
"บ                                                    บ",
"บ                 "      DATE      "                 บ",
"บ                                                    บ",
"บ                                                    บ",
"บ      This program is released into the public      บ",
"บ   domain.  You may use and distribute it freely.   บ",
"บ                                                    บ",
"ศออออออออออออออออออออออออออออออออออออออออออออออออออออผ",
NULL
};


/*
 * The lines in this help screen need to be exactly 80 characters (otherwise
 * editor residue will be visible; however, the config will blank out unused
 * characters). There can be any number of lines, but of course it won't
 * display beyond the bottom of the screen.
 * jmh 990430: turned into a 2d array, removed the signature.
 * jmh 050710: added the viewer screen as another array.
 */
char help_screen[2][HELP_HEIGHT][HELP_WIDTH+1] = { {
"ษออออออออออออออออออออออออออออออออออออ HELP ออออออออออออออออออออออออออออออออออออป",
"บฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ  ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฟ   ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟบ",
"บณ # = Shift   @ = Alt   ^ = Ctrl ณ  ณ ^\\ for Menu ณ   ณ abort command     ^[ ณบ",
"บภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู  ภฤฤฤฤฤฤฤฤฤฤฤฤฤู   ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูบ",
"บฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤ Cursor Movement ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟฺฤฤฤ Screen Movement ฤฤฟบ",
"บณ next line, first char/begin/end        #/^/#^Enter ณณ pan left       @Left ณบ",
"บณ first column                                 #Home ณณ pan right     @Right ณบ",
"บรฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤยฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤดณ pan up           @Up ณบ",
"บณ stream left       #Left ณ stream right      #Right ณณ pan down       @Down ณบ",
"บณ word left         ^Left ณ word right        ^Right ณณ scroll up        ^Up ณบ",
"บณ string left      #^Left ณ string right     #^Right ณณ scroll down    ^Down ณบ",
"บณ word end left        @; ณ word end right        @' ณภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูบ",
"บณ string end left     #@; ณ string end right     #@' ณฺฤฤฤฤฤฤฤฤ Macro ฤฤฤฤฤฤฤฟบ",
"บณ set marker     @1 -  @3 ณ match () [] {} <> \"   ^] ณณ marker           #@M ณบ",
"บณ goto marker   #@1 - #@3 ณ previous position     @~ ณณ pause             ^P ณบ",
"บภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤมฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูบ",
"บฺฤฤฤฤฤฤฤฤฤฤฤฤฤ Insert / Delete ฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟฺฤฤฤฤฤฤฤฤฤฤฤ Block ฤฤฤฤฤฤฤฤฤฤฤฤฟบ",
"บณ insert newline, match indentation    Enter ณณ box                       @B ณบ",
"บณ next tab, add spaces if insert         Tab ณณ line                      @L ณบ",
"บณ previous tab, delete if insert        #Tab ณณ stream                    @X ณบ",
"บณ delete, join lines if at eol       ^Delete ณณ unmark / re-mark          @U ณบ",
"บณ delete word, previous word       ^T, ^Bksp ณณ adjust begin/end    #@[, #@] ณบ",
"บณ restore, undo, redo     Esc, @Bksp, #@Bksp ณณ move to begin/end    @[,  @] ณบ",
"บภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤูบ",
"ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ",
}, {

/*
 * Help screen when in viewer (read-only) mode.
 */
"ษออออออออออออออออออออออออออออออออ VIEWER HELP อออออออออออออออออออออออออออออออออป",
"บ                                                                              บ",
"บ  ฺฤฤฤฤฤฤฤ File ฤฤฤฤฤฤฤฟ   ฺฤฤฤฤฤฤฤ Location ฤฤฤฤฤฤฤฟ   ฺฤฤฤฤฤ Search ฤฤฤฤฤฟ  บ",
"บ  ณ next, prev   :n :p ณ   ณ top of file   '^ B     ณ   ณ define find  < > ณ  บ",
"บ  ณ goto, first  :x :X ณ   ณ end of file   '$ F G   ณ   ณ repeat find  , . ณ  บ",
"บ  ณ open         :e  E ณ   ณ goto line      g       ณ   ณ define regx  / ? ณ  บ",
"บ  ณ close        :q  q ณ   ณ previous      '' `     ณ   ณ repeat regx  n N ณ  บ",
"บ  ณ status       :f  = ณ   ณ set  marker   m1 m2 m3 ณ   ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู  บ",
"บ  ณ exit viewer   v    ณ   ณ goto marker   '1 '2 '3 ณ                         บ",
"บ  ณ exit TDE     :Q  Q ณ   ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู                         บ",
"บ  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู                                                      บ",
"บ                                                                              บ",
"บ  ฺฤฤฤฤฤฤฤฤฤฤฤฤฤ Window ฤฤฤฤฤฤฤฤฤฤฤฟ   ฺฤฤ Screen ฤฤฟ   ฺฤ Miscellaneous ฤฟ   บ",
"บ  ณ up     b       ณ half up     u ณ   ณ up     k y ณ   ณ shell         ! ณ   บ",
"บ  ณ down   f space ณ half down   d ณ   ณ down   j e ณ   ณ user screen   s ณ   บ",
"บ  ณ left   {       ณ half left   [ ณ   ณ left   h   ณ   ณ redraw        R ณ   บ",
"บ  ณ right  }       ณ half right  ] ณ   ณ right  l   ณ   ณ repeat        r ณ   บ",
"บ  ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤมฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู   ภฤฤฤฤฤฤฤฤฤฤฤฤู   ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู   บ",
"บ                                                                              บ",
"บ                                                                              บ",
"บ                   ฺฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฟ                   บ",
"บ                   ณ Press F1 or H for other help screen. ณ                   บ",
"บ                   ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู                   บ",
"บ                                                                              บ",
"ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ",
} };


const char * const regx_help[] = {
"ษออออออออออัอออออออออออออออออออออออออออออออออออออออออออออัออออออออออออออป",
"บ  c       ณ  any non-operator character                 ณ  Felis       บ",
"บ  \\c      ณ  c literally and C escapes (0abefnrtv)      ณ  catus\\.     บ",
"บ  \\n      ณ  backreference (1 to 9)                     ณ  (boo)\\1     บ",
"บ  \\:c     ณ  predefined character set                   ณ  \\:a+.?\\(    บ",
"บ          ณ    a - alphanumeric d - decimal l - lower   ณ              บ",
"บ          ณ    b - white space  h - hex.    u - upper   ณ              บ",
"บ          ณ    c - alphabetic   Uppercase not in set    ณ              บ",
"บ  .       ณ  any character                              ณ  c.t         บ",
"บ  ,       ณ  space or tab characters                    ณ  ,cat,       บ",
"บ  < >     ณ  beginning and end of word                  ณ  <cat>       บ",
"บ  << >>   ณ  beginning and end of string                ณ  <<cat>>     บ",
"บ  ^ $     ณ  beginning and end of line                  ณ  ^cat$       บ",
"บ  [x]     ณ  any character in x                         ณ  [\\:c0-9]    บ",
"บ  [^x]    ณ  any character not in x                     ณ  [^AEIOU]    บ",
"บ  r* r*?  ณ  zero or more r's, longest or shortest      ณ  ca*t        บ",
"บ  r+ r+?  ณ  one or more r's, longest or shortest       ณ  ca[b-t]+    บ",
"บ  r? r??  ณ  zero or one r, prefer one or zero          ณ  c.?t        บ",
"บ  rs      ณ  r followed by s                            ณ  ^$          บ",
"บ  r|s     ณ  either r or s                              ณ  kitty|cat   บ",
"บ  (r)     ณ  r                                          ณ  (c)?(a+)t   บ",
"บ  (?:r)   ณ  r, without generating a backreference      ณ              บ",
"บ  r(?=s)  ณ  match r only if s matches                  ณ  label(?=:)  บ",
"บ  r(?!s)  ณ  match r only if s does not match           ณ  kit(?!ten)  บ",
"ศออออออออออฯอออออออออออออออออออออออออออออออออออออออออออออฯออออออออออออออผ",
NULL
};


const char * const replace_help[] = {
"ษอออออออัอออออออออออออออออออออออออออออออป",
"บ  \\c   ณ  c literally and C escapes    บ",
"บ  \\n   ณ  backreference (1 to 9)       บ",
"บ  \\&   ณ  entire match                 บ",
"บ  \\+?  ณ  uppercase match or backref   บ",
"บ  \\-?  ณ  lowercase match or backref   บ",
"บ  \\^?  ณ  capitalise match or backref  บ",
"ศอออออออฯอออออออออออออออออออออออออออออออผ",
NULL
};


const char * const wildcard_help[] = {
"ษอออออออออออัออออออออออออออออออออออออออออออออออออออออออออป",
"บ  *        ณ  match zero or more characters             บ",
"บ  ?        ณ  match any one character                   บ",
"บ  [set]    ณ  match any character in the set            บ",
"บ  [!set]   ณ  match any character not in the set        บ",
"บ  [^set]   ณ  same as above                             บ",
"บ  a;b!c;d  ณ  match \"a\" or \"b\" but exclude \"c\" and \"d\"  บ",
"บ  !a;b     ณ  match everything except \"a\" and \"b\"       บ",
"บ           ณ                                            บ",
"บ  ...      ณ  enter subdirectories (eg: \".../*.c\")      บ",
"บ  dirname  ณ  same as \"dirname/*\"                       บ",
"วฤฤฤฤฤฤฤฤฤฤฤมฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ",
"บ  A set is a group (\"a1.\") and/or range (\"a-z\") of      บ",
"บ  characters.  To include '!^]-' in the set, precede    บ",
"บ  it with '\\'.  To match ';' or '!' make it part of a   บ",
"บ  set.  The names will be sorted according to the       บ",
"บ  current directory list order (name or extension).     บ",
#if defined( __UNIX__ )
"บ  ':' can be used instead of ';'.                       บ",
#endif
"ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ",
NULL
};


const char * const stamp_help[] = {
"ษอออออออัออออออออออออออออออออออออป",
"บ  %d   ณ  day of month          บ",
"บ  %D   ณ  day of week           บ",
"บ  %e   ณ  enter new line        บ",
"บ  %h   ณ  hour (12-hour)        บ",
"บ  %H   ณ  hour (24-hour)        บ",
"บ  %m   ณ  month (number)        บ",
"บ  %M   ณ  month (word)          บ",
"บ  %n   ณ  minutes               บ",
"บ  %p   ณ  am or pm              บ",
"บ  %s   ณ  seconds               บ",
"บ  %t   ณ  tab                   บ",
"บ  %y   ณ  year (two digits)     บ",
"บ  %Y   ณ  year (full)           บ",
"บ  %Z   ณ  timezone (abbr.)      บ",
"บ  %0?  ณ  zero-padded numbers,  บ",
"บ       ณ  abbreviated words     บ",
"บ  %2?  ณ  blank-padded numbers  บ",
"บ  %+?  ณ  left-aligned words    บ",
"บ  %-?  ณ  right-aligned words   บ",
"บ  %%   ณ  a percent sign        บ",
"ศอออออออฯออออออออออออออออออออออออผ",
NULL
};


const char * const border_help[] = {
"ษออออออออออออออัอออออออออออออออออออออออออออออออออออออออออออออออออออป",
"บ  # of chars  ณ                  Style characters                 บ",
"บ   in style   ณ                     represent                     บ",
"วฤฤฤฤฤฤฤฤฤฤฤฤฤฤลฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ",
"บ      0       ณ  current graphic set                              บ",
"บ      1       ณ  entire border                                    บ",
"บ      2       ณ  vertical and horizontal edges                    บ",
"บ      3       ณ  corners, vertical and horizontal edges           บ",
"บ      4       ณ  left, right, top and bottom edges                บ",
"บ      5       ณ  top-left, top-right, bottom-left,                บ",
"บ              ณ    bottom-right corners and the edges             บ",
"บ      6       ณ  the four corners, vertical and horizontal edges  บ",
"บ      8       ณ  the four corners and the four edges              บ",
"ศออออออออออออออฯอออออออออออออออออออออออออออออออออออออออออออออออออออผ",
NULL
};


const char * const exec_help[] = {
"ษอออออออออัออออออออออออออออออออออออออออป",
"บ  %[=]f  ณ  filename (relative path)  บ",
"บ  %[=]F  ณ  filename (absolute path)  บ",
"บ  %p     ณ  prompt for parameter      บ",
"บ  %w     ณ  copy the current word     บ",
"บ  %W     ณ  copy the current string   บ",
"บ  %%     ณ  a percent sign            บ",
"วฤฤฤฤฤฤฤฤฤมฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤถ",
"บ    The filename will be quoted if    บ",
"บ    needed; '=' will prevent that.    บ",
"ศออออออออออออออออออออออออออออออออออออออผ",
NULL
};


const char * const char_help[] = {
"ษออออออออออออออออออออออออออออออออออออออออป",
"บ              Character Set             บ",
"บ                                        บ",
"บ    ฺฤ0ฤ1ฤ2ฤ3ฤ4ฤ5ฤ6ฤ7ฤ8ฤ9ฤ0ฤ1ฤ2ฤ3ฤ4ฤ5ฤฟ บ",
"บ   0ณ   \x01 \x02 \x03 \x04 \x05 \x06 \x07 "
       "\x08 \x09 \x0a \x0b \x0c \x0d \x0e \x0f ณ บ",
"บ  16ณ \x10 \x11 \x12 \x13 \x14 \x15 \x16 \x17 "
       "\x18 \x19 \x1a \x1b \x1c \x1d \x1e \x1f ณ บ",
"บ  32ณ   ! \" # $ % & ' ( ) * + , - . / ณ บ",
"บ  48ณ 0 1 2 3 4 5 6 7 8 9 : ; < = > ? ณ บ",
"บ  64ณ @ A B C D E F G H I J K L M N O ณ บ",
"บ  80ณ P Q R S T U V W X Y Z [ \\ ] ^ _ ณ บ",
"บ  96ณ ` a b c d e f g h i j k l m n o ณ บ",
"บ 112ณ p q r s t u v w x y z { | } ~  ณ บ",
"บ 128ณ €     …           ณ บ",
"บ 144ณ  ‘ ’ “ ” • – —         ณ บ",
"บ 160ณ   ก ข ฃ ค ฅ ฆ ง จ ฉ ช ซ ฌ ญ ฎ ฏ ณ บ",
"บ 176ณ ฐ ฑ ฒ ณ ด ต ถ ท ธ น บ ป ผ ฝ พ ฟ ณ บ",
"บ 192ณ ภ ม ย ร ฤ ล ฦ ว ศ ษ ส ห ฬ อ ฮ ฯ ณ บ",
"บ 208ณ ะ ั า ำ ิ ี ึ ื ุ ู ฺ Û Ü Ý Þ ฿ ณ บ",
"บ 224ณ เ แ โ ใ ไ ๅ ๆ ็ ่ ้ ๊ ๋ ์ ํ ๎ ๏ ณ บ",
"บ 240ณ ๐ ๑ ๒ ๓ ๔ ๕ ๖ ๗ ๘ ๙ ๚ ๛ ü ý þ ÿ ณ บ",
"บ    ภฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤฤู บ",
"ศออออออออออออออออออออออออออออออออออออออออผ",
NULL
};


#else

const char * const credit_screen[] = {
"+----------------------------------------------------+",
"|                                                    |",
"|           TDE, the Thomson-Davis Editor            |",
"|                                                    |",
"|                 "    VERSION     "                 |",
"|                                                    |",
"|                    Frank Davis                     |",
"|                        and                         |",
"|                     Jason Hood                     |",
"|                                                    |",
"|                 "      DATE      "                 |",
"|                                                    |",
"|                                                    |",
"|      This program is released into the public      |",
"|   domain.  You may use and distribute it freely.   |",
"|                                                    |",
"+----------------------------------------------------+",
NULL
};


char help_screen[2][HELP_HEIGHT][HELP_WIDTH+1] = { {
"+==================================== HELP ====================================+",
"|+--------------------------------+  +-------------+   +----------------------+|",
"|| # = Shift   @ = Alt   ^ = Ctrl |  | ^\\ for Menu |   | abort command     ^[ ||",
"|+--------------------------------+  +-------------+   +----------------------+|",
"|+------------------ Cursor Movement -----------------++--- Screen Movement --+|",
"|| next line, first character                       - || pan left           - ||",
"|| next line, first column                          - || pan right          - ||",
"|+-------------------------+--------------------------+| pan up             - ||",
"|| word left         #Left | word end left          - || pan down           - ||",
"|| word right       #Right | word end right         - || scroll up          - ||",
"|| string left           - | string end left        - || scroll down        - ||",
"|| string right          - | string end right       - |+----------------------+|",
"|+-------------------------+--------------------------++-------- Macro -------+|",
"|| set marker    ^K! - ^K# | match () [] {} <> \"   ^] || marker           ^Km ||",
"|| goto marker   ^K1 - ^K3 | previous position      - || pause            ^Kp ||",
"|+----------------------------------------------------++----------------------+|",
"|+------------- Insert / Delete --------------++----------- Block ------------+|",
"|| insert newline, match indentation    Enter || box                       ^B ||",
"|| next tab, add spaces if insert         Tab || line                      ^L ||",
"|| previous tab, delete if insert        #Tab || stream                    ^X ||",
"|| delete, join lines if at eol             - || unmark / re-mark          ^^ +|",
"|| delete word, previous word           ^T, - || adjust begin/end    ^Kb, ^Kk ||",
"|| undo, redo                           ^U, - || move to begin/end ^K^B, ^K^K ||",
"|+--------------------------------------------++------------------------------+|",
"+==============================================================================+",
}, {

"+================================ VIEWER HELP =================================+",
"|                                                                              |",
"|  +------- File -------+   +------- Location -------+   +----- Search -----+  |",
"|  | next, prev   :n :p |   | top of file   '^ B     |   | define find  < > |  |",
"|  | goto, first  :x :X |   | end of file   '$ F G   |   | repeat find  , . |  |",
"|  | open         :e  E |   | goto line      g       |   | define regx  / ? |  |",
"|  | close        :q  q |   | previous      '' `     |   | repeat regx  n N |  |",
"|  | status       :f  = |   | set  marker   m1 m2 m3 |   +------------------+  |",
"|  | exit viewer   v    |   | goto marker   '1 '2 '3 |                         |",
"|  | exit TDE     :Q  Q |   +------------------------+                         |",
"|  +--------------------+                                                      |",
"|                                                                              |",
"|  +------------- Window -----------+   +-- Screen --+   +- Miscellaneous -+   |",
"|  | up     b       | half up     u |   | up     k y |   | shell         ! |   |",
"|  | down   f space | half down   d |   | down   j e |   | user screen   s |   |",
"|  | left   {       | half left   [ |   | left   h   |   | redraw        R |   |",
"|  | right  }       | half right  ] |   | right  l   |   | repeat        r |   |",
"|  +----------------+---------------+   +------------+   +-----------------+   |",
"|                                                                              |",
"|                                                                              |",
"|                   +--------------------------------------+                   |",
"|                   | Press F1 or H for other help screen. |                   |",
"|                   +--------------------------------------+                   |",
"|                                                                              |",
"+==============================================================================+",
} };


const char * const regx_help[] = {
"+----------+---------------------------------------------+--------------+",
"|  c       |  any non-operator character                 |  Felis       |",
"|  \\c      |  c literally and C escapes (0abefnrtv)      |  catus\\.     |",
"|  \\n      |  backreference (1 to 9)                     |  (boo)\\1     |",
"|  \\:c     |  predefined character set                   |  \\:a+.?\\(    |",
"|          |    a - alphanumeric d - decimal l - lower   |              |",
"|          |    b - white space  h - hex.    u - upper   |              |",
"|          |    c - alphabetic   Uppercase not in set    |              |",
"|  .       |  any character                              |  c.t         |",
"|  ,       |  space or tab characters                    |  ,cat,       |",
"|  < >     |  beginning and end of word                  |  <cat>       |",
"|  << >>   |  beginning and end of string                |  <<cat>>     |",
"|  ^ $     |  beginning and end of line                  |  ^cat$       |",
"|  [x]     |  any character in x                         |  [\\:c0-9]    |",
"|  [^x]    |  any character not in x                     |  [^AEIOU]    |",
"|  r* r*?  |  zero or more r's, longest or shortest      |  ca*t        |",
"|  r+ r+?  |  one or more r's, longest or shortest       |  ca[b-t]+    |",
"|  r? r??  |  zero or one r, prefer one or zero          |  c.?t        |",
"|  rs      |  r followed by s                            |  ^$          |",
"|  r|s     |  either r or s                              |  kitty|cat   |",
"|  (r)     |  r                                          |  (c)?(a+)t   |",
"|  (?:r)   |  r, without generating a backreference      |              |",
"|  r(?=s)  |  match r only if s matches                  |  label(?=:)  |",
"|  r(?!s)  |  match r only if s does not match           |  kit(?!ten)  |",
"+----------+---------------------------------------------+--------------+",
NULL
};


const char * const replace_help[] = {
"+-------+-------------------------------+",
"|  \\c   |  c literally and C escapes    |",
"|  \\n   |  backreference (1 to 9)       |",
"|  \\&   |  entire match                 |",
"|  \\+?  |  uppercase match or backref   |",
"|  \\-?  |  lowercase match or backref   |",
"|  \\^?  |  capitalise match or backref  |",
"+-------+-------------------------------+",
NULL
};


const char * const wildcard_help[] = {
"+-----------+--------------------------------------------+",
"|  *        |  match zero or more characters             |",
"|  ?        |  match any one character                   |",
"|  [set]    |  match any character in the set            |",
"|  [!set]   |  match any character not in the set        |",
"|  [^set]   |  same as above                             |",
"|  a:b!c:d  |  match \"a\" or \"b\" but exclude \"c\" and \"d\"  |",
"|  !a:b     |  match everything except \"a\" and \"b\"       |",
"|           |                                            |",
"|  ...      |  enter subdirectories (eg: \".../*.c\")      |",
"|  dirname  |  same as \"dirname/*\"                       |",
"+-----------+--------------------------------------------+",
"|  A set is a group (\"a1.\") and/or range (\"a-z\") of      |",
"|  characters.  To include '!^]-' in the set, precede    |",
"|  it with '\\'.  To match ';' or '!' make it part of a   |",
"|  set.  The names will be sorted according to the       |",
"|  current directory list order (name or extension).     |",
"|  ';' can be used instead of ':'.                       |",
"+--------------------------------------------------------+",
NULL
};


const char * const stamp_help[] = {
"+-------+------------------------+",
"|  %d   |  day of month          |",
"|  %D   |  day of week           |",
"|  %e   |  enter new line        |",
"|  %h   |  hour (12-hour)        |",
"|  %H   |  hour (24-hour)        |",
"|  %m   |  month (number)        |",
"|  %M   |  month (word)          |",
"|  %n   |  minutes               |",
"|  %p   |  am or pm              |",
"|  %s   |  seconds               |",
"|  %t   |  tab                   |",
"|  %y   |  year (two digits)     |",
"|  %Y   |  year (full)           |",
"|  %Z   |  timezone (abbr.)      |",
"|  %0?  |  zero-padded numbers,  |",
"|       |  abbreviated words     |",
"|  %2?  |  blank-padded numbers  |",
"|  %+?  |  left-aligned words    |",
"|  %-?  |  right-aligned words   |",
"|  %%   |  a percent sign        |",
"+-------+------------------------+",
NULL
};


const char * const border_help[] = {
"+--------------+---------------------------------------------------+",
"|  # of chars  |                  Style characters                 |",
"|   in style   |                     represent                     |",
"+--------------+---------------------------------------------------+",
"|      0       |  current graphic set                              |",
"|      1       |  entire border                                    |",
"|      2       |  vertical and horizontal edges                    |",
"|      3       |  corners, vertical and horizontal edges           |",
"|      4       |  left, right, top and bottom edges                |",
"|      5       |  top-left, top-right, bottom-left,                |",
"|              |    bottom-right corners and the edges             |",
"|      6       |  the four corners, vertical and horizontal edges  |",
"|      8       |  the four corners and the four edges              |",
"+--------------+---------------------------------------------------+",
NULL
};


const char * const exec_help[] = {
"+---------+----------------------------+",
"|  %[=]f  |  filename (relative path)  |",
"|  %[=]F  |  filename (absolute path)  |",
"|  %p     |  prompt for parameter      |",
"|  %w     |  copy the current word     |",
"|  %W     |  copy the current string   |",
"|  %%     |  a percent sign            |",
"+---------+----------------------------+",
"|    The filename will be quoted if    |",
"|    needed; '=' will prevent that.    |",
"+--------------------------------------+",
NULL
};


const char * const char_help[] = {
"+----------------------------------------+",
"|              Character Set             |",
"|                                        |",
"|    +-0-1-2-3-4-5-6-7-8-9-0-1-2-3-4-5-+ |",
"|   0|   \x01 \x02 \x03 \x04 \x05 \x06 \x07 "
       "\x08 \x09 \x0a \x0b \x0c \x0d \x0e \x0f | |",
"|  16| \x10 \x11 \x12 \x13 \x14 \x15 \x16 \x17 "
       "\x18 \x19 \x1a \x1b \x1c \x1d \x1e \x1f | |",
"|  32|   ! \" # $ % & ' ( ) * + , - . / | |",
"|  48| 0 1 2 3 4 5 6 7 8 9 : ; < = > ? | |",
"|  64| @ A B C D E F G H I J K L M N O | |",
"|  80| P Q R S T U V W X Y Z [ \\ ] ^ _ | |",
"|  96| ` a b c d e f g h i j k l m n o | |",
"| 112| p q r s t u v w x y z { | } ~  | |",
"| 128| €     …           | |",
"| 144|  ‘ ’ “ ” • – —         | |",
"| 160|   ก ข ฃ ค ฅ ฆ ง จ ฉ ช ซ ฌ ญ ฎ ฏ | |",
"| 176| ฐ ฑ ฒ ณ ด ต ถ ท ธ น บ ป ผ ฝ พ ฟ | |",
"| 192| ภ ม ย ร ฤ ล ฦ ว ศ ษ ส ห ฬ อ ฮ ฯ | |",
"| 208| ะ ั า ำ ิ ี ึ ื ุ ู ฺ Û Ü Ý Þ ฿ | |",
"| 224| เ แ โ ใ ไ ๅ ๆ ็ ่ ้ ๊ ๋ ์ ํ ๎ ๏ | |",
"| 240| ๐ ๑ ๒ ๓ ๔ ๕ ๖ ๗ ๘ ๙ ๚ ๛ ü ý þ ÿ | |",
"|    +---------------------------------+ |",
"+----------------------------------------+",
NULL
};

#endif
