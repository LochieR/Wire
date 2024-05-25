using System;

using Wire;

namespace Wire.Modules
{
    public class SineOscillatorModule : Module
    {
        private uint phase = 0;
        private OutputModule output = new OutputModule();

        [Control(ControlType.Slider)] public float Frequency = 440.0f;

        [Output] public float Sine { get; private set; }
        [Output] public float Square { get; private set; }

        void OnUpdate()
        {
            Sine = 0.5f * MathF.Sin((2.0f * MathF.PI * phase * Frequency) / AudioProperties.SampleRate;
            Square = 0.5f * (MathF.Floor(MathF.Sin((2.0f * MathF.PI * phase * Frequency) / AudioProperties.SampleRate)) * 2.0f + 1.0f);

            phase++;

            output.LeftChannel = Sine;
            output.RightChannel = Sine;
            output.OnUpdate();
        }
    }
}
