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

#include "mnode.h"
#include "exportfuncs.h"
#include "lenassert.h"
#include <cmath>
#include <limits>

using namespace std;

mnode::mnode() {}

mnode::mnode(glm::dvec3 getPos, glm::dvec3 getDir, double getRoll, double getVel,
             double getNForce, double getLateral) {
    this->vPos = getPos;
    this->vDir = getDir;
    this->vDir = glm::normalize(vDir);
    this->fRoll = getRoll;
    this->fVel = getVel;
    this->fEnergy = 0.0;
    this->forceNormal = getNForce;
    this->forceLateral = getLateral;
    this->fDistFromLast = 0.0;
    this->fHeartDistFromLast = 0.0;
    this->fAngleFromLast = 0.0;
    this->fTrackAngleFromLast = 0.0;
    this->fDirFromLast = 0.0;
    this->fPitchFromLast = 0.0;
    this->fYawFromLast = 0.0;
    this->fTotalLength = 0.0;
    this->fTotalHeartLength = 0.0;
    this->fSmoothSpeed = 0.0;
    this->smoothNormal = 0.0;
    this->smoothLateral = 0.0;

    if (this->vDir.y == 1.0) {
        this->vLat =
            glm::dvec3(glm::angleAxis(TO_RAD(getRoll), glm::dvec3(0.0, -1.0, 0.0)) *
                       glm::dvec4(1.0, 0.0, 0.0, 0.0));
    } else {
        this->vLat = glm::dvec3(-this->vDir.z, 0.0, this->vDir.x);
    }

    this->vLat.y = tan(fRoll * F_PI / 180.0) * sqrt(this->vLat.x * this->vLat.x +
                                                    this->vLat.z * this->vLat.z);
    this->vLat = glm::normalize(vLat);
    this->fRollSpeed = 0.0;
}

void mnode::setRoll(double dRoll) {
    vLat = glm::normalize(glm::angleAxis(TO_RAD(-dRoll), vDir) * vLat);
    this->updateRoll();
    return;
}

void mnode::updateRoll() {
    this->updateNorm();
    fRoll = glm::atan(vLat.y, -vNorm.y) * 180.0 / F_PI;
    return;
}

void mnode::saveNode(ostream& file) {
    /*writeVec3(&file, glm::vec3(vPos));
    writeVec3(&file, glm::vec3(vDir));*/
    glm::vec3 vLatF = glm::vec3(vLat);
    writeVec3(&file, vLatF);
    float fVelF = (float)fVel;
    writeBytes(&file, (const char*)&fVelF, sizeof(float));
}

void mnode::changePitch(double dAngle, bool inverted) {
    glm::dvec3 rotateAround;
    lenAssert(fabs(vLat.y) < 1.9);
    rotateAround = glm::normalize(glm::cross(glm::dvec3(0.0, vNorm.y, 0.0), vDir));
    if (inverted) {
        rotateAround *= -1.0;
    }
    vDir = glm::normalize(glm::angleAxis(TO_RAD(dAngle), rotateAround) * vDir);
    vLat = glm::normalize(glm::angleAxis(TO_RAD(dAngle), rotateAround) * vLat);
    updateNorm();
}

void mnode::changeYaw(double dAngle) {
    vDir = glm::normalize(
        glm::angleAxis(TO_RAD(dAngle), glm::dvec3(0.0, 1.0, 0.0)) * vDir);
    vLat = glm::normalize(
        glm::angleAxis(TO_RAD(dAngle), glm::dvec3(0.0, 1.0, 0.0)) * vLat);
    this->updateNorm();
}

glm::dvec3 mnode::vLatHeart(double fHeart) {
    double estimated;
    double estDistFromLast = 0.7 * fHeartDistFromLast + 0.3 * fDistFromLast;

    if (fAngleFromLast < 0.001) {
        estimated = fHeartDistFromLast;
    } else {
        estimated = fVel / F_HZ;
    }
    double fRollSpeedPerMeter =
        estDistFromLast > 0.0 ? (fRollSpeed + fSmoothSpeed) / F_HZ / estimated
                              : 0.0;
    return glm::normalize(glm::normalize(vLat) -
                          glm::normalize(vDir) * (double)(fRollSpeedPerMeter *
                                                          F_PI * fHeart / 180.0));
}

