/*  FILE:   TEXT_OSX.CPP
 
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


#include "private.h"


/* This function is standard on windows, but not on mac os
*/
char* strupr(char *input)
{
    T_Int32U i, length;
    
    /* empty string doesn't change
     */
    if (input[0] == '\0')
        return(input);
    
    length = strlen(input);
    for (i = 0; i < length; i++)
        input[i] = toupper(input[i]);
    
    return(input);
}


/* This function is standard on windows, but not on mac os
*/
char* strlwr(char *input)
{
    T_Int32U i, length;
    
    /* empty string doesn't change
     */
    if (input[0] == '\0')
        return(input);
    
    length = strlen(input);
    for (i = 0; i < length; i++)
        input[i] = tolower(input[i]);
    
    return(input);
}
