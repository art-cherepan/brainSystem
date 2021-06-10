//---------------------version 1.3--------------------------
//исправил косяк с таймером на 1 секунде во 2 времени, исправил проблему с кнопкой Cтарт в режиме Брейн-ринг
//(отображалось 2 время потому, что кнопка висит на прерывании и бывали случаи, когда считывалось 2 нажатия (срабатывало 2 прерывания), исправил запретом прерываний при первом нажатии)
//адаптировал пол новую систему. Реализвоал управление сдвиг. регистрами в каскаде
//исравлен баг: в брейн-ринге на прерывании по стопу сдвиг регистры не работали. Переписал это для основоного цикла. 
//ПЕРЕПИСАТЬ ВКЛЮЧЕНИЕ СДВИГ. РЕГИСТРОВ В ОСНОВНОМ ЦИКЛЕ!!! .Т.Е В ПРЕРЫВАНИЯХ УСТАНАЛИВАТЬ ТОЛЬКО РАЗЛИЧНЫЕ ПЕРЕМЕННЫЕ-ФЛАГИ, А В ОСНОВОНОМ ЦИКЛЕ ПРОИЗВОДИТЬ ВСЮ РАБОТУ
//это прошивка сейчас зашита

#define NUM_LEDS 16   // число диодов
#define PIN_2812 4
#include <util/delay.h>
#include <EEPROM.h>
#include "Adafruit_NeoPixel.h"
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN_2812, NEO_GRB + NEO_KHZ800);
// frequencies
const int c = 261;
const int d = 294;
const int e = 329;
const int f = 349;
const int g = 391;
const int gS = 415;
const int a = 440;
const int aS = 455;
const int b = 466;
const int cH = 523;
const int cSH = 554;
const int dH = 587;
//const int dSH = 622; - так стояло (изменил для кнопки стоп)
const int dSH = 630;
const int eH = 659;
const int fH = 698;
const int fSH = 740;
const int gH = 784;
const int gSH = 830;
const int aH = 880;

int notes[19] = {261, 294, 329, 349, 391, 415, 440, 45, 466, 523, 554, 587, 630, 659, 698, 740, 784, 830, 880};

int clockPin = A0;   //leds, rgb-led
int latchPin = A1;
int dataPin = A2;
int catod1 = 3;
int catod2 = 2;
int leftTimeBtn = A3;
int setTimeBtn = A5;
int rightTimeBtn = A4;
int pinPower1 = 7;
int pinPower2 = 6;
int pinSpeaker = 5; //нужен ШИМ-выход
bool setTimeFlag;
bool blinkTimer;
bool showTimer;
bool showFirstNumber;
bool showSecondNumber;
bool firstTimerOn;
bool brainRing;
bool myGame;
bool players[6];    //для своей игры и брейна
bool Start;
bool settingTime; //для точки для 2 времени. Когда настраиваем 2 время (для брейн-ринга в случае неверного ответа 1 команды), чтобы понять что это именно ВТОРОЕ время, загорается точка
volatile bool ledsOffFlag;
volatile bool myGameLedsDefaultFlag;
volatile bool brainRingLedsDefaultFlag;

int firstTime = 60; //для установки времени
int secondTime = 20;
//поставить 60 сек для первого времени и 20 сек для второго
int timerOVFcount = 0;
int checkTimerShow;
int firstTimeShow;  //для отбражения на таймере
int secondTimeShow;
int address = 0;  //адрес ячейки eeprom

byte digitsFirstTime[10] = { 0b11110011, 0b00110000, 0b10011011, 0b10111010, 0b01111000, 0b11101010, 0b11101011, 0b00110010, 0b11111011 ,0b01111010 }; //без точки
byte digitsSecondTime[10] = { 0b11110111, 0b00110100, 0b10011111, 0b10111110, 0b01111100, 0b11101110, 0b11101111, 0b00110110, 0b11111111 ,0b01111110 }; //с точкой
byte ledBits; //храним значения битов для сдвигового регистра для ламп игроков и RGB-led при включении таймера 

