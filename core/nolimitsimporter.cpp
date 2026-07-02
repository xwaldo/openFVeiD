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

#include "nolimitsimporter.h"
#include "trackhandler.h"
#include "track.h"
#include "section.h"
#include "exportfuncs.h"
#include "secbezier.h"
#include <fstream>
#include <iostream>
#include <glm/gtx/quaternion.hpp>

using namespace std;

noLimitsImporter::noLimitsImporter(trackHandler* _track, std::string _fileName) {
    inTrack = _track;
    fileName = _fileName;
}

bool noLimitsImporter::importAsNlTrack() {
    fstream fin(fileName.c_str(), ios::in | ios::binary);
    if (!fin) {
        return false;
    }

    section* activeSec = inTrack->trackData->activeSection;
    std::vector<bezier_t*>* bList = &activeSec->bezList;
    std::vector<glm::dvec3>* lineList = &activeSec->supList;

    // Clear existing data
    for (auto b : *bList)
        delete b;
    bList->clear();
    lineList->clear();

    fin.seekg(0, ios::end);
    int length = fin.tellg();
    string temp;
    std::vector<glm::dvec3> TubeNodes[3];

    int bezCount, fundCount, freeCount, tubeCount;
    glm::dvec3 anchor(0.0);
    bool closeTrack = false;

    for (int i = 0; i < length; ++i) {
        fin.seekg(i);
        temp = readString(&fin, 4);
        if (temp == "SEGM") {
            readNulls(&fin, 4);
            int segCount = readInt(&fin);
            closeTrack = readBool(&fin);
            readNulls(&fin, 16);
            for (int k = 0; k < segCount; ++k) {
                int type = readInt(&fin);
                readNulls(&fin, 29);
                // We don't have bList yet here, wait for BEZR block
            }
        }
        if (temp == "BEZR") {
            readInt(&fin); // size
            readNulls(&fin, 16);
            bezCount = readInt(&fin);
            for (int b = 0; b < bezCount; ++b) {
                bezier_t* nb = new bezier_t;
                nb->P1.x = readFloat(&fin);
                nb->P1.y = readFloat(&fin);
                nb->P1.z = readFloat(&fin);
                nb->Kp1.x = readFloat(&fin);
                nb->Kp1.y = readFloat(&fin);
                nb->Kp1.z = readFloat(&fin);
                nb->Kp2.x = readFloat(&fin);
                nb->Kp2.y = readFloat(&fin);
                nb->Kp2.z = readFloat(&fin);
                nb->roll = readFloat(&fin);
                nb->contRoll = readBool(&fin);
                nb->equalDist = readBool(&fin);
                nb->relRoll = readBool(&fin);
                readNulls(&fin, 17);

                if (b == 0)
                    anchor = nb->P1;
                nb->P1 -= anchor;
                nb->Kp1 -= anchor;
                nb->Kp2 -= anchor;

                nb->ptf = 0.0;
                nb->fvdRoll = 0.0;
                nb->fVel = 0.0;
                bList->push_back(nb);
            }
        }
        if (temp == "FUND") {
            readInt(&fin); // size
            readNulls(&fin, 16);
            fundCount = readInt(&fin);
            for (int b = 0; b < fundCount; ++b) {
                glm::dvec3 tNode;
                readNulls(&fin, 16);
                tNode.x = readFloat(&fin);
                tNode.y = readFloat(&fin);
                tNode.z = readFloat(&fin);
                readNulls(&fin, 64);
                TubeNodes[0].push_back(tNode);
            }
        }
        if (temp == "FREN") {
            readInt(&fin); // size
            readNulls(&fin, 16);
            freeCount = readInt(&fin);
            for (int b = 0; b < freeCount; ++b) {
                glm::dvec3 tNode;
                tNode.x = readFloat(&fin);
                tNode.y = readFloat(&fin);
                tNode.z = readFloat(&fin);
                TubeNodes[1].push_back(tNode);
                readNulls(&fin, 16);
            }
        }
        if (temp == "TUBE") {
            int type, index;
            readInt(&fin); // size
            readNulls(&fin, 16);
            tubeCount = readInt(&fin);
            for (int b = 0; b < tubeCount; ++b) {
                type = readInt(&fin);
                readInt(&fin); // segment
                index = readInt(&fin);
                if (type > 0 && type <= 2 && index < TubeNodes[type - 1].size()) {
                    lineList->push_back(TubeNodes[type - 1][index] - anchor);
                }
                readNulls(&fin, 4);
                type = readInt(&fin);
                readInt(&fin); // segment
                index = readInt(&fin);
                if (type > 0 && type <= 2 && index < TubeNodes[type - 1].size() && lineList->size() % 2) {
                    lineList->push_back(TubeNodes[type - 1][index] - anchor);
                } else if (type == 3 && lineList->size() % 2) {
                    lineList->pop_back();
                }
                readNulls(&fin, 20);
            }
        }
    }

    if (closeTrack && !bList->empty()) {
        bezier_t* nb = new bezier_t;
        *nb = *(bList->at(0));
        bList->push_back(nb);
    }

    if (!bList->empty()) {
        glm::dvec3 startPosFVD(0.0);
        glm::dquat rot(1.0, 0.0, 0.0, 0.0);
        double rollDiff = 0.0;
        bool snap = false;

        int secIdx = inTrack->trackData->getSectionNumber(activeSec);
        if (secIdx > 0) {
            section* prevSec = inTrack->trackData->lSections[secIdx - 1];
            if (!prevSec->lNodes.empty()) {
                startPosFVD = prevSec->lNodes.back().vPos;
                glm::dvec3 vFVD = prevSec->lNodes.back().vDir;
                glm::dvec3 vNL = glm::normalize(bList->at(0)->Kp2 - bList->at(0)->P1);
                if (glm::length(vNL) > 0.001 && glm::length(vFVD) > 0.001) {
                    rot = glm::rotation(vNL, glm::normalize(vFVD));
                }
                rollDiff = (prevSec->lNodes.back().fRoll * F_PI / 180.0) - bList->at(0)->roll;
                snap = true;
            }
        } else if (inTrack->trackData->anchorNode) {
            startPosFVD = inTrack->trackData->anchorNode->vPos;
            glm::dvec3 vFVD = inTrack->trackData->anchorNode->vDir;
            glm::dvec3 vNL = glm::normalize(bList->at(0)->Kp2 - bList->at(0)->P1);
            if (glm::length(vNL) > 0.001 && glm::length(vFVD) > 0.001) {
                rot = glm::rotation(vNL, glm::normalize(vFVD));
            }
            rollDiff = (inTrack->trackData->anchorNode->fRoll * F_PI / 180.0) - bList->at(0)->roll;
            snap = true;
        }

        if (snap) {
            for (auto nb : *bList) {
                nb->P1 = startPosFVD + rot * nb->P1;
                nb->Kp1 = startPosFVD + rot * nb->Kp1;
                nb->Kp2 = startPosFVD + rot * nb->Kp2;
                nb->roll += rollDiff;
            }
            for (auto& lp : *lineList) {
                lp = startPosFVD + rot * lp;
            }
        }
    }

    inTrack->trackData->updateTrack(activeSec, 0);
    fin.close();
    return true;
}

