
#include <gloperate/pipeline/Stage.h>

#include <algorithm>

#include <cppassist/logging/logging.h>

#include <gloperate/pipeline/Pipeline.h>
#include <gloperate/pipeline/PipelineEvent.h>
#include <gloperate/pipeline/PipelineWatcher.h>
#include <gloperate/pipeline/AbstractInput.h>
#include <gloperate/pipeline/AbstractParameter.h>
#include <gloperate/pipeline/AbstractOutput.h>
#include <gloperate/pipeline/AbstractProxyOutput.h>


using namespace cppassist;


namespace gloperate
{


Stage::Stage(ViewerContext * viewerContext, const std::string & name, Pipeline * parent)
: cppexpose::Object(name, nullptr)
, m_viewerContext(viewerContext)
, m_alwaysProcess(false)
{
    if (parent) {
        parent->addStage(this, cppexpose::PropertyOwnership::None);
    }
}

Stage::~Stage()
{
    info() << m_name << " destroyed.";

    if (Pipeline * parent = parentPipeline())
    {
        parent->removeStage(this);
    }
}

bool Stage::isPipeline() const
{
    return false;
}

ViewerContext * Stage::viewerContext() const
{
    return m_viewerContext;
}

Pipeline * Stage::parentPipeline() const
{
    return static_cast<Pipeline *>(m_parent);
}

bool Stage::requires(const Stage * stage, bool recursive) const
{
    for (AbstractInputSlot * slot : m_inputs)
    {
        if (slot->isFeedback() || !slot->isConnected())
            continue;

        if (slot->source()->parentStage() == stage || (recursive && slot->source()->parentStage()->requires(stage)))
            return true;
    }

    return false;
}

void Stage::initContext(AbstractGLContext * context)
{
    onContextInit(context);
}

void Stage::deinitContext(AbstractGLContext * context)
{
    onContextDeinit(context);
}

void Stage::process(AbstractGLContext * context)
{
    onProcess(context);
}

bool Stage::needsProcessing() const
{
    if (m_alwaysProcess) {
        return true;
    }

    for (auto output : m_outputs) {
        if (output->isRequired() && !output->isValid()) {
            return true;
        }
    }

    for (auto output : m_proxyOutputs) {
        if (output->isRequired() && !output->isValid()) {
            return true;
        }
    }

    return false;
}

bool Stage::alwaysProcessed() const
{
    return m_alwaysProcess;
}

void Stage::setAlwaysProcessed(bool alwaysProcess)
{
    m_alwaysProcess = alwaysProcess;
}

void Stage::invalidateOutputs()
{
    for (auto output : m_outputs)
    {
        output->invalidate();
    }
}

const std::vector<AbstractInput *> & Stage::inputs() const
{
    return m_inputs;
}

const AbstractInput * Stage::input(const std::string & name) const
{
    return m_inputsMap.at(name);
}

void Stage::addInput(AbstractInput * input, cppexpose::PropertyOwnership ownership)
{
    // Check parameters
    if (!input) {
        return;
    }

    // Add input as property
    addProperty(input, ownership);

    // Add input
    m_inputs.push_back(input);

    if (input->name() != "") {
        m_inputsMap.insert(std::make_pair(input->name(), input));        
    }

    // Emit signal
    inputAdded(input);

    promotePipelineEvent(
        PipelineEvent(PipelineEvent::InputAdded, this, input)
    );
}

void Stage::removeInput(AbstractInput * input)
{
    // Check parameters
    if (!input)
    {
        return;
    }

    // Find input
    auto it = std::find(m_inputs.begin(), m_inputs.end(), input);
    if (it != m_inputs.end())
    {
        // Remove input
        m_inputs.erase(it);
        m_inputsMap.erase(input->name());

        // Emit signal
        inputRemoved(input);

        promotePipelineEvent(
            PipelineEvent(PipelineEvent::InputRemoved, this, input)
        );
    }

    // Remove property
    removeProperty(input);
}

const std::vector<AbstractParameter *> & Stage::parameters() const
{
    return m_parameters;
}

const AbstractParameter * Stage::parameter(const std::string & name) const
{
    return m_parametersMap.at(name);
}

void Stage::addParameter(AbstractParameter * parameter, cppexpose::PropertyOwnership ownership)
{
    // Check parameters
    if (!parameter) {
        return;
    }

    // Add parameter as property
    addProperty(parameter, ownership);

    // Add parameter
    m_parameters.push_back(parameter);
    if (parameter->name() != "") {
        m_parametersMap.insert(std::make_pair(parameter->name(), parameter));        
    }

    // Emit signal
    parameterAdded(parameter);

    promotePipelineEvent(
        PipelineEvent(PipelineEvent::ParameterAdded, this, parameter)
    );
}

void Stage::removeParameter(AbstractParameter * parameter)
{
    // Check parameters
    if (!parameter)
    {
        return;
    }

    // Find parameter
    auto it = std::find(m_parameters.begin(), m_parameters.end(), parameter);
    if (it != m_parameters.end())
    {
        // Remove parameter
        m_parameters.erase(it);
        m_parametersMap.erase(parameter->name());

        // Emit signal
        parameterRemoved(parameter);

        promotePipelineEvent(
            PipelineEvent(PipelineEvent::ParameterRemoved, this, parameter)
        );
    }

    // Remove property
    removeProperty(parameter);
}

const std::vector<AbstractOutput *> & Stage::outputs() const
{
    return m_outputs;
}

const AbstractOutput * Stage::output(const std::string & name) const
{
    return m_outputsMap.at(name);
}

void Stage::addOutput(AbstractOutput * output, cppexpose::PropertyOwnership ownership)
{
    // Check parameters
    if (!output) {
        return;
    }

    // Add output as property
    addProperty(output, ownership);

    // Add output
    m_outputs.push_back(output);
    if (output->name() != "") {
        m_outputsMap.insert(std::make_pair(output->name(), output));        
    }

    // Emit signal
    outputAdded(output);

    promotePipelineEvent(
        PipelineEvent(PipelineEvent::OutputAdded, this, output)
    );
}

void Stage::removeOutput(AbstractOutput * output)
{
    // Check parameters
    if (!output)
    {
        return;
    }

    // Find output
    auto it = std::find(m_outputs.begin(), m_outputs.end(), output);
    if (it != m_outputs.end())
    {
        // Remove output
        m_outputs.erase(it);
        m_outputsMap.erase(output->name());

        // Emit signal
        outputRemoved(output);

        promotePipelineEvent(
            PipelineEvent(PipelineEvent::OutputRemoved, this, output)
        );
    }

    // Remove property
    removeProperty(output);
}

const std::vector<AbstractProxyOutput *> & Stage::proxyOutputs() const
{
    return m_proxyOutputs;
}

const AbstractProxyOutput * Stage::proxyOutput(const std::string & name) const
{
    return m_proxyOutputsMap.at(name);
}

void Stage::addProxyOutput(AbstractProxyOutput * proxyOutput, cppexpose::PropertyOwnership ownership)
{
    // Check parameters
    if (!proxyOutput) {
        return;
    }

    // Add proxy output as property
    addProperty(proxyOutput, ownership);

    // Add proxy output
    m_proxyOutputs.push_back(proxyOutput);
    if (proxyOutput->name() != "") {
        m_proxyOutputsMap.insert(std::make_pair(proxyOutput->name(), proxyOutput));        
    }

    // Emit signal
    proxyOutputAdded(proxyOutput);

    promotePipelineEvent(
        PipelineEvent(PipelineEvent::ProxyOutputAdded, this, proxyOutput)
    );
}

void Stage::removeProxyOutput(AbstractProxyOutput * proxyOutput)
{
    // Check parameters
    if (!proxyOutput)
    {
        return;
    }

    // Find proxy output
    auto it = std::find(m_proxyOutputs.begin(), m_proxyOutputs.end(), proxyOutput);
    if (it != m_proxyOutputs.end())
    {
        // Remove proxy output
        m_proxyOutputs.erase(it);
        m_proxyOutputsMap.erase(proxyOutput->name());

        // Emit signal
        proxyOutputRemoved(proxyOutput);

        promotePipelineEvent(
            PipelineEvent(PipelineEvent::ProxyOutputRemoved, this, proxyOutput)
        );
    }

    // Remove property
    removeProperty(proxyOutput);
}

const std::vector<PipelineWatcher *> & Stage::watchers() const
{
    return m_watchers;
}

void Stage::addWatcher(PipelineWatcher * watcher)
{
    if (!watcher || std::find(m_watchers.begin(), m_watchers.end(), watcher) != m_watchers.end())
    {
        return;
    }

    m_watchers.push_back(watcher);
}

void Stage::removeWatcher(PipelineWatcher * watcher)
{
    if (!watcher)
    {
        return;
    }

    auto it = std::find(m_watchers.begin(), m_watchers.end(), watcher);
    if (it != m_watchers.end())
    {
        m_watchers.erase(it);
    }
}

void Stage::promotePipelineEvent(const PipelineEvent & event)
{
    // Inform the stage itself
    onPipelineEvent(event);

    // Inform registered pipeline watchers
    for (auto watcher : m_watchers)
    {
        watcher->onPipelineEvent(event);
    }

    // Inform parent pipeline
    if (Pipeline * parent = parentPipeline())
    {
        parent->onPipelineEvent(event);
    }
}

void Stage::onContextInit(AbstractGLContext *)
{
}

void Stage::onContextDeinit(AbstractGLContext *)
{
}

void Stage::onProcess(AbstractGLContext *)
{
}

void Stage::onInputValueChanged(AbstractSlot *)
{
    // Invalidate all outputs
    invalidateOutputs();
}

void Stage::onOutputRequiredChanged(AbstractSlot *)
{
    // By default, assume nothing is required
    bool required = false;

    // Check if any output is required
    for (auto output : m_outputs)
    {
        if (output->isRequired()) {
            required = true;
            break;
        }
    }

    // Update all inputs
    for (auto input : m_inputs)
    {
        input->setRequired(required);
    }
}

void Stage::onPipelineEvent(const PipelineEvent & event)
{
    // Ignore events from sub-stages
    if (event.stage() != this)
    {
        return;
    }

    // Value of a slot has changed
    if (event.type() == PipelineEvent::ValueChanged)
    {
        // Get slot
        AbstractSlot * slot = event.slot();

        // Check if this is either an input or parameter
        if (std::find(m_inputs.begin(), m_inputs.end(), slot) != m_inputs.end() ||
            std::find(m_parameters.begin(), m_parameters.end(), slot) != m_parameters.end())
        {
            onInputValueChanged(slot);
        }
    }

    // Required-state of a slot has changed
    if (event.type() == PipelineEvent::RequiredChanged)
    {
        // Get slot
        AbstractSlot * slot = event.slot();

        // Check if this is an output
        if (std::find(m_outputs.begin(), m_outputs.end(), slot) != m_outputs.end())
        {
            onOutputRequiredChanged(slot);
        }
    }
}


} // namespace gloperate
