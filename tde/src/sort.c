/*
 * These functions sort lines according to keys in a marked BOX block.
 *
 * Being that the data structure was changed from a big text buffer to a
 * linked list, we can replace the simple insertion sort algorithm with a
 * much faster sort algorithm.  The classic search and sort reference is by
 * Donald E. Knuth, _The Art of Computer Programming; Volume 3:  Sorting and
 * Searching_.  One of the fastest and most widely used general purpose
 * sorting algorithms is the "Quicksort" algorithm.
 *
 * For implementation hints and guidelines on the Quicksort algorithm, one
 * might read the original Quicksort paper by C. A. R. Hoare or anything
 * by Robert Sedgewick.  Although Dr. Sedgewick includes a chapter on
 * Quicksort in his intro computer science textbooks, _Algorithms in X_,
 * I prefer the detailed hints and guidelines in his 1978 CACM paper.
 * Incidentally, Dr. Sedgewick's Ph.D. dissertation was on the modification
 * and mathematical analysis of the Quicksort algorithm.
 *
 *
 * See:
 *
 *   Charles Antony Richard Hoare, "Algorithm 63: Partition."  _Communications
 *      of the ACM_, 4 (No. 7): 321, 1961.
 *
 *   Charles Antony Richard Hoare, "Algorithm 64: Quicksort."  _Communications
 *      of the ACM_, 4 (No. 7): 321, 1961.
 *
 *   Charles Antony Richard Hoare, "Quicksort."  _The Computer Journal_,
 *      5 (April 1962 - January 1963): 10-15, 1962.
 *
 * See also:
 *
 *   Donald Ervin Knuth, _The Art of Computer Programming; Volume 3:  Sorting
 *     and Searching_, Addison-Wesley, Reading, Mass., 1973, Chapter 5,
 *     Sorting, pp 114-123.  ISBN 0-201-03803-X.
 *
 *   Robert Sedgewick, "Implementing Quicksort Programs."  _Communications
 *      of the ACM_, 21 (No. 10): 847-857, 1978.
 *
 *
 *                           Quicksort in TDE
 *
 * Quicksort in TDE is a stable, non-recursive implementation based on
 * Program 2, page 851, CACM, 21 (No. 10), 1978, by Robert Sedgewick.  My
 * partition algorithm finds the median-of-three.  Then, it walks from the
 * head of the list, comparing nodes, and uses the first occurrence of the
 * key of the median-of-three as the partition node.  Mostly by accident
 * and partly by design, my partition routine uses a "fat pivot" to keep
 * equal nodes in the same relative order as they appear in the file (the
 * definition of a stable sort).  By using the first median-of-three node
 * as the partitioning node, 1) sorting a sorted list is fairly fast and
 * 2) encountering a worst case is very unlikely.  TDE will sort, fairly
 * fast, multiple keys ascending or descending, ignore or match case, and
 * preserve order in the file, while using a custom sort sequence for your
 * favorite domestic or foreign language.
 *
 * Found an error in the comparison function in versions 2.2 & 3.0.  Equal
 * keys were not compared correctly.  Fixed in version 3.1.
 *
 * New editor name:  TDE, the Thomson-Davis Editor.
 * Author:           Frank Davis
 * Date:             June 5, 1991, version 1.0
 * Date:             July 29, 1991, version 1.1
 * Date:             October 5, 1991, version 1.2
 * Date:             January 20, 1992, version 1.3
 * Date:             February 17, 1992, version 1.4
 * Date:             April 1, 1992, version 1.5
 * Date:             June 5, 1992, version 2.0
 * Date:             October 31, 1992, version 2.1
 * Date:             April 1, 1993, version 2.2
 * Date:             June 5, 1993, version 3.0
 * Date:             August 29, 1993, version 3.1
 * Date:             November 13, 1993, version 3.2
 * Date:             June 5, 1994, version 4.0
 * Date:             December 5, 1998, version 5.0 (jmh)
 *
 * This code is released into the public domain, Frank Davis.
 * You may use and distribute it freely.
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"


/*#define USE_QSORT*/               /* use quicksort instead of mergesort */
/*#define TEST     */               /* display number of comparisons and ticks */

