using IoTApi.Services;
using Microsoft.AspNetCore.Mvc;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;

namespace IoTApi.Controllers
{
    public class SensorController : ControllerBase
    {
        private readonly SensorService _sensorService;

        public SensorController(SensorService sensorService)
        {
            _sensorService = sensorService;
        }

    }
}
