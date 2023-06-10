#if defined(__linux__)
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/inotify.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + 16))

int main() {
    const char *path = "/path/to/directory";
    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        exit(1);
    }
    int wd = inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (wd < 0) {
        perror("inotify_add_watch");
        exit(1);
    }

    printf("Tracking directory %s\n", path);

    char buf[BUF_LEN];
    while (1) {
        ssize_t len = read(fd, buf, BUF_LEN);
        if (len < 0) {
            perror("read");
            exit(1);
        }
        char *ptr = buf;
        while (ptr < buf + len) {
            struct inotify_event *event = (struct inotify_event *)ptr;
            if (event->mask & IN_CREATE) {
                printf("File %s created\n", event->name);
            } else if (event->mask & IN_DELETE) {
                printf("File %s deleted\n", event->name);
            } else if (event->mask & IN_MODIFY) {
                printf("File %s modified\n", event->name);
            }
            ptr += EVENT_SIZE + event->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
    return 0;

}
#endif