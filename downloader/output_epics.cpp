/*  FILE:   OUTPUT_EPICS.CPP

    Copyright (c) 2008-2010 by Lone Wolf Development, Inc.  All rights reserved.

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


static void Output_Features(T_Epic_Info * info, T_XML_Node root, T_List_Ids * id_list,
                            C_Pool * pool)
{
    T_Status            status;
    T_Int32U            level;
    T_Int32S            result;
    T_Glyph             backup;
    T_Glyph_Ptr         ptr, end, temp, name, appears, desc;
    T_XML_Node          feature, tag;
    T_Glyph             buffer[5000], buffer2[5000];

    if ((info->features == NULL) || (info->features[0] == '\0')) {
        sprintf(buffer, "No class features found for epic destiny %s\n", info->name);
        Log_Message(buffer);
        return;
        }

    ptr = strstr(info->features, "<b>");
    while (ptr != NULL) {

        /* Look for the end of this feature - this will either be the first
            succeeding "{br}<b>" sequence or a "{br}<p>" sequence. Skip past
            the {br} tags only when we find them, leave the rest to be parsed
            out later.
        */
        end = strstr(ptr, "{br}<b>");
        temp = strstr(ptr, "{br}<p>");
        if ((end == NULL) || ((temp != NULL) && (temp < end)))
            end = temp;
        if (end != NULL) {
            *end = '\0';
            end += 4;
            }

        /* The name goes from the bold tag to the first ( character
        */
        ptr += 3;
        name = ptr;
        temp = strchr(ptr, '(');
        if (x_Trap_Opt(temp == NULL)) {
            sprintf(buffer, "Couldn't get path feature name for %s\n", info->name);
            Log_Message(buffer);
            goto next_feature;
            }
        level = atoi(temp+1);
        ptr = temp+1;
        while ((temp > name) && (temp[-1] == ' '))
            temp--;
        *temp = '\0';
        ptr = strstr(ptr, "</b>");
        if (x_Trap_Opt(ptr == NULL)) {
            sprintf(buffer, "Couldn't get path feature text for %s\n", info->name);
            Log_Message(buffer);
            goto next_feature;
            }
        ptr += 4;
        while ((*ptr == ':') || (*ptr == ' '))
            ptr++;

        /* If we already found a destiny using this feature, reuse it
        */
        temp = Mapping_Find("epicfeats", name);
        if (temp != NULL) {
            Output_Bootstrap(info->node, temp, info);
            goto next_feature;
            }

        /* Now we have the name of the feature, we can try to generate a unique
            id for it.
        */
        if (!Generate_Thing_Id(buffer2, "fEpc", name, id_list)) {
            sprintf(buffer, "Error generating unique id for path feature %s\n", name);
            Log_Message(buffer);
            goto next_feature;
            }

        /* If we have a "{br}<i>Appears in" sequence, terminate stuff there -
            we don't want to include that in the description
        */
        appears = strstr(ptr, "{br}<i>Appears in");
        if (appears != NULL) {
            backup = *appears;
            *appears = '\0';
            }

        /* Parse out the description text - this converts any formatting or
            tabular data in it
        */
        status = Parse_Description_Text(&desc, ptr, pool);
        if (x_Trap_Opt(!x_Is_Success(status)))
            desc = ptr;

        /* Output a new thing for this feature - note that it can't be unique,
            because some scripts on it need to be able to find its source.
        */
        Create_Thing_Node(root, "path feature", "ClassFeat",
                                name, buffer2, desc,
                                UNIQUENESS_NONE, NULL, &feature, info->is_partial);
        if (appears != NULL)
            *appears = backup;

        /* Add a bootstrap node for this feature
        */
        result = Output_Bootstrap(info->node, buffer2, info);
        if (x_Trap_Opt(result != 0))
            goto next_feature;

        /* Save the id we used, so if we find a destiny using an effect with
            a duplicate name, we can reuse it
        */
        Mapping_Add("epicfeats", pool->Acquire(name), pool->Acquire(buffer2));

        /* Add a level requirement tag
        */
        if (level != 0) {
            result = XML_Create_Child(feature, "tag", &tag);
            if (x_Trap_Opt(result != 0)) {
                sprintf(buffer, "Error creating tag node for path feature %s\n", name);
                Log_Message(buffer);
                goto next_feature;
                }
            XML_Write_Text_Attribute(tag, "group", "ReqLevel");
            XML_Write_Integer_Attribute(tag, "tag", level);
            }

next_feature:
        if (end == NULL)
            break;
        ptr = end;
        }
}


T_Status    C_DDI_Epics::Output_Entry(T_XML_Node root, T_Epic_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph         epic_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Generate a unique id for the epic destiny - if we can't, get out
    */
    if (!Generate_Thing_Id(epic_id, "ed", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for epic destiny %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(epic_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "EpicDest", info->name, epic_id, info->description,
                                UNIQUENESS_UNIQUE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* Add this destiny to our internal list of destinies
    */
    Mapping_Add("destiny", info->name, info->id);

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    if ((info->flavor != NULL) && (info->flavor[0] != '\0')) {
        result = Output_Field(info->node, "edsFlavor", info->flavor, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        }

    Output_Features(info, root, &m_id_list, m_pool);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


static void Bootstrap_Powers(T_XML_Node pathnode, T_Glyph_Ptr path_id, T_Epic_Info * info)
{
    T_Int32S            result;
    T_Glyph_CPtr        cptr;
    T_XML_Document      document;
    T_XML_Node          root, node, child;

    /* Get the XML document for the powers list, and iterate through the powers
        in it.
    */
    document = C_DDI_Powers::Get_Output_Document();
    result = XML_Get_Document_Node(document, &root);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not get XML document root.");
        return;
        }
    result = XML_Get_First_Named_Child(root, "thing", &node);
    while (result == 0) {

        /* if this power isn't for the right class, skip it.
        */
        result = XML_Get_First_Named_Child_With_Attr(node, "tag", "group", "PowerDest", &child);
        if (result != 0)
            goto next_power;
        cptr = XML_Get_Attribute_Pointer(child, "tag");
        if (strcmp(cptr, path_id) != 0)
            goto next_power;

        /* Otherwise, bootstrap the power
        */
        cptr = XML_Get_Attribute_Pointer(node, "id");
        result = Output_Bootstrap(pathnode, cptr, info);
        x_Trap_Opt(result != 0);

next_power:
        result = XML_Get_Next_Named_Child(root, &node);
        }
}


T_Status    C_DDI_Epics::Post_Process_Entry(T_XML_Node root, T_Epic_Info * info)
{
    T_Glyph_Ptr     path_id;
    T_Glyph         buffer[1000];

    /* Now that our feats all have unique ids, output our prereqs - we need to
        add identity tags for feats that may not have been processed yet, so
        this must be done in the Post_Process stage
    */
    Output_Prereqs(info, &m_id_list);

    /* Get our class id - we'll need it later
    */
    path_id = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(info->node, "id");
    if (x_Trap_Opt(path_id == NULL)) {
        sprintf(buffer, "Error retrieving unique id for epic destiny %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Now bootstrap all powers for this epic destiny
    */
    Bootstrap_Powers(info->node, path_id, info);

cleanup_exit:
    x_Status_Return_Success();
}