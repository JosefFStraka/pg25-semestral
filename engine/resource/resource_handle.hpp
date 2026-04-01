#include <numeric>

using resource_id = uint32_t;

template<typename T>
class resource_handle {
public:
    resource_handle() = default;
    explicit resource_handle(resource_id id) : id_(id) {}

    resource_id get_id() const { return id_; }

private:
    resource_id id_ = 0;
};
