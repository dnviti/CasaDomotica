Casa Domotica Arduino MQTT + Xamarin Android/iOS + ASP.NET Core + C# MQTT Server + Unity3D Sinottico Casa

Obiettivo: Plastico + Simulazione 3D casa interattivi con input/output replicati ovunque con MQTT + SignalR

ToDo:
- Server (MQTT + ASP.NET API + Blazor)
    - Accettare connessioni solo se con autenticazione basic (user, pass)
    - Ricevere informazioni sensori (JSON) (bmp280(temp, press, alt), GY(pitch, yaw, tilt))
    - Inviare segnali di attuazione (JSON) (es. luce(on, off), motore continuo(on, off, giro orario, giro antiorario), relè(on, off)...)
    - Modello Generico (newtonsoft) MQTT segnale DA arduino (lettura sensori)
    - Memorizzare dati (sia DA arduino che VERSO arduino) su oggetto (mock db)
    - Imbastire il canale MQTT verso i vari client che dovranno consumare i dati (Xamarin, Blazor, Unity3D, Altre app .NET)
    - Imbastire endpoint HTTP(S) per chi non può consumare via MQTT
- Client
    - Preparare sottoscrizione MQTT


Plastico:
- Preparare progetto 2D planimetria
- Preparare 3D da prospetto 2D (con guide per cavi)
- Arredare in 3D
- Acquistare legno per muri, porte e tetto
- Stampare 3d ingranaggi per attuazione
- acquistare motori a basso voltaggio
- altro boh...
