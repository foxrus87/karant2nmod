 /*
   Без ПЛЛ меню
   Код для синтезатора от UD0CAJ
   Без переключения USB/L
   Лёгкая модернизация от R2SBN
   Предел диапазона задается в меню.
*/
#include <Adafruit_SSD1306.h>
#include <si5351.h>
#include <Wire.h>
#include <Encoder.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <TimeLib.h>



#define ENCODER_OPTIMIZE_INTERRUPTS

//byte ONE_WIRE_BUS = 12; // Порт датчика температуры
byte myEncBtn = 4;  // Порт нажатия кноба.
byte mypowerpin = 6; // Порт показометра мощности.
byte mybattpin = 7; // Порт датчика АКБ
byte txpin = 5; //Порт датчика ТХ.
byte menu = 0; //Начальное положение меню.
float mybatt;
int mypower;
float voltage; // переменная мощности вольты
float freqprint;
int temperature;
byte ssbmode = 0;
long oldPosition  = 0;
boolean actencf = false;
boolean txen = false;
int RXifshift = 0;
boolean knobup = true;
boolean exitmenu = false;
boolean reqtemp = false;
boolean timesetup = false;
int pogr = 200;

struct var {
  int stp = 100;
  int battcal = 219;
  unsigned long freq = 1932000UL; // Начальная частота при первом включении.
  unsigned long lofreq = 496600UL; // Начальная ПЧ при первом включении.
  long calibration = 178720; // Начальная калибровка при первом включении.
  int ifshift = 0; // Начальный сдвиг ПЧ при первом включении.
  byte minfreq = 80; // 100KHz Минимальный предел частоты
  byte maxfreq = 10; // 100KHz Максимальный предел частоты
} varinfo;


// unsigned long previousMillis = 0;
unsigned long previousdsp = 0;
unsigned long previoustemp = 0;
unsigned long previoustime = 0;
unsigned long knobMillis = 0;
unsigned long actenc = 0;

Si5351 si5351;
Encoder myEnc(2, 3); //порты подключения енкодера.
Adafruit_SSD1306 display(4);
//OneWire oneWire(ONE_WIRE_BUS);
tmElements_t tm;


