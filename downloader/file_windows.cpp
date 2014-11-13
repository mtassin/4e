/*  FILE:   FILE_WINDOWS.CPP

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

    Implementation of file-related functions.
*/


#include    "private.h"

#include    <windows.h>

#include    <string>
#include    <fstream>


/*  define internal error codes
*/
#define WARN_CORE_CALLBACK_ABORTED_ENUMERATE    0x100
#define WARN_CORE_FILE_ALREADY_EXISTS           0x101

#define ERR_CORE_INVALID_INBOUND_PARAM          0x110
#define ERR_CORE_INVALID_RETURN_PARAM           0x111

#define ERR_CORE_FILE_COPY_FAILED               0x120
#define ERR_CORE_FILE_DOES_NOT_EXIST            0x121
#define ERR_CORE_FILE_CREATE_FAILED             0x122
#define ERR_CORE_FILE_DELETE_FAILED             0x123


/*  Write the specified text out to the given file using a simple ofstream. We
    can't do things this easily on the mac version because xcode 4 has a bug
    that means it doesn't work on leopard (OS X 10.5). If we stop supporting
    10.5, we can just use this version for everything.
*/
T_Status    Quick_Write_Text(T_Glyph_Ptr filename,T_Glyph_Ptr text)
{
    ofstream    output(filename,ios::out);

    /* if there was an error accessing the file, report an error
    */
    if (!output.good())
        x_Status_Return(LWD_ERROR);

    /* write the buffer out to the file
    */
    try {
        output << text;
        }
    catch (...) {
        x_Status_Return(LWD_ERROR);
        }

    x_Status_Return(output.good() ? SUCCESS : LWD_ERROR);
}


/* ***************************************************************************
    FileSys_Does_File_Exist

    Determine whether the specified filename actually points to a live, existing
    file (or files if wildcards are used) on the system. If a file is found that
    matches the given filename, TRUE is returned. The filename can include
    wildcards, in which case ANY file that satisfies the criteria indicated will
    result in a return of TRUE.

    filename    --> filename to verify for a corresponding, existing file
    return      <-- whether a matching file exists for the filename
**************************************************************************** */

bool        FileSys_Does_File_Exist(const T_Glyph_Ptr filename)
{
    WIN32_FIND_DATA     info;
    HANDLE              handle;
    T_Glyph_Ptr         ptr;
    T_Glyph             buffer[10];

    /* validate the paraneters
     */
    if (x_Trap_Opt(filename == NULL)) {
        return(false);
    }

    /* If filename isn't of the form 'X:\', we can just use it as-is.
     */
    if ((filename[3] != '\0') || !x_Is_Alpha(filename[0]) || (filename[1] != ':') || (filename[2] != '\\'))
        ptr = filename;

    /* Otherwise, we need to append "*.*" to it to make the FindFirstFile call
     complete correctly.
     */
    else {
        strcpy(buffer, filename);
        strcat(buffer, "*.*");
        ptr = buffer;
    }

    /* find out if the file exists
     */
    handle = FindFirstFile(ptr,&info);
    if (handle == INVALID_HANDLE_VALUE) {
        return(false);
    }
    FindClose(handle);
    return(true);
}


