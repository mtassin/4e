/*  FILE:   OUTPUT.H

    Copyright (c) 2008-2012 by Lone Wolf Development, Inc.  All rights reserved.

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

    This file implements the C_DDI_Output template class. It has to be in a
    .h file, and not a .cpp file, and must be #included almost anywhere you
    use a class based on C_DDI_Output. Yes this sucks and C++ is a terrible
    language.
*/


template <class T> C_DDI_Output<T>::C_DDI_Output(T_Glyph_Ptr term, T_Glyph_Ptr first_filename,
                                                C_Pool * pool)
{
    m_term = term;
    m_pool = pool;
    m_output_document = NULL;
    memset(m_docs, 0, sizeof(m_docs));
    sprintf(m_docs[0].filename, "ddi_%s.dat", first_filename);
}


template <class T> C_DDI_Output<T>::~C_DDI_Output()
{
    T_Int32U        i;

    /* Destroy all XML documents
    */
    for (i = 0; i < MAX_XML_CONTAINERS; i++)
        if (m_docs[i].document != NULL)
            XML_Destroy_Document(m_docs[i].document);
    m_output_document = NULL;
    memset(m_docs, 0, sizeof(m_docs));
}


template <class T> T_Int32U C_DDI_Output<T>::Append_Item(T * info)
{
    /* Add the item to the list and return the index - we can't return a
        pointer to the item, since the list might grow and invalidate it
    */
    m_list.push_back(*info);
    return(m_list.size() - 1);
}


template <class T> void C_DDI_Output<T>::Append_Potential_Item(T * info)
{
    /* Add the item to the list of potential entries. It may or may not get
        added, based on whether a duplicate is present in the list.
    */
    m_potentials.push_back(*info);
}


template <class T> T *      C_DDI_Output<T>::Get_Item(T_Int32U index)
{
    if (index >= m_list.size())
        return(NULL);
    return(&m_list[index]);
}


template <class T> T *      C_DDI_Output<T>::Find_Item(T_Glyph_CPtr item_name)
{
    T_Int32U        i, count;

    count = m_list.size();
    for (i = 0; i < count; i++) {
        if (stricmp(item_name, m_list[i].name) == 0)
            return(&m_list[i]);
        }

    return(NULL);
}


static void Get_Root(T_XML_Node * root, T_Glyph_Ptr filename, T_XML_Container docs[MAX_XML_CONTAINERS])
{
    T_Int32U            i;

    *root = docs[0].root;
    if (filename != NULL) {
        for (i = 1; i < MAX_XML_CONTAINERS; i++) {
            if (strcmp(docs[i].filename, filename) == 0)
                break;
            if (docs[i].filename[0] == '\0')
                break;
            }
        if ((i >= MAX_XML_CONTAINERS) || (docs[i].filename[0] == '\0'))
            Log_Message("*** Couldn't find XML container for file!\n");
        else
            *root = docs[i].root;
        }
}


template <class T> bool C_DDI_Output<T>::Is_Potential_Match(T * info, T * potential)
{
    /* By default, just check the name
    */
    return(strcmp(info->name, potential->name) == 0);
}


template <class T> void C_DDI_Output<T>::Resolve_Potentials(void)
{
    T_Int32U        i, potential_count, j, list_count;
    T *             info;
    T *             potential;

    potential_count = m_potentials.size();
    for (i = 0; i < potential_count; i++) {
        potential = &m_potentials[i];
        list_count = m_list.size();

        /* Check to see whether this potential entry matches anything in our
            list already - if not, add it to the list
        */
        for (j = 0; j < list_count; j++) {
            info = &m_list[j];
            if (Is_Potential_Match(info, potential))
                break;
            }
        if (j >= list_count)
            m_list.push_back(*potential);
        }
}


