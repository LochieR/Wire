#pragma once

#include "Wire/Core/Application.h"

#include <concepts>

namespace Wire {

	class IResource
	{
	public:
		virtual ~IResource() = default;
	};

	class rbIRef
	{
	public:
		virtual void Release() = 0;
		virtual IResource* GetResource() = 0;
	};

	template<typename T>
	concept IResourceConcept = requires(T)
	{
		std::is_base_of<IResource, T>::value;
	};

	template<typename TRes> requires IResourceConcept<TRes>
	class rbRef : public rbIRef
	{
	public:
		rbRef() = default;
		rbRef(std::nullptr_t) : rbRef() {}
		rbRef(TRes* ptr)
			: m_Resource(ptr)
		{
		}

		virtual void Release() override
		{
			Application::Get().GetRenderer()->Free(*this);
		}

		TRes* Get()
		{
			return m_Resource;
		}

		virtual IResource* GetResource() override
		{
			return m_Resource;
		}

		TRes* operator->()
		{
			return m_Resource;
		}

		operator bool()
		{
			return m_Resource;
		}

		bool operator==(rbRef<TRes> other)
		{
			return m_Resource == other.m_Resource;
		}

		bool operator!=(rbRef<TRes> other)
		{
			return !(operator==(other));
		}
	private:
		TRes* m_Resource = nullptr;
	};

}
