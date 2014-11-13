/*  FILE:   WWW_OSX.CPP

    Copyright (c) 1999-2010 by Lone Wolf Development.  All rights reserved.

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

#include <pthread.h>


#include "private.h"

#include "platform_osx.h"


/*  define private constants used by this source file
*/
#define SIGNATURE           0x496e6574      /* Inet */

#define BUFFER_SIZE         32768

#define LWD_INTERNET_MODE   @"LoneWolfInternetMode"

@class LoneWolfInternet;


/*  define error codes
*/
#define ERR_CORE_INVALID_OBJECT_PARAM           0x60
#define ERR_CORE_INVALID_RETURN_PARAM           0x61

#define ERR_CORE_INTERNET_URL_ACCESS            0x70
#define ERR_CORE_THREAD_CREATE_FAILED           0x71
#define ERR_CORE_THREAD_NOT_EXITABLE            0x72
#define ERR_CORE_INTERNET_CANCEL                0x73
#define ERR_CORE_INTERNET_URL_RETRIEVE          0x74


/*  define the structure of an internet object
*/
typedef struct T_WWW_ {
    T_Int32U            signature;
    // Any info worth saving from the 'create' function
    NSURL              *url;    // Only used for FTP
    NSString           *username;
    NSString           *password;
    // Store any extra required info
    T_Int32U            lastQuerySize;
    }   T_WWW_Body, * T_WWW;


/*  define the Objective C object which implements all our intelligence
 */

@interface LoneWolfInternet : NSObject
{
    T_WWW               internet;
    NSMutableURLRequest*request;
    NSMutableData *     pageData;
    T_Int32U            m_content_length;

    T_Status            status;
    NSError *           lastError;

    T_Void_Ptr          progressContext;

    NSThread *          ownerThread;
    CFRunLoopRef        ownerLoop;
}

+ (LoneWolfInternet*)connectionForInternet:(T_WWW)intr;

- (NSMutableURLRequest*)request;
- (NSData*)pageData;
- (NSError*)lastError;

- (void*)workThread;

@end

static NSString* ConvertString(T_Glyph_Ptr str)
{
    if (str == NULL)
        return nil;
    return [NSString stringWithCString:str encoding:NS_STRING_ENCODING];
}

static NSURL* ConvertURL(T_Glyph_Ptr url)
{
    return [NSURL URLWithString:ConvertString(url)];
}

static void* Request_Thread(void *param)
{
    CollectGarbageHelper helper;
    void *result;

    result = [((id)param) workThread];
    return result;
}

static void StopThread(void)
{
    CFRunLoopStop(CFRunLoopGetCurrent());
}

static void Dummy_Perform(void *info)
{
    // Should never be called!
    x_Trap_Opt(1);
}


@implementation LoneWolfInternet

- (NSMutableURLRequest*)request
{
    return request;
}

- (NSData*)pageData
{
    return pageData;
}

- (NSError*)lastError
{
    return lastError;
}

- initForInternet:(T_WWW)intr
{
    self = [super init];
    internet = intr;
    request = [[NSMutableURLRequest alloc] init];
    pageData = nil;
    m_content_length = 0;
    status = ERR_CORE_INTERNET_URL_ACCESS;
    lastError = nil;
    return self;
}

+ (LoneWolfInternet*)connectionForInternet:(T_WWW)intr
{
    return [[[LoneWolfInternet alloc] initForInternet:intr] autorelease];
}

- (void)dealloc
{
    [lastError release];
    [request release];
    [pageData release];
    [super dealloc];
}

- (void*)workThread
{
    NSURLConnection *connection;

    status = ERR_CORE_INTERNET_URL_ACCESS;
    connection = [[NSURLConnection alloc] initWithRequest:request delegate:self];

    CFRunLoopRun();

    // When it exits, the connection has been finished with
    CFRunLoopStop(ownerLoop);
    ownerLoop = NULL;
    ownerThread = nil;
    return NULL;
}

