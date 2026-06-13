#include "fiasco/fiasco.hpp"

auto root() { return fiasco::json{{"msg", "root"}}; }
auto test() { return fiasco::json{{"msg", "test"}}; }

int main() {
  fiasco::server app;

  app.get("/", root);

  fiasco::router r("/test");
  r.get("/", test);

  app.include_router(r);
  app.include_router(r, "/test");

  app.run(8080);
}