void setup() {
 
  pinMode(myEncBtn, INPUT);
  pinMode(mypowerpin, INPUT);
  digitalWrite(myEncBtn, HIGH);
  analogReference(INTERNAL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();
//  sensors.begin();
  memread();
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_ms_source(SI5351_CLK0, SI5351_PLLA);
//  si5351.set_ms_source(SI5351_CLK2, SI5351_PLLB);
  si5351.set_correction(varinfo.calibration, SI5351_PLL_INPUT_XO);
  freqprint = varinfo.freq / 1000.00;
  //losetup();
  vfosetup();
  battmeter();
  powermeter();
  //tempsensor ();
  //timenow ();
  mainscreen();
}

void loop() { // Главный цикл
  pushknob ();
  readencoder();
  txsensor ();
  if (!menu) {
    if (millis() - previousdsp > 250) {
      storetomem();
      battmeter();
      powermeter();
      mainscreen();
      previousdsp = millis();
    }
  }
}


void txsensor () {
  boolean txsens = digitalRead(txpin);
  //Если радио на приеме и нажали ТХ то обнулить IF-SHIFT
  if (txsens && !txen) {
    txen = true;
    RXifshift = 0;
    vfosetup();
    //losetup();
  }
  //Если радио на передаче и отпустили ТХ то прописать IF-SHIFT
  if (!txsens && txen) {
    txen = false;
    RXifshift = varinfo.ifshift;
    vfosetup();
   // losetup();
  }
  //Если радио на приеме и НЕ нажали ТХ и RXifshift не равно varinfo.ifshift то прописать IF-SHIFT
  if (!txsens && !txen && RXifshift != varinfo.ifshift) {
    txen = false;
    RXifshift = varinfo.ifshift;
    vfosetup();
   // losetup();
  }
}

void pushknob () {  // Обработка нажатия на кноб

  boolean knobdown = digitalRead(myEncBtn);   //Читаем состояние кноба
  if (!knobdown && knobup) {   //Если кноб был отпущен, но нажат сейчас
    knobup = false;   // отмечаем флаг что нажат
    knobMillis = millis();  // запускаем таймер для антидребезга
  }

  if (knobdown && !knobup) { //Если кноб отпущен и был нажат
    knobup = true; // отмечаем флаг что кноб отпущен
    long knobupmillis = millis();
    if (knobupmillis - knobMillis >= 1000) {
      if (menu != 0) menu = 0; //Переходим на меню дальше
      if (menu == 0) menu = 3; //Если меню больше 5 выйти на главный экран
    }

    if (knobupmillis - knobMillis < 1000 && knobupmillis - knobMillis > 100) { //Если кноб отпущен и был нажат и времени от таймера прошло 100Мс
      menu ++; //Переходим на меню дальше
      if (menu == 3) menu = 0; //Если меню больше 3 выйти на главный экран
      if (menu > 7) menu = 0; //Если меню больше 8 выйти на главный экран
    }
    mainscreen();
  }
}

void storetomem() { // Если крутили енкодер, то через 5 секунд все сохранить

  if ((millis() - actenc > 5000UL) && actencf) {
    actenc = millis();
    actencf = false;
    memwrite ();
  }
}

void readencoder() { // работа с енкодером
  long newPosition = myEnc.read() / 4;
  if (newPosition != oldPosition) {
    switch (menu) {

      case 0: //Основная настройка частоты
        if (newPosition > oldPosition && varinfo.freq <= varinfo.maxfreq * 100000UL) {
          if (varinfo.freq % varinfo.stp) {
            varinfo.freq = varinfo.freq + varinfo.stp - (varinfo.freq % varinfo.stp);
          }
          else {
            varinfo.freq = varinfo.freq + varinfo.stp;
          }
        }
        if (newPosition < oldPosition && varinfo.freq >= varinfo.minfreq * 100000UL) {
          if (varinfo.freq % varinfo.stp) {
            varinfo.freq = varinfo.freq - (varinfo.freq % varinfo.stp);
          }
          else {
            varinfo.freq = varinfo.freq - varinfo.stp;
          }
        }
        if (varinfo.freq < varinfo.minfreq * 100000UL) varinfo.freq = varinfo.minfreq * 100000UL;
        if (varinfo.freq > varinfo.maxfreq * 100000UL) varinfo.freq = varinfo.maxfreq * 100000UL;
        vfosetup();
        break;

      case 1: //Настройка ШАГА настройки
        if (newPosition > oldPosition && varinfo.stp <= 1000) varinfo.stp = varinfo.stp * 10;
        if (newPosition < oldPosition && varinfo.stp >= 10) varinfo.stp = varinfo.stp / 2;
        if (varinfo.stp < 10) varinfo.stp = 1000;
        if (varinfo.stp > 1000) varinfo.stp = 10;
        break;

      case 2: //Настройка IF-SHIFT
        if (newPosition > oldPosition && varinfo.ifshift <= 3000) varinfo.ifshift = varinfo.ifshift + 10;
        if (newPosition < oldPosition && varinfo.ifshift >= -3000) varinfo.ifshift = varinfo.ifshift - 10;
        if (varinfo.ifshift > 3000) varinfo.ifshift = 3000;
        if (varinfo.ifshift < -3000) varinfo.ifshift = - 3000;
        //losetup();
        vfosetup();
        break;

      case 3: //Настройка опорного гетеродина
        if (newPosition > oldPosition && varinfo.lofreq <= 550000) varinfo.lofreq = varinfo.lofreq + varinfo.stp;
        if (newPosition < oldPosition && varinfo.lofreq >= 450000) varinfo.lofreq = varinfo.lofreq - varinfo.stp;
        if (varinfo.lofreq < 450000) varinfo.lofreq = 450000;
        if (varinfo.lofreq > 550000) varinfo.lofreq = 550000;
        //losetup();
        break;

      case 4: //Настройка калибровки по питанию
        if (newPosition > oldPosition && varinfo.battcal <= 1000) varinfo.battcal = varinfo.battcal + 1;
        if (newPosition < oldPosition && varinfo.battcal >= 100) varinfo.battcal = varinfo.battcal - 1;
        if (varinfo.battcal > 1000) varinfo.battcal = 1000;
        if (varinfo.battcal < 100) varinfo.battcal = 100;
        break;

      case 5: //Настройка калибровки PLL
        if (newPosition > oldPosition && varinfo.calibration <= 200000) varinfo.calibration = varinfo.calibration + 10;
        if (newPosition < oldPosition && varinfo.calibration >= - 200000) varinfo.calibration = varinfo.calibration - 10;
        if (varinfo.calibration > 200000) varinfo.calibration = 200000;
        if (varinfo.calibration <  - 200000) varinfo.calibration =  - 200000;
        si5351.set_correction(varinfo.calibration, SI5351_PLL_INPUT_XO);
        break;

      case 6: //Настройка minfreq
        if (newPosition > oldPosition && varinfo.minfreq <= 100) varinfo.minfreq = varinfo.minfreq + 1;
        if (newPosition < oldPosition && varinfo.minfreq >= 10) varinfo.minfreq = varinfo.minfreq - 1;
        if (varinfo.minfreq < 10) varinfo.minfreq = 10;
        if (varinfo.minfreq >= varinfo.maxfreq) varinfo.minfreq = varinfo.maxfreq - 1;
        break;

      case 7: //Настройка maxfreq
        if (newPosition > oldPosition && varinfo.maxfreq <= 100) varinfo.maxfreq = varinfo.maxfreq + 1;
        if (newPosition < oldPosition && varinfo.maxfreq >= 10) varinfo.maxfreq = varinfo.maxfreq - 1;
        if (varinfo.maxfreq <= varinfo.minfreq) varinfo.maxfreq = varinfo.minfreq + 1;
        if (varinfo.maxfreq > 100) varinfo.maxfreq = 100;
        break;

    }
    actenc = millis();
    actencf = true;
    mainscreen();
    oldPosition = newPosition;
  }
}

void powermeter () { // Измеритель уровня выхода
  int rawpower = analogRead(mypowerpin);
 mypower = map(rawpower, 0, 1023, 0, 100);
 
  
}
//}

void battmeter () { // Измеритель напряжения питания
  int rawbatt = analogRead(mybattpin);
 mybatt = (map(rawbatt, 0, 1023, 0, varinfo.battcal)) / 10.0;
}

void mainscreen() { //Процедура рисования главного экрана
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextColor(WHITE);
  display.setTextSize(3);
  switch (menu) {

    case 0: //Если не в меню, то рисовать главный экран
      display.println(varinfo.freq / 1000.0);
      display.setTextSize(2);
      display.print(mybatt);
      display.print(" ");
      
      display.print(mypower);
      
       
      break;

    case 1: //Меню 1 - шаг настройки
      display.println(varinfo.stp);
      display.setTextSize(2);
      display.print(menu);
      display.print(" Step");
      break;

    case 2: //Меню 2 - IF-SHIFT
      display.println(varinfo.ifshift);
      display.setTextSize(2);
      display.print(menu);
      display.println(" IF-SHIFT");
      break;

    case 3: //Меню 3 - Настройка опорного гетеродина
      display.println(varinfo.lofreq);
      display.setTextSize(2);
      display.print(menu);
      display.print(" Cal Hz");
      break;


    case 4: //Меню 4 - Настройка калибровки по питанию
      display.println(varinfo.battcal);
      display.setTextSize(2);
      display.print(menu);
      display.print(" Batt");
      break;

    case 5: //Меню 5 - Настройка калибровки кварца
      display.println(varinfo.calibration);
      display.setTextSize(2);
      display.print(menu);
      display.print(" Xtal");
      break;


    case 6: //Меню 6 - Настройка minfreq
      display.println(varinfo.minfreq * 100);
      display.setTextSize(2);
      display.print(menu);
      display.print(" Min Freq");
      break;

    case 7: //Меню 7 - Настройка maxfreq
      display.println(varinfo.maxfreq * 100);
      display.setTextSize(2);
      display.print(menu);
      display.print(" Max Freq");
      break;

      
  }
  display.display();
}


void vfosetup() {
  si5351.set_freq(((varinfo.freq  + 500000 + pogr) * 100), SI5351_CLK0);
}


//void losetup() {
//  si5351.set_freq(((varinfo.lofreq + RXifshift) * 100), SI5351_CLK2);
//}

void memwrite () {
  int crc = 0;
  byte i = 0;
  byte * adr;
  adr =  (byte*)(& varinfo);
  while (i < (sizeof(varinfo)))
  {
    crc += *(adr + i);
    i++;
  }
  EEPROM.put(2, varinfo);
  EEPROM.put(0, crc);
}

void memread() {
  int crc = 0;
  int crcrom = 0;
  byte i = 0;
  EEPROM.get(0, crc);
  while (i < (sizeof(varinfo)))
  {
    crcrom += EEPROM.read((i + 2));

    i++;
  }
  if (crc == crcrom) {
    EEPROM.get(2, varinfo);
  }
  else {
    memwrite ();
  }
}
