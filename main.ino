#include "PietteTech_DHT.h"

// For thingspeak stuff - https://www.hackster.io/sentient-things/thingspeak-particle-photon-using-webhooks-dbd96c
// Some other input - http://isetegija.ee/esp8266-wifi-mooduli-programmeerimine-salvestame-andmed-pilve/

#define DHT_DATA_PIN		D1
#define DHT3_DATA_PIN		D2

// led1 is D0, led2 is D7
int led1 = D0;
int led2 = D7;

int mode = 0;

int uptime = 0;


PietteTech_DHT* s_dht = NULL;
PietteTech_DHT* s_dhtIn = NULL;
PietteTech_DHT* s_dht3 = NULL;

static int s_lastUpdate = 0;
static int s_updateIntervalMin = 60;

static float s_temperature = 0;
static float s_humidity = 0;
static int s_temp = 0;
static int s_hum = 0;

static float s_tempIn = 0;
static float s_humIn = 0;
static int s_tempIntIn = 0;
static int s_humIntIn = 0;
int relayOn = 0;

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

struct DhtParticleInfo
{
	String status;
	int temp;
	int humidity;
};

struct DhtMeasurement
{
	int result;
	float temp;
	float humidity;
};

static struct DhtParticleInfo s_dht3ParticleInfo;

// Last time, we only needed to declare pins in the setup function.
// This time, we are also going to register our Spark function

void dht_wrapper()
{
	if (s_dht != NULL)
		s_dht->isrCallback();
}

void dhtIn_isr()
{
	if (s_dhtIn != NULL)
		s_dhtIn->isrCallback();
}

void dht3_isr()
{
	if (s_dht3 != NULL)
		s_dht3->isrCallback();
}

static void init_devices();
static void temp_humidity_loop();
static DhtParticleInfo convertToParticleInfo(DhtMeasurement* measurement);
static DhtMeasurement doMeasurementOf(PietteTech_DHT* device);

int intervalSet(String command)
{
	long interval = command.toInt();
	if (interval < 1 || interval > 1440)
		return -1;

	s_updateIntervalMin = (int)interval; 
	return s_updateIntervalMin;
}

void setup()
{

	// Here's the pin configuration, same as last time
	pinMode(led1, OUTPUT);
	pinMode(led2, OUTPUT);

	// We are also going to declare a Spark.function so that we can turn the LED on and off from the cloud.
	Particle.function("relee", ledToggle);
	Particle.function("interval", intervalSet);
	Particle.variable("uptime",&uptime, INT); // https://docs.particle.io/reference/firmware/core/#particle-publish-
	// This is saying that when we ask the cloud for the function "led", it will employ the function ledToggle() from this app.

	Particle.variable("temp_out", &s_temp, INT);
	Particle.variable("hum_out", &s_hum, INT);

	Particle.variable("temp_in", &s_tempIntIn, INT);
	Particle.variable("hum_in", &s_humIntIn, INT);

	Particle.variable("DHT3_status", &s_dht3ParticleInfo.status, STRING);
	Particle.variable("temp_3", &s_dht3ParticleInfo.temp, INT);
	Particle.variable("humidity_3", &s_dht3ParticleInfo.humidity, INT);

	Particle.variable("interval", &s_updateIntervalMin, INT);
	Particle.variable("relee", &relayOn, INT);

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
		relayOn = 0;
	}
	else if (mode == 1)
	{
		relayOn = 1;
		digitalWrite(led1, HIGH);
		digitalWrite(led2, HIGH);
	}
	
	int timeNow = Time.now();
	if (timeNow - s_lastUpdate >= s_updateIntervalMin * 60)
	{
		// Device data update.
		temp_humidity_loop();
		tempHumLoopIn();
		DhtMeasurement dht3Measurement = doMeasurementOf(s_dht3);
		s_dht3ParticleInfo = convertToParticleInfo(&dht3Measurement);

		Particle.publish("temp_out", String(s_temperature), 20, PRIVATE);
		Particle.publish("thingSpeakWrite_kolu", "{ \"1\": \"" + String(s_temperature) +
											   "\", \"2\": \"" + String(s_tempIn) + 
											   "\", \"3\": \"" + String(s_hum) +
											   "\", \"4\": \"" + String(s_humIn) +
											   "\", \"5\": \"" + String(relayOn) +
											   "\", \"k\": \"6JSVCMFGRV4O9OQH\" }", 60, PRIVATE);	
		s_lastUpdate = timeNow;
	}

	if (Time.second() == 0)
	{
		temp_humidity_loop();
		tempHumLoopIn();
		DhtMeasurement dht3Measurement = doMeasurementOf(s_dht3);
		s_dht3ParticleInfo = convertToParticleInfo(&dht3Measurement);
	}

	uptime = millis() / 1000;
	delay(1000);
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
    
    Particle.publish("mode", String(mode), 10, PRIVATE);
    
    return mode;
}

static void init_devices()
{
	s_dht = new PietteTech_DHT(DHT_DATA_PIN, DHT22, dht_wrapper);
	s_dhtIn = new PietteTech_DHT(D3, DHT22, dhtIn_isr);
	s_dht3 = new PietteTech_DHT(DHT3_DATA_PIN, DHT22, dht3_isr);
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

static void tempHumLoopIn()
{
	int result = s_dhtIn->acquireAndWait(2000);

	if (result != DHTLIB_OK)
	{
		delete s_dhtIn;
		s_dhtIn = new PietteTech_DHT(D3, DHT22, dhtIn_isr);
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

	s_tempIn = s_dhtIn->getCelsius();
	s_humIn = s_dhtIn->getHumidity();
	s_tempIntIn = (int)s_tempIn;
	s_humIntIn = (int)s_humIn;
}

static DhtMeasurement doMeasurementOf(PietteTech_DHT* device)
{
	DhtMeasurement m{};
	m.result = device->acquireAndWait(2000);

	if (m.result != DHTLIB_OK)
		return m;

	m.temp = device->getCelsius();
	m.humidity = device->getHumidity();
	return m;
}

static String dhtResultToString(int result)
{
	switch (result)
	{
		case DHTLIB_OK:
			return String("OK");
		case DHTLIB_ERROR_CHECKSUM:
			return String("Checksum error");
		case DHTLIB_ERROR_ISR_TIMEOUT:
			return String("ISR timeout");
		case DHTLIB_ERROR_RESPONSE_TIMEOUT:
			return String("Response timeout");
		case DHTLIB_ERROR_DATA_TIMEOUT:
			return String("Data timeout");
		case DHTLIB_ERROR_ACQUIRING:
			return String("Acquire error");
		case DHTLIB_ERROR_DELTA:
			return String("Delta error");
		case DHTLIB_ERROR_NOTSTARTED:
			return String("Not started");
		default:
			return String("Unknown status of " + String(result));
	}
}

static DhtParticleInfo convertToParticleInfo(DhtMeasurement* measurement)
{
	DhtParticleInfo info{};

	info.status = dhtResultToString(measurement->result);
	info.temp = (int)measurement->temp;
	info.humidity = (int)measurement->humidity;

	return info;
}
