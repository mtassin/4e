/*  FILE:   WWW_WINDOWS.CPP

    Copyright (c) 1999-2012 by Lone Wolf Development.  All rights reserved.

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

    Internet API handling for the Win32 platform.
*/


#include "private.h"

#include <windows.h>
#include <wininet.h>
#include <tchar.h>
#include <ras.h>

#ifdef  x_Trace
#undef  x_Trace
#define x_Trace     (void)
#endif

/*  define private constants used by this source file
*/
#define SIGNATURE       0x496e6574      /* Inet */

#define START_SIZE      100000
#define GROW_SIZE       100000

#define CONNECT_TIMEOUT 30000
#define RECEIVE_TIMEOUT 30000

/*  define error codes
*/
#define WARN_CORE_INTERNET_SIZE_MISMATCH        0x50

#define ERR_CORE_INVALID_RETURN_PARAM           0x60
#define ERR_CORE_INVALID_INBOUND_PARAM          0x61
#define ERR_CORE_INVALID_OBJECT_PARAM           0x62

#define ERR_CORE_INTERNET_OPEN_FAILED           0x70
#define ERR_CORE_INTERNET_INVALID_SERVER        0x71
#define ERR_CORE_INTERNET_CLOSE_FAILED          0x72
#define ERR_CODE_INTERNET_URL_ACCESS            0x73
#define ERR_CODE_INTERNET_URL_RESOLUTION        0x74
#define ERR_CORE_INTERNET_URL_RETRIEVE          0x75
#define ERR_CORE_INTERNET_URL_CLOSE             0x76
#define ERR_CORE_INTERNET_CANCEL                0x77
#define ERR_CORE_INTERNET_FTP_CONNECT           0x78
#define ERR_CORE_INTERNET_FTP_TRANSFER          0x79
#define ERR_CORE_INTERNET_CONFIGURE             0x7a
#define ERR_CORE_INTERNET_HEADERS               0x7b


/*  define the structure of an internet object
*/
typedef struct T_WWW_ {
    T_Int32U        signature;
    HINTERNET       internet;
    HINTERNET       connect;
    T_Glyph         server[200];
    T_Glyph_Ptr     url_contents;
    T_Int32U        max_size;
    T_Int32U        current_size;
    T_Int32U        connect_timeout;
    T_Int32U        receive_timeout;
    }   T_WWW_Body, * T_WWW;


/*  declare static variables used below
*/
static  T_Glyph_Ptr     l_hex_digits = "0123456789ABCDEF";


/* ---------------------------------------------------------------------------
    Get_Server_Name

    Retreive the server name from the URL provided. Return whether the URL
    contains a valid server name.

    server      <-- extracted server name from the URL
    url         --> URL to extract the server name from
    return      <-- whether the server name was successfully extracted
---------------------------------------------------------------------------- */

static  T_Boolean   Get_Server_Name(T_Glyph_Ptr server,T_Glyph_Ptr url,
                                        T_Boolean * is_secure)
{
    T_Glyph_Ptr     src,dst;
    T_Int32U        length;

    /* if this is not an HTTP or FTP URL, return that no server was found
    */
    *is_secure = FALSE;
    if (_strnicmp(url,"http://",7) == 0)
        length = 7;
    else if (_strnicmp(url,"https://",8) == 0) {
        length = 8;
        *is_secure = TRUE;
        }
    else if (_strnicmp(url,"ftp://",6) == 0)
        length = 6;
    else
        return(FALSE);

    /* extract the server component from the URL
    */
    src = url + length;
    dst = server;
    while ((*src != '/') && (*src != '\\') && (*src != '\0'))
        *dst++ = *src++;
    *dst = '\0';
    return(TRUE);
}


T_Status    WWW_Close_Server(T_WWW internet)
{
    BOOL    is_ok;

    /* validate the parameter
    */
    if (x_Trap_Opt((internet == NULL) || (internet->signature != SIGNATURE)))
        x_Status_Return(ERR_CORE_INVALID_OBJECT_PARAM);

    /* close the connection
    */
    is_ok = InternetCloseHandle(internet->internet);
    if (x_Trap_Opt(!is_ok))
        x_Status_Return(ERR_CORE_INTERNET_CLOSE_FAILED);

    /* if we have URL contents storage allocated, release it
    */
    if (internet->url_contents != NULL)
        Mem_Release(internet->url_contents);

    /* destroy the object
    */
    internet->signature = 0L;
    Mem_Release(internet);
    x_Status_Return_Success();
}