#ifdef TEST
unsigned *comps, qcomps, icomps;
clock_t ticks, ticks1;
#endif


/*
 * threshold value: partitions smaller than this will use insertion sort.
 */
#define THRESH 25


/*
 * function prototypes for mergesort
 */
void merge_sort_block( long, line_list_ptr );
line_list_ptr run( line_list_ptr, long *, long * );
line_list_ptr merge( line_list_ptr, long, line_list_ptr, long, line_list_ptr );


/*
 * Name:    sort_box_block
 * Purpose: sort lines according to text in marked BOX block
 * Date:    June 5, 1992
 * Passed:  window:  pointer to current window
 * Notes:   quick sort and insertion sort the lines in the BOX buff according
 *           to stuff in a box block.
 * jmh 980614: test for tabs.
 * jmh 030224: sort line blocks.
 * jmh 060827: detab each line instead of detabbing and entabbing the whole
 *              block - this is necessary to properly preserve tabs.
 */
int  sort_box_block( TDE_WIN *window )
{
int  prompt_line;
int  block_type;
line_list_ptr ll;
register file_infos *file;
TDE_WIN *sw;
int  rc;

   /*
    * make sure block is marked OK
    */
   rc = OK;
   prompt_line = window->bottom_line;
   if (un_copy_line( window->ll, window, TRUE, TRUE ) == ERROR)
      return( ERROR );
   check_block( &sw );
   if (g_status.marked == TRUE) {
      file = g_status.marked_file;
      if (file->read_only)
         return( ERROR );
      block_type = file->block_type;
      if (block_type == BOX || block_type == LINE) {
         /*
          * sort ascending or descending?
          */
         rc = get_sort_order( window );
         if (rc != ERROR) {
            file->modified = TRUE;
            if (mode.do_backups == TRUE)
               backup_file( sw );

            show_search_message( SORTING );

            if (block_type == BOX) {
               /*
                * figure the width of the block.
                */
               sort.block_len = file->block_ec + 1 - file->block_bc;

               /*
                * set up the sort structure.
                */
               sort.bc = file->block_bc;
               sort.ec = file->block_ec;
            } else {
               sort.bc = 0;
               sort.ec = MAX_LINE_LENGTH - 1;
               sort.block_len = MAX_LINE_LENGTH;
            }

            sort.order_array = (mode.search_case == IGNORE) ?
                                    sort_order.ignore : sort_order.match;

            /*
             * save the previous node for use with insertion sort.
             */
            ll = file->block_start->prev;

#ifdef TEST
            comps = &icomps;
            qcomps = icomps = 0;
            ticks1 = clock();
            while (ticks1 == (ticks = clock())) ;
#endif

#ifdef USE_QSORT
            quick_sort_block( file->block_br, file->block_er,
                              file->block_start, file->block_end );

#ifdef TEST
            ticks = clock() - ticks;
            printf( "Comparisons: %u (quick = %u, insert = %u); ticks: %lu\n",
                    qcomps + icomps, qcomps, icomps, (unsigned long)ticks );
#endif

#else
            merge_sort_block( file->block_er - file->block_br + 1,
                              file->block_start );

#ifdef TEST
            ticks = clock() - ticks;
            printf( "Comparisons: %u; ticks: %lu\n",
                    icomps, (unsigned long)ticks );
#endif
#endif

            /*
             * housekeeping.  mark the file as dirty and restore the
             *   cursors, which are scrambled during the sort.
             * jmh 980729: Update the syntax flags.
             */
            file->dirty = GLOBAL;
            restore_cursors( file );
            syntax_check_block( file->block_br, file->block_er, ll->next,
                                file->syntax );
            show_search_message( CLR_SEARCH );
         }
      } else {
         /*
          * cannot sort stream blocks
          */
         error( WARNING, prompt_line, block23 );
         rc = ERROR;
      }
   } else {
      /*
       * box not marked
       */
      error( WARNING, prompt_line, block24 );
      rc = ERROR;
   }
   return( rc );
}


#ifdef USE_QSORT

