/*---------------------------------------------------------------------------*/
/* base64                                                                    */
/* ======                                                                    */
/*                                                                           */
/*                                                                           */
/* Base64 encoding is sometimes used for simple HTTP authentication. That is */
/* when strong encryption isn't necessary, Base64 encryption is used to      */
/* authenticate User-ID's and Passwords.                                     */
/*                                                                           */
/* Base64 processes a string by octets (3 Byte blocks).  For every octet in  */
/* the decoded string, four byte blocks are generated in the encoded string. */
/* If the decoded string length is not a multiple of 3, the Base64 algorithm */
/* pads the end of the encoded string with equal signs '='.                  */
/*                                                                           */
/* An example taken from RFC 2617 (HTTP Authentication):                     */
/*                                                                           */
/* Resource (URL) requires basic authentication (Authorization: Basic) for   */
/* access, otherwise a HTTP 401 Unauthorized response is returned.           */
/*                                                                           */
/* User-ID:Password string  = "Aladdin:open sesame"                          */
/* Base64 encoded   string  = "QWxhZGRpbjpvcGVuIHNlc2FtZQ=="                 */
/*                                                                           */
/* Copyright (c) 1994 - 2003                                                 */
/* Marc Niegowski                                                            */
/* Connectivity, Inc.                                                        */
/* All rights reserved.                                                      */
/*---------------------------------------------------------------------------*/
#include    <stdlib.h>                      /* calloc and free prototypes.*/
#include    <string.h>                      /* str* and memset prototypes.*/
#include "base64.h"


typedef
unsigned
char    uchar;                              /* Define unsigned char as uchar.*/

typedef
unsigned
int     uint;                               /* Define unsigned int as uint.*/

bool    b64valid(uchar *);                  /* Tests for a valid Base64 char.*/
char   *b64buffer(char *, bool);            /* Alloc. encoding/decoding buffer.*/


/* Macro definitions:*/

#define b64is7bit(c)  ((c) > 0x7f ? 0 : 1)  /* Valid 7-Bit ASCII character?*/
#define b64blocks(l) (((l) + 2) / 3 * 4 + 1)/* Length rounded to 4 byte block.*/
#define b64octets(l)  ((l) / 4  * 3 + 1)    /* Length rounded to 3 byte octet.*/

/* Note:    Tables are in hex to support different collating sequences*/

static  
const                                       /* Base64 Index into encoding*/
uchar  pIndex[]     =   {                   /* and decoding table.*/
                        0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
                        0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50,
                        0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
                        0x59, 0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                        0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
                        0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
                        0x77, 0x78, 0x79, 0x7a, 0x30, 0x31, 0x32, 0x33,
                        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x2b, 0x2f
                        };

static
const                                       /* Base64 encoding and decoding*/
uchar   pBase64[]   =   {                   /* table.*/
                        0x3e, 0x7f, 0x7f, 0x7f, 0x3f, 0x34, 0x35, 0x36,
                        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x7f,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x00, 0x01,
                        0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                        0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
                        0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
                        0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x1a, 0x1b,
                        0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
                        0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
                        0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33
                        };


/**
 b64encode - Encode a 7-Bit ASCII string to a Base64 string                                                                                          
 @param char *  - The 7-Bit ASCII string to encode                                     
 @return char* - array that contains the new decoded string     
 Note: the caller needs to free() the resultant pointer!             
*/
char* b64encode(char *s)
{
    int     l   = strlen(s);                /* Get the length of the string.*/
    int     x   = 0;                        /* General purpose integers.*/
    char   *b, *p;                          /* Encoded buffer pointers.*/

    while   (x < l)                         /* Validate each byte of the string*/
    {                                       /* ...to ensure that it's 7-Bit...*/
        if (!b64is7bit((uchar) *(s + x)))   /* ...ASCII.*/
        {
            return false;                   /* Return false if it's not.*/
        }
        x++;                                /* Next byte.*/
    }

    if (!(b = b64buffer(s, true)))          /* Allocate an encoding buffer.*/
        return false;                       /* Can't allocate encoding buffer.*/

    memset(b, 0x3d, b64blocks(l) - 1);      /* Initialize it to "=". */

    p = b;                                  /* Save the buffer pointer.*/
    x = 0;                                  /* Initialize string index.*/

    while   (x < (l - (l % 3)))             /* encode each 3 byte octet.*/
    {
        *b++   = pIndex[  s[x]             >> 2];
        *b++   = pIndex[((s[x]     & 0x03) << 4) + (s[x + 1] >> 4)];
        *b++   = pIndex[((s[x + 1] & 0x0f) << 2) + (s[x + 2] >> 6)];
        *b++   = pIndex[  s[x + 2] & 0x3f];
         x    += 3;                         /* Next octet.*/
    }

    if (l - x)                              /* Partial octet remaining?*/
    {
        *b++        = pIndex[s[x] >> 2];    /* Yes, encode it.*/

        if  (l - x == 1)                    /* End of octet?*/
            *b      = pIndex[ (s[x] & 0x03) << 4];
        else                            
        {                                   /* No, one more part.*/
            *b++    = pIndex[((s[x]     & 0x03) << 4) + (s[x + 1] >> 4)];
            *b      = pIndex[ (s[x + 1] & 0x0f) << 2];
        }
    }

    return p;
}


/*---------------------------------------------------------------------------*/
/* b64valid - validate the character to decode.                              */
/* ============================================                              */
/*                                                                           */
/* Checks whether the character to decode falls within the boundaries of the */
/* Base64 decoding table.                                                    */
/*                                                                           */
/* Call with:   char    - The Base64 character to decode.                    */
/*                                                                           */
/* Returns:     bool    - True (!0) if the character is valid.               */
/*                        False (0) if the character is not valid.           */
/*---------------------------------------------------------------------------*/
bool b64valid(uchar *c)
{
    if ((*c < 0x2b) || (*c > 0x7a))         /* If not within the range of...*/
        return false;                       /* ...the table, return false.*/
    
    if ((*c = pBase64[*c - 0x2b]) == 0x7f)  /* If it falls within one of...*/
        return false;                       /* ...the gaps, return false.*/

    return true;                            /* Otherwise, return true.*/
}

/*---------------------------------------------------------------------------*/
/* b64buffer - Allocate the decoding or encoding buffer.                     */
/* =====================================================                     */
/*                                                                           */
/* Call this routine to allocate an encoding buffer in 4 byte blocks or a    */
/* decoding buffer in 3 byte octets.  We use "calloc" to initialize the      */
/* buffer to 0x00's for strings.                                             */
/*                                                                           */
/* Call with:   char *  - Pointer to the string to be encoded or decoded.    */
/*              bool    - True (!0) to allocate an encoding buffer.          */
/*                        False (0) to allocate a decoding buffer.           */
/*                                                                           */
/* Returns:     char *  - Pointer to the buffer or NULL if the buffer        */
/*                        could not be allocated.                            */
/*---------------------------------------------------------------------------*/
char *b64buffer(char *s, bool f)
{
    int     l = strlen(s);                  
    char   *b;                              

    if  (!l)                                
        return  NULL;                       

    b = (char *) calloc((f ? b64blocks(l) : b64octets(l)),sizeof(char));
   
    return  b;                              
} 