T_Status    WWW_Retrieve_URL(T_WWW internet,T_Glyph_Ptr url,
                                    T_Glyph_Ptr * contents,T_Void_Ptr context)
{
    HINTERNET   handle = NULL;
    T_Glyph     actual_url[500];
    TCHAR       temp[500],*src;
    DWORD       size,actual,extra,options;
    BOOL        is_ok;
    T_Glyph_Ptr ptr,new_ptr;
    T_Status    status;

    /* validate the parameters
    */
    if (x_Trap_Opt((internet == NULL) || (internet->signature != SIGNATURE) ||
                   (url == NULL) || (*url == '\0')))
        x_Status_Return(ERR_CORE_INVALID_OBJECT_PARAM);
    if (x_Trap_Opt(contents == NULL))
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
x_Trace("Retrieve_URL: [%s]\n",url);

    /* open the URL for raw data retrieval
    */
    options = INTERNET_FLAG_RAW_DATA | INTERNET_FLAG_RESYNCHRONIZE;
    handle = InternetOpenUrl(internet->internet,url,NULL,0,options,0);
x_Trace("InternetOpenUrl = %x\n",handle);
    if (handle == NULL)
        x_Status_Return(ERR_CODE_INTERNET_URL_ACCESS);

    /* retrieve the actual URL, just in case we were redirected or some such
    */
    size = 500;
    is_ok = InternetQueryOption(handle,INTERNET_OPTION_URL,temp,&size);
x_Trace("InternetQueryOption = %d\n",is_ok);
    if (!is_ok)
        x_Status_Return(ERR_CODE_INTERNET_URL_RESOLUTION);

    /* convert the resolved URL from TCHARs to normal text
    */
    for (ptr = actual_url, src = temp; *src != 0; )
        *ptr++ = (T_Glyph) *src++;
    *ptr = '\0';

    /* make sure that we have initial storage for our URL contents available
    */
    if (internet->url_contents == NULL) {
        status = Mem_Acquire(START_SIZE, (T_Void_Ptr *) &internet->url_contents);
        if (x_Trap_Opt(!x_Is_Success(status)))
            goto cleanup_exit;
        internet->max_size = START_SIZE;
        }

    /* retrieve the page until there's nothing to retrieve
    */
    ptr = internet->url_contents;
    internet->current_size = 0;
    while (TRUE) {

        /* find out how much data is available; if there is no data, we're done
        */
        is_ok = InternetQueryDataAvailable(handle,&size,0,0);
x_Trace("InternetQueryDataAvailable = %d  (size = %d)\n",is_ok,size);
        if (is_ok && (size == 0))
            break;
        if (!is_ok) {
            status = ERR_CORE_INTERNET_URL_RETRIEVE;
            goto cleanup_exit;
            }

        /* make sure that we have enough storage left for the new data; grow the
            size by the larger of our regular increment and the amount we need
        */
        if (internet->current_size + size > internet->max_size) {
            extra = max(GROW_SIZE,size);
            status = Mem_Resize(internet->url_contents,
                                    internet->max_size + extra,
                                    (T_Void_Ptr *) &new_ptr);
            if (x_Trap_Opt(!x_Is_Success(status)))
                goto cleanup_exit;
            internet->max_size += extra;
            internet->url_contents = new_ptr;
            ptr = internet->url_contents + internet->current_size;
            }

        /* read the data into memory
        */
        is_ok = InternetReadFile(handle,ptr,size,&actual);
x_Trace("InternetReadFile = %d  (size = %d)\n",is_ok,actual);
        if (!is_ok) {
            status = ERR_CORE_INTERNET_URL_RETRIEVE;
            goto cleanup_exit;
            }
        if (size != actual) {
            status = ERR_CORE_INTERNET_URL_RETRIEVE;
            goto cleanup_exit;
            }

        /* update all of our accrued information
        */
        ptr += size;
        internet->current_size += size;
        }

    /* be sure to null-terminate the page data
    */
    *ptr = '\0';

    /* if we got here, everything went smoothly
    */
    status = SUCCESS;

    /* report the URL contents back to the caller
    */
    *contents = internet->url_contents;

    /* close the URL handle
    */
cleanup_exit:
    is_ok = InternetCloseHandle(handle);
    if (!is_ok && x_Is_Success(status))
        status = ERR_CORE_INTERNET_URL_CLOSE;

    /* return the ultimate result code of the task
    */
    x_Status_Return(status);
}


