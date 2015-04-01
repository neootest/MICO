#include "MICO.h"
#include "MICODefine.h"
#include "HomeKitPairList.h"
#include "MICOAppDefine.h"
#include "SocketUtils.h"
#include "Platform.h"
#include "platform_config.h"
#include "MicoPlatform.h" 
#include "HTTPUtils.h"
#include "HomeKitTLV.h"
#include "TLVUtils.h"
#include "MICOSRPServer.h"
#include "StringUtils.h"
#include "Curve25519/curve25519-donna.h"
#include "MICOCrypto/crypto_stream_chacha20.h"
#include "MICOCrypto/crypto_aead_chacha20poly1305.h"
#include "MICOCrypto/crypto_sign.h"
#include "SHAUtils/sha.h"
#include "HomeKitPairProtocol.h"
#include "SHAUtils.h"

#define pair_log(M, ...) custom_log("HomeKitPair", M, ##__VA_ARGS__)
#define pair_log_trace() custom_log_trace("HomeKitPair")

typedef enum
{
  eState_M1_VerifyStartRequest      = 1,
  eState_M2_VerifyStartRespond      = 2,
  eState_M3_VerifyFinishRequest     = 3,
  eState_M4_VerifyFinishRespond     = 4,
} HKPairVerifyState_t;

typedef enum
{
  eState_M1_PairingRequest      = 1,
  eState_M2_PairingRespond      = 2,
} HKPairingState_t;

typedef enum
{
  Pair_Setup,
  Pair_MFi_Setup,
  Pair_Verify,
  Pair_Add,
  Pair_Remove,
  Pair_List
} HKPairMethold_t;

const char * hkdfSetupSalt  =        "Pair-Setup-Encrypt-Salt";
const char * hkdfSetupInfo =         "Pair-Setup-Encrypt-Info";

const char * hkdfSetupCSignSalt  =    "Pair-Setup-Controller-Sign-Salt";
const char * hkdfSetupCSignInfo =     "Pair-Setup-Controller-Sign-Info";

const char * hkdfMFiSalt  =    "MFi-Pair-Setup-Salt";
const char * hkdfMFiInfo =     "MFi-Pair-Setup-Info";

const char * hkdfSetupASignSalt  =    "Pair-Setup-Accessory-Sign-Salt";
const char * hkdfSetupASignInfo =     "Pair-Setup-Accessory-Sign-Info";

const char * hkdfVerifySalt =        "Pair-Verify-Encrypt-Salt";
const char * hkdfVerifyInfo =        "Pair-Verify-Encrypt-Info";

const char * hkdfC2AKeySalt =        "Control-Salt";
const char * hkdfC2AInfo =           "Control-Write-Encryption-Key";

const char * hkdfA2CKeySalt =        "Control-Salt";
const char * hkdfA2CInfo =           "Control-Read-Encryption-Key";

const char * AEAD_Nonce_Setup04 =   "PS-Msg04";
const char * AEAD_Nonce_Setup05 =   "PS-Msg05";
const char * AEAD_Nonce_Setup06 =   "PS-Msg06";
const char * AEAD_Nonce_Verify02 =  "PV-Msg02";
const char * AEAD_Nonce_Verify03 =  "PV-Msg03";

const char *stateDescription[7] = {"", "kTLVType_State = M1", "kTLVType_State = M2", "kTLVType_State = M3",
                                   "kTLVType_State = M4", "kTLVType_State = M5", "kTLVType_State = M6"};

static uint8_t  pairErrorNum = 0;

static const uint8_t  *_password = NULL;
static size_t         _len_password = 0;

static const uint8_t  *_verifier = NULL;
static size_t         _len_verifier = 0;

static const uint8_t  *_salt = NULL;
static size_t         _len_salt = 0;

static HAPairSetupState_t haPairSetupState = eState_M1_SRPStartRequest;
const char* hkSRPUser = "Pair-Setup";

OSStatus _HandleState_WaitingForSRPStartRequest( HTTPHeader_t* inHeader, pairInfo_t** inInfo, mico_Context_t * const inContext );
OSStatus _HandleState_HandleSRPStartRespond(int inFd, pairInfo_t* inInfo, mico_Context_t * const inContext);
OSStatus _HandleState_WaitingForSRPVerifyRequest(HTTPHeader_t* inHeader, pairInfo_t* inInfo, mico_Context_t * const inContext );
OSStatus _HandleState_HandleSRPVerifyRespond(int inFd, pairInfo_t* inInfo, mico_Context_t * const inContext);
OSStatus _HandleState_WaitingForExchangeRequest(HTTPHeader_t* inHeader, pairInfo_t* inInfo, mico_Context_t * const inContext );
OSStatus _HandleState_HandleExchangeRespond(int inFd, pairInfo_t** inInfo, mico_Context_t * const inContext);

OSStatus _HandleState_WaitingForVerifyStartRequest( HTTPHeader_t* inHeader, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext );
OSStatus _HandleState_WaitingForVerifyStartRespond(int inFd, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext);
OSStatus _HandleState_WaitingForVerifyFinishRequest(HTTPHeader_t* inHeader, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext );
OSStatus _HandleState_WaitingForVerifyFinishRespond(int inFd, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext);


void HKSetPassword (const uint8_t * password, const size_t passwordLen)
{
  _password = password;
  _len_password = passwordLen;
}

void HKSetVerifier (const uint8_t * verifier, const size_t verifierLen, const uint8_t * salt, const size_t saltLen )
{
  _verifier = verifier;
  _len_verifier = verifierLen;
  _salt = salt;
  _len_salt = saltLen;
}


void HKCleanPairSetupInfo(pairInfo_t **info, mico_Context_t * const inContext){
  if(*info){
    if(inContext->appStatus.haPairSetupRunning == true){
      haPairSetupState = eState_M1_SRPStartRequest;
      inContext->appStatus.haPairSetupRunning = false;
    }
      
    srp_server_delete(&(*info)->SRPServer );
    //if((*info)->SRPUser) free((*info)->SRPUser);
    if((*info)->SRPControllerPublicKey) free((*info)->SRPControllerPublicKey);
    if((*info)->SRPControllerProof) free((*info)->SRPControllerProof);
    if((*info)->HKDF_Key) free((*info)->HKDF_Key);
    free((*info));   
    *info = 0; 
  }
}

pairVerifyInfo_t* HKCreatePairVerifyInfo(void)
{
  pairVerifyInfo_t *pairVerifyInfo = NULL;
  pairVerifyInfo = calloc(1, sizeof( pairVerifyInfo_t ) );
  require( pairVerifyInfo, exit);
  pairVerifyInfo->haPairVerifyState = eState_M1_VerifyStartRequest;
exit:
  return pairVerifyInfo;
}

void HKCleanPairVerifyInfo(pairVerifyInfo_t **verifyInfo){
    if(*verifyInfo){
    if((*verifyInfo)->pControllerCurve25519PK) free((*verifyInfo)->pControllerCurve25519PK);
    if((*verifyInfo)->pAccessoryCurve25519PK) free((*verifyInfo)->pAccessoryCurve25519PK);
    if((*verifyInfo)->pAccessoryCurve25519SK) free((*verifyInfo)->pAccessoryCurve25519SK);
    if((*verifyInfo)->pSharedSecret) free((*verifyInfo)->pSharedSecret);
    if((*verifyInfo)->pHKDFKey) free((*verifyInfo)->pHKDFKey);
    if((*verifyInfo)->A2CKey) free((*verifyInfo)->A2CKey);
    if((*verifyInfo)->C2AKey) free((*verifyInfo)->C2AKey);
    if((*verifyInfo)->pControllerIdentifier) free((*verifyInfo)->pControllerIdentifier);
    
    free((*verifyInfo));   
    *verifyInfo = 0; 
  }
}

