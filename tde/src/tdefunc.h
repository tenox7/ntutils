/*
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991
 *
 * This modification of Douglas Thomson's code is released into the
 * public domain, Frank Davis.  You may distribute it freely.
 *
 * This file contains all prototypes for every function in tde.  It should
 * be included in every source code module.
 *
 * I'm so stupid, I can't keep up with which functions are used in which
 * files.  Let's gather all prototypes into one file, so I don't have
 * to remember too much.
 */


/***********************  function prototypes in bj_ctype.c *************/
int  bj_tolower( int );
int  bj_toupper( int );
int  bj_isspc( int );
int  bj_isnotspc( int );
/************************************************************************/


/*************************  function prototypes in block.c *************/
int  mark_block( TDE_WIN * );
int  move_mark( TDE_WIN * );            /* jmh 050710 */
int  unmark_block( TDE_WIN * );
void restore_marked_block( TDE_WIN *, int );
void shift_tabbed_block( file_infos * );
void shift_block( file_infos *, long, int, int );
int  prepare_block( TDE_WIN *, line_list_ptr, int );
int  pad_dest_line( TDE_WIN *, file_infos *, line_list_ptr );
int  block_operation( TDE_WIN * );
void do_line_block( TDE_WIN *,  TDE_WIN *,  int,  file_infos *,  file_infos *,
                    int,  line_list_ptr,  line_list_ptr,  line_list_ptr,
                    line_list_ptr,  long,  long,  long,  int * );
void do_stream_block( TDE_WIN *,  TDE_WIN *,  int,  file_infos *,
                      file_infos *,  line_list_ptr,  line_list_ptr,
                      line_list_ptr,  line_list_ptr,
                      long,  long,  long,  int,  int,  int,  int * );
void do_box_block( TDE_WIN *,  TDE_WIN *,  int,  file_infos *,  file_infos *,
                   line_list_ptr, line_list_ptr,  long,  long,
                   long,  long,  long,  int,  int,  text_ptr, int *,
                   int,  int,  int,  int,  int,  int * );
void load_box_buff( text_ptr, line_list_ptr, int, int, char, int, int );
int  copy_buff_2file( TDE_WIN *, text_ptr, line_list_ptr, int, int, int );
void block_fill( text_ptr, int, int );
int  block_pattern( text_ptr , int, int, text_ptr, int );  /* jmh 030305 */
void number_block_buff( text_ptr, int, long, int );
void restore_cursors( file_infos * );
int  delete_box_block( TDE_WIN *, line_list_ptr, int, int );
void check_block( TDE_WIN ** );
void find_begblock( file_infos * );
void find_endblock( file_infos * );
int  block_write( TDE_WIN * );
int  block_print( TDE_WIN * );
int  get_block_fill_char( TDE_WIN *, int *, text_ptr );
int  get_block_numbers( long *, long *, int * );
int  block_trim_trailing( TDE_WIN * );
int  block_email_reply( TDE_WIN * );
int  block_convert_case( TDE_WIN * );
void upper_case( text_ptr, size_t );
void lower_case( text_ptr, size_t );
void invert_case( text_ptr, size_t );   /* added by jmh 990915 */
void capitalise( text_ptr, size_t );    /* added by jmh 010624 */
void rot13( text_ptr, size_t );
void fix_uue( text_ptr, size_t );
void strip_hi( text_ptr, size_t );
int  get_block_border_style( TDE_WIN *, text_ptr, int * );
void justify_line_block( TDE_WIN *, line_list_ptr, long, long, int * );
int  block_indent( TDE_WIN * );
int  block_to_clipboard( TDE_WIN * );   /* added by jmh 020826 */
int  comment_block( TDE_WIN * );        /* added by jmh 030302 */
int  uncomment_block( TDE_WIN * );      /* added by jmh 030302 */
/************************************************************************/


/**********************  function prototypes in config.c   ***************/
int  tdecfgfile( TDE_WIN * );
int  readfile( char *, int );
int  read_line( FILE * );               /* added by jmh 021031 */
void parse_line( char *, int );
char *parse_token( char *, char * );
char *parse_literal( char *, char * );
int  search( char *, const CONFIG_DEFS *, int );
void clear_previous_macro( int );
int  parse_macro( char *, int );
MACRO *initialize_macro( long );
void check_macro( MACRO * );
int  cfg_record_keys( MACRO *, long, int, int );
long cfg_search_tree( int, TREE * );
void new_sort_order( text_ptr, text_ptr );
int  add_twokey( long, int );
void new_upper_lower( text_ptr );
void new_bj_ctype( char * );
int  parse_color( char **, char *, int, const char * );
int  parse_key( char * );                       /* added by jmh 020819 */
void parse_cmdline_macro( char *, int );        /* added by jmh 021024 */
const char *new_user_menu( char *, char *, int );        /* jmh 031129 */
/************************************************************************/


