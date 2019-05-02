/*
 * Editor:      TDE, the Thomson-Davis Editor
 * Filename:    memory.c
 * Author:      Jason Hood
 * Date:        November 24 to December 8, 2001
 *
 * Memory management functions.
 *
 * These functions improve the memory efficiency of TDE. In TDE, each line
 * has two mallocs - one for the line structure and another for the line
 * contents. With a lot of lines, the overhead of each malloc adds up. For
 * example, loading a 5« Mb file (151173 lines) in djgpp 2.03 would take
 * approximately 13« Mb of memory; now it takes 8« Mb, much closer to the
 * minimum value of 8.
 *
 * Instead of individual mallocs, one big block is allocated.  If 132 bytes or
 * more is requested, malloc is used, with an extra byte to indicate so. For
 * requests less than 132 bytes, the block is used. In the block, the size and
 * availability of the allocation is stored - bits 0-6 contain the size, with
 * bit 7 indicating used (0) or free (1). If this value is -1, the length is
 * stored in the allocation itself, allowing for large free spaces.
 *
 * Blocks are sorted by largest free space, allowing a binary search to quickly
 * find the block with the most appropriate size. A pointer is maintained to
 * the largest or newly created free space, with each space linked to the next
 * and previous free space. This makes the minimum allocation 6 bytes - two
 * bytes for the length header and trailer and four bytes for the two pointers.
 *
 * Blocks are also sorted by their own memory pointers, allowing the owner of a
 * pointer to be found quickly when it is freed. The free space before and
 * after it is grouped together. A function is provided to (slightly) improve
 * performance when doing multiple frees (such as closing a file).
 */

#include "tdestr.h"
#include "common.h"
#include "tdefunc.h"
#include "define.h"


static void *allocate( unsigned );
static int  find_block( void * );
static int  find_size( unsigned, int );
static int  find_block_size( int );
static void find_largest_free( mem_info * );
static void adjust_sizes( mem_info *, int );

#if defined( __DOS16__ )
static unsigned long ptoul( void * );
#else
#define ptoul (unsigned long)
#endif

static int  *group = NULL;


#define SUS (sizeof(unsigned short))
#define MAX_CHAR 127

#define MIN_SIZE  (2*SUS)
#define MAX_SIZE  (MAX_CHAR + MIN_SIZE)
#define MIN_SPACE (1 + MIN_SIZE + 1)
#define MAX_SPACE (1 + MAX_SIZE + 1)

#define WORD( p )  *(unsigned short*)(p)
#define DWORD( p ) *(unsigned long*)(p)

#define FREE( p )  (*(p) & (MAX_CHAR + 1))
#define SIZE(p)  (*(p) == -1 ? WORD((p)+1+2*SUS) : ((*(p)&MAX_CHAR)+MIN_SPACE))
#define PSIZE(p) (*(p) == -1 ? WORD( (p)-SUS ) : ((*(p) & MAX_CHAR)+MIN_SPACE))

#define SPACE( p, s ) \
   if ((s) >= MAX_SPACE) {\
      *(p) = *((p) + (s) - 1) = -1;\
      WORD( (p) + 1 + 2*SUS ) = WORD( (p) + (s) - SUS - 1 ) = (s);\
   } else\
      *(p) = *((p) + (s) - 1) = (s) - MIN_SPACE + MAX_CHAR + 1

#define NEXT( p )  WORD( (p) + 1 )
#define PREV( p )  WORD( (p) + 1 + SUS )
#define OFS( p )   (unsigned short)((p) - block->base)
#define PTR( p )   (block->base + (p))


/*
 * Name:    my_malloc
 * Purpose: allocate memory
 * Date:    November 24, 2001
 * Passed:  size: memory needed
 *          rc:   pointer to return code
 * Notes:   set the return code only if an ERROR occured with malloc.
 *           returning a NULL pointer is not neccessarily an ERROR.
 */
void *my_malloc( size_t size, int *rc )
{
char *mem;

   if (size == 0)
      /*
       * if 0 bytes are requested, return NULL
       */
      mem = NULL;

   else {
      if (size <= MAX_SIZE)
         mem = allocate( size );
      else {
         mem = malloc( size + 1 );
         if (mem != NULL)
            *mem++ = -1;
      }

      /*
       * if malloc failed, return NULL and an ERROR.
       */
      if (mem == NULL)
         *rc = ERROR;
   }

   return( mem );
}


/*
 * Name:    my_calloc
 * Purpose: allocate memory and set it to zero
 * Date:    November 24, 2001
 * Passed:  size: memory needed
 */