OSStatus HKPairSetupEngine( int inFd, HTTPHeader_t* inHeader, pairInfo_t** inInfo, mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;

  uint8_t *outTLVResponse = NULL;
  size_t outTLVResponseLen = 0;
  uint8_t *tlvPtr;

  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;


  if(pairErrorNum>=10){
    outTLVResponse = NULL;

    /* Build tlv: state and encrypted data */
    outTLVResponseLen = 0;
    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_State;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = eState_M2_SRPStartRespond;

    *tlvPtr++ = kTLVType_Error;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = kTLVError_MaxTries;

    haPairSetupState = eState_M1_SRPStartRequest;

    err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_Pairing_TLV8, outTLVResponseLen, &httpResponse, &httpResponseLen );
    require_noerr( err, exit );
    err = SocketSend( inFd, httpResponse, httpResponseLen );
    require_noerr( err, exit );
    err = SocketSend(inFd, outTLVResponse, outTLVResponseLen);
    require_noerr( err, exit );
    goto exit;
  }

  switch ( haPairSetupState ){
    case eState_M1_SRPStartRequest:
      err = _HandleState_WaitingForSRPStartRequest( inHeader, inInfo, inContext );
      require_noerr_action( err, exit, haPairSetupState = eState_M1_SRPStartRequest);
      err =  _HandleState_HandleSRPStartRespond( inFd , *inInfo, inContext );
      require_noerr_action( err, exit, haPairSetupState = eState_M1_SRPStartRequest);
      break;

    case eState_M3_SRPVerifyRequest:
      err = _HandleState_WaitingForSRPVerifyRequest( inHeader, *inInfo, inContext );
      require_noerr_action( err, exit, haPairSetupState = eState_M1_SRPStartRequest);
      err = _HandleState_HandleSRPVerifyRespond(inFd , *inInfo, inContext);
      require_noerr_action( err, exit, haPairSetupState = eState_M1_SRPStartRequest);
      break;

    case eState_M5_ExchangeRequest:
      err = _HandleState_WaitingForExchangeRequest( inHeader, *inInfo, inContext );
      require_noerr_action( err, exit, haPairSetupState = eState_M1_SRPStartRequest);
      err = _HandleState_HandleExchangeRespond(inFd , inInfo, inContext);
      require_noerr_action( err, exit, haPairSetupState = eState_M1_SRPStartRequest);
      break;

    default:
      pair_log("STATE ERROR");
      err = kNoErr;
  }

exit:
  if(err!=kNoErr){
    HKCleanPairSetupInfo(inInfo, inContext);
    pairErrorNum++;
  }
    
  return err;
}


