#include <Adafruit_NeoPixel.h>
#include <IRremote.h>

//IR
IRsend irsend;
int number = 0;
const int sendingPin = 3;

//LEDs
int LEDs[8]{ 15,14,13,12,11,10,9,8 };

//Neopixel 
#define PIN        17
#define NUMPIXELS  1
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//Serial
long intRecieved = 0;
byte buffer[4] = {0,0,0};

//Temperature
const long interval = 1000;           //interval of the temperaturechecks
const int checkingNumber = 5;         //number of checks
const int ntc = A5;                   //analogRead-pin
const int ntcNominal = 15000;         //resistance at e.g 25°C
const int tempNominal = 25;           //nominaltemperature (e.g. 25°C)
const int bCoefficient = 3528;        //Beta Coefficient(B25 from the datasheet of the NTC)
const int serialResistor = 10000;     //value of the serial resistor
									  
int checks[checkingNumber];           //array variable for averaging the temperatur
float averageValue = 0;               //variable for averaging the temperature
float temp;                           //variable for calculation (Steinhart)
float lastTemp;
unsigned long lastTempCheck = 0;      //saves time of last temperaturemeasurement
int lastTempUpdate = 0;

void setup()
{
	Serial.begin(9600);

 //LEDs
	for (int i = 0; i < 8; i++) {
		pinMode(LEDs[i], OUTPUT);
	}
 //NTC
	pinMode(ntc, INPUT);            
 //Neopixel
	pixels.begin();
	pinMode(17, OUTPUT);
}

void loop()
{
	serialInterface();
	temperature();
}

void serialInterface() {
	if (Serial.available() > 0) {
		intRecieved = Serial.read();		
		IntegerToBytes(intRecieved, buffer);
		
		switch (buffer[0]) {
		// IR - Remote
			case 1:
		    IRremote();
		    break;
		//LED-Light
			case 2:
			lightLED();
			break;
		//RGB
		    case 3:
			neoPixel();
			break;
		}
	}

 //send temperature to FP
	int timeNow = millis();
	if (timeNow - lastTempUpdate > 500) {
		Serial.print(temp);
	}
}

void IntegerToBytes(long val, byte b[4]) {
	b[0] = (byte)((val >> 24) & 0xff);
	b[1] = (byte)((val >> 16) & 0xff);
	b[2] = (byte)((val >> 8) & 0xff);
	b[3] = (byte)(val & 0xff);
}

//lights up the 8 LEDs attached to the GPIOs
void lightLED() {
	for (int i = 0; i <= 7; i++) {
		digitalWrite(LEDs[i], bitRead(buffer[1], i));
	}
}

//lights up the Neopixel 
void neoPixel() {
	byte r = buffer[1];
	byte g = buffer[2];
	byte b = buffer[3];
	pixels.setPixelColor(0, pixels.Color(r, g, b)); 
	pixels.show(); 
}

//sends signal via IR-LED
void IRremote() {
	//sending selected hex-code
	switch (buffer[2]) {
	// ON
	case 1:
		irsend.sendNEC(0xFFB04F, 32);
		Serial.println("ON");
		break;
	// OFF
	case 2:
		irsend.sendNEC(0xFFF807, 32);
		Serial.println("OFF");
		break;
	// RED
	case 3:
		irsend.sendNEC(0xFF9867, 32);
		Serial.println("RED");
		break;
	// GREEN
	case 4:
		irsend.sendNEC(0xFFD827, 32);
		Serial.println("GREEN");
		break;
	// BLUE
	case 5:
		irsend.sendNEC(0xFF8877, 32);
		Serial.println("BLUE");
		break;
	}
}

//checking the NTC and sending temperature to the FP
void temperature() {
	int timeNow = millis();
	if (timeNow - lastTempCheck > 100) {
		//N readouts with a short delay
		for (int i = 0; i < checkingNumber; i++)
		{
			checks[i] = analogRead(ntc);
			delay(10);
		}

//averaging of the measured values
		averageValue = 0;
		for (int i = 0; i < checkingNumber; i++)
		{
			averageValue += checks[i];
		}
		averageValue /= checkingNumber;

//transform measured value (voltage) into resistance
		averageValue = 1023 / averageValue - 1;
		averageValue = serialResistor / averageValue;

//convert measured results 
		temp = averageValue / ntcNominal;     // (R/Ro)
		temp = log(temp);                     // ln(R/Ro)
		temp /= bCoefficient;                 // 1/B * ln(R/Ro)
		temp += 1.0 / (tempNominal + 273.15); // + (1/To)
		temp = 1.0 / temp;                    // invert
		temp -= 273.15;
		lastTempCheck = timeNow;
		lastTemp = temp;
	}
}