/*
 * Name:    quick_sort_block
 * Purpose: sort lines according to text in marked BOX block
 * Date:    Jaunary 10, 1993
 * Passed:  low:        starting line in box block
 *          high:       ending line in a box block
 *          low_node:   starting node in box block
 *          high_node:  ending node in box block
 * Notes:   Quicksort lines in the BOX block according to keys in
 *           a box block.
 *          because the median of three method is used to find the partion
 *           node,  high - low  should be greater than or equal to 2.
 *          with end recursion removal and sorting the smallest sublist
 *           first, our stack only needs room for log2 (N+1)/(M+2) nodes.
 *           a stack size of 24 can reliably handle almost 500 million lines.
 */
void quick_sort_block( long low, long high, line_list_ptr low_node,
                       line_list_ptr high_node )
{
long low_rline_stack[24];
long high_rline_stack[24];
line_list_ptr low_node_stack[24];
line_list_ptr high_node_stack[24];
long low_count;
long high_count;
long count;
line_list_ptr low_start;
line_list_ptr low_head;
line_list_ptr low_tail;
line_list_ptr high_end;
line_list_ptr high_head;
line_list_ptr high_tail;
line_list_ptr equal_head;
line_list_ptr equal_tail;
line_list_ptr walk_node;
line_list_ptr median_node;
int  i;
int  stack_pointer;

   assert( low_node->len  != EOF);
   assert( high_node->len != EOF);

   stack_pointer = 0;
   for (;;) {

      /*
       * being that a median-of-three is used as the partition algorithm,
       *  we probably need to have at least 2 nodes in each sublist.  I
       *  chose a minimum of 25 nodes as a SWAG (scientific wild ass guess).
       *  a simple insertion sort mops the list after quicksort finishes.
       */
      while (high - low > THRESH) {

         assert( high >= 1 );
         assert( low  >= 1 );
         assert( low  <= high );

         /*
          * start the walk node at the head of the list and walk to the
          *  middle of the sublist.
          */
         walk_node = low_node;
         count = (high - low) / 2;
         for (; count > 0; count--)
            walk_node = walk_node->next;

#ifdef TEST
         comps = &qcomps;
#endif

         /*
          * now, find the median of the low, middle, and high node.
          *
          * being that I am subject to error, let's assert that we really
          *  did find the median-of-three.
          */
         load_pivot( low_node );
         if (compare_pivot( walk_node ) < 0) {
            low_head    = walk_node;
            median_node = low_node;
         } else {
            low_head    = low_node;
            median_node = walk_node;
         }
         high_head = high_node;
         load_pivot( median_node );
         if (compare_pivot( high_node ) < 0) {
            high_head   = median_node;
            median_node = high_node;
         }
         load_pivot( median_node );
         if (compare_pivot( low_head ) > 0) {
            low_tail    = median_node;
            median_node = low_head;
            low_head    = low_tail;
         }

         load_pivot( median_node );

         assert( compare_pivot( low_head )  <= 0 );
         assert( compare_pivot( high_head ) >= 0 );

         /*
          * initialize pointers and counters for this partition.
          */
         low_start  = low_node->prev;
         high_end   = high_node->next;
         low_head   = low_tail   = NULL;
         equal_head = equal_tail = NULL;
         high_head  = high_tail  = NULL;
         low_count  = high_count = 0;

         /*
          * PARTITION:
          *  put all nodes less than the pivot on the end of the low list.
          *  put all nodes equal to the pivot on the end of the equal list.
          *  put all nodes greater than the pivot on the end of the high list.
          */
         walk_node = low_node;
         for (count = low; count <= high; count++) {
            i = compare_pivot( walk_node );
            if (i > 0) {
               if (high_head == NULL)
                  high_head = high_tail = walk_node;
               else {
                  high_tail->next = walk_node;
                  walk_node->prev = high_tail;
                  high_tail = walk_node;
               }

               /*
                * keep a count of the number of nodes in the high list.
                */
               ++high_count;
            } else if (i < 0) {
               if (low_head == NULL)
                  low_head = low_tail = walk_node;
               else {
                  low_tail->next  = walk_node;
                  walk_node->prev = low_tail;
                  low_tail = walk_node;
               }

               /*
                * keep a count of the number of nodes in the low list
                */
               ++low_count;
            } else {
               if (equal_head == NULL)
                  equal_head = equal_tail = walk_node;
               else {
                  equal_tail->next = walk_node;
                  walk_node->prev  = equal_tail;
                  equal_tail = walk_node;
               }
            }
            walk_node = walk_node->next;
         }

         assert( low_count  >= 0 );
         assert( low_count  < high - low );
         assert( high_count >= 0 );
         assert( high_count < high - low );

         /*
          * we just partitioned the sublist into low, equal, and high
          *  sublists.  now, let's put the lists back together.
          */
         if (low_count > 0) {
            low_head->prev   = low_start;
            low_start->next  = low_head;
            low_tail->next   = equal_head;
            equal_head->prev = low_tail;
         } else {
            equal_head->prev = low_start;
            low_start->next  = equal_head;
         }
         if (high_count > 0) {
            high_head->prev  = equal_tail;
            equal_tail->next = high_head;
            high_tail->next  = high_end;
            high_end->prev   = high_tail;
         } else {
            equal_tail->next = high_end;
            high_end->prev   = equal_tail;
         }

#ifdef TEST
         comps = &icomps;
#endif

         /*
          * now, lets look at the low list and the high list.  save the node
          *  pointers and counters of the longest sublist on the stack.
          *  then, quicksort the shortest sublist.
          */
         if (low_count > high_count) {

            /*
             * if fewer than THRESH nodes in the high count, don't bother to
             *  push the stack -- sort the low list.
             */
            if (high_count > THRESH) {
               low_rline_stack[stack_pointer]  = low;
               high_rline_stack[stack_pointer] = low + low_count - 1;
               low_node_stack[stack_pointer]   = low_head;
               high_node_stack[stack_pointer]  = low_tail;
               ++stack_pointer;
               low       = high - high_count + 1;
               high      = high;
               low_node  = high_head;
               high_node = high_tail;
            } else {
               insertion_sort_block( high - high_count + 1, high, high_head );
               low       = low;
               high      = low + low_count - 1;
               low_node  = low_head;
               high_node = low_tail;
            }
         } else {

            /*
             * if fewer than THRESH nodes in the low count, don't bother to
             *  push the stack -- sort the high list.
             */
            if (low_count > THRESH) {
               low_rline_stack[stack_pointer]  = high - high_count + 1;
               high_rline_stack[stack_pointer] = high;
               low_node_stack[stack_pointer]   = high_head;
               high_node_stack[stack_pointer]  = high_tail;
               ++stack_pointer;
               low       = low;
               high      = low + low_count - 1;
               low_node  = low_head;
               high_node = low_tail;
            } else {
               insertion_sort_block( low, low + low_count - 1, low_head );
               low       = high - high_count + 1;
               high      = high;
               low_node  = high_head;
               high_node = high_tail;
            }
         }

         assert( stack_pointer < 24 );
      }

      insertion_sort_block( low, high, low_node );

      /*
       * now that we have sorted the smallest sublist, we need to pop
       *  the long sublist(s) that were pushed on the stack.
       */
      --stack_pointer;
      if (stack_pointer < 0)
         break;
      low       = low_rline_stack[stack_pointer];
      high      = high_rline_stack[stack_pointer];
      low_node  = low_node_stack[stack_pointer];
      high_node = high_node_stack[stack_pointer];
   }
}


