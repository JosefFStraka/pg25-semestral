#pragma once
#include <numeric>

using ResourceId = uint32_t;

template<typename T>
class ResourceHandle {
public:
    ResourceHandle() = default;
    explicit ResourceHandle(ResourceId id) : id_(id) {}

    ResourceId get_id() const { return id_; }

private:
    ResourceId id_ = 0;
};
