/*******************  start of original comments  ********************/
/*
 * Written by Douglas Thomson (1989/1990)
 *
 * This source code is released into the public domain.
 */
/*********************  end of original comments   ********************/


/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991
 *
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 *
 * This file contains all the external structure declarations common
 * to all the editor modules.   Being stupid, I can't remember which
 * extern prompts or structures are used by which file.  let's combine
 * them in one file, so I don't have to remember.
 */

/*
 * Some variables are used again and again throughout the editor.
 * Gather them into logical structures and make them global to all
 * modules.
 */
extern displays g_display;

extern status_infos g_status;

extern boyer_moore_type bm;

extern boyer_moore_type sas_bm;

extern REGX_INFO regx;

extern REGX_INFO sas_regx;

extern NFA_TYPE nfa;

extern NFA_TYPE sas_nfa;

extern option_infos g_option, g_option_all;

extern mode_infos mode;

extern KEY_FUNC key_func;

extern MACRO *macro[MODIFIERS][MAX_KEYS];
extern TREE   key_tree;

#if defined( __DOS16__ )
extern CEH ceh;
#endif

extern SORT sort;

extern SORT_ORDER sort_order;
extern int  scancode_map[MAX_KEYS];     /* added by jmh 020903 */

extern DIFF diff;

extern char line_out[MAX_COLS+2];       /* added by jmh 021106 */

extern char ruler_line[];               /* added by jmh 991110 */
extern TDE_WIN ruler_win;               /* added by jmh 050720 */

extern HISTORY h_file;                  /* histories added by jmh 990424 */
extern HISTORY h_find;
extern HISTORY h_border;
extern HISTORY h_fill;
extern HISTORY h_stamp;
extern HISTORY h_line;
extern HISTORY h_lang;
extern HISTORY h_grep;
extern HISTORY h_win;
extern HISTORY h_exec;
extern HISTORY h_parm;


extern TDE_COLORS colour;

extern int syntax_color[SHL_NUM_COLORS];

extern int (* const do_it[NUM_FUNCS])( TDE_WIN * );
extern const char func_flag[NUM_FUNCS]; /* added by jmh 990428 */

extern MENU menu;
extern MENU_STR make_window_menu;       /* added by jmh 030331 */
extern char *key_word[MAX_KEYS];        /* added by jmh for pull-down menu */
extern int  menu_cnt;                   /* added by jmh 050722 */
extern int  user_idx;
extern MENU_LIST *popout_menu;

#include "dialogs.h"


/*
 * Byrial's ctype tables
 */
extern char bj_ctype[];
extern unsigned char upper_lower[];     /* unsigned by jmh 980808 */


/*
 * if we are in un*x, let's emulate the PC color table.
 */
#if defined( __UNIX__ )
extern chtype tde_color_table[256];
#endif

/*
 * credit and help screens
 */
extern const char * const tde_help;     /* added by jmh 010528 */
extern const char * const credit_screen[];
extern char help_screen[2][HELP_HEIGHT][HELP_WIDTH+1];
extern int  help_dim[2][2];
extern const char * const regx_help[];
extern const char * const replace_help[];
extern const char * const wildcard_help[];
extern const char * const stamp_help[];
extern const char * const border_help[];
extern const char * const exec_help[];
extern const char * const char_help[];


/*
 * extern definitions for all prompts
 */

extern const char cb[];

extern const char eof_text[2][12];

extern const char block1[];
extern const char ltol[];
extern const char block2[];
extern const char block3a[];
extern const char block3b[];
extern const char block4[];
extern const char block5[];
extern const char * const block6[][2];
extern const char block7[];
extern const char block13[];
extern const char block14[];
extern const char block15[];
extern const char block15a[];           /* added by jmh 030305 */
extern const char block20[];
extern const char block20a[];
extern const char block21[];
extern const char block23[];
extern const char block24[];
extern const char block25[];
extern const char block26[];
extern const char block27[];
extern const char block28[];
extern const char block28a[];           /* added by jmh 030305 */
extern const char block29[];
extern const char block30[];
extern const char block31[];
extern const char block32[];            /* added by jmh 991029 */
extern const char block33a[];           /* ditto */
extern const char block33b[];           /* added by jmh 991108 */
extern const char block34[];            /* added by jmh 991112 */
extern const char block35[];            /* added by jmh 991112 */
extern const char block36[];            /* added by jmh 030302 */
extern const char block37[];            /* added by jmh 030304 */
extern const char block38[];            /* added by jmh 030304 */


