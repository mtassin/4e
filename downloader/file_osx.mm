/*  FILE:   FILE_OSX.CPP

 Copyright (c) 2012 by Lone Wolf Development.  All rights reserved.

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

#include    "platform_osx.h"

#include    <string>
#include    <fstream>


/*  define internal error codes
*/
#define WARN_CORE_CALLBACK_ABORTED_ENUMERATE    0x100
#define WARN_CORE_FILE_ALREADY_EXISTS           0x101
#define WARN_CORE_PREMATURE_COMPLETION          0x102

#define ERR_CORE_INVALID_INBOUND_PARAM          0x110
#define ERR_CORE_INVALID_RETURN_PARAM           0x111
#define ERR_CORE_INVALID_OBJECT_PARAM           0x112

#define ERR_CORE_FILE_COPY_FAILED               0x120
#define ERR_CORE_FILE_DOES_NOT_EXIST            0x121
#define ERR_CORE_FILE_CREATE_FAILED             0x122
#define ERR_CORE_FILE_DELETE_FAILED             0x123
#define ERR_CORE_FILE_ALREADY_EXISTS            0x124
#define ERR_CORE_FILE_WRITE_FAILED              0x125


/*  define private constants used by this file
*/
#define SIGNATURE           0x99887766


/*  define the options available when creating/opening a file
*/
typedef enum {
    e_open_read =               0x0001, /* access for opening client */
    e_open_write =              0x0002,
    e_open_read_write =         (e_open_read | e_open_write),

    e_open_share_deny_read =    0x0010, /* access for other clients */
    e_open_share_deny_write =   0x0020,
    e_open_exclusive =          (e_open_share_deny_read | e_open_share_deny_write),

    e_open_binary =             0x0000  /* type of file contents */
    /* NOTE! Do NOT define a "text" mode option. The underlying Windows API
                operates on binary files ONLY and doesn't handle the proper
                automatic translations of newlines associated with text files.
    */
    }   T_File_Open_Modes;


/*  define the internal structure of a file object
*/
typedef struct T_File_ *                T_File;
typedef struct T_File_ {
    T_Int32U            signature;  /* unique signature of a file object */
    T_File_Open_Modes   open_mode;  /* mode in which file was opened */
    FILE               *handle;     /* internal handle to open file */
    }   T_File_Body, * T_File;


