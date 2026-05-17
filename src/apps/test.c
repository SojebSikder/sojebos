// Temporary: This is for just testing
#include "../kernel/drivers/console.h"
#include "../kernel/libc/vector.h"

typedef struct {
  int x;
  int y;
} Point;


// test App
void test_app() {
  Vector points;

  // Initialize vector to hold 'Point' structs, starting with a capacity of 2
  if (!vector_init(&points, 2, sizeof(Point))) {
    // Handle init failure
    return;
  }

  // Add elements
  Point p1 = {10, 20};
  Point p2 = {30, 40};
  Point p3 = {
      50,
      60}; // This will trigger the internal resize (kmalloc -> copy -> kfree)

  vector_push_back(&points, &p1);
  vector_push_back(&points, &p2);
  vector_push_back(&points, &p3);

  // Read elements back
  for (size_t i = 0; i < points.size; i++) {
    Point *p = (Point *)vector_get(&points, i);
    if (p) {
      // Do something with p->x and p->y
      console_printf("Point %zu: x=%d, y=%d\n", i, p->x, p->y);
    }
  }

  // Clean up memory
  vector_free(&points);
}