template <class T> T_Status C_DDI_Output<T>::Output(void)
{
    T_Status        status = LWD_ERROR;
    T_Int32U        i, count;
    long            result;
    T *             info;
    T_XML_Node      root;
    T_Glyph         buffer[500];

    sprintf(buffer, "Generating %s entries... ", m_term);
    Log_Message(buffer, true);

    /* Create all necessary XML documents and get their roots
    */
    for (i = 0; i < MAX_XML_CONTAINERS; i++) {
        if (m_docs[i].filename[0] == '\0')
            break;
        result = XML_Create_Document(&m_docs[i].document, DTD_Get_Data(), false);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create XML document.");
            goto cleanup_exit;
            }
        result = XML_Get_Document_Node(m_docs[i].document, &m_docs[i].root);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not get XML document root.");
            goto cleanup_exit;
            }
        }

    /* Set our output document to the 'main' document to let people retrieve
        stuff from it.
    */
    m_output_document = m_docs[0].document;

    /* Resolve whether any potential entries should be on the list
    */
    Resolve_Potentials();

    /* Iterate through our list, adding each power as a thing
    */
    count = m_list.size();
    for (i = 0; i < count; i++) {
        info = &m_list[i];

        /* If this power is a dupe, sip it
        */
        if (info->is_duplicate)
            continue;

        /* If this entry has no name, we have to skip it
        */
        if ((info->name == NULL) || (info->name[0] == '\0')) {
            sprintf(buffer, ">>> %s entry has no name! Skipping\n", m_term);
            Log_Message(buffer);
            }

        /* Get the appropriate document root to use, based on the file we want
            to put this in
        */
        Get_Root(&root, info->filename, m_docs);

        /* Call our output function on this entry
        */
        status = Output_Entry(root, info);
        if (status == LWD_ERROR_NOT_IMPLEMENTED)
            continue;
        if (!x_Is_Success(status)) {
            sprintf(buffer, ">>> Error outputting %s %s\n", m_term, info->name);
            Log_Message(buffer);
            }

        /* If we didn't set our XML node, that's a problem
        */
        if (info->node == NULL) {
            sprintf(buffer, ">>> No XML node created for %s %s\n", m_term, info->name);
            Log_Message(buffer);
            }
        }

    Log_Message("done.\n", true);

    status = SUCCESS;

cleanup_exit:
    x_Status_Return(status);
}


template <class T> T_Status C_DDI_Output<T>::Process(void)
{
    T_Status        status;
    T_Int32U        i, j, count;
    T *             info;

    /* Make sure all output files are set up
    */
    count = m_list.size();
    for (i = 0; i < count; i++) {
        info = &m_list[i];

        /* If this power is a dupe, skip it
        */
        if (info->is_duplicate)
            continue;

        /* If a filename is specified, make sure it's in our list. If it's
            not, add it.
        */
        if (info->filename != NULL) {
            for (j = 0; j < MAX_XML_CONTAINERS; j++) {
                if (strcmp(m_docs[j].filename, info->filename) == 0)
                    break;
                if (m_docs[j].filename[0] == '\0')
                    break;
                }

            /* If we ran off the end of our array, we can't allocate more
                documents
            */
            if (j >= MAX_XML_CONTAINERS) {
                info->filename = NULL;
                Log_Message("*** Ran out of XML containers to use!\n");
                }

            /* Otherwise, if the current filename is NULL, put this there
                to add the filename to the list
            */
            else if (m_docs[j].filename[0] == '\0')
                strcpy(m_docs[j].filename, info->filename);
            }
        }

    /* Output all our details
    */
    status = Output();
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);

    x_Status_Return(status);
}


