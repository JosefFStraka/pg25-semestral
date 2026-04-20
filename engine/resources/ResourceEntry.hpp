#pragma once
#include <memory>
#include <string>
#include <optional>
#include "../../aabb.hpp"

template<typename T>
struct ResourceEntry {
    enum state {
        Error = -1,
        None = 0,
        Loading,
        Ready,
    };

    std::string path;
    std::optional<AABB> aabb;
    std::unique_ptr<T> data{ nullptr };
    state state{state::None};
};
