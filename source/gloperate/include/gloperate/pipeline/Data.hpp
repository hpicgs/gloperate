
#pragma once


#include <gloperate/pipeline/Data.h>

#include <gloperate/pipeline/PipelineEvent.h>


namespace gloperate
{


template <typename T, typename BASE>
Data<T, BASE>::Data(Stage * parent, const std::string & name, const T & value)
: cppexpose::DynamicProperty<T, BASE>(parent, name, value)
{
}

template <typename T, typename BASE>
Data<T, BASE>::~Data()
{
}

template <typename T, typename BASE>
T & Data<T, BASE>::operator*()
{
    return *this->ptr();
}

template <typename T, typename BASE>
const T & Data<T, BASE>::operator*() const
{
    return *this->ptr();
}

template <typename T, typename BASE>
T * Data<T, BASE>::operator->()
{
    return this->ptr();
}

template <typename T, typename BASE>
const T * Data<T, BASE>::operator->() const
{
    return this->ptr();
}

template <typename T, typename BASE>
void Data<T, BASE>::onValueChanged(const T & value)
{
    this->valueChanged(value);

    this->m_owner->promotePipelineEvent(
        PipelineEvent(PipelineEvent::ValueChanged, this->m_owner, this)
    );
}


} // namespace gloperate
