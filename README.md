````java
 __  __ __  __ ___  ___
|  \/  |  \/  | _ \/ __|
| |\/| | |\/| |   / (__
|_|  |_|_|  |_|_|_\\___| - start
````

Modern Model Railroad Control - Testing software for ESP866 wifi capable Arduino and clones.

## Översikt
Detta program är gjort för att testa wifi och MQTT på Arduino-kompatibla kretskortsdatorer. Via MQTT-meddelanden kan du tända och släcka Arduinons/klonens inbyggda lysdiod.

## Förutsättningar
För att kunna använda programmet behöver du följande:

- Trådlöst nätverk (Wifi)
- En MQTT-server (t.ex. [Mosquitto](http://mosquitto.org/)) ansluten till trådlösa nätverket
- En Arduino (eller motsvarande) med inbyggd eller inkopplad ESP8266-krets för trådlös nätverkskommunikation
- Dator med Arduino IDE
- MQTT-klient, exempelvis [Linear MQTT Dashboard](https://play.google.com/store/apps/details?id=com.ravendmaster.linearmqttdashboard) på en Android-telefon
- Kabel mellan dator och Arduino (vanligen USB micro B)

Programmet är testat på [Wemos D1 mini](https://wiki.wemos.cc/products:retired:d1_mini_v2.2.0) och [NodeMCU](https://www.nodemcu.com/index_en.html) men det bör kunna fungera på vilken Arduino/klon som helst som använder en ESP8266-krets för trådlös kommunikation.
Wemos D1 mini finns att köpa hos exempelvis svenska [Lawicel](https://www.lawicel-shop.se/esp8266-esp-12f-d1-mini-with-ch340) för 49 kr, precis som [NodeMCU](https://www.lawicel-shop.se/nodemcu-v3-with-esp-12e-ch340) (2019-02-15).

## Gör så här
Det krävs ganska många steg för att få igång MMRC på en kretskortsdator. Inget är speciellt svårt och det mesta behöver bara göras en gång, men det är inget för nybörjaren. Dessa instruktioner är väldigt översiktliga, mer detaljerade kanske kommer med tiden...

**En gång:**

- Installera en MQTT-broker på en dator ansluten till nätverket
- Installera Arduino IDE på valfri dator
- I Arduino IDE måste två olika "libraries" installeras: EspMQTTClient och PubSubClient
- För vissa kretskortsdatorer måste man på Windows installera drivrutiner för en USB-krets som heter CH340
- Ladda hem MMRC-test från Github & packa upp i Arduino IDEs skissbok (projektmapp)

**För varje klient:**

- Leta reda på MMRCsettings.h och anpassa den för varje klient
- Starta Seriell Monitor i Arduino IDE
- Ladda in programmet i Arduino IDE, koppla in kretskortsdator och prova ladda ner programmet

Har allt gått bra så ska du nu i Seriell Monitor kunna se att kretskortsdator kopplar upp sig mot nätverk och MQTT samt börjar prenumerera på ett ämne. Prova sen att från en annan dator/telefon publicera en etta (1) eller nolla (0) till det ämnet, så ska lysdioden på kretskortsdator tändas och släckas.

Varje gång lysdioden tänds eller släck, bekräftar programmet det genom att publicera en etta eller nolla till "mmrc/[MODULE]/[OBJECT]/[CATEGORY]/turnout/rpt" eller "clees/[CATEGORY]/[MODULE]/turnout/rpt/[OBJECT]" beroende på inställning. Prenumerera på ämnet i en dator/telefon för att se denna status ändras.
