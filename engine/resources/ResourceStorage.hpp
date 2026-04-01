#pragma once
#include <unordered_map>
#include <memory>
#include <functional>

#include "ResourceHandle.hpp"
#include "ResourceEntry.hpp"

template<typename T>
class ResourceStorage {
public:
    using loader_fn = std::function<std::shared_ptr<T>(const std::string&)>;
    void setLoader(loader_fn loader) {
        loader_ = loader;
    }

    ResourceHandle<T> create(ResourceEntry<T> entry) {
        auto id = ++lastResourceId;
        entries_[id] = std::move(entry);
        return ResourceHandle<T>(id);
    }

    template<typename... Args>
    ResourceHandle<T> emplace(std::string path, Args&&... args) {
        auto id = ++lastResourceId;

        std::shared_ptr<T> resource = std::make_shared<T>(std::forward<Args>(args)...);

        entries_[id] = { path, resource };

        return ResourceHandle<T>(id);
    }

    std::shared_ptr<T> get(ResourceHandle<T> handle) {
        auto& entry = entries_.at(handle.get_id());

        if (!entry.data) {
            if (!loader_) {
                throw std::runtime_error("No loader set for resource type");
            }

            entry.data = loader_(entry.path);
        }

        return entry.data;
    }

    const ResourceEntry<T>& getEntry(ResourceHandle<T> handle) const {
        return entries_.at(handle.get_id());
    }

private:
    loader_fn loader_;
    ResourceId lastResourceId{ 0 };
    std::unordered_map<ResourceId, ResourceEntry<T>> entries_;
};
