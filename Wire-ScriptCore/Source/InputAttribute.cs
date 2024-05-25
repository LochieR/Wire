using System;

namespace Wire
{
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field)]
    public class InputAttribute : Attribute
    {
        public InputAttribute()
        {
        }
    }
}
