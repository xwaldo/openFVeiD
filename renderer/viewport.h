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

#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>

class ShaderProgram;
class GlTexture;
class GlFramebuffer;
class trackHandler;
class mnode;

struct Mesh {
    GLuint object;
    GLuint buffer;
    GLuint ebo = 0;
    int count = 0;
};

struct StlMesh {
    Mesh mesh;
    int vertexCount;
    std::string path;
    glm::vec3 color;
    bool visible;
    bool showWireframe;
};

class Viewport {
public:
    Viewport();
    ~Viewport();

    enum class ViewMode {
        Perspective,
        Top,
        Side,
        Front
    };

    void initialize(int width, int height);
    void resize(int width, int height);
    void render(const std::vector<trackHandler*>& trackList);
    void update(float deltaTime);

    // Camera Controls
    void moveCamera(glm::vec3 delta);
    void rotateCamera(float yaw, float pitch);
    void panCamera(float dx, float dy);
    void zoomCamera(float delta);
    void lookAtOrigin();
    void resetView();
    void focusOnSection(int sectionIdx);

    // Settings
    void setMistColor(glm::vec3 color);
    void setFOV(float f) {
        if (fov != f) {
            fov = f;
            sceneDirty = true;
        }
    }
    void setShadowMode(int mode);
    void setLightDirection(float pitch, float yaw);
    void setMSAASamples(int samples);
    void setGroundTextureSize(float size) {
        if (grdTexSize != size) {
            grdTexSize = size;
            sceneDirty = true;
        }
    }
    bool loadGroundTexture(const std::string& path);
    bool addStlMesh(const std::string& path);
    void removeStlMesh(int index);

    void markSceneDirty() {
        sceneDirty = true;
    }
    void setCaptured(bool captured) {
        isCaptured = captured;
    }
    bool getCaptured() const {
        return isCaptured;
    }

    // View Modes
    ViewMode getViewMode() const {
        return viewMode;
    }
    void setViewMode(ViewMode mode);

    // POV Mode
    bool getPOVMode() const {
        return povMode;
    }
    int getPOVPos() const {
        return povPos;
    }
    void setPOVMode(bool enabled) {
        if (povMode != enabled) {
            povMode = enabled;
            sceneDirty = true;
        }
    }
    void setPOVPos(int pos) {
        if (povPos != pos) {
            povPos = pos;
            sceneDirty = true;
        }
    }
    void setActiveTrack(trackHandler* track) {
        if (activeTrack != track) {
            activeTrack = track;
            sceneDirty = true;
        }
    }
    void movePOVCamera(float deltaZ, float deltaTime);
    void adjustPOVHeight(float delta) {
        if (delta != 0.0f) {
            povHeightOffset += delta;
            sceneDirty = true;
        }
    }
    mnode* getPOVNode() const {
        return povNode;
    }

    void setShowPOVMarker3D(bool show) {
        if (showPOVMarker3D != show) {
            showPOVMarker3D = show;
            sceneDirty = true;
        }
    }

    // Orthographic Mode
    bool getOrthoMode() const {
        return viewMode != ViewMode::Perspective;
    }
    void setOrthoMode(bool enabled) {
        setViewMode(enabled ? ViewMode::Top : ViewMode::Perspective);
    }
    float getOrthoScale() const {
        return orthoScale;
    }
    void setOrthoScale(float scale) {
        if (orthoScale != scale) {
            orthoScale = scale;
            sceneDirty = true;
        }
    }

    // Shader Mode
    void setTrackShaderMode(int mode) {
        if (curTrackShader != mode) {
            curTrackShader = mode;
            sceneDirty = true;
        }
    }
    int getTrackShaderMode() const {
        return curTrackShader;
    }

    void captureScreenshot(int multiplier, const std::string& path, const std::vector<trackHandler*>& trackList);

    GLuint getOutputTexture();
    std::vector<StlMesh> stlMeshes;

private:
    void initShaders();
    void initTextures();
    void initFloorMesh();

    void buildMatrices(float offset = 0.0f);
    void drawSky();
    void drawFloor();
    void drawOrthoGrid();
    enum class RenderPass {
        NormalMap,
        Main,
        PlanarShadow
    };

    void drawTrack(trackHandler* track, RenderPass pass);
    void drawStls(RenderPass pass);
    void drawMarkers(RenderPass pass);

    // Framebuffers
    GLuint msaaFBO;
    GLuint msaaColorTex;
    GLuint msaaDepthRB;
    GlFramebuffer* normalMapFb;
    GlFramebuffer* occlusionFb;
    GlFramebuffer* finalOutputFb;

    // Shaders
    ShaderProgram* skyShader;
    ShaderProgram* floorShader;
    ShaderProgram* trackShader;
    ShaderProgram* trackInstancedShader;
    ShaderProgram* normalMapShader;
    ShaderProgram* occlusionShader;
    ShaderProgram* simpleShadowShader;
    ShaderProgram* stlShader;

    GlTexture* skyTexture;
    GlTexture* floorTexture;
    GlTexture* rasterTexture;

    // Meshes
    Mesh skyMesh;
    Mesh floorMesh;
    Mesh markerMesh;
    Mesh orthoGridMesh;

    // Camera State
    glm::vec3 freeFlyPos;
    glm::vec3 freeFlyDir;
    glm::vec3 freeFlySide;
    glm::vec3 cameraPos;
    glm::vec3 lightDir;
    glm::mat4 shadowMatrix;

    glm::mat4 ProjectionMatrix;
    glm::mat4 ModelMatrix;
    glm::mat4 ProjectionModelMatrix;
    glm::mat4 lightSpaceMatrix;

    float fov;
    glm::vec3 mistColor;
    float grdTexSize = 440.0f;
    int viewPortWidth, viewPortHeight;

    bool povMode;
    int povPos;
    mnode* povNode;
    float povHeightOffset = 0.0f;
    bool showPOVMarker3D = true;
    trackHandler* activeTrack = nullptr;

    ViewMode viewMode;
    float orthoScale;

    int shadowMode;
    int curTrackShader;
    int msaaSamples;

    bool sceneDirty = true;
    bool isCaptured = false;
    double lastShadowUpdateTime = 0.0;
    bool shadowIsHighQuality = false;
};

#endif // VIEWPORT_H
