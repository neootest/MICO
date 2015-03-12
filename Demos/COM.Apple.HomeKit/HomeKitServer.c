/**
  ******************************************************************************
  * @file    MICOConfigServer.c 
  * @author  William Xu
  * @version V1.0.0
  * @date    05-May-2014
  * @brief   Local TCP server for mico device configuration 
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


#include "MICO.h"
#include "MICODefine.h"
#include "HomeKitPairList.h"
#include "MICOAppDefine.h"
#include "SocketUtils.h"
#include "StringUtils.h"
#include "HomeKitHTTPUtils.h"
#include "HomeKitPairProtocol.h"
#include "HomeKitProfiles.h"
#include "URLUtils.h"

#define ha_log(M, ...) custom_log("HomeKit", M, ##__VA_ARGS__)
#define ha_log_trace() custom_log_trace("HomeKit")

#define kPAIRSETUP          "/pair-setup"
#define kPAIRVERIFY         "/pair-verify"
#define kPAIRINGS           "/pairings"
#define kReadAcc            "/accessories"
#define kRWCharacter        "/characteristics" 
#define kIdentity           "/identify"  

#define min(a,b) ((a) < (b) ? (a) : (b))

/* Raw type password */
static const char *password = "454-45-454";

/* Verifier and salt for raw type password "454-45-454" */
static const uint8_t verifier[] =  {0x2C,0x21,0x47,0x4C,0x67,0x0B,0xEF,0x8F,0xA6,0x4A,0xF9,0x61,0x46,0xDA,0xB4,0x30,
                                    0x9B,0xB0,0x73,0x35,0xC6,0x7C,0xB5,0xA9,0xFC,0x7A,0x2A,0x02,0xC1,0x04,0x3A,0x49,
                                    0x9E,0xEA,0x6B,0x7F,0x52,0xDD,0xBF,0x75,0xB6,0x4A,0xB6,0x7D,0x78,0x39,0x09,0x6C,
                                    0x6B,0x26,0x9B,0x20,0xE8,0x3B,0xB6,0x13,0xCD,0x2F,0x9C,0x3E,0x08,0x93,0x9F,0x3C,
                                    0x76,0xDA,0x75,0x40,0xD6,0xC9,0xAB,0x76,0xF5,0xD8,0x18,0xF4,0xD6,0x16,0xE1,0xDF,
                                    0xD9,0x99,0xD1,0x16,0xEC,0xE0,0x76,0x04,0x83,0xC1,0x58,0x42,0x17,0x4F,0x0A,0x8B,
                                    0x8A,0xC7,0xF5,0xAA,0x76,0xA7,0x7E,0xF0,0xE8,0x1D,0xBB,0xD6,0xBE,0xCF,0xA5,0x7B,
                                    0x12,0x72,0xAD,0x2B,0x9B,0x5C,0xAB,0x12,0x9E,0xE1,0x1B,0xF8,0x92,0xB0,0xF4,0xDA,
                                    0x37,0x38,0x7C,0x0D,0x6F,0x8F,0xE1,0xF4,0x0D,0xB4,0x4E,0xFD,0x4B,0xB5,0xEE,0xB1,
                                    0xD5,0xBF,0x0A,0xC9,0xCB,0xC9,0x2D,0xA6,0xFC,0x6B,0x51,0x32,0x07,0x86,0x29,0x09,
                                    0xAA,0x02,0x56,0x83,0x26,0xAC,0x55,0xD8,0x21,0x61,0x1A,0xE4,0xFF,0x8B,0x4F,0xDA,
                                    0x48,0x89,0x75,0x05,0x5B,0xD0,0x76,0x98,0xA1,0xC7,0x72,0x8D,0xD3,0x6C,0x7C,0x86,
                                    0x3A,0x69,0x47,0xD0,0xEA,0xED,0x95,0x82,0x36,0x67,0xEF,0x94,0xEB,0x13,0xCC,0x96,
                                    0xB9,0x9C,0x1E,0x30,0xF9,0x14,0x75,0xBA,0x71,0x41,0x7E,0x81,0xD2,0x10,0xC5,0xF9,
                                    0x37,0x20,0xC9,0x78,0x28,0xF9,0x67,0x47,0x69,0x5D,0xB9,0xBF,0x6D,0x86,0x40,0x90,
                                    0xC4,0x97,0x38,0x4E,0x13,0x6F,0xE8,0xB3,0xA8,0x6C,0x8C,0xF9,0x6B,0xD2,0x46,0x13,
                                    0xC8,0x80,0x90,0xCE,0x72,0xB3,0xCF,0x04,0xC8,0x62,0x1B,0x80,0xA2,0x0C,0x60,0xAA,
                                    0xD3,0x5E,0x6E,0x68,0xB1,0x57,0xFA,0x58,0x3D,0x10,0x5F,0x2B,0xA5,0x5C,0x34,0xD0,
                                    0x55,0x53,0x01,0x01,0xA6,0xEA,0x0A,0xD1,0x3F,0x61,0xAF,0xCE,0x9E,0x57,0xAE,0xE5,
                                    0xF6,0x88,0x86,0x23,0x1A,0x76,0x56,0x3B,0x9F,0x60,0x6D,0x8F,0x38,0xA0,0xAD,0x14,
                                    0x1C,0x82,0x5B,0xC9,0xA3,0xAE,0x7A,0x3E,0xB3,0xE1,0x24,0x16,0xBE,0x0D,0x07,0x8E,
                                    0xFA,0xBF,0xBD,0x4C,0xA0,0x39,0xE5,0xC1,0x8F,0x4C,0xAE,0xD5,0x7D,0xAB,0x74,0x29,
                                    0xE3,0x15,0xC4,0xC6,0x57,0x14,0x94,0xA7,0xAB,0x1E,0x05,0x85,0x93,0x2A,0x8A,0x72,
                                    0xC8,0x1C,0x96,0x9D,0xAA,0x03,0xD6,0x82,0x54,0x64,0x7E,0x6F,0xBB,0xD1,0xB4,0x1A};

const uint8_t salt[] = {0x8F,0xCC,0xC9,0xC7,0xEA,0x97,0xA7,0x76,0xE5,0xCA,0x9C,0xF0,0x69,0x0C,0x0A,0xAF};


extern struct _hapAccessory_t hapObjects[];

typedef struct _HK_Context_t {
  pairInfo_t          *pairInfo;
  pairVerifyInfo_t    *pairVerifyInfo;
  security_session_t  *session;
} HK_Context_t;

