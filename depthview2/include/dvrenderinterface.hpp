#pragma once

#include "dvenums.hpp"

class QOpenGLFramebufferObject;
class QOpenGLBuffer;
class QOpenGLExtraFunctions;
class QSGTexture;

/* Vertex attrib locations. */
constexpr unsigned int vertex = 0;
constexpr unsigned int uv     = 1;

/* This class is virtual so it can be used in plugins without linking shenanigans, is implemented in DVWindow. */
class DVRenderInterface {
public:
    /* Get the FBO that QML is rendered to. */
    virtual const QOpenGLFramebufferObject& getInterfaceFramebuffer() = 0;

    /* Get the OpenGL textures for each eye. */
    virtual unsigned int getInterfaceLeftEyeTexture() = 0;
    virtual unsigned int getInterfaceRightEyeTexture() = 0;

    /* Returns the texture handle the current image / video, and sets left & right to where on the texture each eye is. */
    virtual QSGTexture* getCurrentTexture(QRectF& left, QRectF& right) = 0;

    /* Get whether the current image is surround. */
    virtual bool isSurround() = 0;

    /* Draw the default sphere (for surround images). */
    virtual void renderStandardSphere() = 0;

    /* Draw the default fullscreen quad. */
    virtual void renderStandardQuad() = 0;

    /* Get the OpenGL functions. */
    virtual QOpenGLExtraFunctions* getOpenGLFunctions() = 0;

    /* Set up the renderer exactly as all the built-in modes have it set up.
     * The left and right image textures will be bound to TEXTURE0 and TEXTURE1, respectively,
     * the viewport is set to the window size, and surround images will be rendered under the UI. */
    virtual void doStandardSetup() = 0;
};