/**********************  function prototypes in console.c  ************/
void console_init( struct vcfg * );     /* added by jmh 021023 */
void console_suspend( void );           /* ditto */
void console_resume( int );             /* ditto */
void console_exit( void );              /* ditto */
void video_config( struct vcfg * );
long getkey( void );
int  waitkey( int );
int  capslock_active( void );           /* moved by jmh 980723 */
int  numlock_active( void );            /* added by jmh 020903 */
void flush_keyboard( void );
void page( unsigned char );             /* new function added by jmh */
void xygoto( int, int );
void display_line( text_ptr, unsigned char *, int, int, int );
void c_output( int, int, int, int );
void c_repeat( int, int, int, int, int );
void s_output( const char *, int, int, int );
void hlight_line( int, int, int, int );
void cls( void );
void set_cursor_size( int );
void set_overscan_color( int );
void save_screen_line( int, int, Char * );
void restore_screen_line( int, int, Char * );
void save_area( Char *, int, int, int, int );
void restore_area( Char *, int, int, int, int );
void shadow_area( int, int, int, int );         /* added by jmh 991020 */
#if !defined( __UNIX__ )                        /* added by jmh 991022 */
#define refresh( )
#endif
/************************************************************************/


/*************************  function prototypes in diff.c  **************/
int  define_diff( TDE_WIN * );
int  repeat_diff( TDE_WIN * );
int  differ( int, int, int );
int  skip_leading_space( text_ptr, int, int, int );
line_list_ptr skip_eol( TDE_WIN *, int *, int *, int, int );
void show_diff_window( TDE_WIN * );
/************************************************************************/


/*************************  function prototypes in dirlist.c *************/
int  dir_help( TDE_WIN * );
int  dir_help_name( TDE_WIN *, char * );
int  list_and_pick( char *, TDE_WIN * );
void setup_directory_window( DIRECTORY *, LIST *, int );
void recalculate_dir( DIRECTORY *, LIST *, int );
void write_directory_list( LIST *, DIRECTORY * );
int  select_file( LIST *, char *, DIRECTORY * );
void shell_sort( LIST *, int );
int  dir_cmp( char *, char * );
/************************************************************************/


/*************************  function prototypes in ed.c *****************/
int  insert_newline( TDE_WIN * );
int  insert_overwrite( TDE_WIN * );
int  join_line( TDE_WIN * );
int  dup_line( TDE_WIN * );
int  word_delete( TDE_WIN * );
int  back_space( TDE_WIN * );
int  transpose( TDE_WIN * );            /* new function by jmh */
int  line_kill( TDE_WIN * );
int  char_del_under( TDE_WIN * );
int  eol_kill( TDE_WIN * );
int  bol_kill( TDE_WIN * );             /* added by jmh 020911 */
int  set_tabstop( TDE_WIN * );
int  dynamic_tab_size( TDE_WIN * );     /* added by jmh 030304 */
void show_line_col( TDE_WIN * );
void show_asterisk( TDE_WIN * );
int  toggle_overwrite( TDE_WIN * );
int  toggle_smart_tabs( TDE_WIN * );
int  toggle_indent( TDE_WIN * );
int  set_margins( TDE_WIN * );
int  toggle_ww( TDE_WIN * );
int  toggle_crlf( TDE_WIN * );
int  toggle_trailing( TDE_WIN * );
int  toggle_z( TDE_WIN * );
int  toggle_eol( TDE_WIN * );
int  toggle_search_case( TDE_WIN * );
int  toggle_sync( TDE_WIN * );
int  toggle_ruler( TDE_WIN * );
int  toggle_tabinflate( TDE_WIN * );
int  toggle_cursor_cross( TDE_WIN * );  /* new function added by jmh 980724 */
int  toggle_graphic_chars( TDE_WIN * ); /* new function added by jmh 980724 */
int  change_cur_dir( TDE_WIN * );       /* new function added by jmh 981129 */
int  toggle_read_only( TDE_WIN * );     /* new function added by jmh 990428 */
int  toggle_draw( TDE_WIN * );          /* new function added by jmh 991018 */
int  toggle_line_numbers( TDE_WIN * );  /* new function added by jmh 991108 */
int  toggle_cwd( TDE_WIN * );           /* new function added by jmh 030226 */
int  toggle_quickedit( TDE_WIN * );     /* new function added by jmh 060219 */
void cursor_sync( TDE_WIN * );
void editor( void );
int  repeat( TDE_WIN * );               /* new function added by jmh */
void display_dirty_windows( TDE_WIN * );
void show_dirty_window( TDE_WIN * );
int  paste( TDE_WIN * );                /* added by jmh 020826 */
int  context_help( TDE_WIN * );         /* jmh 050710 */
/************************************************************************/


