#ifndef __HOMEKITPROFILES_h__
#define __HOMEKITPROFILES_h__

#include "Common.h"
#include "HTTPUtils.h"

#include "JSON-C/json.h"

#define MAXCharacteristicPerService       20
#define MAXServicePerAccessory            10
#define NumberofAccessories                1

#define lightbulb
//#define thermostat

// ==== HKtatus ====
typedef int32_t         HkStatus;

#define kHKNoErr                           0   //! This specifies a success for the request.
#define kHKPrivilegeErr               -70401   //! Request denied due to insufficient privileges.
#define kHKCommunicateErr             -70402   //! Unable to communicate with requested service.
#define kHKBusyErr                    -70403   //! Resource is busy, try again.
#define kHKWriteToROErr               -70404   //! Cannot write to read only characteristic.
#define kHKReadFromWOErr              -70405   //! Cannot read from a write only characteristic.
#define kHKNotifyUnsupportErr         -70406   //! Notification is not supported for characteristic.
#define kHKResourceErr                -70407   //! Out of resource to process request.
#define kHKTimeOutErr                 -70408   //! Operation timed out.
#define kHKNotExistErr                -70409   //! Resource does not exist.
#define kHKInvalidErr                 -70410   //! Accessory received an invalid value in a write request.

typedef enum _valueType{
  ValueType_bool,
  ValueType_int,
  ValueType_float,
  ValueType_string,
  ValueType_date,
  ValueType_tlv8,
  ValueType_data,
  ValueType_array,
  ValueType_dict,
  ValueType_null,
} valueType;

typedef union {
    bool        boolValue;
    int         intValue;
    float       floatValue;
    char        *stringValue;
    char        *dateValue;
    json_object *array;
    json_object *object;
  } value_union;


struct _hapCharacteristic_t {
  char   *type;

  bool   hasStaticValue;
  valueType valueType;
  value_union value;

  bool   secureRead;
  bool   secureWrite;
  bool   hasEvents;

  bool   hasMinimumValue;
  union {
    int         intValue;
    float       floatValue;
  }      minimumValue;

  bool   hasMaximumValue;
  union {
    int         intValue;
    float       floatValue;
  }      maximumValue;

  bool   hasMinimumStep;
  union {
    int         intValue;
    float       floatValue;
  }      minimumStep;

  bool   hasMaxLength;
  int    maxLength;

  bool   hasMaxDataLength;
  int    maxDataLength;

  char   *description;

  char   *format;
  
  char   *unit;
};

struct _hapService_t {
  char    *type;
  struct  _hapCharacteristic_t characteristic[MAXCharacteristicPerService];
};

struct _hapAccessory_t {
  struct _hapService_t  services[MAXServicePerAccessory];
};


#endif

