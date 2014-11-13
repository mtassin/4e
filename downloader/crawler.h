/*  FILE:   CRAWLER.H

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

    This file implements the C_DDI_Crawler template class. It has to be in a
    .h file, and not a .cpp file, and must be #included almost anywhere you
    use a class based on C_DDI_Crawler. Yes this sucks and C++ is a terrible
    language.
*/


#include "output.h"

#include <algorithm>

#include "ddi_dtds.h"


/* Define useful URLs
*/
#define INSIDER_URL         "http://www.wizards.com/dndinsider/compendium/"
#define INDEX_URL           INSIDER_URL "CompendiumSearch.asmx/ViewAll"
#define INDEX_FILENAME      "%s%s_index_%lu.xml"


/* Define miscellaneous constants
*/
#define MAX_RETRIES         3


template <class T> C_DDI_Crawler<T>::C_DDI_Crawler(T_Glyph_Ptr tab_name, T_Glyph_Ptr term,
                    T_Glyph_Ptr post_param, T_Int32U tab_index, T_Int32U last_cell_index,
                    bool is_filter_dupes, C_Pool * pool) :
                    C_DDI_Output<T>(term, tab_name, pool)
{
    m_tab_name = tab_name;
    m_tab_post_param = post_param;
    m_tab_index = tab_index;
    m_last_cell_index = last_cell_index;
    m_is_filter_dupes = is_filter_dupes;
    m_max_failed_index_pages = 0;
    m_failed_index_pages = 0;

    x_Trap_Opt(s_crawler != NULL);
    s_crawler = this;
}


template <class T> C_DDI_Crawler<T>::~C_DDI_Crawler()
{
    s_crawler = NULL;
}


template <class T> T_Int32U C_DDI_Crawler<T>::Get_URL_Count(void)
{
    /* Normal tabs only have one type of thing we care about
    */
    return(1);
}


template <class T> void     C_DDI_Crawler<T>::Get_URL(T_Int32U index, T_Glyph_Ptr url,
                                T_Glyph_Ptr names[], T_Glyph_Ptr values[], T_Int32U * value_count)
{
    if (x_Trap_Opt(index != 0)) {
        url[0] = '\0';
        return;
        }

    /* Normal tabs just use the default index URL and ask for the right tab
    */
    strcpy(url, INDEX_URL);
    names[0] = "Tab";
    values[0] = m_tab_post_param;
    *value_count = 1;
}


template <class T> T_Status C_DDI_Crawler<T>::Download_Index(T_Filename folder, T_WWW internet)
{
    T_Status                status;
    T_Int32U                i, count, value_count;
    T_Glyph_Ptr             ptr;
    T_Glyph_Ptr             names[] = { "", "", "", "", "", "", "", "", "", "", };
    T_Glyph_Ptr             values[] = { "", "", "", "", "", "", "", "", "", "",  };
    T_Glyph                 buffer[1000], url[1000];

    /* Get the number of retrievals we need to make for this type and go through
        them one by one
    */
    count = Get_URL_Count();
    for (i = 0; i < count; i++) {
        sprintf(buffer, "Retrieving %s index... ", Get_Term());
        if (count > 1) {
            ptr = buffer + strlen(buffer);
            sprintf(ptr, "(page %lu/%lu) ", i+1, count);
            }
        Log_Message(buffer, true);

        /* Get the filename of the index we'll be using
        */
        sprintf(buffer, INDEX_FILENAME, folder, m_tab_name, i);

        /* Get the URL and post parameters to use for the retrieval
        */
        Get_URL(i, url, names, values, &value_count);

        /* Sleep for a while so we don't end up DDOSing the server - this
            means we don't go as fast as we could, but it's better for the
            D&DI servers
        */
        Pause_Execution(PAGE_RETRIEVE_DELAY);

        /* do an HTTP POST to retrieve the data, then save it out
        */
        status = WWW_HTTP_Post(internet, url, value_count, names, values, &ptr);
        if (x_Trap_Opt(!x_Is_Success(status))) {
            Log_Message("Couldn't retrieve file.\n");
            x_Status_Return(status);
            }
        status = Text_Encode_To_File(buffer, ptr);
        if (x_Trap_Opt(!x_Is_Success(status))) {
            Log_Message("Couldn't write index file.");
            x_Status_Return(status);
            }
        Log_Message("done.\n", true);
        }

    x_Status_Return_Success();
}


