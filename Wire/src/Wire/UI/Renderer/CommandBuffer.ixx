module;

#include <type_traits>

export module wire.ui.renderer:cmdBuffer;

namespace wire {

	export class CommandBuffer
	{
	public:
		CommandBuffer() = default;
		CommandBuffer(void* commandBuffer)
			: m_CommandBuffer(commandBuffer)
		{
		}

		template<typename T>
		std::enable_if_t<std::is_pointer<T>::value, T> as()
		{
			return reinterpret_cast<T>(m_CommandBuffer);
		}
	private:
		void* m_CommandBuffer = nullptr;
	};

}
