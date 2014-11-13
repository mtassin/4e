/*  PLATFORM_OSX.H

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

    OS X definitions for D&D insider crawler.
*/

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>


// Same flag for different APIs
#ifndef NS_STRING_ENCODING
#define NS_STRING_ENCODING	NSUTF8StringEncoding
#define CF_STRING_ENCODING	kCFStringEncodingUTF8
#endif


class CollectGarbageHelper
{
private:
	NSAutoreleasePool *pool;
public:
	CollectGarbageHelper()
	{
		pool = [[NSAutoreleasePool alloc] init];
	}
	~CollectGarbageHelper()
	{
		[pool drain];
	}
};
