#pragma once

#include <format>
#include <string>
#include <concepts>
#include <type_traits>

namespace Wire::Controls {

	class INumberControl
	{
	public:
		virtual std::string GetValueString() const = 0;

		virtual void SetValue(void* value) = 0;
		virtual void* GetValueGeneric() const = 0;
	};

	template<typename T>
	concept numeric = std::integral<T> or std::floating_point<T>;

	template<typename T>
	requires numeric<T>
	class SliderControl : public INumberControl
	{
	public:
		SliderControl() = default;
		SliderControl(std::function<T()> getter, std::function<void(T)> setter)
			: m_Getter(getter), m_Setter(setter)
		{
		}

		void SetValue(T value) { m_Setter(value); }
		T GetValue() const { return m_Getter(); }

		virtual std::string GetValueString() const override { return std::format("{}", m_Getter()); }

		virtual void SetValue(void* value) override { m_Setter(*reinterpret_cast<T*>(value)); }
		virtual void* GetValueGeneric() const override { return nullptr; }
	private:
		std::function<T()> m_Getter;
		std::function<void(T)> m_Setter;
	};

}