- (T_Status)execute
{
    CFRunLoopSourceContext  sourceContext;
    CFRunLoopSourceRef      loopSource;
    pthread_t               workThread;

    /* Clear any previous state
     */
    [lastError release];
    lastError = nil;
    ownerThread = [NSThread currentThread];
    ownerLoop = CFRunLoopGetCurrent();

    /* Create a runloop source - this is normally a way to get messages into the
        loop, but here we use it to keep the run loop open until the download
        is complete. If we don't have one in, the runloop is considered
        'finished' and ends, as it has nowhere to get messages from.
    */
    memset(&sourceContext, 0, sizeof(sourceContext));
    sourceContext.perform = Dummy_Perform;
    loopSource = CFRunLoopSourceCreate(NULL, 0, &sourceContext);
    CFRunLoopAddSource(ownerLoop, loopSource, (CFStringRef)LWD_INTERNET_MODE);

    /* Launch the thread
     */
    if (pthread_create(&workThread, NULL, Request_Thread, self) != 0)
        x_Status_Return(ERR_CORE_THREAD_CREATE_FAILED);

    /* Get into the runloop, allowing the worker thread to call us back
        The large number constant means we will run for 20 years before ending,
        assuming nothing else stops us in the mean time. Hopefully that's enough
        time for the download to finish.
     */
    CFRunLoopRunInMode((CFStringRef) LWD_INTERNET_MODE, 600000000, false);

    /* Wait for the worker thread to quit - once it does, the download is done
     */
    if (pthread_join(workThread, NULL) != 0)
        x_Status_Return(ERR_CORE_THREAD_NOT_EXITABLE);

    /* Remove the runloop source
     */
    CFRunLoopSourceInvalidate(loopSource);
    CFRelease(loopSource);

    /* Done
     */
    x_Status_Return(status);
}

// NSURLConnection delegate methods

    // Authentication

- (BOOL)connection:(NSURLConnection*)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace*)protectionSpace
{
    return YES;
}

- (void)connection:(NSURLConnection*)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge*)challenge
{
    NSString *username, *password;

    if (internet->username != nil)
        username = internet->username;
    else
        username = [[request URL] user];
    if (internet->password != nil)
        password = internet->password;
    else
        password = [[request URL] password];
    if ((username == nil) || (password == nil))
        [[challenge sender] continueWithoutCredentialForAuthenticationChallenge:challenge];
    else
        [[challenge sender] useCredential:[NSURLCredential credentialWithUser:username password:password persistence:NSURLCredentialPersistenceNone] forAuthenticationChallenge:challenge];
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection*)connection
{
    return NO;
}

    // Data

- (NSCachedURLResponse*)connection:(NSURLConnection*)connection willCacheResponse:(NSCachedURLResponse*)cachedResponse
{
    return nil;
}

- (void)connection:(NSURLConnection*)connection didReceiveResponse:(NSURLResponse*)response
{
    /* Save our data length for later, if we have it
    */
    m_content_length = (T_Int32U) [response expectedContentLength];
    if (m_content_length == NSURLResponseUnknownLength)
        m_content_length = 0;

    /* Get ready to save new incoming data
    */
    [pageData release];
    pageData = [[NSMutableData dataWithCapacity:1024] retain];
}

- (void)connection:(NSURLConnection*)connection didReceiveData:(NSData*)data
{
    [pageData appendData:data];
}

- (void)connection:(NSURLConnection*)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite
{
}

- (NSURLRequest*)connection:(NSURLConnection*)connection willSendRequest:(NSURLRequest*)sendRequest redirectResponse:(NSURLResponse*)redirectResponse
{
    return sendRequest;
}

    // End situations

- (void)connectionDidFinishLoading:(NSURLConnection*)connection
{
    [pageData appendBytes:"\0" length:1];   // Add the NUL
    status = SUCCESS;
    StopThread();
}

- (void)connection:(NSURLConnection*)connection didFailWithError:(NSError*)error
{
    lastError = [error retain];
    status = ERR_CORE_INTERNET_URL_RETRIEVE;
    StopThread();
}

- (void)connection:(NSURLConnection*)connection didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge*)challenge
{
    [self connection:connection didFailWithError:NULL];
}

@end


T_Status    WWW_Close_Server(T_WWW internet)
{
    CollectGarbageHelper helper;

    /* validate the parameter
    */
    if (x_Trap_Opt((internet == NULL) || (internet->signature != SIGNATURE)))
        x_Status_Return(ERR_CORE_INVALID_OBJECT_PARAM);

    /* destroy the object
    */
    internet->signature = 0L;
    [internet->url release];
    [internet->username release];
    [internet->password release];
    Mem_Release(internet);
    x_Status_Return_Success();
}



