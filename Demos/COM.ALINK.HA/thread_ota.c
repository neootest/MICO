#include "stdio.h"
#include "string.h"
#include "ctype.h"
#include "MICO.h"
#include "MICODefine.h"
#include "Platform.h"
#include "MICOSocket.h"
#include "StringUtils.h"
#include "HTTPUtils.h"
#include "SocketUtils.h"
#include "alink_vendor_mico.h"

#define ota_log(M, ...) custom_log("OTA", M, ##__VA_ARGS__)
#define ota_log_trace() custom_log_trace("OTA")

static uint8_t needOTA = 0;
static char *httpRequest = NULL;
static HTTPHeader_t *httpHeader = NULL;
static int ota_fd = -1;
char http_req[256], ota_file_name[64];
extern mico_semaphore_t      ota_sem;
uint8_t md5_bin[16];
static uint8_t bp_count = 0;

#define OTA_SERVER "api.easylink.io"
#define OTA_PORT 80

#define kNoOTA                      -6774   //! Local firmware is newest, no need to OTA.
#define kBreakPoint                 -6775   //! Need breakpoint resume

#define OTA_MAX_RECONN_NUM       5

//#define OTA_TEST

#ifdef OTA_TEST
#define OTA_KEY "CF2ABGLICzabesr11111"
#else
#define OTA_KEY ALINK_KEY
#endif

static uint32_t flashStorageAddress;

uint32_t get_writed_length(void)
{
  return flashStorageAddress-UPDATE_START_ADDRESS;
}

static int open_url(char *filename);

const char http_head[]=
{
  "GET /v1/rom/lastversion.json?product_id=%s HTTP/1.1\r\n\
Host: api.easylink.io\r\n\
Connection: keep-alive\r\n\
Accept-Encoding: identity\r\n\r\n"
};

const char http_head_bin[]=
{
  "GET %s HTTP/1.1\r\n\
Host: api.easylink.io\r\n\
Connection: keep-alive\r\n\
Range: bytes=%d-\r\n\
Accept-Encoding: identity\r\n\r\n"
};

OSStatus _connectOTAServer( mico_Context_t * const inContext, int *fd)
{
  ota_log_trace();
  
  OSStatus    err;
  struct      sockaddr_t addr;
  //  const char  *json_str;
  //  char version[40];
  char ipstr[16];
  int recvtime = 1000;
  //  json_object *j_get = json_object_new_object();
  //  json_object *j_action = json_object_new_object();
  //  json_object *j_data = json_object_new_object();
  
  //  size_t      httpRequestLen = 0;
  err = gethostbyname(OTA_SERVER, (uint8_t *)ipstr, 16);
  require_noerr(err, exit);
  //  require_noerr(err, ReConnWithDelay);
  //  mico_rtos_get_semaphore(&login_sem, MICO_WAIT_FOREVER);
  *fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  setsockopt(*fd, SOL_SOCKET, SO_RCVTIMEO, &recvtime, sizeof(recvtime));
  addr.s_ip = inet_addr(ipstr);
  addr.s_port = OTA_PORT;
  err = connect(*fd, &addr, sizeof(addr));
  require_noerr(err, exit);
  
  ota_log("Connect to OTA server success, fd: %d", *fd);
  
  //  snprintf(version, 40, "%s%03d", FIRMWARE_REVISION, FIRMWARE_REVISION_NUM);
  //  json_object_object_add(j_data,"version",json_object_new_string(version));
  //  json_object_object_add(j_data,"model",json_object_new_string(OTA_KEY));
  //
  //  json_object_object_add(j_get,"action",json_object_new_string("last_version"));
  //  json_object_object_add(j_get,"data",j_data);
  //
  //  json_str = json_object_to_json_string(j_get);
  //  require( json_str, exit );
  //
  //  ota_log("Send request object=%s", json_str);
  //
  //  err =  CreateHTTPMessage( "POST", "", kMIMEType_JSON, (uint8_t *)json_str, strlen(json_str), &httpRequest, &httpRequestLen );
  //  require_noerr( err, exit );
  //  require( httpRequest, exit );
  //
  //  json_object_put(j_get);
  if(get_writed_length() > 0){
    require_action(bp_count<10, exit, err = kTimeoutErr);
    bp_count++;
    open_url(ota_file_name);
    SocketSend( *fd, (uint8_t *)http_req, strlen(http_req) );  
  } else{
    httpRequest = malloc(256);
    sprintf(httpRequest, http_head, OTA_KEY);
    err = SocketSend( *fd, (uint8_t *)httpRequest, strlen(httpRequest) );
    ota_log("Request sent = %s", httpRequest);
    free(httpRequest);
    require_noerr( err, exit );
  }
  
exit:
  return err;
}