static  void    Set_Timeouts(HINTERNET handle,T_Int32U connect,T_Int32U receive)
{
    BOOL        is_ok;

    is_ok = InternetSetOption(handle,INTERNET_OPTION_CONNECT_TIMEOUT,&connect,sizeof(DWORD));
    x_Trap_Opt(!is_ok);
    is_ok = InternetSetOption(handle,INTERNET_OPTION_RECEIVE_TIMEOUT,&receive,sizeof(DWORD));
    x_Trap_Opt(!is_ok);
}


T_Status    WWW_HTTP_Open(T_WWW * internet,T_Glyph_Ptr url,
                                T_Glyph_Ptr proxy,T_Glyph_Ptr user,T_Glyph_Ptr password)
{
    HINTERNET       handle = NULL,connect = NULL;
    INTERNET_PORT   port;
    DWORD           open_type;
    T_WWW           new_internet = NULL;
    T_Status        status;
    T_Int32U        access, receive;
    T_Glyph         server[200];
    T_Boolean       is_server,is_secure;

    /* validate the parameters
    */
    if (x_Trap_Opt(internet == NULL))
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
    *internet = NULL;
x_Trace("HTTP_Open: [%s]\n",url);

    /* use default values for timeouts
    */
    access = CONNECT_TIMEOUT;
    receive = RECEIVE_TIMEOUT;

    /* extract the server from the URL
    */
    is_server = Get_Server_Name(server,url,&is_secure);
    if (!is_server)
        x_Status_Return(ERR_CORE_INTERNET_INVALID_SERVER);

    /* allocate a new internet object
    */
    status = Mem_Acquire(sizeof(T_WWW_Body), (T_Void_Ptr *) &new_internet);
    if (x_Trap_Opt(!x_Is_Success(status)))
        goto cleanup_exit;

    /* determine the type of connection we need to open, depending on whether
        there is a proxy server specified or not
    */
    if (proxy != NULL)
        open_type = INTERNET_OPEN_TYPE_PROXY;
    else
        open_type = INTERNET_OPEN_TYPE_PRECONFIG;

    /* establish access to the Internet; claim to be Internet Explorer 7, since
        that's as good a choice as anything else
    */
    handle = InternetOpen("Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)",open_type,proxy,NULL,0);
x_Trace("InternetOpen = %x\n",handle);
    if (handle == NULL) {
        x_Trace("InternetOpen: %d\n",GetLastError());
        status = ERR_CORE_INTERNET_OPEN_FAILED;
        goto cleanup_exit;
        }

    /* configure an appropriate timeout for our access
    */
    Set_Timeouts(handle,access,receive);

    /* determine the appropriate port to utilize
    */
    port = (is_secure) ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    /* establish an HTTP connection
    */
    connect = InternetConnect(handle,server,port,user,password,
                                INTERNET_SERVICE_HTTP,0,0);
x_Trace("InternetConnect = %x  (server = %s)\n",connect,server);
    if (connect == NULL) {
        x_Trace("InternetConnect: %d\n",GetLastError());
        status = ERR_CORE_INTERNET_OPEN_FAILED;
        goto cleanup_exit;
        }

    /* configure an appropriate timeout for our connection
    */
    Set_Timeouts(connect,access,receive);

    /* initialize the new internet object and return it
    */
    new_internet->signature = SIGNATURE;
    new_internet->internet = handle;
    new_internet->connect = connect;
    strcpy(new_internet->server,server);
    new_internet->url_contents = NULL;
    new_internet->max_size = 0;
    new_internet->current_size = 0;
    new_internet->connect_timeout = access;
    new_internet->receive_timeout = receive;
    *internet = new_internet;

    /* if we got here, everything was successful
    */
    status = SUCCESS;

    /* cleanup after ourselves and return
    */
cleanup_exit:
    if (!x_Is_Success(status)) {
        if (connect != NULL)
            InternetCloseHandle(connect);
        if (handle != NULL)
            InternetCloseHandle(handle);
        if (new_internet != NULL)
            Mem_Release(new_internet);
        }
    x_Status_Return(status);
}


