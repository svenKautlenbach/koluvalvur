#include "relay_guard.h"

#include "Particle.h"

extern bool relayOn;
extern int led1;
extern int led2;

static bool setRelayOn = false;

void setRelay()
{
	if (setRelayOn)
	{
		digitalWrite(led1, HIGH);
		digitalWrite(led2, HIGH);
	}
	else
	{
		digitalWrite(led1, LOW);
		digitalWrite(led2, LOW);
	}
}

static Timer offlineTimer(60 * 1000, setRelay, true);

void checkRelay()
{
	if (!relayOn)
	{
		offlineTimer.stop();
		return;
	}

	if (Particle.connected())
	{
		setRelayOn = true;
		offlineTimer.reset();
		offlineTimer.start();
	}
	else
	{
		setRelayOn = false;
		offlineTimer.reset();
		offlineTimer.start();	
	}
}
