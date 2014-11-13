/*  FILE:   HELPER.CPP

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

    Useful helper functions for the D&DI Crawler.
*/


#include "private.h"
#include "dtds.h"

#include <fstream>


/* Define useful constants
*/
#define DELETE_TEXT         "***DELETE***"
#define NO_PARTIAL_TEXT     "***NOPARTIAL***"
#define COPY_SCRIPT         "***COPY***"
#define CLASS_FEATURE_PREREQ    "var featureid as string\n" \
                                "featureid = \"%s\"\n" \
                                "call ReqFeature"
#define POWER_PREREQ            "perform hero.childfound[%s].setfocus\n" \
                                "call ReqPower"


/* Define a struct used to manage memory
*/
struct  T_Memory_Info {
    T_Int32U    size;
    /* data goes here */
    };


/* Define all the different types of nodes we might want to duplicate
*/
enum    E_Node_Type {
    e_node_tag = 0,
    e_node_bootstrap,
    e_node_arrayval,
    e_node_eval,
    e_node_evalrule,
    e_node_prereq,
    e_node_exprreq,
    e_node_pickreq,
    e_node_containerreq,
    };


/* Structure to support delayed copying of scripts
*/
struct  T_Delayed_Copy {
    T_XML_Node      target;
    T_Glyph_CPtr    thing_id;
    T_Glyph_CPtr    index;
    T_Glyph_CPtr    nodename;
    };
typedef vector<T_Delayed_Copy>  T_Copy_Vector;
typedef T_Copy_Vector::iterator T_Copy_Iter;


/* Define static variables used below
*/
static ofstream *       l_log = NULL;


void        Initialize_Helper(ofstream * stream)
{
    x_Trap_Opt(l_log != NULL);

    if (Is_Log())
        l_log = stream;
}


void        Shutdown_Helper(void)
{
    l_log = NULL;
}


T_Status    Mem_Acquire(T_Int32U size, T_Void_Ptr * ptr)
{
    T_Memory_Info *     mem;

    mem = (T_Memory_Info *) malloc(size + sizeof(T_Memory_Info));
    if (mem == NULL)
        x_Status_Return(LWD_ERROR);
    mem->size = size;
    *ptr = (T_Void_Ptr) (mem + 1);
    x_Status_Return_Success();
}


T_Status    Mem_Resize(T_Void_Ptr current, T_Int32U requested, T_Void_Ptr * ptr)
{
    T_Memory_Info *     old_mem;
    T_Memory_Info *     new_mem;

    old_mem = (T_Memory_Info *) ((T_Memory_Info *) current - 1);

    /* resize the allocation to the new size
    */
    new_mem = (T_Memory_Info *) realloc(old_mem,requested + sizeof(T_Memory_Info));
    if (new_mem == NULL)
        x_Status_Return(LWD_ERROR);

    new_mem->size = requested;
    *ptr = (T_Void_Ptr) (new_mem + 1);
    x_Status_Return_Success();
}


void        Mem_Release(T_Void_Ptr ptr)
{
    T_Memory_Info *     mem;

    mem = (T_Memory_Info *) ((T_Memory_Info *) ptr - 1);
    free(mem);
}


void        Log_Message(T_Glyph_Ptr message, bool is_console)
{
    /* Output the message to our log file and to the console if requested. Make
        sure to flush appropriately so everything is updated immediately - we
        want that more than we want to save a tiny amount of performance.
    */
    if (is_console) {
        printf("%s", message);
        fflush(stdout);
        }
    if (l_log != NULL) {
        (*l_log) << message;
        l_log->flush();
        }
}


bool        Check_Duplicate_Ids(T_Glyph_Ptr text, T_List_Ids * id_list)
{
    T_Int32U        i, count;
    T_Unique        power_id;
    T_Unique        check_id;

    /* Make sure this is a valid unique id - if not, that's a problem
    */
    if (!UniqueId_Is_Valid(text))
        return(false);
    power_id = UniqueId_From_Text(text);

    /* Check to make sure we don't have a collision
        FIX - this is doing a linear search, optimize it
    */
    count = id_list->size();
    for (i = 0; i < count; i++) {
        check_id = (*id_list)[i];
        if (check_id == power_id)
            return(false);
        }

    return(true);
}


static bool     Compact_Name(T_Glyph_Ptr buffer, T_Glyph_Ptr name, T_List_Ids * id_list,
                                T_Int32U second_length, T_Glyph_Ptr suffix)
{
    T_Int32U            i, word_count, second_word, first_length, suffix_length;
    T_Glyph_Ptr         ptr, dest;
    T_Glyph             name_copy[1000];
    T_Glyph_Ptr         words[100];
    static  T_Glyph_Ptr ignore[] = { "a ", "an ", "the ", "for ", "of ", "and ", "as ", "is ", };

    /* Work out how long the first chunk of our name should be, to allow an
        additional 3 characters from our second word
    */
    suffix_length = (suffix == NULL) ? 0 : strlen(suffix);
    first_length = UNIQUE_ID_LENGTH - strlen(buffer) - second_length - suffix_length;
    if (x_Trap_Opt((T_Int32S) first_length < 0))
        return(false);

    /* Make a copy of our name, since we mess with it destructively, and
        initialize our word array
    */
    strcpy(name_copy, name);
    word_count = 0;
    memset(words, 0, sizeof(words));

    /* We need a good rule for compacting a long power name into a short,
        6-7 character space. Here's a decent one - take the first few characters
        of the first word, then the first 3 characters of the next word. If we
        collide with an existing id, use the first 3 characters of the next
        word instead, and so on.
        Skip all words like "a", "the", "an", etc.
        Start by getting the first word.
    */
    ptr = name_copy;
    while (*ptr != '\0') {

        /* Skip all spaces.
        */
        while (*ptr == ' ')
            ptr++;

        /* Check to see if this next word is one of the ones we ignore or not.
            If it is, skip past it and go on to the next word.
        */
        for (i = 0; i < x_Array_Size(ignore); i++)
            if (strnicmp(ptr, ignore[i], strlen(ignore[i])) == 0)
                break;
        if (i < x_Array_Size(ignore)) {
            ptr += strlen(ignore[i]);
            continue;
            }

        /* Save the location of this word into our words array, and add a null
            terminator after it.
        */
        words[word_count++] = ptr;
        i = 0;
        while ((*ptr != ' ') && (*ptr != '\0'))
            ptr++;
        if (*ptr != '\0')
            *ptr++ = '\0';
        }

    /* Start by appending the first N letters of the first word to the unique
        id.
    */
    dest = buffer + strlen(buffer);
    strncpy(dest, words[0], first_length);
    dest += min((T_Int32U) strlen(words[0]),first_length);

    /* Try making a unique id that isn't already taken out of words 1 & 2, then
        1 & 3, then 1 & 4, etc. If that fails, give up - we'll have to
        hard-code a unique id for the power.
    */
    second_word = 1;
    for (second_word = 1; second_word < word_count; second_word++) {

        /* If this word is less than 2 letters long, skip it, since it won't
            make a good id. Unless it's a number, in which case that's ok.
        */
        if ((words[second_word][0] == '\0') ||
            ((words[second_word][1] == '\0') && !x_Is_Digit(words[second_word][0])))
            continue;

        /* Copy up to three letters of the word into the destination buffer,
            followed by our suffix (if any).
        */
        strncpy(dest, words[second_word], second_length);
        dest[second_length] = '\0';
        if (suffix != NULL)
            strcpy(dest + min(second_length,(T_Int32U) strlen(words[second_word])), suffix);
        buffer[UNIQUE_ID_LENGTH] = '\0';

        /* Check whether this is a usable id - if so, we're done
        */
        if (Check_Duplicate_Ids(buffer, id_list))
            return(true);
        }

    /* We didn't find a usable unique id, so return false.
    */
    return(false);
}


static void     Second_Attempt(T_Glyph_Ptr buffer)
{
    T_Glyph_Ptr     source, dest;

    /* Remove lower case vowels, and see if we can make an id from that - it's
        a bit weird, but worth a shot
    */
    source = buffer;
    dest = buffer;
    while (*source != '\0') {
        if ((*source != 'a') && (*source != 'e') && (*source != 'i') &&
            (*source != 'o') && (*source != 'u'))
            *dest++ = *source;
        source++;
        }
    *dest = '\0';
}


bool            Generate_Thing_Id(T_Glyph_Ptr buffer, T_Glyph_Ptr init, T_Glyph_Ptr thing_name,
                                    T_List_Ids * id_list, T_Int32U second_length, T_Glyph_Ptr suffix,
                                    bool is_try_again)
{
    T_Int32U        words, init_size, append_size, remaining_size, suffix_size;
    T_Glyph_Ptr     ptr;
    T_Unique        thing_id;
    T_Glyph         name[1000];

    /* Zero out the buffer (so we don't have to worry about null terminating)
        and then copy our initial length of stuff into it
    */
    memset(buffer, 0, UNIQUE_ID_LENGTH+1);
    strcpy(buffer, init);
    init_size = strlen(init);
    suffix_size = (suffix == NULL) ? 0 : strlen(suffix);

    /* Check for an explicit id in our list - if so, copy the first N
        characters out of it
    */
    ptr = Mapping_Find("fixthing", thing_name);
    if (ptr != NULL) {
        append_size = strlen(ptr);
        remaining_size = UNIQUE_ID_LENGTH - init_size - suffix_size;
        if (append_size > remaining_size)
            append_size = remaining_size;
        strncat(buffer, ptr, append_size);
        if (suffix != NULL)
            strcat(buffer, suffix);
        if (!Check_Duplicate_Ids(buffer, id_list)) {
            sprintf(name, "Unique id fixup '%s' already taken!\n", buffer);
            Log_Message(name);
            return(false);
            }
        goto finish_id;
        }

    /* Get rid of a "The" on the front to save time later
    */
    if (strncmp(thing_name, "The ", 4) == 0)
        thing_name += 4;

    /* Make a copy of our thing name, including only alphanumeric characters,
        digits, and spaces - this removes any characters that might be
        problematic in unique ids
    */
    ptr = name;
    while (*thing_name != '\0') {
        if (x_Is_Alpha(*thing_name) || x_Is_Digit(*thing_name) || (*thing_name == ' '))
            *ptr++ = *thing_name;
        *thing_name++;
        if (x_Trap_Opt(ptr - name > 500)) {
            sprintf(name, "Invalid thing name '%s'!\n", thing_name);
            Log_Message(name);
            return(false);
            }
        }
    *ptr = '\0';

    /* Get the number of words by counting spaces
    */
    words = 1;
    ptr = name;
    while (*ptr != '\0')
        if (*ptr++ == ' ')
            words++;

    /* If the class name is just one word, just use the first N letters as the
        rest of the id
    */
    if (words == 1) {
        append_size = strlen(name);
        remaining_size = UNIQUE_ID_LENGTH - init_size - suffix_size;
        if (append_size > remaining_size)
            append_size = remaining_size;
        strncat(buffer, name, append_size);
        if (suffix != NULL)
            strcat(buffer, suffix);
        if (!Check_Duplicate_Ids(buffer, id_list)) {

            /* If that's already taken, try removing vowels and trying again
            */
            Second_Attempt(buffer);
            if (!is_try_again || !Check_Duplicate_Ids(buffer, id_list))
                return(false);
            }
        }

    /* Otherwise, do something more complex
    */
    else {
        if (!Compact_Name(buffer, name, id_list, second_length, suffix)) {

            /* If that's already taken, try removing vowels and trying again
            */
            Second_Attempt(name);
            strcpy(buffer, init);
            if (!is_try_again || !Compact_Name(buffer, name, id_list, second_length, suffix))
                return(false);
            }
        }

    /* Make sure this is a valid unique id - if not, that's a problem
    */
finish_id:
    if (!UniqueId_Is_Valid(buffer))
        return(false);
    thing_id = UniqueId_From_Text(buffer);

    /* Add this id to the list so other powers can check it
    */
    id_list->push_back(thing_id);

    /* Add the new id to our list of ids
    */
    return(true);
}


