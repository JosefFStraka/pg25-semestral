#include "engine/rendering/Mesh.hpp"

// Created by JJ, based on https://www.cs.umd.edu/gvil/papers/av_ts.pdf
// Unit cube
Mesh generateCube();

/**
 * Generates a unit sphere mesh using a single long triangle strip with indices,
 * using degenerate triangles to connect the rings into one strip.
 * 
 * @param sectors The number of divisions around the longitude (vertical rings).
 * @param rings The number of divisions along the latitude (horizontal stacks).
 */
  
Mesh generateSphere(unsigned int sectors, unsigned int rings);
