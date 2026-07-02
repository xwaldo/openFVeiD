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

#include "trackmesh.h"
#include "mnode.h"
#include "section.h"
#include "track.h"
#include <iostream>
#include <algorithm>
#include "renderer/viewport.h"
#include "customstyle.h"
#include "dummies.h"
#include "logger.h"
#include <chrono>
#include <map>

trackMesh::trackMesh(track* parent) {
    isInit = false;
    customStyle = nullptr;
    primitiveCylinder = nullptr;
    primitiveBox = nullptr;
    currentMeshQuality = -1;

    for (int i = 0; i < 5; ++i) {
        TrackObject[i] = 0;
        TrackIndices[i] = 0;
        HeartObject[i] = 0;
        HeartIndices[i] = 0;
    }
    for (int i = 0; i < 7; ++i)
        TrackBuffer[i] = 0;
    for (int i = 0; i < 5; ++i)
        HeartBuffer[i] = 0;
    SplineBuffer = 0;

    glGenVertexArrays(5, TrackObject);
    glGenBuffers(7, TrackBuffer);
    glGenBuffers(5, TrackIndices);
    glGenVertexArrays(5, HeartObject);
    glGenBuffers(5, HeartBuffer);
    glGenBuffers(5, HeartIndices);
    glGenBuffers(1, &SplineBuffer);

    trackVertexSize = 0;
    numRails = 0;
    supportsSize = 0;
    heartlineSize = 0;
    trackData = parent;
}

trackMesh::~trackMesh() {
    if (customStyle) {
        delete customStyle;
        customStyle = nullptr;
    }
    for (auto& am : instancedAssets) {
        if (am.vao)
            glDeleteVertexArrays(1, &am.vao);
        if (am.vbo_base)
            glDeleteBuffers(1, &am.vbo_base);
        if (am.ebo_base)
            glDeleteBuffers(1, &am.ebo_base);
        if (am.vbo_instances)
            glDeleteBuffers(1, &am.vbo_instances);
    }
    instancedAssets.clear();

    for (auto& am : instancedExtrusions) {
        if (am.vao)
            glDeleteVertexArrays(1, &am.vao);
        if (am.vbo_base)
            glDeleteBuffers(1, &am.vbo_base);
        if (am.ebo_base)
            glDeleteBuffers(1, &am.ebo_base);
        if (am.vbo_instances)
            glDeleteBuffers(1, &am.vbo_instances);
    }
    instancedExtrusions.clear();

    if (primitiveCylinder)
        delete primitiveCylinder;
    if (primitiveBox)
        delete primitiveBox;

    if (TrackObject[0] != 0)
        glDeleteVertexArrays(5, TrackObject);
    if (HeartObject[0] != 0)
        glDeleteVertexArrays(5, HeartObject);
    if (TrackBuffer[0] != 0)
        glDeleteBuffers(7, TrackBuffer);
    if (HeartBuffer[0] != 0)
        glDeleteBuffers(5, HeartBuffer);
    if (SplineBuffer != 0)
        glDeleteBuffers(1, &SplineBuffer);
    if (TrackIndices[0] != 0)
        glDeleteBuffers(5, TrackIndices);
    if (HeartIndices[0] != 0)
        glDeleteBuffers(5, HeartIndices);
}

void trackMesh::init() {
    if (TrackObject[0] == 0) {
        glGenVertexArrays(5, TrackObject);
        glGenBuffers(7, TrackBuffer);
        glGenBuffers(5, TrackIndices);
        glGenVertexArrays(5, HeartObject);
        glGenBuffers(5, HeartBuffer);
        glGenBuffers(5, HeartIndices);
        glGenBuffers(1, &SplineBuffer);
    }

    isInit = true;
    buildMeshes(0);
}

void trackMesh::appendMeshNode(std::vector<meshnode_t>& list) {
    meshnode_t temp;
    temp.node = nextNode;
    temp.pos = glm::vec3(nextPos);
    list.push_back(temp);
}

