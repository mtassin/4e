/*  FILE:   DDICRAWLER.CPP

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

    Core file for crawling the D&D insider web site and downloading all the
    information there.
*/


#include "private.h"
#include "dtds.h"

#include <fstream>


/*  Include our template class definition - see the comment at the top of the
    crawler.h file for details. Summary: C++ templates are terrible.
*/
#include "crawler.h"


/* Define useful URLs
*/
#define START_URL           "http://www.wizards.com/default.asp?x=dnd/insider"
#define LOGIN_URL           "http://www.wizards.com/global/dnd_login.asp"


/* Vector that holds all our T_Mappings
*/
vector<T_Mapping *> l_mappings;


/* Vector that holds a list of URLs that weren't retrieved properly
*/
vector<T_Failed>   l_failed_downloads;


/* Typedef out horrible types to something a lot nicer
*/
typedef vector<C_DDI_Common *>::iterator ddi_iter;
typedef vector<C_DDI_Single_Common *>::iterator single_iter;
typedef vector<T_Mapping *>::iterator map_iter;
typedef vector<T_Tuple>::iterator tuple_iter;

typedef vector<T_XML_Node> T_XML_Vector;
typedef T_XML_Vector::iterator T_XML_Iter;


/* Define ways the program works
*/
enum E_Query_Mode {
    e_mode_no_password,
    e_mode_password,
    e_mode_process,
    e_mode_append,
    e_mode_delete,
    e_mode_restore,
    e_mode_exit,
};


/* Define filenames for important files
*/
#define LANGUAGE_FILENAME   "ddi_languages.dat"
#define WEPPROP_FILENAME    "ddi_weapon_properties.aug"
#define SOURCE_FILENAME     "ddi_sources.aug"
#define POWERS_FILENAME     "ddi_powers.dat"

#define BACKUP_EXTENSION    ".saved"


static bool                 l_is_password = false;
static T_XML_Node           l_language_root = NULL;
static T_XML_Node           l_wepprop_root = NULL;
static T_XML_Node           l_source_root = NULL;


T_XML_Node      Get_Language_Root(void)
{
    x_Trap_Opt(l_language_root == NULL);
    return(l_language_root);
}


T_XML_Node      Get_WepProp_Root(void)
{
    x_Trap_Opt(l_wepprop_root == NULL);
    return(l_wepprop_root);
}


T_XML_Node      Get_Source_Root(void)
{
    x_Trap_Opt(l_source_root == NULL);
    return(l_source_root);
}


bool            Is_Password(void)
{
    return(l_is_password);
}


static T_Mapping *  Get_Mapping(T_Glyph_Ptr mapping)
{
    T_Unique        id;

    if (x_Trap_Opt(!UniqueId_Is_Valid(mapping)))
        return(NULL);
    id = UniqueId_From_Text(mapping);

    /* Find a mapping with the appropriate unique id
    */
    for (map_iter it = l_mappings.begin(); it != l_mappings.end(); ++it)
        if (id == (*it)->id)
            return(*it);
    return(NULL);
}


T_Tuple *       Mapping_Find_Tuple(T_Glyph_Ptr mapping, T_Glyph_Ptr search,
                                    bool is_partial_mapping, bool is_partial_search)
{
    T_Int32U        length;
    T_Mapping *     map = NULL;

    /* Find a mapping with the appropriate unique id
    */
    map = Get_Mapping(mapping);
    if (x_Trap_Opt(map == NULL))
        return(NULL);

    /* Find an appropriate entry within the mapping - if we're looking for a
        partial match, just make sure the start of the search string matches
        the mapping text
    */
    if (is_partial_search)
        length = strlen(search);
    for (tuple_iter it = map->list.begin(); it != map->list.end(); ++it) {
        if (is_partial_mapping || is_partial_search) {
            if (is_partial_mapping)
                length = strlen(it->a);
            if (strnicmp(search, it->a, length) == 0)
                return(&(*it));
            }
        else {
            if (stricmp(search, it->a) == 0)
                return(&(*it));
            }
        }

    return(NULL);
}


T_Glyph_Ptr     Mapping_Find(T_Glyph_Ptr mapping, T_Glyph_Ptr search,
                                bool is_partial_mapping, bool is_partial_search)
{
    T_Tuple *       tuple;

    if (x_Trap_Opt(search == NULL))
        return(NULL);

    tuple = Mapping_Find_Tuple(mapping, search, is_partial_mapping, is_partial_search);
    if (tuple != NULL)
        return((tuple->b == NULL) ? tuple->a : tuple->b);
    return(NULL);
}


void        Mapping_Add(T_Glyph_Ptr mapping, T_Glyph_Ptr a, T_Glyph_Ptr b, T_Glyph_Ptr c)
{
    T_Mapping *     map = NULL;
    T_Tuple         tuple;

    /* Find a mapping with the appropriate unique id
    */
    map = Get_Mapping(mapping);

    /* If we don't find one - create one
    */
    if (map == NULL) {
        if (x_Trap_Opt(!UniqueId_Is_Valid(mapping)))
            return;
        map = new T_Mapping;
        if (x_Trap_Opt(map == NULL))
            return;
        map->id = UniqueId_From_Text(mapping);
        l_mappings.push_back(map);
        }

    tuple.a = a;
    tuple.b = b;
    tuple.c = c;
    map->list.push_back(tuple);
}