static  T_Status    Open_Request(T_WWW internet,T_Glyph_Ptr url,
                                    T_Glyph_Ptr nature,HINTERNET * request)
{
    DWORD       size,options;
    BOOL        is_ok;
    T_Glyph_Ptr path;
    T_Boolean   is_secure;
    T_Glyph_Ptr types[2] = { "*/*", NULL };

    /* determine if this is a secure retrieval
    */
    is_secure = (_strnicmp(url,"https://",8) == 0);

    /* strip off the path, excluding the server
    */
    path = strstr(url,"//");
    x_Trap_Opt(path == NULL);
    path += 2;
    path = strchr(path,'/');
    x_Trap_Opt(path == NULL);
    path++;

    /* setup appropriate options for the request
    */
    options = INTERNET_FLAG_KEEP_CONNECTION;
    options |= INTERNET_FLAG_RESYNCHRONIZE;
    if (is_secure) {
        options |= INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
        options |= INTERNET_FLAG_IGNORE_CERT_DATE_INVALID;
        options |= INTERNET_FLAG_SECURE;
        }

    /* force all retrievals to ALWAYS re-load from the remote server, ignoring
        anything in the cache
    */
//FIX - this is here to save users from themselves who use aggressive caching; is there are better way?
//FIX - maybe this should also be controlled via a param to let the client decide??
    options |= INTERNET_FLAG_RELOAD;

    /* open an HTTP request
    */
    *request = HttpOpenRequest(internet->connect,nature,path,NULL,NULL,(LPCSTR *) types,options,0);
x_Trace("HttpOpenRequest = %x  (options = %x)\n",*request,options);
    if (x_Trap_Opt(*request == NULL)) {
        x_Trace("HttpOpenRequest: %d\n",GetLastError());
        x_Status_Return(ERR_CORE_INTERNET_URL_RETRIEVE);
        }

    /* configure an appropriate timeout for our request
    */
    Set_Timeouts(*request,internet->connect_timeout,internet->receive_timeout);

    /* if this is a secure connection, setup appropriate security options
    */
    if (is_secure) {
        size = sizeof(options);
        is_ok = InternetQueryOption(*request,INTERNET_OPTION_SECURITY_FLAGS,&options,&size);
        x_Trap_Opt(!is_ok);
        if (is_ok) {
            options |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
            is_ok = InternetSetOption(*request,INTERNET_OPTION_SECURITY_FLAGS,&options,sizeof(options));
            x_Trap_Opt(!is_ok);
            }
        }

    /* we're now open
    */
    x_Status_Return_Success();
}


static  T_Status    Send_Request(HINTERNET request,T_Glyph_Ptr header,T_Int32U header_len,
                                    T_Glyph_Ptr extra,T_Int32U extra_len)
{
    DWORD       options,errcode;
    BOOL        is_ok;
    HWND        window;

    /* retrieve the foreground window for the application
    */
    window = GetForegroundWindow();

    /* enter a loop continually retrying to send the request; we use the loop to
        handle server and proxy authentication, which need to retry the send
    */
    while (1) {

        /* issue the HTTP request to the server
        */
        is_ok = HttpSendRequest(request,header,header_len,extra,extra_len);
x_Trace("HttpSendRequest = %d\n",is_ok);
        if (!is_ok)
            x_Trace("HttpSendRequest: %d\n",GetLastError());

        /* if the foreground window is NULL, then we can't do any proxy stuff,
            since we can't provide a window for the dialog; if this occurs, we
            simply succeed or fail directly based on the request results
            NOTE! The foreground window WILL be NULL when running within a
                    service, such as within an ATL component used under ASP.
        */
        if (window == NULL)
            x_Status_Return(is_ok ? SUCCESS : ERR_CORE_INTERNET_URL_RETRIEVE);

        /* determine the error code to have Windows automatically handle for us
        */
        errcode = (is_ok) ? ERROR_SUCCESS : GetLastError();

        /* display an appropriate dialog to solicit authentication input
        */
        options = FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                    FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS |
                    FLAGS_ERROR_UI_FLAGS_GENERATE_DATA;
        errcode = InternetErrorDlg(window,request,errcode,options,NULL);
x_Trace("InternetErrorDlg = %x\n",errcode);

        /* handle the result appropriately
        */
        if (errcode == ERROR_SUCCESS)
            break;
        if (errcode == ERROR_CANCELLED)
            x_Status_Return(ERR_CORE_INTERNET_CANCEL);
        if (errcode != ERROR_INTERNET_FORCE_RETRY)
            x_Status_Return(ERR_CORE_INTERNET_URL_RETRIEVE);
        }

    /* request has been sent
    */
    x_Status_Return_Success();
}