bool            Generate_Tag_Id(T_Glyph_Ptr buffer, T_Glyph_Ptr tag_name, T_XML_Node root)
{
    long            result;
    T_Unique        id;
    T_Glyph_Ptr     ptr;
    T_XML_Node      node;
    T_List_Ids      id_list;

    /* Check for an explicit id in our list - if so, just use it
    */
    ptr = Mapping_Find("fixtag", tag_name);
    if (ptr != NULL) {
        strcpy(buffer, ptr);
        return(true);
        }

    /* Build a unique id list from our tag group - we should really cache this
        for improved performance but that doesn't matter right now
    */
    result = XML_Get_First_Named_Child(root, "extgroup", &node);
    while (result == 0) {
        ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(node, "tag");
        if (!UniqueId_Is_Valid(ptr))
            x_Trap_Opt(1);
        else {
            id = UniqueId_From_Text(ptr);
            id_list.push_back(id);
            }
        result = XML_Get_Next_Named_Child(root, &node);
        }

    return(Generate_Thing_Id(buffer, "", tag_name, &id_list, 5));
}


static void     Truncate_Errata(T_Glyph_Ptr buffer, T_Glyph_Ptr search)
{
    T_Glyph_Ptr     ptr, temp;

    ptr = strstr(buffer, search);
    if (ptr == NULL)
        return;

    /* Make sure this is an actual errata thing, not just the word
        "Additionally" at the start of a new line for example
    */
    temp = ptr + strlen(search);
    if (x_Is_Space(*temp) || (*temp == '('))
        *ptr = '\0';
}


T_Int32S        Create_Thing_Node(T_XML_Node parent, T_Glyph_Ptr term, T_Glyph_Ptr compset,
                                    T_Glyph_Ptr name, T_Glyph_Ptr id, T_Glyph_Ptr description,
                                    T_Int32U uniqueness, T_Glyph_Ptr source, T_XML_Node * node,
                                    bool is_no_descript_ok)
{
    T_Int32S        result;
    T_Glyph_Ptr     ptr;
    T_Glyph         buffer[MAX_XML_BUFFER_SIZE];

    if (x_Trap_Opt((parent == NULL) || (compset == NULL) || (compset[0] == '\0') ||
         (name == NULL) || (name[0] == '\0') || (id == NULL) || (id[0] == '\0') ||
         (node == NULL)))
        return(-999);

    /* Create a node for this thing and mark it as unique
    */
    result = XML_Create_Child(parent, "thing", node);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "Could not create new %s node.", term);
        Log_Message(buffer);
        return(result);
        }
    XML_Write_Text_Attribute(*node, "compset", compset);
    XML_Write_Text_Attribute(*node, "name", name);
    if (uniqueness == UNIQUENESS_UNIQUE)
        XML_Write_Text_Attribute(*node, "uniqueness", "unique");
    else if (uniqueness == UNIQUENESS_USER_ONCE)
        XML_Write_Text_Attribute(*node, "uniqueness", "useronce");

    /* Add the unique id and description - snip any errata text off the
        description, since we don't care about what the rules used to be.
    */
    XML_Write_Text_Attribute(*node, "id", id);
    if ((description != NULL) && (description[0] != '\0')) {
        strcpy(buffer, description);
        Truncate_Errata(buffer, "{br}Revision");
        Truncate_Errata(buffer, "{br}Addition");
        Truncate_Errata(buffer, "{br}Update");
        Truncate_Errata(buffer, "{br}Deletion");
        ptr = buffer + strlen(buffer);
        while ((ptr >= buffer + 4) && (strcmp(ptr-4, "{br}") == 0)) {
            ptr -= 4;
            *ptr = '\0';
            }
        XML_Write_Text_Attribute(*node, "description", buffer);
        }

    /* If we didn't find a description, raise an error unless our 'no
        description ok' flag is present.
    */
    else if (!is_no_descript_ok) {
        sprintf(buffer, "%s %s has no description text\n", term, name);
        Log_Message(buffer);
        }

    /* Add our source
    */
    result = Output_Source(*node, source);
    if (x_Trap_Opt(result != 0))
        return(result);

    return(0);
}


void        Split_To_Array(T_Glyph_Ptr buffer, T_Glyph_Ptr chunks[], T_Int32U * chunk_count,
                            T_Glyph_Ptr text, T_Glyph_Ptr splitchars,
                            T_Glyph_Ptr ignore_after,
                            T_Glyph_Ptr * split_words, T_Int32U split_word_count)
{
    T_Int32U        i, skip_length, max_chunks = *chunk_count;
    T_Glyph_Ptr     ptr, temp;
    T_Int32U        lengths[100];

    *chunk_count = 0;

    if (text == NULL)
        return;

    /* Cache our split word lengths
    */
    for (i = 0; i < split_word_count; i++)
        lengths[i] = strlen(split_words[i]);

    /* Copy our text into the buffer, so we can do a destructive parse
    */
    strcpy(buffer, text);
    ptr = buffer;

    /* Start iterating through our buffer until we run out of text
    */
    while (*ptr != '\0') {

        /* Skip past any spaces
        */
        while (*ptr == ' ')
            ptr++;

        /* If we reached the end of the buffer, get out
        */
        if (*ptr == '\0')
            break;

        /* If we already have our maximum number of chunks, get out
        */
        if (*chunk_count == max_chunks)
            return;

        /* Save this point as the next chunk in the array
        */
        chunks[*chunk_count] = ptr++;

        /* Find the next splitter character or word in the string - if we get
            to the end, stop looking
        */
        skip_length = 0;
        while (*ptr != '\0') {
            if (strchr(splitchars, *ptr) != NULL) {
                skip_length = 1;
                break;
                }
            for (i = 0; i < split_word_count; i++) {
                if (strncmp(ptr, split_words[i], lengths[i]) == 0) {
                    skip_length = lengths[i];
                    break;
                    }
                }
            if (i < split_word_count)
                break;
            ptr++;
            }

        /* If we found a splitter character, set it to the terminator and move
            on - this terminates the entry we just added to the tags array and
            sets things up for the next tag.
        */
        if (*ptr != '\0') {
            *ptr = '\0';
            ptr += skip_length;
            }

        /* If we have a "ignore after" string specified, find the first
            instance of any of those characters in the chunk and terminate it
            there
        */
        if (ignore_after != NULL ) {
            temp = chunks[*chunk_count];
            while ((*temp != '\0') && (strchr(ignore_after, *temp) == NULL))
                temp++;
            *temp = '\0';
            }

        /* Trim whitespace from around this entry
        */
        Text_Trim_Leading_Space(chunks[*chunk_count]);

        *chunk_count += 1;
        }
}


void        Split_To_Tags(T_XML_Node node, T_Base_Info * info, T_Glyph_Ptr text,
                            T_Glyph_Ptr splitchars, T_Glyph_Ptr group,
                            T_Glyph_Ptr mapping1, T_Glyph_Ptr mapping2,
                            bool is_dynamic_name, T_Glyph_Ptr ignore_after,
                            T_Glyph_Ptr * split_words, T_Int32U split_word_count)
{
    T_Int32U            i, chunk_count;
    T_Glyph_Ptr         chunks[1000];
    T_Glyph             buffer[100000];

    if ((text == NULL) || (text[0] == '\0')) {
        sprintf(buffer, "No tags found for group %s in thing %s\n", group, info->name);
        Log_Message(buffer);
        return;
        }

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, text, splitchars, ignore_after,
                    split_words, split_word_count);

    /* Loop through our tags, adding them to the node.
    */
    for (i = 0; i < chunk_count; i++)
        Output_Tag(node, group, chunks[i], info, mapping1, mapping2, is_dynamic_name);
}


void        Split_To_Dynamic_Tags(T_XML_Node node, T_Base_Info * info, T_Glyph_Ptr text,
                            T_Glyph_Ptr splitchars, T_Glyph_Ptr group,
                            T_Glyph_Ptr mapping1, T_Glyph_Ptr mapping2,
                            bool is_dynamic_name, T_Glyph_Ptr ignore_after)
{
    T_Int32U            i, chunk_count;
    T_Glyph_Ptr         chunks[1000];
    T_Glyph             buffer[100000];

    if ((text == NULL) || (text[0] == '\0')) {
        sprintf(buffer, "No tags found for group %s in thing %s\n", group, info->name);
        Log_Message(buffer);
        return;
        }

    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, text, splitchars, ignore_after);

    /* Loop through our tags, adding them to the node.
    */
    for (i = 0; i < chunk_count; i++)
        Output_Tag(node, group, chunks[i], info, mapping1, mapping2, is_dynamic_name,
                    true);
}


