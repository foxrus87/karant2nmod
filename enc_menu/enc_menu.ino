// ***** I2C дисплей *****
#include <LiquidCrystal_I2C.h> // https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
#define cols 20
#define rows 4
LiquidCrystal_I2C lcd(0x27, cols, rows);
char *Blank;

// ********** Параметры меню **********
#define ShowScrollBar 1     // Показывать индикаторы прокрутки (0/1)
#define ScrollLongCaptions 1// Прокручивать длинные названия (0/1)
#define ScrollDelay 800     // Задержка при прокрутке текста
#define BacklightDelay 20000// Длительность подсветки
#define ReturnFromMenu 0    // Выходить из меню после выбора элемента(0/1)

enum eMenuKey {mkNull, mkBack, mkRoot, mkQuad, mkQuadSetA, mkQuadSetB, mkQuadSetC, mkQuadCalc, mkMulti, mkSettings, mkSetMotors,
               mkMotorsAuto, mkMotorsManual, mkSetSensors, mkSetUltrasonic, mkSetLightSensors, mkSetDefaults
              };

// ********** Переменные для энкодера ***************
#define pin_CLK 2 // Энкодер пин A
#define pin_DT  4 // Энкодер пин B
#define pin_Btn 3 // Кнопка

unsigned long CurrentTime, PrevEncoderTime;
enum eEncoderState {eNone, eLeft, eRight, eButton};
eEncoderState EncoderState;
int EncoderA, EncoderB, EncoderAPrev, counter;
bool ButtonPrev;

// ********** Прототипы функций ***************
eEncoderState GetEncoderState();
void LCDBacklight(byte v = 2);
eMenuKey DrawMenu(eMenuKey Key);

// ********** Обработчики для пунктов меню **********
int InputValue(char* Title, int DefaultValue, int MinValue, int MaxValue) {
  // Вспомогательная функция для ввода значения
  lcd.clear();
  lcd.print(Title);
  lcd.setCursor(0, 1);
  lcd.print(DefaultValue);
  delay(100);
  while (1)
  {
    EncoderState = GetEncoderState();
    switch (EncoderState) {
      case eNone: {
          LCDBacklight();
          continue;
        }
      case eButton: {
          LCDBacklight(1);
          return DefaultValue;
        }
      case eLeft: {
          LCDBacklight(1);
          if (DefaultValue > MinValue) DefaultValue--;
          break;
        }
      case eRight: {
          LCDBacklight(1);
          if (DefaultValue < MaxValue) DefaultValue++;
          break;
        }
    }
    lcd.setCursor(0, 1);
    lcd.print(Blank);
    lcd.setCursor(0, 1);
    lcd.print(DefaultValue);
  }
};

int A = 2, B = 5, C = -3;
void Demo() {
  lcd.clear();
  lcd.print("It's just a demo");
  while (GetEncoderState() == eNone) LCDBacklight();
};

void InputA() {
  A = InputValue("Input A", A, -10, 10);
  while (A == 0) {
    lcd.clear();
    lcd.print("Shouldn't be 0!");
    lcd.setCursor(0, 1);
    lcd.print("Input another value");
    while (GetEncoderState() == eNone) LCDBacklight();
    A = InputValue("Input A", A, -10, 10);
  }
};

void InputB() {
  B = InputValue("Input B", B, -10, 10);
};

void InputC() {
  C = InputValue("Input C", C, -10, 10);
};

void Solve() {
  int D;
  float X1, X2;
  lcd.clear();
  lcd.print(A);
  lcd.print("X^2");
  if (B >= 0) lcd.print("+");
  lcd.print(B);
  lcd.print("X");
  if (C >= 0) lcd.print("+");
  lcd.print(C);
  lcd.print("=0");
  D = B * B - 4 * A * C;
  lcd.setCursor(0, 1);
  if (rows > 2) {
    lcd.print("D=");
    lcd.print(D);
    lcd.setCursor(0, 2);
  }
  if (D == 0) {
    X1 = -B / 2 * A;
    lcd.print("X1=X2="); lcd.print(X1);
  }
  else if (D > 0) {
    X1 = (-B - sqrt(B * B - 4 * A * C)) / (2 * A);
    X2 = (-B + sqrt(B * B - 4 * A * C)) / (2 * A);
    lcd.print("X1=");  lcd.print(X1);
    lcd.print(";X2="); lcd.print(X2);
  }
  else
    lcd.print("Roots are complex");
  while (GetEncoderState() == eNone) LCDBacklight();
};

// ******************** Меню ********************
byte ScrollUp[8]  = {0x4, 0xa, 0x11, 0x1f};
byte ScrollDown[8]  = {0x0, 0x0, 0x0, 0x0, 0x1f, 0x11, 0xa, 0x4};

byte ItemsOnPage = rows;    // Максимальное количество элементов для отображения на экране
unsigned long BacklightOffTime = 0;
unsigned long ScrollTime = 0;
byte ScrollPos;
byte CaptionMaxLength;