static  T_Status    Get_Response(T_WWW internet,HINTERNET request,
                                    T_Glyph_Ptr * contents,T_Int32U * current,
                                    T_Int32U * max_size,
                                    T_Void_Ptr context,T_Boolean is_size_check)
{
    DWORD       size,actual,extra,length;
    BOOL        is_ok;
    T_Glyph_Ptr ptr,new_ptr;
    T_Status    status;
    T_Glyph     header[5000],temp[5000];

    /* retrieve the headers from the response
    */
    size = 5000;
    actual = 0;
    is_ok = HttpQueryInfo(request,HTTP_QUERY_RAW_HEADERS_CRLF,header,&size,&actual);
x_Trace("HttpQueryInfo = %d   (actual = %d)\n",is_ok,actual);
    if (x_Trap_Opt(!is_ok)) {
        x_Trace("HttpQueryInfo: %d\n",GetLastError());
        x_Status_Return(ERR_CORE_INTERNET_URL_RETRIEVE);
        }

    /* locate the "content-length" header and extract the size
    */
    if (actual == 5000)
        header[4999] = '\0';
    strcpy(temp,header);
    strlwr(temp);
    ptr = strstr(temp,"content-length:");
    if (ptr == NULL)
        length = 0;
    else {
        ptr = strchr(ptr,':');
        ptr++;
        while (isspace(*ptr))
            ptr++;
        length = atol(ptr);
        }
x_Trace("Content Length: %d\n",length);

    /* make sure that we have initial storage for our URL contents available
    */
    if (*contents == NULL) {
        status = Mem_Acquire(START_SIZE,(T_Void_Ptr *) contents);
        if (x_Trap_Opt(!x_Is_Success(status)))
            x_Status_Return(status);
        *max_size = START_SIZE;
        }

    /* retrieve the page until there's nothing to retrieve
    */
    ptr = *contents;
    *current = 0;
    while (TRUE) {

        /* find out how much data is available; if there is no data, we're done
        */
        is_ok = InternetQueryDataAvailable(request,&size,0,0);
x_Trace("InternetQueryDataAvailable = %d\n",is_ok);
        if (is_ok && (size == 0))
            break;
        if (!is_ok) {
            x_Trace("InternetQueryDataAvailable: %d\n",GetLastError());
            x_Status_Return(ERR_CORE_INTERNET_URL_RETRIEVE);
            }

        /* make sure that we have enough storage left for the new data; grow the
            size by the larger of our regular increment and the amount we need
        */
        if (*current + size > *max_size) {
            extra = max(GROW_SIZE,size);
            status = Mem_Resize(*contents,*max_size + extra,(T_Void_Ptr *) &new_ptr);
            if (x_Trap_Opt(!x_Is_Success(status)))
                x_Status_Return(status);
            *max_size += extra;
            *contents = new_ptr;
            ptr = *contents + *current;
            }

        /* read the data into memory
        */
        is_ok = InternetReadFile(request,ptr,size,&actual);
x_Trace("InternetReadFile = %d  (actual = %d)\n",is_ok,actual);
        if (!is_ok) {
            x_Trace("InternetReadFile: %d\n",GetLastError());
            x_Status_Return(ERR_CORE_INTERNET_URL_RETRIEVE);
            }
        if (size != actual)
            x_Status_Return(ERR_CORE_INTERNET_URL_RETRIEVE);

        /* update all of our accrued information
        */
        ptr += size;
        *current += size;
        }

    /* be sure to null-terminate the page data
    */
    *ptr = '\0';

    /* if we're asked to verify the size retrieved matches the exact size
        reported as available by the web server, perform that check now
    */
    if (is_size_check)
        if ((length != 0) && (length != *current))
            x_Status_Return(WARN_CORE_INTERNET_SIZE_MISMATCH);

    x_Status_Return_Success();
}