void setup()
{
  setTimeFlag = false;
  firstTimerOn = true;
  pinMode(13, INPUT); //прерывание на 13 пине нужно настраивать как вход (на остальных пинах из pcint_0 как вход настраивать не нужно (установил опытным путем))
  digitalWrite(13, HIGH);

  pinMode(PIN_2812, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(catod1, OUTPUT);
  pinMode(catod2, OUTPUT);
  //
  pinMode(pinSpeaker, OUTPUT);
  pinMode(leftTimeBtn, INPUT);
  pinMode(setTimeBtn, INPUT);
  pinMode(rightTimeBtn, INPUT);
  pinMode(pinPower1, INPUT);
  pinMode(pinPower2, INPUT);
  digitalWrite(latchPin, HIGH);
  digitalWrite(pinSpeaker, HIGH);
  showTimer = false;
  showFirstNumber = true;
  showSecondNumber = false;
  blinkTimer = false;
  Start = false;
  settingTime = false;
  ledsOffFlag = true;
  brainRingLedsDefaultFlag = false;
  myGameLedsDefaultFlag = false;
  
  timerOVFcount = 0;
  checkTimerShow = 0;
  firstTime = EEPROM.read(address);
  ++address;
  secondTime = EEPROM.read(address);
  firstTimeShow = firstTime;
  secondTimeShow = secondTime;
  notDisplayTime();
  WS2812_OFF();
}

void loop()
{ 
  if (digitalRead(setTimeBtn))
  {
    notDisplayTime();
    delay(450);
    if (digitalRead(setTimeBtn))
    {
      notDisplayTime();
      digitalWrite(catod1, LOW);
      digitalWrite(catod2, HIGH);
      if (checkTimerShow == 2)
        checkTimerShow = 0;
      else
        checkTimerShow++;
      while (true)
        {
          if (checkTimerShow == 0)  //погаси табло
            notDisplayTime();
          else if (checkTimerShow == 1) //1 время
          {
            settingTime = true;
            displayTime(true, settingTime);
            showFirstNumber = true;  //нет смысла опрделять showSecondNumber, т.к. это и так флаг
          }
          else if (checkTimerShow == 2) //2 время
          {
            settingTime = true;
            displayTime(false, settingTime);
            showFirstNumber = false;
          }
          if (!digitalRead(setTimeBtn))
            break; 
        }
        if ((checkTimerShow == 1) || (checkTimerShow == 2)) //для главного цикла loop (показывать время или нет)
          showTimer = true;
        else if (checkTimerShow == 0)
          showTimer = false;
    }
  }

  if (digitalRead(leftTimeBtn))
  {
    delay(5);
    if (showTimer)  
    {
      if (showFirstNumber) //если показать первое время
      {
        if (firstTime > 0)
        {
          firstTime--;
          firstTimeShow = firstTime;
          address = 0;
          EEPROM.write(address, firstTime);
        }
        int count = 0;
        while (digitalRead(leftTimeBtn))
        {
          if (firstTime > 0)
          {
            if (count > 20)
            {
              count = 0;
              firstTime--;
              firstTimeShow = firstTime;
              address = 0;
              EEPROM.write(address, firstTime);
            }
            displayTime(true, settingTime);
            ++count;
          }
        }
     }
     else   //если показать второе время
     {
      if (secondTime > 0)
        {
          secondTime--;
          secondTimeShow = secondTime;
          address = 1;
          EEPROM.write(address, secondTime);
        }
        int count = 0;
        while (digitalRead(leftTimeBtn))
        {
          if (secondTime > 0)
          {
            if (count > 20)
            {
              count = 0;
              secondTime--;
              secondTimeShow = secondTime;
              address = 1;
              EEPROM.write(address, secondTime);
            }
            displayTime(false, settingTime);
            ++count;
          }
        }
      }
    }
  }
  if (digitalRead(rightTimeBtn))
  {
    delay(5);
    if (showTimer)  
    {
      if (showFirstNumber) //если показать первое время
      {
        if (firstTime < 99)
        {
          firstTime++;
          firstTimeShow = firstTime;
          address = 0;
          EEPROM.write(address, firstTime);
        }
        int count = 0;
        while (digitalRead(rightTimeBtn))
        {
          if (firstTime < 99)
          {
            if (count > 20)
            {
              count = 0;
              firstTime++;
              firstTimeShow = firstTime;
              address = 0;
              EEPROM.write(address, firstTime);
            }
            displayTime(true, settingTime);
            ++count;
          }
        }
     }
     else   //если показать второе время
     {
      if (secondTime < 99)
        {
          secondTime++;
          secondTimeShow = secondTime;
          address = 1;
          EEPROM.write(address, secondTime);
        }
        int count = 0;
        while (digitalRead(rightTimeBtn))
        {
          if (secondTime < 99)
          {
            if (count > 20)
            {
              count = 0;
              secondTime++;
              secondTimeShow = secondTime;
              address = 1;
              EEPROM.write(address, secondTime);
            }
            displayTime(false, settingTime);
            ++count;
          }
        }
      }
    }
  }
  if (digitalRead(pinPower1))
  {
    delay(50);            //дребезг;
    brainRing = true;
    myGame = false;
    beep(gSH, 300);
    beep(cH, 200);
    brainRingPower();
  }
  if (digitalRead(pinPower2))
  {
    delay(50);            //дребезг
    brainRing = false;
    myGame = true;
    beep(cH, 300);
    beep(gSH, 200);
    myGamePower();
  }
  if (showTimer)
  {
    if (showFirstNumber)
      displayTime(true, settingTime); //значения нижнего сдвигового регистра лежат в ledBits 
    else
      displayTime(false, settingTime);
  }
  else    //гасим индикатор
    notDisplayTime();
  
  if (ledsOffFlag)
    ledsOff();
  else if (myGameLedsDefaultFlag)
    myGameLedsDefault();
  else if (brainRingLedsDefaultFlag)
    brainRingLedsDefault();
}

void myMusic() {
  for (int i=0; i<19; i++)
  {
    tone(pinSpeaker, notes[i], 300);
    delay(500);
  }
}

void beep(int ton, int time)
{
  tone(pinSpeaker, ton, time);
  delay(time + 20);
  digitalWrite(pinSpeaker, HIGH);
}

void displayTime(bool firstNumber, bool isSetTime)
{
  int divv, mod;
  ledsOffFlag = false;
  if (firstNumber)
  {
    divv = firstTimeShow / 10;
    mod = firstTimeShow % 10;
  }
  else
  {
    divv = secondTimeShow / 10;
    mod = secondTimeShow % 10;
  }
  digitalWrite(catod1, LOW);
  digitalWrite(catod2, HIGH);
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, digitsFirstTime[divv]);
  shiftOut(dataPin, clockPin, MSBFIRST, ledBits);
  digitalWrite(latchPin, HIGH);
  delay(5);
  digitalWrite(catod1, HIGH);
  digitalWrite(catod2, LOW);
  digitalWrite(latchPin, LOW);
  if (isSetTime)  //isSetTime нужен только для того чтобы показать точку во 2 времени во время настройки таймера
  {
    if (firstNumber) {
      shiftOut(dataPin, clockPin, MSBFIRST, digitsFirstTime[mod]);
      shiftOut(dataPin, clockPin, MSBFIRST, ledBits);
    }
    else {
      shiftOut(dataPin, clockPin, MSBFIRST, digitsSecondTime[mod]);
      shiftOut(dataPin, clockPin, MSBFIRST, ledBits);
    }
  }
  else {
    shiftOut(dataPin, clockPin, MSBFIRST, digitsFirstTime[mod]);
    shiftOut(dataPin, clockPin, MSBFIRST, ledBits);
  }
  digitalWrite(latchPin, HIGH);
  delay(5);
}

