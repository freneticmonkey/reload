#ifndef _R_NOTIFY_H_
#define _R_NOTIFY_H_

typedef struct r_file_notify r_file_notify;

void r_file_notify_init();
void r_file_notify_destroy();
void r_file_notify_update(float run_time);

bool r_file_notifier_create(const char *directory, bool *files_changed);
void r_file_notifier_destroy(bool *files_changed);

#endif