struct sMenuItem {
  eMenuKey  Parent;       // Ключ родителя
  eMenuKey  Key;          // Ключ
  char      *Caption;     // Название пункта меню
  void      (*Handler)(); // Обработчик
};

sMenuItem Menu[] = {
  {mkNull, mkRoot, "Menu", NULL},
    {mkRoot, mkQuad, "Quadratic Equation Calculator", NULL},
      {mkQuad, mkQuadSetA, "Enter value A", InputA},
      {mkQuad, mkQuadSetB, "Enter value B", InputB},
      {mkQuad, mkQuadSetC, "Enter value C", InputC},
      {mkQuad, mkQuadCalc, "Solve", Solve},
      {mkQuad, mkBack, "Back", NULL},
    {mkRoot, mkMulti, "Multi-level menu example", NULL},
      {mkMulti, mkSettings, "Settings", NULL},
        {mkSettings, mkSetMotors, "Motors", NULL},
          {mkSetMotors, mkMotorsAuto, "Auto calibration", Demo},
          {mkSetMotors, mkMotorsManual, "Manual calibration", Demo},
          {mkSetMotors, mkBack, "Back", NULL},
        {mkSettings, mkSetSensors, "Sensors", NULL},
          {mkSetSensors, mkSetUltrasonic, "Ultrasonic", Demo},
          {mkSetSensors, mkSetLightSensors, "Light sensors", Demo},
          {mkSetSensors, mkBack, "Back", NULL},
        {mkSettings, mkSetDefaults, "Restore defaults", Demo},
        {mkSettings, mkBack, "Back", NULL},
      {mkMulti, mkBack, "Back", NULL}
};

const int MenuLength = sizeof(Menu) / sizeof(Menu[0]);

void LCDBacklight(byte v) { // Управление подсветкой
  if (v == 0) { // Выключить подсветку
    BacklightOffTime = millis();
    lcd.noBacklight();
  }
  else if (v == 1) { //Включить подсветку
    BacklightOffTime = millis() + BacklightDelay;
    lcd.backlight();
  }
  else { // Выключить если время вышло
    if (BacklightOffTime < millis())
      lcd.noBacklight();
    else
      lcd.backlight();
  }
}