typedef struct _HK_Char_ID_t {
  int          aid;
  int          iid;
  int          serviceID;
  int          characteristicID;
} HK_Char_ID_t;

typedef struct _HK_Notify{
  int aid;
  int iid;
  value_union value;
  struct _HK_Notify *next;
} HK_Notify_t;

extern void HKCharacteristicInit(mico_Context_t * const inContext);
extern HkStatus HKReadCharacteristicValue(int accessoryID, int serviceID, int characteristicID, value_union *value, mico_Context_t * const inContext);
extern void HKWriteCharacteristicValue(int accessoryID, int serviceID, int characteristicID, value_union value, bool moreComing, mico_Context_t * const inContext);
extern HkStatus HKReadCharacteristicStatus(int accessoryID, int serviceID, int characteristicID, mico_Context_t * const inContext);
extern HkStatus HKExcuteUnpairedIdentityRoutine( mico_Context_t * const inContext );


static void homeKitClient_thread(void *inFd);
static mico_Context_t *Context;
static OSStatus HKhandleIncomeingMessage(int sockfd, HTTPHeader_t *httpHeader, HK_Notify_t** notifyList, HK_Context_t *inHkContext, mico_Context_t * const inContext);
static OSStatus HKCreateHAPAttriDataBase( struct _hapAccessory_t *inHapObject,  HK_Notify_t* notifyList, json_object **OutHapObjectJson,  mico_Context_t * const inContext);
static OSStatus HKCreateHAPReadRespond( struct _hapAccessory_t inHapObject[],  json_object **OutHapObjectJson, 
                                                int accessoryID, int serviceID, int characteristicID, mico_Context_t * const inContext);
static OSStatus HKCreateHAPWriteRespond( struct _hapAccessory_t inHapObject[],  json_object *inputHapObjectJson, json_object **OutHapObjectJson,
                                                int accessoryID, int serviceID, int characteristicID, mico_Context_t * const inContext);
void _HKReadCharacteristicValue_respond(struct _hapAccessory_t inHapObject[], int accessoryID, int serviceID, 
                                        int characteristicID, json_object **OutHapObjectJson, mico_Context_t * const inContext);
void _HKWriteCharacteristicValue_respond(struct _hapAccessory_t inHapObject[], int accessoryID, int serviceID, 
                                        int characteristicID, json_object **OutHapObjectJson, mico_Context_t * const inContext);


void homeKitListener_thread(void *inContext)
{
  ha_log_trace();
  OSStatus err = kUnknownErr;
  int j;
  Context = inContext;
  struct sockaddr_t addr;
  int sockaddr_t_size;
  fd_set readfds;
  char ip_address[16];
  
  int homeKitlistener_fd = -1;
  //HKSetPassword (password, strlen(password));
  HKSetVerifier(verifier, sizeof(verifier), salt, sizeof(salt));

  Context->appStatus.haPairSetupRunning = false;
  HKCharacteristicInit(inContext);
  /*Establish a TCP server fd that accept the tcp clients connections*/ 
  homeKitlistener_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
  require_action(IsValidSocket( homeKitlistener_fd ), exit, err = kNoResourcesErr );
  addr.s_ip = INADDR_ANY;
  addr.s_port = HA_SERVER_PORT;
  err = bind(homeKitlistener_fd, &addr, sizeof(addr));
  require_noerr( err, exit );

  err = listen(homeKitlistener_fd, 0);
  require_noerr( err, exit );

  ha_log("HomeKit Server established at port: %d, fd: %d", HA_SERVER_PORT, homeKitlistener_fd);
  
  while(1){
    FD_ZERO(&readfds);
    FD_SET(homeKitlistener_fd, &readfds);
    select(1, &readfds, NULL, NULL, NULL);

    /*Check tcp connection requests */
    if(FD_ISSET(homeKitlistener_fd, &readfds)){
      sockaddr_t_size = sizeof(struct sockaddr_t);
      j = accept(homeKitlistener_fd, &addr, &sockaddr_t_size);
      if (j > 0) {
        inet_ntoa(ip_address, addr.s_ip );
        ha_log("HomeKit Client %s:%d connected, fd: %d", ip_address, addr.s_port, j);
        ha_log("memory>>>>>>>>: %d", mico_memory_info()->free_memory);
        err = mico_rtos_create_thread(NULL, MICO_APPLICATION_PRIORITY, "HomeKit Client", homeKitClient_thread, 0x1000, &j);  
        if(err != kNoErr){
          ha_log("HomeKit Client for fd %d create failed", j);
          SocketClose(&j);
        }
      }
    }
   }

exit:
    ha_log("Exit: HomeKit Server exit with err = %d", err);
    mico_rtos_delete_thread(NULL);
    return;
}

void FindCharacteristicByIID(struct _hapAccessory_t inHapObject[], int aid, int iid, int *serviceID, int *characteristicID)
{
  int serviceIndex, characteristicIndex, tmpIID;
  tmpIID = 1;
  *serviceID = 0;
  *characteristicID = 0;

  for(serviceIndex = 0; serviceIndex < MAXServicePerAccessory; serviceIndex++){
    if(inHapObject[aid-1].services[serviceIndex].type == 0 || tmpIID > iid)
      return;
    if(tmpIID == iid){
      *serviceID = serviceIndex + 1;
      return;
    }
    tmpIID ++;

    for(characteristicIndex = 0; characteristicIndex < MAXCharacteristicPerService; characteristicIndex++){
      if(inHapObject[aid-1].services[serviceIndex].characteristic[characteristicIndex].type == 0 )
        break;
      if(tmpIID > iid)
        return;
      if(tmpIID == iid){
        *serviceID = serviceIndex + 1;
        *characteristicID = characteristicIndex + 1;;
        return;
      }
      tmpIID ++;
    }
  }
}

OSStatus HKNotificationAdd( int aid, int iid, value_union value, HK_Notify_t** notifyList )
{
  OSStatus err = kNoErr;
  HK_Notify_t *notify = (HK_Notify_t *)malloc(sizeof(HK_Notify_t));
  HK_Notify_t *tmp = * notifyList;
  require_action(notify, exit, err = kNoMemoryErr);
  notify->aid = aid;
  notify->iid = iid;
  notify->value = value;
  notify->next = NULL;
  if(* notifyList == NULL){
    * notifyList = notify;
  }else{
    if(tmp->aid == aid && tmp->iid == iid){
      free(notify);
      memcpy(&tmp->value, &value, sizeof(value_union));
      return kNoErr;   //Nodify already exist
    }
    while(tmp->next!=NULL){
      tmp = tmp->next;
      if(tmp->aid == aid && tmp->iid == iid){
        memcpy(&tmp->value, &value, sizeof(value_union));
        free(notify);
        return kNoErr;   //Nodify already exist
      }
    }
    tmp->next = notify;
  }
exit:
  return err;
}

