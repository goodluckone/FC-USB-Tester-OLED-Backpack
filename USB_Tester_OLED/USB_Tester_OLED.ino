/*************************************
USB Tester OLED Display
Created by: William Garrido
Created: 01/10/2013
This displays the current and voltage from the USB Tester to the OLED display using the OLED Backpack
Also sends data to serial port for data logging via Java app
Kits are aviaible at www.tindie.com and more info can be found on www.friedcircuits.us

Changelog by Edouard Lafargue

2013.04.11
- Autoscroll of graph

ToDo:
- Remove shunt voltage and load voltage display (are those really necessary ?)
- Autoscale of graph

-William Garrido
2013.08.17
-Removed SVolt and BVolt
-Moved Current\Volt under graph
-Extend graph width of display
-Use clearDisplay to speed up redraws
-Reduce logo display time
-Enabled button to cycle display speed
-Millis based delay
**************************************/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define OLED_DC 5
#define OLED_CS SS
#define OLED_CLK SCK
#define OLED_MOSI MOSI
#define OLED_RESET 9

const int LEDPIN = 13;
int ledWarn = 350; //Threshold in mA

const int maxMode = 4;

// Graph values:
int graph_MAX = 500;
// Graph area is from 0 to 128 (inclusive), 128 points altogether
#define GRAPH_MEMORY 128
int graph_Mem[GRAPH_MEMORY];
int ring_idx = 0; // graph_Mem is managed as a ring buffer.

//Button
const int btnPin = 10;

//Current Sensor
Adafruit_INA219 ina219;


int OLED_REFRESH_SPEED = 100; //Startup refresh delay
//Define refresh delay for each mode
const int speed0 = 0;
const int speed1 = 50;
const int speed2 = 100;
const int speed3 = 250;
const int speed4 = 500;

int graphX = 0; //Start of X for graph
int graphY = 0;  //y placement of graph

//Init OLED Display
Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

  
//FriedCircuits Logo for startup 
static unsigned char PROGMEM FriedCircuitsUSBTester[] = 
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x87, 0x87, 0xC0, 0x0C, 0x41, 0x86, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x41, 0x86, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x40, 0xCC, 0x70,
0xE1, 0xF0, 0xF0, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x0C, 0x40, 0xFC, 0xF8,
0xF3, 0xF9, 0xF9, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x0C, 0x40, 0x31, 0xCC,
0x33, 0x1B, 0x1D, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x18, 0xC0, 0x00, 0x00, 0x0C, 0x40, 0x01, 0x8C,
0x1A, 0x1B, 0x0D, 0x8C, 0x00, 0x07, 0xFF, 0xFF, 0xF8, 0xC0, 0x00, 0x00, 0x0C, 0x40, 0x01, 0x8C,
0x33, 0x1B, 0x99, 0x8C, 0x00, 0x1F, 0xFF, 0xFF, 0xF8, 0xC0, 0x00, 0x00, 0x0C, 0x40, 0x39, 0xFC,
0xF3, 0xF1, 0xF9, 0xF8, 0x00, 0x78, 0x00, 0x00, 0x1F, 0xC0, 0x00, 0x00, 0x0C, 0x40, 0xFC, 0xF8,
0xC0, 0xE0, 0xF0, 0x70, 0x01, 0xE0, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x0C, 0x40, 0xC6, 0x30,
0x00, 0x08, 0x00, 0x00, 0x0F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x40, 0xC6, 0x30,
0x00, 0x0C, 0x00, 0x00, 0x3E, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x40, 0xC6, 0x30,
0x00, 0x1E, 0x00, 0x00, 0xF0, 0x01, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x40, 0xFE, 0x30,
0xFE, 0x1F, 0x1F, 0xFF, 0xC0, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x40, 0x7C, 0x30,
0xFE, 0x3F, 0x9F, 0xC0, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0xBF, 0x8F, 0xBF, 0x1D, 0xF7, 0xE0, 0xF3, 0xBF, 0x06, 0x73, 0xBF, 0xF9, 0x83, 0x9C, 0x60,
0x00, 0xFF, 0xCF, 0xBF, 0xBD, 0xF7, 0xF9, 0xF7, 0xBF, 0x9E, 0xF3, 0xFF, 0xFF, 0x83, 0xDD, 0xF0,
0x00, 0xFF, 0xCE, 0x3F, 0xBD, 0xC7, 0xFB, 0xC7, 0xBF, 0xBE, 0xF3, 0xFB, 0xFF, 0x83, 0xDF, 0xE0,
0x00, 0xFF, 0xCF, 0xBF, 0xBD, 0xF7, 0xBB, 0x87, 0xBF, 0xB8, 0xF3, 0xF9, 0xCF, 0xC3, 0xDD, 0xF0,
0x00, 0xFF, 0xCF, 0xBF, 0xBD, 0xF7, 0xBF, 0x87, 0xBF, 0x78, 0xF7, 0x79, 0xC3, 0xC3, 0xDC, 0xF0,
0x02, 0xFF, 0xCE, 0x3F, 0xFD, 0xF7, 0xFB, 0xF7, 0xBF, 0xBE, 0x7F, 0x79, 0xC7, 0xDB, 0xFD, 0xF0,
0x06, 0xFF, 0xBE, 0x3D, 0xFD, 0xFF, 0xE3, 0xFF, 0xBB, 0xBF, 0x7F, 0x79, 0xC7, 0xB9, 0xF9, 0xE0,
0x03, 0x7F, 0x7E, 0x19, 0x99, 0xFF, 0x80, 0xFB, 0xB9, 0x8F, 0x3C, 0x39, 0xC6, 0x18, 0xF1, 0xC0,
0x03, 0xFC, 0x00, 0x00, 0x00, 0x7F, 0x07, 0xE0, 0x00, 0x00, 0x1C, 0x0C, 0x0C, 0x40, 0x18, 0x30
};


