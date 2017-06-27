
#include <gloperate/stages/base/ClearStage.h>

#include <glbinding/gl/gl.h>

#include <globjects/Framebuffer.h>
#include <globjects/FramebufferAttachment.h>
#include <globjects/AttachedRenderbuffer.h>
#include <globjects/AttachedTexture.h>

#include <gloperate/rendering/ColorRenderTarget.h>
#include <gloperate/rendering/DepthRenderTarget.h>
#include <gloperate/rendering/StencilRenderTarget.h>


namespace gloperate
{


CPPEXPOSE_COMPONENT(ClearStage, gloperate::Stage)


ClearStage::ClearStage(Environment * environment, const std::string & name)
: Stage(environment, "ClearStage", name)
, clear("clear",  this, true)
, renderInterface(this)
{
    inputAdded.connect( [this] (AbstractSlot * connectedInput) {
        auto colorValueInput = dynamic_cast<Input<glm::vec4> *>(connectedInput);
        auto depthValueInput = dynamic_cast<Input<float> *>(connectedInput);
        auto stencilValueInput = dynamic_cast<Input<int> *>(connectedInput);
        auto depthStencilValueInput = dynamic_cast<Input<std::pair<float, int>> *>(connectedInput);

        if (colorValueInput)
        {
            m_colorValueInputs.push_back(colorValueInput);
        }

        if (depthValueInput)
        {
            m_depthValueInputs.push_back(depthValueInput);
        }

        if (stencilValueInput)
        {
            m_stencilValueInputs.push_back(stencilValueInput);
        }

        if (depthStencilValueInput)
        {
            m_depthStencilValueInputs.push_back(depthStencilValueInput);
        }
    });
}

ClearStage::~ClearStage()
{
}

void ClearStage::onContextInit(AbstractGLContext *)
{
    m_defaultFBO = globjects::Framebuffer::defaultFBO();
    m_fbo = cppassist::make_unique<globjects::Framebuffer>();
}

void ClearStage::onContextDeinit(AbstractGLContext *)
{
    m_defaultFBO = nullptr;
    m_fbo = nullptr;
}

void ClearStage::onProcess()
{
    if (*clear)
    {
        if (renderInterface.viewport->z >= 0.0 || renderInterface.viewport->w >= 0.0) {
            // Setup OpenGL state
            gl::glScissor(renderInterface.viewport->x, renderInterface.viewport->y, renderInterface.viewport->z, renderInterface.viewport->w);
            gl::glEnable(gl::GL_SCISSOR_TEST);
        }
        else
        {
            // Clear full render targets if viewport has invalid size
            gl::glDisable(gl::GL_SCISSOR_TEST);
        }

        size_t colorAttachmentIndex        = 0;
        size_t depthAttachmentIndex        = 0;
        size_t stencilAttachmentIndex      = 0;
        size_t depthStencilAttachmentIndex = 0;
        std::set<AbstractRenderTarget *> clearedDepthStencilTargets;

        renderInterface.pairwiseRenderTargetsDo([this, & colorAttachmentIndex](Input<ColorRenderTarget *> * input, Output<ColorRenderTarget *> * output) {
            if (!output->isRequired() || !**input)
            {
                return;
            }

            if (m_colorValueInputs.size() <= colorAttachmentIndex)
            {
                return;
            }

            auto fbo = renderInterface.configureFBO(colorAttachmentIndex, **input, m_fbo.get(), m_defaultFBO.get());

            const auto attachmentBuffer = (**input)->attachmentBuffer();
            const auto attachmentDrawBuffer = (**input)->attachmentDrawBuffer(colorAttachmentIndex);
            const auto clearColor = **m_colorValueInputs.at(colorAttachmentIndex);

            fbo->clearBuffer(attachmentBuffer, attachmentDrawBuffer, clearColor);

            ++colorAttachmentIndex;
        });

        renderInterface.pairwiseRenderTargetsDo([this, & depthAttachmentIndex, & depthStencilAttachmentIndex, & clearedDepthStencilTargets](Input<DepthRenderTarget *> * input, Output<DepthRenderTarget *> * output) {
            if (!output->isRequired() || !**input)
            {
                return;
            }

            if ((**input)->attachmentType() == AttachmentType::Depth)
            {
                if (m_depthValueInputs.size() <= depthAttachmentIndex)
                {
                    return;
                }

                auto fbo = renderInterface.configureFBO(depthAttachmentIndex, **input, m_fbo.get(), m_defaultFBO.get());

                fbo->clearBuffer(gl::GL_DEPTH, (**input)->attachmentDrawBuffer(depthAttachmentIndex), **m_depthValueInputs.at(depthAttachmentIndex));

                ++depthAttachmentIndex;
            }
            else if ((**input)->attachmentType() == AttachmentType::DepthStencil)
            {
                if (m_depthStencilValueInputs.size() <= depthStencilAttachmentIndex)
                {
                    return;
                }

                auto fbo = renderInterface.configureFBO(depthStencilAttachmentIndex, **input, m_fbo.get(), m_defaultFBO.get());

                fbo->clearBuffer(gl::GL_DEPTH_STENCIL, (**m_depthStencilValueInputs.at(depthStencilAttachmentIndex)).first, (**m_depthStencilValueInputs.at(depthStencilAttachmentIndex)).second, (**input)->attachmentDrawBuffer(depthStencilAttachmentIndex));

                ++depthStencilAttachmentIndex;
                clearedDepthStencilTargets.insert(**input);
            }
        });

        renderInterface.pairwiseRenderTargetsDo([this, & stencilAttachmentIndex, & depthStencilAttachmentIndex, & clearedDepthStencilTargets](Input<StencilRenderTarget *> * input, Output<StencilRenderTarget *> * output) {
            if (!output->isRequired() || !**input)
            {
                return;
            }

            if ((**input)->attachmentType() == AttachmentType::Stencil)
            {
                if (m_stencilValueInputs.size() <= stencilAttachmentIndex)
                {
                    return;
                }

                auto fbo = renderInterface.configureFBO(stencilAttachmentIndex, **input, m_fbo.get(), m_defaultFBO.get());

                fbo->clearBuffer(gl::GL_STENCIL, (**input)->attachmentDrawBuffer(stencilAttachmentIndex), **m_stencilValueInputs.at(stencilAttachmentIndex));

                ++stencilAttachmentIndex;
            }
            else if ((**input)->attachmentType() == AttachmentType::DepthStencil)
            {
                if (std::find(clearedDepthStencilTargets.begin(), clearedDepthStencilTargets.end(), **input) != clearedDepthStencilTargets.end())
                {
                    return;
                }

                if (m_depthStencilValueInputs.size() <= depthStencilAttachmentIndex)
                {
                    return;
                }

                auto fbo = renderInterface.configureFBO(depthStencilAttachmentIndex, **input, m_fbo.get(), m_defaultFBO.get());

                fbo->clearBuffer(gl::GL_DEPTH_STENCIL, (**m_depthStencilValueInputs.at(depthStencilAttachmentIndex)).first, (**m_depthStencilValueInputs.at(depthStencilAttachmentIndex)).second, (**input)->attachmentDrawBuffer(depthStencilAttachmentIndex));

                ++depthStencilAttachmentIndex;
            }

        });

        // Reset OpenGL state
        gl::glDisable(gl::GL_SCISSOR_TEST);
    }

    // Update outputs
    renderInterface.updateRenderTargetOutputs();
}

} // namespace gloperate
