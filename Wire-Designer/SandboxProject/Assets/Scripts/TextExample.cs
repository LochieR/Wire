using System;

using Wire;

namespace Sandbox
{
	public class TextExample : Entity
	{
		void OnCreate()
		{
			AddComponent<TextComponent>();
		}

		void OnUpdate()
		{
		}
	}
}
