// -----------------------------------
// Controlling LEDs over the Internet
// -----------------------------------
#include "PietteTech_DHT.h"
// First, let's create our "shorthand" for the pins
// Same as in the Blink an LED example:
// led1 is D0, led2 is D7

#define DHT_DATA_PIN		D1

int led1 = D0;
int led2 = D7;

int mode = 0;

int uptime = 0;


PietteTech_DHT* s_dht = NULL;

static float s_temperature = 0;
static float s_humidity = 0;
static int s_temp = 0;
static int s_hum = 0;

// Last time, we only needed to declare pins in the setup function.
// This time, we are also going to register our Spark function

void dht_wrapper()
{
    s_dht->isrCallback();
}

static void init_devices();
static void temp_humidity_loop();

void setup()
{

   // Here's the pin configuration, same as last time
   pinMode(led1, OUTPUT);
   pinMode(led2, OUTPUT);

   // We are also going to declare a Spark.function so that we can turn the LED on and off from the cloud.
   Particle.function("led",ledToggle);
   Particle.variable("uptime",&uptime, INT); // https://docs.particle.io/reference/firmware/core/#particle-publish-
   // This is saying that when we ask the cloud for the function "led", it will employ the function ledToggle() from this app.

	Particle.variable("temperature", &s_temp, INT);
	Particle.variable("humidity", &s_hum, INT);

   // For good measure, let's also make sure both LEDs are off when we start:
   digitalWrite(led1, LOW);
   digitalWrite(led2, LOW);


	init_devices();
}


// Last time, we wanted to continously blink the LED on and off
// Since we're waiting for input through the cloud this time,
// we don't actually need to put anything in the loop

void loop()
{
    if (mode == 0)
    {
        digitalWrite(led1, LOW);
        digitalWrite(led2, LOW);
    }
    else if (mode == 1)
    {
        digitalWrite(led1, HIGH);
        digitalWrite(led2, HIGH);
        delay(1000);
        digitalWrite(led1, LOW);
        digitalWrite(led2, LOW);
        delay(1000);
    }
    else if (mode == 2)
    {
        digitalWrite(led1, HIGH);
        digitalWrite(led2, LOW);
        delay(300);
        digitalWrite(led1, LOW);
        digitalWrite(led2, HIGH);
        delay(300);
    }
    else
    {
        digitalWrite(led1, HIGH);
        digitalWrite(led2, HIGH);
    }
    uptime = millis() / 1000;
    if (uptime % 60 == 0)
    {
        Particle.publish("alive", String(uptime), 20); // https://console.particle.io/events
	Particle.publish("temperature", String(s_temperature), 20);
	Particle.publish("humidity", String(s_humidity), 20);
        delay(1000);
    }

    temp_humidity_loop();
    delay(1000);
   // Nothing to do here
}

// We're going to have a super cool function now that gets called when a matching API request is sent
// This is the ledToggle function we registered to the "led" Spark.function earlier.


int ledToggle(String command) {
    /* Spark.functions always take a string as an argument and return an integer.
    Since we can pass a string, it means that we can give the program commands on how the function should be used.
    In this case, telling the function "on" will turn the LED on and telling it "off" will turn the LED off.
    Then, the function returns a value to us to let us know what happened.
    In this case, it will return 1 for the LEDs turning on, 0 for the LEDs turning off,
    and -1 if we received a totally bogus command that didn't do anything to the LEDs.
    */

    if (command == "0")
    {
        mode = 0;
    }
    else if (command == "1")
    {
        mode = 1;
    }
    else if (command == "2")
    {
        mode = 2;
    }
    else
    {
        mode = 3;
    }
    
    Particle.publish("mode", String(mode), 10);
    
    return mode;
}

static void init_devices()
{
	s_dht = new PietteTech_DHT(DHT_DATA_PIN, DHT22, dht_wrapper);
}

static void temp_humidity_loop()
{
	int result = s_dht->acquireAndWait(2000);

	if (result != DHTLIB_OK)
	{
		delete s_dht;
		s_dht = new PietteTech_DHT(DHT_DATA_PIN, DHT22, dht_wrapper);
		switch (result)
		{
			case DHTLIB_OK:
				Serial.println("OK");
				break;
			case DHTLIB_ERROR_CHECKSUM:
				Serial.println("Error\n\r\tChecksum error");
				break;
			case DHTLIB_ERROR_ISR_TIMEOUT:
				Serial.println("Error\n\r\tISR time out error");
				break;
			case DHTLIB_ERROR_RESPONSE_TIMEOUT:
				Serial.println("Error\n\r\tResponse time out error");
				break;
			case DHTLIB_ERROR_DATA_TIMEOUT:
				Serial.println("Error\n\r\tData time out error");
				break;
			case DHTLIB_ERROR_ACQUIRING:
				Serial.println("Error\n\r\tAcquiring");
				break;
			case DHTLIB_ERROR_DELTA:
				Serial.println("Error\n\r\tDelta time too small");
				break;
			case DHTLIB_ERROR_NOTSTARTED:
				Serial.println("Error\n\r\tNot started");
				break;
			default:
				Serial.println("Unknown error");
				break;
		}
		return;
	}

	s_temperature = s_dht->getCelsius();
	s_humidity = s_dht->getHumidity();
	s_temp = (int)s_temperature;
	s_hum = (int)s_humidity;
}