OSStatus _HandleState_WaitingForSRPStartRequest( HTTPHeader_t* inHeader, pairInfo_t** inInfo, mico_Context_t * const inContext )
{
  pair_log_trace();
  const uint8_t *             src = (const uint8_t *) inHeader->extraDataPtr;
  const uint8_t * const       end = src + inHeader->extraDataLen;
  uint8_t                     eid;
  const uint8_t *             ptr;
  size_t                      len;
  char *                      tmp;

  OSStatus err = kNoErr;

  /*Another pair procedure is pending*/
  mico_rtos_lock_mutex(&inContext->flashContentInRam_mutex);
  if(haPairSetupState != eState_M1_SRPStartRequest){
    pairErrorNum++;
    mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
    return kNoErr;
  }

  HKCleanPairSetupInfo(inInfo, inContext); 
  *inInfo = calloc(1, sizeof(pairInfo_t));
  require_action(*inInfo, exit, err = kNoMemoryErr);
  inContext->appStatus.haPairSetupRunning = true;

  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {
    tmp = calloc( len + 1, sizeof( uint8_t ) );
    require_action( tmp, exit, err = kNoMemoryErr );
    memcpy( tmp, ptr, len );

    switch( eid )
    {
      case kTLVType_State:
        require_action(haPairSetupState == *(uint8_t *)tmp, exit, err = kStateErr);
        free(tmp);
      break;
        case kTLVType_Method:
        free(tmp);
        break;
      default:
        free( tmp );
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }
  (*inInfo)->SRPUser = (char *)hkSRPUser;
  haPairSetupState = eState_M2_SRPStartRespond;
  
exit:
  mico_rtos_unlock_mutex(&inContext->flashContentInRam_mutex);
  return err;
}

OSStatus _HandleState_HandleSRPStartRespond(int inFd, pairInfo_t* inInfo, mico_Context_t * const inContext)
{
  pair_log_trace();
  OSStatus err;
  int i, j;
  uint8_t *outTLVResponse = NULL;
  size_t outTLVResponseLen = 0;
  uint8_t *tlvPtr;
  char *tempString = NULL;
  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;

  require_action(_verifier||_password, exit, err = kParamErr);
  inInfo->SRPServer = srp_server_setup( SRP_SHA512, SRP_NG_3072, inInfo->SRPUser, 
                                        _password, _len_password, 
                                        _verifier, _len_verifier,
                                        _salt, _len_salt,
                                        0, 0);
  require(inInfo->SRPServer, exit);

#ifdef DEBUG
  tempString = DataToHexString( inInfo->SRPServer->bytes_v, inInfo->SRPServer->len_v );
  require_action( tempString, exit, err = kNoMemoryErr );
  pair_log("Verifier length: %d: %s", inInfo->SRPServer->len_v, tempString);
  free(tempString);
  tempString = DataToHexString( inInfo->SRPServer->bytes_s, inInfo->SRPServer->len_s );
  require_action( tempString, exit, err = kNoMemoryErr );
  pair_log("Salt length: %d: %s", inInfo->SRPServer->len_s, tempString);
  free(tempString);
#endif

  outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
  for(i = inInfo->SRPServer->len_B/kHATLV_MaxStringSize; i>0; i--)
    outTLVResponseLen += kHATLV_MaxStringSize + kHATLV_TypeLengthSize;
  outTLVResponseLen += inInfo->SRPServer->len_B%kHATLV_MaxStringSize + kHATLV_TypeLengthSize;
  outTLVResponseLen += inInfo->SRPServer->len_s + kHATLV_TypeLengthSize;

  outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
  require_action( outTLVResponse, exit, err = kNoMemoryErr );

  tlvPtr = outTLVResponse;

  /* Send pair state - M2 */
  *tlvPtr++ = kTLVType_State;
  *tlvPtr++ = sizeof(uint8_t);
  *tlvPtr++ = (uint8_t)eState_M2_SRPStartRespond;
  
  /* Send 16+ bytes of random salt */
  *tlvPtr++ = kTLVType_Salt;
  *tlvPtr++ = inInfo->SRPServer->len_s;
  memcpy( tlvPtr, inInfo->SRPServer->bytes_s, inInfo->SRPServer->len_s );
  tlvPtr += inInfo->SRPServer->len_s;

  /* Send accessory's SRP public key */
  j = inInfo->SRPServer->len_B/kHATLV_MaxStringSize;
  for(i = 0; i < j; i++){
    *tlvPtr++ = kTLVType_PublicKey;
    *tlvPtr++ = kHATLV_MaxStringSize;
    memcpy( tlvPtr, inInfo->SRPServer->bytes_B+i*kHATLV_MaxStringSize, kHATLV_MaxStringSize );
    tlvPtr += kHATLV_MaxStringSize;
  }
  *tlvPtr++ = kTLVType_PublicKey;
  *tlvPtr++ = inInfo->SRPServer->len_B%kHATLV_MaxStringSize;
  memcpy( tlvPtr, inInfo->SRPServer->bytes_B+kHATLV_MaxStringSize*j, inInfo->SRPServer->len_B%kHATLV_MaxStringSize );
  tlvPtr += inInfo->SRPServer->len_B%kHATLV_MaxStringSize;
  
  /* Send */
  err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_Pairing_TLV8, outTLVResponseLen, &httpResponse, &httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend( inFd, httpResponse, httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend( inFd, outTLVResponse, outTLVResponseLen );
  require_noerr( err, exit );

  haPairSetupState = eState_M3_SRPVerifyRequest;

exit:
  if(outTLVResponse) free(outTLVResponse);
  if(httpResponse) free(httpResponse);
  return err;
}

OSStatus _HandleState_WaitingForSRPVerifyRequest( HTTPHeader_t* inHeader, pairInfo_t* inInfo, mico_Context_t * const inContext )
{
  pair_log_trace();
  const uint8_t *             src = (const uint8_t *) inHeader->extraDataPtr;
  const uint8_t * const       end = src + inHeader->extraDataLen;
  uint8_t                     eid;
  const uint8_t *             ptr;
  size_t                      len;
  char *                      tmp;
  OSStatus err = kNoErr;

  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {

    switch( eid )
    {
      case kTLVType_State:
        tmp = calloc( len, sizeof( uint8_t ) );
        require_action( tmp, exit, err = kNoMemoryErr );
        memcpy( tmp, ptr, len );
        require_action(haPairSetupState == *(uint8_t *)tmp, exit, err = kStateErr);
        free(tmp);
      break;
        case kTLVType_PublicKey:
        if(inInfo->SRPControllerPublicKey == NULL){
          inInfo->SRPControllerPublicKey = calloc( len, sizeof( uint8_t ) );
          require_action( inInfo->SRPControllerPublicKey, exit, err = kNoMemoryErr );
          memcpy( inInfo->SRPControllerPublicKey, ptr, len );
          inInfo->SRPControllerPublicKeyLen = len;
        }else{
          inInfo->SRPControllerPublicKey = realloc( inInfo->SRPControllerPublicKey, inInfo->SRPControllerPublicKeyLen+len );
          require_action( inInfo->SRPControllerPublicKey, exit, err = kNoMemoryErr );
          memcpy( inInfo->SRPControllerPublicKey+inInfo->SRPControllerPublicKeyLen, ptr, len );
          inInfo->SRPControllerPublicKeyLen += len;
        }
        break;
      case kTLVType_Proof:
        if(inInfo->SRPControllerProof == NULL){
          inInfo->SRPControllerProof = calloc( len, sizeof( uint8_t ) );
          require_action( inInfo->SRPControllerProof, exit, err = kNoMemoryErr );
          memcpy( inInfo->SRPControllerProof, ptr, len );
          inInfo->SRPControllerProofLen = len;
        }else{
          inInfo->SRPControllerProof = realloc( inInfo->SRPControllerProof, inInfo->SRPControllerProofLen+len );
          require_action( inInfo->SRPControllerProof, exit, err = kNoMemoryErr );
          memcpy( inInfo->SRPControllerProof+inInfo->SRPControllerProofLen, ptr, len );
          inInfo->SRPControllerProofLen += len;
        }
        break;
      default:
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }


  
  haPairSetupState = eState_M4_SRPVerifyRespond;


exit:
  return err;
}

OSStatus _HandleState_HandleSRPVerifyRespond(int inFd, pairInfo_t* inInfo, mico_Context_t * const inContext)
{
  pair_log_trace();
  OSStatus err = kNoErr;
  uint8_t *outTLVResponse = NULL;
  size_t outTLVResponseLen = 0;
  uint8_t *tlvPtr;
  uint8_t signMFiChallenge[32];
  uint8_t signMFiChallengeSHA[20];
  SHA_CTX ctx;
  uint8_t *MFiProof = NULL;
  size_t  MFiProofLen;
  uint8_t *outCertificatePtr = NULL;
  size_t outCertificateLen;
  uint8_t *encryptedData = NULL;
  unsigned long long encryptedDataLen;
  int i, j;

  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;

  const uint8_t * bytes_HAMK = 0;

  pair_log( "Checking password..." );
  err = srp_server_generate_session_key( inInfo->SRPServer, inInfo->SRPControllerPublicKey, inInfo->SRPControllerPublicKeyLen );
  require_noerr(err, exit);

  srp_server_verify_session( inInfo->SRPServer,  inInfo->SRPControllerProof,  &bytes_HAMK );

  if ( !bytes_HAMK ){
    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_State;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = eState_M4_SRPVerifyRespond;

    *tlvPtr++ = kTLVType_Error;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = kTLVError_Authentication;
    pair_log("Send: kTLVType_Error: 0x%x", kTLVError_Authentication);
    haPairSetupState = eState_M1_SRPStartRequest;
  }
  else{
    /* Generate session key */
    inInfo->HKDF_Key = malloc(32);
    require_action(inInfo->HKDF_Key, exit, err = kNoMemoryErr);
    err = hkdf(SHA512,  (const unsigned char *)hkdfSetupSalt, strlen(hkdfSetupSalt),
                        inInfo->SRPServer->session_key, inInfo->SRPServer->len_session_key,
                        (const unsigned char *)hkdfSetupInfo, strlen(hkdfSetupInfo), inInfo->HKDF_Key, 32);
    require_noerr(err, exit);


    if(inContext->appStatus.useMFiAuth == true){
      err = MicoMFiAuthInitialize( MICO_I2C_CP );
      require_noerr(err, exit);

      err = hkdf(SHA512,  (const unsigned char *)hkdfMFiSalt, strlen(hkdfMFiSalt),
                          inInfo->SRPServer->session_key, inInfo->SRPServer->len_session_key,
                          (const unsigned char *)hkdfMFiInfo, strlen(hkdfMFiInfo), signMFiChallenge, 32);
      require_noerr(err, exit);  
      SHA1_Init( &ctx );
      SHA1_Update( &ctx, signMFiChallenge, 32 );
      SHA1_Final( (uint8_t *)&signMFiChallengeSHA, &ctx );

      err =  MicoMFiAuthCreateSignature( signMFiChallengeSHA, 20 , &MFiProof,  &MFiProofLen);
      require_noerr(err, exit);

      err = MicoMFiAuthCopyCertificate( &outCertificatePtr, &outCertificateLen );
      require_noerr(err, exit);

      /* Build MFi sub-TLV */
      outTLVResponseLen = 0;
      outTLVResponseLen += MFiProofLen + kHATLV_TypeLengthSize;
      for(i = outCertificateLen/kHATLV_MaxStringSize; i>0; i--)
        outTLVResponseLen += kHATLV_MaxStringSize + kHATLV_TypeLengthSize;
      outTLVResponseLen += outCertificateLen%kHATLV_MaxStringSize + kHATLV_TypeLengthSize;

      outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
      require_action( outTLVResponse, exit, err = kNoMemoryErr );

      tlvPtr = outTLVResponse;
      *tlvPtr++ = kTLVType_Signature;
      *tlvPtr++ = MFiProofLen;
      memcpy(tlvPtr, MFiProof, MFiProofLen);
      tlvPtr+= MFiProofLen;

      j = outCertificateLen/kHATLV_MaxStringSize;
      for(i = 0; i < j; i++){
        *tlvPtr++ = kTLVType_Certificate;
        *tlvPtr++ = kHATLV_MaxStringSize;
        memcpy( tlvPtr, outCertificatePtr+i*kHATLV_MaxStringSize, kHATLV_MaxStringSize );
        tlvPtr += kHATLV_MaxStringSize;
      }

      *tlvPtr++ = kTLVType_Certificate;
      *tlvPtr++ = outCertificateLen%kHATLV_MaxStringSize;
      memcpy( tlvPtr, outCertificatePtr+kHATLV_MaxStringSize*j, outCertificateLen%kHATLV_MaxStringSize );
      tlvPtr += outCertificateLen%kHATLV_MaxStringSize;  
  
      encryptedData = calloc(outTLVResponseLen + 16, sizeof(uint8_t));
      require_action(encryptedData, exit, err = kNoMemoryErr);

      err =  crypto_aead_chacha20poly1305_encrypt(encryptedData, &encryptedDataLen, outTLVResponse, outTLVResponseLen,
                                                  NULL, 0, NULL, (const unsigned char *)AEAD_Nonce_Setup04,
                                                  (const unsigned char *)inInfo->HKDF_Key);

      require_noerr_action(err, exit, pair_log("crypto_aead_chacha20poly1305_encrypt failed"));
      require_action(encryptedDataLen - crypto_aead_chacha20poly1305_ABYTES == outTLVResponseLen, exit, pair_log("encryptedDataLen is not properly set"));
      free(outTLVResponse);
      outTLVResponse = NULL;      

    }

    outTLVResponseLen = 0;
    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
    outTLVResponseLen += inInfo->SRPServer->len_AMK + kHATLV_TypeLengthSize;
    if(inContext->appStatus.useMFiAuth == true){
      for(i = encryptedDataLen/kHATLV_MaxStringSize; i>0; i--)
        outTLVResponseLen += kHATLV_MaxStringSize + kHATLV_TypeLengthSize;
      outTLVResponseLen += encryptedDataLen%kHATLV_MaxStringSize + kHATLV_TypeLengthSize;
    }

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_State;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = eState_M4_SRPVerifyRespond;

    *tlvPtr++ = kTLVType_Proof;
    *tlvPtr++ = inInfo->SRPServer->len_AMK;
    memcpy( tlvPtr, bytes_HAMK, inInfo->SRPServer->len_AMK );
    tlvPtr += inInfo->SRPServer->len_AMK;

    if(inContext->appStatus.useMFiAuth == true){
      j = encryptedDataLen/kHATLV_MaxStringSize;
      for(i = 0; i < j; i++){
        *tlvPtr++ = kTLVType_EncryptedData;
        *tlvPtr++ = kHATLV_MaxStringSize;
        memcpy( tlvPtr, encryptedData+i*kHATLV_MaxStringSize, kHATLV_MaxStringSize );
        tlvPtr += kHATLV_MaxStringSize;
      }

      *tlvPtr++ = kTLVType_EncryptedData;
      *tlvPtr++ = encryptedDataLen%kHATLV_MaxStringSize;
      memcpy( tlvPtr, encryptedData+kHATLV_MaxStringSize*j, encryptedDataLen%kHATLV_MaxStringSize );
      tlvPtr += encryptedDataLen%kHATLV_MaxStringSize;
    }

    haPairSetupState = eState_M5_ExchangeRequest;
  }

  err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_Pairing_TLV8, outTLVResponseLen, &httpResponse, &httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend( inFd, httpResponse, httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend(inFd, outTLVResponse, outTLVResponseLen);
  require_noerr( err, exit );

exit:
  if(outTLVResponse) free(outTLVResponse);
  if(encryptedData) free(encryptedData);
  if(httpResponse) free(httpResponse);
  if(MFiProof) free(MFiProof);
  if(outCertificatePtr) free(outCertificatePtr);

  return err;

}

OSStatus _HandleState_WaitingForExchangeRequest( HTTPHeader_t* inHeader, pairInfo_t* inInfo, mico_Context_t * const inContext )
{
  pair_log_trace();
  UNUSED_PARAMETER(inContext);
  const uint8_t *             src = (const uint8_t *) inHeader->extraDataPtr;
  const uint8_t *             end = src + inHeader->extraDataLen;
  uint8_t                     eid;
  const uint8_t *             ptr;
  size_t                      len;
  char *                      tmp;
  OSStatus                    err = kNoErr;
  unsigned char *             encryptedData = NULL;
  unsigned long long          encryptedDataLen;
  unsigned char *             authTag = NULL;

  unsigned char *             decryptedData = NULL;
  unsigned long long          decryptedDataLen = 0;

  char *                      controllerIdentifier = NULL;
  size_t                      controllerIdentifierLen = 0;
  uint8_t *                   controllerLTPK = NULL;

  uint8_t *                   signature = NULL;

  uint8_t                     signHKDF[32];

  pair_list_in_flash_t        *pairList = NULL;

  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {

    switch( eid )
    {
      case kTLVType_State:
        tmp = calloc( len, sizeof( uint8_t ) );
        require_action( tmp, exit, err = kNoMemoryErr );
        memcpy( tmp, ptr, len );
        require_action(haPairSetupState == *(uint8_t *)tmp, exit, err = kStateErr);
        free(tmp);
      break;
      case kTLVType_EncryptedData:
        encryptedData = calloc( len, sizeof( uint8_t ) );
        require_action( encryptedData, exit, err = kNoMemoryErr );
        memcpy( encryptedData, ptr, len );
        encryptedDataLen = len;
        break;
      default:
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }

  decryptedData = malloc( encryptedDataLen-crypto_aead_chacha20poly1305_ABYTES );
  require_action(decryptedData, exit, err = kNoMemoryErr);
  err =  crypto_aead_chacha20poly1305_decrypt(decryptedData, &decryptedDataLen, NULL, 
                                              (const unsigned char *)encryptedData, encryptedDataLen, NULL, 0,  
                                              (const unsigned char *)AEAD_Nonce_Setup05, (const unsigned char *)inInfo->HKDF_Key);
  require_noerr_action(err, exit, pair_log("crypto_aead_chacha20poly1305_decrypt failed"));

  free(encryptedData);
  encryptedData = NULL;

  /* Parse sub-tlv */ 
  src = (const uint8_t *) decryptedData;
  end = src + decryptedDataLen;
  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {
    tmp = calloc( len + 1, sizeof( uint8_t ) ); // +1 can give an end to a C string
    require_action( tmp, exit, err = kNoMemoryErr );
    memcpy( tmp, ptr, len );

    switch( eid )
    {
      case kTLVType_Identifier:
        controllerIdentifier = tmp;
        controllerIdentifierLen = len;
        break;
      case kTLVType_PublicKey:
        controllerLTPK = (uint8_t *)tmp;
        break;
      case kTLVType_Signature:
        signature = (uint8_t *)tmp;
        break;
      default:
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }  

  free(decryptedData);
  decryptedData = NULL;

  /* Check aead sign */
  err = hkdf(SHA512,  (const unsigned char *)hkdfSetupCSignSalt, strlen(hkdfSetupCSignSalt),
                      inInfo->SRPServer->session_key, inInfo->SRPServer->len_session_key,
                      (const unsigned char *)hkdfSetupCSignInfo, strlen(hkdfSetupCSignInfo), signHKDF, 32);
  require_noerr(err, exit);  

  signature = realloc(signature, 64 + 32 + controllerIdentifierLen + 32);
  memcpy(signature+64,    signHKDF, 32);
  memcpy(signature+64+32, controllerIdentifier, controllerIdentifierLen);
  memcpy(signature+64+32+controllerIdentifierLen, controllerLTPK, 32);

  err = crypto_sign_open(NULL, NULL, signature, 64 + 32 + controllerIdentifierLen + 32, controllerLTPK);
  require_noerr_string(err, exit, "Signature verify failed");

  /* Insert pair info */
  if(HKInsertPairInfo(controllerIdentifier, controllerLTPK, true) == kNoSpaceErr)
    inInfo->pairListFull = true;

  haPairSetupState = eState_M6_ExchangeRespond;

exit:
  if(encryptedData) free(encryptedData);
  if(decryptedData) free(decryptedData);
  if(controllerIdentifier) free(controllerIdentifier);
  if(controllerLTPK) free(controllerLTPK);
  if(signature) free(signature);
  if(authTag) free(authTag);
  if(pairList) free(pairList);
  return err; 
}

OSStatus _HandleState_HandleExchangeRespond(int inFd, pairInfo_t** inInfo, mico_Context_t * const inContext)
{
  pair_log_trace();
  OSStatus err = kNoErr;
  uint8_t *outTLVResponse = NULL;
  size_t outTLVResponseLen = 0;
  uint8_t *tlvPtr;

  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;
  uint8_t  signHKDF[32];
  uint8_t LTPK[32];
  unsigned char *       encryptedData = NULL;
  unsigned long long  encryptedDataLen;
  uint8_t *signature = NULL;
  unsigned long long signatureLen;
  uint8_t *XYZ = NULL;
  size_t XYZLen = 0;
  char *accessoryName = NULL;

  if((*inInfo)->pairListFull == true){
    pair_log("Pair list is full!");

    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_Error;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = kTLVError_MaxPeers;
    pair_log("Send: kTLVType_Status: 0x%x", kTLVError_MaxPeers);
  }else{

    if(inContext->flashContentInRam.appConfig.haPairSetupFinished){
      memcpy(LTPK, inContext->flashContentInRam.appConfig.LTSK + 32, 32);
    }else{
      err = crypto_sign_keypair(LTPK, inContext->flashContentInRam.appConfig.LTSK);
      require_noerr(err, exit);     
    }

    accessoryName = __strdup_trans_dot(inContext->micoStatus.mac);

    err = hkdf(SHA512,  (const unsigned char *)hkdfSetupASignSalt, strlen(hkdfSetupASignSalt),
                      (*inInfo)->SRPServer->session_key, (*inInfo)->SRPServer->len_session_key,
                      (const unsigned char *)hkdfSetupASignInfo, strlen(hkdfSetupASignInfo), signHKDF, 32);
    require_noerr(err, exit);  

    /* XYZ: HKDF/identifier/LTPK */
    XYZLen = 32+strlen(accessoryName)+32;

    signature = calloc(64+XYZLen, sizeof(uint8_t));
    require_action(signature, exit, err=kNoMemoryErr);

    XYZ = calloc(XYZLen, sizeof(uint8_t));
    require_action(XYZ, exit, err=kNoMemoryErr);

    memcpy(XYZ, signHKDF, 32);
    memcpy(XYZ+32, accessoryName, strlen(accessoryName));
    memcpy(XYZ+32+strlen(accessoryName), LTPK, 32);
    
    err = crypto_sign(signature,&signatureLen, XYZ, XYZLen, inContext->flashContentInRam.appConfig.LTSK );
    require_noerr_string(err, exit, "crypto sign failed");
    require_string(signatureLen == 64+XYZLen, exit, "crypto sign failed");

    /* Build sub-tlv: identifier, LTPK and signature */
    outTLVResponseLen += strlen(accessoryName) + kHATLV_TypeLengthSize; //identifier
    outTLVResponseLen += 32 + kHATLV_TypeLengthSize;  //LTPK
    outTLVResponseLen += 64 + kHATLV_TypeLengthSize;  //signature

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_Identifier;
    *tlvPtr++ = strlen(accessoryName);
    memcpy( tlvPtr, accessoryName, strlen(accessoryName) );
    tlvPtr += strlen(accessoryName);
    free(accessoryName);
    accessoryName = NULL;

    *tlvPtr++ = kTLVType_PublicKey;
    *tlvPtr++ = 32;
    memcpy( tlvPtr, LTPK, 32 );
    tlvPtr += 32;

    *tlvPtr++ = kTLVType_Signature;
    *tlvPtr++ = 64;
    memcpy( tlvPtr, signature, 64 );

    encryptedData = calloc(outTLVResponseLen + 16, sizeof(uint8_t));

    require_action((*inInfo)->HKDF_Key, exit, err = kParamErr);
    err =  crypto_aead_chacha20poly1305_encrypt(encryptedData, &encryptedDataLen, outTLVResponse, outTLVResponseLen,
                                                NULL, 0, NULL, (const unsigned char *)AEAD_Nonce_Setup06,
                                                (const unsigned char *)(*inInfo)->HKDF_Key);

    require_noerr_action(err, exit, pair_log("crypto_aead_chacha20poly1305_encrypt failed"));
    require_action(encryptedDataLen - crypto_aead_chacha20poly1305_ABYTES == outTLVResponseLen, exit, pair_log("encryptedDataLen is not properly set"));
    free(outTLVResponse);
    outTLVResponse = NULL;

    /* Build tlv: state and encrypted data */
    outTLVResponseLen = 0;
    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
    outTLVResponseLen += encryptedDataLen + kHATLV_TypeLengthSize;

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_State;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = eState_M6_ExchangeRespond;

    *tlvPtr++ = kTLVType_EncryptedData;
    *tlvPtr++ = encryptedDataLen;
    memcpy( tlvPtr, encryptedData, encryptedDataLen);
  }

  haPairSetupState = eState_M1_SRPStartRequest;

  err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_Pairing_TLV8, outTLVResponseLen, &httpResponse, &httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend( inFd, httpResponse, httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend(inFd, outTLVResponse, outTLVResponseLen);
  require_noerr( err, exit );

  /*Save accessory's LPSK*/
  if((*inInfo)->pairListFull != true){
    inContext->flashContentInRam.appConfig.haPairSetupFinished = true;
    err = MICOUpdateConfiguration(inContext);
    require_noerr(err, exit);
  }
  haPairSetupState = eState_M1_SRPStartRequest;
  HKCleanPairSetupInfo(inInfo, inContext);
  inContext->appStatus.haPairSetupRunning = false;

exit:
  if(outTLVResponse) free(outTLVResponse);
  if(httpResponse) free(httpResponse);
  if(signature) free(signature);
  if(accessoryName) free(accessoryName);
  if(XYZ) free(XYZ);
  if(encryptedData) free(encryptedData);
  return err;

}


OSStatus HKPairVerifyEngine( int inFd, HTTPHeader_t* inHeader, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext )
{
  OSStatus err = kNoErr;
  require_action(inInfo, exit, err = kNotPreparedErr);

  switch ( inInfo->haPairVerifyState ){
    case eState_M1_VerifyStartRequest:
      err = _HandleState_WaitingForVerifyStartRequest( inHeader, inInfo, inContext );
      require_noerr_action( err, exit, inInfo->haPairVerifyState = eState_M1_VerifyStartRequest);
      err =  _HandleState_WaitingForVerifyStartRespond( inFd , inInfo, inContext );
      require_noerr_action( err, exit, inInfo->haPairVerifyState = eState_M1_VerifyStartRequest);
      break;

    case eState_M3_VerifyFinishRequest:
      err = _HandleState_WaitingForVerifyFinishRequest( inHeader, inInfo, inContext );
      require_noerr_action( err, exit, inInfo->haPairVerifyState = eState_M1_VerifyStartRequest);
      err = _HandleState_WaitingForVerifyFinishRespond(inFd , inInfo, inContext);
      require_noerr_action( err, exit, inInfo->haPairVerifyState = eState_M1_VerifyStartRequest);
      break;

    default:
      pair_log("STATE ERROR");
      err = kStateErr;
  }

exit:
  return err;
}


OSStatus _HandleState_WaitingForVerifyStartRequest( HTTPHeader_t* inHeader, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext )
{
  pair_log_trace();
  OSStatus                    err = kNoErr;
  const uint8_t *             src = (const uint8_t *) inHeader->extraDataPtr;
  const uint8_t * const       end = src + inHeader->extraDataLen;
  uint8_t                     eid;
  const uint8_t *             ptr;
  size_t                      len;
  char *                      tmp = NULL;

  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {
    tmp = calloc( len+1, sizeof( uint8_t ) );
    require_action( tmp, exit, err = kNoMemoryErr );
    memcpy( tmp, ptr, len );

    switch( eid )
    {
      case kTLVType_State:
        require_action(inInfo->haPairVerifyState == *(uint8_t *)tmp, exit, err = kStateErr);
        free(tmp);
        break;
      case kTLVType_PublicKey:
        inInfo->pControllerCurve25519PK = (uint8_t *)tmp;
        break;
      default:
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }
  inInfo->haPairVerifyState = eState_M2_VerifyStartRespond;

exit:
  return err;
}

OSStatus _HandleState_WaitingForVerifyStartRespond(int inFd, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext)
{
  pair_log_trace();
  OSStatus            err = kNoErr;
  uint8_t             *outTLVResponse = NULL;
  size_t              outTLVResponseLen = 0;
  uint8_t             *tlvPtr;
  uint8_t             *httpResponse = NULL;
  size_t              httpResponseLen = 0;
  uint8_t             *ABC = NULL;
  size_t              ABCLen = 0;
  uint8_t             *signature = NULL;
  unsigned long long  signatureLen = 0;
  unsigned char       *encryptedData = NULL;
  unsigned long long  encryptedDataLen = 0;
  char                *accessoryName = NULL;

  inInfo->pAccessoryCurve25519PK = malloc(32);
  require_action(inInfo->pAccessoryCurve25519PK, exit, err = kNoMemoryErr);
  inInfo->pAccessoryCurve25519SK = malloc(32);
  require_action(inInfo->pAccessoryCurve25519SK, exit, err = kNoMemoryErr);
  inInfo->pSharedSecret = malloc(32);
  require_action(inInfo->pSharedSecret, exit, err = kNoMemoryErr);
  inInfo->pHKDFKey = malloc(32);
  require_action(inInfo->pHKDFKey, exit, err = kNoMemoryErr);


  /* Generate new, random Curve25519 key pair */
  err = PlatformRandomBytes( inInfo->pAccessoryCurve25519SK, 32 );
  require_noerr( err, exit );
  curve25519_donna( inInfo->pAccessoryCurve25519PK, inInfo->pAccessoryCurve25519SK, NULL );

  /* Generate shared secret */
  curve25519_donna( inInfo->pSharedSecret, inInfo->pAccessoryCurve25519SK, inInfo->pControllerCurve25519PK );

  /* Generate signature of accessory's info  ABC: Accessory curve25519 pk/accessory identifier/Controller curve25519 pk */
  accessoryName = __strdup_trans_dot(inContext->micoStatus.mac);

  ABCLen = 32+strlen(accessoryName)+32;

  ABC = calloc(ABCLen, sizeof(uint8_t));
  require_action(ABC, exit, err = kNoMemoryErr);

  signature = calloc(64+ABCLen, sizeof(uint8_t));
  require_action(signature, exit, err = kNoMemoryErr);

  memcpy(ABC,                           inInfo->pAccessoryCurve25519PK,   32);
  memcpy(ABC+32,                        accessoryName,                    strlen(accessoryName));
  memcpy(ABC+32+strlen(accessoryName),  inInfo->pControllerCurve25519PK,  32);

  err = crypto_sign(signature,&signatureLen, ABC, ABCLen, inContext->flashContentInRam.appConfig.LTSK );
  require_noerr_string(err, exit, "crypto sign failed");
  free(ABC);
  ABC = NULL;

  /* Build sub-TLV */
  outTLVResponseLen = 0;
  outTLVResponseLen += strlen(accessoryName) + kHATLV_TypeLengthSize;
  outTLVResponseLen += 64 + kHATLV_TypeLengthSize;

  outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
  require_action( outTLVResponse, exit, err = kNoMemoryErr );
  tlvPtr = outTLVResponse;

  *tlvPtr++ = kTLVType_Identifier;
  *tlvPtr++ = strlen(accessoryName);
  memcpy(tlvPtr, accessoryName, strlen(accessoryName));
  tlvPtr += strlen(accessoryName);
  free(accessoryName);
  accessoryName = NULL;

  *tlvPtr++ = kTLVType_Signature;
  *tlvPtr++ = 64;
  memcpy( tlvPtr, signature, 64 );
  free(signature);
  signature = NULL;

  /* Derive encryption key from Curve25519 shared secret */
  err = hkdf(SHA512,  (const unsigned char *) hkdfVerifySalt, strlen(hkdfVerifySalt),
                      inInfo->pSharedSecret, 32,
                      (const unsigned char *)hkdfVerifyInfo, strlen(hkdfVerifyInfo), inInfo->pHKDFKey, 32);
  require_noerr_string(err, exit, "Generate HKDK key failed");

  /* Encrypt sub-TLV and generate an auth tag*/
  encryptedData = calloc(outTLVResponseLen+16, sizeof(uint8_t));
  require_action(encryptedData, exit, err = kNoMemoryErr);

  err =  crypto_aead_chacha20poly1305_encrypt(encryptedData, &encryptedDataLen, outTLVResponse, outTLVResponseLen,
                                              NULL, 0, NULL, (const unsigned char *)AEAD_Nonce_Verify02,
                                              (const unsigned char *)inInfo->pHKDFKey);

  require_noerr_action(err, exit, pair_log("crypto_aead_chacha20poly1305_encrypt failed"));
  require_action(encryptedDataLen - crypto_aead_chacha20poly1305_ABYTES == outTLVResponseLen, exit, pair_log("encryptedDataLen is not properly set"));
  free(outTLVResponse);
  outTLVResponse = NULL;

  /* Respond with TLV item */
  outTLVResponseLen = 0;
  outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
  outTLVResponseLen += 32 + kHATLV_TypeLengthSize;
  outTLVResponseLen += encryptedDataLen + kHATLV_TypeLengthSize;

  outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
  require_action( outTLVResponse, exit, err = kNoMemoryErr );

  tlvPtr = outTLVResponse;
  *tlvPtr++ = kTLVType_State;
  *tlvPtr++ = sizeof(uint8_t);
  *tlvPtr++ = eState_M2_VerifyStartRespond;

  *tlvPtr++ = kTLVType_PublicKey;
  *tlvPtr++ = 32;
  memcpy( tlvPtr, inInfo->pAccessoryCurve25519PK, 32);
  tlvPtr += 32;

  *tlvPtr++ = kTLVType_EncryptedData;
  *tlvPtr++ = encryptedDataLen;
  memcpy( tlvPtr, encryptedData, encryptedDataLen );

  err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_Pairing_TLV8, outTLVResponseLen, &httpResponse, &httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend( inFd, httpResponse, httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend(inFd, outTLVResponse, outTLVResponseLen);
  require_noerr( err, exit );
  inInfo->haPairVerifyState = eState_M3_VerifyFinishRequest;

exit:
  if(accessoryName) free(accessoryName);
  if(ABC) free(ABC);
  if(signature) free(signature);
  if(encryptedData) free(encryptedData);
  if(outTLVResponse) free(outTLVResponse);
  if(httpResponse) free(httpResponse);
  return err;
}

OSStatus _HandleState_WaitingForVerifyFinishRequest(HTTPHeader_t* inHeader, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext )
{
  pair_log_trace();
  OSStatus                    err = kNoErr;
  (void)                      inContext;
  const uint8_t *             src = (const uint8_t *) inHeader->extraDataPtr;
  const uint8_t *             end = src + inHeader->extraDataLen;
  uint8_t                     eid;
  const uint8_t *             ptr;
  size_t                      len;
  char *                      tmp = NULL;
  uint8_t *                   encryptedData = NULL;
  size_t                      encryptedDataLen = 0;
  uint8_t *                   decryptedData = NULL;
  unsigned long long          decryptedDataLen = 0;
  uint8_t *                   signature = NULL;
  char *                      controllerIdentifier = NULL;
  size_t                      controllerIdentifierLen = 0;

  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {
    tmp = calloc( len, sizeof( uint8_t ) );
    require_action( tmp, exit, err = kNoMemoryErr );
    memcpy( tmp, ptr, len );

    switch( eid )
    {
      case kTLVType_State:
        require_action(inInfo->haPairVerifyState == *(uint8_t *)tmp, exit, err = kStateErr);
        free(tmp);
        break;
      case kTLVType_EncryptedData:
        encryptedData = (uint8_t *)tmp;
        encryptedDataLen = len;
        break;
      default:
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }

  decryptedData = malloc( encryptedDataLen-crypto_aead_chacha20poly1305_ABYTES );
  require_action(decryptedData, exit, err = kNoMemoryErr);
  err =  crypto_aead_chacha20poly1305_decrypt(decryptedData, &decryptedDataLen, NULL, 
                                              (const unsigned char *)encryptedData, encryptedDataLen, NULL, 0,  
                                              (const unsigned char *)AEAD_Nonce_Verify03, (const unsigned char *)inInfo->pHKDFKey);
  require_noerr_action(err, exit, pair_log("crypto_aead_chacha20poly1305_decrypt failed"));

  src = (const uint8_t *) decryptedData;
  end = src + decryptedDataLen;
  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {
    tmp = calloc( len+1, sizeof( uint8_t ) ); //+1 to give an end to C string
    require_action( tmp, exit, err = kNoMemoryErr );
    memcpy( tmp, ptr, len );

    switch( eid )
    {
      case kTLVType_Identifier:
        controllerIdentifier = tmp;
        inInfo->pControllerLTPK = HMFindLTPK(controllerIdentifier);
        inInfo->pControllerIdentifier = malloc(len+1);
        memcpy(inInfo->pControllerIdentifier, tmp, len+1);
        controllerIdentifierLen = len;
        break;
      case kTLVType_Signature:
        signature = (uint8_t *)tmp;
        break;
      default:
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }

  signature = realloc(signature, 64 + 32 + controllerIdentifierLen + 32);
  require_action(signature, exit, err=kNoMemoryErr);
  memcpy(signature+64,                            inInfo->pControllerCurve25519PK,  32);
  memcpy(signature+64+32,                         controllerIdentifier,             controllerIdentifierLen);
  memcpy(signature+64+32+controllerIdentifierLen, inInfo->pAccessoryCurve25519PK,   32);

  err = crypto_sign_open(NULL, NULL, signature, 64 + 32 + controllerIdentifierLen + 32, inInfo->pControllerLTPK);
  require_noerr_string(err, exit, "Signature verify failed");
  pair_log("Signature verify success");

exit:
  if(encryptedData) free(encryptedData);
  if(decryptedData) free(decryptedData);
  if(controllerIdentifier) free(controllerIdentifier);
  if(signature) free(signature);
  return err;
}

OSStatus _HandleState_WaitingForVerifyFinishRespond(int inFd, pairVerifyInfo_t* inInfo, mico_Context_t * const inContext)
{
  pair_log_trace();
  OSStatus err = kNoErr;
  (void)inContext;
  uint8_t *outTLVResponse = NULL;
  size_t outTLVResponseLen = 0;
  uint8_t *tlvPtr;

  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;

  outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

  outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
  require_action( outTLVResponse, exit, err = kNoMemoryErr );

  tlvPtr = outTLVResponse;
  *tlvPtr++ = kTLVType_State;
  *tlvPtr++ = sizeof(uint8_t);
  *tlvPtr++ = eState_M4_SRPVerifyRespond;

  inInfo->verifySuccess = true;
  inInfo->A2CKey = malloc(32);
  require_action(inInfo->A2CKey, exit, err = kNoMemoryErr);
  err = hkdf(SHA512,  (const unsigned char *) hkdfA2CKeySalt, strlen(hkdfA2CKeySalt),
                            inInfo->pSharedSecret, 32,
                            (const unsigned char *)hkdfA2CInfo, strlen(hkdfA2CInfo), inInfo->A2CKey, 32);
  require_noerr(err, exit);

  inInfo->C2AKey = malloc(32);
  require_action(inInfo->C2AKey, exit, err = kNoMemoryErr);
  err = hkdf(SHA512,  (const unsigned char *) hkdfC2AKeySalt, strlen(hkdfC2AKeySalt),
                            inInfo->pSharedSecret, 32,
                            (const unsigned char *)hkdfC2AInfo, strlen(hkdfC2AInfo), inInfo->C2AKey, 32);
  require_noerr(err, exit);

  err =  CreateSimpleHTTPMessageNoCopy( kMIMEType_Pairing_TLV8, outTLVResponseLen, &httpResponse, &httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend( inFd, httpResponse, httpResponseLen );
  require_noerr( err, exit );
  err = SocketSend( inFd, outTLVResponse, outTLVResponseLen );
  require_noerr( err, exit );

exit:
  if(outTLVResponse) free(outTLVResponse);
  if(httpResponse) free(httpResponse);
  return err;
}

OSStatus HKSendPairResponseMessage(int sockfd, int status, uint8_t *payload, int payloadLen, security_session_t *session )
{
  OSStatus err;
  uint8_t *httpResponse = NULL;
  size_t httpResponseLen = 0;
  const char *buffer = NULL;
  int bufferLen;

  buffer = (const char *)payload;
  bufferLen = payloadLen;

  err = CreateHTTPRespondMessageNoCopy( status, kMIMEType_Pairing_TLV8, bufferLen, &httpResponse, &httpResponseLen );

  require_noerr( err, exit );
  require( httpResponse, exit );

  err = HKSecureSocketSend( sockfd, httpResponse, httpResponseLen, session );
  require_noerr( err, exit );
  if(bufferLen){
    err = HKSecureSocketSend( sockfd, (uint8_t *)buffer, bufferLen, session );
    require_noerr( err, exit ); 
  }

exit:
  if(httpResponse) free(httpResponse);
  return err;
}

OSStatus HKPairAddRemoveEngine( int inFd, HTTPHeader_t* inHeader, security_session_t *session  )
{
  pair_log_trace();
  OSStatus err = kNoErr;
  uint8_t methold;
  char *                      controllerIdentifier = NULL;
  uint8_t *                   controllerLTPK = NULL;
  uint32_t        permissions = 0x0;
  uint32_t i;
  uint8_t *httpResponse = NULL;
    uint8_t *outTLVResponse = NULL;
  size_t outTLVResponseLen = 0;
  uint8_t *tlvPtr;
  pair_list_in_flash_t        *pairList = NULL;
  bool needSeparator = false;

  const uint8_t *             src = (const uint8_t *) inHeader->extraDataPtr;
  const uint8_t * const       end = src + inHeader->extraDataLen;
  uint8_t                     eid;
  const uint8_t *             ptr;
  size_t                      len;
  char *                      tmp;

  while( TLVGetNext( src, end, &eid, &ptr, &len, &src ) == kNoErr )
  {
    tmp = calloc( len + 1, sizeof( uint8_t ) );
    require_action( tmp, exit, err = kNoMemoryErr );
    memcpy( tmp, ptr, len );

    switch( eid )
    {
      case kTLVType_State:
        require_action(1 == *(uint8_t *)tmp, exit, err = kStateErr);
        free(tmp);
        break;
      case kTLVType_Method:
        methold = *(uint8_t *)tmp;
        free(tmp);
        break;
      case kTLVType_Identifier:
        controllerIdentifier = tmp;
        break;
      case kTLVType_PublicKey:
        controllerLTPK = (uint8_t *)tmp;
        break;
      case kTLVType_Permissions:
        permissions = *(uint8_t *)tmp;
        free(tmp);
        break;
      default:
        free( tmp );
        pair_log( "Warning: Ignoring unsupported pair setup EID 0x%02X", eid );
        break;
    }
  }

  if( HMFindAdmin(session->controllerIdentifier) == false){ //Require admin
        outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
        outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

        outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
        require_action( outTLVResponse, exit, err = kNoMemoryErr );

        tlvPtr = outTLVResponse;
        *tlvPtr++ = kTLVType_State;
        *tlvPtr++ = sizeof(uint8_t);
        *tlvPtr++ = eState_M2_PairingRespond;

        *tlvPtr++ = kTLVType_Error;
        *tlvPtr++ = sizeof(uint8_t);
        *tlvPtr++ = kTLVError_UnknowErr;

        err = HKSendPairResponseMessage(inFd, kStatusOK, outTLVResponse, outTLVResponseLen, session );
        goto exit;        
      }

  if(methold == Pair_Add){
    require_action(controllerIdentifier && controllerLTPK, exit, err = kParamErr);

    if(HKInsertPairInfo(controllerIdentifier, controllerLTPK, (permissions&0x1)) == kNoSpaceErr){
      outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
      outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

      outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
      require_action( outTLVResponse, exit, err = kNoMemoryErr );

      tlvPtr = outTLVResponse;
      *tlvPtr++ = kTLVType_State;
      *tlvPtr++ = sizeof(uint8_t);
      *tlvPtr++ = eState_M2_PairingRespond;

      *tlvPtr++ = kTLVType_Error;
      *tlvPtr++ = sizeof(uint8_t);
      *tlvPtr++ = kTLVError_MaxPeers;

      err = HKSendPairResponseMessage(inFd, kStatusOK, outTLVResponse, outTLVResponseLen, session );
      goto exit;
    }

    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_State;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = eState_M2_PairingRespond;

    err = HKSendPairResponseMessage(inFd, kStatusOK, outTLVResponse, outTLVResponseLen, session );
    require_noerr( err, exit );

  }else if(methold == Pair_Remove){
    require_action(controllerIdentifier, exit, err = kParamErr);

    if( HMRemoveLTPK(controllerIdentifier) != kNoErr ){ //Remove
      outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;
      outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

      outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
      require_action( outTLVResponse, exit, err = kNoMemoryErr );

      tlvPtr = outTLVResponse;
      *tlvPtr++ = kTLVType_State;
      *tlvPtr++ = sizeof(uint8_t);
      *tlvPtr++ = eState_M2_PairingRespond;

      *tlvPtr++ = kTLVType_Error;
      *tlvPtr++ = sizeof(uint8_t);
      *tlvPtr++ = kTLVError_UnknowErr;

      err = HKSendPairResponseMessage(inFd, kStatusOK, outTLVResponse, outTLVResponseLen, session );
      goto exit;    
    }

    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_State;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = eState_M2_PairingRespond;

    err = HKSendPairResponseMessage(inFd, kStatusOK, outTLVResponse, outTLVResponseLen, session );
    
    require_noerr( err, exit );

  }else if(methold == Pair_List){ //List

    pairList = calloc(1, sizeof(pair_list_in_flash_t));
    require_action(pairList, exit, err = kNoMemoryErr);

    err = HMReadPairList(pairList);
    require_noerr(err, exit);

    outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize; //M2

    needSeparator = false;
    for(i=0; i<MAXPairNumber; i++){
      if(pairList->pairInfo[i].controllerName[0] != 0){
        if(needSeparator)
          outTLVResponseLen += kHATLV_TypeLengthSize; //Separates
        else
          needSeparator = true;
        outTLVResponseLen += strlen(pairList->pairInfo[i].controllerName) + kHATLV_TypeLengthSize; //Identifier
        outTLVResponseLen += 32 + kHATLV_TypeLengthSize;  //Public key
        outTLVResponseLen += sizeof(uint8_t) + kHATLV_TypeLengthSize;  //Permission
      }
    }

    outTLVResponse = calloc( outTLVResponseLen, sizeof( uint8_t ) );
    require_action( outTLVResponse, exit, err = kNoMemoryErr );

    tlvPtr = outTLVResponse;
    *tlvPtr++ = kTLVType_State;
    *tlvPtr++ = sizeof(uint8_t);
    *tlvPtr++ = eState_M2_PairingRespond;

    needSeparator = false;
    for(i=0; i<MAXPairNumber; i++){
      if(pairList->pairInfo[i].controllerName[0] != 0){
        if(needSeparator){
          *tlvPtr++ = kTLVType_Separator;
          *tlvPtr++ = 0x0;
        }
        else
          needSeparator = true;

        *tlvPtr++ = kTLVType_Identifier;
        *tlvPtr++ = strlen(pairList->pairInfo[i].controllerName);
        strncpy((char *)tlvPtr, pairList->pairInfo[i].controllerName, 64);
        tlvPtr+= strlen(pairList->pairInfo[i].controllerName);

        *tlvPtr++ = kTLVType_PublicKey;
        *tlvPtr++ = 32;
        memcpy(tlvPtr, pairList->pairInfo[i].controllerLTPK, 32);
        tlvPtr+= 32;

        *tlvPtr++ = kTLVType_Permissions;
        *tlvPtr++ = sizeof(uint8_t);
        *tlvPtr+= pairList->pairInfo[i].permission;
      }
    }

    err = HKSendPairResponseMessage(inFd, kStatusOK, outTLVResponse, outTLVResponseLen, session );
    
    require_noerr( err, exit );

  }else{
    err = kGeneralErr;
    HKSendResponseMessage(inFd, kStatusBadRequest, NULL, 0, session);
    msleep(100);
    goto exit;
  }

exit: 
  if(pairList) free(pairList);
  if(outTLVResponse) free(outTLVResponse);
  if(controllerIdentifier)  free(controllerIdentifier);
  if(controllerLTPK)  free(controllerLTPK);
  if(httpResponse) free(httpResponse);
  pair_log("Memory check 3: %d", mico_memory_info()->free_memory);
  return err;
}