void notDisplayTime()
{
  digitalWrite(catod1, HIGH);
  digitalWrite(catod2, HIGH);
}

void ledsOff()
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
  digitalWrite(latchPin, HIGH);
  delay(5);
}

void myGameLedsDefault()
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b00100000);
  digitalWrite(latchPin, HIGH);
  delay(5);
}

void brainRingLedsDefault()
{
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
  shiftOut(dataPin, clockPin, MSBFIRST, 0b01000000);
  digitalWrite(latchPin, HIGH);
  delay(5);  
}

void startTimer()
{
  timerOVFcount = 0;
  showTimer = true;
  TCNT1 = 0;
  TIMSK1 = 0b00000001;
}

void stopTimer()
{
  TCNT1 = 0;    //на всякий случай
  Start = false;
  showTimer = false;
  TIMSK1 &= (0 << TOIE1);
  PCMSK0 = 0b00111111;
  digitalWrite(pinSpeaker, HIGH);
}

void playerAnswer(int pinLed)
{
  TIMSK1 &= (0 << TOIE1); //выключ таймер
  beep(dSH, 500);
  players[pinLed] = false;
  if (brainRing)
  {
    brainRingLedsDefaultFlag = false;
    myGameLedsDefaultFlag = false;
    ledsOffFlag = false;
    if (Start) {  //у команд фальшстарта не было, красный цвет не зажигаем
      digitalWrite(latchPin, LOW);
      if (pinLed == 0) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000010);
        ledBits = 0b01000010;
        for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0x0000ff);     // залить синим
          strip.show();                         // отправить на ленту
        }
      }
      else if (pinLed == 3) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000100);
        ledBits = 0b01000100;
        for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0xff0000);     // залить красным
          strip.show();                         // отправить на ленту
        }
      }
      else if (pinLed == 4) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00001000);
        ledBits = 0b01001000;
        for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0xffff00);     // залить желтым
          strip.show();                         // отправить на ленту
        }
      }  
      else if (pinLed == 5) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00010000);
        ledBits = 0b01010000;
        for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0x00ff00);     // залить зеленый
          strip.show();                         // отправить на ленту
        }
      }  
      digitalWrite(latchPin, HIGH);
    }
    else if (!Start) {  //фальшстарт, зажигаем красный цвет
        beep(gH, 500);
        Start = true;
        digitalWrite(latchPin, LOW);
        if (pinLed == 0) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b10000010);
        ledBits = 0b10000010;
      }
      else if (pinLed == 3) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b10000100);
        ledBits = 0b10000100;
      }
      else if (pinLed == 4) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b10001000);
        ledBits = 0b10001000;
      }
      else if (pinLed == 5) {
        shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
        shiftOut(dataPin, clockPin, MSBFIRST, 0b10010000);
        ledBits = 0b10010000;
      }
      digitalWrite(latchPin, HIGH);

      for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0x800080);     // залить пурпурным
          strip.show();                         // отправить на ленту
        }
    } 
  }
  else if (myGame)
  {
    brainRingLedsDefaultFlag = false;
    myGameLedsDefaultFlag = false;
    ledsOffFlag = false;
    digitalWrite(latchPin, LOW);
    if (pinLed == 0) {
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00100010);
      for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0x0000ff);     // залить синим
          strip.show();                         // отправить на ленту
        }
    }
    else if (pinLed == 3) {
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00100100);
      for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0xff0000);     // залить красным
          strip.show();                         // отправить на ленту
        }
    }
    else if (pinLed == 4) {
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00101000);
      for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0xffff00);     // залить желтым
          strip.show();                         // отправить на ленту
        }
    }
    else if (pinLed == 5) {
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00000000);
      shiftOut(dataPin, clockPin, MSBFIRST, 0b00110000);
      for (int i = 0; i < NUM_LEDS; i++ ) {   
          strip.setPixelColor(i, 0x00ff00);     // залить зеленый
          strip.show();                         // отправить на ленту
        }
    }
    digitalWrite(latchPin, HIGH);
  }
  PCMSK0 = 0b00000110; //разреш старт, сброс  
  delay(5);
}