static T_Status File_Create(const T_Glyph_Ptr filename, T_File_Open_Modes open_mode,
                            T_File * file_handle)
{
    T_File          new_file;
    T_Status        status;
    FILE           *handle;
    T_Glyph         access_string[10];
    int             pos;

    /* validate the return value
    */
    if (x_Trap_Opt(file_handle == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
        }
    *file_handle = NULL;

    /* verify that the file does not already exist
    */
    if (x_Trap_Opt(FileSys_Does_File_Exist(filename))) {
        x_Status_Return(ERR_CORE_FILE_ALREADY_EXISTS);
        }

    /* create the file object
    */
    status = Mem_Acquire(sizeof(T_File_Body), (T_Void_Ptr *) &new_file);
    if (x_Trap_Opt(!x_Is_Success(status))) {
        x_Status_Return(status);
        }

    /* determine the internal priveleges with which to open the file
    */
    new_file->open_mode = open_mode;
    pos = 0;
    access_string[pos++] = 'w';
    if (open_mode & e_open_read)
        access_string[pos++] = '+';
    access_string[pos++] = '\0';

    /* open the file
    */
    handle = fopen(filename, access_string);
    if (handle == NULL) {
        Mem_Release(new_file);
        x_Status_Return(ERR_CORE_FILE_CREATE_FAILED);
        }

    /* report the created file handle
    */
    new_file->signature = SIGNATURE;
    new_file->handle = handle;
    *file_handle = new_file;
    x_Status_Return_Success();
}


/* ***************************************************************************
    File_Write

    Write a block of data to an open file. The number of bytes to write is
    specified, along with the buffer containing the data to be written. Once
    the transfer is complete, the actual number of bytes transferred is
    returned.

    file_handle --> handle to file to write data into
    buffer      --> data to be written out to the file
    count       --> requested number of bytes to write to the file
    actual      <-- actual number of bytes written to the file
    return      <-- whether data was written to the file successfully
**************************************************************************** */

static T_Status     File_Write(T_File file_handle,
                                        const T_Void_Ptr buffer,T_Int32U count,
                                        T_Int32U * actual)
{
    /* validate the parameters
    */
    if (x_Trap_Opt((file_handle == NULL) || (file_handle->signature != SIGNATURE))) {
        x_Status_Return(ERR_CORE_INVALID_OBJECT_PARAM);
        }
    if (x_Trap_Opt(buffer == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }
    if (x_Trap_Opt(actual == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
        }

    /* write the entire buffer
    */
    *actual = fwrite(buffer, 1, count, file_handle->handle);
    if ((*actual) == 0) {
        x_Status_Return(ERR_CORE_FILE_WRITE_FAILED);
        }

    /* if we got here, the data was written - report a warning if we didn't
        successfully write everything we asked for
    */
    if (*actual != count) {
        x_Status_Return(WARN_CORE_PREMATURE_COMPLETION);
        }
    x_Status_Return_Success();
}


/* ***************************************************************************
    File_Close

    Close an open file that is identified by the specified file handle.

    file_handle --> handle that identifies the open file to close
    return      <-- whether the file was closed successfully
**************************************************************************** */

static T_Status     File_Close(T_File file_handle)
{
    /* validate the file object
    */
    if (x_Trap_Opt((file_handle == NULL) ||(file_handle->signature != SIGNATURE))) {
        x_Status_Return(ERR_CORE_INVALID_OBJECT_PARAM);
        }

    /* close the file
    */
    fclose(file_handle->handle);

    /* destroy the file object
    */
    file_handle->signature = 0;
    Mem_Release(file_handle);
    x_Status_Return_Success();
}


/*  Write the specified text out to the given file. Note that Windows can use a
    simpler version of this, but the mac can't because xcode 4 has a bug
    that means it doesn't work on leopard (OS X 10.5). If we stop supporting
    10.5, we can just use the windows version for everything.
*/
T_Status    Quick_Write_Text(T_Glyph_Ptr filename,T_Glyph_Ptr text)
{
    T_Status        status;
    T_Int32U        length, actual;
    T_Glyph_Ptr     name, output;
    T_File          file;
    
    /* The lower level APIs don't claim to be ok with const stuff, but they
        are - temporary fix here so we don't have to do 1000 casts below
    */
    name = (T_Glyph_Ptr) filename;
    output = (T_Glyph_Ptr) text;
    
    if (FileSys_Does_File_Exist(name)) {
        status = FileSys_Delete_File(name, FALSE);
        if (x_Trap_Opt(!x_Is_Success(status)))
            x_Status_Return(status);
        }
        
    status = File_Create(name, e_open_write, &file);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);
        
    length = strlen(output);
    status = File_Write(file, output, length, &actual);
    if (x_Trap_Opt(!x_Is_Success(status)))
        goto cleanup_exit;
    if (x_Trap_Opt(length != actual)) {
        status = LWD_ERROR;
        goto cleanup_exit;
        }
    
cleanup_exit:    
    File_Close(file);
    
    x_Status_Return(status);
}


static T_Status FileSys_Does_File_Exist_Internal(T_Glyph_Ptr filename,
                                                 T_File_Attribute attributes,
                                                 T_Void_Ptr context)
{
    // If we're called back, at least one function matched. If one
    // function matched, then this will abort the enumeration. If none
    // exist, the enumeration will be "successful".
    *((T_File_Attribute*)context) = attributes;
    x_Status_Return(WARN_CORE_CALLBACK_ABORTED_ENUMERATE);
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
    /* To support wildcards, call the search function
     */
    T_File_Attribute attributes;
    if (FileSys_Enumerate_Matching_Files(filename, FileSys_Does_File_Exist_Internal, &attributes) != WARN_CORE_CALLBACK_ABORTED_ENUMERATE)
        return false;
    return !(attributes & e_filetype_directory);
}


// Function to convert a C String into a Foundation/Cocoa string
static NSString* GetFilename(T_Glyph_CPtr filename)
{
    return [NSString stringWithCString:filename encoding:NS_STRING_ENCODING];
}

// Function to map an OSX error into a T_Status
static T_Status MapError(NSError *error, T_Status fallback)
{
    switch ([error code])
    {
        case NSFileNoSuchFileError:
            return ERR_CORE_FILE_DOES_NOT_EXIST;
            /* etc. */
        default:
            return fallback;
    }
}

T_Status    FileSys_Copy_File(const T_Glyph_Ptr new_name,
                                    const T_Glyph_Ptr old_name,T_Boolean is_force)
{
    CollectGarbageHelper helper;
    NSFileManager *manager;
    NSString *from, *to;
    NSError *error;
    T_Status result;

    /* validate the parameters
     */
    if (x_Trap_Opt((old_name == NULL) || (new_name == NULL))) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
    }

    from = GetFilename(old_name);
    if (from == NULL)
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
    to = GetFilename(new_name);
    if (to == NULL)
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);

    manager = [NSFileManager defaultManager];
    /* The copy function always fails if the destination exists, so to emulate
     * Windows, scrub the file if it's writable (if it's forced, it doesn't matter)
     */
    if (is_force || [manager isWritableFileAtPath:to])
    {
        if ([manager respondsToSelector:@selector(removeItemAtPath:error:)])
            [manager removeItemAtPath:to error:NULL];
        else
            [manager removeFileAtPath:to handler:nil];
    }

    /* Actually copy it
     */
    if ([manager respondsToSelector:@selector(copyItemAtPath:toPath:error:)])
    {
        if ([manager copyItemAtPath:from toPath:to error:&error])
            result = SUCCESS;
        else
            result = MapError(error, ERR_CORE_FILE_COPY_FAILED);
    }
    else
    {
        if ([manager copyPath:from toPath:to handler:nil])
            result = SUCCESS;
        else
            result = ERR_CORE_FILE_COPY_FAILED;
    }
    x_Status_Return(result);
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
    CollectGarbageHelper helper;
    T_Status            status = WARN_CORE_FILE_NOT_FOUND;
    NSString           *inputString;
    NSString           *searchPath;
    NSString           *searchString;
    NSArray            *dirContents;
    T_File_Attribute    attribs;
    T_Glyph_Ptr         searchPathCString;
    T_Glyph_Ptr         fullpathCString;
    int                 i, j;

    inputString = GetFilename(filespec);

    /* Get search path
     */
    searchPath = [inputString stringByDeletingLastPathComponent];
    if ([searchPath length] == 0)
        searchPath = @".";

    /* Get search string
     */
    searchString = [inputString lastPathComponent];
    searchPathCString = (T_Glyph_Ptr)[searchString cStringUsingEncoding:NS_STRING_ENCODING];

    /* Do search
     */
    dirContents = [[NSFileManager defaultManager] directoryContentsAtPath:searchPath];
    if (dirContents == nil)
        x_Status_Return(WARN_CORE_FILE_NOT_FOUND);

    /* Run through results
     */
    for (i = 0, j = [dirContents count]; i < j; i++)
    {
        /* Use our own wildcard function to match filenames
         */
        NSString *filename = [dirContents objectAtIndex:i];
        if (FileSys_Is_Wildcard_Match((T_Glyph_Ptr)[filename cStringUsingEncoding:NS_STRING_ENCODING], searchPathCString))
        {
            NSString *fullpath = [searchPath stringByAppendingPathComponent:filename];
            fullpathCString = (T_Glyph_Ptr)[fullpath cStringUsingEncoding:NS_STRING_ENCODING];

            /* File matched, so get attributes and call callback
             */

            status = FileSys_Get_File_Attributes(fullpathCString, &attribs);
            if (!x_Is_Success(status))
                continue;

            status = enum_func(fullpathCString, attribs, context);
            if (!x_Is_Success(status))
            {
                status = WARN_CORE_CALLBACK_ABORTED_ENUMERATE;
                break;
            }
        }
    }
    x_Status_Return(status);
}


