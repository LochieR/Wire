using Wire;

namespace Wire.Modules
{
    public class OutputModule : Module
    {
        [Input] public float LeftChannel;
        [Input] public float RightChannel;

        public unsafe void OnUpdate()
        {
            InternalCalls.AudioEngine_WriteOutput(LeftChannel, RightChannel);
        }
    }
}