/*************************  function prototypes in file.c  **************/
int  write_file( char *, int, file_infos *, long, long, int );
int  hw_save( char *, file_infos *, long, long, int );
int  hw_append( char *, file_infos *, long, long, int );
int  load_file( char *, file_infos *, int *, int, line_list_ptr );
void insert_node( file_infos *, line_list_ptr, line_list_ptr );
int  show_file_2big( char *, int );
int  backup_file( TDE_WIN * );
int  edit_file( char *, int, int );
int  edit_another_file( TDE_WIN * );
int  reload_file( TDE_WIN * );          /* added by jmh 030318 */
int  attempt_edit_display( char *, int );
int  file_file( TDE_WIN * );
int  file_all( TDE_WIN * );             /* new function by jmh */
int  save_all( TDE_WIN * );             /* new function by jmh 040715 */
int  save_file( TDE_WIN * );
int  save_backup( TDE_WIN * );
int  save_as_file( TDE_WIN * );
void make_backup_fname( file_infos * );
int  write_to_disk( TDE_WIN *, char * );
int  search_and_seize( TDE_WIN * );
int  edit_next_file( TDE_WIN * );
int  file_exists( char * );             /* added by jmh 021020 */
int  change_mode( char *, int );        /* moved from port.c by jmh 021020 */
int  binary_file( char * );
int  change_fattr( TDE_WIN * );
char *str_fattr( char *, fattr_t );     /* added by jmh 021020 */
int  save_workspace( TDE_WIN * );       /* new function by jmh 020722 */
int  load_workspace( void );
int  set_path( TDE_WIN * );             /* added by jmh 021021 */
/************************************************************************/


/*************************  function prototypes in findrep.c ************/
int  ask_replace( TDE_WIN *, int * );
int  ask_wrap_replace( TDE_WIN *, int * );
void do_replace( TDE_WIN *, int );
int  find_string( TDE_WIN * );
int  define_search( TDE_WIN * );        /* new function added by jmh 990923 */
int  repeat_search( TDE_WIN * );        /* new function added by jmh 990923 */
int  isearch( TDE_WIN * );              /* new function added by jmh 021028 */
int  perform_search( TDE_WIN * );
line_list_ptr forward_search( TDE_WIN *, long *, int * );
line_list_ptr backward_search( TDE_WIN *, long *, int * );
void build_boyer_array( void );
void build_forward_skip( boyer_moore_type * );
void build_backward_skip( boyer_moore_type * );
int  calculate_forward_md2( text_ptr, int );
int  calculate_backward_md2( text_ptr, int );
line_list_ptr search_forward( line_list_ptr, long *, int * );
line_list_ptr search_backward( line_list_ptr, long *, int * );
void show_search_message( int );
void bin_offset_adjust( TDE_WIN *, long );
void find_adjust( TDE_WIN *, line_list_ptr, long, int );
int  replace_string( TDE_WIN * );
int  replace_and_display( TDE_WIN*, line_list_ptr, long, int, int*, int );
int  scan_forward( TDE_WIN *, int *, char, char );
int  scan_backward( TDE_WIN *, int *, char, char );
int  match_pair( TDE_WIN * );
/************************************************************************/


