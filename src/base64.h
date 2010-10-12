/*---------------------------------------------------------------------------*/
/* base64                                                                    */
/* ======                                                                    */
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
/* Note:    For EBCDIC and other collating sequences, the STRING must first  */
/*          be converted to 7-Bit ASCII before passing it to this module and */
/*          the return string must be converted back to the appropriate      */
/*          collating sequence.                                              */
/*                                                                           */
/* Copyright (c) 1994 - 2003                                        */
/* Marc Niegowski                                                            */
/* Connectivity, Inc.                                                        */
/* All rights reserved.                                                      */

/* Licensing: this file is basically free to use in any way as long the above */
/* copyright notice is kept.                                                  */
/*---------------------------------------------------------------------------*/
#ifndef BASE_64_H_INCLUDED
#define BASE_64_H_INCLUDED


#ifndef __cplusplus
	typedef int bool;
	#define false (1 == 0)
	#define true  !false
#endif

/**
 b64encode - Encode a 7-Bit ASCII string to a Base64 string                                                                                          
 @param char *  - The 7-Bit ASCII string to encode                                     
 @return char* - array that contains the new decoded string     
 Note: the caller needs to free() the resultant pointer!             
*/
char*    b64encode(char *);

#endif 
