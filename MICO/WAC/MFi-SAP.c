//
//  MFi-SAP.c
//
//  Created by William Xu on 14-2-14.
//  Copyright (c) 2014  MXCHIP. All rights reserved.


#include "stdio.h"

#include "debug.h"
#include "MICO.h"
#include "MICODefine.h"
#include "Platform.h"
#include "MFi-SAP.h"
#include "HTTPUtils.h"
#include "WACLogging.h"
#include "MFiSAPServer.h"
#include "WACTLV.h"
#include "TLVUtils.h"
#include "MDNSUtils.h"


typedef enum
{
    eState_start                        = -1,
    eState_Initialize                   = 0,
    eState_WaitingForAuthSetupMessage   = 1,
    eState_HandleAuthSetupMessage       = 2,
    eState_WaitingForConfigMessage      = 3,
    eState_HandleConfigMessage          = 4,
    eState_WaitingTCPFINMessage         = 5,
    eState_WaitingForConfiguredMessage  = 6,
    eState_HandleConfiguredMessage      = 7,
    eState_Complete                     = 8,
    eState_Cleanup                      = 9
} _WACState_t;

int connected_socket=-1;
_WACState_t client_state = eState_start;
extern mico_semaphore_t wac_success;

MFiSAPRef  mfiSAPRef;
extern uint8_t App_Available;

#define http_server_log(M, ...) custom_log("HTTPServer", M, ##__VA_ARGS__)
#define http_server_log_trace() custom_log_trace("HTTPServer")

static OSStatus _WACServerEngine( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext );
static OSStatus _HandleState_WaitingForAuthSetupMessage     ( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext);
static OSStatus _HandleState_HandleAuthSetupMessage         ( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext );
static OSStatus _HandleState_WaitingForConfigMessage        ( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext );
static OSStatus _HandleState_HandleConfigMessage            ( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext );
static OSStatus _HandleState_WaitingForConfiguredMessage    ( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext );
static OSStatus _HandleState_HandleConfiguredMessage        ( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext );

// WAC HTTP messages
#define kWACURLAuth          "/auth-setup"
#define kWACURLConfig        "/config"
#define kWACURLConfigured    "/configured"

char *destinationSSID = NULL;
char *destinationPSK  = NULL;
char *accessoryName   = NULL;
char *playPassword    = NULL;

extern OSStatus applyNewConfiguration(char* destinationSSID, char *destinationPSK, char *accessoryName, char *playPassword, mico_Context_t * const inContext);


static OSStatus _ParseTLVConfigMessage( const void * const inTLVPtr,
                                        const size_t inTLVLen,
                                        char **outSSID,
                                        char **outPSK,
                                        char **outName,
                                        char **outPlayPassword )
{
    wac_log_trace();
    OSStatus                    err = kParamErr;
    const uint8_t *             src = (const uint8_t *) inTLVPtr;
    const uint8_t * const       end = src + inTLVLen;
    uint8_t                     eid;
    const uint8_t *             ptr;
    size_t                      len;
    char *                      tmp;
   

    require( inTLVPtr, exit );
    require( inTLVLen, exit );
    require( outSSID, exit );
    require( outPSK, exit );
    require( outName, exit );
    require( outPlayPassword, exit );

    while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
    {
        tmp = calloc( len + 1, sizeof( uint8_t ) );
        require_action( tmp, exit, err = kNoMemoryErr );
        memcpy( tmp, ptr, len );

        switch( eid )
        {
            case kWACTLV_Name:
                *outName = tmp;
                break;

            case kWACTLV_WiFiSSID:
                *outSSID = tmp;
                break;

            case kWACTLV_WiFiPSK:
                *outPSK = tmp;
                break;

            case kWACTLV_PlayPassword:
                *outPlayPassword = tmp;
                break;

            default:
                // Don't recognize the WAC EID
                free( tmp );
                wac_log( "Warning: Ignoring unsupported WAC EID 0x%02X\n", eid );
                break;
        }
    }

    err = kNoErr;

exit:
    return err;
}




void close_client_connection(void){
  if(connected_socket != -1){
    close(connected_socket);
    connected_socket = -1;
  }
  client_state = eState_start;
}

void close_client_connection_Configured(void){
  //if(connected_socket != -1){
    close(connected_socket);
  //  connected_socket = -1;
  //}
  client_state = eState_WaitingForConfiguredMessage;
}

