/*
 * Editor name:      tde, the Thomson-Davis Editor.
 * Filename:         dialogs.h
 * Author:           Jason Hood
 * Date:             November 15, 2003
 *
 * Dialog definitions.
 */


#define D_EMPTY         1       /* callback on empty edit string */
#define D_DONE          2       /* callback on dialog completion */


extern DIALOG find_dialog[];
extern DIALOG replace_dialog[];
extern DIALOG border_dialog[];
extern DIALOG diff_dialog[];
extern DIALOG grep_dialog[];
extern DIALOG exec_dialog[];
extern DIALOG fattr_dialog[];
extern DIALOG number_dialog[];
extern DIALOG margins_dialog[];
extern DIALOG tabs_dialog[];

extern int  find_proc( int, char * );
extern int  replace_proc( int, char * );
extern int  diff_proc( int, char * );
extern int  grep_proc( int, char * );
extern int  exec_proc( int, char * );
extern int  fattr_proc( int, char * );
extern int  margins_proc( int, char * );
extern int  tabs_proc( int, char * );


/*
 * BorderBlockEx
 */
#define EF_Border( n )  (border_dialog + 9 + n)

/*
 * DefineDiff
 */
#define IDE_FIRST       3
#define IDE_SECOND      4
#define IDC_ALL         5
#define IDC_LEAD        6
#define IDC_BLANK       7
#define IDC_EOL         8
#define IDC_HERE        9
#define EF_First        (diff_dialog + IDE_FIRST )
#define EF_Second       (diff_dialog + IDE_SECOND)
#define CB_All           diff_dialog[IDC_ALL  ].n
#define CB_Lead          diff_dialog[IDC_LEAD ].n
#define CB_Blank         diff_dialog[IDC_BLANK].n
#define CB_EOL           diff_dialog[IDC_EOL  ].n
#define CB_Here          diff_dialog[IDC_HERE ].n

/*
 * DefineGrep
 */
#define IDE_G_PATTERN   3
#define IDE_G_FILES     4
#define IDC_G_REGX      5
#define IDC_G_RESULTS   6
#define IDC_G_LOADALL   7
#define IDC_G_BINARY    8
#define EF_G_Pattern    (grep_dialog + IDE_G_PATTERN)
#define EF_G_Files      (grep_dialog + IDE_G_FILES  )
#define CB_G_RegX        grep_dialog[IDC_G_REGX   ].n
#define CB_G_Results     grep_dialog[IDC_G_RESULTS].n
#define CB_G_LoadAll     grep_dialog[IDC_G_LOADALL].n
#define CB_G_Binary      grep_dialog[IDC_G_BINARY ].n

/*
 * DefineSearch
 */
#define IDE_S_PATTERN   2
#define IDC_S_REGX      3
#define IDC_S_BACKWARD  4
#define IDC_S_BEGIN     5
#define IDC_S_BLOCK     6
#define IDC_S_ALL       7
#define IDC_S_RESULTS   8
#define EF_S_Pattern    (find_dialog + IDE_S_PATTERN)
#define CB_S_RegX        find_dialog[IDC_S_REGX    ].n
#define CB_S_Backward    find_dialog[IDC_S_BACKWARD].n
#define CB_S_Begin       find_dialog[IDC_S_BEGIN   ].n
#define CB_S_Block       find_dialog[IDC_S_BLOCK   ].n
#define CB_S_All         find_dialog[IDC_S_ALL     ].n
#define CB_S_Results     find_dialog[IDC_S_RESULTS ].n

/*
 * Execute
 */
#define IDE_COMMAND     2
#define IDC_CAPTURE     3
#define IDC_NECHO       4
#define IDC_NPAUSE      5
#define IDC_ORIGINAL    6
#define IDC_RELOAD      7
#define EF_Command      (exec_dialog + IDE_COMMAND)
#define CB_Capture       exec_dialog[IDC_CAPTURE ].n
#define CB_NEcho         exec_dialog[IDC_NECHO   ].n
#define CB_NPause        exec_dialog[IDC_NPAUSE  ].n
#define CB_Original      exec_dialog[IDC_ORIGINAL].n
#define CB_Reload        exec_dialog[IDC_RELOAD  ].n

/*
 * FileAttributes
 */
#if defined( __UNIX__ )
#define IDC_ATTR         4
#define NUM_ATTR         9
#define IDE_ATTR        14
#define CB_Attr( c )     fattr_dialog[IDC_ATTR + c].n
#else
#define IDC_ATTR         1
#define NUM_ATTR         4
#define IDE_ATTR         6
#define CB_Archive       fattr_dialog[1].n
#define CB_System        fattr_dialog[2].n
#define CB_Hidden        fattr_dialog[3].n
#define CB_Readonly      fattr_dialog[4].n
#endif
#define EF_Attr         (fattr_dialog + IDE_ATTR)

/*
 * NumberBlock
 */
#define EF_Start        (number_dialog + 4)
#define EF_Step         (number_dialog + 5)
#define EF_Base         (number_dialog + 6)
#define CB_Left          number_dialog[7].n
#define CB_Zero          number_dialog[8].n

/*
 * ReplaceString
 */
#define IDE_R_PATTERN    3
#define IDE_R_REPLACE    4
#define IDC_R_REGX       5
#define IDC_R_BACKWARD   6
#define IDC_R_BEGIN      7
#define IDC_R_BLOCK      8
#define IDC_R_ALL        9
#define IDC_R_NPROMPT   10
#define EF_R_Pattern    (replace_dialog + IDE_R_PATTERN)
#define EF_R_Replace    (replace_dialog + IDE_R_REPLACE)
#define CB_R_RegX        replace_dialog[IDC_R_REGX    ].n
#define CB_R_Backward    replace_dialog[IDC_R_BACKWARD].n
#define CB_R_Begin       replace_dialog[IDC_R_BEGIN   ].n
#define CB_R_Block       replace_dialog[IDC_R_BLOCK   ].n
#define CB_R_All         replace_dialog[IDC_R_ALL     ].n
#define CB_R_NPrompt     replace_dialog[IDC_R_NPROMPT ].n

/*
 * SetMargins
 */
#define IDE_LEFT        5
#define IDE_RIGHT       6
#define IDE_PARA        7
#define IDC_JUSTIFY     8
#define EF_Left         (margins_dialog + IDE_LEFT )
#define EF_Right        (margins_dialog + IDE_RIGHT)
#define EF_Para         (margins_dialog + IDE_PARA )
#define CB_Justify       margins_dialog[IDC_JUSTIFY].n

/*
 * SetTabs
 */
#define IDE_LOGICAL     4
#define IDE_PHYSICAL    5
#define EF_Logical      (tabs_dialog + IDE_LOGICAL )
#define EF_Physical     (tabs_dialog + IDE_PHYSICAL)
