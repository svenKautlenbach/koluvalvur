#include "Adafruit_MAX6675.h"
#include "PietteTech_DHT.h"

// For thingspeak stuff - https://www.hackster.io/sentient-things/thingspeak-particle-photon-using-webhooks-dbd96c
// Some other input - http://isetegija.ee/esp8266-wifi-mooduli-programmeerimine-salvestame-andmed-pilve/

// led1 is D0, led2 is D7
int led1 = D0;
int led2 = D7;

int mode = 0;
int s_uptime = 0;

const int thermoCLK = A3;
const int thermoCS = A2;
const int thermoDO = A4;
static Adafruit_MAX6675* s_thermocouple{NULL};
static double s_thermocouple_celsius{};

#define DHT22_DEVICE_COUNT 5
const static uint8_t DHT22_PINS[DHT22_DEVICE_COUNT]{D1, D2, D3, D4, D5};
static PietteTech_DHT* s_dhtDevices[DHT22_DEVICE_COUNT]{NULL, NULL, NULL, NULL, NULL};

static int s_lastUpdate = 0;
static int s_updateIntervalMin = 15;
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

static struct DhtParticleInfo s_dhtParticleInfos[DHT22_DEVICE_COUNT]{};

static volatile int activeDht22DeviceIndex{0};
void dht22_isr()
{
	if (s_dhtDevices[activeDht22DeviceIndex] != NULL)
		s_dhtDevices[activeDht22DeviceIndex]->isrCallback();
}

static void initDevices();
static DhtParticleInfo convertToParticleInfo(const DhtMeasurement* measurement);
static DhtMeasurement doMeasurementOf(PietteTech_DHT* device);

int intervalSet(String command)
{
	long interval = command.toInt();
	if (interval < 1 || interval > 1440)
		return -1;

	s_updateIntervalMin = (int)interval; 
	return s_updateIntervalMin;
}

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));

void setup()
{
	pinMode(led1, OUTPUT);
	pinMode(led2, OUTPUT);

	Particle.function("relee", ledToggle);
	Particle.function("interval_minutes", intervalSet);
	Particle.variable("uptime_seconds", &s_uptime, INT); // https://docs.particle.io/reference/firmware/core/#particle-publish-

	Particle.variable("thermocouple", s_thermocouple_celsius);

	for (int i = 0; i < DHT22_DEVICE_COUNT; i++) {
		Particle.variable(String::format("temp%d", i+1), s_dhtParticleInfos[i].temp);
		Particle.variable(String::format("niiskus%d", i+1), s_dhtParticleInfos[i].humidity);
		Particle.variable(String::format("staatus%d", i+1), s_dhtParticleInfos[i].status);
	}

	Particle.variable("interval_minutes", &s_updateIntervalMin, INT);
	Particle.variable("relee", &s_relayOn, INT);

	digitalWrite(led1, LOW);
	digitalWrite(led2, LOW);
}

static void updateMeasurements() {
	for (int i = 0; i < DHT22_DEVICE_COUNT; i++) {
		activeDht22DeviceIndex = i;
		DhtMeasurement m = doMeasurementOf(s_dhtDevices[i]);
		s_dhtParticleInfos[i] = convertToParticleInfo(&m);
	}
   
	s_thermocouple_celsius = s_thermocouple->readCelsius();
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
		// This is nuisance, but we want to dynamically respond to new pin assemblies.
		initDevices();
		updateMeasurements();
		freeDht22Devices();

		Particle.publish("thingSpeakWrite_kolu", "{ \"1\": \"" + String(s_dhtParticleInfos[0].temp) +
											   "\", \"2\": \"" + String(s_dhtParticleInfos[0].humidity) + 
											   "\", \"3\": \"" + String(s_dhtParticleInfos[1].temp) +
											   "\", \"4\": \"" + String(s_dhtParticleInfos[2].temp) +
											   "\", \"5\": \"" + String(s_dhtParticleInfos[3].temp) +
											   "\", \"6\": \"" + String((int)s_thermocouple_celsius) +
											   "\", \"k\": \"6JSVCMFGRV4O9OQH\" }", 60, PRIVATE);	
		s_lastUpdate = timeNow;
	}

	if (Time.second() == 0)
	{
		initDevices();
		updateMeasurements();
		freeDht22Devices();
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
	for (int i = 0; i < DHT22_DEVICE_COUNT; i++) {
		activeDht22DeviceIndex = i;
		s_dhtDevices[i] = new PietteTech_DHT(DHT22_PINS[i], DHT22, dht22_isr);
	}

    s_thermocouple = new Adafruit_MAX6675(thermoCLK, thermoCS, thermoDO);
}

static void freeDht22Devices()
{
	for (int i = 0; i < DHT22_DEVICE_COUNT; i++) {
		activeDht22DeviceIndex = i;
		if (s_dhtDevices[i] != NULL)
			delete s_dhtDevices[i];
	}
}

static DhtMeasurement doMeasurementOf(PietteTech_DHT* device)
{
	DhtMeasurement m{};
	m.result = device->acquireAndWait(2500);

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

static DhtParticleInfo convertToParticleInfo(const DhtMeasurement* measurement)
{
	DhtParticleInfo info{};

	info.status = dhtResultToString(measurement->result);
	info.temp = (int)measurement->temp;
	info.humidity = (int)measurement->humidity;

	return info;
}