/*
 * Name:    insertion_sort_block
 * Purpose: sort small partitions passed in by qsort
 * Date:    January 10, 1993
 * Passed:  low:          starting line in box block
 *          high:         ending line in a box block
 *          first_node:   first linked list node to sort
 * Notes:   Insertion sort the lines in the BOX buff according to stuff in
 *           a box block.
 */
void insertion_sort_block( long low, long high, line_list_ptr first_node )
{
long down;                      /* relative line number for insertion sort */
long pivot;                     /* relative line number of pivot in block */
long count;
line_list_ptr pivot_node;       /* pointer to actual text in block */
line_list_ptr down_node;        /* pointer used to compare text */
text_ptr key;
int  len;
int  type;

   /*
    * make sure we have more than 1 line to sort.
    */
   count = high - low + 1;
   if (count > 1) {

      pivot_node = first_node->next;
      for (pivot = 1; pivot < count; pivot++) {
         load_pivot( pivot_node );
         key  = pivot_node->line;
         len  = pivot_node->len;
         type = pivot_node->type;
         down_node = pivot_node;
         for (down = pivot; --down >= 0;) {
            /*
             * lets keep comparing the keys until we find the hole for
             *  pivot.
             */
            if (compare_pivot( down_node->prev ) > 0) {
               down_node->line = down_node->prev->line;
               down_node->len  = down_node->prev->len;
               down_node->type = down_node->prev->type;
            } else
               break;
            down_node = down_node->prev;
         }
         down_node->line = key;
         down_node->len  = len;
         down_node->type = type;
         pivot_node = pivot_node->next;
      }
   }
}

