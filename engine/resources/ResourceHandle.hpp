#pragma once
#include <numeric>
#include <functional>

using ResourceId = uint32_t;

template<typename T>
class ResourceHandle {
public:
    ResourceHandle() = default;
    explicit ResourceHandle(ResourceId id) : id_(id) {}

    ResourceId get_id() const { return id_; }

    bool operator==(const ResourceHandle& other) const {
        return id_ == other.id_;
    }
private:
    ResourceId id_ = 0;
};

template<typename T>
struct std::hash<ResourceHandle<T>> {
    size_t operator()(const ResourceHandle<T>& handle) const noexcept {
        return std::hash<ResourceId>{}(handle.get_id());
    }
};