void brainRingPower()
{
  PCICR = 0b00000001;  //разрешаем прерывания PCINT0;
  PCMSK0 = 0b00111111;  //разрешить все кнопки, старт, стоп
  ledBits = 0b00000000;
  showTimer = false;
  Start = false;
  settingTime = false;  //убираем точку во 2 времени
  firstTimeShow = firstTime;
  secondTimeShow = secondTime;
  for (int i=0; i<6; i++)
    players[i] = true;
  brainRingLedsDefaultFlag = true;
  myGameLedsDefaultFlag = false;
  ledsOffFlag = false;
  stopTimer();
  WS2812_OFF();
}

void myGamePower()
{
  PCICR = 0b00000001;
  PCMSK0 = 0b00111111;  //кнопки, старт, стоп
  showTimer = false;
  brainRingLedsDefaultFlag = false;
  myGameLedsDefaultFlag = true;
  ledsOffFlag = false;
  ledsOffFlag = false;
  for (int i = 0; i < 6; i++)
    players[i] = true;
  stopTimer(); 
}

void WS2812_OFF()
{
  strip.begin();
  strip.setBrightness(50);                // яркость, от 0 до 255
  strip.clear();                          // очистить
  strip.show();                           // отправить на ленту
}

ISR(PCINT0_vect)
{
  if (PINB & (1 << PB1)) //стоп
  {
    PCMSK0 = 0b00000000;  //дребезг
    WS2812_OFF();                          // отправить на ленту
    beep(c, 350);
    int count = 0;
    if (brainRing)
      brainRingPower();
    else if (myGame)
      myGamePower();
    brainRingLedsDefaultFlag = false;
    myGameLedsDefaultFlag = false;
    ledsOffFlag = true;
  }
  if (PINB & (1 << PB2)) //старт
  {
    PCMSK0 = 0b00000000;  //борьба с дребезгом
    WS2812_OFF();         // погасить матрицу 
    beep(aH, 1000);
    if (brainRing)
    {
      if (Start)            //старт был нажат, значит предыдущая команда ответила неправильно
      {
        ledBits = 0b00000000;
        secondTimeShow = secondTime;
        PCMSK0 = 0b00111111;
        for (int i = 0; i < 6; i++)
        if ((i == 1) || (i == 2))
          continue;
          else {
            if (!(players[i]))
            {
              if (i == 0)
                PCMSK0 &= 0b11111110;
              else if (i == 3)
                PCMSK0 &= 0b11110111;
              else if (i == 4)
                PCMSK0 &= 0b11101111;
              else if (i == 5)
                PCMSK0 &= 0b11011111;
            }
          }
        ledBits = 0b01000000;
        showSecondNumber = true;
        showFirstNumber = false;
        brainRingLedsDefaultFlag = false;
        myGameLedsDefaultFlag = false;
        ledsOffFlag = false;
        startTimer();
      }
      else
      {
        ledBits = 0b01000000;
        PCMSK0 = 0b00111111;
        Start = true;
        showSecondNumber = false;
        showFirstNumber = true;
        brainRingLedsDefaultFlag = false;
        myGameLedsDefaultFlag = false;
        ledsOffFlag = false;
        startTimer();
      }
    }
    else if (myGame)
    {
      ledsOffFlag = true;
      PCMSK0 = 0b00111111;
      for (int i = 0; i < 6; i++)
        if ((i == 1) || (i == 2))
          continue;
          else 
          {
            if (!(players[i]))
            {
              if (i == 0)
                PCMSK0 &= 0b11111110;
              else if (i == 3)
                PCMSK0 &= 0b11110111;
              else if (i == 4)
                PCMSK0 &= 0b11101111;
              else if (i == 5)
                PCMSK0 &= 0b11011111;
            }
          }
    }
  }
  
  if (PINB & (1 << PB0)) //1 игрок
    playerAnswer(0);

  if (PINB & (1 << PB3)) //2 игрок
    playerAnswer(3);

  if (PINB & (1 << PB4)) //3 игрок
    playerAnswer(4);

  if (PINB & (1 << PB5)) //4 игрок
    playerAnswer(5);
}


