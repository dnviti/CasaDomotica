using System;
using System.Collections.Generic;
using System.Text;

namespace CasaDomoticaMobile.Models
{
    public class HttpResponse
    {
        public int Code { get; set; }
        public string Hardware { get; set; }
        public Dictionary<string, string> Readings { get; set; }
    }
}