void http_server_thread(void *inContext)
{
  int i, j, len, fd_listen = -1, err = kNoErr;
  struct sockaddr_t addr;
  int  WaitingForConfigMessage_timeout = 0;
  fd_set readfds;
  struct timeval_t t;
  char ip_address[16];
  mico_Context_t *Context = inContext;

  HTTPHeader_t *httpHeader = NULL;
  
  httpHeader = malloc( sizeof( HTTPHeader_t ) );
  HTTPHeaderClear( httpHeader );
  
  t.tv_sec = 5;
  t.tv_usec = 0;
  
  /*Establish a TCP server that accept the tcp clients connections*/
  if (fd_listen==-1) {
    fd_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    addr.s_ip = INADDR_ANY;
    addr.s_port = 65520;
    bind(fd_listen, &addr, sizeof(addr));
    listen(fd_listen, 0);
    wac_log("WAC server test: HTTP server established at port: %d, fd: %d", addr.s_port, fd_listen);
  }

  
  while(1){
    /*Check status on erery sockets */
    FD_ZERO(&readfds);
    FD_SET(fd_listen, &readfds);  
    if (connected_socket != -1)
      FD_SET(connected_socket, &readfds);
    
    select(1, &readfds, NULL, NULL, &t);
    
    /*Check tcp connection requests, allow only one client connection */
    if(FD_ISSET(fd_listen, &readfds)){
      j = accept(fd_listen, &addr, &len);
      if (j > 0) {
        inet_ntoa(ip_address, addr.s_ip );
        wac_log("TCP server test: Client %s:%d connected, fd: %d\r\n", ip_address, addr.s_port, j);
        if(client_state != eState_WaitingForConfiguredMessage){
            close_client_connection();
            client_state = eState_WaitingForAuthSetupMessage;
        }
        else{
            close_client_connection_Configured();
        }
        connected_socket = j;
            
      }
    }
    
    /*Read data from client */ 
    if (connected_socket != -1) {
      if (FD_ISSET(connected_socket, &readfds)) {
          err = SocketReadHTTPHeader( connected_socket, httpHeader );

          switch ( err )
          {
            case kNoErr:
                // Read the rest of the HTTP body if necessary
                err = SocketReadHTTPBody( connected_socket, httpHeader );
                require_noerr( err, exit );
                PrintHTTPHeader(httpHeader);
                // Call the HTTPServer owner back with the acquired HTTP header
                err = _WACServerEngine( httpHeader, &client_state, Context );
                require_noerr( err, exit );
                // Reuse HTTPHeader
                HTTPHeaderClear( httpHeader );
            break;

            case EWOULDBLOCK:
                // NO-OP, keep reading
            break;

            case kNoSpaceErr:
                http_server_log("ERROR: Cannot fit HTTPHeader.");
                goto exit;
            break;

            case kConnectionErr:
                // NOTE: kConnectionErr from SocketReadHTTPHeader means it's closed
                //err = kNoErr;
                goto exit;
            default:
                http_server_log("ERROR: HTTP Header parse internal error: %d", err);
                goto exit;
        }
          if(err == 0)
            continue;
exit:
        if(client_state == eState_WaitingTCPFINMessage && err == kConnectionErr){
             http_server_log("HTTP connection disconnected 2 .");
            close(connected_socket);
            connected_socket = -1;
            sleep(1);
            applyNewConfiguration(destinationSSID, destinationPSK, accessoryName, playPassword, Context);
            client_state = eState_WaitingForConfiguredMessage;
            if ( destinationSSID ) free( destinationSSID );
            if ( destinationPSK ) free( destinationPSK );
            if ( accessoryName ) free( accessoryName );
            if ( playPassword ) free( playPassword );
            err = kNoErr;
        }
        else{
          http_server_log("HTTP connection disconnected 1 .");
            close_client_connection();
        }
        
      }
      else{
        if(client_state == eState_WaitingForConfigMessage){
          http_server_log("ERROR: HTTP recv ConfigMessage timeout");
          close_client_connection();
        }
      }
    }
    
    
  }
}

OSStatus _WACServerEngine(HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext )
{
    OSStatus    err     = kNoErr;

        switch ( *inState )
        {
            case eState_WaitingForAuthSetupMessage:
                err = _HandleState_WaitingForAuthSetupMessage( inHeader, inState, inContext );
                require_noerr( err, exit );
                err = _HandleState_HandleAuthSetupMessage( inHeader, inState, inContext );
                require_noerr( err, exit );
            break;

            case eState_WaitingForConfigMessage:
                err = _HandleState_WaitingForConfigMessage( inHeader, inState, inContext );
                require_noerr( err, exit );
                err = _HandleState_HandleConfigMessage( inHeader, inState, inContext );
                require_noerr( err, exit );
            break;

            case eState_WaitingForConfiguredMessage:
                 err = _HandleState_WaitingForConfiguredMessage( inHeader, inState, inContext);
                 require_noerr( err, exit );
                 err = _HandleState_HandleConfiguredMessage( inHeader, inState, inContext );
                 require_noerr( err, exit );
            break;

            //case eState_Complete:
            //     err = _HandleState_Complete( inContext, &state );
            //     require_noerr( err, exit );
            //wac_log("COMPLETE");
            //break;

            default:
                wac_log("STATE ERROR");
                //err = kStateErr;
                err = kNoErr;
                //require( 0, exit );
        }

exit:
    return err;
}