OSStatus HKNotificationRemove( int aid, int iid, HK_Notify_t** notifyList )
{
  OSStatus err = kNoErr;
  HK_Notify_t *temp2;
  HK_Notify_t *temp = *notifyList;
  require_action(temp, exit, err = kDeletedErr);
  do{
    if(temp->aid == aid && temp->iid == iid){
      if(temp == *notifyList){  //first element
        * notifyList = temp->next;
        free(temp);
      }else{
        temp2->next = temp->next;
        free(temp);
      }
       break;
    }
    require_action(temp->next!=NULL, exit, err = kNotFoundErr);
    temp2 = temp;
    temp = temp->next;
  }while(temp!=NULL);

exit:
  return err;
}

OSStatus HKNotifyGetNext( 
        HK_Notify_t *   notifyList, 
        int *           outAID, 
        int *           outIID, 
        value_union *   value, 
        HK_Notify_t **  outNextNotifyList )
{
  if(notifyList == NULL) return kNotFoundErr;
  *outAID = notifyList->aid;
  *outIID = notifyList->iid;
  *value = notifyList->value;
  *outNextNotifyList = notifyList->next;
  return kNoErr;
}

OSStatus HKNotificationFind( int aid, int iid, HK_Notify_t* notifyList )
{
  OSStatus err = kNotFoundErr;
  int _aid, _iid;
  value_union _value;
  HK_Notify_t* _notifyList = notifyList;

  while(HKNotifyGetNext( _notifyList, &_aid, &_iid, &_value, &_notifyList) == kNoErr){
    if(_aid == aid && _iid == iid)
      return kNoErr;
  }
  return err;
}

OSStatus HKNotificationClean( HK_Notify_t** notifyList )
{
  HK_Notify_t* temp = *notifyList;
  HK_Notify_t* temp2;
  if(*notifyList == NULL) return kNoErr;
  do{
    temp2 = temp->next;
    free(temp);
    temp = temp2;
  }while(temp!=NULL);    

exit:
  * notifyList = NULL;
  return kNoErr;
}

void homeKitClient_thread(void *inFd)
{
  ha_log_trace();
  OSStatus err;
  int clientFd = *(int *)inFd;
  struct timeval_t t;
  HTTPHeader_t *httpHeader = NULL;
  int selectResult;
  fd_set      readfds;
  HK_Context_t hkContext;
  int aid, iid, serviceID, characteristicID;
  value_union value, newValue;
  struct _hapCharacteristic_t pCharacteristic;
  json_object *outEventJsonObject = NULL, *outCharacteristics, *outCharacteristic;
  const char *buffer = NULL;

  memset(&hkContext, 0x0, sizeof(HK_Context_t));
  hkContext.session = HKSNewSecuritySession();
  require_action(hkContext.session, exit, err = kNoMemoryErr);

  httpHeader = calloc(1, sizeof( HTTPHeader_t ) );
  require_action( httpHeader, exit, err = kNoMemoryErr );

  HK_Notify_t *notifyList = NULL, *temp;

  bool isChanged;

  t.tv_sec = 1;
  t.tv_usec = 0;  //Check for notify every 1 second
  ha_log("Free memory1: %d", mico_memory_info()->free_memory);

  while(1){
    if(hkContext.session->established == true && hkContext.session->recvedDataLen > 0){
       err = HKhandleIncomeingMessage(clientFd, httpHeader, &notifyList, &hkContext, Context);
    }else{
      FD_ZERO(&readfds);
      FD_SET(clientFd, &readfds);
      selectResult = select(clientFd + 1, &readfds, NULL, NULL, &t);
      require( selectResult >= 0, exit );
      if(FD_ISSET(clientFd, &readfds)){
        err = HKhandleIncomeingMessage(clientFd, httpHeader, &notifyList, &hkContext, Context);
        require_noerr(err, exit);
      }

      if(hkContext.session->established == false) // No nofification in no paired session
        continue;

      outEventJsonObject = json_object_new_object();
      require_action(outEventJsonObject, exit, err = kNoMemoryErr);
      outCharacteristics = json_object_new_array();
      require_action(outCharacteristics, exit, err = kNoMemoryErr);
      json_object_object_add( outEventJsonObject, "characteristics", outCharacteristics);
      temp = notifyList;

      while(HKNotifyGetNext( temp, &aid, &iid, &value, &temp) == kNoErr){
        FindCharacteristicByIID(hapObjects, aid, iid, &serviceID, &characteristicID);   
        if(HKReadCharacteristicValue(aid, serviceID, characteristicID, &newValue, Context)==kHKNoErr){
          pCharacteristic = ((hapObjects[aid-1]).services[serviceID-1]).characteristic[characteristicID-1];

          switch(pCharacteristic.valueType){
            case ValueType_bool:
              if( value.boolValue != newValue.boolValue ){
                outCharacteristic = json_object_new_object();
                json_object_object_add( outCharacteristic, "aid", json_object_new_int(aid));
                json_object_object_add( outCharacteristic, "iid", json_object_new_int(iid));
                json_object_object_add( outCharacteristic, "value", json_object_new_boolean(newValue.boolValue));
                json_object_array_add( outCharacteristics, outCharacteristic );
                HKNotificationAdd( aid, iid, newValue, &notifyList );
              }
              break;
            case ValueType_int:
              if( value.intValue != newValue.intValue ){
                outCharacteristic = json_object_new_object();
                json_object_object_add( outCharacteristic, "aid", json_object_new_int(aid));
                json_object_object_add( outCharacteristic, "iid", json_object_new_int(iid));
                json_object_object_add( outCharacteristic, "value", json_object_new_int(newValue.intValue));
                json_object_array_add( outCharacteristics, outCharacteristic );
                HKNotificationAdd( aid, iid, newValue, &notifyList );
              }
              break;
            case ValueType_float:
              if( value.floatValue != newValue.floatValue ){
                outCharacteristic = json_object_new_object();
                json_object_object_add( outCharacteristic, "aid", json_object_new_int(aid));
                json_object_object_add( outCharacteristic, "iid", json_object_new_int(iid));
                json_object_object_add( outCharacteristic, "value", json_object_new_double(newValue.floatValue));
                json_object_array_add( outCharacteristics, outCharacteristic );
                HKNotificationAdd( aid, iid, newValue, &notifyList );
              }
              break;
            case ValueType_string:
              if( !strcmp(value.stringValue, newValue.stringValue) ){
                outCharacteristic = json_object_new_object();
                json_object_object_add( outCharacteristic, "aid", json_object_new_int(aid));
                json_object_object_add( outCharacteristic, "iid", json_object_new_int(iid));
                json_object_object_add( outCharacteristic, "value", json_object_new_string(newValue.stringValue));
                json_object_array_add( outCharacteristics, outCharacteristic );
                HKNotificationAdd( aid, iid, newValue, &notifyList );
              }
              break;
            case ValueType_date:
              if( !strcmp(value.dateValue, newValue.dateValue) ){
                outCharacteristic = json_object_new_object();
                json_object_object_add( outCharacteristic, "aid", json_object_new_int(aid));
                json_object_object_add( outCharacteristic, "iid", json_object_new_int(iid));
                json_object_object_add( outCharacteristic, "value", json_object_new_string(newValue.dateValue));
                json_object_array_add( outCharacteristics, outCharacteristic );
                HKNotificationAdd( aid, iid, newValue, &notifyList );
              }
              break;
            default:
              break;
          }
        }
      }
            
      if(json_object_array_length(outCharacteristics)){
        buffer = json_object_to_json_string(outEventJsonObject);
        ha_log("Notification json cstring generated, memory remains %d, %s", mico_memory_info()->free_memory, buffer);  
        err = HKSendNotifyMessage( clientFd, (uint8_t *)buffer, strlen(buffer), hkContext.session );
        require_noerr(err, exit);

      }
      
      json_object_put(outEventJsonObject);
      outEventJsonObject = NULL;
          //err = HKSendResponseMessage(sockfd, status, (uint8_t *)buffer->buf, strlen(buffer->buf), inHkContext);
          //require_noerr(err, exit);
    }
  }

exit:
  SocketClose(&clientFd);
  HTTPHeaderClear( httpHeader );
  if(httpHeader)    free(httpHeader);
  HKNotificationClean( &notifyList );
  if(outEventJsonObject) json_object_put(outEventJsonObject);
  HKCleanPairSetupInfo(&hkContext.pairInfo, Context);
  HKCleanPairVerifyInfo(&hkContext.pairVerifyInfo);
  free(hkContext.session);
  ha_log("Last Free memory1: %d", mico_memory_info()->free_memory);
  mico_rtos_delete_thread(NULL);
  return;
}





