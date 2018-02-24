#include <Adafruit_NeoPixel.h>

#include <IRremote.h>

//IR
IRsend irsend;
int number = 0;
const int sendingPin = 3;

//LEDs
int LEDs[8]{ 15,14,13,12,11,10,9,8 };

//Neopixel 
#define PIN 17
uint16_t NUMBER = 1;
uint16_t PIXEL = 0;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER, PIN, NEO_GRB + NEO_KHZ800);

//Serial
byte buffer[4] = {0,0,0,0};
int starttime = 0;

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
	strip.begin();
	strip.setPixelColor(0, 255, 0, 0);
	strip.show();
	delay(1000);
	strip.setPixelColor(0, 0, 0, 0);
	strip.show();
}

void loop() {
	serialInterface();
	control();
	temperature();
	sendTemp();
}

 void serialInterface() {
	if (Serial.available() > 0) {
		while (Serial.available() < 4 && ((millis() - starttime) < 1000)) {} // Wait 'till there are 4 Bytes waiting
		for (int n = 0; n < 4; n++) { buffer[n] = Serial.read(); }		
	}
}

 void sendTemp() {
	 //send temperature to FP
	 int timeNow = millis();
	 if (timeNow - lastTempUpdate > 10000) {
		 Serial.println((int)temp);
		 lastTempUpdate = millis();
	 }
}

void control() {
	if (buffer[0] == 0) {
		return;
	}

	//IRremote
	if (buffer[0] == 1) {
		//IRremote();
		return;
	}

	//LEDs
	if (buffer[0] == 2) {
		lightLED();
		return;
	}

	//RGB
	if (buffer[0] != 3) {
		return;
	}

	uint8_t r = buffer[1];
	uint8_t g = buffer[2];
	uint8_t b = buffer[3];
	if (r < 20) { r = 0; };
	if (g < 20) { g = 0; };
	if (b < 20) { b = 0; };
	strip.setPixelColor(PIXEL, r, g, b);
	delay(10);
	strip.show();
}

//lights up the 8 LEDs attached to the GPIOs
void lightLED() {
	for (int i = 0; i <= 7; i++) {
		digitalWrite(LEDs[i], bitRead(buffer[1], i));
	}
}

//sends signal via IR-LED
void IRremote() {
	switch (buffer[1]) {
	// ON
	case 1:
		irsend.sendNEC(0xFFB04F, 32); //sending selected hex-code
		break;
	// OFF
	case 2:
		irsend.sendNEC(0xFFF807, 32);
		break;
	// RED
	case 3:
		irsend.sendNEC(0xFF9867, 32);
		break;
	// GREEN
	case 4:
		irsend.sendNEC(0xFFD827, 32);
		break;
	// BLUE
	case 5:
		irsend.sendNEC(0xFF8877, 32);
		break;
	}
}

//checking the NTC and sending temperature to the FP
void temperature() {
	int timeNow = millis();
	if (timeNow - lastTempCheck > 100) {
		for (int i = 0; i < checkingNumber; i++) //N readouts with a short delay
		{
			checks[i] = analogRead(ntc);
			delay(10);
		}

//averaging of the measured values
		averageValue = 0;
		for (int i = 0; i < checkingNumber; i++) {
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