#ifndef __MICO_SYSTEM_MONITOR_H__
#define __MICO_SYSTEM_MONITOR_H__

#include "mico_api.h"
#include "PlatformWDG.h"

/** Structure to hold information about a system monitor item */
typedef struct
{
    uint32_t last_update;              /**< Time of the last system monitor update */
    uint32_t longest_permitted_delay;  /**< Longest permitted delay between checkins with the system monitor */
} mico_system_monitor_t;

void mico_system_monitor_thread_main( void* arg );
OSStatus mico_update_system_monitor( mico_system_monitor_t* system_monitor, uint32_t permitted_delay );
OSStatus mico_register_system_monitor( mico_system_monitor_t* system_monitor, uint32_t initial_permitted_delay );


#endif //__MICO_SYSTEM_MONITOR_H__



