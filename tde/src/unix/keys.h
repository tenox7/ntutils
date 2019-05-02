/*
 * Editor name:      tde, the Thomson-Davis Editor.
 * Filename:         keys.h
 * Author:           Jason Hood
 * Date:             November 25, 2003
 *
 * Key translations for Linux.
 */

/*
 * Translate standard ASCII characters to TDE keys.
 */
static const int char_to_scan[128] = {
    _CTRL+_2,           /*   0   */
    _CTRL+_A,           /*   1   */
    _CTRL+_B,           /*   2   */
    _CTRL+_C,           /*   3   */
    _CTRL+_D,           /*   4   */
    _CTRL+_E,           /*   5   */
    _CTRL+_F,           /*   6   */
    _CTRL+_G,           /*   7   */
    _CTRL+_H,           /*   8 Backspace  */
    _CTRL+_I,           /*   9 Tab  */
    _CTRL+_J,           /*  10   */
    _CTRL+_K,           /*  11   */
    _CTRL+_L,           /*  12   */
    _CTRL+_M,           /*  13 Enter  */
    _CTRL+_N,           /*  14   */
    _CTRL+_O,           /*  15   */
    _CTRL+_P,           /*  16   */
    _CTRL+_Q,           /*  17   */
    _CTRL+_R,           /*  18   */
    _CTRL+_S,           /*  19   */
    _CTRL+_T,           /*  20   */
    _CTRL+_U,           /*  21   */
    _CTRL+_V,           /*  22   */
    _CTRL+_W,           /*  23   */
    _CTRL+_X,           /*  24   */
    _CTRL+_Y,           /*  25   */
    _CTRL+_Z,           /*  26   */
    _CTRL+_LBRACKET,    /*  27 Esc  */
    _CTRL+_BACKSLASH,   /*  28   */
    _CTRL+_RBRACKET,    /*  29   */
    _CTRL+_6,           /*  30   */
    _CTRL+_MINUS,       /*  31   */
          _SPACEBAR,    /*  32   */
   _SHIFT+_1,           /*  33 ! */
   _SHIFT+_APOSTROPHE,  /*  34 " */
   _SHIFT+_3,           /*  35 # */
   _SHIFT+_4,           /*  36 $ */
   _SHIFT+_5,           /*  37 % */
   _SHIFT+_7,           /*  38 & */
          _APOSTROPHE,  /*  39 ' */
   _SHIFT+_9,           /*  40 ( */
   _SHIFT+_0,           /*  41 ) */
   _SHIFT+_8,           /*  42 * */
   _SHIFT+_EQUALS,      /*  43 + */
          _COMMA,       /*  44 , */
          _MINUS,       /*  45 - */
          _PERIOD,      /*  46 . */
          _SLASH,       /*  47 / */
          _0,           /*  48 0 */
          _1,           /*  49 1 */
          _2,           /*  50 2 */
          _3,           /*  51 3 */
          _4,           /*  52 4 */
          _5,           /*  53 5 */
          _6,           /*  54 6 */
          _7,           /*  55 7 */
          _8,           /*  56 8 */
          _9,           /*  57 9 */
   _SHIFT+_SEMICOLON,   /*  58 : */
          _SEMICOLON,   /*  59 ; */
   _SHIFT+_COMMA,       /*  60 < */
          _EQUALS,      /*  61 = */
   _SHIFT+_PERIOD,      /*  62 > */
   _SHIFT+_SLASH,       /*  63 ? */
   _SHIFT+_2,           /*  64 @ */
   _SHIFT+_A,           /*  65 A */
   _SHIFT+_B,           /*  66 B */
   _SHIFT+_C,           /*  67 C */
   _SHIFT+_D,           /*  68 D */
   _SHIFT+_E,           /*  69 E */
   _SHIFT+_F,           /*  70 F */
   _SHIFT+_G,           /*  71 G */
   _SHIFT+_H,           /*  72 H */
   _SHIFT+_I,           /*  73 I */
   _SHIFT+_J,           /*  74 J */
   _SHIFT+_K,           /*  75 K */
   _SHIFT+_L,           /*  76 L */
   _SHIFT+_M,           /*  77 M */
   _SHIFT+_N,           /*  78 N */
   _SHIFT+_O,           /*  79 O */
   _SHIFT+_P,           /*  80 P */
   _SHIFT+_Q,           /*  81 Q */
   _SHIFT+_R,           /*  82 R */
   _SHIFT+_S,           /*  83 S */
   _SHIFT+_T,           /*  84 T */
   _SHIFT+_U,           /*  85 U */
   _SHIFT+_V,           /*  86 V */
   _SHIFT+_W,           /*  87 W */
   _SHIFT+_X,           /*  88 X */
   _SHIFT+_Y,           /*  89 Y */
   _SHIFT+_Z,           /*  90 Z */
          _LBRACKET,    /*  91 [ */
          _BACKSLASH,   /*  92 \ */
          _RBRACKET,    /*  93 ] */
   _SHIFT+_6,           /*  94 ^ */
   _SHIFT+_MINUS,       /*  95 _ */
          _BACKQUOTE,   /*  96 ` */
          _A,           /*  97 a */
          _B,           /*  98 b */
          _C,           /*  99 c */
          _D,           /* 100 d */
          _E,           /* 101 e */
          _F,           /* 102 f */
          _G,           /* 103 g */
          _H,           /* 104 h */
          _I,           /* 105 i */
          _J,           /* 106 j */
          _K,           /* 107 k */
          _L,           /* 108 l */
          _M,           /* 109 m */
          _N,           /* 110 n */
          _O,           /* 111 o */
          _P,           /* 112 p */
          _Q,           /* 113 q */
          _R,           /* 114 r */
          _S,           /* 115 s */
          _T,           /* 116 t */
          _U,           /* 117 u */
          _V,           /* 118 v */
          _W,           /* 119 w */
          _X,           /* 120 x */
          _Y,           /* 121 y */
          _Z,           /* 122 z */
   _SHIFT+_LBRACKET,    /* 123 { */
   _SHIFT+_BACKSLASH,   /* 124 | */
   _SHIFT+_RBRACKET,    /* 125 } */
   _SHIFT+_BACKQUOTE,   /* 126 ~ */
          _BACKSPACE    /* 127 Delete in xterm  */
};


