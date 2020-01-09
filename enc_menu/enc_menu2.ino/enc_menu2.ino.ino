#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#define OLED_ADDR   0x3C
Adafruit_SSD1306 display(-1);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#define encoder0PinA  2
#define encoder0PinB  3
volatile unsigned int encoder0Pos = 0;

int valA;
int valB;
int valC;

byte clk;
byte menuCount = 1;
byte dir = 0;
bool runState = false;

void setup() {
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.display();
  display.clearDisplay();
  attachInterrupt(0, doEncoder, CHANGE);  // encoder pin on interrupt 0 - pin 2
}

void loop() {
  clk = digitalRead(4);
  ledControl();
  menuCheck();
  staticMenu();
  display.clearDisplay();
  delay(50);
}

void staticMenu() {
  display.setTextSize(2);
  display.setTextColor(WHITE);

  display.setCursor(10, 0);
  display.println("yyRobotic");
  //---------------------------------
  display.setTextSize(1);
  display.setCursor(10, 20);
  display.println("Value A:");
  display.setCursor(60, 20);
  display.println(valA);

  display.setCursor(10, 30);
  display.println("Value B:");
  display.setCursor(60, 30);
  display.println(valB);

  display.setCursor(10, 40);
  display.println("Value C:");
  display.setCursor(60, 40);
  display.println(valC);

  display.setCursor(10, 50);
  display.println("Start:");
  display.setCursor(45, 50);
  if (encoder0Pos > 5 && menuCount == 4) {
    display.println("Run!");
    runState = true;
  } else {
    display.println("Idle");
    runState = false;
  }
  display.setCursor(2, (menuCount * 10) + 10);
  display.println(">");

  display.display();
}

void ledControl() {
  if (runState == true) {
    analogWrite(6, valA);
    analogWrite(9, valB);
    analogWrite(10, valC);
  }
}
void menuCheck() {
  if (clk == LOW && menuCount < 5) {
    menuCount++;
    encoder0Pos = 0;
  }
  if (clk == LOW && menuCount >= 5) {
    menuCount = 1;
  }

  if (menuCount == 1) {
    valA = encoder0Pos;
  }
  if (menuCount == 2) {
    valB = encoder0Pos;
  }
  if (menuCount == 3) {
    valC = encoder0Pos;
  }
}

void doEncoder() {
  if (digitalRead(encoder0PinA) == HIGH) {
    if (digitalRead(encoder0PinB) == LOW && encoder0Pos > 0) {
      encoder0Pos = encoder0Pos - 1;
      dir = 0;
    }
    else {
      encoder0Pos = encoder0Pos + 1;
      dir = 1;
    }
  }
  else
  {
    if (digitalRead(encoder0PinB) == LOW ) {
      encoder0Pos = encoder0Pos + 1;
      dir = 1;
    }
    else {
      if (encoder0Pos > 0) {
        encoder0Pos = encoder0Pos - 1;
        dir = 0;
      }
    }
  }
}