T_Int32S        Output_Source(T_XML_Node node, T_Glyph_Ptr source)
{
    T_Int32U            i;
    long                result;
    T_Glyph_Ptr         ptr, id_text = NULL;
    bool                is_adventure;
    T_Tuple *           tuple;
    T_XML_Node          child;
    T_Glyph             source_copy[500], buffer[500];
    T_Glyph_Ptr         ignore[] = {    "Player's Handbook", "Players Handbook",
                                        "Dungeon Master's Guide", "Dungeon Masters Guide",
                                        "Monster Manual", "Player's Handbook 1" };
    T_Glyph_Ptr         chop[] = {      "Dragon Magazine", "Dungeon Magazine",
                                        "PH Heroes: Series" };

    /* It's ok to not have a source, since some types of things (e.g. class
        features) often don't bother with them
    */
    if (source == NULL)
        return(0);

    /* Truncate the source at any comma, since it's usually "Some Big Book,
        RPGA Minor Adventure 5"
    */
    strcpy(source_copy, source);
    ptr = strchr(source_copy,',');
    if (ptr != NULL)
        *ptr = '\0';

    /* Make sure it's limited to the maximum length for the source
    */
    source_copy[50] = '\0';

    /* Try to find an initial name for this source in our ignore list - if
        it's a source we ignore, get out.
    */
    for (i = 0; i < x_Array_Size(ignore); i++)
        if (strcmp(ignore[i], source_copy) == 0)
            return(0);

    /* Chop any numbers and whitespace off the end of the source for
        specified sources - this collapses all "Dragon Magazine" entries down
        to a single one, but we don't want to do this for stuff like Player's
        Handbook 2.
    */
    for (i = 0; i < x_Array_Size(chop); i++)
        if (strncmp(chop[i], source_copy, strlen(chop[i])) == 0)
            break;
    if (i < x_Array_Size(chop))
        strcpy(source_copy, chop[i]);

    /* Try to find an id for this source - if we don't find one, look for the
        same in our adventures list. If we still don't find one, get out.
    */
    is_adventure = false;
    tuple = Mapping_Find_Tuple("source", source_copy);
    if (tuple != NULL) {
        id_text = tuple->b;
        source = (tuple->c == NULL) ? tuple->a : tuple->c;
        }
    else {
        tuple = Mapping_Find_Tuple("adventure", source_copy);
        if (tuple != NULL) {
            id_text = tuple->b;
            source = (tuple->c == NULL) ? tuple->a : tuple->c;
            is_adventure = true;
            }
        else {
            sprintf(buffer, "No source id found for %s\n", source);
            Log_Message(buffer);
            return(0);
            }
        }

    /* Create a node for this tag
    */
    result = XML_Create_Child(node, "usesource", &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new source node.");
        return(result);
        }

    /* Set up our source id
    */
    XML_Write_Text_Attribute(child, "source", id_text);

    /* If there's no source by that id under our sources file, create a new
        node for it
    */
    result = XML_Get_First_Named_Child_With_Attr(Get_Source_Root(), "source", "id", id_text, &child);
    if (result == 0)
        return(0);
    XML_Create_Child(Get_Source_Root(), "source", &child);
    XML_Write_Text_Attribute(child, "id", id_text);
    XML_Write_Text_Attribute(child, "name", source);
    XML_Write_Text_Attribute(child, "parent", is_adventure ? "Adventure" : "Supplement");
    sprintf(buffer, "Enable rules from the '%s' supplement.", source);
    XML_Write_Text_Attribute(child, "description", buffer);
    XML_Write_Boolean_Attribute(child, "default", true);
    return(0);
}


T_Int32S        Output_Language(T_XML_Node node, T_Glyph_Ptr name, T_Base_Info * info,
                                T_List_Ids * id_list, C_Pool * pool, T_Glyph_Ptr boot_group,
                                T_Glyph_Ptr boot_tag)
{
    long                result;
    T_Glyph_Ptr         map;
    T_Glyph             lang_id[100], message[500];

    /* If we already know this language, use the existing version
    */
    map = Mapping_Find("language", name);
    if (map != NULL) {
        result = Output_Bootstrap(info->node, map, info, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                    boot_group, boot_tag);
        return(result);
        }

    /* Otherwise, generate the unique id for this language's name
    */
    if (!Generate_Thing_Id(lang_id, "lan", name, id_list)) {
        sprintf(message, "No unique id could be generated for language '%s' for %s\n", name, info->name);
        Log_Message(message);
        return(-1);
        }

    /* Create a node for the new language and add a bootstrap for it
    */
    result = Create_Thing_Node(Get_Language_Root(), "language", "Language",
                            name, lang_id, "", UNIQUENESS_UNIQUE, NULL, &node, true);
    if (x_Trap_Opt(result != 0))
        return(result);
    Output_Bootstrap(info->node, lang_id, info, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                boot_group, boot_tag);
    Mapping_Add("language", pool->Acquire(name), pool->Acquire(lang_id));
    return(0);
}


T_Int32S        Output_Tag(T_XML_Node node, T_Glyph_Ptr group, T_Glyph_Ptr tag,
                        T_Base_Info * info, T_Glyph_Ptr mapping1, T_Glyph_Ptr mapping2,
                        bool is_dynamic_name, bool is_dynamic_group)
{
    T_Int32U            i;
    long                result;
    T_Glyph_Ptr         id_text = NULL, tag_name, ptr;
    T_XML_Node          child;
    T_Tuple *           tuple;
    T_Glyph             buffer[500], id_buffer[UNIQUE_ID_LENGTH+1];

    if ((tag == NULL) || (tag[0] == '\0')) {
        sprintf(buffer, "No tag present for group %s (thing %s).\n", group, info->name);
        Log_Message(buffer);
        return(-1);
        }

    /* If we didn't get a lookup array passed in, just use the tag id as an id
    */
    tag_name = tag;
    if (mapping1 == NULL)
        id_text = tag;

    /* Otherwise, look it up
    */
    else {
        tuple = Mapping_Find_Tuple(mapping1, tag);
        if (tuple != NULL) {
            id_text = (tuple->b == NULL) ? tuple->a : tuple->b;
            tag_name = tuple->a;
            }

        /* If we didn't find it, but we have a second group, check there
        */
        else if (mapping2 != NULL) {
            tuple = Mapping_Find_Tuple(mapping2, tag);
            if (tuple != NULL) {
                id_text = (tuple->b == NULL) ? tuple->a : tuple->b;
                tag_name = tuple->a;
                }
            }

        /* If we still didn't find it, get out, unless this is a dynamic group,
            in which case we just create the new tag
        */
        if (id_text == NULL) {
            if (is_dynamic_group)
                id_text = tag;
            else {
                sprintf(buffer, "No lookup id found for group %s, tag %s (thing %s)\n", group, tag, info->name);
                Log_Message(buffer);
                return(-1);
                }
            }
        }

    /* Check to see if our id text is a valid unique id - if not, and this
        isn't a dynamic group, we're in trouble
    */
    if (!UniqueId_Is_Valid(id_text)) {
        if (!is_dynamic_group) {
            sprintf(buffer, "Invalid tag specified for group %s, tag %s (thing %s)\n", group, tag, info->name);
            Log_Message(buffer);
            return(-1);
            }

        /* First, we'll need to make sure we're using a dynamic name, so set
            the flag
        */
        is_dynamic_name = true;

        /* Now we need to turn our name into a unique id - there's no good way
            to do this, so just take the first 10 valid characters
        */
        memset(id_buffer, 0, sizeof(id_buffer));
        ptr = id_buffer;
        i = 0;
        while ((id_text[i] != '\0') && (ptr < id_buffer + UNIQUE_ID_LENGTH)) {
            if (x_Is_Digit(id_text[i]) || x_Is_Alpha(id_text[i]) || (id_text[i] == '_'))
                *ptr++ = id_text[i];
            i++;
            }

        /* If we didn't have any valid characters at all, get out. Otherwise,
            update our id_text pointer.
        */
        if (id_buffer[0] == '\0') {
            sprintf(buffer, "No valid characters found for unique id in group %s, tag %s (thing %s)\n", group, tag, info->name);
            Log_Message(buffer);
            return(-1);
            }
        id_text = id_buffer;
        }

    /* Create a node for this tag
    */
    result = XML_Create_Child(node, "tag", &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new tag node.\n");
        return(result);
        }

    /* Set up our group and tag ids
    */
    XML_Write_Text_Attribute(child, "group", group);
    XML_Write_Text_Attribute(child, "tag", id_text);
    if (is_dynamic_name && (strcmp(id_text, tag_name) != 0))
        XML_Write_Text_Attribute(child, "name", tag_name);
    return(0);
}


T_Int32S        Output_Link(T_XML_Node node, T_Glyph_Ptr linkage, T_Glyph_Ptr thing,
                        T_Base_Info * info, T_Glyph_Ptr mapping)
{
    long                result;
    T_Glyph_Ptr         id_text = NULL, thing_name;
    T_Tuple *           tuple;
    T_XML_Node          child;
    T_Glyph             buffer[500];

    /* If we didn't get a lookup array passed in, just use the thing id as an id
    */
    thing_name = thing;
    if (mapping == NULL)
        id_text = thing;

    /* Otherwise, look it up
    */
    else {
        tuple = Mapping_Find_Tuple(mapping, thing);
        if (tuple != NULL) {
            id_text = (tuple->b == NULL) ? tuple->a : tuple->b;
            thing_name = tuple->a;
            }

        /* If we didn't find it, get out
        */
        else {
            sprintf(buffer, "No lookup id found for linkage %s (thing %s)\n", linkage, thing);
            Log_Message(buffer);
            return(0);
            }
        }

    /* Create a node for this thing
    */
    result = XML_Create_Child(node, "link", &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new thing node.\n");
        return(result);
        }

    /* Set up our group and thing ids
    */
    XML_Write_Text_Attribute(child, "linkage", linkage);
    XML_Write_Text_Attribute(child, "thing", id_text);
    return(0);
}


T_Int32S        Output_Field(T_XML_Node node, T_Glyph_Ptr field, T_Glyph_Ptr value,
                                T_Base_Info * info)
{
    long                result;
    T_XML_Node          child;
    T_Glyph             buffer[500];

    if ((value == NULL) || (value[0] == '\0')) {
        sprintf(buffer, "No field value present for field %s (thing %s).\n", field, info->name);
        Log_Message(buffer);
        return(0);
        }

    /* Create a node for this fieldval
    */
    result = XML_Create_Child(node, "fieldval", &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new fieldval node.\n");
        return(result);
        }

    /* Output our field details.
    */
    XML_Write_Text_Attribute(child, "field", field);
    XML_Write_Text_Attribute(child, "value", value);
    return(0);
}


