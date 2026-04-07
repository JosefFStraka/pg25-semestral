#pragma once

template<typename T>
struct ResourceEntry {
    enum state {
        Error = -1,
        None = 0,
        Loading,
        Ready,
    };

    std::string path;
    std::unique_ptr<T> data{ nullptr };
    state state{state::None};
};