/*
 * Translate escape sequences to TDE keys.  "\e" is implied.
 * 070401: modified to handle SuSE/KDE sequences.
 */
static const CONFIG_DEFS esc_seq[] = {
   { "OM",     _SHIFT+_ENTER  },  /*  0 */
   { "OP",     _F1            },  /*  1 */
   { "OQ",     _F2            },  /*  2 */
   { "OR",     _F3            },  /*  3 */
   { "OS",     _F4            },  /*  4 */
   { "[A",     _UP            },  /*  5 */
   { "[B",     _DOWN          },  /*  6 */
   { "[C",     _RIGHT         },  /*  7 */
   { "[D",     _LEFT          },  /*  8 */
   { "[F",     _END           },  /*  9 */
   { "[G",     _CENTER        },  /* 10 */
   { "[H",     _HOME          },  /* 11 */
   { "[P",     _CONTROL_BREAK },  /* 12 */
   { "[Z",     _SHIFT+_TAB    },  /* 13 */
   { "[a",     _SHIFT+_UP     },  /* 14 */
   { "[b",     _SHIFT+_DOWN   },  /* 15 */
   { "[c",     _SHIFT+_RIGHT  },  /* 16 */
   { "[d",     _SHIFT+_LEFT   },  /* 17 */

   { "O2P",    _SHIFT+_F1     },  /* 18 */
   { "O2Q",    _SHIFT+_F2     },  /* 19 */
   { "O2R",    _SHIFT+_F3     },  /* 20 */
   { "O2S",    _SHIFT+_F4     },  /* 21 */
   { "[1~",    _HOME          },  /* 22 */
   { "[2F",    _SHIFT+_END    },  /* 23 */
   { "[2H",    _SHIFT+_HOME   },  /* 24 */
   { "[2~",    _INS           },  /* 25 */
   { "[3~",    _DEL           },  /* 26 */
   { "[4~",    _END           },  /* 27 */
   { "[5~",    _PGUP          },  /* 28 */
   { "[6~",    _PGDN          },  /* 29 */
   { "[[A",    _F1            },  /* 30 */
   { "[[B",    _F2            },  /* 31 */
   { "[[C",    _F3            },  /* 32 */
   { "[[D",    _F4            },  /* 33 */
   { "[[E",    _F5            },  /* 34 */

   { "[11~",   _F1            },  /* 35 */
   { "[12~",   _F2            },  /* 36 */
   { "[13~",   _F3            },  /* 37 */
   { "[14~",   _F4            },  /* 38 */
   { "[15~",   _F5            },  /* 39 */
   { "[17~",   _F6            },  /* 40 */
   { "[18~",   _F7            },  /* 41 */
   { "[19~",   _F8            },  /* 42 */
   { "[20~",   _F9            },  /* 43 */
   { "[21~",   _F10           },  /* 44 */
   { "[23~",   _F11           },  /* 45 */
   { "[24~",   _F12           },  /* 46 */
   { "[25~",   _SHIFT+_F1     },  /* 47 */
   { "[26~",   _SHIFT+_F2     },  /* 48 */
   { "[28~",   _SHIFT+_F3     },  /* 49 */
   { "[29~",   _SHIFT+_F4     },  /* 50 */
   { "[31~",   _SHIFT+_F5     },  /* 51 */
   { "[32~",   _SHIFT+_F6     },  /* 52 */
   { "[33~",   _SHIFT+_F7     },  /* 53 */
   { "[34~",   _SHIFT+_F8     },  /* 54 */
   { "[5;2",   _SHIFT+_PGUP   },  /* 55 */
   { "[6;2",   _SHIFT+_PGDN   },  /* 56 */

   { "[2;2~",  _SHIFT+_INS    },  /* 57 */
   { "[3;2~",  _SHIFT+_DEL    },  /* 58 */

   { "[15;2~", _SHIFT+_F5     },  /* 59 */
   { "[17;2~", _SHIFT+_F6     },  /* 60 */
   { "[18;2~", _SHIFT+_F7     },  /* 61 */
   { "[19;2~", _SHIFT+_F8     },  /* 62 */
   { "[20;2~", _SHIFT+_F9     },  /* 63 */
   { "[21;2~", _SHIFT+_F10    },  /* 64 */
   { "[23;2~", _SHIFT+_F11    },  /* 65 */
   { "[24;2~", _SHIFT+_F12    },  /* 66 */
};


/*
 * Index positions for the length of each sequence.
 */
static const int esc_idx[] = { 0, 0, 18, 35, 57, 59, 67 };
