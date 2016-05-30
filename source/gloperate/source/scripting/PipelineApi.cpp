
#include <gloperate/scripting/PipelineApi.h>

#include <vector>
#include <sstream>
#include <string>
#include <iostream>

#include <gloperate/viewer/ViewerContext.h>
#include <gloperate/viewer/RenderSurface.h>
#include <gloperate/pipeline/Stage.h>
#include <gloperate/pipeline/AbstractInputSlot.h>
#include <gloperate/pipeline/AbstractData.h>


namespace gloperate
{


PipelineApi::PipelineApi(ViewerContext * viewerContext)
: cppexpose::Object("pipeline")
, m_viewerContext(viewerContext)
{
    // Register functions
    addFunction("getName",    this, &PipelineApi::getName);
    addFunction("getStages",  this, &PipelineApi::getStages);
    addFunction("getInputs",  this, &PipelineApi::getInputs);
    addFunction("getOutputs", this, &PipelineApi::getOutputs);
    addFunction("getValue",   this, &PipelineApi::getValue);
}

PipelineApi::~PipelineApi()
{
}

std::string PipelineApi::getName(const std::string & name)
{
    Stage * stage = getStage(name);

    return stage->name();
}

cppexpose::Variant PipelineApi::getStages(const std::string & name)
{
    Stage * stage = getStage(name);

    cppexpose::Variant lst = cppexpose::Variant::array();

    if (!stage->isPipeline()) {
        return lst;
    }

    Pipeline * pipeline = static_cast<Pipeline*>(stage);

    for (auto * subStage : pipeline->stages())
    {
        lst.asArray()->push_back(subStage->name());
    }
    return lst;
}

cppexpose::Variant PipelineApi::getInputs(const std::string & name)
{
    Stage * stage = getStage(name);

    cppexpose::Variant lst = cppexpose::Variant::array();

    for (auto * input : stage->inputs())
    {
        lst.asArray()->push_back(input->name());
    }

    for (auto * parameter : stage->parameters())
    {
        lst.asArray()->push_back(parameter->name());
    }

    return lst;
}

cppexpose::Variant PipelineApi::getOutputs(const std::string & name)
{
    Stage * stage = getStage(name);

    cppexpose::Variant lst = cppexpose::Variant::array();

    for (auto * output : stage->outputs())
    {
        lst.asArray()->push_back(output->name());
    }

    for (auto * proxyOutput : stage->proxyOutputs())
    {
        lst.asArray()->push_back(proxyOutput->name());
    }

    return lst;
}

std::string PipelineApi::getValue(const std::string & path)
{
    cppexpose::AbstractProperty * property = getProperty(path);
    if (property) {
        return property->toString();
    } else {
        return "";
    }
}

Stage * PipelineApi::getStage(const std::string & name)
{
    // Get render surface
    if (m_viewerContext->surfaces().size() == 0) {
        return nullptr;
    }

    RenderSurface * surface = static_cast<RenderSurface *>(m_viewerContext->surfaces()[0]);
    if (!surface) {
        return nullptr;
    }

    // Split name by '.'
    std::vector<std::string> names;
    std::istringstream ss(name);
    std::string subname;
    while (std::getline(ss, subname, '.')) {
        names.push_back(subname);
    }

    // Begin with root pipeline
    Stage * stage = surface->rootPipeline();
    for (std::string subname : names)
    {
        // Get sub-stage
        if (stage->isPipeline()) {
            stage = static_cast<Pipeline *>(stage)->stage(subname);
        } else {
            stage = nullptr;
        }

        // Abort if child was not found
        if (!stage) {
            return nullptr;
        }
    }

    return stage;
}

cppexpose::AbstractProperty * PipelineApi::getProperty(const std::string & name)
{
    // Get render surface
    if (m_viewerContext->surfaces().size() == 0) {
        return nullptr;
    }

    RenderSurface * surface = static_cast<RenderSurface *>(m_viewerContext->surfaces()[0]);
    if (!surface) {
        return nullptr;
    }

    // Split name by '.'
    std::vector<std::string> names;
    std::istringstream ss(name);
    std::string subname;
    while (std::getline(ss, subname, '.')) {
        names.push_back(subname);
    }

    // Begin with root pipeline
    Stage * stage = surface->rootPipeline();
    for (size_t i=0; i<names.size(); i++)
    {
        // Get next token
        std::string subname = names[i];

        // If this is the last token, return property from current stage
        if (i == names.size() - 1)
        {
            return stage->property(subname);
        }

        // Get sub-stage
        if (stage->isPipeline()) {
            stage = static_cast<Pipeline *>(stage)->stage(subname);
        } else {
            stage = nullptr;
        }

        // Abort if child was not found
        if (!stage) {
            return nullptr;
        }
    }

    return nullptr;
}


} // namespace gloperate
