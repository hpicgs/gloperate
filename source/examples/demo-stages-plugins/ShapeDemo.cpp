
#include "ShapeDemo.h"

#include <glbinding/gl/enum.h>

#include <gloperate/gloperate.h>

#include <gloperate/stages/base/TextureLoadStage.h>
#include <gloperate/stages/base/BasicFramebufferStage.h>
#include <gloperate/stages/base/TextureRenderTargetStage.h>
#include <gloperate/stages/base/ProgramStage.h>
#include <gloperate/stages/base/RenderPassStage.h>
#include <gloperate/stages/base/RasterizationStage.h>
#include <gloperate/stages/base/ClearStage.h>
#include <gloperate/stages/base/ShapeStage.h>
#include <gloperate/stages/base/TimerStage.h>
#include <gloperate/stages/base/TransformStage.h>
#include <gloperate/stages/base/FloatSelectionStage.h>
#include <gloperate/stages/navigation/TrackballStage.h>
#include <gloperate/rendering/Shape.h>
#include <gloperate/rendering/Quad.h>
#include <gloperate/rendering/ColorRenderTarget.h>
#include <gloperate/stages/base/TextureFromRenderTargetExtractionStage.h>
#include <gloperate/stages/base/ViewportScaleStage.h>


CPPEXPOSE_COMPONENT(ShapeDemo, gloperate::Stage)


using namespace cppexpose;
using namespace gloperate;