ISR(TIMER1_OVF_vect)
{
  if (timerOVFcount == 500)
  {
    timerOVFcount = 0;
    if (showFirstNumber)
    {
      if (firstTimeShow > 0)
        --firstTimeShow;
      else
      {
        stopTimer();
        firstTimeShow = firstTime;
        secondTimeShow = secondTime;
      }
      if (firstTimeShow == 10)
        beep(eH, 600);
      else if (firstTimeShow == 5)
        beep(g, 500);
      else if (firstTimeShow == 4)
        beep(g, 500);
      else if (firstTimeShow == 3)
        beep(g, 500);
      else if (firstTimeShow == 2)
        beep(g, 500);
      else if (firstTimeShow == 1)
        beep(g, 500);
      else if (firstTimeShow == 0)
        beep(dH, 700);
    }
    else if (showSecondNumber)
    {
      if (secondTimeShow > 0)
        --secondTimeShow;
      else
      {
        stopTimer();
        firstTimeShow = firstTime;
        secondTimeShow = secondTime;
      }
      if (secondTimeShow == 10)
        beep(c, 300);
      if (secondTimeShow == 1)
        beep(d, 300);
      if (secondTimeShow == 0)
        beep(e, 300);
    }
  }
  else{
    ++timerOVFcount;
  }
}
