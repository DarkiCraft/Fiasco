#include <fiasco/fiasco.hpp>

int main() {
  fiasco::server app;

  app.get("/", []() {
      // return fiasco::json{{"msg", "Hello, World!"}};
      return fiasco::detail::response::to_text("Hello, World!"); // temporary workaround
  });

  app.run(8080);
}