#else

/*
 * Name:    merge_sort_block
 * Purpose: sort lines according to text in marked BOX block
 * Author:  Jason Hood
 * Date:    November 25, 2005
 * Passed:  lines:  number of lines to sort
 *          first:  first line of block
 * Notes:   my own algorithm for a non-recursive stable merge sort taking
 *           advantage of natural sort order.
 */
void merge_sort_block( long lines, line_list_ptr first )
{
line_list_ptr h[32], p;         /* heads for each run */
long  n[32];                    /* lengths of each run */
int   i;                        /* current run index */

   if (lines <= 1)
      return;

   h[0] = first;
   for (i = 0;;) {
      /*
       * find out how much is already sorted.
       */
      p = run( h[i], &lines, &n[i] );
      if (lines == 0)
         break;
      /*
       * merge similar-sized runs.
       */
      while (i > 0 && n[i] * 3 / 2 >= n[i-1] /*|| i == 31*/) {
         --i;
         h[i]  = merge( h[i], n[i], h[i+1], n[i+1], p );
         n[i] += n[i+1];
      }
      h[++i] = p;
   }

   /*
    * merge all remaining runs.
    */
   while (i > 0) {
      --i;
      h[i]  = merge( h[i], n[i], h[i+1], n[i+1], p );
      n[i] += n[i+1];
   }
}


/*
 * Name:    run
 * Purpose: find sequence of already sorted lines
 * Author:  Jason Hood
 * Date:    November 26, 2005
 * Passed:  list:  start of the sequence
 *          len:   pointer to length of the list
 *          cnt:   pointer to receive length of the sequence
 * Returns: line to start next sequence
 * Notes:   updates len with remaining lines.
 */
line_list_ptr run( line_list_ptr list, long *len, long *cnt )
{
line_list_ptr p;
text_ptr line;
int  llen;
long type;
int  rc;

   p = list->next;
   list->type &= ~EQUAL;
   p->type &= ~EQUAL;
   *cnt = 1;
   if (--*len > 0) {
      load_pivot( list );
      rc = compare_pivot( p );
      if (rc < 0) {
         line = p->line;                /* the first two lines are out of */
         llen = p->len;                 /*  order, so swap them */
         type = p->type;
         p->line = list->line;
         p->len  = list->len;
         p->type = list->type;
         list->line = line;
         list->len  = llen;
         list->type = type;
         /*
         // relink - requires "line_list_ptr *list"
         // example: TOF b a EOF        // list = b, p = a
         (*list)->prev->next = p;       // TOF -> a
         p->prev = (*list)->prev;       //   a <- TOF
         (*list)->next = p->next;       //   b -> EOF
         p->next->prev = *list;         // EOF <- b
         p->next = *list;               //   a -> b
         (*list)->prev = p;             //   b <- a
         *list = p;                     // a
         p = p->next;                   // b
         */
      }
      /*
       * find remaining in-order lines.
       */
      do {
         if (rc == 0)
            p->type |= EQUAL;
         load_pivot( p );
         p = p->next;
         p->type &= ~EQUAL;
         ++*cnt;
      } while (--*len != 0 && (rc = compare_pivot( p )) >= 0);
   }

   return( p );
}


/*
 * Name:    merge
 * Purpose: merge two sorted lists
 * Author:  Jason Hood
 * Date:    November 26, 2005
 * Passed:  one:   start of first list
 *          len1:  length of first list
 *          two:   start of second list
 *          len2:  length of second list
 *          tail:  line after the end of two (ie: two + len2 == tail).
 * Returns: start of merged list
 * Notes:   two is assumed to follow after one (ie: one + len1 == two).
 *          both lengths are assumed to be one or more.
 */