/*************************  function prototypes in hwind.c **************/
void format_time( const char *, char *, struct tm * );     /* added by jmh 980521 */
void show_modes( void );
void show_file_count( void );
void show_window_count( void );
void show_avail_mem( void );
void show_tab_modes( void );
void show_indent_mode( void );
void show_search_case( void );
void show_sync_mode( void );
void show_wordwrap_mode( void );
void show_trailing( void );
void show_control_z( void );
void show_insert_mode( void );
void show_graphic_chars( void );        /* added by jmh 980724 */
void show_cur_dir( void );              /* added by jmh 981129 */
void show_draw_mode( void );            /* added by jmh 991018 */
void show_undo_mode( void );            /* added by jmh 991120 */
void show_undo_move( void );            /* added by jmh 010520 */
void show_recording( void );            /* added by jmh 981129 */
void show_cwd( void );                  /* added by jmh 030226 */
void my_scroll_down( TDE_WIN * );
void eol_clear( int, int, int );
void window_eol_clear( TDE_WIN *, int );
void n_output( long, int, int, int, int );
void combine_strings( char *, const char *, const char *, const char * );
/* str = str1 + str2 - jmh 021031 */
#define join_strings( str, str1, str2 ) combine_strings( str, str1, str2, NULL )
char *reduce_string( char *, const char *, int, int ); /* added by jmh 021105 */
void make_ruler( void );
void show_ruler( TDE_WIN * );
void show_ruler_char( TDE_WIN * );
void show_ruler_pointer( TDE_WIN * );
void show_all_rulers( void );
void make_popup_ruler( TDE_WIN *, text_ptr, unsigned char *, int, int, int );
int  popup_ruler( TDE_WIN * );
int  show_help( void );
void show_strings( const char * const *, int, int, int );
void adjust_area( int *, int *, int *, int *, int * ); /* added by jmh 991023 */
/************************************************************************/


/************************  function prototypes in macro.c  **************/
int  record_on_off( TDE_WIN * );
unsigned find_combination( TDE_WIN * );
TREE *search_tree( long, TREE * );
MACRO *find_pseudomacro( unsigned, TDE_WIN * );
void add_branch( TREE *, TREE * );
void record_key( long, int );
void show_avail_strokes( int );
int  save_strokes( TDE_WIN * );
void write_macro( FILE *, MACRO *, long );
void write_pseudomacro( FILE *, TREE * );
void write_twokeymacro( FILE *, TREE * );
char *key_name( long, char * );
int  clear_macros( TDE_WIN * );
void delete_pseudomacro( TREE * );
void delete_twokeymacro( TREE * );
int  play_back( TDE_WIN * );
int  push_macro_stack( void );
int  pop_macro_stack( void );
int  macro_pause( TDE_WIN * );
long getkey_macro( void );              /* new function added by jmh 980726 */
int  set_break_point( TDE_WIN * );      /* new function added by jmh 980815 */
/************************************************************************/


/*************************  function prototypes in main.c  **************/
int  main( int, char *[] );
void error( int, int, const char * );
void terminate( void );
int  initialize( void );
void hw_initialize( void );
int  get_help( TDE_WIN * );
void show_credits( void );
/************************************************************************/


/**********************  function prototypes in memory.c  ***************/
int  init_memory( void );
void *my_malloc( size_t, int * );
void *my_calloc( size_t );
char *my_strdup( const char * );
void my_free( void * );
void my_free_group( int );
void *my_realloc( void *, size_t, int * );
void my_memcpy( void *, void *, size_t );
void my_memmove( void *, void *, size_t );
/************************************************************************/


/*********************  function prototypes in movement.c  **************/
void first_line( TDE_WIN * );                         /* jmh 030327 */
int  inc_line( TDE_WIN *, int );
int  dec_line( TDE_WIN *, int );
int  move_to_line( TDE_WIN *, long, int );
void move_display( TDE_WIN *, TDE_WIN * );
int  prepare_move_up( TDE_WIN * );
int  prepare_move_down( TDE_WIN * );
int  home( TDE_WIN * );
int  goto_eol( TDE_WIN * );
int  goto_top( TDE_WIN * );
int  goto_bottom( TDE_WIN * );
int  page_up( TDE_WIN * );
int  page_down( TDE_WIN * );
int  scroll_up( TDE_WIN * );
int  scroll_down( TDE_WIN * );
int  pan_up( TDE_WIN * );
int  pan_down( TDE_WIN * );
int  move_up( TDE_WIN * );
int  move_down( TDE_WIN * );
int  beg_next_line( TDE_WIN * );
int  next_line( TDE_WIN * );
int  move_left( TDE_WIN * );
int  move_right( TDE_WIN * );
int  pan_left( TDE_WIN * );
int  pan_right( TDE_WIN * );
int  word_left( TDE_WIN * );
int  word_right( TDE_WIN * );
int  find_dirty_line( TDE_WIN * );
int  find_browse_line( TDE_WIN * );     /* new function by jmh 031116 */
int  center_window( TDE_WIN * );
int  topbot_line( TDE_WIN * );          /* new function by jmh 991025 */
int  screen_right( TDE_WIN * );
int  screen_left( TDE_WIN * );
int  goto_top_file( TDE_WIN * );
int  goto_end_file( TDE_WIN * );
int  goto_position( TDE_WIN * );
int  set_marker( TDE_WIN * );
int  goto_marker( TDE_WIN * );
/************************************************************************/