static int get_filename(char *url, int len)
{
  unsigned char i;
  char *file;
  register char *urlptr;
  char host[64];
  
  url[len] = 0;
  memset(host, 0, 64);
  /* Trim off any spaces in the end of the url. */
  urlptr = url + strlen(url) - 1;
  while(*urlptr == ' ' && urlptr > url) {
    *urlptr = 0;
    --urlptr;
  }
  
  /* Don't even try to go further if the URL is empty. */
  if(urlptr == url) {
    return 0;
  }
  
  /* See if the URL starts with http://, otherwise prepend it. */
  if(strncmp(url, "http://", 7) != 0) {
    //    while(urlptr >= url) {
    //      *(urlptr + 7) = *urlptr;
    //      --urlptr;
    //    }
    //    strncpy(url, "http://", 7);
    //          ota_status = 0;
    //          http_get = 0;
    return 0;
  }
  
  /* Find host part of the URL. */
  urlptr = &url[7];
  for(i = 0; i < sizeof(host); ++i) {
    if(*urlptr == 0 ||
       *urlptr == '/' ||
         *urlptr == ' ' ||
           *urlptr == ':') {
             host[i] = 0;
             break;
           }
    host[i] = *urlptr;
    ++urlptr;
  }
  
  /* Find file part of the URL. */
  while(*urlptr != '/' && *urlptr != 0) {
    ++urlptr;
  }
  if(*urlptr == 0x2f) {
    file = urlptr;
  } else {
    file = "/";
  }
  strncpy(ota_file_name, file, sizeof(ota_file_name));
  return 1;
}

static int open_url(char *filename)
{
  memset(http_req, 0, sizeof(http_req));
  snprintf(http_req, sizeof(http_req), http_head_bin, filename, get_writed_length());
  ota_log("http req =%s", http_req);
  return 1;
}

int hex2data(unsigned char *data, const unsigned char *hexstring, unsigned int len)
{
  unsigned const char *pos = hexstring;
  char *endptr;
  size_t count = 0;
  
  if ((hexstring[0] == '\0') || (strlen((char *)hexstring) % 2)) {
    //hexstring contains no data
    //or hexstring has an odd length
    return kGeneralErr;
  }
  
  for(count = 0; count < len; count++) {
    char buf[5] = {'0', 'x', pos[0], pos[1], 0};
    data[count] = strtol(buf, &endptr, 0);
    pos += 2 * sizeof(char);
    
    if (endptr[0] != '\0') {
      //non-hexadecimal character encountered
      return kGeneralErr;
    }
  }
  
  return kNoErr;
}

static int ota_finished(uint8_t *md5_recv, uint8_t *temp_buf, int temp_buf_len, void * inUserContext)
{
    uint8_t md5_ret[16];
    md5_context ctx;
    int len;
    uint32_t offset = UPDATE_START_ADDRESS;
    mico_Context_t *context = (mico_Context_t *)inUserContext;

    ota_log("Receive OTA data done!");
    InitMd5( &ctx );
    while((len = flashStorageAddress - offset) > 0) {
        if (temp_buf_len < len) {
            len = temp_buf_len;
        }
        MicoFlashRead(MICO_FLASH_FOR_UPDATE, &offset, (uint8_t *)temp_buf, len);
        Md5Update( &ctx, (uint8_t *)temp_buf, len);
    }
    Md5Final( &ctx, md5_ret );
    
    if(memcmp(md5_ret, md5_recv, 16) != 0) {
        return kGeneralErr;
    }
    
    memset(&context->flashContentInRam.bootTable, 0, sizeof(boot_table_t));
    context->flashContentInRam.bootTable.length = get_writed_length();
    context->flashContentInRam.bootTable.start_address = UPDATE_START_ADDRESS;
    context->flashContentInRam.bootTable.type = 'A';
    context->flashContentInRam.bootTable.upgrade_type = 'U';
    MICOUpdateConfiguration(context);
    ota_log("OTA Success!");
    context->micoStatus.sys_state = eState_Software_Reset;
    if (context->micoStatus.sys_state_change_sem);
        mico_rtos_set_semaphore(&context->micoStatus.sys_state_change_sem);

    while(1) // dead loop, wait system reset.
        sleep(100);
}


