#pragma once

#include <cstdint>

typedef uint32_t ResourceID;
typedef uint32_t ThreadID;

class Identifiable
{
public:
    virtual ~Identifiable() = default;
    [[nodiscard]] ResourceID getID() const { return m_ID; }

protected:
    Identifiable() : m_ID(s_IDcounter++) {}

	ResourceID m_ID = 0;

private:
	inline static ResourceID s_IDcounter = 0;
};


class VulkanDeviceSubresource : public Identifiable
{
public:
    explicit VulkanDeviceSubresource(const ResourceID p_ParentID) : m_Device(p_ParentID) {}

    [[nodiscard]] ResourceID getDeviceID() const { return m_Device; }

private:
    virtual void free() = 0;

    uint32_t m_Device;

	friend class VulkanDevice;
};