#pragma once
#include <vector>

struct ResourceDescriptor {
    enum eResourceType {
        None,
        Mesh,
        Texture,
        Shader
    };

    eResourceType type;

    ResourceDescriptor(eResourceType t) : type(t) {}
    virtual ~ResourceDescriptor() = 0;
};
