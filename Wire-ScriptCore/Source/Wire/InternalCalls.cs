using System;
using System.Runtime.CompilerServices;

namespace Wire
{
	public static class InternalCalls
	{
		#region Entity
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static bool Entity_HasComponent(ulong entityID, Type componentType);
		#endregion Entity

		#region TransformComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetRotation(ulong entityID, out Vector3 rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetRotation(ulong entityID, ref Vector3 rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_GetScale(ulong entityID, out Vector3 scale);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void TransformComponent_SetScale(ulong entityID, ref Vector3 scale);
		#endregion TransformComponent

		#region SpriteRendererComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_GetColour(ulong entityID, out Vector4 colour);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_SetColour(ulong entityID, ref Vector4 colour);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static string SpriteRendererComponent_GetTexturePath(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_SetTexturePath(ulong entityID, string path);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static float SpriteRendererComponent_GetTilingFactor(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void SpriteRendererComponent_SetTilingFactor(ulong entityID, float tilingFactor);
		#endregion SpriteRendererComponent

		#region Input
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static bool Input_IsKeyDown(KeyCode keycode);
		#endregion Input
	}
}
