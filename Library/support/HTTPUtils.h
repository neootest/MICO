/**
******************************************************************************
* @file    HTTPUtils.h 
* @author  William Xu
* @version V1.0.0
* @date    05-May-2014
* @brief   This header contains function prototypes. These functions assist 
  with interacting with HTTP clients and servers.
******************************************************************************
* @attention
*
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
* TIME. AS A RESULT, MXCHIP Inc. SHALL NOT BE HELD LIABLE FOR ANY
* DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
* FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
* CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
* <h2><center>&copy; COPYRIGHT 2014 MXCHIP Inc.</center></h2>
******************************************************************************
*/ 


#ifndef __HTTPUtils_h__
#define __HTTPUtils_h__

#include "Common.h"

#include "URLUtils.h"
#include "stdbool.h"

#define kHTTPPostMethod     "POST"

      // Status-Code    =
      //       "100"  ; Section 10.1.1: Continue
      //     | "101"  ; Section 10.1.2: Switching Protocols
      //     | "200"  ; Section 10.2.1: OK
      //     | "201"  ; Section 10.2.2: Created
      //     | "202"  ; Section 10.2.3: Accepted
      //     | "203"  ; Section 10.2.4: Non-Authoritative Information
      //     | "204"  ; Section 10.2.5: No Content
      //     | "205"  ; Section 10.2.6: Reset Content
      //     | "206"  ; Section 10.2.7: Partial Content
      //     | "300"  ; Section 10.3.1: Multiple Choices
      //     | "301"  ; Section 10.3.2: Moved Permanently
      //     | "302"  ; Section 10.3.3: Found
      //     | "303"  ; Section 10.3.4: See Other
      //     | "304"  ; Section 10.3.5: Not Modified
      //     | "305"  ; Section 10.3.6: Use Proxy
      //     | "307"  ; Section 10.3.8: Temporary Redirect
      //     | "400"  ; Section 10.4.1: Bad Request
      //     | "401"  ; Section 10.4.2: Unauthorized
      //     | "402"  ; Section 10.4.3: Payment Required
      //     | "403"  ; Section 10.4.4: Forbidden
      //     | "404"  ; Section 10.4.5: Not Found
      //     | "405"  ; Section 10.4.6: Method Not Allowed
      //     | "406"  ; Section 10.4.7: Not Acceptable


#define kStatusAccept       202
#define kStatusOK           200
#define kStatusForbidden    403      

#define kMIMEType_Binary                "application/octet-stream"
#define kMIMEType_DMAP                  "application/x-dmap-tagged"
#define kMIMEType_ImagePrefix           "image/"
#define kMIMEType_JSON                  "application/json"
#define kMIMEType_SDP                   "application/sdp"
#define kMIMEType_TextHTML              "text/html"
#define kMIMEType_TextParameters        "text/parameters"
#define kMIMEType_TextPlain             "text/plain"
#define kMIMEType_TLV8                  "application/x-tlv8" // 8-bit type, 8-bit length, N-byte value.
#define kMIMEType_MXCHIP_OTA            "application/ota-stream"


typedef struct
{
    char                buf[ 2048 ];        //! Buffer holding the start line and all headers.
    size_t              len;                //! Number of bytes in the header.
    const char *        extraDataPtr;       //! Ptr within "buf" for any extra data beyond the header.
    size_t              extraDataLen;       //! Length of any extra data beyond the header.

    const char *        methodPtr;          //! Request method (e.g. "GET"). "$" for interleaved binary data.
    size_t              methodLen;          //! Number of bytes in request method.
    const char *        urlPtr;             //! Request absolute or relative URL or empty if not a request.
    size_t              urlLen;             //! Number of bytes in URL.
    URLComponents       url;                //! Parsed URL components.
    const char *        protocolPtr;        //! Request or response protocol (e.g. "HTTP/1.1").
    size_t              protocolLen;        //! Number of bytes in protocol.
    int                 statusCode;         //! Response status code (e.g. 200 for HTTP OK).
    const char *        reasonPhrasePtr;    //! Response reason phrase (e.g. "OK" for an HTTP 200 OK response).
    size_t              reasonPhraseLen;    //! Number of bytes in reason phrase.

    uint8_t             channelID;          //! Interleaved binary data channel ID. 0 for other message types.
    uint64_t            contentLength;      //! Number of bytes following the header. May be 0.
    bool             persistent;         //! true=Do not close the connection after this message.

    int            firstErr;           //! First error that occurred or kNoErr.

} HTTPHeader_t;

void PrintHTTPHeader( HTTPHeader_t *inHeader );

int HTTPScanFHeaderValue( const char *inHeaderPtr, size_t inHeaderLen, const char *inName, const char *inFormat, ... );

int SocketReadHTTPHeader( int inSock, HTTPHeader_t *inHeader );

int SocketReadHTTPBody( int inSock, HTTPHeader_t *inHeader );

int HTTPHeaderParse( HTTPHeader_t *ioHeader );

int HTTPHeaderMatchMethod( HTTPHeader_t *inHeader, const char *method );

int HTTPHeaderMatchURL( HTTPHeader_t *inHeader, const char *url );

int HTTPGetHeaderField( const char *inHeaderPtr, 
                             size_t     inHeaderLen, 
                             const char *inName, 
                             const char **outNamePtr, 
                             size_t     *outNameLen, 
                             const char **outValuePtr, 
                             size_t     *outValueLen, 
                             const char **outNext );

void HTTPHeaderClear( HTTPHeader_t *inHeader );

int CreateSimpleHTTPOKMessage( uint8_t **outMessage, size_t *outMessageSize );

OSStatus CreateSimpleHTTPMessage      ( const char *contentType, uint8_t *inData, size_t inDataLen, uint8_t **outMessage, size_t *outMessageSize );
OSStatus CreateSimpleHTTPMessageNoCopy( const char *contentType, size_t inDataLen, uint8_t **outMessage, size_t *outMessageSize );


OSStatus CreateHTTPMessage( const char *methold, const char *url, const char *contentType, uint8_t *inData, size_t inDataLen, uint8_t **outMessage, size_t *outMessageSize );

#endif // __HTTPUtils_h__

