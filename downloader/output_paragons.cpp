/*  FILE:   OUTPUT_PARAGONS.CPP

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


static void Output_Features(T_Paragon_Info * info, T_XML_Node root, T_List_Ids * id_list,
                            C_Pool * pool)
{
    T_Status            status;
    T_Int32U            level, path_name_length;
    T_Int32S            result;
    T_Glyph_Ptr         ptr, end, temp, temp2, name, desc, truncate_at;
    T_XML_Node          feature, tag;
    T_Glyph             buffer[5000], buffer2[5000];

    if ((info->features == NULL) || (info->features[0] == '\0')) {
        sprintf(buffer, "No class features found for paragon path %s\n", info->name);
        Log_Message(buffer);
        return;
        }

    path_name_length = strlen(info->name);

    ptr = strstr(info->features, "<b>");
    while (ptr != NULL) {
    	if (x_Trap_Opt(*ptr == '\0')) {
            sprintf(buffer, "Empty path feature text for path %s\n", info->name);
            Log_Message(buffer);
            return;
    		}

    	/* Skip past any extra line breaks, or they'll cause incorrect matches
    		with the end = strstr line below
    	*/
    	while (strncmp(ptr, "{br}", 4) == 0)
    		ptr += 4;

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

        /* The name goes from the bold tag to the "(11" part - if it starts
            <h1, it's a power, so we should skip to something that identifies
            the end of the power - powers get added separately.
        */
        if (strncmp(ptr, "<h1", 3) == 0) {
            temp = strstr(end, "{br}</p>");
            if (temp != NULL)
                end = temp + 8;
            goto next_feature;
            }
        ptr += 3;
        name = ptr;
        if (strncmp(ptr, "<h1", 3) == 0) {
            temp = strstr(end, "{br}</p>");
            if (temp != NULL)
                end = temp + 8;
            goto next_feature;
            }
        temp = strstr(ptr, "(");
        truncate_at = temp;

        /* If we find the paragon path name in brackets, skip it... this stops
            "Power Name (Path Name) (11th level)" from messing us up
        */
        if ((temp != NULL) && !x_Is_Digit(temp[1]) && strncmp(temp+1, info->name, path_name_length) == 0) {
            temp++;
            temp = strstr(temp, "(");
            }

        /* If we don't find a level indicator like that, assume we'll have to
            add the level as a mapping
        */
        if ((temp == NULL) || !x_Is_Digit(temp[1])) {
            level = 0;
            temp2 = strstr(ptr, "</b>");
            if (x_Trap_Opt(temp2 == NULL))
            	goto next_feature;
            *temp2 = '\0';
            ptr = temp2;
            }
        else {
            level = atoi(temp+1);
            ptr = temp+1;
            while ((truncate_at > name) && (truncate_at[-1] == ' '))
                truncate_at--;
            *truncate_at = '\0';
            ptr = strstr(ptr, "</b>");
            }
        if (x_Trap_Opt(ptr == NULL)) {
            sprintf(buffer, "Couldn't get path feature text for %s\n", info->name);
            Log_Message(buffer);
            goto next_feature;
            }
        ptr += 4;
        while ((*ptr == ':') || (*ptr == ' '))
            ptr++;

        /* If we already found a path using this feature, reuse it
        */
        temp = Mapping_Find("pathfeats", name);
        if (temp != NULL) {
            Output_Bootstrap(info->node, temp, info);
            goto next_feature;
            }

        /* Now we have the name of the feature, we can try to generate a unique
            id for it.
        */
        if (!Generate_Thing_Id(buffer2, "fPar", name, id_list)) {
            sprintf(buffer, "Error generating unique id for path feature %s\n", name);
            Log_Message(buffer);
            goto next_feature;
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

        /* If this just adds a power, auto-hide it
        */
        if (Is_Regex_Match("^You gain the .+ power.$", desc))
            Output_Tag(feature, "Hide", "Special", info);

        /* Add a bootstrap node for this feature
        */
        result = Output_Bootstrap(info->node, buffer2, info);
        if (x_Trap_Opt(result != 0))
            goto next_feature;

        /* Save the id we used, so if we find a path using an effect with
            a duplicate name, we can reuse it
        */
        Mapping_Add("pathfeats", pool->Acquire(name), pool->Acquire(buffer2));

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


T_Status    C_DDI_Paragons::Output_Entry(T_XML_Node root, T_Paragon_Info * info)
{
    T_Status        status = LWD_ERROR;
    T_Int32S        result;
    T_Glyph         paragon_id[UNIQUE_ID_LENGTH+1], buffer[500];

    /* Generate a unique id for the paragon path - if we can't, get out
    */
    if (!Generate_Thing_Id(paragon_id, "pp", info->name, &m_id_list)) {
        sprintf(buffer, "Error generating unique id for paragon path %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    info->id = m_pool->Acquire(paragon_id);

    /* Now that all our required information has been discerned, we can go
        ahead and create the node
    */
    result = Create_Thing_Node(root, m_term, "Paragon", info->name, paragon_id, info->description,
                                UNIQUENESS_UNIQUE, info->source, &info->node, info->is_partial);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* Add this path to our internal list of paths
    */
    Mapping_Add("path", info->name, m_pool->Acquire(paragon_id));

    /* If we don't have a password, we can't get any more data
    */
    if (info->is_partial) {
        status = SUCCESS;
        goto cleanup_exit;
        }

    if ((info->flavor != NULL) && (info->flavor[0] != '\0')) {
        result = Output_Field(info->node, "pptFlavor", info->flavor, info);
        if (x_Trap_Opt(result != 0))
            goto cleanup_exit;
        }

    Output_Features(info, root, &m_id_list, m_pool);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


static void Bootstrap_Powers(T_XML_Node pathnode, T_Glyph_Ptr path_id, T_Paragon_Info * info)
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
        result = XML_Get_First_Named_Child_With_Attr(node, "tag", "group", "PowerPath", &child);
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


T_Status    C_DDI_Paragons::Post_Process_Entry(T_XML_Node root, T_Paragon_Info * info)
{
    T_Glyph_Ptr     path_id;
    T_Glyph         buffer[1000];

    /* Now that our feats all have unique ids, output our prereqs -
        we need to add identity tags for feats that may not have been processed
        yet, so this must be done in the Post_Process stage
    */
    Output_Prereqs(info, &m_id_list);

    /* Get our class id - we'll need it later
    */
    path_id = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(info->node, "id");
    if (x_Trap_Opt(path_id == NULL)) {
        sprintf(buffer, "Error retrieving unique id for paragon path %s\n", info->name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Now bootstrap all powers for this paragon path
    */
    Bootstrap_Powers(info->node, path_id, info);

cleanup_exit:
    x_Status_Return_Success();
}