void        FileSys_Get_Temporary_Folder(T_Glyph_Ptr buffer)
{
    CollectGarbageHelper    helper;
    NSString *              path;

    /* Make sure we end with a directory separator character
    */
    path = NSTemporaryDirectory();
    if ([path characterAtIndex:[path length] - 1] != '/')
        path = [path stringByAppendingString:@"/"];
    [path getCString:buffer maxLength:MAX_FILE_NAME encoding:NS_STRING_ENCODING];
}


/* ***************************************************************************
    FileSys_Create_Directory

    Create the directory specified by the given explicit filename.

    filename    --> filename indicating the new directory to create
    return      <-- whether the directory was created successfully
**************************************************************************** */

T_Status        FileSys_Create_Directory(T_Glyph_CPtr filename)
{
    CollectGarbageHelper helper;
    NSString *dirName;
    NSFileManager *manager;
    NSError *error;
    T_Status result;

    /* validate the parameters
    */
    if (x_Trap_Opt(filename == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }

    dirName = GetFilename(filename);
    if (dirName == NULL)
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);

    manager = [NSFileManager defaultManager];

    /* find out if the directory exists
     */
    if ([manager fileExistsAtPath:dirName])
        x_Status_Return(WARN_CORE_FILE_ALREADY_EXISTS);

    /* create the directory
    */
    if ([manager respondsToSelector:@selector(createDirectoryAtPath:withIntermediateDirectories:attributes:error:)])
    {
        if ([manager createDirectoryAtPath:dirName withIntermediateDirectories:NO attributes:nil error:&error])
            result = SUCCESS;
        else
            result = MapError(error, ERR_CORE_FILE_CREATE_FAILED);
    }
    else
    {
        if ([manager createDirectoryAtPath:dirName attributes:nil])
            result = SUCCESS;
        else
            result = ERR_CORE_FILE_CREATE_FAILED;
    }

    x_Status_Return(result);
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
    CollectGarbageHelper helper;
    NSFileManager *manager;
    NSString *nsFilename;
    T_Status result;

    /* validate the parameters
    */
    if (x_Trap_Opt(filename == NULL)) {
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);
        }

    nsFilename = GetFilename(filename);
    if (nsFilename == NULL)
        x_Status_Return(ERR_CORE_INVALID_INBOUND_PARAM);

    /* Delete file
     */
    manager = [NSFileManager defaultManager];
    if ([manager respondsToSelector:@selector(removeItemAtPath:error:)])
    {
        NSError *nsError;

        if ([manager removeItemAtPath:nsFilename error:&nsError])
            result = SUCCESS;
        else
            result = MapError(nsError, ERR_CORE_FILE_DELETE_FAILED);
    }
    else
    {
        if ([manager removeFileAtPath:nsFilename handler:nil])
            result = SUCCESS;
        else
            result = ERR_CORE_FILE_DELETE_FAILED;
    }
    x_Status_Return(result);
}


