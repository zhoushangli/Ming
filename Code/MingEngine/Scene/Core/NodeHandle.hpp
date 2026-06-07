#pragma once

struct NodeHandle
{
public:
	NodeHandle();
	NodeHandle(unsigned int uid, unsigned int index);

	bool         IsValid() const;
	unsigned int GetIndex() const;
	unsigned int GetUID() const;
	bool         operator==(NodeHandle const& other) const;
	bool         operator!=(NodeHandle const& other) const;
	bool         operator<(NodeHandle const& other) const;

	static const NodeHandle   Invalid;
	static const unsigned int kMaxNodeUid   = 0x0000fffeu;
	static const unsigned int kMaxNodeIndex = 0x0000ffffu;

private:
	unsigned int m_data = 0xffffffffu;
};