static T_Status Login(T_WWW internet, T_Glyph_Ptr email, T_Glyph_Ptr password, bool is_report_success)
{
    static T_Glyph_Ptr      l_last_email = NULL;
    static T_Glyph_Ptr      l_last_password = NULL;
    T_Status                status;
    T_Glyph_Ptr             ptr, newline, invalid;

    /* Save our last email and password if given, or re-use them if we weren't
        passed in an email or password
    */
    if (email != NULL)
        l_last_email = email;
    else
        email = l_last_email;
    if (password != NULL)
        l_last_password = password;
    else
        password = l_last_password;

    /* If we don't have an email or password, we can't log in
    */
    if ((email[0] == '\0') || (password[0] == '\0'))
        x_Status_Return_Success();

    /* Set up our arrays of name/value parameters to use for the login POST
    */
    T_Glyph_Ptr             names[] = { "email", "password" };
    T_Glyph_Ptr             values[] = { email, password };

    /* Logging in is a simple POST of our username & password to the D&DI server
    */
    status = WWW_HTTP_Post(internet,LOGIN_URL,x_Array_Size(names),names,values,&ptr);
    if (x_Trap_Opt(!x_Is_Success(status))) {
        Log_Message("Couldn't make login HTTP post!\n", true);
        x_Status_Return(status);
        }

    /* If the string 'invalid' appears in the first line of our response,
        it's a failed login.
    */
    newline = strchr(ptr, '\n');
    if (newline == NULL) {
        Log_Message("\n**** Login error - could not log in to D&D insider.\n", true);
        x_Status_Return(LWD_ERROR);
        }
    invalid = strstr(ptr, "invalid");
    if ((invalid != NULL) && (invalid < newline)) {
        Log_Message("\n**** Login error - username / password not recognised.\n", true);
        x_Status_Return(LWD_ERROR);
        }

    Log_Message("Login succeeded.\n", is_report_success);
    l_is_password = true;
    x_Status_Return_Success();
}


void    Attempt_Login_Again(T_WWW internet)
{
    if (!Is_Password())
        return;

    Log_Message("Attempting Relogin...\n");
    Login(internet, NULL, NULL, false);
}


static T_Status Create_Document(T_XML_Document * document, T_XML_Node * root, T_Glyph_Ptr name,
                                T_XML_Element * element)
{
    T_Int32S                result;
    T_Glyph                 buffer[500];

    result = XML_Create_Document(document, element, false);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "Could not create %s XML document.", name);
        Log_Message(buffer);
        x_Status_Return(LWD_ERROR);
        }

    result = XML_Get_Document_Node(*document, root);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "Could not get %s XML document root.", name);
        Log_Message(buffer);
        x_Status_Return(LWD_ERROR);
        }

    x_Status_Return_Success();
}


#if 0
// Why bother...
static void Trim_Eval_Whitespace(T_XML_Node node)
{
    long            result;
    T_Glyph_Ptr     ptr;
    T_XML_Node      child;
    T_Glyph         buffer[MAX_XML_BUFFER_SIZE];

    /* If this is an eval script node, trim whitespace on it
    */
    XML_Get_Name(node, buffer);
    if (strcmp(buffer, "eval") == 0) {
        XML_Get_PCDATA(node, buffer);
        ptr = buffer + strlen(buffer);
        while (x_Is_Space(ptr[-1]) && x_Is_Space(ptr[-2]))
            *--ptr = '\0';
        XML_Set_PCDATA(node, buffer);
        }

    /* Call this function recursively on all children
    */
    result = XML_Get_First_Child(node, &child);
    while (result == 0) {
        Trim_Eval_Whitespace(child);
        result = XML_Get_Next_Child(node, &child);
        }
}
#endif


static void     Backup_Existing(T_Glyph_Ptr path)
{
    T_Glyph                 backup[MAX_FILE_NAME+1];

    /* Generate our backup filename and copy the file (fails silently if an
        existing version of the file isn't present, which is fine)
    */
    strcpy(backup, path);
    strcat(backup, BACKUP_EXTENSION);
    FileSys_Copy_File(backup, path, FALSE);
}