template <class T> T_Status C_DDI_Crawler<T>::Read_Index(T_Filename folder)
{
    T_Status                status;
    T_Int32U                i, count;
    T_Int32S                result;
    T_Glyph_Ptr             contents;
    T_XML_Document          document = NULL;
    T_XML_Node              root;
    T_Glyph                 buffer[1000], message[1000];

    sprintf(buffer, "Processing %s index... ", Get_Term());
    Log_Message(buffer, true);

    /* Get the number of retrievals we need to make for this type and go through
        them one by one
    */
    count = Get_URL_Count();
    for (i = 0; i < count; i++) {

        /* Read in the contents of the saved index file.
        */
        sprintf(buffer, INDEX_FILENAME, folder, m_tab_name, i);
        contents = Text_Decode_From_File(buffer);
        if (contents == NULL) {
            sprintf(message, "File '%s' not found. %s import could not continue.\n", buffer, Get_Term());
            Log_Message(message, true);
            x_Status_Return(LWD_ERROR);
            }

        /* Extract the data into an XML document - we're allowed to fail a
            certain number of times before we give up, due to e.g. the
            Compendium not allowing the download of wizard powers.
        */
        result = XML_Extract_Document(&document, &l_ddiindex, contents);
        if ((result != 0) && (m_failed_index_pages < m_max_failed_index_pages)) {
            m_failed_index_pages++;
            Log_Message("!", true);
            goto next_index;
            }

        /* Otherwise, failures are bad and we should just give up
        */
        if (x_Trap_Opt(result != 0)) {
            sprintf(message, "Could not parse XML document '%s'. %s import could not continue.\n", buffer, Get_Term());
            Log_Message(message, true);
            x_Status_Return(LWD_ERROR);
            }
        result = XML_Get_Document_Node(document, &root);
        if (x_Trap_Opt(result != 0)) {
            sprintf(message, "Could not retrieve '%s' document root. %s import could not continue.\n", buffer, Get_Term());
            Log_Message(message, true);
            x_Status_Return(LWD_ERROR);
            }

        /* Parse the index as XML.
        */
        status = Parse_DDI_Index(root, i);
        if (x_Trap_Opt(!x_Is_Success(status)))
            x_Status_Return(status);

        /* Clean up anything we allocated that's no longer needed
        */
next_index:
        if (contents != NULL) {
            delete [] contents;
            contents = NULL;
            }
        if (document != NULL) {
            XML_Destroy_Document(document);
            document = NULL;
            }
        }

    Log_Message("done.\n", true);

    x_Status_Return_Success();
}


static T_Glyph_CPtr Safe_Name(T_Glyph_CPtr name, T_Glyph_Ptr buffer)
{
    /* Most names are fine, but some are iffy - they have a fake apostrophe
        character in them instead of a real apostrophe. If we find such, we
        need to massage the name a bit.
    */
    if (strstr(name, "’") == NULL)
        return(name);

    /* Copy the name and strip out any bad characters. This is a bit of a busy
        function, so it's probably a bad idea to look for every possible bad
        character here - just find the ones that make themselves a nuisance.
    */
    strcpy(buffer, name);
    Strip_Bad_Characters(buffer);
    return(buffer);
}


static T_Glyph_CPtr Get_Node_Name(T_XML_Node node, T_Glyph_Ptr buffer, bool is_class)
{
    T_Int32U        length;
    T_Glyph_CPtr    name, start, end;

    /* Remove any 'funky' characters that are necessary to do a good comparison
    */
    name = XML_Get_PCDATA_Pointer(node);
    name = Safe_Name(name, buffer);

    /* If our class name has parentheses in it like "Cleric (Templar)", change
        it to just the name in the parens to keep it consistent with how we
        used to do stuff, so it sorts properly
    */
    if (!is_class)
        return(name);
    start = strchr(name, '(');
    end = strchr(name, ')');
    if ((start != NULL) && (end != NULL)) {
        start++;
        length = end - start;
        strncpy(buffer, start, length);
        buffer[length] = '\0';
        name = buffer;
        }

    return(name);
}


