/*
#    FVD++, an advanced coaster design tool
#    Copyright (C) 2026 Veia <h27ck@proton.me>
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "stlreader.h"
#include <fstream>
#include <iostream>
#include <sstream>

bool readStl(const std::string& fileName, std::vector<Triangle>& triangles) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file for reading: " << fileName << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize >= 84) {
        file.seekg(80);
        uint32_t numTriangles;
        file.read((char*)&numTriangles, 4);

        if (fileSize == (std::streampos)(84 + (uint64_t)numTriangles * 50)) {
            triangles.reserve(numTriangles);
            for (uint32_t i = 0; i < numTriangles; ++i) {
                Triangle triangle;
                float normal[3];
                file.read((char*)normal, 12);
                triangle.normal = glm::dvec3(normal[0], normal[1], normal[2]);

                for (int v = 0; v < 3; ++v) {
                    float vertex[3];
                    file.read((char*)vertex, 12);
                    triangle.vertices[v] = glm::dvec3(vertex[0], vertex[1], vertex[2]);
                }

                char attr[2];
                file.read(attr, 2);

                triangles.push_back(triangle);
            }
            file.close();
            return true;
        }
    }

    // Fallback to ASCII parsing
    file.close();
    file.open(fileName);
    std::string line;

    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
            continue;
        std::string trimmed = line.substr(start);

        if (trimmed.find("facet normal") == 0) {
            Triangle triangle;
            std::stringstream ss(trimmed);
            std::string dummy, dummy2;
            ss >> dummy >> dummy2 >> triangle.normal.x >> triangle.normal.y >> triangle.normal.z;

            std::getline(file, line); // skip "outer loop"
            for (int i = 0; i < 3; ++i) {
                std::getline(file, line);
                std::stringstream vss(line);
                std::string vdummy;
                vss >> vdummy >> triangle.vertices[i].x >> triangle.vertices[i].y >> triangle.vertices[i].z;
            }
            std::getline(file, line); // skip "endloop"
            std::getline(file, line); // skip "endfacet"
            triangles.push_back(triangle);
        }
    }

    file.close();
    return true;
}

std::vector<glm::dvec3> extractVertices(const std::vector<Triangle>& triangles) {
    std::vector<glm::dvec3> vertices;
    for (const Triangle& triangle : triangles) {
        for (const glm::dvec3& vertex : triangle.vertices) {
            vertices.push_back(vertex);
        }
    }
    return vertices;
}

std::vector<glm::dvec3> extractVerticesNormal(const std::vector<Triangle>& triangles) {
    std::vector<glm::dvec3> vertices;
    for (const Triangle& triangle : triangles) {
        glm::dvec3 normal = triangle.normal;
        for (const glm::dvec3& vertex : triangle.vertices) {
            vertices.push_back(vertex);
            vertices.push_back(normal);
        }
    }
    return vertices;
}
