/*  FILE:   OUTPUT_RITUALS.CPP

    Copyright (c) 2008-2009 by Lone Wolf Development, Inc.  All rights reserved.

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


static void Output_Key_Skills(T_Ritual_Info * info)
{
    T_Int32U            i, length, temp_length, chunk_count;
    T_Glyph_Ptr         ptr, temp, chunks[1000];
    T_Glyph             buffer[100000], message[1000];

    if ((info->keyskill == NULL) || (info->keyskill[0] == '\0')) {
        sprintf(message, "No key skill present for ritual %s\n", info->name);
        Log_Message(message);
        return;
        }

    /* First, check for '(no check)' at the end
    */
    temp = "(no check)";
    temp_length = strlen(temp);
    ptr = info->keyskill;
    length = strlen(ptr);
    if (temp_length < length) {
        ptr = info->keyskill + length - temp_length;
        if (strcmp(ptr, temp) == 0) {
            Output_Tag(info->node, "RitualSkl", "NoCheck", info);
            while ((ptr > info->keyskill) && (ptr[-1] == ' '))
                ptr--;
            *ptr = '\0';
            }
        }

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->keyskill, ", ");
    for (i = 0; i < chunk_count; i++) {

        /* If this is 'or', ignore it
        */
        if (strcmp(chunks[i], "or") == 0)
            continue;

        Output_Tag(info->node, "RitualSkl", chunks[i], info, "skill");
        }
}


T_Status    C_DDI_Rituals::Output_Entry(T_XML_Node root, T_Ritual_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph_Ptr     ptr, src;
    bool            is_level;
    T_Glyph         ritual_id[UNIQUE_ID_LENGTH+1], buffer[5000];

    /* Generate a unique id for the ritual - if we can't, get out
    */
    if (!Generate_Thing_Id(ritual_id, "rt", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for ritual %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(ritual_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Ritual", info->name, ritual_id, info->description,
                                UNIQUENESS_NONE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    is_level = false;
    if (!x_Is_Digit(info->level[0]))
        ptr = info->level;
    else {
        is_level = true;
        Output_Tag(info->node, "ReqLevel", As_Int(atoi(info->level)), info);
        ptr = strchr(info->level, ' ');
        while ((ptr != NULL) && (*ptr == ' '))
            ptr++;
        }
    if ((ptr != NULL) && (ptr[0] != '\0'))
        Output_Field(info->node, "ritLevText", ptr, info);
    else if (!is_level) {
        sprintf(buffer, "No ritual level found for ritual %s\n", info->name);
        Log_Message(buffer);
        }

    Output_Field(info->node, "ritCompCst", info->componentcost, info);

    Output_Key_Skills(info);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    Split_To_Dynamic_Tags(info->node, info, info->category, ",", "RitualCat", "ritualcat");

    Output_Field(info->node, "ritTime", info->time, info);

    if ((info->duration != NULL) && (info->duration[0] != '\0'))
        Output_Field(info->node, "ritLasts", info->duration, info);

    if (x_Is_Digit(info->price[0])) {
        strcpy(buffer, info->price);
        ptr = buffer;
        src = buffer;
        while (*src != '\0') {
            if (*src != ',')
                *ptr++ = *src;
            src++;
            }
        *ptr = '\0';
        Output_Field(info->node, "ritPrice", As_Int(atoi(buffer)), info);
        }
    else if ((strcmp(info->price, "Unique") != 0) && (strcmp(info->price, "-") != 0) &&
             (strcmp(info->price, "None") != 0)) {
        sprintf(buffer, "Invalid price '%s' found for ritual %s\n", info->price, info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


T_Status    C_DDI_Rituals::Post_Process_Entry(T_XML_Node root, T_Ritual_Info * info)
{
    /* Now that our feats all have unique ids, output our prereqs -
        we need to add identity tags for feats that may not have been processed
        yet, so this must be done in the Post_Process stage
    */
    Output_Prereqs(info, &m_id_list);

   x_Status_Return_Success();
}