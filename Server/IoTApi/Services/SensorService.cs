using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace IoTApi.Services
{
    
    public class SensorService
    {
        private Dictionary<string, string> _values;
        public SensorService()
        {
            _values = new Dictionary<string, string>();
        }

        void AddUpdateInformation(string type,string value)
        {
            RemoveInformation(type);

            _values.Add(type, value);
        }

        void RemoveInformation(string type)
        {
            if (_values.ContainsKey(type))
                _values.Remove(type);
        }
    }
}
