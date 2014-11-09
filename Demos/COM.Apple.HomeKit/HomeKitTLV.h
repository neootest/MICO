#ifndef __HOMEKITTLV_h__
#define __HOMEKITTLV_h__

/*! @group      WACTLVConstants
    @abstract   Constants for element IDs, keys, etc.
*/

// [Integer] Method to use for pairing.
#define kTLVType_Method                 0x00

// [UTF-8] Identifier for authentication.
#define kTLVType_Identifier             0x01

// [bytes] 16+ bytes of random salt.
#define kTLVType_Salt                   0x02

// [bytes] Curve25519, SRP public key or signed Ed25519 key.
#define kTLVType_PublicKey              0x03

// [bytes] ED25519 or SRP proof.
#define kTLVType_Proof                  0x04

// [bytes] Encrypted data with auth tag at end.
#define kTLVType_EncryptedData          0x05

// [bytes] State of the pairing process. 1 = M1, 2 = M2, etc.
#define kTLVType_State                	0x06 

// [Integer] Error code. Must only be present if error code is not 0
#define kTLVType_Error                  0x07

// [Integer] Seconds to delay until retrying a setup code.
#define kTLVType_RetryDelay             0x08

// [bytes] X.509 Certificate
#define kTLVType_Certificate            0x09

// [bytes] Ed25519 or MFi auth IC signature.
#define kTLVType_Signature              0x0A

// [Integer] Bit value describing permissions of the controller being added, 
    //None (0x00): Regular user
    //Bit 1 (0x01): Admin that is able to add and remove pairings against the accessory
#define kTLVType_Permissions            0x0B

// [bytes] Non-last fragment of data. If length is 0, it's an ack
#define kTLVType_FragmentData           0x0C

// [bytes] Last fragment of data
#define kTLVType_FragmentLast           0x0D

// [null] Zero-length TLV that separates different TLVs in a list.
#define kTLVType_Separator              0xFF

#define kHATLV_MaxStringSize           	255
#define kHATLV_TypeLengthSize          	2


// Success, This is not normally included in a message. Absence of a status item implies seccess
#define kTLVError_NoErr   				0x00

// Generic error to handle unexcepted errors
#define kTLVError_UnknowErr   			0x01

// Password or signature verification failed
#define kTLVError_Authentication    	0x02

// Client must look at <RetryDelay> TLV item and wait that many seconds before retrying
#define kTLVError_Backoff               0x03

// Server cannot accept any more pairings
#define kTLVError_MaxPeers   		    0x04

// Server reached its maximum number f authentication attempts
#define kTLVError_MaxTries   		    0x05




#endif // __WACTLV_h__