extern const char config0[];
extern const char config1[];
extern const char config2[];
extern const char config3[];
extern const char config6[];
extern const char config6a[];           /* added by jmh 990420 */
extern const char config7[];
extern const char config8[];
extern const char config9[];
extern const char config10[];
extern const char config11[];
extern const char config12[];
extern const char config13[];
extern const char config14[];
extern const char config15[];
extern const char config16[];
extern const char config17[];
extern const char config18[];
extern const char config19[];
extern const char config20[];
extern const char config21[];
extern const char config22[];
extern const char config23[];
#if 0 //defined( __UNIX__ )
extern const char config24[];
#endif
extern const char config25[];           /* added by jmh 980719 */
extern const char config26[];           /* added by jmh 980720 */
extern const char config27[];           /* added by jmh 980805 */
extern const char config28[];           /* added by jmh 980820 */
extern const char config29[];           /* added by jmh 991020 */
extern const char config30[];           /* added by jmh 020903 */
extern const char config31[];           /* added by jmh 021025 */
extern const char config32[];           /* added by jmh 031130 */
extern const char config33[];           /* added by jmh 031130 */
extern const char config34[];           /* added by jmh 050723 */
extern const char config35[];           /* added by jmh 050723 */


extern const char diff_prompt0[];       /* added by jmh 980726 */
extern const char diff_prompt1[];       /* added by jmh 031202 */
extern const char diff_prompt4[];
extern const char diff_prompt5[];
extern const char diff_prompt6[];

extern const char diff_message[];


extern const char dir1[];
extern const char dir2[];
extern const char dir3[];
extern const char dir4[];
extern const char dir5[];
extern const char dir5a[];
extern const char dir6[];
extern const char dir7[];
extern const char dir8[];

extern const char dir10[];              /* added by jmh 060914 */
extern const char dir11[];
extern const char dir12[];
extern const char dir13[];
extern const char dir14[][8];


extern const char ed1[];
extern const char ed2[];
extern const char ed3[];
extern const char ed4[];
extern const char ed5[];

extern const char ed7c[];
extern const char ed8a[];
extern const char ed8b[];
extern const char ed10[];
extern const char ed12[];
extern const char ed14[];
extern const char ed15[];
extern const char ed15a[];              /* added by jmh */
extern const char ed15b[];              /* added by jmh 021024 */
extern const char ed16[];
extern const char ed17[];
extern const char ed18[];
extern const char ed19[];               /* added by jmh 980726 */
extern const char ed20[];               /* ditto */
extern const char ed21[];               /* added by jmh 981129 */
extern const char ed22[];               /* added by jmh 010528 */
extern const char ed23[];               /* added by jmh 010528 */
extern const char ed23a[];              /* added by jmh 021024 */
extern const char ed23b[];              /* ditto */
extern const char ed23c[];              /* ditto */
extern const char ed24[];               /* added by jmh 020802 */
extern const char ed25[];               /* added by jmh 020817 */
extern const char ed26[];               /* added by jmh 030226 */
extern const char ed27a[];              /* added by jmh 050725 */
extern const char ed27b[];              /* added by jmh 050725 */

extern const char paused1[];
extern const char paused1a[];           /* added by jmh 031114 */
extern const char paused2[];


extern const char find1[];
extern const char find2[];
extern const char find3[];
extern const char find4[];
extern const char find5a[];
extern const char find5b[];
extern const char find6[];
extern const char find7[][11];
extern const char find8[];
extern const char find9[];
extern const char find10[];
extern const char find11[];
extern const char find12[];
extern const char find13[];
extern const char find17[];
extern const char find18a[];            /* added by jmh 021028 */
extern const char find18b[];            /* ditto */
extern const char find18c[];            /* ditto */


extern const char file_win[];           /* split from file_win_mem */
extern const char mem_eq[];             /*  by jmh 981130 */
extern const char tabs[];
extern const char smart[2];
extern const char tab_mode[3];
extern const char indent_mode[][7];
extern const char case_mode[][7];
extern const char sync_mode[][5];
extern const char ww_mode[][3];
extern const char mode_lf[];
extern const char crlf_mode[][5];
extern const char mode_bin[];
extern       char graphic_mode[];
extern const char * const cur_dir_mode[];
extern const char draw_mode[];
extern const char ruler_help[];         /* jmh 050718 */
extern const char ruler_bad[];          /* jmh 050718 */


