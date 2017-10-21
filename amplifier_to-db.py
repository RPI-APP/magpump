import serial
import time
import relay

print "Amplifier Reader"
print "(C) 2017 Ted Berger"
print ""
print "Instructions (Windows):"
print " * Find the Arduino COM port using device manager"
print " * Type the COM port in below (ex COM6)"
print ""
print "Instructions (Linux)"
print " * Find the Arduino port by using ls in /dev/"
print " * Type the full port name below (ex /dev/ttyACM4)"
print ""
print "Instructions (All)"
print " * Press Ctrl+C to quit data collection"
print ""


port = raw_input("Pick a port: ")
ser = serial.Serial(port, 115200)

j=0
while True:
        chunk = str(ser.readline()).strip()
        try:
                data = chunk.split(",")
		
                if j%3 == 0:
                        print(data)
		
	


                g, d = data[0], data[1]
                relay.sendChunk(g,d)
		

        except:
                print "BAD_DATA"
		
        j+=1
