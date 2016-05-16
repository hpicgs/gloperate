
#include <gloperate/pipeline/RenderPipeline.h>


namespace gloperate
{


RenderPipeline::RenderPipeline(ViewerContext * viewerContext, const std::string & name, Pipeline * parent)
: Pipeline(viewerContext, name, parent)
, deviceViewport (this, "deviceViewport")
, virtualViewport(this, "virtualViewport")
, backgroundColor(this, "backgroundColor")
, frameCounter   (this, "frameCounter")
, timeDelta      (this, "timeDelta")
, redrawNeeded   (this, "redrawNeeded")
{
}

RenderPipeline::~RenderPipeline()
{
}


} // namespace gloperate