static HkStatus HKCreateHAPAttriDataBase( struct _hapAccessory_t inHapObject[], HK_Notify_t* notifyList, json_object **OutHapObjectJson, mico_Context_t * const inContext)
{
  HkStatus err = kNoErr;
  uint32_t accessoryIndex, serviceIndex, characteristicIndex;
  uint32_t aid = 1;
  uint32_t iid = 1;
  struct _hapCharacteristic_t             pCharacteristic;
  bool hasConstraint = false;
  value_union value;
  bool event;

  json_object *hapJsonObject, *accessories, *accessory, *services, *service, *characteristics, *characteristic, *properties;
  json_object *constraints, *metaData;

  hapJsonObject = json_object_new_object();
  accessories = json_object_new_array();
  json_object_object_add( hapJsonObject, "accessories", accessories ); 

  for(accessoryIndex = 0; accessoryIndex < NumberofAccessories; accessoryIndex++){

    accessory = json_object_new_object();
    json_object_array_add (accessories, accessory);

    json_object_object_add( accessory, "aid", json_object_new_int(aid++) );   
    services = json_object_new_array();
    json_object_object_add( accessory, "services", services);

    for(serviceIndex = 0, iid = 1; serviceIndex < MAXServicePerAccessory; serviceIndex++){
      if(inHapObject[accessoryIndex].services[serviceIndex].type == 0)
        break;
      service = json_object_new_object();

      json_object_object_add( service, "type", json_object_new_string(inHapObject[0].services[serviceIndex].type));
      json_object_object_add( service, "iid",  json_object_new_int(iid++));

      characteristics = json_object_new_array();

      json_object_object_add( service, "characteristics",  characteristics);

      for(characteristicIndex = 0; characteristicIndex < MAXCharacteristicPerService; characteristicIndex++){
        pCharacteristic = inHapObject[accessoryIndex].services[serviceIndex].characteristic[characteristicIndex];
        if(pCharacteristic.type){
          characteristic = json_object_new_object();
          json_object_array_add( characteristics, characteristic ); 
          /*Type*/
          json_object_object_add( characteristic, "type", json_object_new_string(pCharacteristic.type));

          /*Instance ID*/
          json_object_object_add( characteristic, "iid", json_object_new_int(iid++));

          HKReadCharacteristicValue(accessoryIndex+1, serviceIndex+1, characteristicIndex+1, &value, inContext);
          /*Value*/
          if(pCharacteristic.hasStaticValue)
            value = pCharacteristic.value;

          switch(pCharacteristic.valueType){
            case ValueType_bool:
              json_object_object_add( characteristic, "value", json_object_new_boolean(value.boolValue));
              break;
            case ValueType_int:
              json_object_object_add( characteristic, "value", json_object_new_int(value.intValue));
              break;
            case ValueType_float:
              json_object_object_add( characteristic, "value", json_object_new_double(value.floatValue));
              break;
            case ValueType_string:
              json_object_object_add( characteristic, "value", json_object_new_string(value.stringValue));
              break;
            case ValueType_date:
              json_object_object_add( characteristic, "value", json_object_new_string(value.dateValue));
              break;
            case ValueType_null:
              json_object_object_add( characteristic, "value", NULL);
              break;
            default:
              break;
          }

          properties = json_object_new_array();
          if(pCharacteristic.secureRead)
            json_object_array_add( properties, json_object_new_string("pr") );
          if(pCharacteristic.secureWrite)
            json_object_array_add( properties, json_object_new_string("pw") );
          json_object_object_add( characteristic, "perms", properties);

          if(pCharacteristic.hasEvents){
            if(HKNotificationFind(aid, iid, notifyList)==kNoErr)
              json_object_object_add( characteristic, "ev", json_object_new_boolean(true));
            else
              json_object_object_add( characteristic, "ev", json_object_new_boolean(false));
          }

          if(pCharacteristic.hasMinimumValue){
            switch(pCharacteristic.valueType){
              case ValueType_int:
                json_object_object_add( characteristic, "minValue",  json_object_new_int(pCharacteristic.minimumValue.intValue) );
                break;
              case ValueType_float:
                json_object_object_add( characteristic, "minValue",  json_object_new_double(pCharacteristic.minimumValue.floatValue) );
                break;
              default:
                break;
            }
          }

          if(pCharacteristic.hasMaximumValue){
            switch(pCharacteristic.valueType){
              case ValueType_int:
                json_object_object_add( characteristic, "maxValue",  json_object_new_int(pCharacteristic.maximumValue.intValue) );
                break;
              case ValueType_float:
                json_object_object_add( characteristic, "maxValue",  json_object_new_double(pCharacteristic.maximumValue.floatValue) );
                break;
              default:
                break;
            }
          }

          if(pCharacteristic.hasMinimumStep){
            switch(pCharacteristic.valueType){
              case ValueType_int:
                json_object_object_add( characteristic, "minStep",  json_object_new_int(pCharacteristic.minimumStep.intValue) );
                break;
              case ValueType_float:
                json_object_object_add( characteristic, "minStep",  json_object_new_double(pCharacteristic.minimumStep.floatValue) );
                break;
              default:
                break;
            }
          }
               
          if(pCharacteristic.hasMaxLength)
            json_object_object_add( characteristic, "maxLen",     json_object_new_int(pCharacteristic.maxLength));

          if(pCharacteristic.hasMaxDataLength)
            json_object_object_add( characteristic, "maxDataLen",     json_object_new_int(pCharacteristic.maxDataLength));

          if(pCharacteristic.description)
            json_object_object_add( characteristic, "description", json_object_new_string(pCharacteristic.description));

          if(pCharacteristic.format)
            json_object_object_add( characteristic, "format", json_object_new_string(pCharacteristic.format));

          if(pCharacteristic.unit)
            json_object_object_add( characteristic, "unit", json_object_new_string(pCharacteristic.unit));
        }
      }
      
      json_object_array_add( services, service ); 
    }    
  }
  
  *OutHapObjectJson = hapJsonObject;

  return err;

}



