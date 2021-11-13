all: $(clean)
	particle compile photon main.ino relay_guard.* am2302/firmware/PietteTech_DHT.* ParticlePhoton-MAX6675-master/firmware/Adafruit_MAX6675.* --saveTo firmware.bin

clean:
	rm *.bin

dev-list:
	particle list

flash:
	particle flash Koluvalvur firmware.bin

serial:
	particle serial monitor

