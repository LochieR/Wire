using System;

using Wire;

namespace Sandbox
{
	public class Camera : Entity
	{
		void OnCreate()
		{
			Debug.LogWarning($"Camera: {ID}");
			Debug.LogInfo($"Camera Position: {Translation.X}, {Translation.Y}, {Translation.Z}");
			Debug.LogError($"Camera Rotation: {Rotation.X}, {Rotation.Y}, {Rotation.Z}");
		}

		void OnUpdate(float ts)
		{
			float speed = 2.5f;
			Vector3 velocity = Vector3.Zero;

			if (Input.IsKeyDown(KeyCode.W))
				velocity.Y = 1.0f;
			else if (Input.IsKeyDown(KeyCode.S))
				velocity.Y = -1.0f;

			if (Input.IsKeyDown(KeyCode.A))
				velocity.X = -1.0f;
			else if (Input.IsKeyDown(KeyCode.D))
				velocity.X = 1.0f;

			velocity *= speed;
			
			Vector3 translation = Translation;
			translation += velocity * ts;
			Translation = translation;

			Vector3 rotation = Rotation;
			rotation.Z += 2.0f * ts;
			Rotation = rotation;
		}
	}
}