static OSStatus onReceivedData(struct _HTTPHeader_t * inHeader, uint32_t inPos, uint8_t * inData, size_t inLen, void * inUserContext )
{
  OSStatus err = kUnknownErr;
  const char *    value;
  size_t          valueSize;

  err = HTTPGetHeaderField( inHeader->buf, inHeader->len, "Content-Type", NULL, NULL, &value, &valueSize, NULL );
  if(err == kNoErr && strnicmpx( value, valueSize, kMIMEType_Stream ) == 0){
#ifdef MICO_FLASH_FOR_UPDATE  
    err = MicoFlashWrite(MICO_FLASH_FOR_UPDATE, &flashStorageAddress, (uint8_t *)inData, inLen);
    require_noerr(err, flashErrExit);
#else
    ota_log("OTA storage is not exist");
    return kUnsupportedErr;
#endif
  }
  else if(inHeader->chunkedData == true){
    ota_log("ChunkedData: %d, %d:", inPos, inLen);
  }
  else{
    return kUnsupportedErr;
  }

  if(err!=kNoErr)  ota_log("onReceivedData");
  return err;

#ifdef MICO_FLASH_FOR_UPDATE  
flashErrExit:
  MicoFlashFinalize(MICO_FLASH_FOR_UPDATE);
  return err;
#endif
}


extern void alink_string_to_standard(char *str);
OSStatus OTAIncommingJsonMessage( const char *input, mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;
  json_object *new_obj;
  ota_log_trace();
  char tempStr[100];
  
  new_obj = json_tokener_parse(input);
  require_action(new_obj, exit, err = kUnknownErr);
  ota_log("Recv config object=%s", json_object_to_json_string(new_obj));
  json_object_object_foreach(new_obj, key, val) {
    if(!strcmp(key, "version")){
      err = strncmp(json_object_get_string(val), FIRMWARE_REVISION, strlen(FIRMWARE_REVISION));
      require_noerr_action(err, exit, err = kGeneralErr);
      strncpy(tempStr, json_object_get_string(val), sizeof(tempStr));
      require_action(atoi(strstr(tempStr, "@")+1)>FIRMWARE_REVISION_NUM, exit, err = kGeneralErr);
      needOTA = 1;
    }else if(!strcmp(key, "bin_md5")){
      err = hex2data(md5_bin, (uint8_t *)json_object_get_string(val), sizeof(md5_bin));
      require_noerr(err, exit);
      ota_log("md5 = %s", json_object_get_string(val));
    }else if(!strcmp(key, "bin")){
      strncpy(tempStr, json_object_get_string(val), 100);
      alink_string_to_standard(tempStr);
      get_filename(tempStr,strlen(json_object_get_string(val)));
      open_url(ota_file_name);
    }else if(!strcmp(key, "error")){
      err = kGeneralErr;
      goto exit;
    }
  }
  if(needOTA){
    SocketSend( ota_fd, (uint8_t *)http_req, strlen(http_req) );
  }
exit:
  json_object_put(new_obj);
  return err;
}


OSStatus _OTARespondInComingMessage(int fd, HTTPHeader_t* inHeader, mico_Context_t * const inContext)
{
  OSStatus            err = kUnknownErr;
  const char *        value;
  size_t              valueSize;
  char                *p_content;
  
  ota_log_trace();
  ota_log("Receive OTA statusCode %d\r\n", inHeader->statusCode);
  switch(inHeader->statusCode){
  case kStatusAccept:
    ota_log("OTA server accepted!");
    err = kNoErr;
    goto exit;

  case kStatusOK:
    ota_log("OTA server respond status OK!");
    err = HTTPGetHeaderField( inHeader->buf, inHeader->len, "Content-Type", NULL, NULL, &value, &valueSize, NULL );
    require_noerr(err, exit);
    if( strnicmpx( value, 16, kMIMEType_JSON ) == 0 ){
      ota_log("Receive JSON version data!");
      p_content = strstr((char *)inHeader, "\r\n\r\n")+4;
      require(p_content, exit);
      p_content = strstr(p_content, "{");
      require(p_content, exit);
      err = OTAIncommingJsonMessage(p_content , inContext);
      require_noerr(err, exit);
    }else if(strnicmpx( value, 24, kMIMEType_Stream ) == 0){
      err = ota_finished(md5_bin, (uint8_t*)inHeader->buf, sizeof(inHeader->buf), inContext);
      if(err != kNoErr) {
        ota_log("MD5 check error!");
        MicoFlashFinalize(MICO_FLASH_FOR_UPDATE);
        err = kGeneralErr;
        goto exit;
      }
    }else{
      return kUnsupportedDataErr;
    }
    err = kNoErr;
    goto exit;
    
  case kStatusPartialContent:
    ota_log("kStatusPartialContent!");
    err = HTTPGetHeaderField( inHeader->buf, inHeader->len, "Content-Type", NULL, NULL, &value, &valueSize, NULL );
    require_noerr(err, exit);
    if(strnicmpx( value, 24, kMIMEType_Stream ) == 0){
      err = ota_finished(md5_bin, (uint8_t*)inHeader->buf, sizeof(inHeader->buf), inContext);
      if(err != kNoErr) {
        ota_log("MD5 check error!");
        MicoFlashFinalize(MICO_FLASH_FOR_UPDATE);
        err = kGeneralErr;
        goto exit;
      }
    }
    else{
      return kUnsupportedDataErr;
    }
    break;
  case kStatusNotFound:
    err = kNoOTA;
    goto exit;
    
  default:
    goto exit;
  }
  
exit:
  return err;
  
}