glm::dvec3 mnode::vDirHeart(double fHeart) {
    double estimated;
    if (fAngleFromLast < 0.001) {
        estimated = fHeartDistFromLast;
    } else {
        estimated = fVel / F_HZ;
    }
    double fRollSpeedPerMeter =
        fHeartDistFromLast > 0.0 ? (fRollSpeed + fSmoothSpeed) / F_HZ / estimated
                                 : 0.0;
    if (fRollSpeedPerMeter != fRollSpeedPerMeter)
        fRollSpeedPerMeter = 0.0;
    return glm::normalize(
        vDir + vLat * (double)(fRollSpeedPerMeter * F_PI * fHeart / 180.0));
}

void mnode::exportNode(std::vector<bezier_t*>& bezList, mnode* last, mnode*,
                       mnode* anchor, double fHeart, double fRollThresh) {
#define SCALING 3.0

    bezList.push_back(new bezier_t);

    double realDist = this->fTotalLength - last->fTotalLength;

    double fThreshold =
        this->fTotalLength - last->fTotalLength; // glm::distance(last->fPosHeart(fHeart),
                                                 // this->fPosHeart(fHeart));
    double temp = glm::length(glm::dvec3(anchor->vDir.x, 0.0, anchor->vDir.z));
    glm::dmat3 anchorBase =
        glm::dmat3(-anchor->vDir.z / temp, 0.0, -anchor->vDir.x / temp, 0.0, 1.0,
                   0.0, anchor->vDir.x / temp, 0.0, -anchor->vDir.z / temp);

    double radius = this->fVel / (this->fTrackAngleFromLast * F_PI / 180.0) / F_HZ;
    double angle =
        (F_HZ * this->fTrackAngleFromLast * F_PI / 180.0) / this->fVel * realDist;

    fThreshold = 0.998 * 2.0 / 3.0 * radius * tan(angle / 2.0);

    if (fThreshold != fThreshold) {
        fThreshold = 0.998 * 2.0 / 3.0 * realDist / 2.0;
    }

    bezList.back()->P1 =
        anchorBase * (this->vPosHeart(fHeart) - anchor->vPosHeart(fHeart));

    if (bezList.size() > 1) {
        bezList.back()->Kp1 = bezList[bezList.size() - 2]->P1 +
                              anchorBase * (fThreshold * last->vDirHeart(fHeart));
    } else {
        bezList.back()->Kp1 =
            anchorBase * (last->vPosHeart(fHeart) - anchor->vPosHeart(fHeart) +
                          fThreshold * last->vDirHeart(fHeart));
    }
    bezList.back()->Kp2 =
        bezList.back()->P1 - anchorBase * (fThreshold * this->vDirHeart(fHeart));

    temp = 0.0;

    if (fabs(this->vDirHeart(fHeart).y) < fRollThresh) {
        temp = glm::atan(this->vLatHeart(fHeart).y, -this->vNorm.y);
    } else {
        glm::dvec3 rotateAxis =
            glm::cross(last->vDirHeart(fHeart), this->vDirHeart(fHeart));
        glm::dvec3 rotated =
            glm::dvec3(glm::rotate(glm::angle(last->vDirHeart(fHeart),
                                              this->vDirHeart(fHeart)),
                                   rotateAxis) *
                       glm::dvec4(last->vLatHeart(fHeart), 0.0));
        temp = glm::angle(rotated, this->vLatHeart(fHeart)) * F_PI / 180.0;
        if (temp != temp) {
            temp = 0.0;
        }
    }

    bezList.back()->roll = temp;

    if (fabs(this->vDirHeart(fHeart).y) < fRollThresh) {
        bezList.back()->relRoll = false;
    } else {
        bezList.back()->relRoll = true;
    }
}

void mnode::calcSmoothForces() {
    glm::dvec3 forceVec;
    double temp = cos(fabs(getPitch()) * F_PI / 180.0);
    if (fabs(fAngleFromLast) < std::numeric_limits<double>::epsilon()) {
        forceVec = glm::dvec3(0.0, 1.0, 0.0);
    } else {
        double normalDAngle = F_PI / 180.0 *
                              (-fPitchFromLast * cos(fRoll * F_PI / 180.0) -
                               temp * fYawFromLast * sin(fRoll * F_PI / 180.0));
        double lateralDAngle = F_PI / 180.0 *
                               (fPitchFromLast * sin(fRoll * F_PI / 180.0) -
                                temp * fYawFromLast * cos(fRoll * F_PI / 180.0));
        forceVec = glm::dvec3(0.0, 1.0, 0.0) +
                   lateralDAngle * fVel * F_HZ / F_G * vLat +
                   normalDAngle * fHeartDistFromLast * F_HZ * F_HZ / F_G * vNorm;
    }
    smoothNormal = -glm::dot(forceVec, glm::normalize(vNorm)) - forceNormal;
    smoothLateral = -glm::dot(forceVec, glm::normalize(vLat)) - forceLateral;
}