static OSStatus _HandleState_WaitingForAuthSetupMessage( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext )
{
    wac_log_trace();
    OSStatus err = kUnknownErr;

    // Check that the HTTP URL is the auth setup URL
    err = HTTPHeaderMatchURL( inHeader, kWACURLAuth );
    require_noerr_action( err, exit, wac_log("ERROR: received different URL. Expecting: %s ", kWACURLAuth); err = kOrderErr );

    wac_log("%s received", kWACURLAuth);
    *inState = eState_HandleAuthSetupMessage;

exit:
    return err;
}

static OSStatus _HandleState_HandleAuthSetupMessage( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext )
{
    wac_log_trace();
    OSStatus err = kUnknownErr;
    Boolean mfiSAPComplete;
    ssize_t sentNum;

    uint8_t *httpResponse = NULL;
    size_t httpResponseLen = 0;

    uint8_t *mfiSAPResponseDataPtr = NULL;
    size_t mfiSAPResponseDataLen = 0;


    // Create MFiSAP
    err = MFiSAP_Create( &mfiSAPRef, kMFiSAPVersion1 );
    require_noerr( err, exit );

    // Do MFiSAP Exchange
    err = MFiSAP_Exchange( mfiSAPRef,                            // MFiSAPRef
                           (uint8_t*)inHeader->extraDataPtr,   // data from auth request
                           inHeader->extraDataLen,             // length of data from auth request
                           &mfiSAPResponseDataPtr,                          // exchange data that should be sent to client
                           &mfiSAPResponseDataLen,                          // length of exchange data that should be sent to client
                           &mfiSAPComplete );                               // Boolean to see if MFiSAP is complete
    require_noerr_action( err, exit, wac_log("ERROR: MFi-SAP Exchange: %d", err) );
    require( mfiSAPComplete, exit );

    err =  CreateSimpleHTTPMessage( kMIMEType_Binary, mfiSAPResponseDataPtr, mfiSAPResponseDataLen, &httpResponse, &httpResponseLen );
    require_noerr( err, exit );
    require( httpResponse, exit );

    sentNum = send( connected_socket, httpResponse, httpResponseLen,0 );
    require( sentNum==(ssize_t)httpResponseLen, exit );
    wac_log("Auth response sent, len= %d", sentNum);

    *inState = eState_WaitingForConfigMessage;
    return err;


exit:
    if ( mfiSAPResponseDataPtr ) free( mfiSAPResponseDataPtr );
    if ( httpResponse ) free( httpResponse );
    return err;
}

static OSStatus _HandleState_WaitingForConfigMessage( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext )
{
    wac_log_trace();
    OSStatus err = kNoErr;

    // Check that the HTTP URL is the config URL
    err = HTTPHeaderMatchURL( inHeader, kWACURLConfig );
    require_noerr_action( err, exit, wac_log("ERROR: received different URL. Expecting: %s", kWACURLConfig); err = kOrderErr );

    wac_log("%s received", kWACURLConfig);

    *inState = eState_HandleConfigMessage;

exit:
    return err;
}