/*************************  function prototypes in port.c *************/
long my_heapavail( void );
#if defined( __DOS16__ )
# define my_ltoa ltoa
#else
# if defined( __UNIX__ )
char *my_ltoa( int, char *, int );
# else
#  define my_ltoa itoa
# endif
#endif
FTYPE *my_findfirst( char *, FFIND *, int );
FTYPE *my_findnext( FFIND * );
#if defined( __DJGPP__ )
void my_findclose( FFIND * );
#elif defined( __WIN32__ )
# define my_findclose( dta ) FindClose( (dta)->find_info )
#elif defined( __UNIX__ )
# define my_findclose( dta ) closedir( (dta)->find_info )
#else
# define my_findclose( dta )
#endif
int  get_fattr( char *, fattr_t * );
int  set_fattr( char *, fattr_t );
int  get_current_directory( char * );
int  set_current_directory( char * );
void get_full_path( char *, char * );   /* new function added by jmh */
int  get_ftime( char *, ftime_t * );    /* time functions added jmh 030320 */
int  set_ftime( char *, ftime_t * );
#if defined( __UNIX__ )
#define ftime_to_tm( ft ) localtime( ft )
#else
struct tm *ftime_to_tm( const ftime_t * );
#endif
#if defined( __DOS16__ )
void unixify( char * );                 /* added by jmh 010604 */
#endif
#if defined( __DJGPP__ ) || defined( __WIN32__ )
int  set_clipboard( char *, int );      /* added by jmh 020826 */
char *get_clipboard( void );            /* ditto */
#endif
/************************************************************************/


/*************************  function prototypes in pull.c *************/
int  main_pull_down( TDE_WIN * );
long lite_bar_menu( int * );
long pull_me( MENU_STR *, int, int, int * );
void get_bar_spacing( int *, int * );
void draw_lite_head( int, int [] );
void get_minor_counts( MENU_STR * );
void init_menu( int );                  /* new function added by jmh */
int  viewer( long );                    /* added by jmh 020818 */
/************************************************************************/


/*************************  function prototypes in query.c **************/
int  getfunc( long );
long translate_key( int, int, text_t ); /* added by jmh 980725 */
int  get_string( int, int, int, int, int, char *, HISTORY * );
int  get_name( const char *, int, char *, HISTORY * ); /* history jmh 990424 */
int  copy_word( TDE_WIN *, char *, int, int );   /* added by jmh 980731 */
#if defined( __DJGPP__ ) || defined( __WIN32__ )
int  copy_clipboard( char *, int );              /* added by jmh 021021 */
#endif
int  get_number( const char *, int, long *, HISTORY * ); /* by jmh 021106 */
int  get_response( const char *, int, int, int, int, int, ... );
#define get_yn( prompt, line, flag ) \
   get_response( prompt, line, flag, 2, L_YES, A_YES, L_NO, A_NO )
#define get_ny( prompt, line, flag ) \
   get_response( prompt, line, flag, 2, L_NO, A_NO, L_YES, A_YES )
long prompt_key( const char *, int );           /* added by jmh 050922 */
int  get_sort_order( TDE_WIN * );
void add_to_history( char *, HISTORY * );       /* added by jmh 990424 */
int  do_dialog( DIALOG *, DLG_PROC );           /* added by jmh 031115 */
int  check_box( int );
void check_box_enabled( int, int );
int  set_dlg_text( DIALOG *, const char * );
char *get_dlg_text( DIALOG * );
int  set_dlg_num( DIALOG *, long );
long get_dlg_num( DIALOG * );
/************************************************************************/


