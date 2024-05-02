#include <corey.hh>

int main(int argc, char* argv[]) {
    corey::Application app(argc, argv, corey::ApplicationInfo{
        .name = "demo",
        .description = "A corey demo application",
        .version = "0.1.0"
    });
    return app.run([](const corey::ParseResult&) -> corey::Future<int> {    
        co_return 42;
    });
}
