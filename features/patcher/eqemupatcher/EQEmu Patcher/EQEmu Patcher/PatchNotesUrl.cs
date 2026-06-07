using System;

namespace EQEmu_Patcher
{
    [AttributeUsage(AttributeTargets.Assembly)]
    public class PatchNotesUrl : Attribute
    {
        public string Value { get; set; }

        public PatchNotesUrl(string value)
        {
            Value = value;
        }
    }
}
