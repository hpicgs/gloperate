#include <gloperate/navigation/Trackball.h>

#include <glm/gtx/norm.hpp>

namespace gloperate
{

Trackball::Trackball()
: m_radius(1.0)
{
}

void Trackball::reset()
{
	m_orientation = glm::dquat();
}

void Trackball::reset(const glm::dmat3& rotation)
{
    m_orientation = glm::dquat(rotation);
}

void Trackball::setRadius(double radius)
{
	m_radius = radius;
}

glm::dquat Trackball::orientation() const
{
	return m_orientation;
}

glm::dmat3 Trackball::orientationMatrix() const
{
	return glm::mat3_cast(m_orientation);
}

void Trackball::rotate(const glm::dvec2 & from, const glm::dvec2 & to)
{
    m_orientation *= glm::dquat(mapToSphere(from), mapToSphere(to));
}

glm::dvec3 Trackball::mapToSphere(const glm::dvec2 & pos) const
{
	return glm::normalize(glm::dvec3(-pos.x, pos.y, depth(pos)));
}

double Trackball::depth(const glm::dvec2 & pos) const
{
	// http://www.opengl.org/wiki/TrackballNavigation
	double d = glm::length2(pos);
	double r2 = m_radius * m_radius;

	return d <= r2 * 0.5 ? glm::sqrt(r2 - d) : r2 * 0.5 / glm::sqrt(d);
}

} // namespace gloperate
