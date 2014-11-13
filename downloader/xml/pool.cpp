/*  FILE:   POOL.CPP

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

    Implementation of a class for managing a storage pool. The pool can be
    allocated from in pieces, growing as needed. When the pool is no longer
    needed, all resources are disposed of at once. The advantage of a pool is
    that it is ideal for handling high-performance allocations of small chunks
    (such as small strings) without all of the overhead of the memory manager.
    No individual allocations can be freed, so it's great when reading in a
    file or synthesizing data for subsequent output - process the entire block
    of data, then discard it all in one shot.

    Note! All storage herein is consequentive, without any sentinels or overrun
            checking. Memory allocated from a pool must be handled with care,
            else the entire pool can be corrupted quite readily.
*/


#include    "private.h"


/*  Instantiate the object, specifying the initial size and increment to grow by
    when the current allocation is exhausted.
    Note! We use malloc for portability. This code is published as part of the
            XML source for AB.
*/
C_Pool::C_Pool(T_Int32U start,T_Int32U grow)
{
    m_buffer = (char *) malloc(start);
    memset(m_buffer,0,start);
    m_size = start;
    m_grow = grow;
    m_position = 0;
    m_ptr = m_buffer;
    m_next = NULL;
    m_alloc = this;
}


C_Pool::~C_Pool()
{
    if (m_next != NULL)
        x_Delete(m_next);
    free(m_buffer);
}


C_Pool *    C_Pool::Grow(T_Int32U grow)
{
    C_Pool *    next;

    /* allocate a new pool; if failed, throw an exception
        NOTE! We need to use m_grow for the grow size in case we're growing an
                extra large amount to accommodate an exceptional sized request.
    */
    next = new C_Pool(grow,m_grow);
    if (next == NULL)
        x_Exception(-1000);

    /* chain to the new pool and return a pointer to it
    */
    m_next = next;
    return(next);
}


void *      C_Pool::Acquire(T_Int32U size)
{
    void *      ptr;
    T_Int32U    grow;

    /* if we don't have enough space in the current pool, allocate a new one;
        if our default grow size isn't big enough to honor the request, make
        sure to allocate a sufficiently large pool just this one time
    */
    if (size + m_alloc->m_position >= m_alloc->m_size) {
        grow = m_grow;
        if (size > grow)
            grow = ((size / m_grow) + 1) * m_grow;
        m_alloc = m_alloc->Grow(grow);
        }

    /* extract the requested block from the pool
    */
    ptr = m_alloc->m_ptr;
    m_alloc->m_ptr += size;
    m_alloc->m_position += size;
    return(ptr);
}


void        C_Pool::Reset(void)
{
    /* delete all pools chained from this one
    */
    if (m_next != NULL)
        x_Delete(m_next);

    /* reset member variables to re-use the current buffer for this pool
    */
    m_position = 0;
    m_ptr = m_buffer;
    m_next = NULL;
    m_alloc = this;
}


T_Int32U    C_Pool::Consumed_Size(void)
{
    T_Int32U    size;
    C_Pool *    pool;

    for (size = 0, pool = this; pool->m_next != NULL; pool = pool->m_next)
        size += pool->m_size;
    size += pool->m_position;
    return(size);
}


T_Int32U    C_Pool::Block_Count(void)
{
    T_Int32U    count;
    C_Pool *    pool;

    for (count = 1, pool = this; pool->m_next != NULL; pool = pool->m_next, count++);
    return(count);
}


void *      C_Pool::Base_Pointer(T_Int32U index)
{
    T_Int32U    count;
    C_Pool *    pool;

    for (count = 0, pool = this;
         (count < index) && (pool->m_next != NULL);
         pool = pool->m_next, count++);
    x_Assert_Opt(count >= index);
    return(pool->m_buffer);
}
