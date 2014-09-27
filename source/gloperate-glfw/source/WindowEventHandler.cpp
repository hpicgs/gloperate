#include <gloperate-glfw/WindowEventHandler.h>

#include <globjects/globjects.h>

#include <gloperate-glfw/Window.h>
#include <gloperate-glfw/events.h>

#include <gloperate/capabilities/AbstractViewportCapability.h>
#include <gloperate/capabilities/AbstractVirtualTimeCapability.h>

#include <gloperate/tools/ScreenshotTool.h>


using namespace gloperate;
namespace gloperate_glfw
{

WindowEventHandler::WindowEventHandler()
{
}

WindowEventHandler::~WindowEventHandler()
{
}

void WindowEventHandler::initialize(Window & window)
{
    globjects::init();
    IF_DEBUG(globjects::DebugMessage::enable(true);)

    if (window.painter())
        window.painter()->initialize();
}

void WindowEventHandler::framebufferResizeEvent(ResizeEvent & event)
{
    if (event.window()->painter()) {
        // Check if the painter supports the viewport capability
        AbstractViewportCapability * viewportCapability = event.window()->painter()->getCapability<AbstractViewportCapability>();

        if (viewportCapability)
        {
            // Resize painter
            viewportCapability->setViewport(0, 0, event.width(), event.height());
        }
    }
}

void WindowEventHandler::paintEvent(PaintEvent & event)
{
    if (event.window()->painter()) {
        // Call painter
        event.window()->painter()->paint();
    }
}

void WindowEventHandler::keyPressEvent(KeyEvent & event)
{
    if (event.key() == GLFW_KEY_F10)
    {
        if (ScreenshotTool::isApplicableTo(event.window()->painter()))
        {
            ScreenshotTool screenshot(event.window()->painter(), event.window()->resourceManager());

            screenshot.initialize();

            screenshot.save("screenshot.png");
        }
    }
}

void WindowEventHandler::timerEvent(TimerEvent & event)
{
    if (!event.window()->painter())
        return;

    AbstractVirtualTimeCapability * timeCapability = event.window()->painter()->getCapability<AbstractVirtualTimeCapability>();

    if (timeCapability && timeCapability->enabled())
    {
        timeCapability->update(std::chrono::duration_cast<std::chrono::duration<float>>(event.elapsed()).count());
        event.window()->repaint();
    }
}

} // namespace gloperate_glfw