/* NOTE: We assume that all crawlers have been Processed before this function
    is called on any of them.
*/
template <class T> T_Status C_DDI_Output<T>::Post_Process(T_Filename temp_folder)
{
    T_Status        status = LWD_ERROR;
    T_Int32U        i, j, count;
    long            result;
    bool            is_partial;
    T_XML_Node      root, node;
    T *             info;
    vector <T_XML_Node> nodes;
    T_Glyph         buffer[500], temp[500];

    /* Iterate through our list, post-processing each power
    */
    is_partial = false;
    count = m_list.size();
    for (i = 0; i < count; i++) {
        info = &m_list[i];
        if (info->is_partial)
            is_partial = true;

        /* Skip anything without an XML node, since there's nothing we can do
        */
        if (info->node == NULL)
            continue;

        /* Get the appropriate document root to use, based on the file we want
            to put this in
        */
        Get_Root(&root, info->filename, m_docs);

        /* Call our output function on this entry
        */
        status = Post_Process_Entry(root, info);
        if (!x_Is_Success(status)) {
            sprintf(buffer, ">>> Error post-processing %s %s\n", m_term, info->name);
            Log_Message(buffer);
            }
        }

    /* Go through our XML documents and write them out to a temporary file -
        they still need to have extensions appended to them
    */
    for (i = 0; i < MAX_XML_CONTAINERS; i++) {
        if (m_docs[i].document == NULL)
            break;

        /* Check to make sure that all nodes in the document are valid - delete
            any invalid ones. This may cause problems loading stuff into HL (if
            the nodes are bootstrapped by others, for example), but if we don't,
            it will CERTAINLY cause problems loading stuff into HL.
        */
        nodes.empty();
        result = XML_Get_First_Child(m_docs[i].root, &node);
        while (result == 0) {
            result = XML_Validate_Node(node);
            if (x_Trap_Opt(result != 0)) {
                XML_Get_Name(node, temp);
                sprintf(buffer, ">>> %s node removed from document %s!\n", temp, m_docs[i].filename);
                Log_Message(buffer);
                }
            result = XML_Get_Next_Child(m_docs[i].root, &node);
            }
        for (j = 0; j < nodes.size(); i++)
            XML_Delete_Node(nodes[j]);

        /* If we have partial nodes, set a flag in the document to indicate
            that, so we know about it when we read things back in
        */
        if (is_partial)
            XML_Write_Boolean_Attribute(m_docs[i].root, "ispartial", true);

        /* Write out the document to a temporary folder
        */
        sprintf(buffer, "%s" DIR "%s", temp_folder, m_docs[i].filename);
        result = XML_Write_Document(m_docs[i].document, buffer, false, true);
        if (x_Trap_Opt(result != 0))
            Log_Message("Could not write XML document.");
        }

    x_Status_Return_Success();
}


template <class T> C_DDI_Single<T>::C_DDI_Single(T_Glyph_Ptr term, T_Glyph_Ptr first_filename,
                                                    T_Glyph_Ptr url, T_Glyph_Ptr filename,
                                                    C_Pool * pool) :
                    C_DDI_Output<T>(term, first_filename, pool)
{
    m_url = url;
    m_filename = filename;
}


template <class T> C_DDI_Single<T>::~C_DDI_Single()
{
}


template <class T> T_Status C_DDI_Single<T>::Download(T_Filename folder, T_WWW internet)
{
    T_Status            status;
    T_Glyph_Ptr         contents;
    T_Filename          filename;
    T_Glyph             message[1000];

    sprintf(message, "Downloading %ss... ", Get_Term());
    Log_Message(message, true);

    /* Download the file from the internet
    */
    status = WWW_Retrieve_URL(internet, m_url, &contents, NULL);
    if (x_Trap_Opt(!x_Is_Success(status))) {
        sprintf(message, "Couldn't retrieve individual %s download.\n", Get_Term());
        Log_Message(message);
        x_Status_Return(LWD_ERROR);
        }

    /* Write it out to a file we can read in later
    */
    sprintf(filename, "%s" DIR "%s", folder, m_filename);
    status = Text_Encode_To_File(filename, contents);
    if (x_Trap_Opt(!x_Is_Success(status))) {
        sprintf(message, "Couldn't write individual %s download.\n", Get_Term());
        Log_Message(message);
        x_Status_Return(LWD_ERROR);
        }

    Log_Message("done.\n", true);

    x_Status_Return_Success();
}


template <class T> T_Status C_DDI_Single<T>::Read(T_Filename folder)
{
    T_Status            status;
    T_Glyph_Ptr         contents;
    T_Filename          filename;
    T_Glyph             message[1000];

    sprintf(message, "Reading %s entries... ", Get_Term());
    Log_Message(message, true);

    /* Read in our previously-downloaded file
    */
    sprintf(filename, "%s" DIR "%s", folder, m_filename);
    contents = Text_Decode_From_File(filename);
    if (x_Trap_Opt(contents == NULL)) {
        sprintf(message, "Couldn't read individual %s download.\n", Get_Term());
        Log_Message(message);
        x_Status_Return(LWD_ERROR);
        }

    /* Call our derived class function to evaluate the contents
    */
    status = Read_File(contents);
    if (!x_Is_Success(status)) {
        sprintf(message, "Error reading individual %s download.\n", Get_Term());
        Log_Message(message);
        x_Status_Return(LWD_ERROR);
        }

    Log_Message("done.\n", true);
    x_Status_Return_Success();
}