T_Int32S        Output_Exprreq(T_XML_Node node, T_Glyph_Ptr message, T_Glyph_Ptr expr,
                                T_Base_Info * info)
{
    long                result;
    T_XML_Node          child;
    T_Glyph             buffer[500];

    if ((message == NULL) || (message[0] == '\0') ||
        (expr == NULL) || (expr[0] == '\0')) {
        sprintf(buffer, "No message or expression present for exprreq on thing %s.\n", info->name);
        Log_Message(buffer);
        return(0);
        }

    /* Create a node for this exprreq
    */
    result = XML_Create_Child(node, "exprreq", &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new exprreq node.\n");
        return(result);
        }

    /* Set up our group and tag ids
    */
    XML_Write_Text_Attribute(child, "message", message);
    XML_Set_PCDATA(child, expr);
    return(0);
}


T_Int32S        Output_Prereq(T_XML_Node node, T_Glyph_Ptr message, T_Glyph_Ptr script,
                                T_Base_Info * info)
{
    long                result;
    T_XML_Node          child, validate;
    T_Glyph             buffer[500];

    if ((message == NULL) || (message[0] == '\0') ||
        (script == NULL) || (script[0] == '\0')) {
        sprintf(buffer, "No message or script present for prereq on thing %s.\n", info->name);
        Log_Message(buffer);
        return(0);
        }

    /* Create a node for this scriptreq
    */
    result = XML_Create_Child(node, "prereq", &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new prereq node.\n");
        return(result);
        }
    result = XML_Create_Child(child, "validate", &validate);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new prereq node.\n");
        return(result);
        }

    /* Set up our group and tag ids
    */
    XML_Write_Text_Attribute(child, "message", message);
    XML_Set_PCDATA(validate, script);
    return(0);
}


T_Int32S        Output_Bootstrap(T_XML_Node node, T_Glyph_CPtr thing, T_Base_Info * info,
                                    T_Glyph_Ptr condition, T_Glyph_Ptr phase, T_Glyph_Ptr priority,
                                    T_Glyph_Ptr before, T_Glyph_Ptr after,
                                    T_Glyph_Ptr assign_field, T_Glyph_Ptr assign_value,
                                    T_Glyph_Ptr auto_group, T_Glyph_Ptr auto_tag,
                                    T_Glyph_Ptr auto_group2, T_Glyph_Ptr auto_tag2)
{
    long                result;
    T_XML_Node          child, containerreq, assignval, timing;
    T_Glyph             buffer[500];

    if ((thing == NULL) || (thing[0] == '\0')) {
        sprintf(buffer, "No thing present for bootstrap on thing %s.\n", info->name);
        Log_Message(buffer);
        return(0);
        }

    /* Create a node for this bootstrap
    */
    result = XML_Create_Child(node, "bootstrap", &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new bootstrap node.\n");
        return(result);
        }

    /* Set up our thing id
    */
    XML_Write_Text_Attribute(child, "thing", thing);

    /* Add an assignval if we have it
    */
    if (assign_field != NULL) {
        result = XML_Create_Child(child, "assignval", &assignval);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new assignval node.\n");
            return(result);
            }
        XML_Write_Text_Attribute(assignval, "field", assign_field);
        XML_Write_Text_Attribute(assignval, "value", assign_value);
        }
    if (auto_group != NULL) {
        result = XML_Create_Child(child, "autotag", &assignval);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new autotag node.\n");
            return(result);
            }
        XML_Write_Text_Attribute(assignval, "group", auto_group);
        XML_Write_Text_Attribute(assignval, "tag", auto_tag);
        }
    if (auto_group2 != NULL) {
        result = XML_Create_Child(child, "autotag", &assignval);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new autotag node.\n");
            return(result);
            }
        XML_Write_Text_Attribute(assignval, "group", auto_group2);
        XML_Write_Text_Attribute(assignval, "tag", auto_tag2);
        }

    /* If we don't have a condition, we're done
    */
    if ((condition == NULL) || (condition[0] == '\0'))
        return(0);

    /* Otherwise, add a bootstrap condition
    */
    result = XML_Create_Child(child, "containerreq", &containerreq);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new containerreq node.\n");
        return(result);
        }
    XML_Write_Text_Attribute(containerreq, "phase", phase);
    XML_Write_Text_Attribute(containerreq, "priority", priority);
    XML_Set_PCDATA(containerreq, condition);
    if (before != NULL) {
        result = XML_Create_Child(containerreq, "before", &timing);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new timing node.\n");
            return(result);
            }
        XML_Write_Text_Attribute(timing, "name", before);
        }
    if (after != NULL) {
        result = XML_Create_Child(containerreq, "after", &timing);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new timing node.\n");
            return(result);
            }
        XML_Write_Text_Attribute(timing, "name", after);
        }
    return(0);
}


static T_Int32S Output_Script(T_Glyph_Ptr type, T_XML_Node node, T_Glyph_CPtr script, T_Glyph_Ptr phase,
                            T_Glyph_Ptr priority, T_Base_Info * info, T_Glyph_Ptr before,
                            T_Glyph_Ptr after, T_XML_Node * script_node)
{
    long                result;
    T_XML_Node          child, timing;
    T_Glyph             buffer[500];

    if ((script == NULL) || (script[0] == '\0')) {
        sprintf(buffer, "No script present for script on thing %s.\n", info->name);
        Log_Message(buffer);
        return(0);
        }

    /* Create a node for this eval script
    */
    result = XML_Create_Child(node, type, &child);
    if (x_Trap_Opt(result != 0)) {
        Log_Message("Could not create new script node.\n");
        return(result);
        }

    /* Get the next index and assign it
    */
    XML_Write_Text_Attribute(child, "index", As_Int(XML_Get_Named_Child_Count(node, type)));

    /* Set up our script, phase and priority
    */
    XML_Write_Text_Attribute(child, "phase", phase);
    XML_Write_Text_Attribute(child, "priority", priority);
    XML_Set_PCDATA(child, script);

    /* Add any before / after nodes
    */
    if (before != NULL) {
        result = XML_Create_Child(child, "before", &timing);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new timing node.\n");
            return(result);
            }
        XML_Write_Text_Attribute(timing, "name", before);
        }
    if (after != NULL) {
        result = XML_Create_Child(child, "after", &timing);
        if (x_Trap_Opt(result != 0)) {
            Log_Message("Could not create new timing node.\n");
            return(result);
            }
        XML_Write_Text_Attribute(timing, "name", after);
        }

    if (script_node != NULL)
        *script_node = child;
    return(0);
}


T_Int32S        Output_Eval(T_XML_Node node, T_Glyph_CPtr script, T_Glyph_Ptr phase,
                            T_Glyph_Ptr priority, T_Base_Info * info, T_Glyph_Ptr before,
                            T_Glyph_Ptr after)
{
    return(Output_Script("eval", node, script, phase, priority, info, before, after, NULL));
}


T_Int32S        Output_Eval_Rule(T_XML_Node node, T_Glyph_CPtr script, T_Glyph_Ptr phase,
                            T_Glyph_Ptr priority, T_Glyph_Ptr message, T_Base_Info * info,
                            T_Glyph_Ptr before, T_Glyph_Ptr after)
{
    long        result;
    T_XML_Node  script_node;

    result = Output_Script("evalrule", node, script, phase, priority, info, before, after,
                            &script_node);
    if (result != 0)
        return(result);

    XML_Write_Text_Attribute(script_node, "message", message);

    return(0);
}


static T_Glyph_Ptr Find_Deity(T_Glyph_Ptr deity_name)
{
    T_Glyph_Ptr         ptr;
    T_Glyph             buffer[1000];

    /* Skip past any spaces and then capitalise the name, to make sure we find
        stuff consistently and avoid problems with "the Raven Queen" vs "The
        Raven Queen"
    */
    while (*deity_name == ' ')
        deity_name++;
    strcpy(buffer, deity_name);
    buffer[0] = toupper(buffer[0]);

    ptr = Mapping_Find("deity", buffer);
    if (ptr != NULL)
        return(ptr);

    /* If we didn't find a deity yet, try to find a partial match - this helps
        when the prereq only uses the deity's first name
    */
    ptr = Mapping_Find("deity", buffer, false, true);
    return(ptr);
}


static bool         Check_Thing_Name(T_XML_Node root, T_Glyph_Ptr id_text, T_Glyph_Ptr req_name)
{
    long                result;
    T_XML_Node          node;
    T_Glyph_Ptr         name;

    result = XML_Get_First_Named_Child_With_Attr(root, "thing", "id", id_text, &node);
    if (result != 0)
        return(false);
    name = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(node, "name");
    if (stricmp(req_name, name) == 0)
        return(true);
    return(false);
}


static T_Glyph_Ptr  Search_Classes(T_Glyph_Ptr name, T_Glyph_Ptr find_id, bool is_class,
                                    bool * is_power)
{
    T_Int32U            i, count;
    long                result;
    T_XML_Document      document;
    T_XML_Node          root, node, bootstrap;
    T_Glyph_Ptr         id_text;
    vector<T_Glyph_Ptr> id_list;

    *is_power = false;

    if (is_class)
        document = C_DDI_Classes::Get_Crawler()->Get_Output_Document();
    else
        document = C_DDI_Races::Get_Crawler()->Get_Output_Document();
    if (x_Trap_Opt(document == NULL))
        return(NULL);
    result = XML_Get_Document_Node(document, &root);
    if (x_Trap_Opt(result != 0))
        return(NULL);

    /* Find the thing node with an appropriate id
    */
    result = XML_Get_First_Named_Child_With_Attr(root, "thing", "id", find_id, &node);
    if (x_Trap_Opt(result != 0))
        return(NULL);

    /* Go through the bootstraps for the thing and record the unique ids
    */
    result = XML_Get_First_Named_Child(node, "bootstrap", &bootstrap);
    while (result == 0) {
        id_text = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(bootstrap, "thing");
        id_list.push_back(id_text);
        result = XML_Get_Next_Named_Child(node, &bootstrap);
        }

    /* Look for a node with the appropriate id and our feature name - if we
        find one, that id is what we need
    */
    count = id_list.size();
    for (i = 0; i < count; i++) {
        id_text = id_list[i];
        if (Check_Thing_Name(root, id_text, name))
            return(id_text);
        }

    /* If we didn't find a feature with that id, we need to look for an
        appropriate power in the powers document
    */
    document = C_DDI_Powers::Get_Crawler()->Get_Output_Document();
    if (x_Trap_Opt(document == NULL))
        return(NULL);
    result = XML_Get_Document_Node(document, &root);
    if (x_Trap_Opt(result != 0))
        return(NULL);
    count = id_list.size();
    for (i = 0; i < count; i++) {
        id_text = id_list[i];
        if (Check_Thing_Name(root, id_text, name)) {
            *is_power = true;
            return(id_text);
            }
        }

    return(NULL);
}


