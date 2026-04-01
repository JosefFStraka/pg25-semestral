#pragma once
#include <unordered_map>
#include <memory>
#include <functional>

#include "ResourceHandle.hpp"
#include "ResourceEntry.hpp"

template<typename T>
class ResourceStorage {
public:
    using loader_fn = std::function<std::unique_ptr<T>(const std::string&)>;
    void setLoader(loader_fn loader) {
        loader_ = loader;
    }

    ResourceHandle<T> create(std::unique_ptr<T> resource, std::string path = "") {
        if (path.empty() && cache_.contains(path)) {
            return cache_.at(path);
        }

        auto id = ++lastResourceId;

        entries_[id] = { path, std::move(resource) };

        ResourceHandle<T> res = ResourceHandle<T>(id);

        if (!path.empty()) {
            cache_[path] = res;
        }

        return res;
    }

    template<typename... Args>
    ResourceHandle<T> emplace(std::string path, Args&&... args) {
        if (path.empty() && cache_.contains(path)) {
            return cache_.at(path);
        }

        auto id = ++lastResourceId;

        std::unique_ptr<T> resource = std::make_unique<T>(std::forward<Args>(args)...);

        entries_[id] = { path, std::move(resource) };

        ResourceHandle<T> res = ResourceHandle<T>(id);

        if (!path.empty()) {
            cache_[path] = res;
        }

        return res;
    }

    T* get(ResourceHandle<T> handle) {
        auto& entry = entries_.at(handle.get_id());

        if (!entry.data) {
            if (!loader_) {
                throw std::runtime_error("No loader set for resource type");
            }

            entry.data = loader_(entry.path);
        }

        return entry.data.get();
    }

    const ResourceEntry<T>& getEntry(ResourceHandle<T> handle) const {
        return entries_.at(handle.get_id());
    }

private:
    loader_fn loader_;
    ResourceId lastResourceId{ 0 };
    std::unordered_map<ResourceId, ResourceEntry<T>> entries_;
    std::unordered_map<std::string, ResourceHandle<T>> cache_;
};
