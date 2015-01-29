/***
 * File: patch_keil.h
 * porting iar project to keil, need to do something.
 *
 * */

#ifndef __PATCH_KEIL_H__
#define __PATCH_KEIL_H__

int _mutex_initialize(void* mutex);
void _mutex_acquire(void* mutex);

void _mutex_release(void* mutex);
void _mutex_free(void* mutex);


#endif

