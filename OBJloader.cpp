#include <string>
#include <algorithm>
#include <GL/glew.h> 
#include <glm/glm.hpp>
#include <iostream>
#include <unordered_map>

#include "OBJloader.hpp"

#define MAX_LINE_SIZE 256 //should not be longer
#define MAX_VERTICES 4
#define NO_INDEX 0

long resolve_position(long number, long max) {
	long idx = (number > 0) ? number - 1 : max + number;

	if (idx < 0 || idx >= max) {
		return -1; // invalid
	}
	return idx;
}
std::vector<std::string> split(const std::string& str, char delimiter) {
	std::vector<std::string> tokens;
	std::stringstream ss(str);
	std::string item;

	while (std::getline(ss, item, delimiter)) {
		tokens.push_back(item);
	}

	return tokens;
}

bool loadOBJ(const std::filesystem::path& filename, std::vector<Vertex>& vertices, std::vector<GLuint>& indices) {
	std::cout << "Loading model: " << filename.string() << std::endl;

	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

	std::unordered_map<Vertex, GLuint, VertexHasher> vertexCache;

	vertices.clear();
	indices.clear();

	FILE* file = nullptr;
	fopen_s(&file, filename.string().c_str(), "r");
	if (file == NULL) {
		printf("Impossible to open the file !\n");
		return false;
	}

	int line_number = 0;
	while (1) {
		line_number++;
		char line[MAX_LINE_SIZE];
		if (!fgets(line, sizeof(line), file)) {
			break;
		}

		char lineHeader[3];
		int res = sscanf_s(line, "%2s", lineHeader, 3);
		if (res == EOF) {
			continue;
		}
		if (strcmp(lineHeader, "#") == 0) continue;
		if (strcmp(lineHeader, "v") == 0) {
			glm::vec3 vertex;
			sscanf_s(line + 2, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		} else if (strcmp(lineHeader, "vt") == 0) {
			glm::vec2 uv;
			sscanf_s(line + 2, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		} else if (strcmp(lineHeader, "vn") == 0) {
			glm::vec3 normal;
			sscanf_s(line + 2, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		} else if (strcmp(lineHeader, "f") == 0) {
			std::stringstream ss(line);
			std::string prefix;
			ss >> prefix; // skip 'f'

			std::vector<Vertex> faceVertices;

			std::string vertToken;
			while (ss >> vertToken) {
				std::vector<std::string> parts = split(vertToken, '/');

				int vi = NO_INDEX, ti = NO_INDEX, ni = NO_INDEX;

				if (parts.size() >= 1 && !parts[0].empty())
					vi = std::stoi(parts[0]);

				if (parts.size() >= 2 && !parts[1].empty())
					ti = std::stoi(parts[1]);

				if (parts.size() >= 3 && !parts[2].empty())
					ni = std::stoi(parts[2]);

				Vertex v{};

				// position
				if (vi != NO_INDEX) {
					long idx = resolve_position(vi, (long)temp_vertices.size());
					if (idx >= 0 && idx < (long)temp_vertices.size()) {
						v.position = temp_vertices[idx];
					} else {
						std::cerr << "Invalid vertex index at line " << line_number << std::endl;
						continue;
					}
				} else {
					std::cerr << "Missing vertex position at line " << line_number << std::endl;
					continue;
				}

				// texcoords
				if (ti != NO_INDEX) {
					long idx = resolve_position(ti, (long)temp_uvs.size());
					if (idx >= 0 && idx < (long)temp_uvs.size()) {
						v.texCoords = temp_uvs[idx];
					} else {
						v.texCoords = glm::vec2();
					}
				}

				// normals
				if (ni != NO_INDEX) {
					long idx = resolve_position(ni, (long)temp_normals.size());
					if (idx >= 0 && idx < (long)temp_normals.size()) {
						v.normal = temp_normals[idx];
					} else {
						v.normal = glm::vec3();
					}
				} else {
					v.normal = glm::vec3();
				}

				faceVertices.push_back(v);
			}

			// TRIANGULATE (fan method)
			for (size_t i = 1; i + 1 < faceVertices.size(); i++) {
				Vertex tri[3] = {
					faceVertices[0],
					faceVertices[i],
					faceVertices[i + 1]
				};

				for (int k = 0; k < 3; k++) {
					auto it = vertexCache.find(tri[k]);
					GLuint index;

					if (it == vertexCache.end()) {
						index = vertices.size();
						vertices.push_back(tri[k]);
						vertexCache[tri[k]] = index;
					} else {
						index = it->second;
					}

					indices.push_back(index);
				}
			}
		}
	}

	std::cout << "Model loaded: " << filename.string() << std::endl;

	fclose(file);
	return true;
}
