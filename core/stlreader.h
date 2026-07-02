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

#ifndef STLREADER_H
#define STLREADER_H

#include <vector>
#include <string>
#include "lenassert.h"

struct Triangle {
    glm::dvec3 normal;
    glm::dvec3 vertices[3];
};

bool readStl(const std::string& fileName, std::vector<Triangle>& triangles);

std::vector<glm::dvec3> extractVertices(const std::vector<Triangle>& triangles);
std::vector<glm::dvec3> extractVerticesNormal(const std::vector<Triangle>& triangles);

#endif // STLREADER_H
