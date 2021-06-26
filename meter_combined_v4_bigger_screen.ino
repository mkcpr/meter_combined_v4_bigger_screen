#define SHOW_BMW_ANIMATION
//#define RANDOM_WATTS_DEMO

//----------------------------------------------------------------------------

#define TFT_CS        10
#define TFT_RST        8
#define TFT_DC         9

#include <SPI.h>
#include <TFT_ST7735.h>
TFT_ST7735 tft = TFT_ST7735(TFT_CS, TFT_DC, TFT_RST);

#include "_fonts/akashi20.c"

const char string_BMW[] PROGMEM = "BMW";

//----------------------------------------------------------------------------

#include <DHT.h>
#define DHTPIN    2         // Digital pin connected to the DHT sensor 
#define DHTTYPE   DHT22     // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE);

float prevTemp = 99;
float prevHum = 99;

unsigned long startMillis;
const unsigned long period = 1000;

//----------------------------------------------------------------------------

#include <Wire.h>
#include "INA226.h"
INA226 ina;

bool inaConnected = false;
const char string_0[] PROGMEM = "INA226 NOT CONNECTED";

#define max_readings 20

int watt_readings[max_readings + 1] = {0};
byte reading = max_readings; //1; //directly shift data for nicer visual

//----------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);


  //-------------------------------------------------------------------------
  //TFT screen
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(BLACK, BLACK);
  //-------------------------------------------------------------------------

  
  //-------------------------------------------------------------------------
  //DHT-22
  dht.begin();
  startMillis = millis() + period;  //Directly start
  //-------------------------------------------------------------------------

  
  //-------------------------------------------------------------------------
  //INA-226
  Wire.begin();
  testINAConnection();
  ina.begin(0x40);  // Default INA226 address is 0x40
  //-------------------------------------------------------------------------

  
  //-------------------------------------------------------------------------
  // Configure INA226
  //ina.configure(INA226_AVERAGES_1, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);
  ina.configure(INA226_AVERAGES_1024, INA226_BUS_CONV_TIME_1100US, INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);

  // Calibrate INA226. Rshunt = 0.01 ohm, Max excepted current = 4A
  //ina.calibrate(0.01, 4);
  ina.calibrate(0.002, 4);
  
  for (int x = 0; x <= max_readings; x++) {
    watt_readings[x] = 0;
  }
  //-------------------------------------------------------------------------

  
  //-------------------------------------------------------------------------
  //BMW intro animation
  #if defined(SHOW_BMW_ANIMATION)
    tft.fillScreen(WHITE, WHITE);
  
    delay(500);
  
    int BMWLogo_x = 64;
    int BMWLogo_y = 102;
    int BMWText_y = 1;
    
    drawBMWLogo(BMWLogo_x, BMWLogo_y);
    drawBMWText(BMWText_y, WHITE, BLACK, 60);
    
    delay(3000);
  
    removeBMWLogo(BMWLogo_x, BMWLogo_y);
    drawBMWText(BMWText_y, BLACK, WHITE, 10);
    tft.fillScreen(WHITE, WHITE);
    
    delay(1000);
  #endif
  //-------------------------------------------------------------------------
  

  tft.fillScreen(BLACK, BLACK);
}

void loop() {

  unsigned long currentMillis = millis();
  if (currentMillis - startMillis >= period){  //test whether the period has elapsed

    //---------------------------------------------------------------------------------
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    int h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float t = dht.readTemperature();
    
    drawTemperature(t);
    drawHumidity(h);
    //---------------------------------------------------------------------------------
  
  
    //---------------------------------------------------------------------------------
    testINAConnection();
    
    #if defined(RANDOM_WATTS_DEMO)
      float watt = random(0, 3000);
      watt /= 10;
      if (inaConnected){
        ina.readBusPower();
      }
    #else
      float watt = 0;
      if (inaConnected){
        watt = ina.readBusPower();
      }
    #endif
    
    tft.fillRect(0, 70, 128, 21, BLACK);
  
    tft.setFont(&akashi20);
    tft.setTextScale(1);
    tft.setCursor(CENTER, 70);

    if (inaConnected){
      char temperatureStringRounded[6];
      dtostrf(watt, 3, 1, temperatureStringRounded);
      char temperatureStringSpaces[6];
      sprintf(temperatureStringSpaces, "%5s w", temperatureStringRounded);
  
      tft.print(temperatureStringSpaces);
    }
    else{
      tft.setFont(&defaultFont);
      tft.setTextScale(1);
      tft.setTextWrap(false);
      tft.setCursor(CENTER, 78);

      char buff[21];
      strcpy_P(buff, string_0);
      tft.print(buff);
    }
    
    drawGraph(10, 90, 5, 34, 0, watt_readings);
  
    reading = reading + 1;
    if (reading > max_readings) { // if number of readings exceeds max_readings (e.g. 100) then shift all array data to the left to effectively scroll the display left
      reading = max_readings;
      for (int i = 1; i < max_readings; i++) {
        watt_readings[i] = watt_readings[i + 1];
      }
      watt_readings[reading] = watt;
    }
    //---------------------------------------------------------------------------------


    startMillis = currentMillis;
  }

  delay(20);
}

