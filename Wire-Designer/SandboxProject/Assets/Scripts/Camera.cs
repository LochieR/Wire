using System;

using Wire;

namespace Sandbox
{
	public class Camera : Entity
	{
		public float DistanceFromPlayer = 5.0f;

		private Entity m_Player;

		void OnCreate()
		{
			Debug.Log($"Camera: {ID}");
			m_Player = FindEntityByName("Player");
		}

		void OnUpdate(float ts)
		{
			if (m_Player != null)
				Translation = new Vector3(m_Player.Translation.X, m_Player.Translation.Y, m_Player.Translation.Z + DistanceFromPlayer);
		}
	}
}
