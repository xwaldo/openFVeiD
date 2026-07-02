/*
#    FVD++, an advanced coaster design tool for NoLimits
#    Copyright (C) 2012-2015, Stephan "Lenny" Alt <alt.stephan@web.de>
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
#    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "exportfuncs.h"
#include <algorithm>

using namespace std;

void writeBytes(ostream* file, const char* data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        file->write(data + length - 1 - i, 1);
    }
}

void writeFloats(ostream* file, const float data) {
    *file << std::fixed << std::setprecision(2) << data;
    *file << " ";
}

void writeNulls(ostream* file, size_t length) {
    char data[1] = {0x00};
    for (size_t i = 0; i < length; i++) {
        writeBytes(file, data, 1);
    }
}

void writeVec3(ostream* file, const glm::vec3& vec) {
    writeBytes(file, (const char*)&vec.x, sizeof(float));
    writeBytes(file, (const char*)&vec.y, sizeof(float));
    writeBytes(file, (const char*)&vec.z, sizeof(float));
}

string readString(istream* file, size_t length) {
    if (length > 10485760) { // Sanity bound: 10MB to prevent OOM on corrupted files
        std::cerr << "Warning: readString length (" << length << ") exceeded 10MB safety bound. Clamping to 10MB." << std::endl;
        length = 10485760;
    }
    string temp(length, '\0');
    file->read(&temp[0], length);
    return temp;
}

bool readNulls(istream* file, size_t length) {
    file->ignore(length);
    return true;
}

glm::vec3 readVec3(istream* file) {
    float x = readFloat(file);
    float y = readFloat(file);
    float z = readFloat(file);
    return glm::vec3(x, y, z);
}

float readFloat(istream* file) {
    union {
        char c[4];
        float f;
    } temp;
    file->read(temp.c, 4);
    std::swap(temp.c[0], temp.c[3]);
    std::swap(temp.c[1], temp.c[2]);
    return temp.f;
}

int readInt(istream* file) {
    union {
        char c[4];
        int i;
    } temp;
    file->read(temp.c, 4);
    std::swap(temp.c[0], temp.c[3]);
    std::swap(temp.c[1], temp.c[2]);
    return temp.i;
}

bool readBool(istream* file) {
    char temp;
    file->read(&temp, 1);
    return temp != 0;
}

void readBytes(istream* file, void* _ptr, size_t length) {
    char* ptr = (char*)_ptr;
    for (size_t i = 0; i < length; i++) {
        file->read(ptr + length - 1 - i, 1);
    }
}

void writeToExportFile(std::ostream* file, std::vector<bezier_t*>& bezList) {
    // Note: Implementation stays identical, just using ostream*
    for (int i = 0; i < (int)bezList.size(); ++i) {
        float kp1x = (float)bezList[i]->Kp1.x;
        float kp1y = (float)bezList[i]->Kp1.y;
        float kp1z = (float)bezList[i]->Kp1.z;
        writeBytes(file, (const char*)&kp1x, 4);
        writeBytes(file, (const char*)&kp1y, 4);
        writeBytes(file, (const char*)&kp1z, 4);

        float kp2x = (float)bezList[i]->Kp2.x;
        float kp2y = (float)bezList[i]->Kp2.y;
        float kp2z = (float)bezList[i]->Kp2.z;
        writeBytes(file, (const char*)&kp2x, 4);
        writeBytes(file, (const char*)&kp2y, 4);
        writeBytes(file, (const char*)&kp2z, 4);

        float p1x = (float)bezList[i]->P1.x;
        float p1y = (float)bezList[i]->P1.y;
        float p1z = (float)bezList[i]->P1.z;
        writeBytes(file, (const char*)&p1x, 4);
        writeBytes(file, (const char*)&p1y, 4);
        writeBytes(file, (const char*)&p1z, 4);

        float roll = (float)bezList[i]->roll;
        writeBytes(file, (const char*)&roll, 4);

        char cTemp = 0xFF;
        writeBytes(file, &cTemp, 1); // CONT ROLL
        cTemp = bezList[i]->relRoll ? 0xFF : 0x00;
        writeBytes(file, &cTemp, 1); // REL ROLL
        cTemp = 0x00;
        writeBytes(file, &cTemp, 1); // equal dist CP
        writeNulls(file, 7);         // were 5
    }
}

void writeToExportFileAscii(std::ostream* file, std::vector<bezier_t*>& bezList) {
    for (int i = 0; i < (int)bezList.size(); ++i) {
        writeFloats(file, bezList[i]->Kp1.x);
        writeFloats(file, bezList[i]->Kp1.y);
        writeFloats(file, bezList[i]->Kp1.z);

        writeFloats(file, bezList[i]->Kp2.x);
        writeFloats(file, bezList[i]->Kp2.y);
        writeFloats(file, bezList[i]->Kp2.z);

        writeFloats(file, bezList[i]->P1.x);
        writeFloats(file, bezList[i]->P1.y);
        writeFloats(file, bezList[i]->P1.z);

        writeFloats(file, bezList[i]->roll);

        *file << std::endl;
    }
}