void ota_thread(void *inContext)
{
  ota_log_trace();
  
  OSStatus err = kNoErr;
  mico_Context_t *context = inContext;
  fd_set readfds;
  struct timeval_t t;
  int reConnCount = 0;

  flashStorageAddress = UPDATE_START_ADDRESS;
  err = MicoFlashInitialize( MICO_FLASH_FOR_UPDATE );
  require_noerr(err, threadexit);
  
  httpHeader = HTTPHeaderCreateWithCallback(onReceivedData, NULL, inContext);
  require_action( httpHeader, threadexit, err = kNoMemoryErr );
  HTTPHeaderClear( httpHeader );
  
  t.tv_sec = 100;
  t.tv_usec = 0;
  
  while(1){
    if(ota_fd == -1){
      err = _connectOTAServer(inContext, &ota_fd);
      if(err == kTimeoutErr)
        goto threadexit;
      else
        require_noerr(err, Reconn);
    }else{
      FD_ZERO(&readfds);
      FD_SET(ota_fd, &readfds);
      
      err = select(1, &readfds, NULL, NULL, &t);
      
      if(FD_ISSET(ota_fd, &readfds)){
        err = SocketReadHTTPHeader( ota_fd, httpHeader );          
        
        switch ( err )
        {
        case kNoErr:
          // Read the rest of the HTTP body if necessary
          err = SocketReadHTTPBody( ota_fd, httpHeader );
          if(err == kBreakPoint){
            SocketClose(&ota_fd);
            ota_fd = -1;
            HTTPHeaderClear( httpHeader );
            httpHeader = HTTPHeaderCreateWithCallback(onReceivedData, NULL, inContext);
            require_action( httpHeader, threadexit, err = kNoMemoryErr );
            msleep(500);
            continue;
          }else
            require_noerr(err, Reconn);
          PrintHTTPHeader(httpHeader);
          // Call the HTTPServer owner back with the acquired HTTP header
          err = _OTARespondInComingMessage( ota_fd, httpHeader, context );
          require_noerr( err, threadexit );
          // Reuse HTTPHeader
          HTTPHeaderClear( httpHeader );
          break;
          
        case EWOULDBLOCK:
          // NO-OP, keep reading
          break;
          
        case kNoSpaceErr:
          ota_log("ERROR: Cannot fit HTTPHeader.");
          goto threadexit;
                    
        case kConnectionErr:
          // NOTE: kConnectionErr from SocketReadHTTPHeader means it's closed
          ota_log("ERROR: Connection closed.");
          goto threadexit;
          
        case kNoOTA:
          goto threadexit;
          
        default:
          ota_log("ERROR: HTTP Header parse internal error: %d", err);
          goto threadexit;
        }
      }
    }
    continue;
  Reconn:
    HTTPHeaderClear( httpHeader );
    SocketClose(&ota_fd);
    require(reConnCount < OTA_MAX_RECONN_NUM, threadexit);
    reConnCount++;
    sleep(3);
  }
  
  /*Module is ignored by FTC server, */
threadexit:
  HTTPHeaderClear( httpHeader );
  SocketClose(&ota_fd);
  ota_fd = -1;
  mico_rtos_set_semaphore(&ota_sem);
  mico_rtos_delete_thread( NULL );
  return;
}


