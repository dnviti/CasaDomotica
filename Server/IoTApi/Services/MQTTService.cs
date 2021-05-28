using Microsoft.Extensions.Hosting;
using MQTTnet;
using MQTTnet.Protocol;
using MQTTnet.Server;
using MQTTnet.Client;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using MQTTnet.Client.Options;
using MQTTnet.Extensions.ManagedClient;

namespace IoTApi.Services
{
    public class MQTTService : IHostedService
    {
        IMqttServer _mqttServer;
        IManagedMqttClient _mqttClient;
        public MQTTService()
        {
            _mqttServer = new MqttFactory().CreateMqttServer();
            //_mqttClient = new MqttFactory().CreateMqttClient();
        }

        public async Task SetupClientAsync(CancellationToken cancellationToken)
        {
            // Setup and start a managed MQTT client.
            var options = new ManagedMqttClientOptionsBuilder()
                .WithAutoReconnectDelay(TimeSpan.FromSeconds(5))
                .WithClientOptions(new MqttClientOptionsBuilder()
                    .WithClientId("Server")
                    //.WithWebSocketServer("localhost:50482/mqtt")
                    .Build())
                .Build();

            _mqttClient = new MqttFactory().CreateManagedMqttClient();

            await _mqttClient.StartAsync(options);


            await _mqttClient.SubscribeAsync(new MqttTopicFilterBuilder().WithTopic("MqttWeatherForecast/90210/temperature").Build());

            _mqttClient.UseConnectedHandler(e =>
            {
                //Console.WriteLine($"Connection Result: {e.AuthenticateResult.ResultCode}");
            });

            _mqttClient.UseApplicationMessageReceivedHandler(e =>
            {
                //Console.WriteLine($"Message from {e.ClientId}: {e.ApplicationMessage.Payload.Length} bytes.");
            });

            await _mqttClient.StartAsync(options);

            await _mqttClient.PublishAsync(new ManagedMqttApplicationMessageBuilder().WithApplicationMessage(msg =>
            {
                msg.WithAtLeastOnceQoS();
                msg.WithPayload(BitConverter.GetBytes(98.6d));
                msg.WithTopic("inTopic");
            }).Build());


        }

        public async Task StartAsync(CancellationToken cancellationToken)
        {
            var optionsBuilder = new MqttServerOptionsBuilder()
                .WithDefaultEndpoint().WithDefaultEndpointPort(1883).WithConnectionValidator(
                    c =>
                    {
                        c.ReasonCode = MqttConnectReasonCode.Success;
                    }).WithSubscriptionInterceptor(
                    c =>
                    {
                        c.AcceptSubscription = true;
                    }).WithApplicationMessageInterceptor(
                    c =>
                    {
                        string s = string.Empty;

                        foreach (byte b in c.ApplicationMessage.Payload)
                        {
                            s += Convert.ToChar(b);
                        }

                        Console.WriteLine($"Sender: {c.ClientId} - Topic: {c.ApplicationMessage.Topic} - Message: {s}");

                        c.AcceptPublish = true;
                    });



            await _mqttServer.StartAsync(optionsBuilder.Build());
        }



        public async Task StopAsync(CancellationToken cancellationToken)
        {
            await _mqttServer.StopAsync();
        }
    }
}