static bool Sort_Nodes(const T_XML_Node & node1, const T_XML_Node & node2)
{
    T_Int32U        bonus1, bonus2;
    T_Int32S        result;
    T_XML_Node      child;
    T_Glyph_CPtr    name1, name2;
    bool            is_hybrid1, is_hybrid2, is_class;
    T_Glyph         buffer1[500], buffer2[500], nodename[500];

    XML_Get_Name(node1, nodename);
    is_class = stricmp("Class", nodename) == 0;

    /* First, get the names so we can compare them
    */
    result = XML_Get_First_Named_Child(node1, "Name", &child);
    if (x_Trap_Opt(result != 0))
        return(false);
    name1 = Get_Node_Name(child, buffer1, is_class);

    result = XML_Get_First_Named_Child(node2, "Name", &child);
    if (x_Trap_Opt(result != 0))
        return(false);
    name2 = Get_Node_Name(child, buffer2, is_class);

    /* If we're comparing "Class" nodes, we need to sort Hybrid classes last,
        to make sure all the 'real' classes are evaluated first
    */
    if (is_class) {
        is_hybrid1 = (strncmp(name1, "Hybrid ", 7) == 0);
        is_hybrid2 = (strncmp(name2, "Hybrid ", 7) == 0);
        if (!is_hybrid1 && is_hybrid2)
            return(true);
        if (is_hybrid1 && !is_hybrid2)
            return(false);
        }

    /* Now compare the names
    */
    if (stricmp(name1, name2) < 0)
        return(true);
    if (stricmp(name1, name2) > 0)
        return(false);

    /* Items have several special ways to compare them, to make sure they sort
        properly.
    */
    if (stricmp("Item", nodename) != 0)
        return(false);

    /* Next, compare the EnhancementValue nodes if we have them - this ensures
        that everything sorts by name, then by enhancement bonus
    */
    result = XML_Get_First_Named_Child(node1, "EnhancementValue", &child);
    if (result != 0)
        return(false);
    bonus1 = atoi(XML_Get_PCDATA_Pointer(child));

    result = XML_Get_First_Named_Child(node2, "EnhancementValue", &child);
    if (result != 0)
        return(false);
    bonus2 = atoi(XML_Get_PCDATA_Pointer(child));

    if (bonus1 < bonus2)
        return(true);
    if (bonus1 > bonus2)
        return(false);

    /* Finally, compare the Level nodes if we have them - this ensures
        that everything sorts by name, then by enhancement bonus, then level
    */
    result = XML_Get_First_Named_Child(node1, "Level", &child);
    if (result != 0)
        return(false);
    bonus1 = atoi(XML_Get_PCDATA_Pointer(child));

    result = XML_Get_First_Named_Child(node2, "Level", &child);
    if (result != 0)
        return(false);
    bonus2 = atoi(XML_Get_PCDATA_Pointer(child));

    return (bonus1 < bonus2);
}


template <class T> T_Status C_DDI_Crawler<T>::Parse_DDI_Index(T_XML_Node root, T_Int32U subtype)
{
    T_Int32S            result;
    T_XML_Node          toplevel, node;
    vector<T_XML_Node>  list;

    /* Get the top-level Results node
    */
    result = XML_Get_First_Named_Child(root, "Results", &toplevel);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("No <Results> tag found in document.\n");
        x_Status_Return_Success();
        }

    /* Now iterate through the child nodes, retrieving each one and adding them
        to a list
    */
    result = XML_Get_First_Named_Child(toplevel, m_tab_post_param, &node);
    while (result == 0) {
        list.push_back(node);
        result = XML_Get_Next_Named_Child(toplevel, &node);
        }

    /* Sort our nodes into alphabetical order by name, then extract the data
        from them all.
    */
    sort(list.begin(), list.end(), Sort_Nodes);
    for (auto it = list.begin(); it != list.end(); ++it)
        Extract_Entry_From_Row(*it, subtype);

    /* Sort the list of nodes again in case any names were fixed up during
        processing
    */
    sort(list.begin(), list.end(), Sort_Nodes);

    x_Status_Return_Success();
}


template <class T> T_Status C_DDI_Crawler<T>::Get_Required_Child_PCDATA(T_Glyph_Ptr * dest,
                                                T_XML_Node node, T_Glyph_Ptr child_name)
{
    T_Int32S        result;
    T_XML_Node      child;
    T_Glyph         buffer[500];

    result = XML_Get_First_Named_Child(node, child_name, &child);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "No <%s> tag found for cell.\n", child_name);
        Log_Message(buffer);
        x_Status_Return(LWD_ERROR);
        }
    *dest = Get_Pool()->Acquire(XML_Get_PCDATA_Pointer(child));
    Strip_Bad_Characters(*dest);
    x_Status_Return_Success();
}