void *my_calloc( size_t size )
{
void *mem;
int  rc;

   mem = my_malloc( size, &rc );
   if (mem != NULL)
      memset( mem, 0, size );

   return( mem );
}


/*
 * Name:    my_strdup
 * Purpose: duplicate a NUL-terminated string
 * Author:  Jason Hood
 * Date:    August 10, 2005
 * Passed:  str:  string to copy
 * Returns: pointer to new copy of string, or NULL if no memory
 * Notes:   my_free should be called when the string is no longer needed
 */
char *my_strdup( const char *str )
{
char *p;
int  len;

   len = strlen( str ) + 1;
   p = my_malloc( len, &len );
   if (len != ERROR)
      memcpy( p, str, len );

   return( p );
}


/*
 * Name:    my_free
 * Purpose: free memory
 * Date:    November 25, 2001
 * Passed:  mem: pointer to memory to free
 * Notes:   can handle NULL pointers.
 */
void my_free( void *mem )
{
char *p;
mem_info *block;
int  i;
unsigned len;

   if (mem == NULL)
      return;

   p = (char *)mem - 1;

   if (*p == -1)
      free( p );

   else {
      i = find_block( p );
      len = *p + MIN_SPACE;
      if (group) {
         group[i] = TRUE;
         SPACE( p, len );
      } else {
         block = g_status.memory + i;
         if (block->space == 0)
            NEXT( p ) = PREV( p ) = OFS( p );
         else {
            char *next_free, *prev_free, *q = p + len;
            int  before = (p > block->base  &&  FREE( p-1 ));
            if (before) {
               unsigned space = PSIZE( p-1 );
               len += space;
               p -= space;
            }
            if (q < PTR( block->size )  &&  FREE( q )) {
               len += SIZE( q );
               next_free = PTR( NEXT( q ) );
               prev_free = PTR( PREV( q ) );
               if (before) {
                  if (next_free == prev_free)
                     NEXT( p ) = PREV( p ) = OFS( p );
                  else if (prev_free == p) {
                     NEXT( p ) = NEXT( q );
                     PREV( next_free ) = OFS( p );
                  } else if (next_free == p) {
                     PREV( p ) = PREV( q );
                     NEXT( prev_free ) = OFS( p );
                  } else {
                     NEXT( prev_free ) = NEXT( q );
                     PREV( next_free ) = PREV( q );
                  }
               } else {
                  if (prev_free == q)
                     NEXT( p ) = PREV( p ) = OFS( p );
                  else {
                     DWORD( p+1 ) = DWORD( q+1 );
                     NEXT( prev_free ) = PREV( next_free ) = OFS( p );
                  }
               }
            } else if (!before) {
               NEXT( p ) = OFS( block->free );
               PREV( p ) = PREV( block->free );
               prev_free = PTR( PREV( block->free ) );
               PREV( block->free ) = NEXT( prev_free ) = OFS( p );
            }
         }
         SPACE( p, len );

         block->free = p;

         if (len > block->space) {
            int si = find_block_size( i );
            block->space = len;
            adjust_sizes( block, si );
         }
      }
   }
}


/*
 * Name:    my_free_group
 * Purpose: indicate a lot of frees
 * Date:    November 29, 2001
 * Passed:  begin: TRUE to begin, FALSE to end
 * Notes:   used to increase performance of my_free when freeing a lot of
 *           items (such as closing a file).
 *          No allocations should be made during a group free.
 */
void my_free_group( int begin )
{
int  i, si;
mem_info *block;
char *p, *q, *end;
unsigned len, size, largest;

   if (begin)
      group = calloc( g_status.mem_num, sizeof(int) );

   else if (group != NULL) {
      for (i = g_status.mem_num; --i >= 0;) {
         if (group[i]) {
            block = g_status.memory + i;
            largest = 0;
            p = block->base;
            end = p + block->size;
            while (!FREE( p ))
               p += *p + MIN_SPACE;
            block->free = p;
            for (;;) {
               q = p;
               size = 0;
               do {
                  len = SIZE( p );
                  size += len;
                  p += len;
               } while (p < end  &&  FREE( p ));
               SPACE( q, size );

               if (size > largest)
                  largest = size;

               while (p < end  &&  !FREE( p ))
                  p += *p + MIN_SPACE;
               if (p >= end) {
                  NEXT( q ) = OFS( block->free );
                  PREV( block->free ) = OFS( q );
                  break;
               }
               NEXT( q ) = OFS( p );
               PREV( p ) = OFS( q );
            }
            if (largest > block->space) {
               si = find_block_size( i );
               block->space = largest;
               adjust_sizes( block, si );
            }
         }
      }
      free( group );
      group = NULL;
   }
}