T_Status    WWW_HTTP_Post(T_WWW internet,T_Glyph_Ptr url,
                                    T_Int32U count,T_Glyph_Ptr * names,
                                    T_Glyph_Ptr * values,
                                    T_Glyph_Ptr * contents)
{
    HINTERNET   request = NULL;
    T_Int32U    i,size;
    T_Glyph     buffer[1000];
    T_Glyph_Ptr data = NULL;
    T_Glyph_Ptr ptr;
    T_Status    status;
    BOOL        is_ok;

    /* validate the parameters
    */
    if (x_Trap_Opt((internet == NULL) || (internet->signature != SIGNATURE) ||
                   (url == NULL) || (*url == '\0')))
        x_Status_Return(ERR_CORE_INVALID_OBJECT_PARAM);
    if (x_Trap_Opt(contents == NULL))
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
x_Trace("HTTP_Post: [%s]\n",url);

    /* calculate the total size of the data to be sent
    */
    for (i = size = 0; i < count; i++) {
        size += strlen(names[i]) + 1;           // '='
        size += strlen(values[i]) + 1;          // '&'
        }
    size--;     // extra '&'

    /* allocate storage for our data (include null-terminator)
    */
    status = Mem_Acquire(size + 1,(T_Void_Ptr *) &data);
    if (x_Trap_Opt(!x_Is_Success(status)))
        x_Status_Return(status);

    /* synthesize the data to be sent
    */
    for (i = 0, ptr = data; i < count; i++) {
        if (i > 0)
            *ptr++ = '&';
        strcpy(ptr,names[i]);
        ptr += strlen(ptr);
        *ptr++ = '=';
        strcpy(ptr,values[i]);
        ptr += strlen(ptr);
        }
    *ptr = '\0';
//Debug_Printf("contents:\n%s\n",data);

    /* open a POST request for the URL
    */
    status = Open_Request(internet,url,"POST",&request);
    if (!x_Is_Success(status))
        goto cleanup_exit;

    /* setup the appropriate request header for content type
    */
    ptr = "Content-Type: application/x-www-form-urlencoded\r\n";
    is_ok = HttpAddRequestHeaders(request,ptr,strlen(ptr),HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
    if (x_Trap_Opt(!is_ok)) {
        x_Trace("HttpAddRequestHeaders: %d\n",GetLastError());
        status = ERR_CORE_INTERNET_HEADERS;
        goto cleanup_exit;
        }

    /* setup the appropriate request header for content length
    */
    sprintf(buffer,"Content-Length: %d\r\n",size);
    is_ok = HttpAddRequestHeaders(request,buffer,strlen(buffer),HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
    if (x_Trap_Opt(!is_ok)) {
        x_Trace("HttpAddRequestHeaders: %d\n",GetLastError());
        status = ERR_CORE_INTERNET_HEADERS;
        goto cleanup_exit;
        }

    /* send the request to the server
    */
    status = Send_Request(request,NULL,0,data,size);
    if (!x_Is_Success(status))
        goto cleanup_exit;

    /* retrieve the response from the server
    */
    status = Get_Response(internet,request,&internet->url_contents,
                            &internet->current_size,&internet->max_size,NULL,FALSE);
    if (!x_Is_Success(status))
        goto cleanup_exit;

    /* everything went smoothly, so report the URL contents back to the caller
    */
    *contents = internet->url_contents;
    status = SUCCESS;

    /* cleanup after everything and return
    */
cleanup_exit:
    if (request != NULL)
        InternetCloseHandle(request);
    if (data != NULL)
        Mem_Release(data);
    x_Status_Return(status);
}
