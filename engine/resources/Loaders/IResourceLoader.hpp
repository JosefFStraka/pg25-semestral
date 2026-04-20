#pragma once
#include <vector>

#include "engine/resources/Descriptors/ResourceDescriptor.hpp"

struct CPUData {};

class IResourceLoader {
    virtual CPUData* ProcessCPU(ResourceDescriptor* descriptor, std::vector<ResourceDescriptor*> dependencies, ResourceDescriptor* parent) = 0;
    virtual void* ProcessGPU(CPUData* data) = 0;
};
