#pragma once
#include <cstdint>
class BufferID
{
public:
	// Constructor
	BufferID() : m_ID(0) {}
	// Constructor
	BufferID(const uint32_t& id) : m_ID(id) {}
	// Destructor
	~BufferID() = default;
	// Getters
	const uint32_t& GetID() const { return m_ID; }
	// Setters
	void SetID(const uint32_t& id) { m_ID = id; }
protected:
	uint32_t m_ID;
};