T_Status    FileSys_Copy_File(const T_Glyph_Ptr new_name,
                                    const T_Glyph_Ptr old_name,T_Boolean is_force)
{
    WIN32_FIND_DATA     info;
    HANDLE              handle;
    BOOL                is_ok;
    DWORD               new_attr;

    /* validate the parameters
     */
    if (x_Trap_Opt((old_name == NULL) || (new_name == NULL))) {
        x_Status_Return(-1);
    }

    /* find out if the source file exists
     */
    handle = FindFirstFile(old_name,&info);
    FindClose(handle);
    if (handle == INVALID_HANDLE_VALUE) {
        x_Status_Return(-1);
    }

    /* If the 'force' flag has been specified, and if the target file exists,
     unset the read-only flag if it's set.
     */
    if (is_force) {
        handle = FindFirstFile(new_name,&info);
        if ((handle != INVALID_HANDLE_VALUE) && (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
            new_attr = info.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY;
            is_ok = SetFileAttributes(new_name, new_attr);
            x_Trap_Opt(!is_ok);
        }
        FindClose(handle);
    }

    /* copy the file, overwriting any existing file
     */
    if (!CopyFile(old_name,new_name,FALSE)) {
        x_Status_Return(-1);
    }
    x_Status_Return_Success();
}


/* ---------------------------------------------------------------------------
    Translate_Attributes

    Translate the attributes from the native platform to the generic equivalent.

    attributes  --> platform-specific information about the file
    return      <-- translated attributes for the file
---------------------------------------------------------------------------- */

static  T_File_Attribute    Translate_Attributes(DWORD attributes)
{
    T_File_Attribute    attribs;

    attribs = e_filetype_normal;
    if ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
        attribs = (T_File_Attribute) (attribs | e_filetype_directory);
    if ((attributes & FILE_ATTRIBUTE_READONLY) != 0)
        attribs = (T_File_Attribute) (attribs | e_filetype_read_only);
    if ((attributes & FILE_ATTRIBUTE_SYSTEM) != 0)
        attribs = (T_File_Attribute) (attribs | e_filetype_system);
    if ((attributes & FILE_ATTRIBUTE_HIDDEN) != 0)
        attribs = (T_File_Attribute) (attribs | e_filetype_hidden);
    return(attribs);
}


static  T_File_Attribute    Translate_Attributes_From_Find(WIN32_FIND_DATA * info)
{
    T_File_Attribute    attribs;
    attribs = Translate_Attributes(info->dwFileAttributes);
    return(attribs);
}


/* ***************************************************************************
 FileSys_Enumerate_Matching_Files

 This function will enumerate the list of files which match the specified
 file specification, invoking the indicated callback function for each
 matching file or sub-directory. The callback function is of type
 T_Fn_File_Enum and receives two parameters - the matching filename and the
 context provided by the caller. The return value from the callback function
 must indicate SUCCESS to continue the enumeration. If a non-success value is
 returned, the enumeration stops immediately and a user-abort warning is
 returned by this function. To iterate down through a directory hierarchy,
 this function may be called recursively from within the callback function.

 filespec    --> filename defining the set of files to enumerate over
 enum_func   --> callback function to invoke for each matching file
 context     --> client parameter passed into enumeration function
 return      <-- whether the enumeration was performed successfully
 **************************************************************************** */

T_Status    FileSys_Enumerate_Matching_Files(T_Glyph_Ptr filespec,
                                             T_Fn_File_Enum enum_func,
                                             T_Void_Ptr context)
{
    WIN32_FIND_DATA     info;
    HANDLE              handle;
    BOOL                is_ok;
    T_Status            status = WARN_CORE_FILE_NOT_FOUND;
    T_File_Attribute    attribs;
    T_Glyph             name[MAX_FILE_NAME + 1];
    T_Glyph_Ptr         ptr;

    /* validate the parameters
    */
    if (x_Trap_Opt((filespec == NULL) || (enum_func == NULL))) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }

    /* carve out the proper path portion of the search filespec
    */
    strcpy(name,filespec);
    ptr = strrchr(name,'\\');
    if (ptr == NULL) {
        ptr = strrchr(name,':');
        if (ptr == NULL)
            ptr = name - 1;
        }
    ptr++;

    /* initiate a search through all files that match the filespec
    */
    handle = FindFirstFile(filespec,&info);
    is_ok = (handle != INVALID_HANDLE_VALUE);

    /* loop through every file that matches the filespec, invoking the
        callback for every file
    */
    while (is_ok) {

        /* if the file is either the "." or ".." directory entries, skip it
            and get the next file in the list
        */
        if (((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) &&
            ((strcmp(info.cFileName,".") == 0) ||
             (strcmp(info.cFileName,"..") == 0))) {
            is_ok = FindNextFile(handle,&info);
            continue;
            }

        /* map the file attributes to the portable set
        */
        attribs = Translate_Attributes_From_Find(&info);

        /* invoke the callback function for the file
        */
        strcpy(ptr,info.cFileName);
        status = enum_func(name,attribs,context);
        if (!x_Is_Success(status)) {
            status = WARN_CORE_CALLBACK_ABORTED_ENUMERATE;
            break;
            }

        /* find out if there are still more files to process
        */
        is_ok = FindNextFile(handle,&info);
        }

    /* everything is completed, so destroy our handle and then return the
        appropriate status code
    */
    if (handle != INVALID_HANDLE_VALUE) {
        FindClose(handle);
        }
    x_Status_Return(status);
}


void        FileSys_Get_Temporary_Folder(T_Glyph_Ptr buffer)
{
    GetTempPath(MAX_FILE_NAME,buffer);
}


/* ***************************************************************************
    FileSys_Create_Directory

    Create the directory specified by the given explicit filename.

    filename    --> filename indicating the new directory to create
    return      <-- whether the directory was created successfully
**************************************************************************** */

T_Status        FileSys_Create_Directory(T_Glyph_CPtr filename)
{
    /* validate the parameters
    */
    if (x_Trap_Opt(filename == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }

    /* find out if the directory exists
    */
    if (FileSys_Does_Folder_Exist(filename)) {
        x_Status_Return(WARN_CORE_FILE_ALREADY_EXISTS);
        }

    /* create the directory
    */
    if (CreateDirectory(filename,NULL) == 0) {
        x_Status_Return(ERR_CORE_FILE_CREATE_FAILED);
        }
    x_Status_Return_Success();
}


/* ***************************************************************************
    FileSys_Delete_File

    Delete the file specified by the given filename. If the file does not exist,
    cannot be deleted, or refers to multiple files, an error is returned. The
    filename must specify a single, explicit file - wildcards cannot be used.

    filename    --> filename indicating the file to be deleted
    is_force    --> whether to delete the file if it's read-only, system or hidden
    return      <-- whether the file was deleted successfully
**************************************************************************** */

T_Status        FileSys_Delete_File(T_Glyph_CPtr filename,T_Boolean is_force)
{
    WIN32_FIND_DATA     info;
    HANDLE              handle;
    BOOL                is_ok;
    DWORD               new_attr;

    /* validate the parameters
    */
    if (x_Trap_Opt(filename == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }

    /* find out if the file exists
    */
    handle = FindFirstFile(filename,&info);
    FindClose(handle);
    if (handle == INVALID_HANDLE_VALUE) {
        x_Status_Return(ERR_CORE_FILE_DOES_NOT_EXIST);
        }

    /* If the 'force' flag has been specified, unset the read-only flag if it's
        set.
    */
    if (is_force && (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
        new_attr = info.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY;
        is_ok = SetFileAttributes(filename, new_attr);
        x_Trap_Opt(!is_ok);
        }

    /* delete the file
    */
    is_ok = DeleteFile(filename);
    if (!is_ok) {
        x_Status_Return(ERR_CORE_FILE_DELETE_FAILED);
        }
    x_Status_Return_Success();
}


T_Glyph_Ptr     FileSys_Get_Current_Directory(T_Glyph_Ptr buffer)
{
    DWORD           result;

    buffer[0] = '\0';
    result = GetCurrentDirectory(MAX_FILE_NAME,buffer);
    x_Trap_Opt(result == 0);
    return(buffer);
}


bool        FileSys_Verify_Write_Privileges(T_Glyph_Ptr folder)
{
    T_Filename      filename;
    HANDLE          handle;

    strcpy(filename, folder);
    strcat(filename, "\\definition.def");
    if (x_Trap_Opt(!FileSys_Does_File_Exist(filename)))
        return(true);

    /* If we can't open the definition file for writing, we have a problem
    */
    handle = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        Log_Message("Error!\n\nYou must run the D&DI Downloader as an Administrator from its current location.\n\nTo run the downloader as an Administrator, you can right-click the program's icon and choose \"Run as administrator\", or modify the shortcut properties so it is always run as an Administrator.", true);
        return(false);
        }

    CloseHandle(handle);
    return(true);
}


/* ***************************************************************************
    FileSys_Get_File_Attributes

    Retrieve the attributes of the file specified by the explicit filename.

    filename    --> filename indicating the file to retrieve attributes for
    attributes  <-- attributes of the file specified
    return      <-- whether the attributes were retrieved successfully
**************************************************************************** */

T_Status    FileSys_Get_File_Attributes(T_Glyph_CPtr filename,
                                            T_File_Attribute * attribute)
{
    DWORD               attributes;

    /* validate the parameters
    */
    if (x_Trap_Opt(filename == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }
    if (x_Trap_Opt(attribute == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
        }

    /* make sure there are no wildcards in the filename
    */
    if (x_Trap_Opt((strchr(filename,'*') != NULL) || (strchr(filename,'?') != NULL))) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }

    /* If we get a string of the form 'X:' with no trailing backslash, trap an
        error; it'll work anyway, but we really should have a trailing \ on the
        end.
    */
    x_Trap_Opt((filename[2] == '\0') && x_Is_Alpha(filename[0]) && (filename[1] != ':'));

    /* retrieve the information for the file
    */
    attributes = GetFileAttributes(filename);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        x_Status_Return(ERR_CORE_FILE_DOES_NOT_EXIST);
        }

    /* map the file information to the appropriate attributes
    */
    *attribute = Translate_Attributes(attributes);
    x_Status_Return_Success();
}