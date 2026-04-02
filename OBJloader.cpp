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
	//std::cout << "Loading model: " << filename.string() << std::endl;

	std::vector< glm::vec3 > temp_vertices;
	std::vector< glm::vec2 > temp_uvs;
	std::vector< glm::vec3 > temp_normals;

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
			char* ptr = line + 2; // skip "f "

			std::vector<Vertex> faceVertices;

			while (*ptr) {
				// skip whitespace
				while (*ptr == ' ' || *ptr == '\t') ptr++;
				if (*ptr == '\0' || *ptr == '\n') break;

				int vi = 0, ti = 0, ni = 0;

				// --- parse vertex index ---
				vi = strtol(ptr, &ptr, 10);

				// --- parse texcoord / normal ---
				if (*ptr == '/') {
					ptr++;

					// texture index (optional)
					if (*ptr != '/') {
						ti = strtol(ptr, &ptr, 10);
					}

					// normal index
					if (*ptr == '/') {
						ptr++;
						ni = strtol(ptr, &ptr, 10);
					}
				}

				Vertex v{};

				// position (required in practice)
				if (vi != 0) {
					long idx = resolve_position(vi, (long)temp_vertices.size());
					if (idx >= 0 && idx < (long)temp_vertices.size()) {
						v.position = temp_vertices[idx];
					} else {
						std::cerr << "Invalid vertex index at line " << line_number << std::endl;
						continue;
					}
				}

				// texcoords
				if (ti != 0) {
					long idx = resolve_position(ti, (long)temp_uvs.size());
					if (idx >= 0 && idx < (long)temp_uvs.size()) {
						v.texCoords = temp_uvs[idx];
					}
				} else {
					v.texCoords = glm::vec2();
				}

				// normals
				if (ni != 0) {
					long idx = resolve_position(ni, (long)temp_normals.size());
					if (idx >= 0 && idx < (long)temp_normals.size()) {
						v.normal = temp_normals[idx];
					}
				} else {
					v.normal = glm::vec3();
				}

				faceVertices.push_back(v);
			}

			// --- triangulate (fan) ---
			for (size_t i = 1; i + 1 < faceVertices.size(); i++) {
				Vertex tri[3] = {
					faceVertices[0],
					faceVertices[i],
					faceVertices[i + 1]
				};

				for (int k = 0; k < 3; k++) {
					GLuint index;

					index = vertices.size();
					vertices.push_back(tri[k]);
					indices.push_back(index);
				}
			}
		}
	}

	//std::cout << "Model loaded: " << filename.string() << std::endl;

	fclose(file);
	return true;
}
