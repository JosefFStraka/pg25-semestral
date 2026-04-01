#pragma once
template<typename T>
struct ResourceEntry {
    std::string path;
    std::shared_ptr<T> data{ nullptr };
};