static T_Status Append_Extension(T_Glyph_Ptr filename, T_File_Attribute attributes,
                             T_Void_Ptr context)
{
    long            result;
    bool            is_partial;
    T_Glyph_Ptr     base_filename, output_folder = (T_Glyph_Ptr) context;
    T_XML_Node      root;
    T_XML_Document  document;
    T_Filename      output_filename;

    if ((attributes & e_filetype_directory) != 0)
        x_Status_Return_Success();

    /* Read the file in from the temporary folder as an XML document
    */
    result = XML_Read_Document(&document, &l_data, filename);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not read XML document.");
        x_Status_Return_Success();
        }

    /* Get the base filename to use for this file
    */
    base_filename = strrchr(filename, DIR[0]);
    if (x_Trap_Opt(base_filename == NULL))
        x_Status_Return(LWD_ERROR);
    base_filename++;
    if (x_Trap_Opt(*base_filename == '\0'))
        x_Status_Return(LWD_ERROR);

    /* Query and unset the artificial "partial" flag we added, so it
        doesn't confuse things by showing up in the real data file we're
        about to output.
    */
    result = XML_Get_Document_Node(document, &root);
    if (x_Trap_Opt(result != 0))
        x_Status_Return_Success();
    is_partial = XML_Read_Boolean_Attribute(root, "ispartial");
    XML_Write_Boolean_Attribute(root, "ispartial", false);

    /* Append any extensions to the document
     */
    Append_Extensions(document, output_folder, base_filename, is_partial);

    /* Back up any existing file and write out the new one
    */
    sprintf(output_filename, "%s" DIR "%s", output_folder, base_filename);
    Backup_Existing(output_filename);
    result = XML_Write_Document(document, output_filename, false, true);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not write XML document.");
        x_Status_Return_Success();
        }

    XML_Destroy_Document(document);
    x_Status_Return_Success();
}


static void Append_Extensions(T_Filename temp_folder, T_Filename output_folder)
{
    T_Status        status;
    T_Filename      input_wildcard;

    /* Go through our data files and post-process them
     */
    sprintf(input_wildcard, "%s" DIR "*.dat", temp_folder);
    status = FileSys_Enumerate_Matching_Files(input_wildcard, Append_Extension, output_folder);
    if (x_Trap_Opt(status == WARN_CORE_FILE_NOT_FOUND)) {
        printf("<No files to append to!>\n");
        return;
        }
    x_Trap_Opt(!x_Is_Success(status));
}


static void     Fixup_Wizard_Powers(T_Glyph_Ptr folder)
{
    T_Int32U            count;
    long                result;
    T_XML_Node          root_old, root_new, node, tag;
    T_XML_Document      doc_old = NULL, doc_new = NULL;
    bool                is_match;
    T_XML_Iter          iter;
    T_Glyph             buffer[500];
    T_Filename          file_old, file_new;
    T_XML_Vector        nodes;

    /* Open the previous version of the powers file - if we don't have one,
        there's nothing to restore.
    */
    sprintf(file_old, "%s" DIR "%s%s", folder, POWERS_FILENAME, BACKUP_EXTENSION);
    if (!FileSys_Does_File_Exist(file_old))
        goto cleanup_exit;
    result = XML_Read_Document(&doc_old, &l_data, file_old);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not read old powers XML document.\n", true);
        goto cleanup_exit;
        }
    result = XML_Get_Document_Node(doc_old, &root_old);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* Open the new powers file
    */
    sprintf(file_new, "%s" DIR "%s", folder, POWERS_FILENAME);
    if (!FileSys_Does_File_Exist(file_new))
        goto cleanup_exit;
    result = XML_Read_Document(&doc_new, &l_data, file_new);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not read new powers XML document.\n", true);
        goto cleanup_exit;
        }
    result = XML_Get_Document_Node(doc_new, &root_new);
    if (x_Trap_Opt(result != 0))
        goto cleanup_exit;

    /* Go through the old powers file, finding all powers (things from the
        "Power" compset) for the Wizard class (with a "PowerClass.clsWizard"
        tag). Add their nodes to a list.
    */
    Log_Message("Copying missing powers from old powers file...", true);
    result = XML_Get_First_Named_Child_With_Attr(root_old, "thing", "compset", "Power", &node);
    while (result == 0) {
        is_match = false;
        result = XML_Get_First_Named_Child_With_Attr(node, "tag", "group", "PowerClass", &tag);
        while (result == 0) {
            XML_Read_Text_Attribute(tag, "tag", buffer);
            if (strcmp("clsWizard", buffer) == 0) {
                is_match = true;
                break;
                }
            result = XML_Get_Next_Named_Child_With_Attr(node, &tag);
            }
        if (is_match)
            nodes.push_back(node);
        result = XML_Get_Next_Named_Child_With_Attr(root_old, &node);
        }

    /* For each node in our list, make sure a node with the same id doesn't
        exist in the new powers file, since we don't want to introduce
        duplicates.
    */
    count = 0;
    for (iter = nodes.begin(); iter != nodes.end(); iter++) {
        XML_Read_Text_Attribute(*iter, "id", buffer);
        result = XML_Get_First_Named_Child_With_Attr(root_new, "thing", "id", buffer, &node);
        if (result == 0)
            continue;

        /* If it doesn't, copy the node from the old file to the new file. This
            lets someone keep their wizard powers, even though the new data
            doesn't include them.
        */
        result = XML_Create_Child(root_new, "thing", &node);
        if (x_Trap_Opt(result != 0))
            continue;
        result = XML_Duplicate_Node(*iter, &node);
        x_Trap_Opt(result != 0);
        count++;
        }

    /* Write out the new contents of the new powers file.
    */
    sprintf(buffer, " done (%lu powers restored).\n", count);
    Log_Message(buffer, true);
    result = XML_Write_Document(doc_new, file_new, false, true);
    if (x_Trap_Opt(result != 0))
        Log_Message("Could not write XML document.", true);

    /* We're finished with the XML documents.
    */