static OSStatus _HandleState_HandleConfigMessage( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext )
{
    wac_log_trace();
    OSStatus err = kParamErr;

    uint8_t *httpResponse = NULL;
    size_t httpResponseLen = 0;


    ssize_t sentNum;

    // Remove the WAC bonjour service
    // err = RemoveWACBonjourService( inContext );
    // require_noerr( err, exit );
    suspend_bonjour_service(ENABLE);


    // Decrypt the /config message
    uint8_t *decryptedConfigData = malloc( inHeader->extraDataLen );
    require_action( decryptedConfigData, exit, err = kNoMemoryErr );
    err = MFiSAP_Decrypt( mfiSAPRef,                 // MFiSAPRef
                          inHeader->extraDataPtr,  // Data to decrypt
                          inHeader->extraDataLen,  // Length of data to decrpyt
                          decryptedConfigData );                // Decrypted data destination pointer
    require_noerr( err, exit );

    // Parse the /config message, TLV format
    err = _ParseTLVConfigMessage( decryptedConfigData,                  // Data pointer to parse
                                  inHeader->extraDataLen,               // Data length
                                  &destinationSSID,                     // Pointer to SSID c string
                                  &destinationPSK,                      // Pointer to PSK c string
                                  &accessoryName,                       // Pointer to name c string
                                  &playPassword );                      // Pointer to name c string
    free( decryptedConfigData );
    require_noerr_action( err, exit, err = kResponseErr );
    require_action( destinationSSID, exit, err = kResponseErr );

    wac_log("Received SSID: %s", destinationSSID);
    if(destinationPSK)
      wac_log("Received PSK: %s", destinationPSK);
    wac_log("Received NAME: %s", accessoryName);
    if(playPassword)
      wac_log("Received PASSWORD: %s", playPassword);

    //If we have an EA response to send
    if ( App_Available )
    {
        // Create the TLV response
        uint8_t *eaTLVResponse;
        size_t  eaTLVResponseLen;
        err = CreateTLVConfigResponseMessage( &eaTLVResponse, &eaTLVResponseLen );
        require_noerr( err, exit );
        require_action( eaTLVResponse, exit, err = kNoMemoryErr );
        require_action( eaTLVResponseLen, exit, err = kNoMemoryErr );

        // Encrypt the TLV response
        uint8_t *encryptedConfigData = malloc( eaTLVResponseLen * sizeof( uint8_t ) );
        require( encryptedConfigData, exit );

        err = MFiSAP_Encrypt( mfiSAPRef,     // MFiSAPRef
                              eaTLVResponse,            // Data to encrypt
                              eaTLVResponseLen,         // Length of data to encrypt
                              encryptedConfigData );    // Encrypted data destination pointer

        err = CreateSimpleHTTPMessage( kMIMEType_TLV8,      // MIME type of message
                                       encryptedConfigData, // encrypted data to send
                                       eaTLVResponseLen,    // length of data to send
                                       &httpResponse,       // pointer to http response
                                       &httpResponseLen );  // length of http response

        free( encryptedConfigData );
        free( eaTLVResponse );
        wac_log("Sending EA /config response");
    }
    else
    {
        err = CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
        wac_log("Sending simple (non-EA) /config response");
    }
    require_noerr( err, exit );

    sentNum = send( connected_socket, httpResponse, httpResponseLen,0 );
    require( sentNum==(ssize_t)httpResponseLen, exit );
    wac_log("Config response sent, len= %d", sentNum);

    *inState = eState_WaitingTCPFINMessage;

    //close_client_connection_Configured();
    //close(connected_socket);

    //require(new_configuration, exit);
    //mico_rtos_set_semaphore(&new_configuration);


exit:
    //if ( inContext->httpServer ) free( inContext->httpServer );
    // malloc'd by CreateSimpleHTTPOKMessage()
    if ( httpResponse ) free( httpResponse );

    return err;
}

static OSStatus _HandleState_WaitingForConfiguredMessage( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext )
{
    wac_log_trace();
    OSStatus err = kNoErr;

    require( inHeader, exit );

    // Check that the HTTP URL is the configured URL
    err = HTTPHeaderMatchURL( inHeader, kWACURLConfigured );
    require_noerr_action( err, exit, wac_log("ERROR: received different URL. Expecting: %s", kWACURLConfigured); err = kOrderErr );

    wac_log("%s received", kWACURLConfigured);

    *inState = eState_HandleConfiguredMessage;

exit:
    return err;
}

static OSStatus _HandleState_HandleConfiguredMessage( HTTPHeader_t* inHeader, _WACState_t *inState, mico_Context_t * const inContext )
{
    wac_log_trace();
    OSStatus err = kUnknownErr;
    ssize_t sentNum;

    uint8_t *httpResponse = NULL;
    size_t httpResponseLen = 0;

    require( inHeader, exit );

    // Remove the WAC bonjour service
    suspend_bonjour_service(ENABLE);

    err = CreateSimpleHTTPOKMessage( &httpResponse, &httpResponseLen );
    require_noerr( err, exit );

    sentNum = send( connected_socket, httpResponse, httpResponseLen,0 );
    require( sentNum==(ssize_t)httpResponseLen, exit );
    wac_log("Config response sent, len= %d", sentNum);

    //err = HTTPServerShutdownSocket( inContext->httpServer );
    //require_noerr( err, exit );

    // Wait on semaphore for the HTTP Server to stop
    //sem_wait( &inContext->httpServerStoppedSemaphore );
    close_client_connection_Configured();
    mico_rtos_set_semaphore(&wac_success);

    *inState = eState_Complete;

    inContext->flashContentInRam.micoSystemConfig.configured = allConfigured;
    MICOUpdateConfiguration(inContext);
    inContext->micoStatus.sys_state = eState_Software_Reset;
    require(inContext->micoStatus.sys_state_change_sem, exit);
    mico_rtos_set_semaphore(&inContext->micoStatus.sys_state_change_sem);

exit:
    // malloc'd by CreateSimpleHTTPOKMessage()
    if ( httpResponse ) free( httpResponse );
    return err;
}



