using System;

namespace Wire
{
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field)]
    public class OutputAttribute : Attribute
    {
        public OutputAttribute()
        {
        }
    }
}