cleanup_exit:
    if (doc_old != NULL)
        XML_Destroy_Document(doc_old);
    if (doc_new != NULL)
        XML_Destroy_Document(doc_new);
}


static T_Status Finish_Document(T_XML_Document document, T_Glyph_Ptr output_folder,
                                T_Glyph_Ptr filename)
{
    T_Int32S                result;
    T_Glyph                 buffer[MAX_FILE_NAME+1];

    sprintf(buffer, "%s" DIR "%s", output_folder, filename);
    Backup_Existing(buffer);

    /* Output the document into our output folder
    */
    result = XML_Write_Document(document, buffer, false, true);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "Could not write %s XML document.", filename);
        Log_Message(buffer);
        x_Status_Return(LWD_ERROR);
        }

    x_Status_Return_Success();
}


static void Get_Temporary_Folder(T_Filename folder)
{
    FileSys_Get_Temporary_Folder(folder);
    strcat(folder, "ddicrawler" DIR);
}


static void Retry_Failed_Downloads(T_WWW www)
{
    bool                        is_ok;
    T_Glyph                     choice;
    vector<T_Failed>::iterator  iter;
    T_Glyph                     buffer[5000];

    /* If we don't have any failures, there's nothing to do
    */
    if (l_failed_downloads.size() <= 0)
        return;

    /* Keep trying until the user tells us to stop
    */
    while (true) {

        Log_Message("Retrying failed downloads...\n", true);

        iter = l_failed_downloads.begin();
        while (iter != l_failed_downloads.end()) {

            /* Try the download again - if it succeeds this time, just erase the
                item from the list, since we managed to download it. Otherwise,
                go on to the next one - we'll ask the user if they want to come
                back and try again later.
            */
            is_ok = Download_Page(www, iter->info, iter->url, iter->filename);
            if (is_ok)
                iter = l_failed_downloads.erase(iter);
            else
                iter++;
            }

        /* If there's nothing left in the failed list, yay! We downloaded all
            the failed files.
        */
        if (l_failed_downloads.size() <= 0) {
            Log_Message("done.\n", true);
            return;
            }

        /* Otherwise, ask the user what to do next - if they want to continue,
            we'll go round the loop again. Otherwise, skip out.
        */
        sprintf(buffer, "\n%lu records were not retrieved from the D&D Compendium,\ndue to server problems. This issue usually resolves itself\nwithin a few hours - we recommend you wait at least 30 minutes,\nthen press 'r' to retry the download.\n\n(Do not close this program if you wish to retry - if you do,\nall downloading progress will be lost!)\n\nPress 'r' to retry the download, or 's' to skip. ", l_failed_downloads.size());
        Log_Message(buffer, true);
        choice = Get_Character();
        Log_Message("\n\n", true);
        if (choice == 's') {
            Log_Message("Download abandoned. Some data will not be present in Hero Lab.\n", true);
            return;
            }

        /* If we're retrying, we might have been logged out due to waiting too
            long, so make sure to log in again if necessary
        */
        Attempt_Login_Again(www);
        }
}


