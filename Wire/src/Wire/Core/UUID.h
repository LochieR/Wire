#pragma once

#include <string>

namespace wire {

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() const { return m_UUID; }
		
		bool operator==(UUID other) const
		{
			return m_UUID == other.m_UUID;
		}

		bool operator!=(UUID other) const
		{
			return !operator==(other);
		}
	private:
		uint64_t m_UUID;
	};

}

namespace std {

	template<typename T> struct hash;

	template<>
	struct hash<wire::UUID>
	{
		std::size_t operator()(const wire::UUID& uuid) const
		{
			return (uint64_t)uuid;
		}
	};

}
