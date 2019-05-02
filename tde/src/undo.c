/*
 * Editor:      TDE, the Thomson-Davis Editor
 * Filename:    undo.c
 * Author:      Jason Hood
 * Date:        November 19, 1999
 *
 * Functions dealing with undo and redo.
 *
 * Frank's original undo just stored a copy of the line before it was changed.
 * Whilst being nice and easy to implement, it's very limited. My undo will
 * restore every change made to a file (except SortBoxBlock and the box tabs)
 * and cursor (if it changes line number and/or column) movement. It's not
 * perfect, since it works with only one file - to undo a block move from one
 * file to another will require an undo in each file.
 */


#include "tdestr.h"
#include "tdefunc.h"
#include "common.h"


int  copied_rcol;               /* original rcol position for RestoreLine */
int  copied_mod;                /* original modified flag for RestoreLine */
int  copied_dirty;


/*
 * Name:    restore_line
 * Purpose: To retrieve unaltered line if possible.
 * Date:    June 5, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Changes are made to the line buffer so the underlying text has
 *          not changed.  Put the unchanged line from the file into the
 *          line buffer and display it.
 * jmh 010521: remember what it was, so you can change between original and
 *              modified.
 * jmh 021011: restore original column position, as well.
 * jmh 030225: restore modified flag.
 * jmh 031114: use the copied buff_node pointer to determine if the buffer is
 *              still valid (instead of swapping the line with the buffer).
 */
int  restore_line( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */
int  rcol;

   win = window;
   if (g_status.buff_node == win->ll) {
      if (!copied_dirty) {
         win->ll->type ^= DIRTY;
         win->file_info->modified = copied_mod || (win->ll->type & DIRTY);
      }
      g_status.copied = !g_status.copied;
      win->file_info->dirty = GLOBAL;
      show_changed_line( win );
      if (copied_rcol != win->rcol) {
         show_ruler_char( win );
         rcol = win->rcol;
         check_virtual_col( win, copied_rcol, copied_rcol );
         copied_rcol = rcol;
      }
   }
   return( OK );
}


/*
 * Name:    retrieve_line
 * Purpose: To retrieve (pop) a line from the undo stack
 * Date:    September 26, 1991
 * Passed:  window:  pointer to current window
 * Notes:   Insert an empty line into the file then pop the line in the undo
 *          stack.  When we pop line 0, there are no more lines on the stack.
 *          Set the stack pointer to -1 to indicate an empty stack.
 */
int  retrieve_line( TDE_WIN *window )
{
register TDE_WIN *win;   /* put window pointer in a register */
line_list_ptr node;

   win = window;
   if (win->file_info->undo_lines > 0 && win->ll->prev != NULL) {
      if (un_copy_line( win->ll, win, TRUE, TRUE ) == ERROR)
         return( ERROR );

      node = win->file_info->undo_top;
      win->file_info->undo_top = node->next;
      win->file_info->undo_top->prev = NULL;
      --win->file_info->undo_lines;

      node->next = node->prev = NULL;

      ++win->file_info->length;

      win->ll->prev->next = node;
      node->prev = win->ll->prev;

      win->ll->prev = node;
      node->next = win->ll;
      win->ll = node;
      win->ll->type |= DIRTY;

      --win->rline;
      adjust_windows_cursor( win, 1 );
      restore_marked_block( win, 1 );
      ++win->rline;
      syntax_check_lines( win->ll, win->file_info->syntax );

      /*
       * we have now undeleted a line.  increment the file length and display
       * it.
       */
      win->file_info->dirty = GLOBAL;
      win->file_info->modified = TRUE;
      show_size( win );
      show_avail_mem( );
   }
   return( OK );
}


/*
 * Name:    load_undo_buffer
 * Purpose: To copy the cursor line to the undo buffer.
 * Date:    September 26, 1991
 * Passed:  file:          pointer to file
 *          line_to_undo:  pointer to line in file to save
 * Notes:   save the last mode.undo_max_lines lines in a stack.  when we
 *           overflow the stack, dump the oldest line.
 * jmh 010522: if undo_max_lines is zero, store an "unlimited" number of lines.
 */