static T_Status Crawl_Data(bool use_cache, T_Glyph_Ptr email, T_Glyph_Ptr password,
                            T_Filename output_folder, bool is_clear)
{
    T_Status                status;
    T_WWW                   internet = NULL;
    bool                    is_exists;
    T_Glyph_Ptr             path_ptr;
    T_Filename              folder;
    T_XML_Document          doc_languages = NULL;
    T_XML_Document          doc_wepprops = NULL;
    T_XML_Document          doc_sources = NULL;
    C_Pool                  pool(10000000,10000000);
    C_DDI_Deities           deities(&pool);
    C_DDI_Skills            skills(&pool);
    C_DDI_Rituals           rituals(&pool);
    C_DDI_Items             items(&pool);
    C_DDI_Classes           classes(&pool);
    C_DDI_Races             races(&pool);
    C_DDI_Paragons          paragons(&pool);
    C_DDI_Epics             epics(&pool);
    C_DDI_Feats             feats(&pool);
    C_DDI_Powers            powers(&pool);
    C_DDI_Monsters          monsters(&pool);
    C_DDI_Backgrounds       backgrounds(&pool);
    vector<C_DDI_Common *>  list;
    vector<C_DDI_Single_Common *>   singles;

    /* Add all of our downloader classes to the list
    */
    list.push_back(&deities);
    list.push_back(&skills);
    list.push_back(&rituals);
    list.push_back(&items);
    list.push_back(&classes);
    list.push_back(&races);
    list.push_back(&paragons);
    list.push_back(&epics);
    list.push_back(&feats);
    list.push_back(&powers);
//Monsters aren't supported yet...
#ifdef NOTYET
    list.push_back(&monsters);
#endif
    list.push_back(&backgrounds);

    /* Find a temporary folder to use
    */
    Get_Temporary_Folder(folder);
    path_ptr = folder + strlen(folder);

    /* Search for the folder and create it if it doesn't exist
    */
    is_exists = FileSys_Does_Folder_Exist(folder);
    if (!is_exists) {
        status = FileSys_Create_Directory(folder);
        if (x_Trap_Opt(!x_Is_Success(status))) {
            Log_Message("Couldn't create temporary folder.");
            x_Status_Return(LWD_ERROR);
            }
        }

    /* If we're not using cached copies, log in to D&D insider (if we don't
        have a username and password, this just establishes the connection)
    */
    if (!use_cache) {
        Log_Message("Connecting to server...\n", true);

        /* Open a connection to the server first
        */
        status = WWW_HTTP_Open(&internet,LOGIN_URL,NULL,NULL,NULL);
        if (x_Trap_Opt(!x_Is_Success(status))) {
            Log_Message("Couldn't open connection to login server!\n", true);
            goto cleanup_exit;
            }

        status = Login(internet, email, password, true);
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }

    /* If we're not using the cache, we want to make sure we have fresh copies
        of everything, so delete the files in our temporary folder
    */
    if (!use_cache) {
        strcpy(path_ptr,"*.*");
        FileSys_Delete_Files(folder);
        *path_ptr = '\0';
        }

    /* Create XML documents for deities languages - the crawlers can add their
        own entries for these during processing
    */
    status = Create_Document(&doc_languages, &l_language_root, "language",
                                DTD_Get_Data());
    if (!x_Is_Success(status))
        goto cleanup_exit;
    status = Create_Document(&doc_wepprops, &l_wepprop_root, "weapon property",
                                DTD_Get_Augmentation());
    if (!x_Is_Success(status))
        goto cleanup_exit;
    status = Create_Document(&doc_sources, &l_source_root, "source",
                                DTD_Get_Augmentation());
    if (!x_Is_Success(status))
        goto cleanup_exit;

    /* First, download all the index pages if we need to
    */
    if (!use_cache) {
        for (ddi_iter it = list.begin(); it != list.end(); ++it) {
            status = (*it)->Download_Index(folder, internet);
            if (x_Trap_Opt(!x_Is_Success(status)))
                goto cleanup_exit;
            }
        for (single_iter it = singles.begin(); it != singles.end(); ++it) {
            status = (*it)->Download(folder, internet);
            if (x_Trap_Opt(!x_Is_Success(status)))
                goto cleanup_exit;
            }
        }

    /* Read them in
    */
    for (ddi_iter it = list.begin(); it != list.end(); ++it) {
        status = (*it)->Read_Index(folder);
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }
    for (single_iter it = singles.begin(); it != singles.end(); ++it) {
        status = (*it)->Read(folder);
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }

    /* Now download the rest of the content all at once
        NOTE: As of March 2011, there's no further content for download unless
            you have a password.
    */
    if (!use_cache && l_is_password) {
        for (ddi_iter it = list.begin(); it != list.end(); ++it) {
            status = (*it)->Download_Content(folder, internet);
            if (x_Trap_Opt(!x_Is_Success(status)))
                goto cleanup_exit;
            }

        /* It's possible that some downloads were screwed up due to the
            servers acting weirdly. If so, they were added to a failed list,
            so try them again
        */
        Retry_Failed_Downloads(internet);
        }

    /* Read it in and process it
    */
    for (ddi_iter it = list.begin(); it != list.end(); ++it) {
        status = (*it)->Read_Content(folder);
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }

    /* Process all our stuff
    */
    for (ddi_iter it = list.begin(); it != list.end(); ++it) {
        status = (*it)->Process();
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }
    for (single_iter it = singles.begin(); it != singles.end(); ++it) {
        status = (*it)->Process();
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }

    /* Post-process all our stuff - this requires it to have been output first,
        and puts finishing touches on everything
    */
    for (ddi_iter it = list.begin(); it != list.end(); ++it) {
        status = (*it)->Post_Process(folder);
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }
    for (single_iter it = singles.begin(); it != singles.end(); ++it) {
        status = (*it)->Post_Process(folder);
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        }

    /* Output our languages to the temporary folder, so we can append
        extensions to them later
    */
    status = Finish_Document(doc_languages, folder, LANGUAGE_FILENAME);
    if (!x_Is_Success(status))
        goto cleanup_exit;

    /* Now append any fixup extensions that are needed to compensate for the
        data being terrible
    */
    Append_Extensions(folder, output_folder);

    /* Load the old "powers" XML document and try and copy any wizard powers
        out of it
    */
//We don't need to do this any more, but keep the code around just in case...
//    Fixup_Wizard_Powers(output_folder);

    /* Sources and weapon properties don't need any extensions, so they can go
        directly into the output folder - plus they're augmentation files,
        which aren't messed with anyway
    */
    status = Finish_Document(doc_sources, output_folder, SOURCE_FILENAME);
    if (!x_Is_Success(status))
        goto cleanup_exit;
    status = Finish_Document(doc_wepprops, output_folder, WEPPROP_FILENAME);
    if (!x_Is_Success(status))
        goto cleanup_exit;

    /* wrapup everything
    */
cleanup_exit:
    l_language_root = NULL;
    if (doc_languages != NULL)
        XML_Destroy_Document(doc_languages);
    l_wepprop_root = NULL;
    if (doc_wepprops != NULL)
        XML_Destroy_Document(doc_wepprops);
    l_source_root = NULL;
    if (doc_sources != NULL)
        XML_Destroy_Document(doc_sources);
    if (internet != NULL)
        WWW_Close_Server(internet);

    /* Delete everything in our folder if required
    */
    if (is_clear) {
        strcpy(path_ptr,"*.*");
        FileSys_Delete_Files(folder);
        *path_ptr = '\0';
        }

    x_Status_Return(status);
}


