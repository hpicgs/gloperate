
#pragma once

#include <globjects/Buffer.h>
#include <globjects/VertexArray.h>

#include <gloperate/rendering/AbstractDrawable.h>
#include <gloperate/rendering/Drawable.h>

#include <gloperate/gloperate_api.h>


namespace globjects
{


class Shader;


}


namespace gloperate
{


/**
*  @brief
*    Screen aligned triangle rendering helper
*
*    This class can be used to render a screen aligned triangle.
*/
class GLOPERATE_API ScreenAlignedTriangle : public AbstractDrawable
{
public:
    /**
    *  @brief
    *    Constructor
    *
    *  @remarks
    *    Creates OpenGL objects, thus, a current context is required.
    */
    ScreenAlignedTriangle();

    /**
    *  @brief
    *    Destructor
    *
    *  @remarks
    *    Destroys OpenGL objects, thus, a current context is required.
    */
    virtual ~ScreenAlignedTriangle();

    /**
    *  @brief
    *    Get underlying drawable object
    *
    *  @return
    *    Pointer to the drawable object (always valid)
    */
    const Drawable * drawable() const;

    /**
    *  @brief
    *    Draw geometry
    *
    *  @remarks
    *    This function is the main draw entry point, used by the superclass
    *    AbstractDrawable.
    */
    virtual void draw() const override;


public:
    /**
    *  @brief
    *    Create new default vertex shader
    *
    *  @return
    *    Pointer to a newly created default vertex shader object
    */
    static globjects::Shader* createDefaultVertexShader();

    /**
    *  @brief
    *    Create new default fragment shader
    *
    *  @return
    *    Pointer to a newly created default fragment shader object
    */
    static globjects::Shader* createDefaultFragmentShader();


protected:
    std::unique_ptr<Drawable> m_drawable;


protected:
    static const std::string s_defaultVertexShaderSource;
    static const std::string s_defaultFragmentShaderSource;

};

} // namespace gloperate
