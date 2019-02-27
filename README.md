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

## Koncept
Tanken med MMRC är att skapa ett flexibelt och decentraliserat sätt att styra modelljärnvägar. Grunden i konceptet är att man har en central meddelandeserver (s.k. MQTT-broker) och olika typer av klienter som sköter olika funktioner på modelljärnvägen. Klienterna kan sedan kommunicera med varandra via meddelandeservern för att både styra och bli styrda av andra klienter.

Genom att använda trådlöst nätverk och små, billiga kretskortsdatorer blir systemet väldigt flexibelt. Det passar speciellt bra på modelljärnvägsmoduler som kan vara placerade på olika platser i en bana, men ändå ska kunna styras och övervakas centralt. 

## Meddelandeserver
En viktig del i MMRC är den centrala meddelandeservern. Till den bör man använda en lite mer kapabel dator och ett förslag är att använda Raspberry Pi med t.ex. [Mosquitto](http://mosquitto.org/) installerat.

## Klienter
Det finns inga specifika typer av klienter i detta system. Det är upp till den som programmerar klienten att bestämma vad den ska kunna göra. Denna test-klient kan exempelvis bara tända och släcka en lysdiod, men mer troliga funktioner är att styra växlar, signaler och känna av tryckknappar.

Det finns inget som bestämmer hur många uppgifter en klient utför. På en liten modul kanske en klient sköter både växel och signaler, medan en större station kanske har olika klienter för växlar, signaler och ställverket. Man kan också tänka sig en signalmodul som bara har in- och utfartssignal styrd av en egen klient.

## CLEES-kompatibel
Idéerna till MMRC har jag haft länge, men det var först när [CLEES](https://github.com/TomasLan/CLEES/) dök upp som jag fick inspiration nog att ta tag i mina egna idéer på allvar. Skillnaderna mellan MMRC och CLEES är små; i grunden är det samma tankar. Största skillnaden är att CLEES koncentrerar sig till på en enda typ av klient som då ska klara av allt.

Tack vare likheterna finns möjligheten att MMRC kan samexistera med CLEES och styra/styras av CLEES-klienter. Det finns en inställning i detta testprogram som gör att klienten blir CLEES-kompatibel.

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
