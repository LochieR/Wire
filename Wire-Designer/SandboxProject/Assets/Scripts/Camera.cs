using System;

using Wire;

namespace Sandbox
{
	public class Camera : Entity
	{
		public float Speed;
		public float Timestep;
		public Entity OtherEntity;
		public Vector3 Value;

		void OnCreate()
		{
			Debug.LogInfo($"Camera: {ID}");
		}

		void OnUpdate(float ts)
		{
			Timestep += ts;

			if (Name != "Camera B")
			{
				float speed = Speed;
				Vector3 velocity = Vector3.Zero;

				if (Input.IsKeyDown(KeyCode.W))
					velocity.Y = 1.0f;
				else if (Input.IsKeyDown(KeyCode.S))
					velocity.Y = -1.0f;

				if (Input.IsKeyDown(KeyCode.A))
					velocity.X = -1.0f;
				else if (Input.IsKeyDown(KeyCode.D))
					velocity.X = 1.0f;

				Entity other = FindEntityByName("Camera B");//.As<Camera>();
				other.Translation = Translation;

				if (Input.IsKeyDown(KeyCode.Tab))
				{
					// switch primary camera

					other.GetComponent<CameraComponent>().Primary = other.GetComponent<CameraComponent>().Primary ? false : true;
					GetComponent<CameraComponent>().Primary = GetComponent<CameraComponent>().Primary ? false : true;
				}

				velocity *= speed;

				Vector3 translation = Translation;
				translation += velocity * ts;
				Translation = translation;
			}
		}
	}
}