static bool Find_Feature(T_Glyph_Ptr text, T_Glyph_Ptr last_class, T_Base_Info * info,
                                bool is_class, bool is_last_gasp)
{
    T_Glyph_Ptr         ptr;
    bool                is_power;
    T_Glyph             message[1000], expr[1000];

    /* We need a class to reliably search for a feature
    */
    if (last_class == NULL) {
        if (!is_last_gasp) {
            sprintf(message, "No class / race requirement found for feature '%s' on thing %s\n", text, info->name);
            Log_Message(message);
            }
        return(false);
        }

    /* Get the class file and search for a class feature with the
        appropriate name
    */
    ptr = Search_Classes(text, last_class, is_class, &is_power);
    if (ptr == NULL) {
        if (!is_last_gasp) {
            sprintf(message, "Class / race feature requirement '%s' on thing %s not found\n", text, info->name);
            Log_Message(message);
            }
        return(false);
        }
    sprintf(message, "'%s' feature required", text);
    message[1] = toupper(message[1]);
    if (is_power)
        sprintf(expr, POWER_PREREQ, ptr);
    else
        sprintf(expr, CLASS_FEATURE_PREREQ, ptr);
    Output_Prereq(info->node, message, expr, info);

    return(true);
}


static bool Find_Feat(T_Glyph_Ptr text, T_Base_Info * info, bool is_last_gasp)
{
    T_Tuple *       tuple;
    T_Glyph_Ptr     ptr;
    T_Glyph         message[1000], expr[1000];

    /* If this is a shield proficiency we require, use the tag as the
        requirement instead of wanting the feat
    */
    if (stricmp(text, "Shield Proficiency (Light)") == 0) {
        sprintf(message, "'%s' required", text);
        strcpy(expr, "hero.tagis[ArmorProf.apShieldLg] <> 0");
        Output_Exprreq(info->node, message, expr, info);
        return(true);
        }

    /* Otherwise, find the tuple and generate a standard exprreq
    */
    else {
        tuple = Mapping_Find_Tuple("feat", text);
        if (tuple != NULL) {
            ptr = (tuple->b == NULL) ? tuple->a : tuple->b;
            sprintf(message, "Feat '%s' required", tuple->a);
            sprintf(expr, "hero.tagis[Feat.%s] <> 0", ptr);
            Output_Exprreq(info->node, message, expr, info);
            return(true);
            }
        }

    if (!is_last_gasp) {
        sprintf(message, "Feat requirement '%s' on thing %s not found\n", text, info->name);
        Log_Message(message);
        }
    return(false);
}


static T_Glyph_Ptr  Find_Alternate_End(T_Glyph_Ptr text, T_Glyph_Ptr * next_ptr)
{
    T_Glyph_Ptr     end, next;

    end = strstr(text, " or ");
    next = NULL;
    if (end != NULL) {
        next = end + 4;
        while (x_Is_Space(*next))
            next++;
        }

    *next_ptr = next;
    return(end);
}


static bool Find_Alternates(T_Glyph_Ptr text, T_Base_Info * info, T_Glyph_Ptr * last, bool is_last_gasp,
                            T_Glyph_Ptr mapping, T_Glyph_Ptr group, T_Glyph_Ptr mapping_alt = NULL)
{
    bool            is_found;
    T_Glyph_Ptr     temp, end, next;
    T_Glyph         message[1000];

    /* Add a requirement for every one we find
    */
    is_found = false;
    do {
        /* Skip over any "the" at the front
        */
        while (x_Is_Space(*text))
            text++;
        if (strnicmp(text, "the ", 4) == 0) {
            text += 4;
            while (x_Is_Space(*text))
                text++;
            }

        end = Find_Alternate_End(text, &next);
        if (end != NULL)
            *end = '\0';

        /* Look in our primary (and alternate, if available) mappings for a
            match - this allows us to match essentials classes by looking in
            the "fake" class list
        */
        temp = Mapping_Find(mapping, text);
        if ((temp == NULL) && (mapping_alt != NULL))
            temp = Mapping_Find(mapping_alt, text);

        if (temp != NULL) {
            if (last != NULL)
                *last = temp;
            Output_Tag(info->node, group, temp, info);
            is_found = true;
            }
        else if (!is_last_gasp) {
            sprintf(message, "No %s matched for '%s' on prereq for %s\n", mapping, text, info->name);
            Log_Message(message);
            }

        /* Find the place where we separate from the next item
        */
        text = next;
        } while (text != NULL);

    return(is_found);
}


static bool Find_Classes(T_Glyph_Ptr text, T_Base_Info * info, T_Glyph_Ptr * last_class, bool is_last_gasp)
{
    return(Find_Alternates(text, info, last_class, is_last_gasp, "class", "ReqClass", "fakeclass"));
}


static bool Find_Races(T_Glyph_Ptr text, T_Base_Info * info, T_Glyph_Ptr * last_race, bool is_last_gasp)
{
    return(Find_Alternates(text, info, last_race, is_last_gasp, "races", "ReqRace"));
}


static bool Find_Weapons(T_Glyph_Ptr text, T_Base_Info * info, bool is_last_gasp)
{
    return(Find_Alternates(text, info, NULL, is_last_gasp, "weapon", "ReqWepProf"));
}


static void Find_Background(T_Glyph_Ptr text, T_Base_Info * info)
{
    T_Glyph_Ptr     ptr;
    T_Glyph         message[1000];

    ptr = Mapping_Find("background", text);
    if (ptr != NULL) {
        Output_Tag(info->node, "ReqBackgr", ptr, info);
        }
    else {
        sprintf(message, "No background matched for '%s' on prereq for %s\n", text, info->name);
        Log_Message(message);
        }
}


static void Output_Attr_Requirement(T_XML_Node node, T_Glyph_Ptr attr, T_Glyph_Ptr text,
                                    T_Base_Info * info)
{
    T_Int32U            value, value2;
    T_Glyph_Ptr         ptr, name, name2, attr_id, attr_id2;
    T_Glyph             message[500], script[500];

    /* Get the attribute id in case we need it
    */
    attr_id = Mapping_Find("attr", text, true);
    if (attr_id == NULL)
        attr_id = Mapping_Find("attrabbr", text, true);
    if (attr_id == NULL)
        return;

    /* Skip to after the attribute name (terminating it so we can use it as the
        name
    */
    ptr = text;
    name = text;
    while (!isdigit(*ptr) && (*ptr != '\0')) {
        if (isspace(*ptr))
            *ptr = '\0';
        ptr++;
        }

    /* Parse out the value
    */
    value = atoi(ptr);
    while (isdigit(*ptr) || isspace(*ptr))
        ptr++;

    /* If we don't have any other text, we just need to output a simple field
        requirement
    */
    if ((*ptr == '\0') || (strncmp(ptr, "or ", 3) != 0)) {
        Output_Field(info->node, attr, As_Int(value), info);
        return;
        }

    /* Parse out an 'or'
    */
    ptr += 3;
    while (isspace(*ptr))
        ptr++;

    /* Otherwise, try to parse out a second attribute name or abbreviation
        and value
    */
    attr_id2 = Mapping_Find("attr", ptr, true);
    if (attr_id2 == NULL)
        attr_id2 = Mapping_Find("attrabbr", ptr, true);
    if (attr_id2 == NULL) {
        Output_Field(info->node, attr, As_Int(value), info);
        return;
        }
    name2 = ptr;
    while (!isdigit(*ptr) && (*ptr != '\0')) {
        if (isspace(*ptr))
            *ptr = '\0';
        ptr++;
        }
    value2 = atoi(ptr);

    /* Generate a prereq for the double 'or' requirement
    */
    sprintf(message, "Need %s %lu or %s %lu", name, value, name2, value2);
    sprintf(script, "validif (#trait[%s] >= %lu)\nvalidif (#trait[%s] >= %lu)",
            attr_id, value, attr_id2, value2);
    Output_Prereq(node, message, script, info);
}


