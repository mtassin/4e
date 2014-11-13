/*  FILE:   ENCODE.CPP

    Copyright (c) 2008-2009 by Lone Wolf Development, Inc.  All rights reserved.

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

    Encoding functions for the D&DI Crawler.
*/


#include    "private.h"


/*  define constants used below
*/
#define DEFAULT_LINE_SIZE       72      // 72 is the default for MIME encoding

#define NO_BREAKS               ((T_Int32U) -1)


/*  declase static variables used below
*/
static  char        l_encode[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static  char        l_decode[] = "|$$$}rstuvwxyz{$$$$$$$>?@ABCDEFGHIJKLMNOPQRSTUVW$$$$$$XYZ[\\]^_`abcdefghijklmnopq";

static  T_Int32U    l_line_size = DEFAULT_LINE_SIZE;


// Encode 3 8-bit binary bytes as 4 '6-bit' characters.
static  void    Encode_Block(T_Int8U in[3],T_Int8U out[4],T_Int32U count)
{
    out[0] = l_encode[in[0] >> 2];
    out[1] = l_encode[((in[0] & 0x03) << 4) | ((in[1] & 0xf0) >> 4)];
    if (count > 1)
        out[2] = l_encode[((in[1] & 0x0f) << 2) | ((in[2] & 0xc0) >> 6)];
    else
        out[2] = '=';
    if (count > 2)
        out[3] = l_encode[in[2] & 0x3f];
    else
        out[3] = '=';
}


// Encode a stream adding padding and line breaks as per spec.
// WARNING! The encoded output buffer is assumed to be large enough to hold then
//          encode text, which is 33% larger than the source data.
void        Text_Encode(T_Byte_Ptr data,T_Int32U in_len,T_Glyph_Ptr encode)
{
    T_Int8U     in[3],out[4];
    T_Int32U    i,count,blocks;

    /* process our input stream until there's nothing left
    */
    blocks = 0;
    while (in_len > 0) {

        /* populate our next 3-byte block for processing
        */
        for (i = 0, count = 0; i < 3; i++) {
            if (in_len > 0) {
                in[i] = *data++;
                in_len--;
                count++;
                }
            else
                in[i] = 0;
            }

        /* if we have a block available, encode it and output it
        */
        if (count > 0) {
            Encode_Block(in,out,count);
            for (i = 0; i < 4; i++)
                *encode++ = out[i];
            blocks++;
            }

        /* if we've reached the end of a line, emit the line break
            NOTE! We also emit a line break after the last line of data, EXCEPT
                    when there is no new data to output (i.e. blocks == 0).
        */
        if (l_line_size != (T_Int32U) NO_BREAKS) {
            if ((blocks >= (l_line_size / 4)) || (in_len == 0)) {
                if (blocks > 0) {
                    *encode++ = '\r';
                    *encode++ = '\n';
                    }
                blocks = 0;
                }
            }
        }

    /* null-terminate our encoded text
    */
    *encode = '\0';
}


// Decode 4 '6-bit' characters into 3 8-bit binary bytes
static  void    Decode_Block(T_Int8U in[4],T_Int8U out[3])
{
    out[0] = (T_Int8U) (in[0] << 2 | in[1] >> 4);
    out[1] = (T_Int8U) (in[1] << 4 | in[2] >> 2);
    out[2] = (T_Int8U) (((in[2] << 6) & 0xc0) | in[3]);
}


// Decode an encoded stream, discarding padding, line breaks and noise.
// WARNING! The decoded output buffer is assumed to be large enough to hold then
//          decoded data, which is 25% smaller than the source data.
void        Text_Decode(T_Glyph_Ptr encode,T_Byte_Ptr data,T_Int32U * out_len)
{
    T_Int8U     in[4],out[3],value;
    T_Int32U    i,count;

    /* reset the output length to empty
    */
    *out_len = 0;

    /* keep processing our input stream until we run out
    */
    while (*encode != '\0') {

        /* extract the next 4 '6-bit' characters from the stream
        */
        for (count = 0, i = 0; (i < 4) && (*encode != '\0'); i++) {

            /* get the next meaningful data and map it in (skipping linebreaks)
            */
            for (value = 0; (*encode != '\0') && (value == 0); ) {
                value = (T_Int8U) *encode++;
                if ((value < 43) || (value > 122))
                    value = 0;
                else
                    value = l_decode[value - 43];
                if (value == '$')
                    value = 0;
                if (value != 0)
                    value = (T_Int8U) (value - 61);
                }

            /* if we have a value waiting, accrue it as input
            */
            if (value == 0)
                in[i] = 0;
            else {
                count++;
                in[i] = (T_Int8U) (value - 1);
                }
            }

        /* if there is a block of data waiting, decode it
        */
        if (count > 0) {
            Decode_Block(in,out);
            for (i = 0; i < count - 1; i++) {
                *data++ = out[i];
                (*out_len)++;
                }
            }
        }
}


T_Status    Text_Encode_To_File(T_Glyph_Ptr filename,T_Glyph_Ptr text)
{
    T_Status            status;
    T_Int32U            length;
    T_Glyph_Ptr         temp;

    /* When we're allocating space for the encoded text, make sure that we
        allow for the overhead of the encoding. Also make sure to encode out
        the null terminator at the end of the string, otherwise it isn't there
        when we decode the file and read it back in...
    */
    length = strlen(text)+1;
    temp = new T_Glyph[(T_Int32U) (length * 1.5)];
    if (x_Trap_Opt(temp == NULL))
        x_Status_Return(LWD_ERROR);

    Text_Encode((T_Byte_Ptr) text, length, temp);

    status = Quick_Write_Text(filename, temp);
    delete [] temp;
    x_Status_Return(status);
}


T_Glyph_Ptr Text_Decode_From_File(T_Glyph_Ptr filename)
{
    T_Int32U            length;
    T_Glyph_Ptr         temp, text;

    temp = Quick_Read_Text(filename);
    if (x_Trap_Opt(temp == NULL))
        return(NULL);

    text = new T_Glyph[(T_Int32U) (strlen(temp) * 1.1)];
    if (x_Trap_Opt(text == NULL)) {
        delete [] temp;
        return(NULL);
        }

    Text_Decode(temp, (T_Byte_Ptr) text, &length);
    delete [] temp;
    return(text);
}