void trackMesh::buildMeshes(int fromNode) {

    if (trackData->lSections.empty()) {
        rails.clear();
        crossties.clear();
        rendersupports.clear();
        supports.clear();
        heartline.clear();
        nodeList.clear();
        pipeIndices.clear();
        shadowIndices.clear();
        posList.clear();
        secList.clear();
        allPoints.clear();
        trackVertexSize = 0;
        supportsSize = 0;
        heartlineSize = 0;
        numRails = 0;

        for (auto& am : instancedExtrusions) {
            am.instances.clear();
        }
        for (auto& am : instancedAssets) {
            am.instances.clear();
        }

        updateVertexArrays(0);
        return;
    }

    if (loadedStyleFile != trackData->customStyleFile ||
        instancedExtrusions.size() != trackData->customExtrusions.size() ||
        instancedAssets.size() != trackData->customAssets.size()) {
        loadedStyleFile = trackData->customStyleFile;
        fromNode = 0;
    }

    generatePrimitives();

    auto start = std::chrono::high_resolution_clock::now();

    int numPoints = trackData->getNumPoints();
    float totalTrackLength = numPoints > 0 ? (float)trackData->getPoint(numPoints)->fTotalLength : 0.0f;

    // Performance Optimization: Cache flattened points incrementally
    if (fromNode == 0) {
        allPoints.clear();
        allPoints.reserve(numPoints + 1);
        allPoints.push_back(trackData->anchorNode);
    } else if (fromNode < (int)allPoints.size()) {
        allPoints.resize(fromNode);
    }

    if ((int)allPoints.size() <= numPoints) {
        int sectionIdx, nodeInSec;
        trackData->getSecNode(allPoints.size(), &nodeInSec, &sectionIdx);
        for (int s = sectionIdx; s < (int)trackData->lSections.size(); ++s) {
            auto sec = trackData->lSections[s];
            int startNode = (s == sectionIdx) ? nodeInSec : 1;
            for (int i = startNode; i < (int)sec->lNodes.size(); ++i) {
                allPoints.push_back(&sec->lNodes[i]);
            }
        }
    }

    float startDist = 0.0f;
    if (fromNode > 0 && fromNode <= numPoints) {
        startDist = std::max(0.0f, (float)allPoints[fromNode]->fTotalLength - 0.2f); // Small overlap for safety
    }

    int spliceSpline = std::floor(startDist / 0.1f);
    if (spliceSpline < (int)gpuSpline.size()) {
        gpuSpline.resize(spliceSpline);
    } else {
        spliceSpline = (int)gpuSpline.size();
    }

    // Performance Optimization: Use a linear cursor instead of binary search for every node
    int currentSplineIdx = (spliceSpline == 0) ? 0 : trackData->getIndexFromDist(spliceSpline * 0.1f);

    for (float d = spliceSpline * 0.1f; d <= totalTrackLength + 0.15f; d += 0.1f) {
        while (currentSplineIdx < numPoints && allPoints[currentSplineIdx + 1]->fTotalLength <= d) {
            currentSplineIdx++;
        }
        int idx0 = currentSplineIdx;
        int idx1 = std::min(idx0 + 1, numPoints);
        mnode* n0 = allPoints[idx0];
        mnode* n1 = allPoints[idx1];

        gpu_spline_node_t gn;
        if (n0 && n1) {
            double d0 = n0->fTotalLength;
            double d1 = n1->fTotalLength;
            double t = (d1 > d0) ? (double)((d - d0) / (d1 - d0)) : 0.0;
            t = glm::clamp(t, 0.0, 1.0);

            double heartOffset = (double)trackData->fHeart;
            glm::dvec3 pos = glm::mix(n0->vPos, n1->vPos, t);
            glm::dvec3 lat = glm::normalize(glm::mix(n0->vLatHeart(heartOffset), n1->vLatHeart(heartOffset), t));
            glm::dvec3 norm = glm::normalize(glm::mix(n0->vNorm, n1->vNorm, t));
            glm::dvec3 dir = glm::normalize(glm::mix(n0->vDirHeart(heartOffset), n1->vDirHeart(heartOffset), t));

            pos += heartOffset * norm;

            float sel = 0.0f;
            int sectionIdx = -1, nodeInSec = -1;

            // Find which section this node belongs to (more efficiently)
            int runningSum = 0;
            for (int s = 0; s < (int)trackData->lSections.size(); ++s) {
                int secNodes = (int)trackData->lSections[s]->lNodes.size() - 1;
                if (idx0 <= runningSum + secNodes) {
                    sectionIdx = s;
                    nodeInSec = idx0 - runningSum;
                    break;
                }
                runningSum += secNodes;
            }

            if (sectionIdx >= 0) {
                section* curSec = trackData->lSections[sectionIdx];
                if (trackData == gloParent->curTrack() && curSec == trackData->activeSection) {
                    sel = curSec->isInFunction(nodeInSec, gloParent->selectedFunc) ? 2.0f : 1.0f;
                }
            }

            gn.pos = glm::vec4(pos, (double)d);
            gn.lat = glm::vec4(lat, (double)sel);
            gn.norm = glm::vec4(norm, 0.0);
            gn.dir = glm::vec4(dir, 0.0);
        } else {
            gn.pos = glm::vec4(0.0);
            gn.lat = glm::vec4(1, 0, 0, 0);
            gn.norm = glm::vec4(0, 1, 0, 0);
            gn.dir = glm::vec4(0, 0, 1, 0);
        }
        gpuSpline.push_back(gn);
    }

    if (fromNode == 0) {
        rails.clear();
        crossties.clear();
        rendersupports.clear();
        supports.clear();
        heartline.clear();
        nodeList.clear();
        pipeIndices.clear();
        shadowIndices.clear();
        posList.clear();
        secList.clear();
        trackVertexSize = 0;
        supportsSize = 0;
        heartlineSize = 0;
    } else {
        if (fromNode < (int)heartline.size()) {
            heartline.resize(fromNode);
            nodeList.resize(fromNode);
            heartlineSize = fromNode;
        }
    }

    int startN = (fromNode == 0) ? 0 : (int)heartline.size();
    for (int n = startN; n <= numPoints; ++n) {
        mnode* node = trackData->getPoint(n);
        nextPos = node->vPos;
        nextNode = n;
        nodeList.push_back(n);
        appendMeshNode(heartline);
        heartlineSize++;
    }

    while (instancedExtrusions.size() < trackData->customExtrusions.size()) {
        instancedExtrusions.push_back({});
    }
    while (instancedExtrusions.size() > trackData->customExtrusions.size()) {
        auto& am = instancedExtrusions.back();
        if (am.vao)
            glDeleteVertexArrays(1, &am.vao);
        if (am.vbo_base)
            glDeleteBuffers(1, &am.vbo_base);
        if (am.ebo_base)
            glDeleteBuffers(1, &am.ebo_base);
        if (am.vbo_instances)
            glDeleteBuffers(1, &am.vbo_instances);
        instancedExtrusions.pop_back();
    }

    for (size_t extIdx = 0; extIdx < trackData->customExtrusions.size(); ++extIdx) {
        auto& ext = trackData->customExtrusions[extIdx];
        auto& am = instancedExtrusions[extIdx];

        CustomTrackStyle* targetModel = (ext.shape == track::ExtrusionShape::Cylindrical) ? primitiveCylinder : primitiveBox;
        if (am.sourceModel != targetModel) {
            if (am.vao != 0) {
                glDeleteVertexArrays(1, &am.vao);
                glDeleteBuffers(1, &am.vbo_base);
                glDeleteBuffers(1, &am.ebo_base);
                glDeleteBuffers(1, &am.vbo_instances);
                am.vao = 0;
            }
            am.sourceModel = targetModel;
            am.instances.clear(); // Full rebuild if model changed
        }
        am.indexCount = (int)am.sourceModel->indices.size();

        int spliceExt = std::floor(startDist / 1.0f);
        if (spliceExt < (int)am.instances.size()) {
            am.instances.resize(spliceExt);
        } else if (startDist == 0.0f) {
            am.instances.clear();
        }

        int currentExtIdx = (am.instances.empty()) ? 0 : trackData->getIndexFromDist(am.instances.size() * 1.0f);

        for (float d = am.instances.size() * 1.0f; d < totalTrackLength; d += 1.0f) {
            while (currentExtIdx < numPoints && allPoints[currentExtIdx + 1]->fTotalLength <= d) {
                currentExtIdx++;
            }
            int nodeIdx = currentExtIdx;

            float selectedState = 0.0f;
            int sectionIdx = -1, nodeInSec = -1;

            int runningSum = 0;
            for (int s = 0; s < (int)trackData->lSections.size(); ++s) {
                int secNodes = (int)trackData->lSections[s]->lNodes.size() - 1;
                if (nodeIdx <= runningSum + secNodes) {
                    sectionIdx = s;
                    nodeInSec = nodeIdx - runningSum;
                    break;
                }
                runningSum += secNodes;
            }

            if (sectionIdx >= 0) {
                section* curSection = trackData->lSections[sectionIdx];
                if (trackData == gloParent->curTrack() && curSection == trackData->activeSection) {
                    selectedState = curSection->isInFunction(nodeInSec, gloParent->selectedFunc) ? 2.0f : 1.0f;
                }
            }
            mnode* n = allPoints[nodeIdx];

            glm::mat4 mat(1.0f);
            mat[0] = glm::vec4(ext.size.x * 0.5f, 0, 0, 0);
            mat[1] = glm::vec4(0, (ext.shape == track::ExtrusionShape::Cylindrical ? ext.size.x : ext.size.y) * 0.5f, 0, 0);
            mat[3].x = d;
            mat[3].y = 0.0f;
            mat[3].z = 0.0f;

            track_asset_instance_t inst;
            inst.matrix = mat;
            inst.attributes1 = glm::vec4(selectedState, (float)n->fVel, (float)n->fRollSpeed, (float)n->forceNormal);
            inst.attributes2 = glm::vec4((float)n->forceLateral, (float)n->fFlexion(), (float)ext.offset.x, (float)ext.offset.y);
            inst.color = glm::vec3(0.6f);
            am.instances.push_back(inst);
        }
    }

    while (instancedAssets.size() < trackData->customAssets.size()) {
        instancedAssets.push_back({});
    }
    while (instancedAssets.size() > trackData->customAssets.size()) {
        auto& am = instancedAssets.back();
        if (am.vao)
            glDeleteVertexArrays(1, &am.vao);
        if (am.vbo_base)
            glDeleteBuffers(1, &am.vbo_base);
        if (am.ebo_base)
            glDeleteBuffers(1, &am.ebo_base);
        if (am.vbo_instances)
            glDeleteBuffers(1, &am.vbo_instances);
        instancedAssets.pop_back();
    }

    for (size_t assetIdx = 0; assetIdx < trackData->customAssets.size(); ++assetIdx) {
        auto& asset = trackData->customAssets[assetIdx];
        auto& assetMesh = instancedAssets[assetIdx];

        if (asset.filepath.empty()) {
            assetMesh.instances.clear();
            continue;
        }

        if (!asset.loadedModel) {
            asset.loadedModel = new CustomTrackStyle();
            if (!asset.loadedModel->load(asset.filepath)) {
                delete asset.loadedModel;
                asset.loadedModel = nullptr;
                continue;
            }
        }
        if (!asset.loadedModel->isValid)
            continue;

        if (assetMesh.sourceModel != asset.loadedModel) {
            if (assetMesh.vao != 0) {
                glDeleteVertexArrays(1, &assetMesh.vao);
                glDeleteBuffers(1, &assetMesh.vbo_base);
                glDeleteBuffers(1, &assetMesh.ebo_base);
                glDeleteBuffers(1, &assetMesh.vbo_instances);
                assetMesh.vao = 0;
            }
            assetMesh.sourceModel = asset.loadedModel;
            assetMesh.instances.clear();
        }
        assetMesh.indexCount = (int)asset.loadedModel->indices.size();

        float minZ = 999999.0f;
        for (const auto& v : asset.loadedModel->vertices)
            if (v.pos.z < minZ)
                minZ = v.pos.z;

        float start = std::max(0.0f, asset.startDist);
        float actualEndDist = (asset.endDist < 0.0f) ? totalTrackLength : asset.endDist;
        float end = std::min(totalTrackLength, actualEndDist);
        float interval = std::max(0.01f, asset.interval);

        float relStart = startDist - start;
        if (relStart > 0.0f) {
            int spliceAsset = std::floor(relStart / interval);
            if (spliceAsset < (int)assetMesh.instances.size()) {
                assetMesh.instances.resize(spliceAsset);
            }
        } else if (startDist == 0.0f) {
            assetMesh.instances.clear();
        }

        int currentAssetIdx = (assetMesh.instances.empty()) ? 0 : trackData->getIndexFromDist(start + assetMesh.instances.size() * interval);

        for (float d = start + assetMesh.instances.size() * interval; d <= end; d += interval) {
            while (currentAssetIdx < numPoints && allPoints[currentAssetIdx + 1]->fTotalLength <= d) {
                currentAssetIdx++;
            }
            int nodeIdx = currentAssetIdx;

            float selectedState = 0.0f;
            int sectionIdx = -1, nodeInSec = -1;

            int runningSum = 0;
            for (int s = 0; s < (int)trackData->lSections.size(); ++s) {
                int secNodes = (int)trackData->lSections[s]->lNodes.size() - 1;
                if (nodeIdx <= runningSum + secNodes) {
                    sectionIdx = s;
                    nodeInSec = nodeIdx - runningSum;
                    break;
                }
                runningSum += secNodes;
            }

            if (sectionIdx >= 0) {
                section* curSection = trackData->lSections[sectionIdx];
                if (trackData == gloParent->curTrack() && curSection == trackData->activeSection) {
                    selectedState = curSection->isInFunction(nodeInSec, gloParent->selectedFunc) ? 2.0f : 1.0f;
                }
            }
            mnode* n = allPoints[nodeIdx];

            glm::mat4 mat(1.0f);
            mat[3].x = d;
            mat[3].y = 0.0f; // Uniform used instead
            mat[3].z = minZ;
            mat[3].w = 1.0f;

            track_asset_instance_t inst;
            inst.matrix = mat;

            inst.attributes1 = glm::vec4(selectedState, (float)n->fVel, (float)n->fRollSpeed, (float)n->forceNormal);
            inst.attributes2 = glm::vec4((float)n->forceLateral, (float)n->fFlexion(), 0.0f, 0.0f);
            inst.color = asset.color;
            assetMesh.instances.push_back(inst);
        }
    }

    updateVertexArrays(fromNode);
}