extern const char main2[];
extern const char main3[];
extern const char main4[];
extern const char main4a[];
extern const char main6[];
extern const char main7a[];
extern const char main7b[];
extern const char main9[];
extern const char main10a[];
extern const char main10b[];
extern const char main11[];
extern const char main12[];
extern const char main12a[];            /* 12a & 12b added by jmh */
extern const char main12b[];
extern const char main13[];
extern const char main14[];
extern const char main15[];
extern const char main16[];
extern const char main17[];             /* jmh 050709 */
extern const char main18[];             /* ditto */
extern const char main19[];
extern const char main20a[];            /* 20a, b and c added by jmh 980717 */
extern const char main20b[];
extern const char main20c[];
extern const char main21[];
extern const char main21a[];            /* jmh 010604 */
extern const char main22[];             /* added by jmh */
extern const char main23[];             /* jmh 030320 */
extern const char main23a[];            /* jmh 030320 */
extern const char main24[];             /* jmh 030320 */
extern const char main25a[];            /* jmh 030320 */
extern const char main25b[];            /* jmh 030320 */


extern const char reg1[];
extern const char reg2[];
extern const char reg3[];
extern const char reg4[];
extern const char reg5[];
extern const char reg6[];
extern const char reg7[];
extern const char reg8[];
extern const char reg9[];
extern const char reg10[];
extern const char reg11[];
extern const char reg12[];
extern const char reg13[];
extern const char reg14[];


extern const char syntax1[];
extern const char syntax2[];
extern const char syntax3[];
extern const char syntax4[];
extern const char syntax5[];
extern const char syntax6[];
extern const char syntax7[];
extern const char syntax8a[];
extern const char syntax8b[];
extern const char syntax9[];
extern const char syntax10[];
extern const char syntax11[];
extern const char syntax12[];
extern const char syntax13a[];
extern const char syntax13b[];
extern const char syntax14a[];
extern const char syntax14b[];


extern const char utils4[];
extern const char utils6[];
extern const char utils7a[];
extern const char utils7b[];
extern const char utils7c[];            /* added by jmh 021023 */
extern const char utils7d[];            /* added by jmh 030730 */
extern const char utils8[];
extern const char utils9[];
extern const char utils10[];
extern const char utils12[];
extern const char utils12a[];           /* added by jmh */
extern       char utils13[];            /* added by jmh 010523 */
extern const char utils15[];
extern const char utils16[];
extern const char utils17[];            /* added by jmh 980521 */
extern const char utils18a[];           /* added by jmh 031116 */
extern const char utils18b[];           /* added by jmh 031117 */
extern const char utils19[];            /* added by jmh 031026 */
extern const char utils20[];            /* added by jmh 031029 */


extern const char win1[];
extern const char win1a[];              /* added by jmh 991109 */
extern const char win1b[];              /* added by jmh 030323 */
extern const char win2[];
extern const char win3[];
extern const char win4[];
extern const char win5[];
extern const char win6[];
extern const char win7[];
extern const char win8[];
extern const char win9[];
extern const char win9b[];              /* added by jmh 031116 */
extern const char win9a[];              /* added by jmh */
extern const char win18[];              /* added by jmh 070501 */
extern const char win19[];
extern const char win20[];              /* added by jmh 990502 */
extern const char win21a[];             /* ditto */
extern const char win21b[];             /* ditto */
extern const char win21c[];             /* added by jmh 030401 */
extern const char win22a[];             /* added by jmh 990502 */
extern const char win22b[];             /* ditto */
extern const char win23[];              /* added by jmh 030331 */


extern const char ww1[];
extern const char ww2[];


extern const char windowletters[];

extern const char * const time_ampm[2];

/* all the following added by jmh 980521 */
extern const char * const months[2][12];
extern const char * const days[2][7];
extern const int  longest_month;        /* 010624 */
extern const int  longest_day;          /* ditto */

/* added by jmh 980724 */
extern const char graphic_char[GRAPHIC_SETS][11];

/* Status strings added by jmh 990410 */
extern char *stat_screen[];
extern const char status0[];            /* 991021 */
extern const char * const status1a[];
extern const char * const status1b[];
extern const char * const status_block[];
extern const char * const status2[];
extern const char * const status2a[];   /* 050714 */
extern const char * const status2b[];   /* 050714 */
extern const char * const status3[];
extern const char * const status4[];
extern const char * const status5[];    /* 010605 */
extern const char status6[];            /* 030320 */
extern const char status7[];            /* 030320 */
extern const char stat_time[];

/* Statistics strings added by jmh 010605 */
extern const char stats0[];
extern const char * const stats1[3][3];
extern const char * const stats[];
extern const char stats4[];
extern const char stats5[];
extern const char * const eol_mode[];   /* 991111 */
extern const char pipe_file[];          /* 030226 */
extern const char scratch_file[];       /* 030226 */
