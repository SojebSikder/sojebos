#ifndef APPS_H
#define APPS_H

typedef void (*AppFunc)();

typedef struct {
  const char *name;
  AppFunc func;
} ConsoleApp;

#define MAX_APPS 8
extern ConsoleApp apps[MAX_APPS];
extern int app_count;

void register_app(const char *name, AppFunc func);
void register_all_apps();

#endif