static E_Query_Mode Query_Mode(int argc,char ** argv,bool * is_command_line)
{
    T_Glyph         choice;
    T_Glyph_Ptr     ptr;

    *is_command_line = false;

    printf("**********************************************************\n");
    printf("Welcome to the 4e data downloader. Please choose an option\n");
    printf("by pressing 1, 2, r, or d, or press any other key to exit.\n\n");

    printf("This downloader was updated on: August 7th 2012. Always make\n");
    printf("sure you're running the version of the downloader listed in\n");
    printf("the message that appears when you launch the 4e data files,\n");
    printf("or you may experience unexpected results.\n\n");

    printf("For more details on this program, please see the 4e user\n");
    printf("manual (Hero Lab -> Help menu -> 4th edition manual).\n");
    printf("**********************************************************\n\n");

    printf("1. Download data (no D&D insider account)\n");
    printf("2. Download data (with D&D insider account)\n");
    printf("r. Restore previous data (if present)\n");
    printf("d. Delete existing data\n");
    printf("x. Exit\n\n");

    /* If our last parameter was one character long, assume it's the option to
        use - otherwise, ask the user to choose
    */
    if (argc <= 1)
        choice = Get_Character();
    else {
        ptr = argv[argc - 1];
        choice = ptr[0];
        if (strlen(ptr) != 1)
            choice = Get_Character();
        else {
            printf("%s (from command-line parameter)\n\n", ptr);
            *is_command_line = true;
            }
        }

    switch (choice) {
        case '1' :      return(e_mode_no_password);
        case '2' :      return(e_mode_password);
        case 'r' :      return(e_mode_restore);
        case 'd' :      return(e_mode_delete);
        case '7' :      return(e_mode_process);
        case '9' :      return(e_mode_append);
        }
    return(e_mode_exit);
}


static void Get_Credentials(T_Glyph_Ptr username, T_Glyph_Ptr password)
{
    printf("IMPORTANT NOTE:\n");
    printf("The downloader does not save your login information\n");
    printf("anywhere - it is only used to log in to D&D insider,\n");
    printf("and then discarded.\n\n");

    printf("Enter your D&D insider username (email address) and press enter: ");
    scanf("%s", username);

    printf("\nEnter your D&D insider password and press enter: ");
    scanf("%s", password);
}


static void Delete_Generated_Files(T_Glyph_Ptr output_folder)
{
    T_Glyph         choice;
    T_Filename      wildcard;

    /* Make sure the user knows what they're doing.
    */
    printf("The following files will be deleted:\n");
    strcpy(wildcard, output_folder);
    strcat(wildcard, DIR "ddi_*.*");

    FileSys_List_Files(wildcard);
    printf("\nTo continue, press 'y', or 'n' to cancel.\n\n");
    choice = Get_Character();
    if (tolower(choice) != 'y') {
        printf("Cancelled.\n\n");
        return;
        }

    /* Delete all files in the folder with "ddi_" at the start of the name
    */
    FileSys_Delete_Files(wildcard);
    Log_Message("All output files deleted.\n", true);
}