bool noLimitsImporter::importAsCsv() {
    std::ifstream file(fileName.c_str());
    if (!file.is_open()) {
        return false;
    }

    section* activeSec = inTrack->trackData->activeSection;
    std::vector<bezier_t*>* bList = &activeSec->bezList;
    std::vector<glm::dvec3>* lineList = &activeSec->supList;

    // Clear existing data
    for (auto b : *bList)
        delete b;
    bList->clear();
    lineList->clear();

    std::string line;
    int lineCount = 0;
    std::vector<glm::dvec3> dirs;

    while (std::getline(file, line)) {
        if (lineCount > 0) { // skip header
            std::stringstream ss(line);
            std::string item;
            std::vector<std::string> lineSplitted;

            char delimiter = '\t';
            if (line.find(',') != std::string::npos)
                delimiter = ',';

            while (std::getline(ss, item, delimiter)) {
                lineSplitted.push_back(item);
            }

            if (lineSplitted.size() >= 4) {
                double x = std::stod(lineSplitted[1]);
                double y = std::stod(lineSplitted[2]);
                double z = std::stod(lineSplitted[3]);

                double parsedRoll = 0.0;
                glm::dvec3 front(0.0);
                if (lineSplitted.size() >= 10) {
                    front = glm::dvec3(std::stod(lineSplitted[4]), std::stod(lineSplitted[5]), std::stod(lineSplitted[6]));
                    glm::dvec3 left(-std::stod(lineSplitted[7]), -std::stod(lineSplitted[8]), -std::stod(lineSplitted[9]));
                    mnode mathNode;
                    mathNode.vDir = front;
                    mathNode.vLat = left;
                    mathNode.updateRoll();
                    parsedRoll = mathNode.fRoll * F_PI / 180.0;
                }

                bezier_t* nb = new bezier_t;
                nb->P1.x = x;
                nb->P1.y = y;
                nb->P1.z = z;
                nb->roll = parsedRoll;
                nb->contRoll = true;
                nb->equalDist = true;
                nb->relRoll = false;
                nb->ptf = 0.0;
                nb->fvdRoll = parsedRoll;
                nb->fVel = 20.0; // Prevent physics stalling which causes memory explosion
                bList->push_back(nb);
                dirs.push_back(front);
            }
        }
        lineCount++;
    }

    if (bList->size() < 2) {
        file.close();
        return false;
    }

    // Calculate tangents automatically using exact CSV orientation and optimized Bezier arc approximation
    for (int b = 0; b < (int)bList->size() - 1; ++b) {
        glm::dvec3 P1 = bList->at(b)->P1;
        glm::dvec3 P2 = bList->at(b + 1)->P1;
        glm::dvec3 T1 = glm::normalize(dirs[b]);
        glm::dvec3 T2 = glm::normalize(dirs[b + 1]);

        double dist = glm::distance(P1, P2);
        if (dist > 0.001) {
            glm::dvec3 chord = (P2 - P1) / dist;
            double cosTheta1 = glm::clamp(glm::dot(T1, chord), -0.5, 1.0);
            double cosTheta2 = glm::clamp(glm::dot(T2, chord), -0.5, 1.0);

            double L1 = (2.0 * dist) / (3.0 * (1.0 + cosTheta1));
            double L2 = (2.0 * dist) / (3.0 * (1.0 + cosTheta2));

            bList->at(b)->Kp2 = P1 + T1 * L1;
            bList->at(b + 1)->Kp1 = P2 - T2 * L2;
        } else {
            bList->at(b)->Kp2 = P1;
            bList->at(b + 1)->Kp1 = P2;
        }
    }

    // Endpoints exterior bounds
    bList->at(0)->Kp1 = bList->at(0)->P1 - glm::normalize(dirs[0]) * (glm::distance(bList->at(0)->P1, bList->at(1)->P1) / 3.0);
    bList->back()->Kp2 = bList->back()->P1 + glm::normalize(dirs.back()) * (glm::distance(bList->at(bList->size() - 2)->P1, bList->back()->P1) / 3.0);

    // Snap to previous section
    glm::dvec3 startPosFVD(0.0);
    glm::dquat rot(1.0, 0.0, 0.0, 0.0);
    double rollDiff = 0.0;
    bool snap = false;

    int secIdx = inTrack->trackData->getSectionNumber(activeSec);
    glm::dvec3 anchorNL = bList->at(0)->P1;

    if (secIdx > 0) {
        section* prevSec = inTrack->trackData->lSections[secIdx - 1];
        if (!prevSec->lNodes.empty()) {
            startPosFVD = prevSec->lNodes.back().vPos;
            glm::dvec3 vFVD = prevSec->lNodes.back().vDir;
            glm::dvec3 vNL = glm::normalize(bList->at(0)->Kp2 - bList->at(0)->P1);
            if (glm::length(vNL) > 0.001 && glm::length(vFVD) > 0.001) {
                rot = glm::rotation(vNL, glm::normalize(vFVD));
            }
            rollDiff = (prevSec->lNodes.back().fRoll * F_PI / 180.0) - bList->at(0)->roll;
            snap = true;
        }
    } else if (inTrack->trackData->anchorNode) {
        startPosFVD = inTrack->trackData->anchorNode->vPos;
        glm::dvec3 vFVD = inTrack->trackData->anchorNode->vDir;
        glm::dvec3 vNL = glm::normalize(bList->at(0)->Kp2 - bList->at(0)->P1);
        if (glm::length(vNL) > 0.001 && glm::length(vFVD) > 0.001) {
            rot = glm::rotation(vNL, glm::normalize(vFVD));
        }
        rollDiff = (inTrack->trackData->anchorNode->fRoll * F_PI / 180.0) - bList->at(0)->roll;
        snap = true;
    }

    for (auto nb : *bList) {
        nb->P1 -= anchorNL;
        nb->Kp1 -= anchorNL;
        nb->Kp2 -= anchorNL;
        if (snap) {
            nb->P1 = startPosFVD + rot * nb->P1;
            nb->Kp1 = startPosFVD + rot * nb->Kp1;
            nb->Kp2 = startPosFVD + rot * nb->Kp2;
            nb->roll += rollDiff;
        }
    }

    inTrack->trackData->updateTrack(activeSec, 0);
    file.close();
    return true;
}