bool            FileSys_Verify_Write_Privileges(T_Glyph_Ptr folder)
{
    /* I'm not sure this is even a thing we need to worry about on Mac OS? We
        check on windows because we might need administrator privileges or
        something silly like that, but I don't think that will be an issue on
        Mac OS as everything should always be under the user's home directory
    */
    return(true);
}



T_Glyph_Ptr     FileSys_Get_Current_Directory(T_Glyph_Ptr buffer)
{
    char *          ptr;

    buffer[0] = '\0';
    ptr = getcwd(buffer, MAX_FILE_NAME);
    x_Trap_Opt(ptr == NULL);
    return(buffer);
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
    CollectGarbageHelper helper;
    NSFileManager *manager = [NSFileManager defaultManager];
    NSString *nsFilename;
    BOOL isDirectory;
    int value;

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

    /* retrieve the information for the file
    */
    // Could use stat here. Instead, just be lazy.
    nsFilename = GetFilename(filename);
    if (![manager fileExistsAtPath:nsFilename isDirectory:&isDirectory])
    {
        x_Status_Return(ERR_CORE_FILE_DOES_NOT_EXIST);
    }

    /* map the file information to the appropriate attributes
     */
    value = e_filetype_normal;
    if (isDirectory)
        value |= e_filetype_directory;
    if (![manager isWritableFileAtPath:nsFilename])
        value |= e_filetype_read_only;
    if ([[nsFilename lastPathComponent] characterAtIndex:0] == '.') // Also a flag?
        value |= e_filetype_hidden;
    *attribute = (T_File_Attribute)value;

    x_Status_Return_Success();
}