/*
 * Name:    my_realloc
 * Purpose: resize already allocated memory
 * Date:    November 25, 2001
 * Passed:  mem:  pointer to current memory
 *          size: size of new memory
 *          rc:   pointer to return code
 */
void *my_realloc( void *mem, size_t size, int *rc )
{
char *old_mem, *new_mem;

   if (mem == NULL)
      return( my_malloc( size, rc ) );

   if (size == 0) {
      my_free( mem );
      return( NULL );
   }

   new_mem = mem;
   old_mem = new_mem - 1;

   if (*old_mem == -1) {
      if (size > MAX_SIZE) {
         new_mem = realloc( old_mem, size + 1 );
         if (new_mem != NULL)
            ++new_mem;
      } else {
         new_mem = allocate( size );
         if (new_mem != NULL) {
            memcpy( new_mem, mem, size );
            free( old_mem );
         }
      }
   } else if (size > MAX_SIZE) {
      new_mem = malloc( size + 1 );
      if (new_mem != NULL) {
         *new_mem++ = -1;
         memcpy( new_mem, mem, *old_mem + MIN_SIZE );
         my_free( mem );
      }
   } else {
      if (size < MIN_SIZE)
         size = MIN_SIZE;
      if (*old_mem + MIN_SIZE != size) {
         new_mem = allocate( size );
         if (new_mem != NULL) {
            if (*old_mem + MIN_SIZE < size)
               size = *old_mem + MIN_SIZE;
            memcpy( new_mem, mem, size );
            my_free( mem );
         }
      }
   }

   if (new_mem == NULL)
      *rc = ERROR;

   return( new_mem );
}


/*
 * Name:    my_memcpy
 * Purpose: copy memory
 * Date:    November 13, 1993
 * Passed:  dest: pointer to destination
 *          src:  pointer to source
 *          size: number of bytes to copy
 */
void my_memcpy( void *dest, void *src, size_t size )
{
   if (size > 0) {
      assert( dest != NULL );
      assert( src  != NULL );
      memcpy( dest, src, size );
   }
}


/*
 * Name:    my_memmove
 * Purpose: move memory
 * Date:    November 13, 1993
 * Passed:  dest: pointer to destination
 *          src:  pointer to source
 *          size: number of bytes to copy
 */
void my_memmove( void *dest, void *src, size_t size )
{
   if (size > 0) {
      assert( dest != NULL );
      assert( src  != NULL );
      memmove( dest, src, size );
   }
}


/*
 * Name:    init_memory
 * Purpose: initialise the memory allocation structures
 * Date:    December 2, 2001
 * Returns: ERROR if no memory available, OK otherwise
 * Notes:   requires one full chunk to be initially available.
 */
int  init_memory( void )
{
mem_info *block;

   g_status.errmsg = main4a;    /* not enough base memory */

   block = malloc( MEM_GROUP * (sizeof(mem_info) + sizeof(int)) );
   if (block == NULL)
      return( ERROR );

   block->base = malloc( MEM_CHUNK );
   if (block->base == NULL) {
      free( block );
      return( ERROR );
   }

   g_status.memory = block;
   g_status.mem_size = (int *)(block + MEM_GROUP);
   g_status.mem_size[0] = 0;

   g_status.mem_avail = MEM_GROUP;
   g_status.mem_num   = 1;
   g_status.mem_cur   = 0;

   block->size = block->space = MEM_CHUNK;
   block->base[0] = block->base[MEM_CHUNK-1] = -1;
   WORD( block->base + 1 + 2*SUS )         =
   WORD( block->base + MEM_CHUNK-1 - SUS ) = MEM_CHUNK;
   DWORD( block->base + 1 ) = 0;
   block->free = block->base;

   return( OK );
}


/*
 * Name:    allocate
 * Purpose: allocate memory in a block
 * Date:    November 24, 2001
 * Passed:  size: size to allocate
 * Notes:   assumes 0 < size < MAX_SIZE
 */
