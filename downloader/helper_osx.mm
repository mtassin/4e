/*  FILE:   HELPER_OSX.MM
 
 Copyright (c) 2012 by Lone Wolf Development, Inc.  All rights reserved.
 
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


#include <unistd.h>

#include <iostream>

#include "platform_osx.h"

#include "private.h"


void     Pause_Execution(T_Int32U milliseconds)
{
    usleep(milliseconds * 1000);
}


T_Glyph  Get_Character(void)
{
    T_Glyph     buffer[500];
    
    /* This doesn't work as well as getch() on windows - it requires an enter
        key to be pressed after the keypress - but the only way to avoid that
        is apparently annoyingly complicated
    */
    printf("Enter your choice, then press enter to continue.\n\n");

    /* We need to use cin.getline so that the newline is read off the end of the
        line. If we just use cin.get, the next time we call it it'll read off
        the newline as the next character.
    */
    cin.getline(buffer, 499, '\n');
    return(buffer[0]);
}


bool    Is_Log(void)
{
    SInt32          major, minor;

    /* On Mac OS, don't log if we're using 10.5, as Xcode has a bug that means
        using an ofstream causes an error...
    */
    Gestalt(gestaltSystemVersionMajor, &major);
    Gestalt(gestaltSystemVersionMinor, &minor);
    return !((major == 10) && (minor <= 5));
}
