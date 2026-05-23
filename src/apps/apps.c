#include "apps.h"
#include "../apps/calc.h"
#include "../apps/clear.h"
#include "../apps/coreutils.h"
#include "../apps/hello.h"
#include "../apps/ping.h"
#include "../apps/test.h"
#include "../kernel/drivers/console.h"
#include "../kernel/libc/vector.h"

Vector apps_vector;

void register_app(const char *name, AppFunc func) {
  ConsoleApp new_app;
  new_app.name = name;
  new_app.func = func;

  if (!vector_push_back(&apps_vector, &new_app)) {
    console_printf("Failed to register app '%s'\n", name);
  }
}

void register_all_apps() {
  if (!vector_init(&apps_vector, 16, sizeof(ConsoleApp))) {
    return;
  }

  register_app("hello", hello_app);
  register_app("calc", calculator_app);
  register_app("clear", clear_app);
  register_app("ls", ls_app);
  register_app("cat", cat_app);
  register_app("write", write_app);
  register_app("rm", rm_app);
  register_app("df", disk_usage_app);
  register_app("ping", command_ping);
  register_app("test", test_app);
}
