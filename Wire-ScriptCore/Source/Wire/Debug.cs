using System;
using System.Globalization;
using System.IO;

namespace Wire
{
	public static class Debug
	{

		private static void _Log(string message)
			=> InternalCalls.Debug_Log(0, message);

		private static void _LogInfo(string message) => Log(message);

		private static void _LogWarning(string message)
			=> InternalCalls.Debug_Log(1, message);

		private static void _LogError(string message)
			=> InternalCalls.Debug_Log(2, message);

		public static void Log(bool value)
		{
			_Log(value.ToString());
		}

		public static void Log(char value)
		{
			_Log(new string(value, 1));
		}

		public static void Log(char[] buffer)
		{
			_Log(new string(buffer));
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
			_Log(value.ToString());
		}

		public static void Log(double value)
		{
			_Log(value.ToString());
		}

		public static void Log(float value)
		{
			_Log(value.ToString());
		}

		public static void Log(int value)
		{
			_Log(value.ToString());
		}

		public static void Log(long value)
		{
			_Log(value.ToString());
		}

		public static void Log(object value)
		{
			_Log(value.ToString());
		}

		public static void Log(string value)
		{
			_Log(value);
		}

		public static void Log(string format, object arg0)
		{
			_Log(string.Format(format, arg0));
		}

		public static void Log(string format, object arg0, object arg1)
		{
			_Log(string.Format(format, arg0, arg1));
		}

		public static void Log(string format, object arg0, object arg1, object arg2)
		{
			_Log(string.Format(format, arg0, arg1, arg2));
		}

		public static void Log(string format, params object[] arg)
		{
			_Log(string.Format(format, arg));
		}

		public static void Log(uint value)
		{
			_Log(value.ToString());
		}

		public static void Log(ulong value)
		{
			_Log(value.ToString());
		}

		public static void LogInfo(bool value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogInfo(char value)
		{
			_LogInfo(new string(value, 1));
		}

		public static void LogInfo(char[] buffer)
		{
			_LogInfo(new string(buffer));
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
			_LogInfo(value.ToString());
		}

		public static void LogInfo(double value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogInfo(float value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogInfo(int value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogInfo(long value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogInfo(object value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogInfo(string value)
		{
			_LogInfo(value);
		}

		public static void LogInfo(string format, object arg0)
		{
			_LogInfo(string.Format(format, arg0));
		}

		public static void LogInfo(string format, object arg0, object arg1)
		{
			_LogInfo(string.Format(format, arg0, arg1));
		}

		public static void LogInfo(string format, object arg0, object arg1, object arg2)
		{
			_LogInfo(string.Format(format, arg0, arg1, arg2));
		}

		public static void LogInfo(string format, params object[] arg)
		{
			_LogInfo(string.Format(format, arg));
		}

		public static void LogInfo(uint value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogInfo(ulong value)
		{
			_LogInfo(value.ToString());
		}

		public static void LogWarning(bool value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogWarning(char value)
		{
			_LogWarning(new string(value, 1));
		}

		public static void LogWarning(char[] buffer)
		{
			_LogWarning(new string(buffer));
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
			_LogWarning(value.ToString());
		}

		public static void LogWarning(double value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogWarning(float value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogWarning(int value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogWarning(long value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogWarning(object value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogWarning(string value)
		{
			_LogWarning(value);
		}

		public static void LogWarning(string format, object arg0)
		{
			_LogWarning(string.Format(format, arg0));
		}

		public static void LogWarning(string format, object arg0, object arg1)
		{
			_LogWarning(string.Format(format, arg0, arg1));
		}

		public static void LogWarning(string format, object arg0, object arg1, object arg2)
		{
			_LogWarning(string.Format(format, arg0, arg1, arg2));
		}

		public static void LogWarning(string format, params object[] arg)
		{
			_LogWarning(string.Format(format, arg));
		}

		public static void LogWarning(uint value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogWarning(ulong value)
		{
			_LogWarning(value.ToString());
		}

		public static void LogError(bool value)
		{
			_LogError(value.ToString());
		}

		public static void LogError(char value)
		{
			_LogError(new string(value, 1));
		}

		public static void LogError(char[] buffer)
		{
			_LogError(new string(buffer));
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
			_LogError(value.ToString());
		}

		public static void LogError(double value)
		{
			_LogError(value.ToString());
		}

		public static void LogError(float value)
		{
			_LogError(value.ToString());
		}

		public static void LogError(int value)
		{
			_LogError(value.ToString());
		}

		public static void LogError(long value)
		{
			_LogError(value.ToString());
		}

		public static void LogError(object value)
		{
			_LogError(value.ToString());
		}

		public static void LogError(string value)
		{
			_LogError(value);
		}

		public static void LogError(string format, object arg0)
		{
			_LogError(string.Format(format, arg0));
		}

		public static void LogError(string format, object arg0, object arg1)
		{
			_LogError(string.Format(format, arg0, arg1));
		}

		public static void LogError(string format, object arg0, object arg1, object arg2)
		{
			_LogError(string.Format(format, arg0, arg1, arg2));
		}

		public static void LogError(string format, params object[] arg)
		{
			_LogError(string.Format(format, arg));
		}

		public static void LogError(uint value)
		{
			_LogError(value.ToString());
		}

		public static void LogError(ulong value)
		{
			_LogError(value.ToString());
		}
	}
}
