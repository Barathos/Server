using System;

namespace EQEmu_Patcher
{
    [AttributeUsage(AttributeTargets.Assembly)]
    public class ServiceStatusUrl : Attribute
    {
        public string Value { get; set; }

        public ServiceStatusUrl(string value)
        {
            Value = value;
        }
    }
}