OSStatus IDGetNext( 
        const uint8_t *     inSrc, 
        const uint8_t *     inEnd, 
        int *               outAID, 
        int *               outIID, 
        const uint8_t **    outNext )
{
  char tmp;
  if(inSrc>=inEnd) return kNotFoundErr;
  uint8_t *endPoint = memchr((const char *)inSrc, ',', inEnd - inSrc);
  if(!endPoint) endPoint = (uint8_t *)inEnd;
  tmp = *endPoint;
  *endPoint = 0x0;
  sscanf((const char *)inSrc, "%d.%d", outAID, outIID);
  *endPoint = tmp;
  *outNext = endPoint + 1;
  return kNoErr;
}



HkStatus _HKCreateReadResponsePerCharacteristic(struct _hapAccessory_t inHapObject[], HK_Char_ID_t id, 
                                                bool needMeta, bool needperms, bool needType, bool needEv, HK_Notify_t* notifyList,
                                                json_object *inHapReadRespondJson, mico_Context_t * const inContext)
{
  HkStatus hkErr = kNoErr;
  value_union value;
  bool event;
  static json_object *characteristic;
  struct _hapCharacteristic_t pCharacteristic = ((inHapObject[id.aid-1]).services[id.serviceID-1]).characteristic[id.characteristicID-1];
  
  characteristic = json_object_new_object();
  json_object_array_add(inHapReadRespondJson, characteristic);
  json_object_object_add( characteristic, "aid", json_object_new_int(id.aid));
  json_object_object_add( characteristic, "iid", json_object_new_int(id.iid));

  if(pCharacteristic.secureRead == false){
    hkErr = kHKReadFromWOErr;
  }else if(id.serviceID == 0)
    hkErr = kHKNotExistErr;
  else{
    if(pCharacteristic.hasStaticValue)
      value = pCharacteristic.value;
    else
      hkErr = HKReadCharacteristicValue(id.aid, id.serviceID, id.characteristicID, &value, inContext);    
  }

  json_object_object_add( characteristic, "status", json_object_new_int(hkErr)); //If no err occure, remove this key before send the respond

  if(hkErr == kNoErr){
    switch(pCharacteristic.valueType ){
      case ValueType_bool:
        json_object_object_add( characteristic, "value", json_object_new_boolean(value.boolValue));
        break;
      case ValueType_int:
        json_object_object_add( characteristic, "value", json_object_new_int(value.intValue));
        break;
      case ValueType_float:
        json_object_object_add( characteristic, "value", json_object_new_double(value.floatValue));
        break;
      case ValueType_string:
        json_object_object_add( characteristic, "value", json_object_new_string(value.stringValue));
        break;
      case ValueType_date:
        json_object_object_add( characteristic, "value", json_object_new_string(value.stringValue));
        break;
      case ValueType_null:
        break;
      default:
        break;
    }  
  }

  if(needMeta){
     if(pCharacteristic.hasMinimumValue){
      switch(pCharacteristic.valueType){
        case ValueType_int:
          json_object_object_add( characteristic, "minValue",  json_object_new_int(pCharacteristic.minimumValue.intValue) );
          break;
        case ValueType_float:
          json_object_object_add( characteristic, "minValue",  json_object_new_double(pCharacteristic.minimumValue.floatValue) );
          break;
        default:
          break;
      }
    }

    if(pCharacteristic.hasMaximumValue){
      switch(pCharacteristic.valueType){
        case ValueType_int:
          json_object_object_add( characteristic, "maxValue",  json_object_new_int(pCharacteristic.maximumValue.intValue) );
          break;
        case ValueType_float:
          json_object_object_add( characteristic, "maxValue",  json_object_new_double(pCharacteristic.maximumValue.floatValue) );
          break;
        default:
          break;
      }
    }

    if(pCharacteristic.hasMinimumStep){
      switch(pCharacteristic.valueType){
        case ValueType_int:
          json_object_object_add( characteristic, "minStep",  json_object_new_int(pCharacteristic.minimumStep.intValue) );
          break;
        case ValueType_float:
          json_object_object_add( characteristic, "minStep",  json_object_new_double(pCharacteristic.minimumStep.floatValue) );
          break;
        default:
          break;
      }
    }
         
    if(pCharacteristic.hasMaxLength)
      json_object_object_add( characteristic, "maxLen",     json_object_new_int(pCharacteristic.maxLength));

    if(pCharacteristic.hasMaxDataLength)
      json_object_object_add( characteristic, "maxDataLen",     json_object_new_int(pCharacteristic.maxDataLength));

    if(pCharacteristic.description)
      json_object_object_add( characteristic, "description", json_object_new_string(pCharacteristic.description));

    if(pCharacteristic.format)
      json_object_object_add( characteristic, "format", json_object_new_string(pCharacteristic.format));

    if(pCharacteristic.unit)
      json_object_object_add( characteristic, "unit", json_object_new_string(pCharacteristic.unit));   
  }

  if(needperms){
    json_object *properties = json_object_new_array();
    if(pCharacteristic.secureRead)
      json_object_array_add( properties, json_object_new_string("pr") );
    if(pCharacteristic.secureWrite)
      json_object_array_add( properties, json_object_new_string("pw") );
    json_object_object_add( characteristic, "perms", properties);
  }

  if(needType){
    json_object_object_add( characteristic, "type", json_object_new_string(pCharacteristic.type));
  }

  if(needEv){
    if(pCharacteristic.hasEvents){
      if(HKNotificationFind(id.aid, id.iid, notifyList)==kNoErr)
        json_object_object_add( characteristic, "ev", json_object_new_boolean(true));
      else
        json_object_object_add( characteristic, "ev", json_object_new_boolean(false));
    }    
  }

  return hkErr;
}