void load_undo_buffer( file_infos *file, text_ptr line_to_undo, int len )
{
int  rc;
text_ptr l;
line_list_ptr temp_ll;

   rc = OK;
   if (mode.undo_max_lines  &&  file->undo_lines >= mode.undo_max_lines) {
      --file->undo_lines;
      temp_ll = file->undo_bot->prev;
      temp_ll->prev->next = file->undo_bot;
      file->undo_bot->prev = temp_ll->prev;
      my_free( temp_ll->line );
   } else
      temp_ll = (line_list_ptr)my_malloc( sizeof(line_list_struc), &rc );

   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );

   l = my_malloc( len, &rc );

   if (rc == ERROR) {
      my_free( l );
      my_free( temp_ll );
   } else {
      my_memcpy( l, line_to_undo, len );
      temp_ll->line = l;
      temp_ll->len  = len;
      temp_ll->type = DIRTY;

      temp_ll->prev = NULL;
      temp_ll->next = file->undo_top;
      file->undo_top->prev = temp_ll;
      file->undo_top = temp_ll;

      ++file->undo_lines;
   }
}


/*
 * Name:    new_undo
 * Purpose: allocate memory for the undo structure
 * Author:  Jason Hood
 * Date:    November 19, 1999
 * Passed:  file:  pointer to file requiring undo
 * Returns: pointer to structure or NULL if no memory
 */
UNDO *new_undo( file_infos *file )
{
UNDO *undo;

   if (mode.undo_max && file->undo_count > mode.undo_max) {
      undo = file->undo_last->next;
      if ((unsigned long)file->undo_last->text > 255)
         my_free( file->undo_last->text );
      my_free( file->undo_last );
      undo->prev = NULL;
      file->undo_last = undo;
      --file->undo_count;
   }

   undo = my_calloc( sizeof(UNDO) );
   if (undo != NULL) {
      if (file->undo != NULL) {
         undo->prev = file->undo;
         file->undo->next = undo;
      }
      file->undo = undo;
      ++file->undo_count;
   }
   return( undo );
}


/*
 * Name:    undo_move
 * Purpose: undo a cursor movement
 * Author:  Jason Hood
 * Date:    November 19, 1999
 * Passed:  window:  pointer to window requiring undo
 *          func:    function to effect undo
 * Notes:   If func is 0, remember the current position, otherwise use func.
 */
void undo_move( TDE_WIN *window, int func )
{
file_infos *file;
UNDO *undo;

   if (mode.undo_move) {
      file = window->file_info;
      undo = file->undo;
      if (func == 0 || undo == NULL || undo->op != func) {
         undo = new_undo( file );
         if (undo == NULL)
            return;
         undo->op = func;
         if (g_status.command_count > 0)
            ++undo->group;
      }
      undo->line = window->rline;
      undo->col  = window->rcol;
      ++undo->len;
   }
}


/*
 * Name:    undo_char
 * Purpose: undo a character overwrite or delete
 * Author:  Jason Hood
 * Date:    November 19, 1999
 * Passed:  window:  pointer to window requiring undo
 *          ch:      character to undo
 *          ins:     insert (U_INS) or overwrite (U_OVR)
 */
void undo_char( TDE_WIN *window, char ch, int ins )
{
file_infos *file = window->file_info;
UNDO *undo = file->undo;
int  op;
char c;

   op = U_CHAR | ins;
   if (undo == NULL || undo->op != op || mode.cur_dir != CUR_RIGHT) {
      undo = new_undo( file );
      if (undo == NULL)
         return;
      undo->op    = op;
      undo->dirty = (int)window->ll->type & DIRTY;
      undo->mod   = file->modified;
      undo->text  = (text_ptr)(int)ch;
   } else {
      if (undo->len == 1) {
         c = (char)(long)undo->text;
         undo->text = my_malloc( 2, &op );
         if (op == ERROR)
            return;
         *undo->text = c;
      } else {
         undo->text = my_realloc( undo->text, undo->len + 1, &op );
         if (op == ERROR)
            return;
      }
      undo->text[undo->len] = ch;
   }
   undo->line = window->rline;
   undo->col  = window->rcol;
   ++undo->len;
}


