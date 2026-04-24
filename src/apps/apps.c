#include "apps.h"
#include "../apps/calc.h"
#include "../apps/clear.h"
#include "../apps/coreutils.h"
#include "../apps/hello.h"
#include "../apps/ping.h"

ConsoleApp apps[MAX_APPS];
int app_count = 0;

void register_app(const char *name, AppFunc func) {
  if (app_count < MAX_APPS) {
    apps[app_count].name = name;
    apps[app_count].func = func;
    app_count++;
  }
}

void register_all_apps() {
  register_app("hello", hello_app);
  register_app("calc", calculator_app);
  register_app("clear", clear_app);
  register_app("ls", ls_app);
  register_app("cat", cat_app);
  register_app("write", write_app);
  register_app("rm", rm_app);
  register_app("df", disk_usage_app);
  register_app("ping", command_ping);
}