static T_Status Restore_Old_File(T_Glyph_Ptr filename, T_File_Attribute attributes,
                             T_Void_Ptr context)
{
    T_Status            status;
    T_Glyph_Ptr         base_filename, output_folder = (T_Glyph_Ptr) context;
    T_Filename          file_old, file_new, file_temp;
    T_Glyph             buffer[1000];

    if ((attributes & e_filetype_directory) != 0)
        x_Status_Return_Success();

    /* Get the base filename to use for this file
    */
    base_filename = strrchr(filename, DIR[0]);
    if (x_Trap_Opt(base_filename == NULL))
        x_Status_Return(LWD_ERROR);
    base_filename++;
    if (x_Trap_Opt(*base_filename == '\0'))
        x_Status_Return(LWD_ERROR);

    /* Get the temporary filename we'll use for stuff
    */
    sprintf(file_temp, "%s" DIR "temporary.ignore", output_folder);

    /* Get filenames for our source, and target files. The source is the
        one we just found; the target is that with ".saved" chopped off the
        end.
    */
    sprintf(file_old, "%s" DIR "%s", output_folder, base_filename);
    strcpy(file_new, file_old);
    file_new[strlen(file_new) - strlen(BACKUP_EXTENSION)] = '\0';
    sprintf(buffer, "Restoring %s...\n", base_filename);
    Log_Message(buffer, true);

    /* We want to swap the two files, so that the new is replaced by the
        old.
    */
    FileSys_Delete_File(file_temp, FALSE);
    if (FileSys_Does_File_Exist(file_new)) {
        status = FileSys_Copy_File(file_temp, file_new, FALSE);
        if (!x_Is_Success(status)) {
            sprintf(buffer, "Couldn't make a backup of %s!\n", file_new);
            Log_Message(buffer, true);
            }
        }
    if (FileSys_Does_File_Exist(file_old)) {
        status = FileSys_Copy_File(file_new, file_old, FALSE);
        if (!x_Is_Success(status)) {
            sprintf(buffer, "Couldn't restore old file %s!\n", file_old);
            Log_Message(buffer, true);
            }
        }
    if (FileSys_Does_File_Exist(file_temp)) {
        status = FileSys_Copy_File(file_old, file_temp, FALSE);
        if (!x_Is_Success(status)) {
            sprintf(buffer, "Couldn't restore backup of new file %s!\n", file_new);
            Log_Message(buffer, true);
            }
        }
    FileSys_Delete_File(file_temp, FALSE);
    x_Status_Return_Success();
}


static void Restore_Old_Files(T_Glyph_Ptr output_folder)
{
    T_Status            status;
    T_Filename          wildcard;

    /* Go through our data files and post-process them
    */
    sprintf(wildcard, "%s" DIR "ddi_*" BACKUP_EXTENSION, output_folder);
    status = FileSys_Enumerate_Matching_Files(wildcard, Restore_Old_File, output_folder);
    if (x_Trap_Opt(status == WARN_CORE_FILE_NOT_FOUND)) {
        Log_Message("No old files found to restore!\n", true);
        return;
        }
    x_Trap_Opt(!x_Is_Success(status));
}


static T_XML_Document   Load_Mappings(T_Glyph_Ptr folder)
{
    long                result;
    T_XML_Document      document;
    T_XML_Node          root, map, tuple;
    T_Glyph_Ptr         ptr;
    T_Mapping *         mapping;
    T_Tuple             t;
    T_Filename          filename;

    /* Load the mappings XML file
    */
    sprintf(filename, "%s" DIR "ddidownloader" DIR "mapping.xml", folder);
    result = XML_Read_Document(&document, &l_mapping, filename);
    if (x_Trap_Opt(result != 0))
        return(NULL);
    result = XML_Get_Document_Node(document, &root);
    if (x_Trap_Opt(result != 0))
        return(NULL);

    /* Go through all our mappings
    */
    result = XML_Get_First_Named_Child(root, "map", &map);
    while (result == 0) {

        /* Get the unique id for this mapping
        */
        ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(map, "name");
        if (!UniqueId_Is_Valid(ptr)) {
            Log_Message("**** Invalid unique id found for mapping!\n", true);
            goto next_map;
            }

        /* Allocate a new mapping structure
        */
        mapping = new T_Mapping;
        if (x_Trap_Opt(mapping == NULL))
            goto next_map;
        mapping->id = UniqueId_From_Text(ptr);

        /* Add each tuple to the list
        */
        result = XML_Get_First_Named_Child(map, "tuple", &tuple);
        while (result == 0) {
            t.a = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(tuple, "a");
            if (x_Trap_Opt((t.a == NULL) || (t.a[0] == '\0'))) {
                Log_Message("**** Empty tuple found!\n", true);
                goto next_tuple;
                }
            t.b = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(tuple, "b");
            if ((t.b != NULL) && (t.b[0] == '\0'))
                t.b = NULL;
            t.c = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(tuple, "c");
            if ((t.c != NULL) && (t.c[0] == '\0'))
                t.c = NULL;
            mapping->list.push_back(t);
next_tuple:
            result = XML_Get_Next_Named_Child(map, &tuple);
            }

        /* Add our new mapping structure to the main list
        */
        l_mappings.push_back(mapping);

next_map:
        result = XML_Get_Next_Named_Child(root, &map);
        }

    /* Return the completed XML document, since it's already storing all the
        strings we're using - this way we don't have to allocate new space
        for them, we just throw the document away before we exit
    */
    return(document);
}


