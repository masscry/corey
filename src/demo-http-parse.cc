#include "demo-http-parse.hh"

#include "utils/log.hh"
#include "fmt/std.h"
#include "fmt/ostream.h"

#include <compare>
#include <regex>

namespace corey::http {

class MultilineText {
public:
    MultilineText(std::span<const char> text) : _text(text) {}
    MultilineText(const MultilineText&) = default;
    MultilineText& operator=(const MultilineText&) = default;
    ~MultilineText() = default;

    auto text() const { return _text; }

    class iterator {
    public:
        using value_type = std::span<const char>;
        using difference_type = void;
        using pointer = void;
        using reference = std::span<const char>&;
        using iterator_category = std::forward_iterator_tag;
        
        iterator() = default;
        iterator(std::span<const char> line): _left(line) { next(); }
        iterator(const iterator&) = default;
        iterator& operator=(const iterator&) = default;
        ~iterator() = default;

        auto operator*() const { return _cur; }
        auto operator->() { return &_cur; }

        iterator& operator++() { next(); return *this; }
        iterator operator++(int) { auto copy = *this; next(); return copy; }

        auto left() && { return MultilineText(_left); }

        friend bool operator==(const iterator& a, const iterator& b) {
            return a._cur.data() == b._cur.data();
        }

        friend bool operator!=(const iterator& a, const iterator& b) {
            return !(a == b);
        }

    private:

        void next() {
            if (_left.empty()) {
                _cur = {};
                return;
            }
            auto pos = static_cast<size_t>(
                std::find(_left.data(), _left.data() + _left.size(), '\n') - _left.data()
            );
            _cur = _left.subspan(0, pos);
            if (_cur.back() == '\r') {
                _cur = _cur.subspan(0, _cur.size() - 1);
            }
            _left = _left.subspan(pos + 1);
        }

        std::span<const char> _cur;
        std::span<const char> _left;
    };

    auto begin() {
        return iterator{_text};
    }

    auto end() {
        return iterator{};
    }

private:
    std::span<const char> _text;
};

namespace {

const std::regex request_line(R"(([A-Z]+) ([^ ]+) HTTP/1\.1)");
const std::regex header_line(R"(([^:]+): (.+))");

} // namespace

void Request::set_request(std::span<const char> req) {
    std::cmatch match;
    if (!std::regex_search(req.data(), req.data() + req.size(), match, request_line)) {
        throw std::runtime_error(fmt::format("Invalid request line ({})", std::string(req.begin(), req.end())));
    }
    this->method = match[1];
    this->path = match[2];
}

void Request::set_header(std::span<const char> header) {
    std::cmatch match;
    if (!std::regex_match(header.data(), header.data() + header.size(), match, header_line)) {
        throw std::runtime_error(fmt::format("Invalid header line ({})", std::string(header.begin(), header.end())));
    }
    this->headers[match[1]] = match[2];
}

Request parse_request(std::span<const char> request) {
    Log log("parse_request");

    Request result;
    auto req = MultilineText(request);
    enum {
        REQUEST,
        HEADER,
    } state = REQUEST;
    for (auto it = req.begin(), end = req.end(); it != end; ++it) {
        switch (state) {
        case REQUEST:
            log.info("Request line: {}", std::string(it->begin(), it->end()));
            result.set_request(*it);
            state = HEADER;
            break;
        case HEADER:
            if (it->empty()) {
                req = std::move(it).left();
                if (result.headers["Content-Length"].empty()) {
                    return result;
                }
                auto content_length = std::stoul(result.headers["Content-Length"]);
                if (content_length != req.text().size()) {
                    throw std::runtime_error(fmt::format("Invalid content length ({} != {})", content_length, req.text().size()));
                }
                result.body = std::string(req.text().begin(), req.text().end());
                return result;
            }
            log.info("Request line: {}", std::string(it->begin(), it->end()));
            result.set_header(*it);
            break;
        }
    }
    return result;
}

} // namespace corey::http