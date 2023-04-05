using System;

namespace Wire
{
	public class Debug
	{
		public static void Log(string message)
			=> InternalCalls.UILog_Log(0, message);

		public static void LogInfo(string message) => Log(message);

		public static void LogWarning(string message)
			=> InternalCalls.UILog_Log(1, message);

		public static void LogError(string message)
			=> InternalCalls.UILog_Log(2, message);
	}
}