static T_Status Internal_Retrieve_URL(T_WWW internet,T_Glyph_Ptr url,
                                      T_Glyph_Ptr * contents,T_Int32U * size,
                                      T_Void_Ptr context)
{
    CollectGarbageHelper helper;
    T_Status    status;
    T_Int32U    contentSize;
    LoneWolfInternet *connection;

    /* retrieve the URL contents normally
     */
    connection = [LoneWolfInternet connectionForInternet:internet];
    [[connection request] setURL:ConvertURL(url)];

    status = [connection execute];
    if (!x_Is_Success(status))
        x_Status_Return(status);

    /* report the size properly and return
     */
    contentSize = [[connection pageData] length];
    status = Mem_Acquire(contentSize, (T_Void_Ptr*)contents);
    if (!x_Is_Success(status))
        x_Status_Return(status);
    [[connection pageData] getBytes:*contents length:contentSize];
    if (size != NULL)
        *size = contentSize - 1; // -1 for NUL character
    internet->lastQuerySize = contentSize - 1;

    x_Status_Return_Success();
}


T_Status    WWW_Retrieve_URL(T_WWW internet,T_Glyph_Ptr url,
                                    T_Glyph_Ptr * contents,T_Void_Ptr context)
{
    return Internal_Retrieve_URL(internet, url, contents, NULL, context);
}


T_Status    WWW_HTTP_Open(T_WWW * internet,T_Glyph_Ptr url,
                                T_Glyph_Ptr proxy,T_Glyph_Ptr user,T_Glyph_Ptr password)
{
    CollectGarbageHelper helper;
    T_WWW           new_internet = NULL;
    T_Status        status;

    /* validate the parameters
    */
    if (x_Trap_Opt(internet == NULL))
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
    *internet = NULL;

    /* allocate a new internet object
    */
    status = Mem_Acquire(sizeof(T_WWW_Body),
                            (T_Void_Ptr *) &new_internet);
    if (x_Trap_Opt(!x_Is_Success(status)))
        goto cleanup_exit;

    /* initialize the new internet object and return it
    */
    new_internet->signature = SIGNATURE;
    new_internet->url = [ConvertURL(url) retain];
    new_internet->username = [ConvertString(user) retain];
    new_internet->password = [ConvertString(password) retain];
    new_internet->lastQuerySize = 0;
    *internet = new_internet;

    /* if we got here, everything was successful
    */
    status = SUCCESS;

    /* cleanup after ourselves and return
    */
cleanup_exit:
    if (!x_Is_Success(status))
    {
        if (new_internet != NULL)
            Mem_Release(new_internet);
    }
    x_Status_Return(status);
}


T_Status    WWW_HTTP_Post(T_WWW internet,T_Glyph_Ptr url,
                                    T_Int32U count,T_Glyph_Ptr * names,
                                    T_Glyph_Ptr * values,
                                    T_Glyph_Ptr * contents)
{
    CollectGarbageHelper helper;
    T_Int32U    i,size;
    T_Glyph_Ptr data = NULL;
    T_Glyph_Ptr ptr;
    T_Status    status;
    T_Int32U    contentSize;
    LoneWolfInternet *connection;

    /* validate the parameters
    */
    if (x_Trap_Opt((internet == NULL) || (internet->signature != SIGNATURE) ||
                   (url == NULL) || (*url == '\0')))
        x_Status_Return(ERR_CORE_INVALID_OBJECT_PARAM);
    if (x_Trap_Opt(contents == NULL))
        x_Status_Return(ERR_CORE_INVALID_RETURN_PARAM);
//x_Trace("HTTP_Post: [%s]\n",url);

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

    /* open a POST request for the URL
    */
    connection = [LoneWolfInternet connectionForInternet:internet];
    [[connection request] setURL:ConvertURL(url)];
    [[connection request] setHTTPMethod:@"POST"];
    [[connection request] setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [[connection request] setValue:[NSString stringWithFormat:@"%d", size] forHTTPHeaderField:@"Content-Length"];
    [[connection request] setHTTPBody:[NSData dataWithBytes:data length:size]];

    /* send the request to the server
    */
    status = [connection execute];
    if (!x_Is_Success(status))
        goto cleanup_exit;

    /* everything went smoothly, so report the URL contents back to the caller
    */
    contentSize = [[connection pageData] length];
    status = Mem_Acquire(contentSize, (T_Void_Ptr*)contents);
    if (!x_Is_Success(status))
        goto cleanup_exit;
    [[connection pageData] getBytes:*contents length:contentSize];
    internet->lastQuerySize = contentSize - 1;

    status = SUCCESS;

    /* cleanup after everything and return
    */
cleanup_exit:
    if (data != NULL)
        Mem_Release(data);
    x_Status_Return(status);
}