static void *allocate( unsigned size )
{
mem_info *block;
char *mem, *new_mem;
char *prev_free, *next_free;
unsigned space, len;
int  si;

   if (size < MIN_SIZE)
      size = MIN_SIZE;
   size += 2;           /* include the header and trailer */

   block = g_status.memory + g_status.mem_size[g_status.mem_cur];
   space = block->space;
   /*
    * Since we require six bytes minimum, we can extend an allocation if
    * there would be a couple of bytes left over. However, that won't work
    * if the extension exceeds 133. Eg: 60 bytes will fit in 62, by allocating
    * two extra bytes, but 130 won't fit in 134, because 134 won't fit in seven
    * bits (134 - 6 = 128 = eight bits).
    */
   if (space < size  ||  (space < size + MIN_SPACE  &&  space > MAX_SPACE)) {
      si = find_size( size, FALSE );
      if (si == -1) {
         /*
          * No block is big enough - create a new one.
          */
         len = g_status.mem_avail;
         if (g_status.mem_num == (int)len) {
            int new_len = len + MEM_GROUP;
            block = malloc( new_len * (sizeof(mem_info) + sizeof(int)) );
            if (block == NULL)
               return( NULL );
            memcpy( block, g_status.memory, len * sizeof(mem_info) );
            memcpy( block + new_len, g_status.mem_size, len * sizeof(int) );
            free( g_status.memory );
            g_status.memory = block;
            g_status.mem_size = (int *)(block + new_len);
            g_status.mem_avail = new_len;
         }
         len = MEM_CHUNK;
         while ((mem = malloc( len )) == NULL  &&  len >= MEM_MIN_CHUNK)
            len >>= 1;
         if (mem == NULL) {
            mem = malloc( size + 1 );
            if (mem == NULL)
               return( NULL );
            *mem = -1;
            return( mem + 1 );
         }
         /*
          * Maintain the array sorted by base pointers.
          */
         if (ptoul( mem ) > ptoul( g_status.memory[g_status.mem_num-1].base ))
            si = g_status.mem_num;
         else {
            int i;
            si = find_block( mem );
            memmove( g_status.memory + si + 1, g_status.memory + si,
                     (g_status.mem_num - si) * sizeof(mem_info) );
            /*
             * Shift all the relevant size index positions up one to account
             * for the new base index positions.
             */
            for (i = g_status.mem_num; --i >= 0;)
               if (g_status.mem_size[i] >= si)
                  ++g_status.mem_size[i];
         }
         g_status.memory[si].base = g_status.memory[si].free = mem;
         g_status.memory[si].size = g_status.memory[si].space = len;
         SPACE( mem, len );
         DWORD( mem+1 ) = 0;
         g_status.mem_size[g_status.mem_num] = si;
         si = g_status.mem_num++;
      }
      g_status.mem_cur = si;
      block = g_status.memory + g_status.mem_size[si];
      space = block->space;
   }

   mem = block->free;
   for (;;) {
      len = SIZE( mem );
      if (len >= size  &&  (len <= MAX_SPACE  ||  len >= size + MIN_SPACE))
         break;
      mem = PTR( NEXT( mem ) );
   }
   if (len < size + MIN_SPACE)
      size = len;
   new_mem = mem + size;
   *mem = *(new_mem - 1) = size - MIN_SPACE;
   prev_free = PTR( PREV( mem ) );
   next_free = PTR( NEXT( mem ) );
   if (len != size) {
      unsigned new_len = len - size;
      SPACE( new_mem, new_len );
      if (prev_free == mem)
         NEXT( new_mem ) = PREV( new_mem ) = OFS( new_mem );
      else {
         DWORD( new_mem + 1 ) = DWORD( mem + 1 );
         NEXT( prev_free ) = PREV( next_free ) = OFS( new_mem );
      }
      block->free = new_mem;
   } else {
      if (prev_free != mem) {
         block->free = PTR( NEXT( mem ) );
         NEXT( prev_free ) = NEXT( mem );
         PREV( next_free ) = PREV( mem );
      }
   }
   if (len == space) {
      find_largest_free( block );
      adjust_sizes( block, g_status.mem_cur );
   }

   return( mem + 1 );
}


/*
 * Name:    find_block
 * Purpose: find the block owning a pointer
 * Date:    November 26, 2001
 * Passed:  mem: pointer
 * Returns: index of block if found
 *          position in array where insertion should occur if not found
 * Notes:   assumes an allocated pointer will be found.
 *          the not found case is for maintaining the order of base pointers.
 */
static int  find_block( void *mem )
{
int  bot;
int  mid;
int  top;
unsigned long p, b;
mem_info *block;

   p = ptoul( mem );
   bot = 0;
   top = g_status.mem_num - 1;
   while (bot <= top) {
      mid = (bot + top) / 2;
      block = g_status.memory + mid;
      b = ptoul( block->base );
      if (p < b)
         top = mid - 1;
      else if (p >= b + block->size)
         bot = mid + 1;
      else
         return( mid );
   }

   return( bot );
}


/*
 * Name:    find_size
 * Purpose: find a block with a certain amount of free space
 * Date:    November 27, 2001
 * Passed:  size: free space required
 *          sort: TRUE to find an exact size for sorting
 * Returns: index in size array or -1 if no space is big enough
 * Notes:   attempts to find an exact size first, otherwise uses best fit.
 */
