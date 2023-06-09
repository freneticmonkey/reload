#include <stdio.h>
#include <CoreServices/CoreServices.h>

void callback(ConstFSEventStreamRef streamRef,
              void *clientCallBackInfo,
              size_t numEvents,
              void *eventPaths,
              const FSEventStreamEventFlags eventFlags[],
              const FSEventStreamEventId eventIds[]) {
    char **paths = eventPaths;
    for (int i = 0; i < numEvents; i++) {
        printf("File %s changed\n", paths[i]);
    }
}

int main() {
    const char *path = "/path/to/directory";
    CFStringRef cfPath = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&cfPath, 1, NULL);
    FSEventStreamRef stream = FSEventStreamCreate(NULL,
                                                  &callback,
                                                  NULL,
                                                  pathsToWatch,
                                                  kFSEventStreamEventIdSinceNow,
                                                  1.0,
                                                  kFSEventStreamCreateFlagNone);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);
    CFRunLoopRun();
    return 0;
}