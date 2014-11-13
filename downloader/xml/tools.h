/*  FILE:   TOOLS.H

    Copyright (c) 2000-2009 by Lone Wolf Development.  All rights reserved.

    This code is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation; either version 2 of the License, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
    more details.

    You should have received a copy of the GNU General Public License along with
    this program; if not, write to the Free Software Foundation, Inc., 59 Temple
    Place, Suite 330, Boston, MA 02111-1307 USA

    You can find more information about this project here:

    http://code.google.com/p/ddidownloader/

    This file includes:

    Definitions for support mechanisms used by XML code.
*/


#include <cstring>


/*  define constants used in our string processing stuff
*/
#define MAX_STRING_PARAMS       5


/*  define basic types used in certain places
*/
typedef unsigned char   T_Int8U;
typedef unsigned long   T_Int32U;
typedef char            T_Glyph;
typedef T_Glyph *       T_Glyph_Ptr;
typedef const T_Glyph * T_Glyph_CPtr;
typedef unsigned int    T_Boolean;
typedef T_Int32U        T_Status;


/*  if we don't have true/false yet (used for T_Boolean) define them
*/
#ifndef FALSE
#define FALSE           0
#endif
#ifndef TRUE
#define TRUE            (!FALSE)
#endif


/*  define a function we can set a breakpoint in if we're running under the
    debugger
*/
inline void Breakpoint(T_Glyph_Ptr filename, T_Int32U line, T_Glyph_Ptr condition)
{
    /* do nothing - set a breakpoint here in the debugger, then call this
        function whenever you want to hit it
    */
}


/*  define a 'trap' function that can be used if you want to be notified of an
    unexpected condition
*/
#define x_Trap_Opt(cond) \
            (((cond) != 0) \
                ? Breakpoint(__FILE__,__LINE__,#cond), \
                  1 \
                : 0)


/*  define structures for internal message string management
*/
typedef struct {
    T_Int32U    id;
    T_Glyph_Ptr text;
    }   T_Internal_String;

typedef struct {
    T_Int32U            module;     // corresponding MODULE_ID_ constant
    T_Int32U            count;
    T_Internal_String * list;
    T_Boolean           is_sorted;
    }   T_Internal_String_Table;


/*  define a quick function to load a string from an internal string table
*/
inline T_Glyph_Ptr Tools_Load_String(T_Int32U id, T_Internal_String_Table * table)
{
    T_Int32U        i, count;

    count = table->count;
    for (i = 0; i < count; i++) {
        if (table->list[i].id == id)
            return(table->list[i].text);
        }
    x_Trap_Opt(1);
    return("Unknown String!");
}


/*  define a few macros that turn certain private functions into no-ops
*/
#define Debug_Printf            (void)
#define x_Trap(x)               x
#define x_Break_Opt()           (0)
#define x_Assert(x)             (0)
#define x_Assert_Opt(x)         (0)
#define x_Delete(x)             delete x
#define PUBLISH                 /*ignore*/


/*  Define versions of the isX character classification functions. Casting the
    parameter to an unsigned char here solves sign-extension problems that
    occur when you pass a character with a value > 128 to these functions.
*/
#define x_Is_Punct(x)       ispunct((unsigned char) (x))
#define x_Is_Alnum(x)       isalnum((unsigned char) (x))
#define x_Is_Cntrl(x)       iscntrl((unsigned char) (x))
#define x_Is_Digit(x)       isdigit((unsigned char) (x))
#define x_Is_Print(x)       isprint((unsigned char) (x))
#define x_Is_Space(x)       isspace((unsigned char) (x))
#define x_Is_Alpha(x)       isalpha((unsigned char) (x))


/*  turn off some annoying compiler warnings about strnicmp, etc, not being the
    right names to use - we don't care
*/
#pragma warning( disable : 4996 )


/*  define a simple class for handling exceptions - it is entirely inline
*/
class   C_Exception
{
public:
    C_Exception(long errcode,const T_Glyph * filename,unsigned long linenum)
                    {   m_errcode = errcode;
                        m_filename = (T_Glyph *) filename;
                        m_linenum = linenum;
                    }

    inline  long            Get_Error(void)
                                { return(m_errcode); }
    inline  T_Glyph *       Get_Filename(void) const
                                { return(m_filename); }
    inline  unsigned long   Get_Line_Number(void)
                                { return(m_linenum); }

protected:
    long            m_errcode;
    T_Glyph *       m_filename;
    unsigned long   m_linenum;
};


/*  define a macro that throws an exception with the proper file/line info
*/
#define x_Exception(errcode)    throw(C_Exception(errcode,__FILE__,__LINE__))


/*  define a class for managing an accumulated allocation string that gets
    slowly built and then thrown away en masse when no longer needed
*/
class   C_Napkin
{
public:
    C_Napkin(T_Int32U start = 10000,T_Int32U grow = 10000);
    ~C_Napkin();

    inline  T_Glyph *   Get(void)
                            { return(m_buffer); }
    inline  void        Reset(void)
                            { m_position = 0; m_ptr = m_buffer; *m_ptr = '\0'; }

    C_Napkin *  Append(const T_Glyph * text);
    C_Napkin *  Append(T_Glyph ch);

private:
    void        Grow(T_Int32U minsize);

    T_Glyph *   m_buffer;
    T_Int32U    m_size;
    T_Int32U    m_grow;
    T_Int32U    m_position;
    T_Glyph *   m_ptr;
};


/*  define a class for managing a sub-allocated storage pool that is high
    performance due to its inability to handle freeing of individual elements
*/
class   C_Pool
{
public:
    C_Pool(T_Int32U start = 10000,T_Int32U grow = 10000);
    ~C_Pool();

    inline T_Glyph_Ptr  Acquire(T_Glyph_CPtr buffer)
                            { T_Glyph_Ptr   ptr;
                              ptr = (T_Glyph_Ptr) Acquire(strlen(buffer) + 1);
                              strcpy(ptr, buffer);
                              return(ptr);
                            }

    void *      Acquire(T_Int32U size);

    void        Reset(void);

    T_Int32U    Consumed_Size(void);

    T_Int32U    Block_Count(void);
    void *      Base_Pointer(T_Int32U index);

private:
    C_Pool *    Grow(T_Int32U grow);

    char *      m_buffer;   // char* is used as an arbitrary pointer to bytes - NOT text
    T_Int32U    m_size;
    T_Int32U    m_grow;
    T_Int32U    m_position;
    char *      m_ptr;      // char* is used as an arbitrary pointer to bytes - NOT text
    C_Pool *    m_next;
    C_Pool *    m_alloc;
};


/* Functions provided by the downloader
*/
T_Status    Quick_Write_Text(T_Glyph_Ptr filename,T_Glyph_Ptr text);


/*  define functions to aid in formatting messages
*/
#define As_Glyph(value)     As_Any("%c",value)
#define As_Int(value)       As_Any("%d",value)
#define As_Hex(value)       As_Any("%x",value)
#define As_Float(value)     As_Any("%g",value)
#define As_Id(id)           UniqueId_To_Text(id)
T_Glyph_Ptr     String_Printf(T_Glyph_Ptr target,T_Glyph_Ptr format,...);
T_Glyph_Ptr     As_Any(T_Glyph_Ptr format,...);
