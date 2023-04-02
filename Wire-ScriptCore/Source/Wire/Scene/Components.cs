using System;

namespace Wire
{
	public abstract class Component
	{
		public Entity Entity { get; internal set; }
	}

	public class TransformComponent : Component
	{
		public Vector3 Translation
		{
			get
			{
				InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 translation);
				return translation;
			}
			set
			{
				InternalCalls.TransformComponent_SetTranslation(Entity.ID, ref value);
			}
		}

		public Vector3 Rotation
		{
			get
			{
				InternalCalls.TransformComponent_GetRotation(Entity.ID, out Vector3 rotation);
				return rotation;
			}
			set
			{
				InternalCalls.TransformComponent_SetRotation(Entity.ID, ref value);
			}
		}

		public Vector3 Scale
		{
			get
			{
				InternalCalls.TransformComponent_GetScale(Entity.ID, out Vector3 scale);
				return scale;
			}
			set
			{
				InternalCalls.TransformComponent_SetScale(Entity.ID, ref value);
			}
		}
	}

	public class SpriteRendererComponent : Component
	{
		public Vector4 Colour
		{
			get
			{
				InternalCalls.SpriteRendererComponent_GetColour(Entity.ID, out Vector4 colour);
				return colour;
			}
			set
			{
				InternalCalls.SpriteRendererComponent_SetColour(Entity.ID, ref value);
			}
		}

		public string Texture
		{
			get
			{
				return InternalCalls.SpriteRendererComponent_GetTexturePath(Entity.ID);
			}
			set
			{
				InternalCalls.SpriteRendererComponent_SetTexturePath(Entity.ID, value);
			}
		}

		public float TilingFactor
		{
			get
			{
				return InternalCalls.SpriteRendererComponent_GetTilingFactor(Entity.ID);
			}
			set
			{
				InternalCalls.SpriteRendererComponent_SetTilingFactor(Entity.ID, value);
			}
		}
	}
}
