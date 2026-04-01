template<typename T>
struct resource_entry {
    std::string path;
    std::optional<std::shared_ptr<T>> data;
};
