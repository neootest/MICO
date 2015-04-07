
#include "HomeKitProfiles.h"
#include "Common.h"
#include "MICODefine.h"
#include "platform_config.h"

const struct _hapAccessory_t hapObjects[NumberofAccessories] = 
{
  {
    .services = {
      [0] = { //public.hap.service.accessory-information
        .type = "3E",
        .characteristic = { //public.hap.characteristic.name
          [0] = {
            .type = "23",
            .valueType = ValueType_string,
            .secureRead = true,
            .hasEvents = false,
          },
          [1] = { //public.hap.characteristic.manufacturer
            .type = "20",
            .valueType = ValueType_string,
            .hasStaticValue = true,
            .value.stringValue = MANUFACTURER,
            .secureRead = true,
            .hasEvents = false,
          },
          [2] = { //public.hap.characteristic.serial-number
            .type = "30",
            .valueType = ValueType_string,
            .hasStaticValue = true,
            .value.stringValue = SERIAL_NUMBER,
            .secureRead = true,
            .hasEvents = false,
          },	
          [3] = { //public.hap.characteristic.model
            .type = "21",
            .valueType = ValueType_string,
            .hasStaticValue = true,
            .value.stringValue = MODEL,
            .secureRead = true,
            .hasEvents = false,
          },
          [4] = { //public.hap.characteristic.identify 
            .type = "14",
            .valueType = ValueType_null,
            .hasStaticValue = true,
            .value = NULL,
            .secureWrite = true,
            .hasEvents = false,
          }	
        }
      },
#ifdef lightbulb
    [1] = { //public.hap.service.lightbulb
        .type = "43",
        .characteristic = {
          [0] = { //public.hap.characteristic.on
            .type = "25",
            .valueType = ValueType_bool,
            .secureRead = true,
            .secureWrite = true,
            .hasEvents = true,
          },
          [1] = { //public.hap.characteristic.brightness
            .type = "8",
            .valueType = ValueType_int,
            .secureRead = true,
            .secureWrite = true,
            .hasEvents = true,
            .hasMinimumValue = true,
            .minimumValue = 0,
            .hasMaximumValue = true,
            .maximumValue = 100,
            .hasMinimumStep = true,
            .minimumStep = 1,
            .unit = "percentage",
          },
          [2] = { //public.hap.characteristic.hue
            .type = "13",
            .valueType = ValueType_float,
            .secureRead = true,
            .secureWrite = true,
            .hasEvents = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 0,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 360,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 1,
            .unit = "arcdegrees",
          }, 
          [3] = { //public.hap.characteristic.saturation
            .type = "2F",
            .valueType = ValueType_float,
            .secureRead = true,
            .secureWrite = true,
            .hasEvents = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 0,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 100,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 1,
            .unit = "percentage",
          }, 
          [4] = { //public.hap.characteristic.name
            .type = "23",
            .valueType = ValueType_string,
            .secureRead = true,
            .hasEvents = false,
          }
        }
      },
#endif
#ifdef thermostat
    [1] = {
        .type = "public.hap.service.thermostat",
        .characteristic = {
          [0] = {
            .type = "public.hap.characteristic.heating-cooling.current",
            .valueType = ValueType_string,
            .hasStaticValue = false,
            .value.stringValue = "cool",  //cool, off, heat
            .secureRead = true,
          },
          [1] = {
            .type = "public.hap.characteristic.heating-cooling.target",
            .valueType = ValueType_string,
            .hasStaticValue = false,
            .value.stringValue = "auto", //cool, off, heat, auto
            .secureRead = true,
            .secureWrite = true,
          },
          [2] = {
            .type = "public.hap.characteristic.temperature.current",
            .valueType = ValueType_float,
            .secureRead = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 0,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 100,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 0.1,
          }, 
          [3] = {
            .type = "public.hap.characteristic.temperature.target",
            .valueType = ValueType_float,
            .secureRead = true,
            .secureWrite = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 10,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 27,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 0.1,
          }, 
          [4] = {
            .type = "public.hap.characteristic.temperature.units",
            .valueType = ValueType_string,
            .hasStaticValue = false,
            .value.stringValue = "celsius", //celsius, fahrenheit, kelvin
            .secureRead = true,
            .secureWrite = true,
          }, 
          [5] = {
            .type = "public.hap.characteristic.relative-humidity.current",
            .valueType = ValueType_float,
            .secureRead = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 0,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 100,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 0.01,
          }, 
          [6] = {
            .type = "public.hap.characteristic.relative-humidity.target",
            .valueType = ValueType_float,
            .secureRead = true,
            .secureWrite = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 0,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 100,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 1,
          }, 
          [7] = {
            .type = "public.hap.characteristic.temperature.heating-threshold",
            .valueType = ValueType_float,
            .secureRead = true,
            .secureWrite = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 0,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 25,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 0.1
          }, 
          [8] = {
            .type = "public.hap.characteristic.temperature.cooling-threshold",
            .valueType = ValueType_float,
            .secureRead = true,
            .secureWrite = true,
            .hasMinimumValue = true,
            .minimumValue.floatValue = 10,
            .hasMaximumValue = true,
            .maximumValue.floatValue = 35,
            .hasMinimumStep = true,
            .minimumStep.floatValue = 0.1,
          }, 
          [9] = {
            .type = "public.hap.characteristic.name",
            .valueType = ValueType_string,
            .hasStaticValue = false,
            .value.stringValue = "William's thermostat",
            .secureRead = true,
            .secureWrite = true,
          }
        }
      }
#endif
    }
  }
};
