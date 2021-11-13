#include "PietteTech_DHT.h"

// For thingspeak stuff - https://www.hackster.io/sentient-things/thingspeak-particle-photon-using-webhooks-dbd96c
// Some other input - http://isetegija.ee/esp8266-wifi-mooduli-programmeerimine-salvestame-andmed-pilve/

#define DHT1_DATA_PIN		D1
#define DHT2_DATA_PIN		D2
#define DHT3_DATA_PIN		D3
#define DHT4_DATA_PIN		D4
#define DHT5_DATA_PIN		D5

// led1 is D0, led2 is D7
int led1 = D0;
int led2 = D7;

int mode = 0;
int s_uptime = 0;

// Use primary serial over USB interface for logging output
static SerialLogHandler s_logHandler;

static PietteTech_DHT* s_dht1 = NULL;
static PietteTech_DHT* s_dht2 = NULL;
static PietteTech_DHT* s_dht3 = NULL;
static PietteTech_DHT* s_dht4 = NULL;
static PietteTech_DHT* s_dht5 = NULL;

static int s_lastUpdate = 0;
static int s_updateIntervalMin = 60;
static int s_relayOn = 0;

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

static struct DhtParticleInfo s_dht1ParticleInfo;
static struct DhtParticleInfo s_dht2ParticleInfo;
static struct DhtParticleInfo s_dht3ParticleInfo;
static struct DhtParticleInfo s_dht4ParticleInfo;
static struct DhtParticleInfo s_dht5ParticleInfo;

void dht1_isr()
{
	if (s_dht1 != NULL)
		s_dht1->isrCallback();
}

void dht2_isr()
{
	if (s_dht2 != NULL)
		s_dht2->isrCallback();
}

void dht3_isr()
{
	if (s_dht3 != NULL)
		s_dht3->isrCallback();
}

void dht4_isr()
{
	if (s_dht4 != NULL)
		s_dht4->isrCallback();
}

void dht5_isr()
{
	if (s_dht5 != NULL)
		s_dht5->isrCallback();
}

static void initDevices();
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
	Log.info("System version: %s", (const char*)System.version());

	pinMode(led1, OUTPUT);
	pinMode(led2, OUTPUT);

	Particle.function("relee", ledToggle);
	Particle.function("interval_minutes", intervalSet);
	Particle.variable("uptime_seconds", &s_uptime, INT); // https://docs.particle.io/reference/firmware/core/#particle-publish-

	Particle.variable("temp1", &s_dht1ParticleInfo.temp, INT);
	Particle.variable("niiskus1", &s_dht1ParticleInfo.humidity, INT);
	Particle.variable("staatus1", &s_dht1ParticleInfo.status, STRING);

	Particle.variable("temp2", &s_dht2ParticleInfo.temp, INT);
	Particle.variable("niiskus2", &s_dht2ParticleInfo.humidity, INT);
	Particle.variable("staatus2", &s_dht2ParticleInfo.status, STRING);

	Particle.variable("temp3", &s_dht3ParticleInfo.temp, INT);
	Particle.variable("niiskus3", &s_dht3ParticleInfo.humidity, INT);
	Particle.variable("staatus3", &s_dht3ParticleInfo.status, STRING);

	Particle.variable("temp4", &s_dht4ParticleInfo.temp, INT);
	Particle.variable("niiskus4", &s_dht4ParticleInfo.humidity, INT);
	Particle.variable("staatus4", &s_dht4ParticleInfo.status, STRING);
	
	Particle.variable("temp5", &s_dht5ParticleInfo.temp, INT);
	Particle.variable("niiskus5", &s_dht5ParticleInfo.humidity, INT);
	Particle.variable("staatus5", &s_dht5ParticleInfo.status, STRING);

	Particle.variable("interval_minutes", &s_updateIntervalMin, INT);
	Particle.variable("relee", &s_relayOn, INT);

	digitalWrite(led1, LOW);
	digitalWrite(led2, LOW);

	initDevices();
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
		s_relayOn = 0;
	}
	else if (mode == 1)
	{
		s_relayOn = 1;
		digitalWrite(led1, HIGH);
		digitalWrite(led2, HIGH);
	}
	
	int timeNow = Time.now();
	if (timeNow - s_lastUpdate >= s_updateIntervalMin * 60)
	{
		// Device data update.
		DhtMeasurement dht1Measurement = doMeasurementOf(s_dht1);
		s_dht1ParticleInfo = convertToParticleInfo(&dht1Measurement);
		DhtMeasurement dht2Measurement = doMeasurementOf(s_dht2);
		s_dht2ParticleInfo = convertToParticleInfo(&dht2Measurement);
		DhtMeasurement dht3Measurement = doMeasurementOf(s_dht3);
		s_dht3ParticleInfo = convertToParticleInfo(&dht3Measurement);
		DhtMeasurement dht4Measurement = doMeasurementOf(s_dht4);
		s_dht4ParticleInfo = convertToParticleInfo(&dht4Measurement);
		DhtMeasurement dht5Measurement = doMeasurementOf(s_dht5);
		s_dht5ParticleInfo = convertToParticleInfo(&dht5Measurement);

		Particle.publish("thingSpeakWrite_kolu", "{ \"1\": \"" + String(s_dht1ParticleInfo.temp) +
											   "\", \"2\": \"" + String(s_dht2ParticleInfo.temp) + 
											   "\", \"3\": \"" + String(s_dht1ParticleInfo.humidity) +
											   "\", \"4\": \"" + String(s_dht2ParticleInfo.humidity) +
											   "\", \"5\": \"" + String(s_relayOn) +
											   "\", \"k\": \"6JSVCMFGRV4O9OQH\" }", 60, PRIVATE);	
		s_lastUpdate = timeNow;
	}

	if (Time.second() == 0)
	{
		DhtMeasurement dht1Measurement = doMeasurementOf(s_dht1);
		s_dht1ParticleInfo = convertToParticleInfo(&dht1Measurement);
		DhtMeasurement dht2Measurement = doMeasurementOf(s_dht2);
		s_dht2ParticleInfo = convertToParticleInfo(&dht2Measurement);
		DhtMeasurement dht3Measurement = doMeasurementOf(s_dht3);
		s_dht3ParticleInfo = convertToParticleInfo(&dht3Measurement);
		DhtMeasurement dht4Measurement = doMeasurementOf(s_dht4);
		s_dht4ParticleInfo = convertToParticleInfo(&dht4Measurement);
		DhtMeasurement dht5Measurement = doMeasurementOf(s_dht5);
		s_dht5ParticleInfo = convertToParticleInfo(&dht5Measurement);
	}

	s_uptime = millis() / 1000;
	delay(1000);
}

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

static void initDevices()
{
	s_dht1 = new PietteTech_DHT(DHT1_DATA_PIN, DHT22, dht1_isr);
	s_dht2 = new PietteTech_DHT(DHT2_DATA_PIN, DHT22, dht2_isr);
	s_dht3 = new PietteTech_DHT(DHT3_DATA_PIN, DHT22, dht3_isr);
	s_dht4 = new PietteTech_DHT(DHT4_DATA_PIN, DHT22, dht4_isr);
	s_dht5 = new PietteTech_DHT(DHT5_DATA_PIN, DHT22, dht5_isr);
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
