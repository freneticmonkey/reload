#if defined(__APPLE__)

#include <stdio.h>
#include <CoreServices/CoreServices.h>

#include "memory/allocator.h"

#include "filetracker/notify/notify.h"

#define MAX_WATCHERS 64

typedef struct r_file_notify {
    FSEventStreamRef stream;
    CFStringRef      cfPath;
    CFArrayRef       pathsToWatch;
    bool            *files_changed;
} r_file_notify;

void file_change_callback(ConstFSEventStreamRef streamRef,
              void *clientCallBackInfo,
              size_t numEvents,
              void *eventPaths,
              const FSEventStreamEventFlags eventFlags[],
              const FSEventStreamEventId eventIds[]) {
    char **paths = eventPaths;
    for (int i = 0; i < numEvents; i++) {

        // Determine the flag for the event
        FSEventStreamEventFlags flag = eventFlags[i];

        // print the event flags to the console
        printf("Event flags: ");
        
        if (flag & kFSEventStreamEventFlagNone) {
            printf("FlagNone");
        }   
        if (flag & kFSEventStreamEventFlagMustScanSubDirs) {
            printf("FlagMustScanSubDirs");
        } 
        if (flag & kFSEventStreamEventFlagUserDropped) {
            printf("FlagUserDropped");
        } 
        if (flag & kFSEventStreamEventFlagKernelDropped) {
            printf("FlagKernelDropped");
        } 
        if (flag & kFSEventStreamEventFlagEventIdsWrapped) {
            printf("FlagEventIdsWrapped");
        } 
        if (flag & kFSEventStreamEventFlagHistoryDone) {
            printf("FlagHistoryDone");
        } 
        if (flag & kFSEventStreamEventFlagRootChanged) {
            printf("FlagRootChanged");
        } 
        if (flag & kFSEventStreamEventFlagMount) {
            printf("FlagMount");
        }  
        if (flag & kFSEventStreamEventFlagUnmount) {
            printf("FlagUnmount");
        } 
        if (flag & kFSEventStreamEventFlagItemCreated) {
            printf("FlagItemCreated");
        }
        if (flag & kFSEventStreamEventFlagItemRemoved) {
            printf("FlagItemRemoved");
        }
        if (flag & kFSEventStreamEventFlagItemInodeMetaMod) {
            printf("FlagItemInodeMetaMod");
        }
        if (flag & kFSEventStreamEventFlagItemRenamed) {
            printf("FlagItemRenamed");
        }
        if (flag & kFSEventStreamEventFlagItemModified) {
            printf("FlagItemModified");
        }
        if (flag & kFSEventStreamEventFlagItemFinderInfoMod) {
            printf("FlagItemFinderInfoMod");
        }
        if (flag & kFSEventStreamEventFlagItemChangeOwner) {
            printf("FlagItemChangeOwner");
        }
        if (flag & kFSEventStreamEventFlagItemXattrMod) {
            printf("FlagItemXattrMod");
        }
        if (flag & kFSEventStreamEventFlagItemIsFile) {
            printf("FlagItemIsFile");
        }
        if (flag & kFSEventStreamEventFlagItemIsDir) {
            printf("FlagItemIsDir");
        }
        if (flag & kFSEventStreamEventFlagItemIsSymlink) {
            printf("FlagItemIsSymlink");
        }
        if (flag & kFSEventStreamEventFlagOwnEvent) {
            printf("FlagOwnEvent");
        }
        if (flag & kFSEventStreamEventFlagItemIsHardlink) {
            printf("FlagItemIsHardlink");
        }
        if (flag & kFSEventStreamEventFlagItemIsLastHardlink) {
            printf("FlagItemIsLastHardlink");
        }
        if (flag & kFSEventStreamEventFlagItemCloned) {
            printf("FlagItemCloned");
        }

        printf("Path %s changed\n", paths[i]);
    }

    // don't confirm the direct matching, just assume that apple has it sorted
    r_file_notify *notify = (r_file_notify *)clientCallBackInfo;
    *notify->files_changed = true;
}

static CFRunLoopRef  runLoop = NULL;
static r_file_notify watchers[MAX_WATCHERS];
static uint32_t      watcher_count = 0;

// Create a new file notify instance and setup the stream associated with the directory parameter
bool r_file_notifier_create(const char *directory, bool *files_changed) {
    
    if (watcher_count == MAX_WATCHERS) {
        return false;
    }
    
    r_file_notify *notify = &watchers[watcher_count++];

    if (notify == NULL) {
        return false;
    }
    
    notify->files_changed = files_changed;

    FSEventStreamContext ctx = {
        .version = 0,
        .info = (void *)notify,
        .retain = NULL,
        .release = NULL,
    };

    CFStringRef cfPath = CFStringCreateWithCString(NULL, directory, kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&cfPath, 1, NULL);
    FSEventStreamRef stream = FSEventStreamCreate(NULL,
                                                  &file_change_callback,
                                                  &ctx,
                                                  pathsToWatch,
                                                  kFSEventStreamEventIdSinceNow,
                                                  1.0,
                                                  kFSEventStreamCreateFlagNone);
    FSEventStreamScheduleWithRunLoop(stream, runLoop, kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);
    
    notify->stream = stream;
    notify->cfPath = cfPath;
    notify->pathsToWatch = pathsToWatch;

    return true;
}

// Destroy a file notify instance
void r_file_notifier_destroy(bool *files_changed) {

    // Find the notifier using the bool pointer
    r_file_notify *notify = NULL;
    uint32_t inst = 0;
    for (uint32_t i = 0; i < watcher_count; i++) {
        if (watchers[i].files_changed == files_changed) {
            inst = i;
            notify = &watchers[i];
            break;
        }
    }

    FSEventStreamStop(notify->stream);
    FSEventStreamInvalidate(notify->stream);
    FSEventStreamRelease(notify->stream);
    CFRelease(notify->pathsToWatch);
    CFRelease(notify->cfPath);
    
    // shuffle the watchers down to fill the gap
    for (uint32_t i = inst; i < watcher_count - 1; i++) {
        watchers[i] = watchers[i + 1];
    }
    watcher_count--;
}

// r_notify lifecycle functions

// r_file_notify_init creates a runloop reference to be used in the stream creation
// and update pump
void r_file_notify_init() {
    runLoop = CFRunLoopGetCurrent();
}

void r_file_notify_update(float run_time) {
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, run_time, false);
}

void r_file_notify_destroy() {
    runLoop = NULL;
    // Destroy all the watchers
    for (uint32_t i = 0; i < watcher_count; i++) {
        r_file_notifier_destroy(watchers[i].files_changed);
    }
}


#endif