void            Output_Prereqs(T_Base_Info * info, T_List_Ids * id_list)
{
    T_Int32U            i, chunk_count;
    T_Int32S            value;
    T_Glyph_Ptr         deity_id, ptr, temp, last_class, last_race, chunks[1000];
    T_Glyph             buffer[100000], buffer2[1000], message[1000];

    if ((info->prerequisite == NULL) || (info->prerequisite[0] == '\0'))
        return;
    Output_Field(info->node, "reqText", info->prerequisite, info);

    last_class = NULL;
    last_race = NULL;
    chunk_count = x_Array_Size(chunks);
    Split_To_Array(buffer, chunks, &chunk_count, info->prerequisite, ",.;");
    for (i = 0; i < chunk_count; i++) {

        /* If this starts with "or", skip that - it's a follow-on from a
            previous thingy, so we have to parse it separately
        */
        if (strnicmp(chunks[i], "or ", 3) == 0) {
            chunks[i] += 3;
            while (x_Is_Space(*chunks[i]))
                chunks[i]++;
            }

        /* "Any class specific multiclass feat" = requires multiclass tag
        */
        if (strcmp(chunks[i], "Any class-specific multiclass feat") == 0) {
            Output_Exprreq(info->node, "Multiclass feat required", "hero.tagis[Multiclass.?] <> 0", info);
            continue;
            }

        /* "Paragon multiclassing as a <x>" = requires multiclass tag for class
            X, requires Paragon tier
        */
        if (strnicmp(chunks[i], "Paragon multiclassing as a", 26) == 0) {
            ptr = chunks[i] + 26;

            /* Output the paragon tag if we don't already have a tier tag
            */
            if (XML_Get_Named_Child_With_Attr_Count(info->node, "tag", "group", "Tier") == 0)
                Output_Tag(info->node, "Tier", "Paragon", info);

            /* Parse out the letter 'n' (for catching 'an') and any spaces,
                then find the multiclass tag for that class
                NOTE: also check the fake class list, to handle things like the
                fighter/warlord that were converted to essentials classes)
            */
            if (*ptr == 'n')
                ptr++;
            while (*ptr == ' ')
                ptr++;
            *ptr = toupper(*ptr);
            temp = Mapping_Find("class", ptr);
            if (temp == NULL)
                temp = Mapping_Find("fakeclass", ptr);
            if (temp == NULL) {
                sprintf(message, "Couldn't find class '%s' on prereq for %s\n", ptr, info->name);
                Log_Message(message);
                }
            else {
                sprintf(buffer2, "Requires %s multiclass", ptr);
                sprintf(message, "hero.tagis[Multiclass.%s] <> 0", temp);
                Output_Exprreq(info->node, buffer2, message, info);
                }
            continue;
            }

        /* Channel Divinity
        */
        if (stricmp(chunks[i], "Channel Divinity class feature") == 0) {
            Output_Tag(info->node, "User", "ReqChanDiv", info);
            continue;
            }

        /* If this chunk has 'level' in it, treat it as a level requirement -
            parse the first number out and assume that's the requirement.
        */
        if (strstr(chunks[i], "level") != NULL) {
            value = atoi(chunks[i]);
            if (value != 0)
                Output_Tag(info->node, "ReqLevel", As_Int(value), info);
            continue;
            }

        /* If this chunk starts with 'must worship' or 'worship', add it as a
            deity (if not already present) and then add a requirement for it.
            NOTE: If it just specifies "a deity of the X domain", skip it -
                the user can validate it themself.
        */
        if (strnicmp(chunks[i], "must worship", 12) == 0) {
            ptr = chunks[i] + 12;
            if (strnicmp(ptr, " a deity of the", 15) == 0)
                continue;
            deity_id = Find_Deity(ptr);
            if (deity_id != NULL)
                Output_Tag(info->node, "ReqDeity", deity_id, info);
            continue;
            }
        if (strnicmp(chunks[i], "worship", 7) == 0) {
            ptr = chunks[i] + 7;
            if (strnicmp(ptr, " a deity of the", 15) == 0)
                continue;
            deity_id = Find_Deity(ptr);
            if (deity_id != NULL)
                Output_Tag(info->node, "ReqDeity", deity_id, info);
            continue;
            }

        /* If this chunk starts with 'trained in', search for an appropriate
            skill
        */
        if (strnicmp(chunks[i], "trained in", 10) == 0) {
            ptr = chunks[i] + 10;
            while (*ptr == ' ')
                ptr++;
            ptr = Mapping_Find("skill", ptr);
            if (ptr != NULL)
                Output_Tag(info->node, "ReqSkill", ptr, info);
            else {
                sprintf(message, "No skill matched for '%s' on prereq for %s\n", chunks[i], info->name);
                Log_Message(message);
                }
            continue;
            }
        if (strnicmp(chunks[i], "training in", 11) == 0) {
            ptr = chunks[i] + 11;
            while (*ptr == ' ')
                ptr++;
            ptr = Mapping_Find("skill", ptr);
            if (ptr != NULL)
                Output_Tag(info->node, "ReqSkill", ptr, info);
            else {
                sprintf(message, "No skill matched for '%s' on prereq for %s\n", chunks[i], info->name);
                Log_Message(message);
                }
            continue;
            }

        /* If this chunk starts with 'profiency with', search for an appropriate
            weapon
        */
        if (strnicmp(chunks[i], "proficiency with", 16) == 0) {
            ptr = chunks[i] + 16;
            Find_Weapons(ptr, info, false);
            continue;
            }

        /* If this chunk ends in "role", match a role name
        */
        ptr = " role";
        value = strlen(ptr);
        if (stricmp(chunks[i] + strlen(chunks[i]) - value, ptr) == 0) {
            chunks[i][strlen(chunks[i])-value] = '\0';
            ptr = Mapping_Find("role", chunks[i]);
            if (ptr != NULL) {
                Output_Tag(info->node, "ReqRole", ptr, info);
                }
            else {
                sprintf(message, "No role matched for '%s' on prereq for %s\n", chunks[i], info->name);
                Log_Message(message);
                }
            continue;
            }

        /* If the chunk ends in "regional benefit" or "background", match it
        */
        ptr = " regional benefit";
        value = strlen(ptr);
        if (stricmp(chunks[i] + strlen(chunks[i]) - value, ptr) == 0) {
            chunks[i][strlen(chunks[i])-value] = '\0';
            Find_Background(chunks[i], info);
            continue;
            }
        ptr = " regional background";
        value = strlen(ptr);
        if (stricmp(chunks[i] + strlen(chunks[i]) - value, ptr) == 0) {
            chunks[i][strlen(chunks[i])-value] = '\0';
            Find_Background(chunks[i], info);
            continue;
            }
        ptr = " background";
        value = strlen(ptr);
        if (stricmp(chunks[i] + strlen(chunks[i]) - value, ptr) == 0) {
            chunks[i][strlen(chunks[i])-value] = '\0';
            Find_Background(chunks[i], info);
            continue;
            }

        /* If this chunk starts in "any" and ends in "class", match a power
            source
        */
        ptr = "any ";
        value = strlen(ptr);
        if (strnicmp(chunks[i], ptr, value) == 0) {
            ptr = " class";
            value = strlen(ptr);
            if (stricmp(chunks[i] + strlen(chunks[i]) - value, ptr) == 0) {
                chunks[i][strlen(chunks[i])-value] = '\0';
                ptr = Mapping_Find("powersrc", chunks[i]+4);
                if (ptr != NULL)
                    Output_Tag(info->node, "ReqPwrSrc", ptr, info);
                else {
                    sprintf(message, "No power source matched for '%s' on prereq for %s\n", chunks[i], info->name);
                    Log_Message(message);
                    }
                continue;
                }
            }

        /* If this chunk matches a race name, add a 'required race' tag
        */
        temp = Mapping_Find("race", chunks[i]);
        if (temp != NULL) {
            last_race = temp;
            Output_Tag(info->node, "ReqRace", last_race, info);
            continue;
            }

        /* If this chunk matches a class name, add a 'required class' tag
            (strip off any " class" at the end first)
        */
        ptr = " class";
        if (stricmpright(chunks[i], ptr) == 0) {
            chunks[i][strlen(chunks[i])-strlen(ptr)] = '\0';
            Find_Classes(chunks[i], info, &last_class, false);
            continue;
            }

        /* If this chunk starts with 'only a', search for an appropriate
            class as the next word
        */
        if (strnicmp(chunks[i], "only a ", 7) == 0) {
            ptr = chunks[i] + 7;
            temp = strchr(ptr, ' ');
            if (temp != NULL)
                *temp = '\0';
            Find_Classes(ptr, info, &last_class, false);
            continue;
            }

        /* If this chunk matches the first three letters of an ability score
            name (or the full name), parse out the required ability total
        */
        ptr = Mapping_Find("reqattrnam", chunks[i], true);
        if (ptr == NULL)
            ptr = Mapping_Find("reqattr", chunks[i], true);
        if (ptr != NULL) {
            Output_Attr_Requirement(info->node, ptr, chunks[i], info);
            continue;
            }

        /* If this chunk is a class feature, try to match it for the last class
            requirement we parsed
        */
        ptr = " class feature";
        if (stricmpright(chunks[i], ptr) == 0) {
            chunks[i][strlen(chunks[i])-strlen(ptr)] = '\0';
            Find_Feature(chunks[i], last_class, info, true, false);
            continue;
            }

        /* If this chunk is a racial feature or power, try to match it for the
            last race requirement we parsed
        */
        ptr = " racial feature";
        if (stricmpright(chunks[i], ptr) == 0) {
            chunks[i][strlen(chunks[i])-strlen(ptr)] = '\0';
            Find_Feature(chunks[i], last_race, info, false, false);
            continue;
            }
        ptr = " racial power";
        if (stricmpright(chunks[i], ptr) == 0) {
            chunks[i][strlen(chunks[i])-strlen(ptr)] = '\0';
            Find_Feature(chunks[i], last_race, info, false, false);
            continue;
            }

        /* If this chunk matches a feat name, add a 'required feat' exprreq
            element
        */
        ptr = " feat";
        if (stricmpright(chunks[i], ptr) == 0) {
            chunks[i][strlen(chunks[i])-strlen(ptr)] = '\0';
            Find_Feat(chunks[i], info, false);
            continue;
            }

        /* Finally, as a last gasp look for a class, race, feat or class
            feature with the exact text required, since it's probably one of
            those
        */
        if (Find_Classes(chunks[i], info, &last_class, true))
            continue;
        if (Find_Races(chunks[i], info, &last_class, true))
            continue;
        if (Find_Weapons(chunks[i], info, true))
            continue;
        if (Find_Feat(chunks[i], info, true))
            continue;
        if (Find_Feature(chunks[i], last_class, info, true, true))
            continue;

        sprintf(message, "No prereq matched for '%s' on thing %s\n", chunks[i], info->name);
        Log_Message(message);
        }
}


static bool     Check_Delete_Tag(T_XML_Node node, T_XML_Node ext_child)
{
    T_Int32S        result;
    T_Glyph_CPtr    name, group, id, test_id;
    T_XML_Node      tag_node;
    T_Glyph         buffer[500];

    /* If the extension node's name isn't the "DELETE" string, we don't have to
        delete this tag
    */
    name = XML_Get_Attribute_Pointer(ext_child, "name");
    if (strcmp(name, DELETE_TEXT) != 0)
        return(false);
    group = XML_Get_Attribute_Pointer(ext_child, "group");
    id = XML_Get_Attribute_Pointer(ext_child, "tag");

    /* Otherwise, find a tag with the same group & id on the node, and delete
        it
    */
    result = XML_Get_First_Named_Child_With_Attr(node, "tag", "group", group, &tag_node);
    while (result == 0) {
        test_id = XML_Get_Attribute_Pointer(tag_node, "tag");
        if (strcmp(id, test_id) == 0) {
            XML_Delete_Node(tag_node);
            return(true);
            }
        result = XML_Get_Next_Named_Child_With_Attr(node, &tag_node);
        }

    /* We tried to delete something we didn't have...
    */
    sprintf(buffer, "Tried to delete tag %s.%s that wasn't present.\n", group, id);
    Log_Message(buffer);
    return(true);
}