template <class T> T_Status C_DDI_Crawler<T>::Extract_Entry_From_Row(T_XML_Node node,
                                                                        T_Int32U subtype)
{
    T_Status        status;
    T_Int32U        i, count;
    T_Glyph_Ptr     id;
    T               info;

    memset(&info, 0, sizeof(info));

    /* Generate the URL
    */
    status = Get_Required_Child_PCDATA(&id, node, "ID");
    if (!x_Is_Success(status))
        x_Status_Return_Success();
    sprintf(info.url, "%s.aspx?id=%s", Get_Term(), id);

    /* Get the name and source of the cell - if we don't have either of these
        this isn't a reliable cell to use, so get out without adding it to the
        list
    */
    status = Get_Required_Child_PCDATA(&info.name, node, "Name");
    if (!x_Is_Success(status))
        x_Status_Return_Success();
    status = Get_Required_Child_PCDATA(&info.source, node, "SourceBook");
    if (!x_Is_Success(status))
        x_Status_Return_Success();

    /* Check to see if our name is in the "skip" list - we put names in here
        when they're actively harmful or just broken, such as causing a server
        error when we try to retrieve them.
    */
    if (Mapping_Find_Tuple("skip", info.name) != NULL)
        x_Status_Return_Success();

    /* Finally, see if 'teaser' info is available for this thing
    */
//Teaser info used to be available for a lot of things, but not any more -
//maybe they'll put it back some day
#if 0
    status = Get_Required_Child_PCDATA(&teaser, node, "Teaser");
    if (!x_Is_Success(status))
        x_Status_Return_Success();
    info.is_teaser = (atoi(teaser) != 0);
#endif
    info.is_teaser = false;

    /* Finally, let the client do what it likes with the rest of the info
    */
    status = Parse_Index_Cell(node, &info, subtype);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return_Success();

    /* If this component filters duplicate names, iterate through our list, and
        see if there's an entry with the current name here already - if there
        is, mark it as a duplicate.
    */
    if (m_is_filter_dupes) {
        count = Get_List()->size();
        for (i = 0; i < count; i++) {
            if (strcmp(info.name, Get_List_Item(i)->name) == 0) {
                info.is_duplicate = true;
                break;
                }
            }
        }

    /* Add the completed item to our list
    */
    Get_List()->push_back(info);
    x_Status_Return_Success();
}


template <class T> T_Status C_DDI_Crawler<T>::Parse_Index_Cell(T_XML_Node node, T * info,
                                                                T_Int32U subtype)
{
    /* The derived class can override us if it wants to do anything with the
        index cells
    */
    x_Status_Return_Success();
}


static void Get_Page_URL(T_Glyph_Ptr buffer, T_Filename filename, T_Filename folder,
                    T_Glyph_Ptr url, T_Glyph_Ptr term)
{
    T_Glyph_Ptr     temp, ptr;

    strcpy(buffer, INSIDER_URL);
    strcat(buffer, url);
    temp = url;
    while (!x_Is_Digit(*temp) && (temp != '\0'))
        temp++;
    if (x_Trap_Opt(*temp == '\0')) {
        buffer[0] = '\0';
        return;
        }
    strcpy(filename, folder);
    ptr = filename + strlen(filename);
    strcpy(ptr, term);
    ptr += strlen(ptr);
    while (x_Is_Digit(*temp) && (temp != '\0'))
        *ptr++ = *temp++;
    strcpy(ptr, ".html");
}


