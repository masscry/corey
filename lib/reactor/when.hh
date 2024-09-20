#pragma once

#include "future.hh"
#include "reactor.hh"
#include "coroutine.hh"

#include <vector>

namespace corey {

template<typename... Args>
struct AnyResult {
    int index;
    std::tuple<Args...> futs;
};

template<typename... Args>
auto when_any(Args&&... futs) -> Future<AnyResult<Args...>> {
    while(true) {
        int index = 0;
        for (auto& item: { std::ref(futs)...}) {
            if (item.get().is_ready()) {
                co_return AnyResult<Args...>{
                    .index = index,
                    .futs = std::make_tuple(std::forward<Args>(futs)...)
                };
            }
            ++index;
        }
        co_await yield();
    }
}

template<typename... Args>
auto when_all(Args&&... futs) -> Future<> {
    auto fut_tuple = std::make_tuple(std::forward<Args>(futs)...);
    for (auto& item: fut_tuple) {
        co_await std::move(item);
    }
}

} // namespace corey