void testINAConnection(){
  //Test INA connection
  inaConnected = false;
  Wire.beginTransmission(64); //0x40
  byte error = Wire.endTransmission();
  if (error == 0){
    inaConnected = true;
  }
}

void drawGraph(int x_pos, int y_pos, int barWidth, int height, int Y1Max, int DataArray[max_readings]) {

  #define auto_scale_major_tick 5 // Sets the autoscale increment, so axis steps up in units of e.g. 5
  int maxYscale = 0;
  
  if (true) {
    for (int i = 1; i <= max_readings; i++ ) if (maxYscale <= DataArray[i]) maxYscale = DataArray[i];
    maxYscale = ((maxYscale + auto_scale_major_tick + 2) / auto_scale_major_tick) * auto_scale_major_tick; // Auto scale the graph and round to the nearest value defined, default was Y1Max
    //if (maxYscale < Y1Max) Y1Max = maxYscale;
    Y1Max = maxYscale;
  }

  int x1, y1, x2, y2;
  for (int gx = 1; gx <= max_readings; gx++) {
    x1 = x_pos + (gx * barWidth);
    y1 = y_pos + height;
    x2 = x_pos + (gx * barWidth);
    y2 = y_pos + height - constrain(DataArray[gx], 0, Y1Max) * height / Y1Max + 1;

    for (int bx = 0; bx < barWidth - 1; bx++) {
      tft.drawLine(x1 + bx, y1, x1 + bx, y_pos, BLACK);
      tft.drawLine(x1 + bx, y1, x2 + bx, y2, RED);
    }
  }

}


void drawTemperature(float temperature){
  char temperatureStringRounded[6];
  dtostrf(temperature, 3, 1, temperatureStringRounded);

  float curTemp = atof(temperatureStringRounded);
  if (prevTemp == curTemp){
    return;
  }
  prevTemp = curTemp;

  char temperatureStringSpaces[6];
  sprintf(temperatureStringSpaces, "%5s", temperatureStringRounded);   // print with leading spaces


  //tft.fillRect(1, 1, 69, 32, RED);
  tft.fillRect(1, 1, 69, 32, BLACK);
  
  tft.setFont(&akashi20);
  tft.setTextScale(1);
  
  tft.setCursor(6, 5);
  tft.print(temperatureStringSpaces);
  
  tft.setFont(&defaultFont);
  tft.setTextScale(1);
  tft.print(F("C"));
}

void drawHumidity(int humidity){
  if (prevHum == humidity){
    return;
  }
  prevHum = humidity;
  
  
  //tft.fillRect(70, 1, 58, 32, BLUE);
  tft.fillRect(70, 1, 58, 32, BLACK);

  tft.setFont(&akashi20);
  tft.setTextScale(1);
  tft.setCursor(74, 5);
  tft.print(humidity);
  tft.print(F("%"));
}

void drawBMWLogo(int x, int y){
  for (int radius = 0; radius <= 20; radius += 2) {
    tft.drawArc(x, y, radius + 5, 5, 0, 360, BLACK);
    tft.drawArc(x, y, radius, radius, 0, 90, WHITE);
    tft.drawArc(x, y, radius, radius, 90, 180, WHITE);
    tft.drawArc(x, y, radius, radius, 180, 270, WHITE);
    tft.drawArc(x, y, radius, radius, 270, 360, WHITE);
    //delay(20);
  }

  tft.drawPixel(x, y, 0x351A);
  
  for (int w = 0; w <= 20; w += 1) {
    tft.drawLine(x - w, y, x + w, y, 0x351A);
    delay(20);
  }

  for (int angle = 0; angle <= 90; angle += 2) {
    tft.drawArc(x, y, 20, 20, 90, 90 + angle, 0x351A);
    tft.drawArc(x, y, 20, 20, 270, 270 + angle, 0x351A);
  }
}

void removeBMWLogo(int x, int y){
  for (int w = 0; w <= 25; w += 1) {
    tft.drawLine(x - w, y, x + w, y, WHITE);
    delay(2);
  }

  for (int angle = 0; angle <= 180; angle += 20) {
    tft.drawArc(x, y, 25, 25, 90, 90 + angle, WHITE);
    tft.drawArc(x, y, 25, 25, 270, 270 + angle, WHITE);
  }
}

void drawBMWText(int y, uint16_t color1, uint16_t color2, int steps){
  for (int fade = 0; fade <= steps; fade += 1) {
    tft.setFont(&akashi20);
    tft.setTextScale(2);
    tft.setCursor(CENTER, y);
    tft.setTextColor(tft.colorInterpolation(color1, color2, fade, steps));
    //tft.print("BMW");
    
    char buff[4];
    strcpy_P(buff, string_BMW);
    tft.print(buff);
  }
}
