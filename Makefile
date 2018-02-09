all: $(clean)
	particle compile photon main.ino am2302/firmware/PietteTech_DHT.* --saveTo firmware.bin

clean:
	rm *.bin

dev-list:
	particle list

flash:
	particle flash Koluvalvur firmware.bin

serial:
	particle serial monitor

