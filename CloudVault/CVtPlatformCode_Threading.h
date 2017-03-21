//
//  CVtPlatformCode_Threading.h
//  CloudVault
//
//  Created by Wai Man Chan on 2/14/17.
//
//

#ifndef CVtPlatformCode_Threading_h
#define CVtPlatformCode_Threading_h

#include <stdio.h>

#ifdef __APPLE__
#include <dispatch/dispatch.h>
//Regarding semaphore
#define CVtsemaphore dispatch_semaphore_t
#define CVtsemaphoreCreate(initCount) dispatch_semaphore_create(initCount)
#define CVtsemaphoreLock(sem) dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER)
#define CVtsemaphoreUnlock(sem) dispatch_semaphore_signal(sem)
#define CVtsemaphoreInstantLock(sem) dispatch_semaphore_wait(sem, DISPATCH_TIME_NOW)==0
#define CVtsemaphoreRelease(sem) dispatch_release(sem);
//Regarding threading
#define CVtthreading_pool dispatch_queue_t
#define CVtthreading_poolCreate(name) dispatch_queue_create(name, NULL);
#define CVtthreading_performAsyncNoPara(pool, func) dispatch_async(pool, ^{ func(); } )
#define CVtthreading_performSyncNoPara(pool, func) dispatch_sync(pool, ^{ func(); } )
#endif

#endif /* CVtPlatformCode_Threading_h */