/*************************  function prototypes in regx.c *************/
int  find_regx( TDE_WIN * );
line_list_ptr regx_search_forward( line_list_ptr, long *, int * );
line_list_ptr regx_search_backward( line_list_ptr, long *, int * );
int  nfa_match( void );
int  build_nfa( void );
int  expression( void );
int  term( void );
int  factor( void );
int  escape_char( int );
void emit_cnode( int, int, int, int );
void emit_nnode( int, int, int, int, int );
int  put_dq( int );
int  push_dq( int );
int  pop_dq( void );
int  dequeempty( void );
void init_nfa( void );
void regx_error( const char * );
int  separator( int );
int  Kleene_star( int );
int  letter( int );
/**********************************************************************/


/*************************  function prototypes in sort.c *************/
int  sort_box_block( TDE_WIN * );
void quick_sort_block( long, long, line_list_ptr, line_list_ptr );
void insertion_sort_block( long, long, line_list_ptr );
void load_pivot( line_list_ptr );
int  compare_pivot( line_list_ptr );
int  my_memcmp( text_ptr, text_ptr, int );
/************************************************************************/


/**********************  function prototypes in syntax.c  ***************/
int  init_syntax( TDE_WIN * );
int  scan_syntax( char *, char *, int );
int  syntax_read_line( FILE *, char ** );
LANGUAGE *add_language( char *, LANGUAGE * );
void syntax_parse_file( FILE *, LANGUAGE * );
void add_keyword_list( LANGUAGE *, int, char *, char * );
void add_keyword( LANGUAGE *, char *, int );
text_ptr is_keyword( LANGUAGE *, char *, int );
void syntax_init_colors( FILE * );
void syntax_init_lines( file_infos * );
void syntax_check( line_list_ptr, syntax_info * );
long syntax_check_lines( line_list_ptr, LANGUAGE * );
void syntax_check_block( long, long, line_list_ptr, LANGUAGE * );
int  syntax_attr( text_ptr, int, long, unsigned char *, LANGUAGE * );
int  syntax_toggle( TDE_WIN * );
int  syntax_select( TDE_WIN * );
int  my_strcmp( text_ptr, text_ptr );   /* added by jmh 980716 */
/************************************************************************/


/**********************  function prototypes in tab.c  ******************/
int  tab_key( TDE_WIN * );
int  backtab( TDE_WIN * );
int  next_smart_tab( TDE_WIN * );
int  prev_smart_tab( TDE_WIN * );
int  entab( text_ptr, int, int, int );
void detab_linebuff( int, int );
void entab_linebuff( int, int );
text_ptr detab_a_line( text_ptr, int *, int, int );
text_ptr tabout( text_ptr, int *, int, int );
int  detab_adjust_rcol( text_ptr, int, int );
int  entab_adjust_rcol( text_ptr, int, int, int );
int  block_expand_tabs( TDE_WIN * );
int  block_compress_tabs( TDE_WIN * );
/************************************************************************/


/*************************  function prototypes in undo.c  **************/
int  restore_line( TDE_WIN * );
int  retrieve_line( TDE_WIN * );
void load_undo_buffer( file_infos *, text_ptr, int );
UNDO *new_undo( file_infos * );
void undo_move( TDE_WIN *, int );
void undo_char( TDE_WIN *, char, int );
void undo_line( TDE_WIN *, text_ptr, int, int );
void undo_del( TDE_WIN *, int, int );
void undo_space( TDE_WIN *, int, int );
int  undo( TDE_WIN * );
int  redo( TDE_WIN * );
int  toggle_undo_group( TDE_WIN * );
int  toggle_undo_move( TDE_WIN * );
/************************************************************************/