void _HKCreateWritePerCharacteristic(struct _hapAccessory_t inHapObject[], HK_Char_ID_t id, json_object *value_obj, bool moreComing, mico_Context_t * const inContext)
{
  struct _hapCharacteristic_t pCharacteristic;
  value_union value;

  if(id.serviceID == 0 || id.characteristicID == 0)
    return;
  
  pCharacteristic = ((inHapObject[id.aid-1]).services[id.serviceID-1]).characteristic[id.characteristicID-1];
  
  if( pCharacteristic.secureWrite == true && id.serviceID && id.characteristicID ){
    switch(pCharacteristic.valueType ){
      case ValueType_bool:
        value.boolValue = json_object_get_boolean(value_obj);
        break;
      case ValueType_int:
        value.intValue = json_object_get_int(value_obj);
        break;
      case ValueType_float:
        value.floatValue = json_object_get_double(value_obj);
        break;
      case ValueType_string:
        value.stringValue = (char *)json_object_get_string(value_obj);
        break;
      case ValueType_date:
        value.stringValue = (char *)json_object_get_string(value_obj);
        break;
      case ValueType_null:
        break;
      default:
        break;
    }      
    HKWriteCharacteristicValue(id.aid, id.serviceID, id.characteristicID, value, moreComing, inContext);    
  }
}

void _HKCreateWriteEVPerCharacteristic(struct _hapAccessory_t inHapObject[], HK_Char_ID_t id, json_object *value_obj, HK_Notify_t** notifyList, mico_Context_t * const inContext)
{
  struct _hapCharacteristic_t pCharacteristic;
  bool enableNotify = json_object_get_boolean(value_obj);
  value_union value;

  if(id.serviceID == 0 || id.characteristicID == 0)
    return;

  pCharacteristic = ((inHapObject[id.aid-1]).services[id.serviceID-1]).characteristic[id.characteristicID-1];
  
  if( pCharacteristic.hasEvents == true && id.serviceID && id.characteristicID ){
    if(enableNotify){
      FindCharacteristicByIID(hapObjects, id.aid, id.iid, &id.serviceID, &id.characteristicID);
      HKReadCharacteristicValue(id.aid, id.serviceID, id.characteristicID, &value, inContext);
      HKNotificationAdd(id.aid, id.iid, value, notifyList);
    }else{
      HKNotificationRemove(id.aid, id.iid, notifyList);
    }
  }
}


HkStatus _HKCreateWriteResponsePerCharacteristic(struct _hapAccessory_t inHapObject[], HK_Char_ID_t id, json_object *inHapReadRespondJson, bool check_value, bool check_event, mico_Context_t * const inContext)
{
  HkStatus hkErr = kNoErr;
  value_union value;
  bool event;
  int httpStatus = kStatusOK;
  static json_object *characteristic;
  struct _hapCharacteristic_t pCharacteristic = ((inHapObject[id.aid-1]).services[id.serviceID-1]).characteristic[id.characteristicID-1];
  
  characteristic = json_object_new_object();
  json_object_array_add(inHapReadRespondJson, characteristic);
  json_object_object_add( characteristic, "aid", json_object_new_int(id.aid));
  json_object_object_add( characteristic, "iid", json_object_new_int(id.iid));

  if(id.serviceID == 0 || id.characteristicID == 0)
    return kHKNotExistErr;

  if(check_value){
    if(pCharacteristic.secureWrite == false){
      hkErr = kHKWriteToROErr;
    }else{
      hkErr = HKReadCharacteristicStatus(id.aid, id.serviceID, id.characteristicID, inContext);    
    }    
  }
  
  if(check_event){
    if(!pCharacteristic.hasEvents && hkErr == kHKNoErr){
      hkErr = kHKNotifyUnsupportErr;
    }
  }



  json_object_object_add( characteristic, "status", json_object_new_int(hkErr)); 

  return hkErr;
}