static int  find_size( unsigned size, int sort )
{
int  bot;
int  mid;
int  top;
int  i;
unsigned space;

   i = -1;
   bot = 0;
   top = g_status.mem_num - 1;
   while (bot <= top) {
      mid = (bot + top) / 2;
      space = g_status.memory[g_status.mem_size[mid]].space;
      if (size == space)
         return( mid );
      else if (size < space) {
         if (sort  ||  (space <= MAX_SPACE  ||  space >= size + MIN_SPACE))
            i = mid;
         top = mid - 1;
      } else
         bot = mid + 1;
   }

   return( i );
}


/*
 * Name:    find_block_size
 * Purpose: find the block in the size array
 * Date:    November 27, 2001
 * Passed:  block: index of block to find
 * Returns: index in size array
 */
static int  find_block_size( int block )
{
unsigned size;
int  i, j;

   size = g_status.memory[block].space;
   i = find_size( size, TRUE );

   if (block == g_status.mem_size[i])
      return( i );

   /*
    * Didn't find the right one, so try all the ones before it.
    */
   for (j = i - 1; j >= 0; --j) {
      if (size != g_status.memory[g_status.mem_size[j]].space)
         break;
      if (block == g_status.mem_size[j])
         return( j );
   }

   /*
    * Still didn't find it, so it must be one of the ones after it.
    */
   while (block != g_status.mem_size[++i]);
   return( i );
}


/*
 * Name:    find_largest_free
 * Purpose: find the largest available free space
 * Date:    November 24, 2001
 * Passed:  block: block in which to find
 * Notes:   the space member is set to the largest free space.
 */
static void find_largest_free( mem_info *block )
{
char *p, *q;
unsigned largest;
unsigned size;

   largest = 0;
   p = q = block->free;
   if (FREE( p )) {
      do {
         size = SIZE( p );
         if (size > largest) {
            largest = size;
            if (largest >= MAX_SPACE + MIN_SPACE) {
               block->free = p;
               break;
            }
         }
         p = PTR( NEXT( p ) );
      } while (p != q);
   }
   block->space = largest;
}


/*
 * Name:    adjust_sizes
 * Purpose: reposition a block in the size array
 * Date:    November 27, 2001
 * Passed:  block: block to reposition
 *          si:    its current index in the size array
 * Notes:   ensures the size array remains sorted.
 */
static void adjust_sizes( mem_info *block, int si )
{
unsigned size;
int  *sp;
int  num;
int  idx;

   size = block->space;
   sp = g_status.mem_size + si;
   num = g_status.mem_num - 1;
   /*
    * See if it's still correctly positioned.
    */
   if ((si == 0   || g_status.memory[sp[-1]].space <= size)  &&
       (si == num || g_status.memory[sp[+1]].space >= size))
      return;

   idx = *sp;

   /*
    * Remove it from the list so find_size will work correctly.
    */
   if (si != num)
      memmove( sp, sp + 1, (num - si) * sizeof(int) );

   --g_status.mem_num;

   si = find_size( size, TRUE );

   ++g_status.mem_num;

   if (si == -1)
      si = num;
   else
      memmove( g_status.mem_size + si + 1, g_status.mem_size + si,
               (num - si) * sizeof(int) );

   g_status.mem_size[si] = idx;
}


#if defined( __DOS16__ )
/*
 * Name:    ptoul - pointer to unsigned long
 * Purpose: convert a (far) pointer to unsigned long integer
 * Date:    June 5, 1991
 * Passed:  s:  a (far) pointer
 * Notes:   combine the offset and segment like so:
 *                offset       0000
 *                segment   + 0000
 *                          =======
 *                            00000
 *          result is returned in dx:ax
 */
#if defined( __TURBOC__ )
#pragma warn -rvl
#endif
static unsigned long ptoul( void *s )
{
   ASSEMBLE {
        mov     ax, word ptr s          /* ax = OFFSET of s */
        mov     dx, word ptr s+2        /* dx = SEGMENT of s */
        mov     bx, dx          /* put copy of segment in bx */
        mov     cl, 12          /* cl = 12 - shift hi word 3 hex digits */
        shr     dx, cl          /* convert to 'real segment' */
        mov     cl, 4           /* cl = 4  - shift hi word 1 hex digit left */
        shl     bx, cl          /* shift bx - add 3 digits of seg to 4 of off */
        add     ax, bx          /* add low part of segment to offset */
        adc     dx, 0           /* if carry, bump to next 'real' segment */
   }
}
#if defined( __TURBOC__ )
#pragma warn +rvl
#endif
#endif
