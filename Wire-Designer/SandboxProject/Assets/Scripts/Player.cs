using System;
using System.Linq;
using System.Runtime.InteropServices;
using Wire;

namespace Sandbox
{
	public class Player : Entity
	{
		public float Speed;
		public float AccelerationSpeed;

		private Entity m_Camera;
		private float m_Acceleration = 0.0f;

		void OnCreate()
		{
			m_Camera = FindEntityByName("Camera A");
		}

		void OnUpdate(float ts)
		{
			Vector3 velocity = Vector3.Zero;

			bool[] keys = new[] { false, false, false, false };

			if (keys[0] = Input.IsKeyDown(KeyCode.W))
				velocity.Y = 1.0f * m_Acceleration;
			else if (keys[1] = Input.IsKeyDown(KeyCode.S))
				velocity.Y = -1.0f * m_Acceleration;
			if (keys[2] = Input.IsKeyDown(KeyCode.D))
				velocity.X = 1.0f * m_Acceleration;
			else if (keys[3] = Input.IsKeyDown(KeyCode.A))
				velocity.X = -1.0f * m_Acceleration;

			bool pressed = false;

			foreach (bool key in keys)
				if (key)
					pressed = true;

			if (!pressed)
				m_Acceleration = 0.0f;

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

			m_Acceleration += m_Acceleration < 3.0f ? (AccelerationSpeed * ts) : 0.0f;
		}
	}
}
