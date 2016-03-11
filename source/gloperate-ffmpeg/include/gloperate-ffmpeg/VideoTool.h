
#pragma once


#include <glm/glm.hpp>

#include <string>
#include <functional>

#include <globjects/base/ref_ptr.h>

#include <gloperate-ffmpeg/VideoEncoder.h>

#include <gloperate-ffmpeg/gloperate-ffmpeg_api.h>


namespace gloperate {
    class ViewerContext;
    class RenderSurface;
    class AbstractGLContext;
}


namespace gloperate_ffmpeg
{


/**
*  @brief
*    A tool which renders a given Stage into an output video file.
*/
class GLOPERATE_FFMPEG_API VideoTool
{
public:
    /**
    *  @brief
    *    Constructor
    *
    *  @param[in] filename
    *    Name of output video file
    *  @param[in] stage
    *    The stage that will be rendered into a video
    *  @param[in] glContext
    *    A valid OpenGL context
    *  @param[in] fps
    *    Frames per second of output video
    *  @param[in] length
    *    Length (in seconds) of output video
    *  @param[in] width
    *    Width (in pixels) of output video
    *  @param[in] height
    *    Height (in pixels) of output video
    */
    VideoTool(const std::string & filename,
              gloperate::RenderSurface * surface,
              uint fps,
              uint length,
              uint width,
              uint height);

    /**
    *  @brief
    *    Destructor
    */
    virtual ~VideoTool();

    /**
    *  @brief
    *    Actual call to start video creation
    *
    *  @param[in] progress
    *    Progress callback function
    */
    void createVideo(std::function<void(int, int)> progress);


protected:
    globjects::ref_ptr<VideoEncoder>  m_videoEncoder;
    gloperate::ViewerContext        * m_context;
    gloperate::RenderSurface        * m_surface;
    gloperate::AbstractGLContext    * m_glContext;
    std::string                       m_filename;
    uint                              m_fps;
    uint                              m_length;
    uint                              m_width;
    uint                              m_height;
    float                             m_timeDelta;
};


} // namespace gloperate_ffmpeg
