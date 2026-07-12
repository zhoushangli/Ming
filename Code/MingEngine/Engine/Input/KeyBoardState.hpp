#pragma once

struct KeyButtonState
{
public:
	bool m_isDown;
	bool m_wasPressed;
	bool m_wasReleased;
	int  m_repeatCount;
};