/*************************  function prototypes in utils.c **************/
int  execute( TDE_WIN * );
int  myiswhitespc( int );
int  myisnotwhitespc( int );
void check_virtual_col( TDE_WIN *, int, int );
int  check_cline( TDE_WIN *, int );                         /* jmh 030327 */
line_list_ptr new_line( long, int * );                      /* jmh 030227 */
line_list_ptr new_line_text( text_ptr, int, long, int * );  /* ditto      */
void copy_line( line_list_ptr, TDE_WIN *, int );
int  un_copy_line( line_list_ptr, TDE_WIN *, int, int );
/* jmh 980729: This function is currently unused.       */
/* int  un_copy_tab_buffer( line_list_ptr, TDE_WIN * ); */
void set_prompt( const char *, int );
void show_eof( TDE_WIN * );
void display_current_window( TDE_WIN * );
int  redraw_screen( TDE_WIN * );
void redraw_current_window( TDE_WIN * );
void show_changed_line( TDE_WIN * );
void show_curl_line( TDE_WIN * );
void update_line( TDE_WIN * );
void dup_window_info( TDE_WIN *, TDE_WIN * );
void adjust_windows_cursor( TDE_WIN *, long );
int  first_non_blank( text_ptr, int, int, int );
int  find_end( text_ptr, int, int, int );
int  is_line_blank( text_ptr, int, int );
void show_window_header( TDE_WIN * );
void show_window_number_letter( TDE_WIN * );
void show_window_fname( TDE_WIN * );
void show_crlf_mode( TDE_WIN * );
void show_size( TDE_WIN * );
int  quit( TDE_WIN * );
int  quit_all( TDE_WIN * );             /* new function added by jmh */
int  date_time_stamp( TDE_WIN * );
int  stamp_format( TDE_WIN * );         /* new function added by jmh */
int  add_chars( text_ptr, TDE_WIN * );
int  shell( TDE_WIN * );                /* new function added by jmh */
int  user_screen( TDE_WIN * );          /* new function added by jmh */
int  show_status( TDE_WIN * );          /* new function added by jmh 990410 */
int  show_statistics( TDE_WIN * );      /* new function added by jmh 010605 */
char *create_frame( int, int, int, int *, int, int );     /* jmh 031119 */
int  numlen( long );                    /* added by jmh 991108 */
char *relative_path( char *, TDE_WIN *, int );  /* added by jmh 031028 */
/************************************************************************/


/*************************  function prototypes in window.c *************/
int  next_window( TDE_WIN * );
int  prev_window( TDE_WIN * );
int  goto_window( TDE_WIN * );          /* new function added by jmh 990502 */
void change_window( TDE_WIN *, TDE_WIN * );          /* added by jmh 990502 */
int  split_horizontal( TDE_WIN * );
int  make_horizontal( TDE_WIN * );      /* added by jmh 030323 */
int  split_vertical( TDE_WIN * );
int  make_vertical( TDE_WIN * );        /* added by jmh 030323 */
void show_vertical_separator( TDE_WIN * );
int  balance_horizontal( TDE_WIN * );   /* added by jmh 050724 */
int  balance_vertical( TDE_WIN * );     /* added by jmh 050724 */
int  size_window( TDE_WIN * );
int  zoom_window( TDE_WIN * );
int  initialize_window( void );
int  get_next_letter( int );
void setup_window( TDE_WIN * );
int  create_window( TDE_WIN **, int, int, int, int, file_infos * );
int  finish( TDE_WIN * );
int  find_window( TDE_WIN **, char *, int );    /* added by jmh 990502 */
TDE_WIN *find_file_window( file_infos * );      /* added by jmh 031117 */
int  title_window( TDE_WIN * );         /* added by jmh 030331 */
/************************************************************************/


/**********************  function prototypes in wordwrap.c **************/
int  find_left_margin( line_list_ptr, int, int, int );
void word_wrap( TDE_WIN * );
int  format_paragraph( TDE_WIN * );
void combine_wrap_spill( TDE_WIN *, int, int, int, int, int );
void justify_right_margin( TDE_WIN *, line_list_ptr, int, int, int );
void remove_spaces( int );
int  find_word( text_ptr, int, int );
int  flush_left( TDE_WIN * );
int  flush_right( TDE_WIN * );
int  flush_center( TDE_WIN * );
/************************************************************************/


#if defined( __DOS16__ )
/***********  function prototype for dos/kbdint.asm *******/
void kbd_install( int );
/*****************************************************/

/***********  function prototype for dos/criterr.asm **********/
void install_ceh( void * );
/*****************************************************/
#endif


/***********  function prototype for *\criterr.c *************/
#if defined( __WIN32__ )
BOOL ctrl_break_handler( DWORD );
#endif
#if !defined( __DOS16__ )
void crit_err_handler( int );
# if defined( __DJGPP__ )
void crit_err_handler_end( void );
# endif
#else
int  crit_err_handler( void );
void show_error_screen( void );
#endif
/*****************************************************/
