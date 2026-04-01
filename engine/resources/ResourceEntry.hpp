#pragma once
template<typename T>
struct ResourceEntry {
    std::string path;
    std::unique_ptr<T> data{ nullptr };
};
