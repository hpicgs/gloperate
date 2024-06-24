
#pragma once


#include <algorithm>
#include <typeinfo>

#include <gloperate/pipeline/Input.h>
#include <gloperate/pipeline/Output.h>


namespace gloperate
{


template <typename T>
Input<T> * Stage::CreateConnectedInputProxy::operator<<(Slot<T> & source)
{
    ++m_createdCount;
    return m_stage->createConnectedInput(m_name, source);
}

template <typename T>
Input<T> * Stage::CreateConnectedInputProxy::operator=(const T & value)
{
    ++m_createdCount;
    return m_stage->createInput(m_name, value);
}


template <typename T>
std::vector<Input<T> *> Stage::inputs() const
{
    auto result = std::vector<Input<T> *>{};

    // We do not reserve a heuristically derived number of elements as we assume
    // the vector growing strategy would handle most cases efficiently
    // result.reserve(inputs().size() / 2);

    for (auto input : inputs())
    {
        auto * typedInput = dynamic_cast<Input<T> *>(input);

        // [TODO] This workaround is currently needed on macos for Input<cppassist::FilePath>,
        //        because the dynamic cast fails.
        if (!typedInput && input->type() == typeid(T))
        {
            typedInput = static_cast<Input<T> *>(input);
        }

        if (typedInput)
        {
            result.push_back(typedInput);
        }
    }

    return result;
}

template <typename T>
Input<T> * Stage::createInput(const std::string & name, const T & defaultValue)
{
    auto input = cppassist::make_unique<Input<T>>(name, defaultValue);
    auto inputPtr = input.get();

    this->addInput(std::move(input));

    return inputPtr;
}

template <typename T>
Input<T> * Stage::createConnectedInput(const std::string & name, Slot<T> & source)
{
    auto input = createInput<T>(name);
    input->connect(&source);

    return input;
}

template <typename T>
std::vector<Output<T> *> Stage::outputs() const
{
    auto result = std::vector<Output<T> *>{};

    // We do not reserve a heuristically derived number of elements as we assume
    // the vector growing strategy would handle most cases efficiently
    // result.reserve(outputs().size() / 2);

    for (auto output : outputs())
    {
        auto * typedOutput = dynamic_cast<Output<T> *>(output);

        // [TODO] This workaround is currently needed on macos for Input<cppassist::FilePath>,
        //        because the dynamic cast fails.
        if (!typedOutput && output->type() == typeid(T))
        {
            typedOutput = static_cast<Output<T> *>(input);
        }

        if (typedOutput)
        {
            result.push_back(typedOutput);
        }
    }

    return result;
}

template <typename T>
Output<T> * Stage::createOutput(const std::string & name, const T & defaultValue)
{
    auto output = cppassist::make_unique<Output<T>>(name, defaultValue);
    auto outputPtr = output.get();

    this->addOutput(std::move(output));

    return outputPtr;
}

template <typename T>
Slot<T> * Stage::createSlot(const std::string & slotType, const std::string & name, const T & defaultValue)
{
         if (slotType == "Input")  return createInput <T>(name, defaultValue);
    else if (slotType == "Output") return createOutput<T>(name, defaultValue);

    return nullptr;
}

template <typename T>
gloperate::Input<T> * Stage::findInput(std::function<bool(gloperate::Input<T> *)> callback)
{
    const auto it = std::find_if(m_inputs.begin(), m_inputs.end(), [callback](gloperate::AbstractSlot * slot) {
        const auto input = dynamic_cast<gloperate::Input<T> *>(slot);

        if (!input)
        {
            return false;
        }

        return callback(input);
    });

    if (it == m_inputs.end())
    {
        return nullptr;
    }

    return static_cast<gloperate::Input<T> *>(*it);
}

template <typename T>
void Stage::forAllInputs(std::function<void(gloperate::Input<T> *)> callback)
{
    forAllInputs([& callback](gloperate::AbstractSlot * input) {
        const auto inputT = dynamic_cast<gloperate::Input<T> *>(input);

        if (!inputT)
        {
            return;
        }

        callback(inputT);
    });
}

template <typename T>
gloperate::Output<T> * Stage::findOutput(std::function<bool(gloperate::Output<T> *)> callback)
{
    const auto it = std::find_if(m_outputs.begin(), m_outputs.end(), [callback](gloperate::AbstractSlot * slot) {
        const auto output = dynamic_cast<gloperate::Output<T> *>(slot);

        if (!output)
        {
            return false;
        }

        return callback(output);
    });

    if (it == m_outputs.end())
    {
        return nullptr;
    }

    return static_cast<gloperate::Output<T> *>(*it);
}

template <typename T>
void Stage::forAllOutputs(std::function<void(gloperate::Output<T> *)> callback)
{
    forAllOutputs([& callback](gloperate::AbstractSlot * output) {
        const auto outputT = dynamic_cast<gloperate::Output<T> *>(output);

        if (!outputT)
        {
            return;
        }

        callback(outputT);
    });
}


} // namespace gloperate
