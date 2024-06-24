
#include <gloperate/stages/base/RenderPassStage.h>

#include <cppassist/logging/logging.h>

#include <glbinding/gl/enum.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <globjects/Program.h>
#include <globjects/State.h>
#include <globjects/Texture.h>
#include <globjects/TextureHandle.h>

#include <gloperate/gloperate.h>
#include <gloperate/rendering/Color.h>
#include <gloperate/rendering/RenderPass.h>
#include <gloperate/rendering/Drawable.h>
#include <gloperate/rendering/Camera.h>


namespace gloperate
{


CPPEXPOSE_COMPONENT(RenderPassStage, gloperate::Stage)


RenderPassStage::RenderPassStage(Environment * environment, const std::string & name)
: Stage(environment, "RenderPassStage", name)
, drawable("drawable", this)
, program("program", this)
, camera("camera", this)
, modelMatrix("modelMatrix", this, glm::mat4(1.0))
, depthTest("depthTest", this, true)
, depthMask("depthMask", this, true)
, depthFunc("depthFunc", this, gl::GL_LEQUAL)
, culling("culling", this, true)
, cullFace("cullFace", this, gl::GL_BACK)
, frontFace("frontFace", this, gl::GL_CCW)
, blending("blending", this, false)
, renderPass("renderPass", this)
{
    // Invalidate output when input slots have been added or removed
    m_inputAddedConnection = inputAdded.connect([this] (gloperate::AbstractSlot *)
    {
        renderPass.invalidate();
    });

    m_inputRemovedConnection = inputRemoved.connect([this] (gloperate::AbstractSlot *)
    {
        renderPass.invalidate();
    });
}

RenderPassStage::~RenderPassStage()
{
}

void RenderPassStage::onContextInit(AbstractGLContext *)
{
    // Create render pass
    m_renderPass = cppassist::make_unique<gloperate::RenderPass>();
    renderPass.setValue(m_renderPass.get());

    // Create OpenGL state set
    m_beforeState = cppassist::make_unique<globjects::State>(globjects::State::DeferredMode);
    m_renderPass->setStateBefore(m_beforeState.get());

    renderPass.invalidate();
}

void RenderPassStage::onContextDeinit(AbstractGLContext *)
{
    // Create OpenGL state set
    m_beforeState = nullptr;

    // Create render pass
    m_renderPass = nullptr;

    renderPass.setValue(nullptr);
}

void RenderPassStage::onProcess()
{
    // Setup render pass geometry and program
    m_renderPass->setGeometry(*drawable);
    m_renderPass->setProgram(*program);

    // Check if a camera or a model matrix is set
    Camera * camera = (this->camera.isValid() && *this->camera) ? *this->camera : nullptr;
    bool hasModelMatrix = (this->modelMatrix.isValid());
    const glm::mat4 modelMatrix = hasModelMatrix ? *this->modelMatrix : glm::mat4(1.0f);

    if (camera)
    {
        (*program)->setUniform<glm::mat4>("viewProjectionMatrix",         camera->viewProjectionMatrix());
        (*program)->setUniform<glm::mat4>("viewProjectionInvertedMatrix", camera->viewProjectionInvertedMatrix());
        (*program)->setUniform<glm::mat4>("viewMatrix",                   camera->viewMatrix());
        (*program)->setUniform<glm::mat4>("viewInvertexMatrix",           camera->viewInvertedMatrix());
        (*program)->setUniform<glm::mat4>("projectionMatrix",             camera->projectionMatrix());
        (*program)->setUniform<glm::mat4>("projectionInvertedMatrix",     camera->projectionInvertedMatrix());
        (*program)->setUniform<glm::mat3>("normalMatrix",                 camera->normalMatrix());
    }

    if (hasModelMatrix)
    {
        (*program)->setUniform<glm::mat4>("modelMatrix", modelMatrix);
    }

    if (camera && hasModelMatrix)
    {
        (*program)->setUniform<glm::mat4>("modelViewProjectionMatrix",         camera->viewProjectionMatrix() * modelMatrix);
        (*program)->setUniform<glm::mat4>("modelViewProjectionInvertedMatrix", glm::inverse(camera->viewProjectionMatrix() * modelMatrix));
        (*program)->setUniform<glm::mat4>("modelViewMatrix",                   camera->viewMatrix() * modelMatrix);
        (*program)->setUniform<glm::mat4>("modelViewInvertexMatrix",           glm::inverse(camera->viewMatrix() * modelMatrix));
        (*program)->setUniform<glm::mat3>("modelNormalMatrix",                 glm::inverseTranspose(glm::mat3(camera->viewMatrix() * modelMatrix)));
    }

    // Update OpenGL states
    if (*this->depthTest) m_renderPass->stateBefore()->enable (gl::GL_DEPTH_TEST);
    else                  m_renderPass->stateBefore()->disable(gl::GL_DEPTH_TEST);
    m_renderPass->stateBefore()->depthMask(*this->depthMask ? gl::GL_TRUE : gl::GL_FALSE);
    m_renderPass->stateBefore()->depthFunc(*this->depthFunc);

    if (*this->culling) m_renderPass->stateBefore()->enable (gl::GL_CULL_FACE);
    else                m_renderPass->stateBefore()->disable(gl::GL_CULL_FACE);
    m_renderPass->stateBefore()->cullFace(*this->cullFace);
    m_renderPass->stateBefore()->frontFace(*this->frontFace);

    if (*this->blending) m_renderPass->stateBefore()->enable (gl::GL_BLEND);
    else                 m_renderPass->stateBefore()->disable(gl::GL_BLEND);

    // Update uniforms from dynamic inputs
    unsigned int textureIndex = 0;
    unsigned int shaderStorageBufferIndex = 0;

    for (auto input : inputs())
    {
        // Only conside dynamic inputs here
        if (!input->isDynamic())
            continue;

        // Texture
        if (auto * typedInput = dynamic_cast<Input<globjects::Texture *> *>(input))
        {
            // Get texture
            globjects::Texture * texture = typedInput->value();

            if (!texture)
                continue;

            // Attach texture
            (*program)->setUniform<int>(typedInput->name(), textureIndex);
            m_renderPass->setTexture(textureIndex, texture);

            if (texture->target() == gl::GL_TEXTURE_CUBE_MAP)
            {
                m_renderPass->stateBefore()->enable(gl::GL_TEXTURE_CUBE_MAP_SEAMLESS);
            }

            ++textureIndex;
        }

        // Shader storage buffer
        else if (auto * typedInput = dynamic_cast<Input<globjects::Buffer *> *>(input))
        {
            // Get buffer
            globjects::Buffer * buffer = typedInput->value();

            if (!buffer)
                continue;

            // Attach shader storage buffer
            m_renderPass->setShaderStorageBuffer(shaderStorageBufferIndex, buffer);
            ++shaderStorageBufferIndex;
        }

        // Color
        else if (auto * typedInput = dynamic_cast<Input<Color> *>(input))
        {
            // Get color
            const Color & color = **typedInput;

            // Set color uniform
            (*program)->setUniform<glm::vec4>(input->name(), color.toVec4());
        }

        // Basic uniform
        else
        {
            setUniformValue(*program, input);
        }
    }

    // Update outputs
    renderPass.setValue(m_renderPass.get());
}

void RenderPassStage::setUniformValue(globjects::Program * program, AbstractSlot * input)
{
    if (auto * typedInput = dynamic_cast<Input<float> *>(input)) {
        program->setUniform<float>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<int> *>(input)) {
        program->setUniform<int>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<unsigned int> *>(input)) {
        program->setUniform<unsigned int>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<bool> *>(input)) {
        program->setUniform<bool>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::vec2> *>(input)) {
        program->setUniform<glm::vec2>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::vec3> *>(input)) {
        program->setUniform<glm::vec3>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::vec4> *>(input)) {
        program->setUniform<glm::vec4>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::ivec2> *>(input)) {
        program->setUniform<glm::ivec2>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::ivec3> *>(input)) {
        program->setUniform<glm::ivec3>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::ivec4> *>(input)) {
        program->setUniform<glm::ivec4>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::uvec2> *>(input)) {
        program->setUniform<glm::uvec2>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::uvec3> *>(input)) {
        program->setUniform<glm::uvec3>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::uvec4> *>(input)) {
        program->setUniform<glm::uvec4>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat2> *>(input)) {
        program->setUniform<glm::mat2>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat3> *>(input)) {
        program->setUniform<glm::mat3>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat4> *>(input)) {
        program->setUniform<glm::mat4>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat2x3> *>(input)) {
        program->setUniform<glm::mat2x3>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat3x2> *>(input)) {
        program->setUniform<glm::mat3x2>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat2x4> *>(input)) {
        program->setUniform<glm::mat2x4>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat4x2> *>(input)) {
        program->setUniform<glm::mat4x2>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat3x4> *>(input)) {
        program->setUniform<glm::mat3x4>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<glm::mat4x3> *>(input)) {
        program->setUniform<glm::mat4x3>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<gl::GLuint64> *>(input)) {
        program->setUniform<gl::GLuint64>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<globjects::TextureHandle> *>(input)) {
        program->setUniform<globjects::TextureHandle>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<float>> *>(input)) {
        program->setUniform<std::vector<float>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<int>> *>(input)) {
        program->setUniform<std::vector<int>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<unsigned int>> *>(input)) {
        program->setUniform<std::vector<unsigned int>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<bool>> *>(input)) {
        program->setUniform<std::vector<bool>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::vec2>> *>(input)) {
        program->setUniform<std::vector<glm::vec2>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::vec3>> *>(input)) {
        program->setUniform<std::vector<glm::vec3>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::vec4>> *>(input)) {
        program->setUniform<std::vector<glm::vec4>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::ivec2>> *>(input)) {
        program->setUniform<std::vector<glm::ivec2>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::ivec3>> *>(input)) {
        program->setUniform<std::vector<glm::ivec3>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::ivec4>> *>(input)) {
        program->setUniform<std::vector<glm::ivec4>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::uvec2>> *>(input)) {
        program->setUniform<std::vector<glm::uvec2>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::uvec3>> *>(input)) {
        program->setUniform<std::vector<glm::uvec3>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::uvec4>> *>(input)) {
        program->setUniform<std::vector<glm::uvec4>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat2>> *>(input)) {
        program->setUniform<std::vector<glm::mat2>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat3>> *>(input)) {
        program->setUniform<std::vector<glm::mat3>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat4>> *>(input)) {
        program->setUniform<std::vector<glm::mat4>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat2x3>> *>(input)) {
        program->setUniform<std::vector<glm::mat2x3>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat3x2>> *>(input)) {
        program->setUniform<std::vector<glm::mat3x2>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat2x4>> *>(input)) {
        program->setUniform<std::vector<glm::mat2x4>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat4x2>> *>(input)) {
        program->setUniform<std::vector<glm::mat4x2>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat3x4>> *>(input)) {
        program->setUniform<std::vector<glm::mat3x4>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<glm::mat4x3>> *>(input)) {
        program->setUniform<std::vector<glm::mat4x3>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<gl::GLuint64>> *>(input)) {
        program->setUniform<std::vector<gl::GLuint64>>(input->name(), typedInput->value());
    } else if (auto * typedInput = dynamic_cast<Input<std::vector<globjects::TextureHandle>> *>(input)) {
        program->setUniform<std::vector<globjects::TextureHandle>>(input->name(), typedInput->value());
    }
}


} // namespace gloperate