static bool     Check_Delete_Bootstrap(T_XML_Node node, T_XML_Node ext_child)
{
    T_Int32S        result;
    T_Glyph_CPtr    phase, thing;
    T_XML_Node      boot_node;
    T_Glyph         buffer[500];

    /* If the extension node's phase isn't the "DELETE" string, we don't have to
        delete this bootstrap
    */
    phase = XML_Get_Attribute_Pointer(ext_child, "phase");
    if (strcmp(phase, DELETE_TEXT) != 0)
        return(false);
    thing = XML_Get_Attribute_Pointer(ext_child, "thing");

    /* Otherwise, find a bootstrap with the same thing id on the node, and delete
        it
    */
    result = XML_Get_First_Named_Child_With_Attr(node, "bootstrap", "thing", thing, &boot_node);
    if (result == 0) {
        XML_Delete_Node(boot_node);
        return(true);
        }

    /* We tried to delete something we didn't have...
    */
    sprintf(buffer, "Tried to delete bootstrap %s that wasn't present.\n", thing);
    Log_Message(buffer);
    return(true);
}


static bool     Check_Delete_Eval(T_XML_Node node, T_Glyph_Ptr nodename, T_XML_Node ext_child)
{
    T_Int32S        result;
    T_Glyph_CPtr    phase, index;
    T_XML_Node      script_node;
    T_Glyph         buffer[500];

    /* If the extension node's phase isn't the "DELETE" string, we don't have to
        delete this script
    */
    phase = XML_Get_Attribute_Pointer(ext_child, "phase");
    if (strcmp(phase, DELETE_TEXT) != 0)
        return(false);
    index = XML_Get_Attribute_Pointer(ext_child, "index");

    /* Otherwise, find a script with the same thing index on the node, and delete
        it
    */
    result = XML_Get_First_Named_Child_With_Attr(node, nodename, "index", index, &script_node);
    if (result == 0) {
        XML_Delete_Node(script_node);
        return(true);
        }

    /* We tried to delete something we didn't have...
    */
    sprintf(buffer, "Tried to delete %s %s that wasn't present.\n", nodename, index);
    Log_Message(buffer);
    return(true);
}


static bool     Check_Delete_Fieldval(T_XML_Node node, T_XML_Node ext_child)
{
    T_Int32S        result;
    T_Glyph_CPtr    value, field;
    T_XML_Node      field_node;
    T_Glyph         buffer[500];

    /* If the extension node's value isn't the "DELETE" string, we don't have to
        delete this field
    */
    value = XML_Get_Attribute_Pointer(ext_child, "value");
    if (strcmp(value, DELETE_TEXT) != 0)
        return(false);
    field = XML_Get_Attribute_Pointer(ext_child, "field");

    /* Otherwise, find a fieldval with the same field id on the node, and delete
        it
    */
    result = XML_Get_First_Named_Child_With_Attr(node, "fieldval", "field", field, &field_node);
    if (result == 0) {
        XML_Delete_Node(field_node);
        return(true);
        }

    /* We tried to delete somefield we didn't have...
    */
    sprintf(buffer, "Tried to delete fieldval %s that wasn't present.\n", field);
    Log_Message(buffer);
    return(true);
}


static void     Get_Node_Details(E_Node_Type type, T_Glyph_Ptr * nodename, T_Glyph_Ptr * pcdata_child)
{
    *nodename = NULL;
    *pcdata_child = NULL;

    switch (type) {
        case e_node_tag :           *nodename = "tag"; return;
        case e_node_bootstrap :     *nodename = "bootstrap"; return;
        case e_node_arrayval :      *nodename = "arrayval"; return;
        case e_node_eval :          *nodename = "eval"; *pcdata_child = ""; return;
        case e_node_evalrule :      *nodename = "evalrule"; *pcdata_child = ""; return;
        case e_node_prereq :        *nodename = "prereq"; *pcdata_child = "validate"; return;
        case e_node_exprreq :       *nodename = "exprreq"; *pcdata_child = ""; return;
        case e_node_pickreq :       *nodename = "pickreq"; return;
        case e_node_containerreq :  *nodename = "containerreq"; return;
        };
}


static void     Fix_OSX_PCDATA(T_XML_Node node)
{
#ifdef _OSX
    T_Glyph_CPtr    cptr;
    T_Int32S        result;
    T_XML_Node      child;
    T_Glyph         buffer[MAX_XML_BUFFER_SIZE];
    
    cptr = XML_Get_PCDATA_Pointer(node);
    Text_Find_Replace(buffer, (T_Glyph_Ptr) cptr, "\r\n", "\n");
    XML_Set_PCDATA(node, buffer);
    
    result = XML_Get_First_Child(node, &child);
    while (result == 0) {
        Fix_OSX_PCDATA(child);
        result = XML_Get_Next_Child(node, &child);
        }
#endif
}


static void     Duplicate_Nodes(T_XML_Node node, T_XML_Node ext_node, bool is_partial,
                                E_Node_Type type, T_Copy_Vector * copies)
{
    T_Int32S        result;
    T_XML_Node      child, ext_child, pcdata_node;
    T_Glyph_CPtr    cptr;
    T_Glyph_Ptr     nodename, pcdata_child;
    T_Delayed_Copy  copy;
    T_Glyph         buffer[1000];

    Get_Node_Details(type, &nodename, &pcdata_child);
    if (x_Trap_Opt(nodename == NULL))
        return;

    result = XML_Get_First_Named_Child(ext_node, nodename, &ext_child);
    while (result == 0) {

        /* Check to see if this is actually an instruction to delete a node
        */
        if ((type == e_node_tag) && Check_Delete_Tag(node, ext_child))
            goto next_node;
        if ((type == e_node_bootstrap) && Check_Delete_Bootstrap(node, ext_child))
            goto next_node;
        if (((type == e_node_eval) || (type == e_node_evalrule)) &&
            Check_Delete_Eval(node, nodename, ext_child))
            goto next_node;

        /* If we're a bootstrap, check for our 'ignore in partial mode'
            phase and skip it if so
        */
        if ((type == e_node_bootstrap) && is_partial) {
            cptr = XML_Get_Attribute_Pointer(ext_child, "phase");
            if (strcmp(cptr, NO_PARTIAL_TEXT) == 0)
                goto next_node;
            }

        /* If we have a pcdata child node, check it for our "ignore me in
            partial mode" string
        */
        if (is_partial && (pcdata_child != NULL)) {
            if (pcdata_child[0] == '\0')
                pcdata_node = ext_child;
            else {
                result = XML_Get_First_Named_Child(ext_child, pcdata_child, &pcdata_node);
                if (x_Trap_Opt(result != 0))
                    goto next_node;
                }
            if (x_Trap_Opt(pcdata_node == NULL))
                goto next_node;
            cptr = XML_Get_PCDATA_Pointer(pcdata_node);
            if (strstr(cptr, "~ignore if partial") != NULL)
                goto next_node;
            }

        /* Create a new node in the original node to hold the copy
        */
        result = XML_Create_Child(node, nodename, &child);
        if (result != 0) {
            sprintf(buffer, "Could not create new extension document %s node.\n", nodename);
            Log_Message(buffer);
            return;
            }

        /* If we're an eval script or eval rule, check for our "copy from this
            other script here" indicator.
        */
        if (((type == e_node_eval) || (type == e_node_evalrule)) &&
            (strcmp(XML_Get_Attribute_Pointer(ext_child, "phase"), COPY_SCRIPT) == 0)) {

            /* The id to copy the script from is in the priority, and we copy
                the script with the same index as this one. We need to save the
                record to copy later, because we can't have two iterations
                through the XML root node at once.
            */
            copy.target = child;
            copy.thing_id = XML_Get_Attribute_Pointer(ext_child, "priority");
            copy.index = XML_Get_Attribute_Pointer(ext_child, "index");
            copy.nodename = nodename;
            copies->push_back(copy);
            }

        /* Otherwise just do a normal duplicate
        */
        else {
            result = XML_Duplicate_Node(ext_child, &child);
            if (result != 0) {
                sprintf(buffer, "Could not duplicate %s node from extension document.\n", nodename);
                Log_Message(buffer);
                return;
                }

            /* If we're a bootstrap, check for our 'ignore in partial mode'
                phase and unset it
            */
            if ((type == e_node_bootstrap) && (strcmp(XML_Get_Attribute_Pointer(child, "phase"), NO_PARTIAL_TEXT) == 0))
                XML_Write_Text_Attribute(child, "phase", "*");
                
            /* On OS X, we need to convert \r\n to \n in scripts or it looks
                bad
            */
            Fix_OSX_PCDATA(child);
            }

next_node:
        result = XML_Get_Next_Named_Child(ext_node, &ext_child);
        }
}


static void     Fixup_Nodes(T_XML_Node node, T_XML_Node ext_node, T_Glyph_Ptr nodename,
                            T_Glyph_Ptr attrname)
{
    T_Int32S        result;
    T_Glyph_Ptr     ptr;
    T_XML_Node      child, ext_child;
    bool            is_fieldval;
    T_Glyph         buffer[1000];

    is_fieldval = strcmp(nodename, "fieldval") == 0;

    result = XML_Get_First_Named_Child(ext_node, nodename, &ext_child);
    while (result == 0) {


        /* Check to see if this is actually an instruction to delete a node
        */
        if (is_fieldval && Check_Delete_Fieldval(node, ext_child))
            goto next_node;

        /* Create a new node in the original node to hold the copy, and
            duplicate the extension node into it
        */
        ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(ext_child, attrname);
        result = XML_Get_First_Named_Child_With_Attr(node, nodename, attrname, ptr, &child);
        if (result != 0) {
            result = XML_Create_Child(node, nodename, &child);
            if (result != 0) {
                sprintf(buffer, "Could not create new extension document %s node.", nodename);
                Log_Message(buffer);
                return;
                }
            }
        result = XML_Duplicate_Node(ext_child, &child);
        if (result != 0) {
            sprintf(buffer, "Could not duplicate %s node from extension document.", nodename);
            Log_Message(buffer);
            return;
            }
            
        /* On OS X, we need to convert \r\n to \n in scripts or it looks
            bad
        */
        Fix_OSX_PCDATA(child);

next_node:
        result = XML_Get_Next_Named_Child(ext_node, &ext_child);
        }
}