/*
 * Name:    undo_line
 * Purpose: undo a line or string overwrite or delete
 * Author:  Jason Hood
 * Date:    November 19, 1999
 * Passed:  window:  pointer to window requiring undo
 *          line:    pointer to line/string
 *          len:     length of above
 *          op:      insert/overwrite, line/string
 */
void undo_line( TDE_WIN *window, text_ptr line, int len, int op )
{
UNDO *undo;
file_infos *file = window->file_info;

   undo = new_undo( file );
   if (undo == NULL)
      return;
   undo->line  = window->rline;
   undo->col   = window->rcol;
   undo->op    = op;
   undo->dirty = (int)window->ll->type & DIRTY;
   undo->mod   = file->modified;
   undo->len   = len;
   undo->text  = my_malloc( len, &op );
   if (op != ERROR)
      my_memcpy( undo->text, line, len );
}


/*
 * Name:    undo_del
 * Purpose: undo a character, string or line insertion
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  window:  pointer to window requiring undo
 *          len:     length to delete
 *          type:    type of delete
 * Notes: if type is U_CHAR, len is assumed to be one.
 */
void undo_del( TDE_WIN *window, int len, int type )
{
file_infos *file = window->file_info;
UNDO *undo = file->undo;
int  op;

   op = U_DEL | type;
   if (type == U_CHAR && undo != NULL && undo->op == op &&
       mode.cur_dir == CUR_RIGHT) {
      ++undo->len;
   } else {
      undo = new_undo( file );
      if (undo == NULL)
         return;
      undo->len   = len;
      undo->op    = op;
      undo->dirty = (int)window->ll->type & DIRTY;
      undo->mod   = file->modified;
   }
   undo->line = window->rline;
   undo->col  = window->rcol;
}


/*
 * Name:    undo_space
 * Purpose: undo an indentation or tab
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  window:  pointer to window requiring undo
 *          len:     number of spaces
 *          type:    insert or delete
 */
void undo_space( TDE_WIN *window, int len, int type )
{
UNDO *undo;
file_infos *file = window->file_info;

   undo = new_undo( file );
   if (undo == NULL)
      return;
   undo->line  = window->rline;
   undo->col   = window->rcol;
   undo->op    = U_SPACE | type;
   undo->len   = len;
   undo->dirty = (int)window->ll->type & DIRTY;
   undo->mod   = file->modified;
}


/*
 * Name:    undo
 * Purpose: reverse an action
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  window:  pointer to current window
 * Notes:
 */
int  undo( TDE_WIN *window )
{
   return( OK );
}


/*
 * Name:    redo
 * Purpose: reverse an undo action
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  window:  pointer to current window
 * Notes:
 */
int  redo( TDE_WIN *window )
{
   return( OK );
}


/*
 * Name:    toggle_undo_group
 * Purpose: toggle between group and individual undo
 * Author:  Jason Hood
 * Date:    November 20, 1999
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Notes:   use the diagnostic display to indicate current state
 */
int  toggle_undo_group( TDE_WIN *arg_filler )
{
   mode.undo_group = !mode.undo_group;
   show_undo_mode( );
   return( OK );
}


/*
 * Name:    toggle_undo_move
 * Purpose: toggle movement undo
 * Author:  Jason Hood
 * Date:    May 20, 2001
 * Passed:  arg_filler:  argument to satisfy function prototype
 * Notes:   use the diagnostic display to indicate current state
 */
int  toggle_undo_move( TDE_WIN *arg_filler )
{
   mode.undo_move = !mode.undo_move;
   show_undo_move( );
   return( OK );
}
