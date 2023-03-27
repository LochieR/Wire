using System;
using System.Runtime.CompilerServices;

namespace Wire
{
	public struct Vector3
	{
		public float X, Y, Z;

		public Vector3(float x, float y, float z)
		{
			X = x;
			Y = y;
			Z = z;
		}
	}

	public static class InternalCalls
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog(string text, int parameter);

		[MethodImpl(MethodImplOptions.InternalCall)]
		internal extern static void NativeLog_Vector(ref Vector3 parameter, out Vector3 result);
	}

	public class Entity
	{
		public float FloatVar { get; set; }

		public Entity()
		{
			Console.WriteLine("Main constructor!");
			Log("Hello: ", 324289);

			Vector3 pos = new Vector3(5, 2.2f, 1);
			var result = Log(pos);
			Console.WriteLine($"{result.X}, {result.Y}, {result.Z}");
		}

		public void PrintMessage()
		{
			Console.WriteLine("Hello World from C#!");
		}

		public void PrintInt(int value)
		{
			Console.WriteLine($"C# says: {value}");
		}

		public void PrintCustomMessage(string message)
		{
			Console.WriteLine($"C# says: {message}");
		}

		private void Log(string text, int parameter)
		{
			InternalCalls.NativeLog(text, parameter);
		}

		private Vector3 Log(Vector3 parameter)
		{
			InternalCalls.NativeLog_Vector(ref parameter, out Vector3 result);
			return result;
		}
	}
}