line_list_ptr merge( line_list_ptr one, long len1,
                     line_list_ptr two, long len2, line_list_ptr tail )
{
line_list_ptr head, t1;
line_list_ptr p;
int  rc;

   p = head = one->prev;
   t1 = two->prev;                      /* the last line of one */
   t1->next = tail;                     /* make one finish at the tail */

   /*
    * can two just move before one?  this generally increases the number of
    * comparisons, but it *greatly* reduces it for reverse sorting.
    */
   load_pivot( one );
   if (compare_pivot( tail->prev ) < 0) {
      /*
       *    one   two    tail
       * TOF 4 5 6 1 2 3 EOF --> TOF 1 2 3 4 5 6 EOF
       */
      head->next = two;                 /* TOF -> 1   */
      two->prev = head;                 /*   1 <- TOF */
      tail->prev->next = one;           /*   3 -> 4   */
      one->prev = tail->prev;           /*   4 <- 3   */
      /*t1->next = tail;*/              /*   6 -> EOF */
      tail->prev = t1;                  /* EOF <- 6   */
      return( two );
   }

   for (;;) {
      rc = compare_pivot( two );
      if (rc >= 0) {                    /* add line(s) from one */
         if (rc == 0)
            two->type |= EQUAL;
         one->prev = p;
         p->next = one;
         do {
            one = one->next;
            --len1;
         } while (one->type & EQUAL);
         if (len1 == 0) {               /* add the rest of two */
            t1->next = two;
            two->prev = t1;
            break;
         }
         p = one->prev;
         load_pivot( one );
      }
      if (rc <= 0) {                    /* add line(s) from two */
         two->prev = p;
         p->next = two;
         do {
            two = two->next;
            --len2;
         } while (two->type & EQUAL);
         p = two->prev;
         if (len2 == 0) {               /* add the rest of one */
            p->next = one;
            one->prev = p;
            two->prev = t1;             /* make the tail point back */
            break;                      /*  to the end of one       */
         }
      }
   }

   return( head->next );
}

#endif


/*
 * Name:    load_pivot
 * Purpose: load pivot point for insertion sort
 * Date:    June 5, 1992
 * Passed:  node:  line that contains the pivot
 *
 * jmh 060827: use the line buffer to store the detabbed line.
 */
void load_pivot( line_list_ptr node )
{
text_ptr s;
int len;

   len = node->len;
   s = detab_a_line( node->line, &len, g_status.marked_file->inflate_tabs,
                                       g_status.marked_file->ptab_size );
   memcpy( g_status.line_buff, s, len );
   sort.pivot_ptr = g_status.line_buff;
   sort.pivot_len = len;
}


/*
 * Name:    compare_pivot
 * Purpose: compare pivot string with text string
 * Date:    June 5, 1992
 * Passed:  text:  pointer to current line
 * Notes:   the sort structure keeps track of the pointer to the pivot line
 *           and the pivot line length.
 *
 * jmh 060827: detab the line.
 */
int  compare_pivot( line_list_ptr node )
{
int  len;
register int bc;
int  rc;
int  left_over;
int  dir;
text_ptr s;

#ifdef TEST
   ++*comps;
#endif

   len = node->len;
   bc  = sort.bc;
   dir =  sort.direction == ASCENDING ?  1 : -1;

   assert( bc  >= 0 );
   assert( len >= 0 );

   s = detab_a_line( node->line, &len, g_status.marked_file->inflate_tabs,
                                       g_status.marked_file->ptab_size );

   /*
    * is the current line length less than beginning column?  if so, just
    *  look at the length of the pivot line.  no need to compare keys.
    */
   if (len < bc+1) {
      if (sort.pivot_len < bc+1)
         return( 0 );
      else
         return( -dir );

   /*
    * is the pivot line length less than beginning column?  if so, just
    *  look at the length of the current line.  no need to compare keys.
    */
   } else if (sort.pivot_len < bc+1) {
      if (len < bc+1)
         return( 0 );
      else
         return( dir );
   } else {

      /*
       * if lines are of equal length or greater than the ending column,
       *  then lets consider them equal.
       */
      if (len == sort.pivot_len  ||
          (len > sort.ec  &&  sort.pivot_len > sort.ec))
         left_over = 0;
      else {

         /*
          * if one line does not extend thru the ending column, give
          *  preference to the longest key.
          */
         left_over =  len > sort.pivot_len ? dir : -dir;
      }

      /*
       * only need to compare up to length of the key in the pivot line.
       */
      if (len > sort.pivot_len)
         len = sort.pivot_len;
      len -= bc;
      if (len > sort.block_len)
         len = sort.block_len;

      assert( len > 0 );

      if (sort.direction == ASCENDING)
         rc = my_memcmp( s + bc, sort.pivot_ptr + bc, len );
      else
         rc = my_memcmp( sort.pivot_ptr + bc, s + bc, len );

      /*
       * if keys are equal, let's see if one key is longer than the other.
       */
      if (rc == 0)
         rc = left_over;
      return( rc );
   }
}


