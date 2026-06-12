#include "MingEngine/Scene/Core/NodePath.hpp"

NodePath::NodePath(std::string path) { Parse(std::move(path)); }

void NodePath::Parse(std::string path)
{
	m_paths.clear();
	m_isAbsolute = !path.empty() && path.front() == '/';

	if (path == ".")
	{
		m_paths.push_back(".");
		return;
	}

	if (path.empty() || (m_isAbsolute && path.size() == 1) || path.back() == '/')
	{
		return;
	}

	size_t segmentStart = m_isAbsolute ? 1 : 0;
	while (segmentStart < path.size())
	{
		size_t const      separator  = path.find('/', segmentStart);
		size_t const      segmentEnd = separator == std::string::npos ? path.size() : separator;
		std::string const segment    = path.substr(segmentStart, segmentEnd - segmentStart);
		if (segment.empty() || segment == "." || segment == "..")
		{
			m_paths.clear();
			return;
		}

		m_paths.push_back(segment);
		if (separator == std::string::npos)
		{
			break;
		}
		segmentStart = separator + 1;
	}
}