int     main(int argc,char ** argv)
{
    T_Status                status;
    T_Int32S                result = 0;
    bool                    use_cache, is_command_line, is_clear = true;
    E_Query_Mode            mode;
    T_Filename              output_folder, folder, logfile;
    T_XML_Document          mappings = NULL;
    T_Glyph                 xml_errors[1000], email[1000], password[1000];

    /* On windows, get the current directory to use as the output folder. On the
        mac, the current directory could well be the user's home directory, so
        get the directory we were launched from
    */
#ifdef _WIN32
    FileSys_Get_Current_Directory(output_folder);
#elif defined(_OSX)
    strcpy(output_folder, argv[0]);
    T_Glyph_Ptr ptr;
    ptr = strrchr(output_folder, DIR[0]);
    if (x_Trap_Opt(ptr == NULL)) {
        printf("Error getting path of executable.");
        exit(1);
        }
    *++ptr = '\0';
#else
#error Unknown platform!
#endif

    /* Set up our static output stream for logging
    */
    sprintf(logfile, "%s" DIR "output.txt", output_folder);
    ofstream    output(logfile,ios::out);
    if (output.good())
        Initialize_Helper(&output);
    else {
        Log_Message("Error opening output log file.", true);
        goto cleanup_exit;
        }

    /* Set up an error buffer for our XML library
    */
    XML_Set_Error_Buffer(xml_errors);

    /* Initialize our unique id mechanisms
    */
    UniqueId_Initialize();

    /* Verify that we have write privileges in our output folder - if not, then
        downloading stuff would be a bit of a waste of time (this can happen if
        the downloader needs to be run with administrator rights).
    */
    if (!FileSys_Verify_Write_Privileges(output_folder)) {
        status = LWD_ERROR;
        goto cleanup_exit;
        }

    /* Load up our mappings from the mapping XML file
    */
    mappings = Load_Mappings(output_folder);
    if (x_Trap_Opt(mappings == NULL)) {
        Log_Message("Could not open mapping file.\n", true);
        goto cleanup_exit;
        }

    /* Set up some sensible defaults
    */
    email[0] = '\0';
    password[0] = '\0';
    use_cache = false;

    /* If our first parameter is the secret "don't delete stuff" code, unset
        the flag that causes everything to be deleted
    */
    if ((argc > 1) && (stricmp(argv[1], "-nodelete") == 0))
        is_clear = false;

    /* Ask the user how the program is going to run - if we're told to exit,
        just get out now
    */
    mode = Query_Mode(argc, argv, &is_command_line);
    if (mode == e_mode_exit)
        goto cleanup_exit;

    /* If we want to download with no password, do nothing - this is the
        normal way we'll work
    */
    if (mode == e_mode_no_password)
        /* do nothing */;

    /* If we just want to reprocess stuff without downloading, set our 'use cache'
        flag, and don't clear the data after processing it
    */
    else if (mode == e_mode_process) {
        use_cache = true;
        is_clear = false;
        }

    /* If we just want to append extensions, do that
    */
    else if (mode == e_mode_append) {
        Get_Temporary_Folder(folder);
        Append_Extensions(folder, output_folder);
        status = SUCCESS;
        goto cleanup_exit;
        }

    /* If we want to download with no password, get the username and password
    */
    else if (mode == e_mode_password) {
        Get_Credentials(email, password);
        if ((email[0] == '\0') || (password[0] == '\0')) {
            Log_Message("Empty username or password - cancelling download.\n", true);
            goto cleanup_exit;
            }
        }

    /* Restore mode
    */
    else if (mode == e_mode_restore) {
        Restore_Old_Files(output_folder);
        Log_Message("\nThe old versions of your files were restored.\nYou can use this command again to switch back to the \"new\" versions.\n", true);
        goto cleanup_exit;
        }

    /* If we just want to delete stuff, delete it
    */
    else if (mode == e_mode_delete) {
        Delete_Generated_Files(output_folder);
        goto cleanup_exit;
        }

    /* Otherwise, something is wrong
    */
    else {
        x_Trap_Opt(1);
        Log_Message("Unrecognised option - exiting.\n", true);
        goto cleanup_exit;
        }

    /* Crawl everything
    */
    status = Crawl_Data(use_cache, email, password, output_folder, is_clear);
    if (x_Trap_Opt(!x_Is_Success(status)))
        goto cleanup_exit;

    if (status == SUCCESS)
        Log_Message("\n\nImport complete. Reload the game system to see the new content.\n\n", true);
    else
        Log_Message("\n\nThere were one or more problems with the import. Please see the manual for more details.\n\n", true);

    /* wrapup everything
    */
cleanup_exit:
    for (map_iter it = l_mappings.begin(); it != l_mappings.end(); ++it)
        delete *it;
    if (mappings != NULL)
        XML_Destroy_Document(mappings);

    Shutdown_Helper();

    /* The window doesn't auto-close on OS X, so we don't need this
    */
#ifndef _OSX
    if (!is_command_line) {
        printf("Press any key to quit.\n\n");
        Get_Character();
        }
#endif

    if (!x_Is_Success(status))
        result = -1;
    return(result);
}
