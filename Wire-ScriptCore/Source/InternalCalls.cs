#pragma warning disable 649 // '***' is never assigned to, and will always have its default value

global using CString = Coral.Managed.Interop.NativeString;

namespace Wire
{
    internal unsafe class InternalCalls
    {
        internal static delegate*<uint> AudioProperties_GetSampleRate;
        internal static delegate*<uint> AudioProperties_GetNumChannels;

        internal static delegate*<float, float, void> AudioEngine_WriteOutput;
    }
}
