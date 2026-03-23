#include <string>
#include <algorithm>
#include <GL/glew.h> 
#include <glm/glm.hpp>
#include <iostream>
#include <unordered_map>

#include "OBJloader.hpp"

#define MAX_LINE_SIZE 256 //should not be longer
#define MAX_VERTICES 3
#define NO_INDEX    -1

long resolve_position(long number, size_t max) {
	if (number < 0) {
		return max + number;
	}
	return number - 1;
}

typedef struct {
	int vertex[MAX_VERTICES];
	int tex[MAX_VERTICES];
	int normal[MAX_VERTICES];
} Face;

int parseFaceLine(const char* line, Face* face) {
	// Initialize all indices to NO_INDEX
	for (int i = 0; i < MAX_VERTICES; i++)
		face->vertex[i] = face->tex[i] = face->normal[i] = NO_INDEX;

	// Try v/vt/vn first
	int n = sscanf_s(line,
		"%d/%d/%d %d/%d/%d %d/%d/%d",
		&face->vertex[0], &face->tex[0], &face->normal[0],
		&face->vertex[1], &face->tex[1], &face->normal[1],
		&face->vertex[2], &face->tex[2], &face->normal[2]);
	if (n == 9) return 4;

	// Try v//vn (no texture)
	n = sscanf_s(line,
		"%d//%d %d//%d %d//%d",
		&face->vertex[0], &face->normal[0],
		&face->vertex[1], &face->normal[1],
		&face->vertex[2], &face->normal[2]);
	if (n == 6) return 3;

	// Try v/vt (no normals)
	n = sscanf_s(line,
		"%d/%d %d/%d %d/%d",
		&face->vertex[0], &face->tex[0],
		&face->vertex[1], &face->tex[1],
		&face->vertex[2], &face->tex[2]);
	if (n == 6) return 2;

	// Try vertex only
	n = sscanf_s(line, "%d %d %d",
		&face->vertex[0],
		&face->vertex[1],
		&face->vertex[2]);
	if (n == 3) return 1;

	return 0; // failed to parse
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
			Face face;
			int result = parseFaceLine(line + 2, &face);
			if (result == 0) {
				printf("File can't be read by simple parser :( Try exporting with other options\n\t\"%s\"\n", line + 2);
			}

			for (int i = 0; i < 3; i++) {
				GLuint currentIndex;
				Vertex currentVertex;
				long vi = resolve_position(face.vertex[i], temp_vertices.size());
				// if (vi < 0 || vi >= temp_vertices.size()) {
				// 	printf("erm %d\n", line_number);
				// 	continue; // skip invalid
				// }

				currentVertex.position = temp_vertices[vi]; // OBJ array start from 1

				if (result == 4 || result == 2)
					currentVertex.texCoords = temp_uvs[resolve_position(face.tex[i], temp_uvs.size())];
				else
					currentVertex.texCoords = glm::vec2();

				if (result == 4 || result == 3)
					currentVertex.normal = temp_normals[resolve_position(face.normal[i], temp_normals.size())];
				else
					currentVertex.normal = glm::vec3();

				// avoid duplicit vertices
				auto it = vertexCache.find(currentVertex);
				GLuint index;
				if (it == vertexCache.end()) {
					index = vertices.size();
					vertices.push_back(currentVertex);
					vertexCache[currentVertex] = index;
				} else {
					index = it->second;
				}
				indices.push_back(index);
			}
		}
	}

	std::cout << "Model loaded: " << filename.string() << std::endl;

	fclose(file);
	return true;
}
