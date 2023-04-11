using System;

using Wire;

namespace Sandbox
{
	public class TextureTest : Entity
	{
		public string Path;
		private string m_LastPath;

		private static int m_NumTests = 0;

		private bool m_TextureHasChanged = false;
		private float m_Time = 0.0f;

		void OnCreate()
		{
			m_Time = m_NumTests * 90;
			m_NumTests++;
		}

		void OnUpdate(float ts)
		{
			if (Input.IsKeyDown(KeyCode.Tab) && !m_TextureHasChanged)
			{
				Path = "Assets/Textures/WireLogo.png";
			}

			if (m_LastPath != Path)
			{
				GetComponent<SpriteRendererComponent>().Texture = Path;
			}

			m_LastPath = Path;

			float tilingFactor = Math.Abs(10.0f * (float)Math.Sin(m_Time));
			GetComponent<SpriteRendererComponent>().TilingFactor = tilingFactor;

			m_Time += 3.0f * m_NumTests * ts;
		}
	}

}