void            Append_Extensions(T_XML_Document document, T_Glyph_Ptr output_folder,
                                    T_Glyph_Ptr filename, bool is_partial)
{
    T_Int32S            result;
    T_Glyph_Ptr         ptr;
    T_XML_Document      ext_document;
    T_XML_Node          root, node, ext_root, ext_node, bootstrap, thing_node, script_node;
    T_Filename          full_name;
    vector<T_XML_Node>  list_nopartial;
    vector<T_XML_Node>::iterator    iter;
    T_Copy_Vector       script_copies;
    T_Copy_Iter         copy_iter;
    T_Glyph             buffer[1000], contents[100000];

    /* See if we have a file in our ddidownloader directory with the appropriate
        name - if not, there's nothing to do
    */
    sprintf(full_name, "%s" DIR "ddidownloader" DIR "%s", output_folder, filename);
    if (!FileSys_Does_File_Exist(full_name))
        return;

    strcpy(contents, filename+4);
    ptr = strchr(contents, '.');
    if (ptr != NULL)
        *ptr = '\0';
    ptr = strchr(contents, '_');
    if (ptr != NULL)
        *ptr = ' ';
    sprintf(buffer, "Finishing %s...", contents);
    Log_Message(buffer, true);

    /* Otherwise, read in the document
    */
    result = XML_Read_Document(&ext_document,DTD_Get_Data(), full_name);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "Could not parse XML document %s.\n%ld - %s", full_name, XML_Get_Line(),
                XML_Get_Error());
        Log_Message(buffer);
        return;
        }
    result = XML_Get_Document_Node(ext_document, &ext_root);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "Could not get root node for extension XML document %s.", full_name);
        Log_Message(buffer);
        goto cleanup_exit;
        }
    result = XML_Get_Document_Node(document, &root);
    if (x_Trap_Opt(result != 0)) {
        sprintf(buffer, "Could not get root node for output XML document %s.", full_name);
        Log_Message(buffer);
        goto cleanup_exit;
        }

    /* Go through the nodes in the document
    */
    result = XML_Get_First_Named_Child(ext_root, "thing", &ext_node);
    while (result == 0) {

        /* Look for a node in our master document with the same id - if we
            don't find one, just copy the node to the master document
        */
        ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(ext_node, "id");
        result = XML_Get_First_Named_Child_With_Attr(root, "thing", "id", ptr, &node);
        if (result != 0) {

            /* If the node has no name or compset, skip it - we can't create a
                new node without those, since that would make an invalid data
                file
            */
            if ((strlen(XML_Get_Attribute_Pointer(ext_node, "name")) == 0) ||
                (strlen(XML_Get_Attribute_Pointer(ext_node, "compset")) == 0)) {
                sprintf(buffer, "Could not find node '%s' to overwrite.", ptr);
                Log_Message(buffer);
                goto next_node;
                }

            /* Create a new node in the master document, and duplicate into it
            */
            result = XML_Create_Child(root, "thing", &node);
            if (result != 0) {
                sprintf(buffer, "Could not create new extension document node in %s.", full_name);
                Log_Message(buffer);
                goto cleanup_exit;
                }
            result = XML_Duplicate_Node(ext_node, &node);
            if (result != 0) {
                sprintf(buffer, "Could not duplicate node from extension document %s.", full_name);
                Log_Message(buffer);
                goto cleanup_exit;
                }
                
            /* On OS X, we need to convert \r\n to \n in scripts or it looks
                bad
            */
            Fix_OSX_PCDATA(node);

            /* Check for bootstraps with a 'nopartial' phase
            */
            result = XML_Get_First_Named_Child(node, "bootstrap", &bootstrap);
            list_nopartial.clear();
            while (result == 0) {
                ptr = (T_Glyph_Ptr) XML_Get_Attribute_Pointer(bootstrap, "phase");
                if (strcmp(ptr, NO_PARTIAL_TEXT) == 0)
                    list_nopartial.push_back(bootstrap);
                result = XML_Get_Next_Named_Child(node, &bootstrap);
                }

            /* If we have partial data, delete all bootstraps on our list;
                otherwise just delete the phase
            */
            for (iter = list_nopartial.begin(); iter != list_nopartial.end(); iter++) {
                if (is_partial)
                    XML_Delete_Node(*iter);
                else
                    XML_Write_Text_Attribute(*iter, "phase", "*");
                }

            goto next_node;
            }

        /* Otherwise, we need to fix up the 'real' node with anything in the
            extension - iterate through all the attributes, setting them if
            they're non-empty
        */
        result = XML_Get_First_Attribute(ext_node, buffer);
        while (result == 0) {
            XML_Get_Attribute(ext_node, buffer, contents, false);
            if (contents[0] != '\0')
                XML_Write_Text_Attribute(node, buffer, contents);
            result = XML_Get_Next_Attribute(ext_node, buffer);
            }

        /* Now add any tags and bootstraps on the extension node to the real node
        */
        Duplicate_Nodes(node, ext_node, is_partial, e_node_tag, &script_copies);
        Duplicate_Nodes(node, ext_node, is_partial, e_node_bootstrap, &script_copies);
        Duplicate_Nodes(node, ext_node, is_partial, e_node_arrayval, &script_copies);

        /* Same with eval scripts, eval rules, and pre-reqs
        */
        Duplicate_Nodes(node, ext_node, is_partial, e_node_eval, &script_copies);
        Duplicate_Nodes(node, ext_node, is_partial, e_node_evalrule, &script_copies);
        Duplicate_Nodes(node, ext_node, is_partial, e_node_prereq, &script_copies);
        Duplicate_Nodes(node, ext_node, is_partial, e_node_exprreq, &script_copies);
        Duplicate_Nodes(node, ext_node, is_partial, e_node_pickreq, &script_copies);
        Duplicate_Nodes(node, ext_node, is_partial, e_node_containerreq, &script_copies);

        /* Do the same with fields and linkages, but replace any existing value
            for the field / linkage
        */
        Fixup_Nodes(node, ext_node, "fieldval", "field");
        Fixup_Nodes(node, ext_node, "link", "linkage");

        /* Onward and upward
        */
next_node:
        result = XML_Get_Next_Named_Child(ext_root, &ext_node);
        }

    /* Now finalize any script copies
    */
    for (copy_iter = script_copies.begin(); copy_iter != script_copies.end(); copy_iter++) {
        result = XML_Get_First_Named_Child_With_Attr(ext_root, "thing", "id", copy_iter->thing_id, &thing_node);
        if (result != 0) {
            sprintf(buffer, "Could not find thing '%s' in extension document.\n", copy_iter->thing_id);
            Log_Message(buffer);
            return;
            }

        /* Now just copy the script with the appropriate index
        */
        result = XML_Get_First_Named_Child_With_Attr(thing_node, copy_iter->nodename, "index", copy_iter->index, &script_node);
        if (result != 0) {
            sprintf(buffer, "Could not find script '%s' on thing '%s' in extension document.\n", copy_iter->index, copy_iter->thing_id);
            Log_Message(buffer);
            return;
            }

        /* Now we have the right script, so duplicate it
        */
        result = XML_Duplicate_Node(script_node, &copy_iter->target);
        if (result != 0) {
            sprintf(buffer, "Could not duplicate '%s' node from extension document.\n", copy_iter->nodename);
            Log_Message(buffer);
            return;
            }
                
        /* On OS X, we need to convert \r\n to \n in scripts or it looks
            bad
        */
        Fix_OSX_PCDATA(copy_iter->target);
        }

    /* Destroy the extension document now that we're done with it
    */
cleanup_exit:
    Log_Message(" done.\n", true);
    XML_Destroy_Document(ext_document);
}


T_XML_Element * DTD_Get_Data(void)
{
    return(&l_data);
}


T_XML_Element * DTD_Get_Augmentation(void)
{
    return(&l_augmentation);
}


bool    Download_Page(T_WWW www, T_Base_Info * info, T_Glyph_Ptr url, T_Glyph_Ptr filename)
{
    static T_Int32U l_count = 0;
    static T_Int32U l_last_login = 0;
    T_Status        status;
    T_Glyph_Ptr     contents;
    bool            is_ok;
    T_Glyph         buffer[500];

    is_ok = false;
    sprintf(buffer, "Downloading %s\n", url);
    Log_Message(buffer);

    /* Sleep for a while so we don't end up DDOSing the server - this
        means we don't go as fast as we could, but it's better for the
        D&DI servers
    */
    Pause_Execution(PAGE_RETRIEVE_DELAY);

    status = WWW_Retrieve_URL(www, url, &contents, NULL);
    if (x_Trap_Opt(!x_Is_Success(status))) {
        Log_Message("Couldn't retrieve individual entry file.\n");
        goto cleanup_exit;
        }

    /* Runtime error = try again, server is hiccuping
    */
    if (strstr(contents, "<title>Runtime Error") != NULL) {
        Log_Message("Server runtime error encountered.\n");
        goto cleanup_exit;
        }

    /* Runtime error = try again, server is hiccuping
    */
    if (strstr(contents, "<title>Wizards.com server temporarily unavailable") != NULL) {
        Log_Message("Server temporarily unavailable.\n");
        goto cleanup_exit;
        }

    /* Subscribe now page = try again, server is hiccuping (unless we don't
        have a password, in which case there's no point)
    */
    if (Is_Password() && (strstr(contents, "<h1>Save Time with Every Search") != NULL)) {
        Log_Message("Subscribe page encountered unexpectedly.\n");

        /* If we've downloaded 50 pages since the last time we tried this, try
            logging in again - it's possible that the server has "forgotten"
            that we were logged in? Hopefully this will fix people when their
            downloads suddenly start failing...
        */
        if ((l_last_login + 50) <= l_count) {
            Attempt_Login_Again(www);
            l_last_login = l_count;
            }

        goto cleanup_exit;
        }

    /* Write out the file so we can peer at the raw data later - if we
        can't write it out, something bizarre is going on, so there's
        no point retrying
    */
    status = Text_Encode_To_File(filename, contents);
    if (x_Trap_Opt(!x_Is_Success(status))) {
        info->is_partial = true;
        goto cleanup_exit;
        }

    is_ok = true;

    /* Print a character to show download progress
    */
cleanup_exit:
    l_count++;
    Log_Message((T_Glyph_Ptr) (is_ok ? "." : "!"), true);
    return(is_ok);
}
