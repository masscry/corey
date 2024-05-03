#include <corey.hh>

#include <cstdint>
#include <cstdlib>

int main(int argc, char* argv[]) {
    using namespace corey;

    auto app = Application(argc, argv, ApplicationInfo{
        .name = "demo",
        .description = "A corey demo application",
        .version = "0.1.0"
    });

    app.add_options()
        ("input", "Input file", cxxopts::value<std::string>())
        ("output", "Output file", cxxopts::value<std::string>());
    app.add_positional_options("input", "output");
    app.set_positional_help("input output");

    return app.run([](const ParseResult& opts) -> Future<int> {    
        auto console = Sink<Console>::make();
        if (opts.count("input") == 0) {
            console.write(fmt::format("Error: missing input file\n"));
            co_return EXIT_FAILURE;
        }
        if (opts.count("output") == 0) {
            console.write(fmt::format("Error: missing output file\n"));
            co_return EXIT_FAILURE;
        }

        auto input = co_await File::open(opts["input"].as<std::string>().c_str(), O_RDONLY);
        auto output = co_await File::open(opts["output"].as<std::string>().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);

        std::exception_ptr eptr;
        try {
            uint64_t cursor = 0;
            std::vector<Future<uint64_t>> futures;
            while (true) {
                char buf[4096];
                auto read_bytes = co_await input.read(cursor, std::span(buf, sizeof(buf)));
                auto written_bytes = co_await output.write(cursor, std::span(buf, read_bytes));
                if (read_bytes != written_bytes) {
                    co_await std::make_exception_ptr(std::runtime_error("write failed"));
                }
                if (read_bytes < sizeof(buf)) { break; }
                cursor += read_bytes;
            }
        } catch (...) {
            eptr = std::current_exception();
        }
        co_await input.close();
        co_await output.close();

        if (eptr) { co_return eptr; }
        co_return EXIT_SUCCESS;
    });
}