OSStatus HKhandleIncomeingMessage(int sockfd, HTTPHeader_t *httpHeader, HK_Notify_t** notifyList, HK_Context_t *inHkContext, mico_Context_t * const inContext)
{
  OSStatus err = kNoErr;
  HkStatus hkErr = kNoErr;
  printbuf *buffer = NULL;
  uint32_t idx;
  size_t arrayLen;
  err = HKSocketReadHTTPHeader( sockfd, httpHeader, inHkContext->session );
  int accessoryID, serviceID, characteristicID;
  json_object *characteristics, *characteristic, *outCharacteristics, *outCharacteristic, *event_obj;
  json_object *value_obj = NULL;
  json_object *inhapJsonObject = NULL, *outhapJsonObject = NULL;
  value_union value;
  bool event, isFound;
  int status = kStatusOK;
  int aid, iid;
  HK_Char_ID_t id;

  switch ( err )
  {
    case kNoErr:
        err = HKSocketReadHTTPBody( sockfd, httpHeader, inHkContext->session );
        require_noerr(err, exit);
        /*Pair set engine*/
        if(HTTPHeaderMatchURL( httpHeader, kPAIRSETUP ) == kNoErr) {
          err = HTTPHeaderMatchMethod( httpHeader, "POST");
          require_noerr_action(err, exit, status = kStatusMethodNotAllowed);

          err = HKPairSetupEngine( sockfd, httpHeader, &inHkContext->pairInfo, inContext );
          require_noerr( err, exit );
          if(inContext->appStatus.haPairSetupRunning == false){err = kConnectionErr; goto exit;};
        }
        /*Pair verify engine*/ 
        else if(HTTPHeaderMatchURL( httpHeader, kPAIRVERIFY ) == kNoErr){
          err = HTTPHeaderMatchMethod( httpHeader, "POST");
          require_noerr_action(err, exit, status = kStatusMethodNotAllowed);

          if(inHkContext->pairVerifyInfo == NULL){
            inHkContext->pairVerifyInfo = HKCreatePairVerifyInfo();
            require_action( inHkContext->pairVerifyInfo, exit, err = kNoMemoryErr );
          }
          err = HKPairVerifyEngine( sockfd, httpHeader, inHkContext->pairVerifyInfo, inContext );
          require_noerr_action( err, exit, HKCleanPairVerifyInfo(&inHkContext->pairVerifyInfo));
          if(inHkContext->pairVerifyInfo->verifySuccess){
            inHkContext->session->established = true;
            memcpy(inHkContext->session->InputKey,  inHkContext->pairVerifyInfo->C2AKey, 32);
            memcpy(inHkContext->session->OutputKey, inHkContext->pairVerifyInfo->A2CKey, 32);
            strncpy(inHkContext->session->controllerIdentifier, inHkContext->pairVerifyInfo->pControllerIdentifier, 64);
            HKCleanPairVerifyInfo(&inHkContext->pairVerifyInfo);
          }
        }
        /* Add or remove pairs */ 
        else if(HTTPHeaderMatchURL( httpHeader, kPAIRINGS ) == kNoErr){
          err = HTTPHeaderMatchMethod( httpHeader, "POST");
          require_noerr_action(err, exit, status = kStatusMethodNotAllowed);

          require_action( inHkContext->session->established == true, exit, err = kAuthenticationErr; status = kStatusAuthenticationErr );

          err = HKPairAddRemoveEngine( sockfd, httpHeader, inHkContext->session );
          require_noerr( err, exit );
        }
        /* Identity routine under unpaired */
        else if(HTTPHeaderMatchURL( httpHeader, kIdentity ) == kNoErr){
          err = HTTPHeaderMatchMethod( httpHeader, "POST");
          require_noerr_action(err, exit, status = kStatusMethodNotAllowed);

          require_action( inHkContext->session->established == false, exit, err = kAuthenticationErr; status = kStatusBadRequest; hkErr = kHKPrivilegeErr );

          HKExcuteUnpairedIdentityRoutine( inContext );

          HKSendResponseMessage(sockfd, kStatusNoConetnt, NULL, 0, inHkContext->session);
          require_noerr(err, exit);
        }
        /*Read accessories database*/
        else if(HTTPHeaderMatchURL( httpHeader, kReadAcc ) == kNoErr){
          err = HTTPHeaderMatchMethod( httpHeader, "GET");
          require_noerr_action(err, exit, status = kStatusMethodNotAllowed);

          require_action( inHkContext->session->established == true, exit, err = kAuthenticationErr; status = kStatusAuthenticationErr );

          err = HKCreateHAPAttriDataBase(hapObjects, *notifyList, &outhapJsonObject, inContext);
          require_noerr( err, exit );
          buffer = json_object_to_json_string_ex(outhapJsonObject);
          ha_log("Json cstring generated, memory remains %d, %s", mico_memory_info()->free_memory, buffer->buf);
          json_object_put(outhapJsonObject);
          outhapJsonObject = NULL;
          err = HKSendResponseMessage(sockfd, status, (uint8_t *)buffer->buf, strlen(buffer->buf), inHkContext->session);
          require_noerr(err, exit);
        }
        /*Read or write characteristics*/
        else if (HTTPHeaderMatchPartialURL( httpHeader, kRWCharacter ) != NULL){

          require_action( inHkContext->session->established == true, exit, err = kAuthenticationErr; status = kStatusAuthenticationErr );

          if(HTTPHeaderMatchMethod( httpHeader, "GET")!=kNotFoundErr){ //Read
            hkErr = kNoErr;
            outhapJsonObject = json_object_new_object();
            outCharacteristics = json_object_new_array();
            json_object_object_add( outhapJsonObject, "characteristics", outCharacteristics);

            void *metaPtr = memmem((void *)httpHeader->url.queryPtr, httpHeader->url.queryLen, "meta=", strlen("meta="));
            void *permsPtr = memmem((void *)httpHeader->url.queryPtr, httpHeader->url.queryLen, "perms=", strlen("perms="));
            void *typePtr = memmem((void *)httpHeader->url.queryPtr, httpHeader->url.queryLen, "type=", strlen("type="));
            void *evPtr = memmem((void *)httpHeader->url.queryPtr, httpHeader->url.queryLen, "ev=", strlen("ev="));
            
            void *idPtr = memmem((void *)httpHeader->url.queryPtr, httpHeader->url.queryLen, "id=", strlen("id="));
            require_action(idPtr, exit, err = kNotFoundErr; status = kStatusBadRequest);
            void *idPtrEnd = (void *)(httpHeader->url.queryPtr + httpHeader->url.queryLen);
            if(memchr(idPtrEnd, '&', (uint8_t *)idPtrEnd - (uint8_t *)idPtr)) idPtrEnd = memchr(idPtrEnd, '&', (uint8_t *)idPtrEnd - (uint8_t *)idPtr);

            bool needMeta = metaPtr? *(uint8_t *)((uint8_t *)metaPtr+strlen("meta="))-0x30 :false;
            bool needPerms = permsPtr? *(uint8_t *)((uint8_t *)permsPtr+strlen("perms="))-0x30 :false;
            bool needEv = evPtr? *(uint8_t *)((uint8_t *)evPtr+strlen("ev="))-0x30 :false;
            bool needType = typePtr? *(uint8_t *)((uint8_t *)typePtr+strlen("type="))-0x30 :false;


            const uint8_t *         src = (const uint8_t *) idPtr + strlen("id=");
            const uint8_t * const   end = idPtrEnd;

            while( IDGetNext( src, end, &id.aid, &id.iid, &src ) == kNoErr ) //Not support ev, perm...
            {
              FindCharacteristicByIID(hapObjects, id.aid, id.iid, &id.serviceID, &id.characteristicID);
              if(id.characteristicID == 0){ //Read every haracteristic in one service
                for(id.characteristicID = 1; id.characteristicID <= MAXCharacteristicPerService; id.characteristicID++){
                  if(hapObjects[id.aid-1].services[id.serviceID-1].characteristic[id.characteristicID-1].type == 0 )
                    break;

                  if(_HKCreateReadResponsePerCharacteristic(hapObjects, id, 
                                                            needMeta, needPerms, needType, needEv, *notifyList,
                                                            outCharacteristics, inContext)!=kHKNoErr)
                    status = kStatusPartialContent;
                  id.characteristicID ++;
                }
              }else{ //Read single haracteristic
                if(_HKCreateReadResponsePerCharacteristic(hapObjects, id, 
                                                          needMeta, needPerms, needType, needEv, *notifyList,
                                                          outCharacteristics, inContext)!=kHKNoErr)
                  status = kStatusPartialContent;
              }
            }
            
            /* Remove status object if no error occured */
            if(status != kStatusPartialContent){
              arrayLen = json_object_array_length(outCharacteristics);
              for(idx = 0; idx < arrayLen; idx++){
                outCharacteristic = json_object_array_get_idx(outCharacteristics, idx);
                json_object_object_del(outCharacteristic, "status");
              }
            }

            /* Send the respond */
            buffer = json_object_to_json_string_ex(outhapJsonObject);
            ha_log("Json cstring generated, memory remains %d, %s", mico_memory_info()->free_memory, buffer->buf);
            json_object_put(outhapJsonObject);
            outhapJsonObject = NULL;
            err = HKSendResponseMessage(sockfd, status, (uint8_t *)buffer->buf, strlen(buffer->buf), inHkContext->session);
            require_noerr(err, exit);
          }
        /* Write characteristic */
        else if(HTTPHeaderMatchMethod( httpHeader, "PUT")!=kNotFoundErr){
          inhapJsonObject = json_tokener_parse(httpHeader->extraDataPtr);
          require_action(inhapJsonObject, exit, err = kMalformedErr);
          characteristics = json_object_object_get(inhapJsonObject, "characteristics");
          require_action(characteristics, exit, err = kMalformedErr);
          require_action(json_object_is_type(characteristics, json_type_array), exit, err = kMalformedErr);

          hkErr = kNoErr;
          status = kStatusNoConetnt;
          outhapJsonObject = json_object_new_object();
          outCharacteristics = json_object_new_array();
          json_object_object_add( outhapJsonObject, "characteristics", outCharacteristics);

          /* Parse every value and write*/
          arrayLen = json_object_array_length(characteristics);
          for(idx = 0; idx < arrayLen; idx++){

            characteristic = json_object_array_get_idx(characteristics, idx);
            id.aid = json_object_get_int(json_object_object_get(characteristic, "aid"));
            id.iid = json_object_get_int(json_object_object_get(characteristic, "iid"));
            FindCharacteristicByIID(hapObjects, id.aid, id.iid, &id.serviceID, &id.characteristicID);      

            value_obj = json_object_object_get(characteristic, "value");
            event_obj = json_object_object_get(characteristic, "ev");

            if(value_obj){
              _HKCreateWritePerCharacteristic(hapObjects, id, value_obj, idx < arrayLen-1, inContext);
          }
            if(event_obj){
              require_action(json_object_is_type(event_obj, json_type_boolean), exit, err = kMalformedErr);
              _HKCreateWriteEVPerCharacteristic(hapObjects, id, event_obj, notifyList, inContext);
            }
          }

          /* Read the write respond */
          for(idx = 0; idx < arrayLen; idx++){
            characteristic = json_object_array_get_idx(characteristics, idx);
            id.aid = json_object_get_int(json_object_object_get(characteristic, "aid"));
            id.iid = json_object_get_int(json_object_object_get(characteristic, "iid"));
            FindCharacteristicByIID(hapObjects, id.aid, id.iid, &id.serviceID, &id.characteristicID);

            value_obj = json_object_object_get(characteristic, "value");
            event_obj = json_object_object_get(characteristic, "ev");

            if(_HKCreateWriteResponsePerCharacteristic(hapObjects, id, outCharacteristics, value_obj, event_obj, inContext)!=kHKNoErr)
              status = kStatusPartialContent;               
          }

          /* Create response json string */
          buffer = json_object_to_json_string_ex(outhapJsonObject);
          ha_log("Json cstring generated, memory remains %d, %s", mico_memory_info()->free_memory, buffer->buf);
          json_object_put(outhapJsonObject);
          outhapJsonObject = NULL;
          json_object_put(inhapJsonObject);
          inhapJsonObject = NULL;

          if(status == kStatusNoConetnt)
            HKSendResponseMessage(sockfd, status, NULL, 0, inHkContext->session);
          else{
            if(arrayLen == 1)
              HKSendResponseMessage(sockfd, kStatusBadRequest, (uint8_t *)buffer->buf, strlen(buffer->buf), inHkContext->session);
            else
              HKSendResponseMessage(sockfd, status, (uint8_t *)buffer->buf, strlen(buffer->buf), inHkContext->session);
          }

        }else
        /*Unkown Methold*/
          err = kGeneralErr;
          status = kStatusBadRequest;
          goto exit;
      }
        /*Unknow URL path*/
      else{
        err = kNotFoundErr;
        status = kStatusNotFound;
        goto exit;
      }

    break;

    case EWOULDBLOCK:
        // NO-OP, keep reading
    break;

    case kNoSpaceErr:
      status = kStatusBadRequest;
      ha_log("ERROR: Cannot fit HTTPHeader.");
      goto exit;
    break;

    case kConnectionErr:
      // NOTE: kConnectionErr from SocketReadHTTPHeader means it's closed
      ha_log("ERROR: Connection closed.");
      goto exit;
    break;
    default:
      status = kStatusBadRequest;
      ha_log("ERROR: HTTP Header parse internal error: %d", err);
      goto exit;
  }
exit:
  if( err != kNoErr && status != kStatusOK ){
    outhapJsonObject = json_object_new_object();
    json_object_object_add( outhapJsonObject, "status", json_object_new_int(hkErr));
    buffer = json_object_to_json_string_ex(outhapJsonObject);
    ha_log("Json cstring generated, memory remains %d, %s", mico_memory_info()->free_memory, buffer->buf);
    HKSendResponseMessage(sockfd, status, (uint8_t *)buffer->buf, strlen(buffer->buf), inHkContext->session);
    msleep(100);
  }

  HTTPHeaderClear( httpHeader );
  if(outhapJsonObject) json_object_put(outhapJsonObject);
  if(inhapJsonObject) json_object_put(inhapJsonObject);
  if(buffer) printbuf_free(buffer);
  return err;

}