void trackMesh::recolorTrack() {
    if (static_cast<int>(trackData->lSections.size()) == 0)
        return;

    for (int i = 0; i < (int)rails.size(); ++i) {
        int node = rails[i].node;
        int sectionIdx;
        trackData->getSecNode(node, &node, &sectionIdx);

        if (sectionIdx >= 0) {
            section* curSec = trackData->lSections[sectionIdx];
            rails[i].selected =
                (trackData == gloParent->curTrack() &&
                 curSec == trackData->activeSection)
                    ? (curSec->isInFunction(node, gloParent->selectedFunc) ? 2.f : 1.f)
                    : 0.f;
        } else {
            rails[i].selected = 0.0f;
        }
    }

    for (int i = 0; i < static_cast<int>(crossties.size()); ++i) {
        int node = crossties[i].node;
        int sectionIdx;
        trackData->getSecNode(node, &node, &sectionIdx);

        if (sectionIdx >= 0) {
            section* curSec = trackData->lSections[sectionIdx];
            crossties[i].selected =
                (trackData == gloParent->curTrack() &&
                 curSec == trackData->activeSection)
                    ? 1.f
                    : 0.f;
        } else {
            crossties[i].selected = 0.0f;
        }
    }
    updateVertexArrays(0);
}

void trackMesh::clearParametricStyles() {
    for (auto& am : instancedExtrusions) {
        if (am.vao != 0) {
            glDeleteVertexArrays(1, &am.vao);
            glDeleteBuffers(1, &am.vbo_base);
            glDeleteBuffers(1, &am.ebo_base);
            glDeleteBuffers(1, &am.vbo_instances);
            am.vao = 0;
        }
        am.instances.clear();
    }
    instancedExtrusions.clear();

    for (auto& am : instancedAssets) {
        if (am.vao != 0) {
            glDeleteVertexArrays(1, &am.vao);
            glDeleteBuffers(1, &am.vbo_base);
            glDeleteBuffers(1, &am.ebo_base);
            glDeleteBuffers(1, &am.vbo_instances);
            am.vao = 0;
        }
        am.instances.clear();
    }
    instancedAssets.clear();

    loadedStyleFile = "";
}

