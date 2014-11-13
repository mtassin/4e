/*  FILE:   OUTPUT_DEITIES.CPP

    Copyright (c) 2008-2011 by Lone Wolf Development, Inc.  All rights reserved.

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
*/


#include "private.h"


T_Status    C_DDI_Deities::Output_Entry(T_XML_Node root, T_Deity_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph         deity_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Generate a unique id for the class - if we can't, get out
    */
    if (!Generate_Thing_Id(deity_id, "dty", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for deity %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(deity_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Deity", info->name, deity_id, info->description,
                                UNIQUENESS_UNIQUE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* Add this deity to our internal list
    */
    Mapping_Add("deity", info->name, m_pool->Acquire(deity_id));

    if (stricmp(info->alignment, "Lawful Good") == 0) {
        Output_Tag(info->node, "Alignment", "Lawful", info);
        Output_Tag(info->node, "Alignment", "Good", info);
        }
    else if (stricmp(info->alignment, "Chaotic Evil") == 0) {
        Output_Tag(info->node, "Alignment", "Chaotic", info);
        Output_Tag(info->node, "Alignment", "Evil", info);
        }
    else if (stricmp(info->alignment, "Unaligned") != 0)
        Output_Tag(info->node, "Alignment", info->alignment, info, "alignment");

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    Output_Field(info->node, "dtyFlavor", info->flavor, info);

    Output_Tag(info->node, "Gender", info->gender, info, "gender");

    Output_Field(info->node, "dtySphere", info->sphere, info);

    Output_Field(info->node, "dtyDomin", info->dominion, info);

    Output_Field(info->node, "dtyPriests", info->priests, info);

    Output_Field(info->node, "dtyAdject", info->adjective, info);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


T_Status    C_DDI_Deities::Post_Process_Entry(T_XML_Node root, T_Deity_Info * info)
{
    x_Status_Return_Success();
}