template <class T> T_Status C_DDI_Crawler<T>::Download_Content(T_Filename folder, T_WWW internet)
{
    T_Int32U        i, count, retries;
    T *             info;
    T_Filename      filename;
    T_Glyph         url[500], buffer[500];
    T_Failed        failed;

    sprintf(buffer, "Downloading approx. %lu %s entries...\n", Get_List()->size(), Get_Term());
    Log_Message(buffer, true);

    count = Get_List()->size();
    for (i = 0; i < count; i++) {
        info = Get_List_Item(i);

        /* If this thing has no URL, it's a special thing that was added by
            something funky (like a power being added via a race), so skip it -
            we don't need to download or read in anything for it.
        */
        if (info->url[0] == '\0')
            continue;

        /* If we're downloading stuff, and we don't have a password, and we
            don't have teaser info for this thing, it's partial
        */
        if (!Is_Password() && !info->is_teaser)
            info->is_partial = true;

        /* If this is a partial entry, we shouldn't bother downloading the
            page, since it'll just redirect to the login page
        */
        if (info->is_partial)
            continue;

        /* Retrieve the URL and filename for this page
        */
        Get_Page_URL(url, filename, folder, info->url, Get_Term());
        if (x_Trap_Opt(url[0] == '\0')) {
            info->is_partial = true;
            continue;
            }

        /* Try to download the page several times, in case the first time fails
        */
        for (retries = 0; retries < MAX_RETRIES; retries++) {
            if (retries > 0)
                Log_Message("Retry...\n");

            /* If the download succeeded, stop trying - otherwise we sleep a
                few seconds to give the server time to stop hiccuping, and try
                again. If the page is marked as "partial data", just skip it,
                because there was some serious problem that indicates it can't
                be downloaded at all.
            */
            if (Download_Page(internet, info, url, filename))
                break;
            if (info->is_partial)
                break;
            Pause_Execution(10000); // 10s
            }

        /* If we failed because of too many retries, add this page to a list to
            try and grab later, once we've finished
        */
        if (retries >= MAX_RETRIES) {
            failed.info = info;
            strcpy(failed.url, url);
            strcpy(failed.filename, filename);
            l_failed_downloads.push_back(failed);
            }
        }

    Log_Message("done.\n", true);

    x_Status_Return_Success();
}


template <class T> T_Status C_DDI_Crawler<T>::Read_Content(T_Filename folder)
{
    T_Status        status;
    T_Int32U        i, count;
    T *             info;
    T_Glyph_Ptr     contents, detail, checkpoint;
    T_Filename      filename;
    vector<T>       extras;
    T_Glyph         url[500], buffer[500];

    sprintf(buffer, "Reading %s entries... ", Get_Term());
    Log_Message(buffer, true);

    count = Get_List()->size();
    for (i = 0; i < count; i++) {
        info = Get_List_Item(i);

        /* Skip things without URLs or partial entries
        */
        if ((info->url[0] == '\0') || info->is_partial)
            continue;

        /* Retrieve the URL and filename for this thing
        */
        Get_Page_URL(url, filename, folder, info->url, Get_Term());
        if (x_Trap_Opt(url[0] == '\0')) {
            info->is_partial = true;
            continue;
            }

        /* Open the file
        */
        contents = Text_Decode_From_File(filename);
        if (contents == NULL) {
            info->is_partial = true;
            continue;
            }

        /* Search for the 'detail' div - the data we want is in a block just after it
        */
        detail = strstr(contents, "<div id=\"detail\">");
        if (x_Trap_Opt(detail == NULL)) {
            sprintf(buffer, "Server error for %s %s, skipping.\n", Get_Term(), info->name);
            Log_Message(buffer);
            info->is_partial = true;
            goto next_power;
            }

        /* Search for the closing div tag - we're not interested in anything past
            here.
        */
        checkpoint = strstr(detail, "</div>");
        if (x_Trap_Opt(detail == NULL)) {
            Log_Message("Couldn't find closing detail div.\n");
            info->is_partial = true;
            goto next_power;
            }

        /* We only want one block from the middle of the file, so treating
            it as XML data is a bit of a waste of time (since the HTML is
            often invalid XML). Just parse it as text.
        */
        extras.clear();
        status = Parse_Entry_Details(contents, info, detail, checkpoint, &extras);
        if (x_Trap_Opt(!x_Is_Success(status))) {
            info->is_partial = true;
            goto next_power;
            }

        /* If any extra info entries need to be added, do so and adjust our
            position and count - we assume they're already processed here, but
            we need to make sure they're on the list to be output properly
            later
        */
        for (auto iter = extras.begin(); iter != extras.end(); iter++) {
            Get_List()->insert(Get_List()->begin() + i + 1, *iter);
            i++;
            count++;
            }

next_power:
        if (contents != NULL)
            delete [] contents;
        }

    Log_Message("done.\n", true);

    x_Status_Return_Success();
}