void trackMesh::updateVertexArrays(int fromNode) {
    // Calculate start distance for incremental updates (same logic as buildMeshes)
    float startDist = 0.0f;
    if (fromNode > 0) {
        int numPoints = trackData->getNumPoints();
        if (fromNode <= numPoints) {
            startDist = std::max(0.0f, (float)trackData->getPoint(fromNode)->fTotalLength - 0.2f);
        }
    }

    // 1. Spline SSBO
    if (!gpuSpline.empty()) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, SplineBuffer);
        size_t requiredSize = gpuSpline.size() * sizeof(gpu_spline_node_t);

        if (fromNode == 0 || requiredSize > gpuSplineCapacity) {
            // Orphan and Full Upload
            glBufferData(GL_SHADER_STORAGE_BUFFER, requiredSize, nullptr, GL_DYNAMIC_DRAW);
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, requiredSize, gpuSpline.data());
            gpuSplineCapacity = requiredSize;
        } else {
            // Partial Upload
            int spliceIndex = std::floor(startDist / 0.1f);
            if (spliceIndex < (int)gpuSpline.size()) {
                size_t offset = spliceIndex * sizeof(gpu_spline_node_t);
                size_t size = (gpuSpline.size() - spliceIndex) * sizeof(gpu_spline_node_t);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, gpuSpline.data() + spliceIndex);
            }
        }
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SplineBuffer);
    }

    // 2. Main Track Buffer (Rails/Crossties)
    glBindBuffer(GL_ARRAY_BUFFER, TrackBuffer[0]);
    size_t requiredTrackSize = rails.size() * sizeof(tracknode_t);
    if (fromNode == 0 || requiredTrackSize > trackBufferCapacity) {
        glBufferData(GL_ARRAY_BUFFER, requiredTrackSize, nullptr, GL_DYNAMIC_DRAW);
        if (requiredTrackSize > 0)
            glBufferSubData(GL_ARRAY_BUFFER, 0, requiredTrackSize, rails.data());
        trackBufferCapacity = requiredTrackSize;
    } else if (requiredTrackSize > 0) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, requiredTrackSize, rails.data());
    }

    glBindVertexArray(TrackObject[0]);
    // Attributes only need to be set once during initialization or if buffer layout changes
    // but for now we keep them to ensure correct state after possible GL state changes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), 0);
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(6 * sizeof(float)));
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(8 * sizeof(float)));
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(9 * sizeof(float)));
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(10 * sizeof(float)));
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(11 * sizeof(float)));
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(12 * sizeof(float)));
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(tracknode_t), (void*)(13 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glEnableVertexAttribArray(6);
    glEnableVertexAttribArray(7);
    glEnableVertexAttribArray(8);

    // 3. Instanced Assets
    for (auto& am : instancedAssets) {
        if (am.instances.empty() || am.sourceModel == nullptr)
            continue;
        if (am.vao == 0) {
            glGenVertexArrays(1, &am.vao);
            glGenBuffers(1, &am.vbo_base);
            glGenBuffers(1, &am.ebo_base);
            glGenBuffers(1, &am.vbo_instances);
            glBindVertexArray(am.vao);
            glBindBuffer(GL_ARRAY_BUFFER, am.vbo_base);
            glBufferData(GL_ARRAY_BUFFER, am.sourceModel->vertices.size() * sizeof(StyleVertex), am.sourceModel->vertices.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StyleVertex), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(StyleVertex), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(StyleVertex), (void*)(6 * sizeof(float)));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, am.ebo_base);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, am.sourceModel->indices.size() * sizeof(unsigned int), am.sourceModel->indices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, am.vbo_instances);
            glBufferData(GL_ARRAY_BUFFER, am.instances.size() * sizeof(track_asset_instance_t), am.instances.data(), GL_DYNAMIC_DRAW);
            for (int i = 0; i < 4; ++i) {
                glEnableVertexAttribArray(3 + i);
                glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(i * sizeof(glm::vec4)));
                glVertexAttribDivisor(3 + i, 1);
            }
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(sizeof(glm::mat4)));
            glVertexAttribDivisor(7, 1);
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(sizeof(glm::mat4) + sizeof(glm::vec4)));
            glVertexAttribDivisor(8, 1);
            glEnableVertexAttribArray(9);
            glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(sizeof(glm::mat4) + 2 * sizeof(glm::vec4)));
            glVertexAttribDivisor(9, 1);
            glBindVertexArray(0);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, am.vbo_instances);
            size_t requiredSize = am.instances.size() * sizeof(track_asset_instance_t);

            if (fromNode == 0 || requiredSize > instanceCapacities[am.vbo_instances]) {
                glBufferData(GL_ARRAY_BUFFER, requiredSize, nullptr, GL_DYNAMIC_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, requiredSize, am.instances.data());
                instanceCapacities[am.vbo_instances] = requiredSize;
            } else {
                int spliceIndex = 0;
                if (startDist > 0.0f) {
                    for (int i = (int)am.instances.size() - 1; i >= 0; --i) {
                        if (am.instances[i].matrix[3].x < startDist) {
                            spliceIndex = i + 1;
                            break;
                        }
                    }
                }

                if (spliceIndex < (int)am.instances.size()) {
                    size_t offset = spliceIndex * sizeof(track_asset_instance_t);
                    size_t size = (am.instances.size() - spliceIndex) * sizeof(track_asset_instance_t);
                    glBufferSubData(GL_ARRAY_BUFFER, offset, size, am.instances.data() + spliceIndex);
                }
            }
        }
    }

    // 4. Instanced Extrusions
    for (auto& am : instancedExtrusions) {
        if (am.instances.empty() || am.sourceModel == nullptr)
            continue;
        if (am.vao == 0) {
            glGenVertexArrays(1, &am.vao);
            glGenBuffers(1, &am.vbo_base);
            glGenBuffers(1, &am.ebo_base);
            glGenBuffers(1, &am.vbo_instances);
            glBindVertexArray(am.vao);
            glBindBuffer(GL_ARRAY_BUFFER, am.vbo_base);
            glBufferData(GL_ARRAY_BUFFER, am.sourceModel->vertices.size() * sizeof(StyleVertex), am.sourceModel->vertices.data(), GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StyleVertex), (void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(StyleVertex), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(StyleVertex), (void*)(6 * sizeof(float)));
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, am.ebo_base);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, am.sourceModel->indices.size() * sizeof(unsigned int), am.sourceModel->indices.data(), GL_STATIC_DRAW);

            glBindBuffer(GL_ARRAY_BUFFER, am.vbo_instances);
            glBufferData(GL_ARRAY_BUFFER, am.instances.size() * sizeof(track_asset_instance_t), am.instances.data(), GL_DYNAMIC_DRAW);
            for (int i = 0; i < 4; ++i) {
                glEnableVertexAttribArray(3 + i);
                glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(i * sizeof(glm::vec4)));
                glVertexAttribDivisor(3 + i, 1);
            }
            glEnableVertexAttribArray(7);
            glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(sizeof(glm::mat4)));
            glVertexAttribDivisor(7, 1);
            glEnableVertexAttribArray(8);
            glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(sizeof(glm::mat4) + sizeof(glm::vec4)));
            glVertexAttribDivisor(8, 1);
            glEnableVertexAttribArray(9);
            glVertexAttribPointer(9, 3, GL_FLOAT, GL_FALSE, sizeof(track_asset_instance_t), (void*)(sizeof(glm::mat4) + 2 * sizeof(glm::vec4)));
            glVertexAttribDivisor(9, 1);
            glBindVertexArray(0);
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, am.vbo_instances);
            size_t requiredSize = am.instances.size() * sizeof(track_asset_instance_t);

            if (fromNode == 0 || requiredSize > instanceCapacities[am.vbo_instances]) {
                glBufferData(GL_ARRAY_BUFFER, requiredSize, nullptr, GL_DYNAMIC_DRAW);
                glBufferSubData(GL_ARRAY_BUFFER, 0, requiredSize, am.instances.data());
                instanceCapacities[am.vbo_instances] = requiredSize;
            } else {
                int spliceIndex = 0;
                if (startDist > 0.0f) {
                    for (int i = (int)am.instances.size() - 1; i >= 0; --i) {
                        if (am.instances[i].matrix[3].x < startDist) {
                            spliceIndex = i + 1;
                            break;
                        }
                    }
                }

                if (spliceIndex < (int)am.instances.size()) {
                    size_t offset = spliceIndex * sizeof(track_asset_instance_t);
                    size_t size = (am.instances.size() - spliceIndex) * sizeof(track_asset_instance_t);
                    glBufferSubData(GL_ARRAY_BUFFER, offset, size, am.instances.data() + spliceIndex);
                }
            }
        }
    }

    glBindVertexArray(HeartObject[0]);
    glBindBuffer(GL_ARRAY_BUFFER, HeartBuffer[0]);
    glBufferData(GL_ARRAY_BUFFER, heartline.size() * sizeof(meshnode_t), nullptr, GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, heartline.size() * sizeof(meshnode_t), heartline.data());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(meshnode_t), 0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void trackMesh::generatePrimitives() {
    int quality = gloParent->mOptions->meshQuality;
    if (quality == currentMeshQuality && primitiveCylinder != nullptr)
        return;
    currentMeshQuality = quality;

    if (primitiveCylinder)
        delete primitiveCylinder;
    if (primitiveBox)
        delete primitiveBox;
    for (auto& am : instancedExtrusions) {
        if (am.vao != 0) {
            glDeleteVertexArrays(1, &am.vao);
            glDeleteBuffers(1, &am.vbo_base);
            glDeleteBuffers(1, &am.ebo_base);
            glDeleteBuffers(1, &am.vbo_instances);
            am.vao = 0;
            am.sourceModel = nullptr;
        }
    }

    int sides = 8, rings = 5;
    if (quality == 1) {
        sides = 12;
        rings = 10;
    } else if (quality == 2) {
        sides = 16;
        rings = 20;
    } else if (quality == 3) {
        sides = 24;
        rings = 30;
    } else if (quality == 4) {
        sides = 32;
        rings = 50;
    }

    primitiveCylinder = new CustomTrackStyle();
    primitiveCylinder->length = 1.0f;
    for (int r = 0; r <= rings; ++r) {
        float z = (float)r / (float)rings;
        for (int s = 0; s < sides; ++s) {
            float angle = (float)s * 2.0f * 3.14159265358979323846 / (float)sides;
            StyleVertex v;
            v.pos = glm::vec3(cos(angle), sin(angle), z);
            v.normal = glm::vec3(cos(angle), sin(angle), 0.0f);
            v.uv = glm::vec2((float)s / (float)sides, z);
            primitiveCylinder->vertices.push_back(v);
        }
    }
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sides; ++s) {
            int i0 = r * sides + s, i1 = r * sides + (s + 1) % sides, i2 = (r + 1) * sides + s, i3 = (r + 1) * sides + (s + 1) % sides;
            primitiveCylinder->indices.push_back(i0);
            primitiveCylinder->indices.push_back(i2);
            primitiveCylinder->indices.push_back(i1);
            primitiveCylinder->indices.push_back(i1);
            primitiveCylinder->indices.push_back(i2);
            primitiveCylinder->indices.push_back(i3);
        }
    }
    primitiveCylinder->isValid = true;

    primitiveBox = new CustomTrackStyle();
    primitiveBox->length = 1.0f;
    for (int r = 0; r <= rings; ++r) {
        float z = (float)r / (float)rings;
        glm::vec3 p[4] = {glm::vec3(-1, -1, z), glm::vec3(1, -1, z), glm::vec3(1, 1, z), glm::vec3(-1, 1, z)};
        glm::vec3 n[4] = {glm::vec3(0, -1, 0), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(-1, 0, 0)};

        for (int f = 0; f < 4; ++f) {
            StyleVertex v1, v2;
            v1.pos = p[f];
            v1.normal = n[f];
            v1.uv = glm::vec2(0.0f, z);
            v2.pos = p[(f + 1) % 4];
            v2.normal = n[f];
            v2.uv = glm::vec2(1.0f, z);
            primitiveBox->vertices.push_back(v1);
            primitiveBox->vertices.push_back(v2);
        }
    }
    for (int r = 0; r < rings; ++r) {
        for (int f = 0; f < 4; ++f) {
            int base = r * 8 + f * 2;
            int next = (r + 1) * 8 + f * 2;
            primitiveBox->indices.push_back(base);
            primitiveBox->indices.push_back(next);
            primitiveBox->indices.push_back(base + 1);
            primitiveBox->indices.push_back(base + 1);
            primitiveBox->indices.push_back(next);
            primitiveBox->indices.push_back(next + 1);
        }
    }
    primitiveBox->isValid = true;
}
