/*  FILE:   FILE.CPP

 Copyright (c) 2000-2012 by Lone Wolf Development.  All rights reserved.

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

 Implementation of file-related functions.
 */


#include    "private.h"

#include    <string>
#include    <fstream>


/*  read the contents of a text file into memory as one large string
*/
T_Glyph_Ptr Quick_Read_Text(T_Glyph_Ptr filename)
{
    std::string     contents;
    ifstream        input(filename,ios::in);
    T_Glyph_Ptr     ptr;
    T_Glyph         buffer[50000];

    /* if there was an error accessing the file, return NULL
    */
    if (!input.good())
        return(NULL);

    /* read the file contents, appending it to our string
    */
    try {
        while (!input.eof()) {
            input.read(buffer,sizeof(buffer));
            contents.append(buffer,(T_Int32U) input.gcount());
            }
        }
    catch (...) {
        return(NULL);
        }

    /* allocate storage for the total contents
    */
    ptr = new T_Glyph[strlen(contents.c_str()) + 1];
    strcpy(ptr,contents.c_str());

    return(ptr);
}


static T_Status FileSys_Delete_Files_Internal(T_Glyph_Ptr filename,
                                                 T_File_Attribute attributes,
                                                 T_Void_Ptr context)
{
    if ((attributes & e_filetype_directory) != 0)
        x_Status_Return_Success();

    FileSys_Delete_File(filename, FALSE);
    x_Status_Return_Success();
}


void        FileSys_Delete_Files(T_Glyph_Ptr wildcard)
{
    FileSys_Enumerate_Matching_Files(wildcard, FileSys_Delete_Files_Internal, NULL);
}


static T_Status FileSys_List_Files_Internal(T_Glyph_Ptr filename,
                                            T_File_Attribute attributes,
                                            T_Void_Ptr context)
{
    T_Glyph_Ptr     base_filename;

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

    printf("%s\n", base_filename);
    x_Status_Return_Success();
}


void        FileSys_List_Files(T_Glyph_Ptr wildcard)
{
    T_Status        status;

    status = FileSys_Enumerate_Matching_Files(wildcard, FileSys_List_Files_Internal, NULL);
    if (x_Trap_Opt(status == WARN_CORE_FILE_NOT_FOUND)) {
        Log_Message("<No files>\n", true);
        return;
        }
}


/*  Compare the filename against the wildcard specification and return whether
    it matches the wildcard spec or not. The wildcard spec must not include any
    directory/path information. Any directory/path in the filename is ignored.

    filename    -   filename to check against the wildcard spec
    wildspec    -   wildcard specification to compare against
    return      -   whether the filename matches the wildcard spec
*/
T_Boolean   FileSys_Is_Wildcard_Match(T_Glyph_Ptr filename,T_Glyph_Ptr wildspec)
{
    T_Boolean   is_period = FALSE;
    T_Glyph_Ptr ptr,period;

    /* skip over any directory/path info within the filename
     */
#ifdef _WIN32
    ptr = strchr(filename,':');
    if (ptr != NULL)
        filename = ptr + 1;
#endif
    ptr = strrchr(filename,DIR[0]);
    if (ptr != NULL)
        filename = ptr + 1;

    /* identify the LAST period in the filename string; since the name can
        contain multiple periods, only the last one indicates the file extension
    */
    period = strrchr(filename,'.');

    /* step through both strings, processing the match on the way
    */
    while (1) {

        /* if we've hit the end of the filename string, make sure that there
            is nothing further required from the wildcard string
        */
        if (*filename == '\0') {
            for ( ; *wildspec != '\0'; wildspec++)
                if ((*wildspec != '.') && (*wildspec != '?') && (*wildspec != '*'))
                    return(FALSE);
            return(TRUE);
            }

        /* based on the wildcard string, check the filename string
        */
        switch (*wildspec) {

            /* if we hit the end and haven't hit the end of the filename string,
                they definitely don't match
            */
            case '\0' :
                return(FALSE);

            /* for a '?', match any single character
            */
            case '?' :
                filename++;
                wildspec++;
                break;

            /* on an '*', if we've hit the period already, we've got a match;
                otherwise, match everything up to the file extension period
            */
            case '*' :
                if (is_period)
                    return(TRUE);
                else {
                    filename = (period != NULL) ? period : filename + strlen(filename);
                    while ((*wildspec != '.') && (*wildspec != '\0'))
                        wildspec++;
                    }
                break;

            /* on our file extension transition period, set the period flag;
                always drop through for a literal match check
            */
            case '.' :
                if (filename == period)
                    is_period = TRUE;
                /* FALL THROUGH */

            /* on any other character, the match must be literal
            */
            default :
                if (tolower(*filename) != tolower(*wildspec))
                    return(FALSE);
                filename++;
                wildspec++;
                break;
            }
        }
}


bool        FileSys_Does_Folder_Exist(T_Glyph_CPtr filename)
{
    T_Status            status;
    T_File_Attribute    attr;

    /* If the file attributes have our 'directory' flag set, the folder exists.
    */
    status = FileSys_Get_File_Attributes(filename, &attr);
    if (!x_Is_Success(status))
        return(false);
    if ((attr & e_filetype_directory) == 0)
        return(false);
    return(true);
}
