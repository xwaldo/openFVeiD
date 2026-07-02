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

#include "smoothhandler.h"
#include "track.h"
#include <string>
#include <iostream>

#include "exportfuncs.h"

smoothHandler::smoothHandler(track* _track, int _section, char* customChar,
                             int _length, int _iterations, int _fromNode,
                             int _toNode) {
    m_track = _track;
    active = false;

    if (_section == -1) {
        sec = (section*)-1;
    } else if (_section == -2) {
        sec = NULL;
        active = true;
    } else {
        sec = m_track->lSections[_section];
    }
    if (sec == NULL) {
        toNode = _toNode;
        fromNode = _fromNode;
        name = "custom Region";
    }
    length = _length;
    iterations = _iterations;

    update(customChar);
}

smoothHandler::~smoothHandler() {}

void smoothHandler::update(char* customChar) {
    if (sec == (section*)-1) {
        toNode = m_track->getNumPoints();
        fromNode = 0;
    } else if (sec != NULL) {
        fromNode = m_track->getNumPoints(sec);
        toNode = fromNode + sec->lNodes.size() - 2;
    }

    if (sec == NULL) {
        if (customChar != NULL) {
            (*customChar)++;
        }
    } else {
        if (sec == (section*)-1) {
            name = m_track->name;
        } else {
            name = sec->sName;
        }
    }
}

int smoothHandler::getFrom() {
    return fromNode;
}

int smoothHandler::getTo() {
    return toNode;
}

int smoothHandler::getLength() {
    return length;
}

int smoothHandler::getIterations() {
    return iterations;
}

void smoothHandler::setFrom(int _arg) {
    fromNode = _arg;
}
void smoothHandler::setTo(int _arg) {
    toNode = _arg;
}

void smoothHandler::setLength(int _arg) {
    length = _arg;
}

void smoothHandler::setIterations(int _arg) {
    iterations = _arg;
}

void smoothHandler::saveSmooth(std::ostream& file) {
    int namelength = name.length();

    writeBytes(&file, (const char*)&namelength, sizeof(int));
    file << name;

    writeBytes(&file, (const char*)&fromNode, sizeof(int));
    writeBytes(&file, (const char*)&toNode, sizeof(int));
    writeBytes(&file, (const char*)&length, sizeof(int));
    writeBytes(&file, (const char*)&iterations, sizeof(int));
    writeBytes(&file, (const char*)&active, sizeof(bool));
}

void smoothHandler::loadSmooth(std::istream& file) {
    int namelength = readInt(&file);
    name = readString(&file, namelength);

    setFrom(readInt(&file));
    setTo(readInt(&file));
    setLength(readInt(&file));
    setIterations(readInt(&file));
    active = readBool(&file);

    update();
}