/*
 * Name:    my_memcmp
 * Purpose: compare strings using ignore or match case sort order
 * Date:    October 31, 1992
 * Passed:  s1:  pointer to string 1
 *          s2:  pointer to string 2
 *          len: number of characters to compare
 * Notes:   let's do our own memcmp, so we can sort languages that use
 *           extended characters as part of their alphabet.
 */
int  my_memcmp( text_ptr s1, text_ptr s2, int len )
{
unsigned char *p;
#if !defined( __DOS16__ )
register int c = 0;
#endif

   assert( len >= 0 );
   assert( len < MAX_LINE_LENGTH );
   assert( s1 != NULL );
   assert( s2 != NULL );

   p = sort.order_array;

   /*
    * the C comparison function is equivalent to the assembly version;
    *  however, once the assembly routine initializes, it is much faster
    *  than the C version.
    * jmh - had a look at djgpp's assembly output and it's pretty similar
    *       to the assembly version, anyway. Borland, OTOH, isn't as good,
    *       so why not use the assembly version all the time?
    */
#if !defined( __DOS16__ )
   if (len > 0)
      while ((c = (int)p[*s1++] - (int)p[*s2++]) == 0  &&  --len != 0) ;
   return( c );

#else
   ASSEMBLE {

   /*
   ; Register strategy:
   ;   ax  == p[*s1]
   ;   bx  == p[*s2]
   ;   dx:cx  == s1
   ;   es:di  == s2
   ;   bx  == *s1  or  *s2
   ;   ds:[si+bx]  == p[*s1]  or  p[*s2]
   ;
   ;  CAVEAT:  sort.order_array is assumed to be located in the stack segment
   ;  jmh 990504: not any more.
   */

        push    ds                      /* push required registers */
        push    si
        push    di

        xor     ax, ax                  /* zero ax */
        cmp     WORD PTR len, ax        /* len <= 0? */
        jle     get_out                 /* yes, get out */

        mov     dx, WORD ptr s1+2
        mov     cx, WORD ptr s1         /* dx:cx = s1 */
        les     di, s2                  /* es:di = s2 */
        xor     bx, bx                  /* zero out bx */
      }
top:

   ASSEMBLE {
        mov     ds, dx
        mov     si, cx                  /* ds:si = s1 */
        mov     bl, BYTE PTR ds:[si]    /* bl = *s1 */
        lds     si, p                   /* ds:si = sort order array */
        mov     al, BYTE PTR ds:[si+bx] /* al = p[*s1] */
        mov     bl, BYTE PTR es:[di]    /* bl = *s2 */
        mov     bl, BYTE PTR ds:[si+bx] /* bl = p[*s2] */
        sub     ax, bx                  /* ax = p[*s1] - p[*s2] */
        jne     get_out
        inc     cx                      /* s1++ */
        inc     di                      /* s2++ */
        dec     WORD PTR len            /* len-- */
        jnz     top
      }
get_out:

   ASSEMBLE {
        pop     di                      /* pop the registers we pushed */
        pop     si
        pop     ds                      /* ax keeps the return value */
      }
#if defined( __TURBOC__ )               /* jmh 980614: damn warning is  */
   return _AX;                          /*             driving me crazy */
#endif
#endif
}