ShapeDemo::ShapeDemo(Environment * environment, const std::string & name)
: Pipeline(environment, "ShapeDemo", name)
, canvasInterface(this)
, shape("shape", this, ShapeType::Box)
, texture("texture", this)
, angle("angle", this, 0.0f)
, rotate("rotate", this, false)
, color("color", this, Color(255, 255, 255, 255))
, m_timer(cppassist::make_unique<TimerStage>(environment, "Timer"))
, m_floatSelection(cppassist::make_unique<FloatSelectionStage>(environment, "FloatSelection"))
, m_trackball(cppassist::make_unique<TrackballStage>(environment, "Trackball"))
, m_viewportScale(cppassist::make_unique<ViewportScaleStage>(environment, "ViewportScale"))
, m_shape(cppassist::make_unique<ShapeStage>(environment, "Shape"))
, m_texture(cppassist::make_unique<TextureLoadStage>(environment, "Texture"))
, m_framebuffer(cppassist::make_unique<BasicFramebufferStage>(environment, "Framebuffer"))
, m_clear(cppassist::make_unique<ClearStage>(environment, "Clear"))
, m_shapeTransform(cppassist::make_unique<TransformStage>(environment, "ShapeTransform"))
, m_shapeProgram(cppassist::make_unique<ProgramStage>(environment, "ShapeProgram"))
, m_shapeRenderPass(cppassist::make_unique<RenderPassStage>(environment, "ShapeRenderPass"))
, m_shapeRasterization(cppassist::make_unique<RasterizationStage>(environment, "ShapeRasterization"))
, m_colorizeProgram(cppassist::make_unique<ProgramStage>(environment, "ColorizeProgram"))
, m_textureExtractionStage(cppassist::make_unique<TextureFromRenderTargetExtractionStage>(environment, "TextureExtraction"))
, m_colorizeRenderPass(cppassist::make_unique<RenderPassStage>(environment, "ColorizeRenderPass"))
, m_colorizeRasterization(cppassist::make_unique<RasterizationStage>(environment, "ColorizeRasterization"))
{
    // Get data path
    const auto dataPath = gloperate::dataPath();

    // Setup parameters
    texture = dataPath + "/gloperate/textures/gloperate-logo.glraw";

    angle.setOption("minimumValue", 0.0f);
    angle.setOption("maximumValue", 2.0f * glm::pi<float>());
    angle.setOption("updateOnDrag", true);

    rotate.valueChanged.connect(this, &ShapeDemo::onRotateChanged);
    m_timer->virtualTime.valueChanged.connect([this](const float & angle) {
        // Set angle to current timer value
        this->angle = angle;
    });

    // Timer stage
    addStage(m_timer.get());
    m_timer->interval = 2.0f * glm::pi<float>();
    m_timer->interval.setOption("maximumValue", 2.0f * glm::pi<float>());

    addStage(m_floatSelection.get());
    m_floatSelection->createInput("timerValue") << m_timer->virtualTime;
    m_floatSelection->createInput("angle") << angle;
    m_floatSelection->index = 1u;

    addStage(m_viewportScale.get());
    m_viewportScale->viewport << canvasInterface.viewport;
    //m_viewportScale->scaleFactor = 1.0;
    m_viewportScale->scaleFactor << *createInput<float>("ViewportScaleFactor", 1.0f);

    // Trackball stage
    addStage(m_trackball.get());
    m_trackball->viewport << m_viewportScale->scaledViewport;

    // Shape stage
    addStage(m_shape.get());
    m_shape->shapeType << this->shape;
    m_shape->width     = 1.0f;
    m_shape->height    = 1.0f;
    m_shape->depth     = 1.0f;
    m_shape->radius    = 1.0f;
    m_shape->texCoords = true;

    // Texture loader stage
    addStage(m_texture.get());
    m_texture->filename << this->texture;

    // Framebuffer stage
    addStage(m_framebuffer.get());
    m_framebuffer->viewport << m_viewportScale->scaledViewport;

    // Transform stage for shape
    addStage(m_shapeTransform.get());
    m_shapeTransform->rotationAngle << m_floatSelection->value;

    // Program stage for shape
    addStage(m_shapeProgram.get());
    *m_shapeProgram->createInput<cppfs::FilePath>("vertexShader")   = dataPath + "/gloperate/shaders/geometry/geometry.vert";
    *m_shapeProgram->createInput<cppfs::FilePath>("fragmentShader") = dataPath + "/gloperate/shaders/geometry/geometry.frag";

    // Render pass stage for shape
    addStage(m_shapeRenderPass.get());
    m_shapeRenderPass->culling = false;
    m_shapeRenderPass->drawable << m_shape->drawable;
    m_shapeRenderPass->program << m_shapeProgram->program;
    m_shapeRenderPass->camera << m_trackball->camera;
    m_shapeRenderPass->modelMatrix << m_shapeTransform->modelMatrix;
    m_shapeRenderPass->createInput("color") << this->color;
    m_shapeRenderPass->createInput("tex0") << m_texture->texture;

    // Clear stage
    addStage(m_clear.get());
    m_clear->createInput("ColorAttachment") << m_framebuffer->colorBuffer;
    m_clear->createInput("DepthAttachment") << m_framebuffer->depthBuffer;
    m_clear->createInput("ColorValue") << canvasInterface.backgroundColor;
    m_clear->createInput("DepthValue") = 1.0f;
    m_clear->renderInterface.viewport << m_viewportScale->scaledViewport;

    // Invalidation of Clear Stage if new rendering should take place
    m_clear->createInput("renderPass") << m_shapeRenderPass->renderPass;

    // Rasterization stage for shape
    addStage(m_shapeRasterization.get());
    m_shapeRasterization->createInput("ColorAttachment") << *m_clear->createOutput<gloperate::ColorRenderTarget *>("ColorAttachmentOut");
    m_shapeRasterization->createInput("DepthAttachment") << *m_clear->createOutput<gloperate::DepthRenderTarget *>("DepthAttachmentOut");
    m_shapeRasterization->renderInterface.viewport << m_viewportScale->scaledViewport;
    m_shapeRasterization->drawable << m_shapeRenderPass->renderPass;

    // Colorize program stage
    addStage(m_colorizeProgram.get());
    *m_colorizeProgram->createInput<cppfs::FilePath>("shader1") = dataPath + "/gloperate/shaders/geometry/screenaligned.vert";
    *m_colorizeProgram->createInput<cppfs::FilePath>("shader2") = dataPath + "/gloperate/shaders/demos/colorize.frag";

    auto shapeColorOutput = m_shapeRasterization->createOutput<gloperate::ColorRenderTarget *>("ColorAttachmentOut");

    /* Hack Start */
    shapeColorOutput->valueInvalidated.onFire([=]() {
        m_clear->renderInterface.colorRenderTargetOutput(0)->invalidate();
        // m_clear->renderInterface.depthRenderTargetOutput(0)->invalidate();
    });
    /* Hack End */

    addStage(m_textureExtractionStage.get());
    m_textureExtractionStage->colorRenderTarget << *shapeColorOutput;

    // Colorize render pass stage
    addStage(m_colorizeRenderPass.get());
    // m_colorizeRenderPass->drawable is set in onContextInit()
    m_colorizeRenderPass->program << m_colorizeProgram->program;
    m_colorizeRenderPass->culling = false;
    m_colorizeRenderPass->depthTest = false;
    m_colorizeRenderPass->createInput("color") << this->color;
    m_colorizeRenderPass->createInput("source") << m_textureExtractionStage->texture;

    // Colorize rasterization stage
    addStage(m_colorizeRasterization.get());
    m_colorizeRasterization->createInput("ColorAttachment") << *createInput<gloperate::ColorRenderTarget *>("Color");
    m_colorizeRasterization->renderInterface.viewport << m_viewportScale->scaledViewport;
    m_colorizeRasterization->drawable << m_colorizeRenderPass->renderPass;

    // Outputs
    *createOutput<gloperate::ColorRenderTarget *>("ColorOut") << *m_colorizeRasterization->createOutput<gloperate::ColorRenderTarget *>("ColorOut");
    //*createOutput<gloperate::ColorRenderTarget *>("ColorOut") << *shapeColorOutput;
    *createOutput<glm::vec4>("ViewportOut") << m_viewportScale->scaledViewport;

    // Start rotation
    rotate = true;
}

ShapeDemo::~ShapeDemo()
{
}

void ShapeDemo::onContextInit(AbstractGLContext * context)
{
    Pipeline::onContextInit(context);

    m_quad = cppassist::make_unique<Quad>(2.0f);

    m_colorizeRenderPass->drawable = m_quad.get();
}

void ShapeDemo::onContextDeinit(AbstractGLContext * context)
{
    m_quad = nullptr;

    Pipeline::onContextDeinit(context);
}

void ShapeDemo::onRotateChanged(const bool & rotate)
{
    // Switch rotation on
    if (rotate)
    {
        // Set timer to current rotation value
        m_timer->virtualTime = *angle;

        // Switch angle to timer and resume timer
        m_timer->timeDelta << canvasInterface.timeDelta;
        m_floatSelection->index = 0u;
    }

    // Switch rotation off
    else
    {
        // Stop time
        m_timer->timeDelta.disconnect();

        // Switch timer to angle
        m_floatSelection->index = 1u;
    }
}
