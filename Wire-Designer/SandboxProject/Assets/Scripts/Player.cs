using System;

using Wire;

namespace Sandbox
{
	public class Player : Entity
	{
		public float Speed;

		private Entity m_Camera;

		void OnCreate()
		{
			m_Camera = FindEntityByName("Camera A");
		}

		void OnUpdate(float ts)
		{
			Vector3 velocity = Vector3.Zero;

			if (Input.IsKeyDown(KeyCode.W))
				velocity.Y = 1.0f;
			else if (Input.IsKeyDown(KeyCode.S))
				velocity.Y = -1.0f;
			if (Input.IsKeyDown(KeyCode.A))
				velocity.X = -1.0f;
			else if (Input.IsKeyDown(KeyCode.D))
				velocity.X = 1.0f;

			if (m_Camera != null)
			{
				Camera camera = m_Camera.As<Camera>();

				if (Input.IsKeyDown(KeyCode.Q))
					camera.DistanceFromPlayer += 2.0f * Speed * ts;
				else if (Input.IsKeyDown(KeyCode.E))
					camera.DistanceFromPlayer -= 2.0f * Speed * ts;
			}

			velocity *= Input.IsKeyDown(KeyCode.LeftShift) ? Speed * 0.5f : Speed;
			Vector3 translation = Translation;
			translation += velocity * ts;
			Translation = translation;
		}
	}
}
