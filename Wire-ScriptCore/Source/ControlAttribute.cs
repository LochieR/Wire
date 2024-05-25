using System;

namespace Wire
{
    public enum ControlType
    {
        Literal = 0,
        Knob,
        Slider
    }

    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field)]
    public class ControlAttribute : Attribute
    {
        public ControlType ControlType { get; set; }

        public ControlAttribute(ControlType type)
        {
            ControlType = type;
        }
    }
}
