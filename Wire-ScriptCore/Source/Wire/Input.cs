namespace Wire
{
	public class Input
	{
		public static bool IsKeyDown(KeyCode keycode)
		{
			return InternalCalls.Input_IsKeyDown(keycode);
		}

		public static bool IsMouseButtonPressed(MouseButton button)
		{
			return InternalCalls.Input_IsMouseButtonPressed(button);
		}

		public static (float X, float Y) GetMousePosition()
		{
			return (InternalCalls.Input_GetMouseX(), InternalCalls.Input_GetMouseY());
		}

		public static float GetMouseX()
		{
			return InternalCalls.Input_GetMouseX();
		}

		public static float GetMouseY()
		{
			return InternalCalls.Input_GetMouseY();
		}
	}
}
