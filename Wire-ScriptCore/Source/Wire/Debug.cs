using System;

namespace Wire
{
	public static class Debug
	{
		#region Log
		private static void Log_Internal(string message)
			=> InternalCalls.Debug_Log(0, message);

		private static void LogInfo_Internal(string message) => Log(message);

		private static void LogWarning_Internal(string message)
			=> InternalCalls.Debug_Log(1, message);

		private static void LogError_Internal(string message)
			=> InternalCalls.Debug_Log(2, message);

		public static void Log(bool value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(char value)
		{
			Log_Internal(new string(value, 1));
		}

		public static void Log(char[] buffer)
		{
			Log_Internal(new string(buffer));
		}

		public static void Log(char[] buffer, int index, int count)
		{
			for (int i = 0; i < count; i++)
			{
				Log(buffer[index + i]);
			}
		}

		public static void Log(decimal value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(double value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(float value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(int value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(long value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(object value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(string value)
		{
			Log_Internal(value);
		}

		public static void Log(string format, object arg0)
		{
			Log_Internal(string.Format(format, arg0));
		}

		public static void Log(string format, object arg0, object arg1)
		{
			Log_Internal(string.Format(format, arg0, arg1));
		}

		public static void Log(string format, object arg0, object arg1, object arg2)
		{
			Log_Internal(string.Format(format, arg0, arg1, arg2));
		}

		public static void Log(string format, params object[] arg)
		{
			Log_Internal(string.Format(format, arg));
		}

		public static void Log(uint value)
		{
			Log_Internal(value.ToString());
		}

		public static void Log(ulong value)
		{
			Log_Internal(value.ToString());
		}

		public static void LogInfo(bool value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(char value)
		{
			LogInfo_Internal(new string(value, 1));
		}

		public static void LogInfo(char[] buffer)
		{
			LogInfo_Internal(new string(buffer));
		}

		public static void LogInfo(char[] buffer, int index, int count)
		{
			for (int i = 0; i < count; i++)
			{
				LogInfo(buffer[index + i]);
			}
		}

		public static void LogInfo(decimal value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(double value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(float value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(int value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(long value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(object value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(string value)
		{
			LogInfo_Internal(value);
		}

		public static void LogInfo(string format, object arg0)
		{
			LogInfo_Internal(string.Format(format, arg0));
		}

		public static void LogInfo(string format, object arg0, object arg1)
		{
			LogInfo_Internal(string.Format(format, arg0, arg1));
		}

		public static void LogInfo(string format, object arg0, object arg1, object arg2)
		{
			LogInfo_Internal(string.Format(format, arg0, arg1, arg2));
		}

		public static void LogInfo(string format, params object[] arg)
		{
			LogInfo_Internal(string.Format(format, arg));
		}

		public static void LogInfo(uint value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogInfo(ulong value)
		{
			LogInfo_Internal(value.ToString());
		}

		public static void LogWarning(bool value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(char value)
		{
			LogWarning_Internal(new string(value, 1));
		}

		public static void LogWarning(char[] buffer)
		{
			LogWarning_Internal(new string(buffer));
		}

		public static void LogWarning(char[] buffer, int index, int count)
		{
			for (int i = 0; i < count; i++)
			{
				LogWarning(buffer[index + i]);
			}
		}

		public static void LogWarning(decimal value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(double value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(float value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(int value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(long value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(object value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(string value)
		{
			LogWarning_Internal(value);
		}

		public static void LogWarning(string format, object arg0)
		{
			LogWarning_Internal(string.Format(format, arg0));
		}

		public static void LogWarning(string format, object arg0, object arg1)
		{
			LogWarning_Internal(string.Format(format, arg0, arg1));
		}

		public static void LogWarning(string format, object arg0, object arg1, object arg2)
		{
			LogWarning_Internal(string.Format(format, arg0, arg1, arg2));
		}

		public static void LogWarning(string format, params object[] arg)
		{
			LogWarning_Internal(string.Format(format, arg));
		}

		public static void LogWarning(uint value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogWarning(ulong value)
		{
			LogWarning_Internal(value.ToString());
		}

		public static void LogError(bool value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(char value)
		{
			LogError_Internal(new string(value, 1));
		}

		public static void LogError(char[] buffer)
		{
			LogError_Internal(new string(buffer));
		}

		public static void LogError(char[] buffer, int index, int count)
		{
			for (int i = 0; i < count; i++)
			{
				LogError(buffer[index + i]);
			}
		}

		public static void LogError(decimal value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(double value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(float value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(int value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(long value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(object value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(string value)
		{
			LogError_Internal(value);
		}

		public static void LogError(string format, object arg0)
		{
			LogError_Internal(string.Format(format, arg0));
		}

		public static void LogError(string format, object arg0, object arg1)
		{
			LogError_Internal(string.Format(format, arg0, arg1));
		}

		public static void LogError(string format, object arg0, object arg1, object arg2)
		{
			LogError_Internal(string.Format(format, arg0, arg1, arg2));
		}

		public static void LogError(string format, params object[] arg)
		{
			LogError_Internal(string.Format(format, arg));
		}

		public static void LogError(uint value)
		{
			LogError_Internal(value.ToString());
		}

		public static void LogError(ulong value)
		{
			LogError_Internal(value.ToString());
		}
		#endregion Log
	}
}