void setup()
{

  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH);
  
  pinMode(btnPin, INPUT);
  
  Serial.begin(115200);
  Serial.println("USB Tester OLED Backup Online");
   
  //Init current sensor
  ina219.begin();
  
  //by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC);
  
  //Setup display
  display.setTextSize(1);
  display.setTextColor(WHITE);
 
  // Initialize ring buffer
  for (int i=0; i < GRAPH_MEMORY; i++) {
    graph_Mem[i] = 0;
  }

  
  //show splashscreen
  display.clearDisplay(); 
  display.drawBitmap(0, 0, FriedCircuitsUSBTester, 128, 24, WHITE);
  display.display();
  delay(1500);
  display.clearDisplay();   // clears the screen and buffer
  
  digitalWrite(LEDPIN, LOW);
  
}


void loop()
{
    
  float shuntvoltage = 0;
  float busvoltage = 0;
  float current_mA = 0;
  float loadvoltage = 0;
  
  static unsigned long last_interrupt_time2 = 0;
  unsigned long interrupt_time2 = millis();
  
  if (interrupt_time2 - last_interrupt_time2 > OLED_REFRESH_SPEED){
  

  shuntvoltage = ina219.getShuntVoltage_mV();
  busvoltage = ina219.getBusVoltage_V();
  current_mA = ina219.getCurrent_mA();
  loadvoltage = busvoltage + (shuntvoltage / 1000);
  
  //Setup placement for sensor readouts
  display.clearDisplay();

  //Refresh graph from current sensor data
  drawGraph(current_mA);
 
  //Set x,y and print sensor data
  display.setCursor(0,25);
  //display.print("V:");
  display.print(loadvoltage);   display.print("V ");
  //display.print("A:");
  display.print(current_mA);   display.print("mA ");
  display.print((current_mA*loadvoltage)/1000);  display.print("W");
  display.display();

//Serial output for data logging with Java app  
  Serial.print(":");
  Serial.print(busvoltage);
  Serial.print(":");
  Serial.print(shuntvoltage);
  Serial.print(":");
  Serial.print(loadvoltage);
  Serial.print(":");
  Serial.print(current_mA);
  Serial.print(":");
  //Serial.print(freeRam()); //Was for checking free ram during development
  //Serial.print(":");
  Serial.print(digitalRead(btnPin));
  Serial.println(":");
  
  
  last_interrupt_time2 = interrupt_time2;
}
 
  
  if (Serial.available() > 0){

     char in[4];
     int index = 0;
     
     while (Serial.available() > 0) {

        in[index] = Serial.read();
        index++;
     
     }
     in[index] ='\0';
     ledWarn = atoi(in);
  }
  
  int btnState = digitalRead(btnPin);
  
  if ((current_mA >= ledWarn) || (btnState)){
   
    digitalWrite(LEDPIN, HIGH);
   
  }
  else {
    
     digitalWrite(LEDPIN, LOW);
  }

  if (btnState) setButtonMode(digitalRead(btnPin));
  

  
  //delay(OLED_REFRESH_SPEED); 

}

// Draw the complete graph:
void drawGraph(float reading) {
  // Clear display:
  //display.fillRect(0, 0, 128, 24, BLACK);
  graph_Mem[ring_idx] = mapf(reading, 0, graph_MAX, 24, 0);
  ring_idx = (ring_idx+1)%GRAPH_MEMORY;
  for (int i=0; i < GRAPH_MEMORY; i++) {
    display.drawPixel(i, graph_Mem[(i+ring_idx)%GRAPH_MEMORY], WHITE);
  }
}


//Copy of Arduino map function converted for floats, from a forum post
float mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//From Arduino Playground
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


void setButtonMode(int button){
  static unsigned long last_interrupt_time = 0;
  static uint8_t currMode;
  unsigned long interrupt_time = millis();
  // If interrupts come faster than 200ms, assume it's a bounce and ignore
  if (interrupt_time - last_interrupt_time > 200)
  {
    if (currMode == maxMode)
	{
		currMode = 0;
	}
	else 
	{
		currMode++;
	}
	
	switch(currMode){
          case 0:
            OLED_REFRESH_SPEED = speed0;
          break;
            
          case 1:
            OLED_REFRESH_SPEED = speed1;
          break;            
          
          case 2:
            OLED_REFRESH_SPEED = speed2;
          break;          
          
          case 3:
            OLED_REFRESH_SPEED = speed3;
          break;          
          
          case 4:
            OLED_REFRESH_SPEED = speed4;
          break;
        } 
 }
  last_interrupt_time = interrupt_time;

}