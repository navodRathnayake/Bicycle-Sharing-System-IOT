#include "U8glib.h"
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE | U8G_I2C_OPT_DEV_0); 


const int sensor = 11;
unsigned long startTime = 0;
unsigned long endTime = 0;
int steps = 0;
float stepsOld = 0;
float temp = 0;
float rps = 0;
float speedKMPH = 0;

//variabels for clock
int hours = 1;      
int minutes = 00;  
int seconds = 0;  
char *number[12] = {"6", "5", "4", "3", "2", "1", "12", "11", "10", "9", "8", "7"};
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
float radius = min(SCREEN_HEIGHT, SCREEN_WIDTH) / 2 - 1;

const int X_CENTER = SCREEN_WIDTH / 2;
const int Y_CENTER = SCREEN_HEIGHT / 2;

int x1, y1, x2, y2;
double angle;

void draw(void) {
  
  u8g.drawCircle(X_CENTER, Y_CENTER, 1);
  //draw minute's ticks (60 lines)
  for (int j = 1; j <= 60; j++) {
    angle = j * 6;
    angle = angle * 0.0174533;

    x1 = X_CENTER + (sin(angle) * radius);
    y1 = Y_CENTER + (cos(angle) * radius);
    x2 = X_CENTER + (sin(angle) * (radius));
    y2 = Y_CENTER + (cos(angle) * (radius));
    u8g.drawLine(x1, y1, x2, y2);

  }
 
  for (int j = 0; j < 12; j++) {
    angle = j * 30;
    angle = angle * 0.0174533;

    x1 = X_CENTER + (sin(angle) * radius);
    y1 = Y_CENTER + (cos(angle) * radius);
    x2 = X_CENTER + (sin(angle) * (radius - 4));
    y2 = Y_CENTER + (cos(angle) * (radius - 4));
    u8g.drawLine(x1, y1, x2, y2);
  
    x2 = X_CENTER + (sin(angle) * (radius - 8));
    y2 = Y_CENTER + (cos(angle) * (radius - 8));
    u8g.setFont(u8g_font_chikita);
    u8g.drawStr(x2 - 2, y2 + 3, String(number[j]).c_str());

    angle = seconds * 6;
    angle = angle * 0.0174533;
    x2 = X_CENTER + (sin(angle) * (radius - 1));
    y2 = Y_CENTER - (cos(angle) * (radius - 1));
    u8g.drawLine(X_CENTER, Y_CENTER, x2, y2);

    angle = minutes * 6;
    angle = angle * 0.0174533;
    x2 = X_CENTER + (sin(angle) * (radius - 10));
    y2 = Y_CENTER - (cos(angle) * (radius - 10));
    u8g.drawLine(X_CENTER, Y_CENTER, x2, y2);

    angle = hours * 30 + ((minutes / 12) * 6);
    angle = angle * 0.0174533;
    x2 = X_CENTER + (sin(angle) * (radius / 2));
    y2 = Y_CENTER - (cos(angle) * (radius / 2));
    u8g.drawLine(X_CENTER, Y_CENTER, x2, y2);
  }
}

void setup() {
	Serial.begin(9600);
  u8g.begin();
  pinMode(sensor, INPUT_PULLUP);
  Serial.println("STEPS - 0");
  Serial.println("RPS   - 0.00");
  Serial.println("Speed (KMPH) - 0.00");
}

void loop() {
  seconds += 1;
  if (seconds == 60) {
    seconds = seconds - 60;
    minutes += 1;
  }
  if (minutes == 60) {
    minutes = 0;
    hours += 1;
  }
  if (hours == 24) {
    hours = 1;
  }
  //delay(800);

 startTime = millis();
  endTime = startTime + 1000;
  while (millis() < endTime) {
    if (digitalRead(sensor)) {
      steps = steps + 1;
      while (digitalRead(sensor));
    }
  }
  temp = steps - stepsOld;
  stepsOld = steps;
  rps = temp / 20.0;

 float distancePerStep = 0.1; 
  speedKMPH = (temp * distancePerStep) / 1000.0 * 3600.0; // 3600.0 converts from m/s to KMPH


    
    Serial.print("Speed: ");
    Serial.print(speedKMPH, 2);
    Serial.println(" km/h");
		

    
    u8g.firstPage();
    do {
      if (speedKMPH <= 0) {
        draw();
			
      } else {
        u8g.setFont(u8g_font_profont22);
        u8g.setPrintPos(10, 30);
        u8g.print(speedKMPH, 2);
        u8g.setPrintPos(75, 30);
        u8g.print("km/h");
      }
    } while (u8g.nextPage());
    	delay(500);
  }