eMenuKey DrawMenu(eMenuKey Key) { // Отрисовка указанного уровня меню и навигация по нему
  eMenuKey Result;
  int k, l, Offset, CursorPos, y;
  sMenuItem **SubMenu = NULL;
  bool NeedRepaint;
  String S;
  l = 0;
  LCDBacklight(1);
  // Запишем в SubMenu элементы подменю
  for (byte i = 0; i < MenuLength; i++) {
    if (Menu[i].Key == Key) {
      k = i;
    }
    else if (Menu[i].Parent == Key) {
      l++;
      SubMenu = (sMenuItem**) realloc (SubMenu, l * sizeof(void*));
      SubMenu[l - 1] = &Menu[i];
    }
  }

  if (l == 0) { // l==0 - подменю нет
    if ((ReturnFromMenu == 0) and (Menu[k].Handler != NULL)) (*Menu[k].Handler)(); // Вызываем обработчик если он есть
    LCDBacklight(1);
    return Key; // и возвращаем индекс данного пункта меню
  }

  // Иначе рисуем подменю
  CursorPos = 0;
  Offset = 0;
  ScrollPos = 0;
  NeedRepaint = 1;
  do {
    if (NeedRepaint) {
      NeedRepaint = 0;
      lcd.clear();
      y = 0;
      for (int i = Offset; i < min(l, Offset + ItemsOnPage); i++) {
        lcd.setCursor(1, y++);
        lcd.print(String(SubMenu[i]->Caption).substring(0, CaptionMaxLength));
      }
      lcd.setCursor(0, CursorPos);
      lcd.print(">");
      if (ShowScrollBar) {
        if (Offset > 0) {
          lcd.setCursor(cols - 1, 0);
          lcd.write(0);
        }
        if (Offset + ItemsOnPage < l) {
          lcd.setCursor(cols - 1, ItemsOnPage - 1);
          lcd.write(1);
        }
      }
    }
    EncoderState = GetEncoderState();
    switch (EncoderState) {
      case eLeft: {
          // Прокрутка меню вверх
          LCDBacklight(1);
          ScrollTime = millis() + ScrollDelay * 5;
          if (CursorPos > 0) {  // Если есть возможность, поднимаем курсор
            if ((ScrollLongCaptions) and (ScrollPos)) {
              // Если предыдущий пункт меню прокручивался, то выводим его заново
              lcd.setCursor(1, CursorPos);
              lcd.print(Blank);
              lcd.setCursor(1, CursorPos);
              lcd.print(String(SubMenu[Offset + CursorPos]->Caption).substring(0, CaptionMaxLength));
              ScrollPos = 0;
            }
            // Стираем курсор на старом месте, рисуем в новом
            lcd.setCursor(0, CursorPos--);
            lcd.print(" ");
            lcd.setCursor(0, CursorPos);
            lcd.print(">");
          }
          else if (Offset > 0) {
            //Курсор уже в крайнем положении. Если есть пункты выше, то перерисовываем меню
            Offset--;
            NeedRepaint = 1;
          }
          break;
        }
      case eRight: {
          // Прокрутка меню вниз
          LCDBacklight(1);
          ScrollTime = millis() + ScrollDelay * 5;
          if (CursorPos < min(l, ItemsOnPage) - 1) {// Если есть возможность, то опускаем курсор
            if ((ScrollLongCaptions) and (ScrollPos)) {
              // Если предыдущий пункт меню прокручивался, то выводим его заново
              lcd.setCursor(1, CursorPos);
              lcd.print(Blank);
              lcd.setCursor(1, CursorPos);
              lcd.print(String(SubMenu[Offset + CursorPos]->Caption).substring(0, CaptionMaxLength));
              ScrollPos = 0;
            }
            // Стираем курсор на старом месте, рисуем в новом
            lcd.setCursor(0, CursorPos++);
            lcd.print(" ");
            lcd.setCursor(0, CursorPos);
            lcd.print(">");
          }
          else {
            // Курсор уже в крайнем положении. Если есть пункты ниже, то перерисовываем меню
            if (Offset + CursorPos + 1 < l) {
              Offset++;
              NeedRepaint = 1;
            }
          }
          break;
        }
      case eButton: {
          // Выбран элемент меню. Нажатие кнопки Назад обрабатываем отдельно
          LCDBacklight(1);
          ScrollTime = millis() + ScrollDelay * 5;
          if (SubMenu[CursorPos + Offset]->Key == mkBack) {
            free(SubMenu);
            return mkBack;
          }
          Result = DrawMenu(SubMenu[CursorPos + Offset]->Key);
          if ((Result != mkBack) and (ReturnFromMenu)) {
            free(SubMenu);
            return Result;
          }
          NeedRepaint = 1;
          break;
        }
      case eNone: {
          if (ScrollLongCaptions) {
            // При бездействии прокручиваем длинные названия
            S = SubMenu[CursorPos + Offset]->Caption;
            if (S.length() > CaptionMaxLength)
            {
              if (ScrollTime < millis())
              {
                ScrollPos++;
                if (ScrollPos == S.length() - CaptionMaxLength)
                  ScrollTime = millis() + ScrollDelay * 2; // Небольшая задержка когда вывели все название
                else if (ScrollPos > S.length() - CaptionMaxLength)
                {
                  ScrollPos = 0;
                  ScrollTime = millis() + ScrollDelay * 5; // Задержка перед началом прокрутки
                }
                else
                  ScrollTime = millis() + ScrollDelay;
                lcd.setCursor(1, CursorPos);
                lcd.print(Blank);
                lcd.setCursor(1, CursorPos);
                lcd.print(S.substring(ScrollPos, ScrollPos + CaptionMaxLength));
              }
            }
          }
          LCDBacklight();
        }
    }
  } while (1);
}
//****************************************

void setup() {
  pinMode(pin_CLK, INPUT);
  pinMode(pin_DT,  INPUT);
  pinMode(pin_Btn, INPUT_PULLUP);
  lcd.begin();
  lcd.backlight();
  CaptionMaxLength = cols - 1;
  Blank = (char*) malloc(cols * sizeof(char));
  for (byte i = 0; i < CaptionMaxLength; i++)
    Blank[i] = ' ';
  if (ShowScrollBar) {
    CaptionMaxLength--;
    lcd.createChar(0, ScrollUp);
    lcd.createChar(1, ScrollDown);
  }
  Blank[CaptionMaxLength] = 0;
}

void loop() {
  DrawMenu(mkRoot);
}

// ******************** Энкодер с кнопкой ********************
eEncoderState GetEncoderState() {
  // Считываем состояние энкодера
  eEncoderState Result = eNone;
  CurrentTime = millis();
  if (CurrentTime >= (PrevEncoderTime + 5)) {
    PrevEncoderTime = CurrentTime;
    if (digitalRead(pin_Btn) == LOW ) {
      if (ButtonPrev) {
        Result = eButton; // Нажата кнопка
        ButtonPrev = 0;
      }
    }
    else {
      ButtonPrev = 1;
      EncoderA = digitalRead(pin_DT);
      EncoderB = digitalRead(pin_CLK);
      if ((!EncoderA) && (EncoderAPrev)) { // Сигнал A изменился с 1 на 0
        if (EncoderB) Result = eRight;     // B=1 => энкодер вращается по часовой
        else          Result = eLeft;      // B=0 => энкодер вращается против часовой
      }
      EncoderAPrev = EncoderA; // запомним текущее состояние
    }
  }
  return Result;
}
