namespace Wire
{
    public static unsafe class AudioProperties
    {
        public static uint SampleRate
        {
            get
            {
                return InternalCalls.AudioProperties_GetSampleRate();
            }
        }

        public static uint Channels
        {
            get
            {
                return InternalCalls.AudioProperties_GetNumChannels();
            }
        }
    }
}
