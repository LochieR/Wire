using System;
using System.Runtime.CompilerServices;

namespace Wire
{
	public static class InternalCalls
	{
		#region Debug
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void Debug_Log(int logLevel, string message);
		#endregion Debug

		#region Entity
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Entity_HasComponent(ulong entityID, Type componentType);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern ulong Entity_FindEntityByName(string name);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern object GetScriptInstance(ulong entityID);
		#endregion Entity

		#region TagComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string TagComponent_GetTag(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TagComponent_SetTag(ulong entityID, string value);
		#endregion TagComponent

		#region TransformComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetTranslation(ulong entityID, out Vector3 translation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetTranslation(ulong entityID, ref Vector3 translation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetRotation(ulong entityID, out Vector3 rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetRotation(ulong entityID, ref Vector3 rotation);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_GetScale(ulong entityID, out Vector3 scale);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void TransformComponent_SetScale(ulong entityID, ref Vector3 scale);
		#endregion TransformComponent

		#region SpriteRendererComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_GetColour(ulong entityID, out Vector4 colour);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetColour(ulong entityID, ref Vector4 colour);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern string SpriteRendererComponent_GetTexturePath(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetTexturePath(ulong entityID, string path);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float SpriteRendererComponent_GetTilingFactor(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void SpriteRendererComponent_SetTilingFactor(ulong entityID, float tilingFactor);
		#endregion SpriteRendererComponent

		#region CameraComponent
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_IsPrimary(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPrimary(ulong entityID, bool primary);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern CameraComponent.ProjectionType CameraComponent_GetProjectionType(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetProjectionType(ulong entityID, CameraComponent.ProjectionType value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveVerticalFOV(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveVerticalFOV(ulong entityID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveNear(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveNear(ulong entityID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetPerspectiveFar(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetPerspectiveFar(ulong entityID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicSize(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicSize(ulong entityID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicNear(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicNear(ulong entityID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern float CameraComponent_GetOrthographicFar(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetOrthographicFar(ulong entityID, float value);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool CameraComponent_IsFixedAspectRatio(ulong entityID);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern void CameraComponent_SetFixedAspectRatio(ulong entityID, bool value);
		#endregion CameraComponent

		#region Input
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal static extern bool Input_IsKeyDown(KeyCode keycode);
		#endregion Input
	}
}
