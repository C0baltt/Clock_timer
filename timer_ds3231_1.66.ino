//================================================================================================================================
//1.64 Добавлен датчик температуры и влажности  DHT 22,
//1.63 Добавлена блокировка клавиатуры. Изменены функции кнопок:
//Set: нажатие переключает режим отображения дисплея, удержание переход в настройки

//Ajust: удержание - блокировка/разблокировка клавиатуры, нажатие в режиме настройки=>
//=>изменение настраиваемого параметра на 1
//1.61_min_func Нажтие кнопок выделяется инверсией. В настройках времени=>
//=> начальным значением выводится текущее. Исправлены мелкие баги
//1.60_min_func Работает настройка двух независимых таймеров, а так же настройка =>
//=> даты и времени
//1.3 Работают два независимых таймера, оба выводятся на дисплей вместе со временем.
//Есть отдельный режим для вывода таймеров на экран, там же добавлен вывод температуры
//модуля времени. Отсчёт таймеров организован путем проверки смены секунд текущего времени
//1.2 Работает два варианта отображения времени на дисплее, реализовано через вызов функций
//1.1 Работает два варианта отображения времени на дисплее, выбирать пока нельзя
//================================
/*
  Подключения к пинам Arduino:
  Кнопки set и ajust: d2, d3;
  Кнопки управления питанием розеток: d7, d8;
  реле: d5, d6
  oled дисплей: a4, a5;
  модуль реального времени DS3231 RTC:
  a4,a5;
  //================================
  памятка по подключению oled дисплея по i2c:
  пины          пины
  дисплея       arduino
  gnd     ==>   -
  vcc     ==>   +
  scl     ==>   a5
  sda     ==>   a4

  памятка по подключению модуля реального времени DS3231 RTC по i2c:
  пины          пины
  модуля        arduino
  +       ==>   +
  d       ==>   a4
  c       ==>   a5
  nc      ==>   не используется
  -       ==>   -

  памятка по подключению датчика температуры и влажности DHT 22
  1       ==>   +
  2       ==>   d10
  3       ==>   не используется
  4       ==>   -

*/
//================================================================================================================================
// Date and time functions using a DS3231 RTC connected via I2C and Wire lib

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "Wire.h"
#include "SoftwareSerial.h"           // Подключаем библиотеку SoftwareSerial.h
#include <iarduino_OLED_txt.h>        // Подключаем библиотеку iarduino_OLED_txt.
#include "RTClib.h"                   // Подключаем библиотеку RTClib.h
#include "GyverButton.h"              // Подключаем библиотеку GyverButton.h

#define DHTPIN 10     // Digital pin connected to the DHT sensor
#define button_pin_set 2              // определяем пин кнопки set
#define button_pin_ajust 3            // определяем пин кнопки ajust
#define electricSocket1_pin 5         // определяем пин розетки 1
#define electricSocket2_pin 6         // определяем пин розетки 2

#define DHTTYPE DHT22

//DHT dht(DHTPIN, DHTTYPE);

DHT_Unified dht(DHTPIN, DHTTYPE);

//8965
RTC_DS3231 rtc;

uint32_t delayMS;

iarduino_OLED_txt myOLED(0x3C);       //Объявляем объект myOLED, =>
//=> указывая адрес дисплея на шине I2C: 0x3C или 0x3D.
GButton buttAjust(button_pin_ajust);  //Объявляем  объект buttAjust класса GButton с адресом пина button_pin_ajust
GButton buttSet(button_pin_set);      //Объявляем объект buttSet класса GButton с адресом пина button_pin_set

extern uint8_t MediumFontRus[];
extern uint8_t SmallFontRus[];
extern uint8_t BigNumbers[];          // Подключаем шрифт BigNumbers
extern uint8_t MegaNumbers[];

//======================== ПЕРЕМЕННЫЕ ======================
//char daysOfTheWeek[7][12] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
//byte daysOfTheWeek[7] = {7, 1, 2, 3, 4, 5, 6}; //массив с номерами дней недели
const char daysOfTheWeek[7][12] = {"ВС", "ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ"};//массив,=>
//=>хранящий названия дней недели
//====================Символьные массивы=======================
const char SETTINGS[] = "НАСТРОЙКИ", TIMER[] = "ТАЙМЕР", SPACE[] = "                            ";
const char HOURS[] = "ЧАСЫ", MINUTE[] = "МИНУЫ", YEARS[] = "ГОДЫ", MONTH[] = "МЕСЯЦЫ", DAYS[] = "ДНИ";
const char SET_HOUR[] = "Уст.час.", SET_MINUTE[] = "Уст.мин.", SET_YEAR[] = "Уст.лет";
const char SET_MONTH[] = "Уст.мес.", SET_DAY[] = "Уст.день";
const char NEXT[] = "ДАЛЕЕ", SAVE[] = "СОХРАНИТЬ", CANCEL[] = "ОТМЕНА";
const char SAVED[] = "СОХРАНЕНО", CANCELED[] = "ОТМЕНЕНО";;
const char CHOOSE_OPTION[] = "Вы выбрали настройки", DATE_AND_TIME[] = "даты и времени:";
const char TIMER_OFF[] = "ТАЙМ.ВЫКЛ";
//=============Переменые кнопок==========
boolean button_ajust_is_pressed;  //флажок срабатывания ajust()(т.е. нажатие засчитано)
boolean button_set_is_pressed;//флажок срабатывания set(т.е. нажатие засчитано)
//==============================================
byte snow, mnow;//
bool showTemp = 0;//флаг, переключающий отображение даты или температуры и влажности
bool var_showTemp;
bool var_bigOrMediumOrMegaNumbers;

bool bigOrMediumOrMegaNumbers = 0;
byte timer_bigOrMediumOrMegaNumbers;//счетчик интервала смены шрифта времени
byte timer_bigOrMediumOrMegaNumbers_max = 6;//интервал смены отображения шрифта времени

//=======================Переменные датчика температуры и влажности=======================
byte humid, temp;//переменные, куда сохраняются значения температуры и влажности
byte timer_showTemp = 0;      //счетчик интервала опроса датчика DHT22
byte timer_showTemp_max = 5; //интервал смены отображения показаний датчика=>
//=> на дату и обратно в секундах

//=============Переменые настроек==========
//далее переменные, используемые для временного хранения времени,=>
//=>которое пользователь задает в настройках перед сохранением его,=>
//=>как актуальное
byte h_settings, m_settings, y_settings, mon_settings, d_settings;//=>
//=>переменные, в которые сохраняются настройки часов, минут, лет, месяцев,=>
//=> дней, а после применяются, как актуальные
byte h_timer1_set, m_timer1_set, h_timer2_set, m_timer2_set;//=>переменные,
//=> в которые сохраняются настройки часов и минут таймеров,
//=> а после применяются, как актуальные
//==============================================
unsigned long startTime;  //сохраняет точное время начала работы системы
boolean flag_settings;    //Флаг, показывающий, что активированы настройки
boolean flagDec = 1;      //флаг, активирующий настройку десятичных разрядов,
// после сброса настраиваются
boolean initialisation;

byte blockCount;//таймер появления информации о блокировке
boolean blockButton;//флаг блокировки кнопок
byte  i;
bool flagCancel, flagSave;

byte number_timeToDisplay = 1;  // переменная, хранящая номер текущего варианта отображения времени
byte number_settingsToDisplay;  // переменная, хранящая номер текущего варианта отображения настроек
const byte max_number_timeToDisplay = 5; // количество вариантов отображения дисплея

//===================переменные таймеров =================
int timer1_h = 0, timer1_m = 0, timer1_s = 0;
int timer2_h = 0, timer2_m = 0, timer2_s = 0;
//===================флаги таймеров =================
boolean timer1_off = 0;//флаг окончания работы таймера
boolean timer1_h_end, timer1_m_end, timer1_s_end;//флаги окончания отсчета соотв. часов, минут, секунд
boolean timer2_off = 0;//флаг окончания работы таймера
boolean timer2_h_end, timer2_m_end, timer2_s_end;//флаги окончания отсчета соотв. часов, минут, секунд

boolean showTimerOff;
//=======================================================================================
void setup () {
  Serial.begin(9600);

  dht.begin();//инитиализация датчика температуры
  //delayMS = sensor.min_delay / 1000;

  sensor_t sensor;
  dht.temperature().getSensor(&sensor);//считываем показания с сенсора
  getParametrFromSensors();

  myOLED.begin();                  // Инициируем работу с дисплеем.
  myOLED.setCursor(20, 4);
  myOLED.setFont(MediumFontRus);
  nightMode();
  myOLED.print(F("ЗАГРУЗКА"));
  buttAjust.setDebounce(80);
  buttSet.setDebounce(80);
  buttAjust.setTimeout(2000);      // настройка таймаута на удержание (по умолчанию 500 мс)
  buttSet.setTimeout(500);        // настройка таймаута на удержание (по умолчанию 500 мс)
  Serial.println(F("Serial.begin"));

#ifndef ESP8266
  while (!Serial);                     // for Leonardo/Micro/Zero
#endif

  delay(3000);                         // wait for console opening
  DateTime now = rtc.now();
  if (! rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }
  //=================================
  //rtc.adjust(DateTime(2020, 12, 31, 23, 59, 59)); //раскомментируй, если надо записать точное время,
  //и далее:
  //1. прошей, чтобы время было актуальным,
  //2. снова закомментируй,
  //3. прошей еще раз, иначе после сброса останется время с первой прошивки

  snow = now.second();//присваиваем переменной snow текущее время
  //==============================================
  pinMode(electricSocket1_pin, OUTPUT);//
  pinMode(electricSocket2_pin, OUTPUT);//
  blockCount = 0;//таймер отсчета времени пока отображается надпись "блокировка"
  blockButton = 1;//включаем блокировку кнопок
  startTime = now.unixtime();        //записываем время начала в переменную startTime
  myOLED.clrScr();
}
//===============================================================================================================================
void loop () {
  for (;;) {//используется бесконечный for т.к. это ускоряет переход
    //=>к следующей итерации
    buttAjust.tick();                  //опрос кнопки для срабатывания
    buttSet.tick();                    //опрос кнопки для срабатывания
    DateTime now = rtc.now();
    //==================================
    if (snow != now.second()) {        //сравнивает текущее время со временем на дисплее,
      snow = now.second();             //в случае изменений =>
      if (timer1_off != 1) {           //=>проверяет включен ли таймер и если включен вызывает timer1()
        countdownTimer1();
      }
      if (timer2_off != 1) {           //проверяет включен ли таймер и если включен вызывает timer2()
        countdownTimer2();
      }
      showTimerOff = !showTimerOff;
      if (now.minute() == 0)
      {
        nightMode();//ночной режим -- включает инверсию дисплея
        if (snow == 30) { //раз в минуту обновляются показания датчика
          getParametrFromSensors();
        }
      }
      timer_showTemp++;//отсчитывает время до смены отображения даты или температуры
      if (timer_showTemp >= timer_showTemp_max) {
        timer_showTemp = 0;
        showTemp = !showTemp;//показывать температуру или дату
      }
      timer_bigOrMediumOrMegaNumbers++;
      if (timer_bigOrMediumOrMegaNumbers >= timer_bigOrMediumOrMegaNumbers_max)
      {
        timer_bigOrMediumOrMegaNumbers  = 0;
        bigOrMediumOrMegaNumbers  = !bigOrMediumOrMegaNumbers ;//показывать температуру или дату
      }
    }
    if (buttAjust.isHolded()) {
      blockButton = !blockButton;//удержание Ajust блокирует/разблокирует кнопки
      myOLED.clrScr();
      //Serial.print(F("blockButton = ")); Serial.print(blockButton);
    }

    sendModeToDisplay();//отображение времени или настроек на дисплее,=>
    //=>в зависимости от режима

    if (blockButton == 0) {//проверка на блокировку кнопок=>
      //=>если рабзлокировано, то идет проверка на нажатия
      if (buttAjust.isClick()) {
        button_ajust_is_pressed = 1;
      }
      if (buttSet.isClick()) {
        button_set_is_pressed = 1;
      }
      if ((flag_settings == 0) && (button_set_is_pressed == 1)) { //если флаг=>
        change_timeToDisplay();    //=> настроек сброшен и кнопка set нажата,=>
        //=>то вызываем change_timeToDisplay()
        button_set_is_pressed = 0;
      }

      if ((flag_settings == 0) && (buttSet.isHold())) activationSettings();//если
      //=> флаг настроек сброшен и флаг нажатия кнопки установлен,=>
      //=>то вызываем activationSettings(), для корректной отработки =>
      //=> set, в зависимости от режима
    }
    //if (((button_set_is_pressed == 1) || (button_ajust_is_pressed == 1) || (blockCount != 0)) && (blockButton == 1))
    //}
  }
  //======================== КОНЕЦ for(;;) ========================
}
//====================================== КОНЕЦ LOOP =========================================================================================
//===============================================================================================================================
//================================= ФУНКЦИИ =======================================================================================
void getParametrFromSensors() {//опрос датчика температуры и влажности
  sensors_event_t event;
  dht.temperature().getEvent(&event);

  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }  else {
    temp = event.temperature;
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }  else {
    humid = event.relative_humidity;
  }
}
//===============================================================================================================================
void nightMode(){//включает ночной режим (инвертирует цвет экрана)
  DateTime now = rtc.now();
  switch (now.hour())
  {
    case 22:
    case 23:
    case 00:
    case 01:
    case 02:
    case 03:
    case 04:
    case 05:
    case 06:
      myOLED.invScr(true);
      break;
    default:
      myOLED.invScr(false);
      break;
  }
}
//===========================================================================================
void zero(int parametr) { //(byte x; byte y; int parametr){//если параметр =>
  //=> меньше 10, то перед числом подставляет 0, чтобы сохранить двузначность,=>
  //=> чтобы надпись не съезжала
  if (parametr < 10) {
    myOLED.print(0);
  }
  myOLED.print(parametr);
}
//============================ВЫВОД ИЗОБРАЖЕНИЯ==============================================
void timer1ToDisplay(byte x, byte y) {         //выводит состояние таймера=>
  //=>верху дисплея мелким шрифтом
  myOLED.setCursor(x, y);
  if (timer1_off == 1) {
      if (showTimerOff == 1) {
        myOLED.print(TIMER_OFF);//(TIMER_OFF, x, y); //"ТАЙМ.ВЫКЛ"
      }else {
    myOLED.print("         ");
      }
  } else {
    //myOLED.setFont(SmallFontRus);
    //вывод таймера
    myOLED.print(" ");//добавлен пробел, чтоб вывод времени занимал то же положение, что "ТАЙМ.ВЫКЛ"
    zero(timer1_h);

    //myOLED.print(timer1_h);       //часов
    myOLED.print(":");              //":"

    zero(timer1_m);

    //myOLED.print(timer1_m);       //минут
    myOLED.print(":");

    zero(timer1_s);

    //myOLED.print(timer1_s);       //секунд
  }
}
//=======================================================================================
void timer2ToDisplay(byte x, byte y) {            //выводит состояние таймера верху дисплея мелким шрифтом
  myOLED.setCursor(x, y);
  if (timer2_off == 1) {
    //myOLED.setFont(SmallFontRus);
    if (showTimerOff == 1) {
        myOLED.print(TIMER_OFF);//(TIMER_OFF, x, y); //"ТАЙМ.ВЫКЛ"
      }else {
    myOLED.print("         ");
      }
    //myOLED.print(TIMER_OFF);
  } else {
    //myOLED.setFont(SmallFontRus);

    //вывод таймера
    myOLED.print(" ");//добавлен пробел, чтоб вывод времени занимал то же положение, что "ТАЙМ.ВЫКЛ"
    zero(timer2_h);
    /*
      if (timer2_h < 10) {
      myOLED.print(0);
      } */
    //myOLED.print(timer2_h);       //часов
    myOLED.print(":");              //":"

    zero(timer2_m);
    /*if (timer2_m < 10) {
      myOLED.print(0);
      }*/

    //myOLED.print(timer2_m);       //минут
    myOLED.print(":");

    zero(timer2_s);
    /*if (timer2_s < 10) {
      myOLED.print(0);
      } */
    //myOLED.print(timer2_s);       //секунд
  }
}
//============================================================================
void tempAndHumidToDisplay(byte x, byte y)    {
  myOLED.setCursor(x, y);
  myOLED.print(F("T="));
  myOLED.print(temp);
  myOLED.print(F(" "));

  myOLED.print(F("H="));
  myOLED.print(humid);
  myOLED.print(F("%"));
}
//===========РЕЖИМЫ ОТОБРАЖЕНИЯ ИНФОРМАЦИИ=======================
void dataToDisplay (byte x, byte y) { //вывод даты
  //вывод на эран: день недели число.месяц.год шрифт MediumFontRus (средний)
  //======================       Дата                //======================
  DateTime now = rtc.now();
  myOLED.setFont(MediumFontRus);    //установка шрифта SmallFontRus,варианты:SmallFontRus MediumFontRus BigNumbers MegaNumbers
  myOLED.setCursor(x, y);           //это верхняя строка (9, 3) (лево-право,верх-низ)
  myOLED.print(daysOfTheWeek[now.dayOfTheWeek()]);//вывод номера дня недели
  myOLED.setFont(SmallFontRus);
  myOLED.print(" ");
  myOLED.setFont(MediumFontRus);

  zero(now.day());
  /*if (now.day() < 10) {
    myOLED.print(0);
    }*/
  //myOLED.print(now.day());        //даты
  myOLED.setFont(SmallFontRus);
  myOLED.print(".");
  myOLED.setFont(MediumFontRus);

  zero(now.month());
  /*if (now.month() < 10) {
    myOLED.print(0);
    }*/
  //myOLED.print(now.month());      //месяца
  myOLED.setFont(SmallFontRus);
  myOLED.print(".");
  myOLED.setFont(MediumFontRus);
  myOLED.print(now.year() % 2000);  //года (взят остаток от деления, потому что полностью не влазит)
}
//============================================================================
void timeToDisplay (byte x, byte y, bool showSeconds) {
  DateTime now = rtc.now();
  myOLED.setCursor(x, y);           //вывод на экран: myOLED.setCursor(3,7);(лево-право,верх-низ)

  zero(now.hour());

  myOLED.print(".");                //"."

  zero(now.minute());

  if (showSeconds == 1) {
    myOLED.print(".");
    myOLED.setFont(MediumFontRus);

    zero(now.second());
  }
}
//============================================================================
void timeMode1ToDisplay() {// вывод на эран верхняя строка:день недели число.месяц.год шрифт MediumFontRus (средний)
  //нижняя строка:часы.минуты.секуды шрифт MediumFontRus (средний)
  DateTime now = rtc.now();
  myOLED.setFont(SmallFontRus);
  timer1ToDisplay(5, 1);                //вывод таймера 1
  timer2ToDisplay(66, 1);                //вывод таймера 2
  //myOLED.setFont(BigNumbers);
  myOLED.setFont(MediumFontRus);

  if (var_showTemp != showTemp) {
    myOLED.print(SPACE, 1, 3);
    var_showTemp = showTemp;
  }

  if (showTemp == 1) {
    tempAndHumidToDisplay(3, 3);//(showTemp == 0)
  } else {
    dataToDisplay (8, 3); //вызов отображения даты на дисплей (byte x, byte y)
  }


  if (var_bigOrMediumOrMegaNumbers != bigOrMediumOrMegaNumbers) {
    //myOLED.print(SPACE, 3, 4);
    //myOLED.print(SPACE, 3, 5);
    myOLED.print(SPACE, 3, 6);
    myOLED.print(SPACE, 3, 7);
    var_bigOrMediumOrMegaNumbers = bigOrMediumOrMegaNumbers;
  }
  
  if (bigOrMediumOrMegaNumbers == 0)
  {
    myOLED.setFont(BigNumbers);
    timeToDisplay (9, 7, 1);//(положение на экране: x - по горизонтали, =>
  // => y - по вертикали, showSeconds - флаг отображения секунд)
  } else {
    myOLED.setFont(MediumFontRus);
    timeToDisplay (17, 6, 1);//(положение на экране: x - по горизонтали, =>
  // => y - по вертикали, showSeconds - флаг отображения секунд)
  }
  
}
//======================                          //======================
//============================================================================
void timeMode2ToDisplay() {// вывод на эран верхняя строка: день недели-число.месяц.год шрифт MediumFontRus (средний)
  //нижняя строка: часы.минуты шрифт MegaNumbers (огромный)
  DateTime now = rtc.now();
  myOLED.setFont(SmallFontRus);
  timer1ToDisplay(0, 0);                 //вывод таймера 1
  timer2ToDisplay(68, 0);                //вывод таймера 2
  myOLED.setFont(MediumFontRus);         //установка шрифта MediumFontRus,варианты:SmallFontRus MediumFontRus BigNumbers MegaNumbers

  if (var_showTemp != showTemp) {
    myOLED.print(SPACE, 1, 2);
    var_showTemp = showTemp;
  }

  if (showTemp == 1) {
    tempAndHumidToDisplay(3, 2);
  } else {
    dataToDisplay (8, 2); //вызов отображения даты на дисплей (byte x, byte y)
  }

  if (var_bigOrMediumOrMegaNumbers != bigOrMediumOrMegaNumbers) {
    myOLED.print(SPACE, 3, 4);
    myOLED.print(SPACE, 3, 5);
    myOLED.print(SPACE, 3, 6);
    myOLED.print(SPACE, 3, 7);
    var_bigOrMediumOrMegaNumbers = bigOrMediumOrMegaNumbers;
  }

  if (bigOrMediumOrMegaNumbers == 0)
  {
    myOLED.setFont(MegaNumbers);
    timeToDisplay (5, 7, 0);
  } else {
    myOLED.setFont(BigNumbers);
    timeToDisplay (30, 6, 0);
  }
}
//==================================
void diagnosticsToDisplay() {
  //byte  x = 30;                        //положение таймеров на дисплее по оси x
  DateTime now = rtc.now();
  //вывод температуры
  //  getParametrFromSensors();
  myOLED.setFont(SmallFontRus);

  if (var_showTemp != showTemp) {
    myOLED.print(SPACE, 0, 0);
    var_showTemp = showTemp;
  }
  if (showTemp == 1) {
    tempAndHumidToDisplay(35, 0);
  } else {
    myOLED.print(F("Темп. RTC "), 15, 0);
    myOLED.print(rtc.getTemperature());
    myOLED.print(F(" C"));
  }
  //myOLED.print(F("Темп. "));
  //======================================
  myOLED.setFont(MediumFontRus);
  timeToDisplay (20, 2, 1);
  //=============вывод таймеров на дисплей
  myOLED.setFont(MediumFontRus);
  timer1ToDisplay(8, 5);
  timer2ToDisplay(8, 7);
}
//=======================================================================================
void timeHasPassedToDisplay() {
  int d, mon, h, m, s;//переменные даты и времени
  byte y = 1;//
  unsigned long  timeHasPassed;
  DateTime now = rtc.now();
  timeHasPassed = now.unixtime() - startTime;

  //сколько времени прошло со старта, расчет:
  d = timeHasPassed / 86400;                  //дней
  h = ((timeHasPassed % 86400) / 3600);       //часов
  m = ((timeHasPassed % 3600) / 60);          //минут
  s = timeHasPassed % 60;                     //секунд

  //вывод текущего времени
  myOLED.setFont(MediumFontRus);              //установка шрифта MediumFontRus
  timeToDisplay (18, y, 1);

  //вывод сколько времени прошло со старта
  myOLED.setCursor(7, y + 6);

  zero(d);
  /*if (d < 10) {
    myOLED.print(0);
    }*/
  //myOLED.print(d);                            //дней прошло со старта
  myOLED.setFont(SmallFontRus); myOLED.print(":");
  myOLED.setFont(MediumFontRus);

  zero(h);
  /*if (h < 10) {
    myOLED.print(0);
    }*/

  //myOLED.print(h);                         //часов прошло со старта

  myOLED.setFont(SmallFontRus); myOLED.print(":");
  myOLED.setFont(MediumFontRus);

  zero(m);
  /*if (m < 10) {
    myOLED.print(0);
    }*/

  //myOLED.print(m);                           //минут прошло со старта

  myOLED.setFont(SmallFontRus); myOLED.print(":");
  myOLED.setFont(MediumFontRus);

  zero(s);
  /*if (s < 10) {
    myOLED.print(0);
    }*/
  //myOLED.print(s);                           //секунд прошло со старта

  myOLED.setCursor(0, y + 2);
  myOLED.setFont(SmallFontRus);
  myOLED.print(timeHasPassed);//вывод сколько времени прошло со старта в секундах

  myOLED.setCursor(13, y + 4);
  myOLED.setFont(SmallFontRus);
  myOLED.print(F("ВРЕМЕНЯ СО СТАРТА"));
}
//==========================================================================
void change_SettingsToDisplay() {           //смена режима отображения дисплея
  byte max_number_settingsToDisplay = 11;   //количество вариантов отображения дисплея
  number_settingsToDisplay++;
  button_set_is_pressed = 0;
  if (number_settingsToDisplay > max_number_settingsToDisplay) {
    number_settingsToDisplay = 1;
    flag_settings = 0;
  }
  myOLED.clrScr();
}
//==========================================================================
void send_timeToDisplay() {//вызов одного из вариантов отображения времени в зависмости от=>
  //=>переменной number_timeToDisplay, которую определяет функция change_timeToDisplay()
  switch (number_timeToDisplay) {
    case 1:
      timeMode1ToDisplay();
      break;
    case 2:
      timeMode2ToDisplay();
      break;
    case 3:
      diagnosticsToDisplay();
      break;
    case 4:
      timeHasPassedToDisplay();
      break;
  }
}
//=========================================================================
void change_timeToDisplay() {               //смена режима отображения дисплея
  byte max_number_timeToDisplay = 4;        //количество вариантов отображения дисплея
  number_timeToDisplay++;                   //переключаем режим отображения дисплея
  button_set_is_pressed = 0;                //сбрасываем флаг нажатия кнопки
  if (number_timeToDisplay > max_number_timeToDisplay) number_timeToDisplay = 1;//=>
  //=>если номер режима больше количества режимов, то приравниваем к единице
  myOLED.clrScr();//очистка дисплея, т.к. при смене режима остаются артефакты
}
//=============================================== ТАЙМЕРЫ: ОТСЧЁТ ВРЕМЕНИ ================================
void countdownTimer1() {                         //таймер 1: отсчёт времени
  if (timer1_off == 0) {                         //если таймер включен, то
    digitalWrite(electricSocket1_pin, HIGH);     //подаем питание на розетку
    timer1_s--;                                  //декремент секунд
    if (timer1_s < 0) {                          //если секунды <0,
      timer1_s = 59; timer1_s_end = 1; timer1_m--; //устанавливаем флаг, что секунды закончились, декремент минут
      if (timer1_m < 0) {
        timer1_m = 59; timer1_m_end = 1; timer1_h--; //аналогично секундам
        if (timer1_h < 0) {
          timer1_h = 0; timer1_h_end = 1;
        }
      }
    }
  }
  if (timer1_h_end == 1 && timer1_m_end == 1 && timer1_s_end == 1) { //проверяем =>
    //=>флаги окончания отчёта часов, =>
    //=>минут и секунд: если все установлены,=>
    //=>значит отсчёт завершён и ==>
    digitalWrite(electricSocket1_pin, LOW);
    timer1_off = 1;//==>устанавливаем флаг окончания отсчёта
    timer1_h = 0;
    timer1_m = 0;
    timer1_s = 0;
    myOLED.clrScr();
  }
}
//=======================================================================================
void countdownTimer2() {                                   //таймер 2: отсчёт времени
  if (timer2_off == 0) {
    digitalWrite(electricSocket2_pin, HIGH);
    timer2_s--;
    if (timer2_s < 0) {
      timer2_s = 59; timer2_s_end = 1; timer2_m--;
      if (timer2_m < 0) {
        timer2_m = 59; timer2_m_end = 1; timer2_h--;
        if (timer2_h < 0) {
          timer2_h = 0; timer2_h_end = 1;
        }
      }
    }
  }
  if (timer2_h_end == 1 && timer2_m_end == 1 && timer2_s_end == 1) {
    digitalWrite(electricSocket2_pin, LOW);
    timer2_off = 1;
    timer2_h = 0;
    timer2_m = 0;
    timer2_s = 0;
    myOLED.clrScr();
  }
}
//==========================================================================
void activationSettings() {              //определяет, когда активируются настройки
  if (buttSet.isHolded()) {              //если кнопка set нажата, то
    if (flag_settings == 0) {            //если флаг настроек сброшен=>
      //=>(это значит, что меню настроек не активно), то
      flag_settings = 1;                 //устанавливаем флаг настроек(выводим начальный экран настроек)
      //button_set_is_pressed = 0;       //сбрасываем флаг нажатия кнопки set
      myOLED.clrScr();
      //проверяем в каком режиме дисплей(режим часов или таймера),
      //чтобы вывести соответствующие настройки:
      if (number_timeToDisplay < 3) {    //режим 1 и 2  (отображение времени)
        number_settingsToDisplay = 1;    //выводим настройки времени
      } else if (number_timeToDisplay == 3 || number_timeToDisplay == 4) { //режим таймеров
        number_settingsToDisplay = 7;    //выводим настройки таймеров
      } /*else if (number_timeToDisplay == 4) { //
        myOLED.clrScr();
        myOLED.setFont(MediumFontRus);
        myOLED.print(F("Нет"), 40, 3);
        myOLED.print(F("настроек"), 15, 5);
        if (button_ajust_is_pressed == 1) {
          button_ajust_is_pressed = 0;
          number_timeToDisplay = 1;
          flag_settings = 0;
        }
      }*/
      else {
        Serial.println(F("Неизвестный пункт настроек"));
      }
    }
  }
}
//==============================================================================
byte howManyDaysInMonth(byte month1) {              //эта функция определяет максимальное =>
  //=>значение для количества дней, в зависимости от месяца выбранного в настройках
  switch (month1) {

    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      //if ((month1 == 1) || (month1 == 3) || (month1 ==  5) || (month1 == 7) || (month1 == 8) || (month1 == 10) || (month1 == 12)) { // месяцы, где 31 день
      return 31;
      break;

    case 4:
    case 6:
    case 9:
    case 11:
      //} else if ((month1 == 4) || (month1 == 6) || (month1 == 9) || (month1 == 11)) {         //месяцы, где 30 дней
      return 30;
      break;

    case 2:
      //} else if (month1 == 2) {               //февраль
      if ((y_settings == 2024) || (y_settings == 2028) || (y_settings == 2032) || (y_settings == 2036)) {
        return 29;                           //високосный год
      } else {
        return 28;                           //обычный год
      }
      break;

    default:
      Serial.println(F("Неизвесный месяц"));
      break;
  }
}
//==============================================================================
void saveTimers() {//эта функция сохраняет либо отменяет значения таймеров, =>
  //=> которые задал пользователь в режиме настройки
  byte  x = 50;
  if (i == 0) {//если i == 0, то это значит, что функция проходит первую =>
    //=>итерацию и в текущем цикле на дисплей разово будет выведена информация=>
    //=> о выбранных настройках и вариантах действий пользователя. =>
    //=>Дальнейшее обновление дисплея не требуется

    //вывод изображения на дисплей
    myOLED.setFont(SmallFontRus);
    myOLED.print(F("ТАЙМЕРЫ"), 45, 0);             //"ТАЙМЕРЫ"
    //myOLED.print(SPACE);                   //"                 "
    myOLED.print(CHOOSE_OPTION, 0, 2);       //"Вы выбрали настройки "
    myOLED.print(F("таймеров:"), 0, 3);      //"таймеров:"

    myOLED.setCursor(x, 4);
    if (h_timer1_set < 10) myOLED.print(0);
    myOLED.print(h_timer1_set); myOLED.print(":");
    if (m_timer1_set < 10) myOLED.print(0);
    myOLED.print(m_timer1_set);
    myOLED.setCursor(x, 5);
    if (h_timer2_set < 10) myOLED.print(0);
    myOLED.print(h_timer2_set); myOLED.print(":");
    if (m_timer2_set < 10) myOLED.print(0);
    myOLED.print(m_timer2_set);

    myOLED.print(SAVE, 6, 6); myOLED.print(" ");//Сохранить
    myOLED.print(SETTINGS);                 //"НАСТРОЙКИ"
    myOLED.print("?");                      //?
    //myOLED.print(SPACE);                    //"               "

    //myOLED.setFont(SmallFontRus);
    myOLED.print(SAVE, 0, 7);               //"СОХРАНИТЬ"
    //myOLED.print(SPACE);                    //"               "
    myOLED.print(CANCEL, 90, 7);            //"Отмена"
    //myOLED.print(SPACE);                    //"               "
  }

  if (((button_ajust_is_pressed == 1) || (flagCancel == 1)) && (flagSave != 1)) {     //отмена изменений
    button_ajust_is_pressed = 0;
    flagCancel = 1;
    myOLED.setFont(MediumFontRus);
    if (i == 0) myOLED.clrScr();
    myOLED.invScr(i > 20);//через 20 циклов включает инверсию дисплея
    myOLED.print(CANCELED, 15, 4);         //"ОТМЕНЕНО"
    i++;//счетчик ожидания для того, чтоб успеть рассмотреть надпись
    if (i > 40) {//проверка счетчика
      i = 0;
      flagCancel = 0;
      flag_settings = 0;
      myOLED.clrScr();
      //myOLED.invScr(false);
      nightMode();
    }
  }

  if (((button_set_is_pressed == 1) || (flagSave == 1)) && (flagCancel != 1)) { //сохранение изменений
    button_set_is_pressed = 0;
    flagSave = 1;
    myOLED.setFont(MediumFontRus);
    if (i == 0) myOLED.clrScr();
    myOLED.invScr(i > 20);//через 20 циклов включает инверсию дисплея
    myOLED.print(SAVED, 10, 4);            //"СОХРАНЕНО"
    i++;//счетчик ожидания для того, чтоб успеть рассмотреть надпись

    //сохраняем настройки из временной переменной в переменные таймеров:
    //и просматриваем выбранные значения
    if (i > 40) {//проверка счетчика
      //таймер 1:
      timer1_h = h_timer1_set;
      if (timer1_h > 0) timer1_h_end = 0;//если значение часов больше 0, то=>
      timer1_m = m_timer1_set;        //=>сбрасываем флаг окончания отсчета часов
      if (timer1_m > 0) timer1_m_end = 0;// анологично для минут
      timer1_s = 0;
      if ((timer1_h_end == 1) && (timer1_m_end == 1) && (timer1_s_end == 1)) {
        timer1_off = 1;//если флаги проверки нулевых значений всех переменных=>
        //=>установлены, то это значит, что таймер не настроен, устанавливаем=>
        //=>флаг отключения таймера
      } else {//иначе сбрасываем флаг отключения, это значит, что таймер=>
        timer1_off = 0;//=> настроен и начнет работать
      }
      //таймер 2:
      timer2_h = h_timer2_set;
      if (timer2_h > 0) timer2_h_end = 0;//если значение часов больше 0, то=>
      timer2_m = m_timer2_set;//=>сбрасываем флаг окончания отсчета часов
      if (timer2_m > 0) timer2_m_end = 0;// анологично для минут
      timer2_s = 0;
      if ((timer2_h_end == 1) && (timer2_m_end == 1) && (timer2_s_end == 1)) {
        timer2_off = 1;//если флаги проверки нулевых значений всех переменных=>
        //=>установлены, то это значит, что таймер не настроен, устанавливаем=>
        //=>флаг отключения таймера
      } else {//иначе сбрасываем флаг отключения, это значит, что таймер=>
        timer2_off = 0;//=> настроен и начнет работать
      }
      i = 0;
      flagSave = 0;
      flag_settings = 0;   //сбрасываем флаг настроек, что влечет возврат в обычный режим
      myOLED.clrScr();
      nightMode();
      //myOLED.invScr(false);
    }
  }
}
//==============================================================================
void send_SettingsToDisplay() {            //вызов одного из вариантов настроек, в зависимости от переменной
  DateTime now = rtc.now();

  switch (number_settingsToDisplay) {      //h_settings, m_settings, ;
    case 1:                                //настройка времени и даты:
      y_settings = settingTimeToDisplay(now.year() % 100,  99, YEARS, SET_YEAR); //год  byte (now.year()
      break;
    case 2:
      mon_settings = settingTimeToDisplay(now.month(),  12, MONTH, SET_MONTH);//месяц
      break;
    case 3:
      d_settings = settingTimeToDisplay(now.day(), howManyDaysInMonth(mon_settings), DAYS, SET_DAY);//день
      break;
    case 4:
      h_settings = settingTimeToDisplay(now.hour(),  23, HOURS, SET_HOUR);//часы
      break;
    case 5:
      m_settings = settingTimeToDisplay(now.minute(),  59, MINUTE, SET_MINUTE);//минуты
      break;
    case 6:
      saveSettings();
      break;
    //==============Настройка таймеров===========
    case 7:
      h_timer1_set = settingTimeToDisplay(timer1_h,  23, HOURS, SET_HOUR);//часы
      break;
    case 8:
      m_timer1_set = settingTimeToDisplay(timer1_m,  59, MINUTE, SET_MINUTE);//минуты
      break;
    case 9:
      h_timer2_set = settingTimeToDisplay(timer2_h,  23, HOURS, SET_HOUR);//часы
      break;
    case 10:
      m_timer2_set = settingTimeToDisplay(timer2_m,  59, MINUTE, SET_MINUTE);//минуты
      break;
    case 11:
      saveTimers();
      break;
  }
}
//==============================================================================
void sendModeToDisplay() {               //в зависимости от режима выводит=>
  //=>на дисплей время, или настройки

  if ( (blockButton == 1) && ( (buttSet.isClick()) || (buttAjust.isClick()) || (blockCount != 0) ) ) {//=>
    //=>если блокировка включена, то
    //button_set_is_pressed = 0;
    //button_ajust_is_pressed = 0;
    if (blockCount == 0) { //если счетчик
      myOLED.clrScr();//
    }

    blockCount++;

    myOLED.setFont(MediumFontRus);
    myOLED.print(F("БЛОКИРОВКА"), 0, 1);
    myOLED.setFont(SmallFontRus);
    myOLED.print(F("Чтобы разблокировать,"), 0, 3);
    myOLED.print(F("удерживайте"), 0, 5);
    //myOLED.print(F("''Снять блокировку''"), 0, 5);
    myOLED.invText(buttAjust.state());//цвет следующей надписи будет инвертирован, пока=>
    //=>удерживается кнопка Ajust
    /*if (butt1.isHolded()) Serial.println("Holded");       // проверка на удержание
      if (butt1.isHold()) Serial.println("Holding");        // проверка на удержание
      //if (butt1.state()) Serial.println("Hold");          // возвращает состояние кнопки
    */
    myOLED.print(F("Снять блокировку"), 30, 7);

    if (blockCount == 100) {//
      blockCount = 0;
      myOLED.clrScr();
    }
  } else { /*if (blockButton == 0)*/     //если блокировка отключена, то в=>
    //=>зависимости от режима выводит=>
    //=>на дисплей время, или настройки
    if (flag_settings == 1) {
      send_SettingsToDisplay();
    } else {
      send_timeToDisplay();
    }
  }
  myOLED.invText(false);
}
//==========================================================================
void saveSettings() {//эта функция сохраняет, либо отменяет настройки даты и времени, =>
  //=>заданные пользователем в режиме настройки
  /*  static byte  i;
    static bool flagCancel, flagSave;*/
  if (i == 0) {
    myOLED.setFont(SmallFontRus);
    myOLED.print(SETTINGS, 40, 0);//"НАСТРОЙКИ"

    myOLED.print(CHOOSE_OPTION, 0, 2);//"Вы выбрали следующие настройки"
    myOLED.print(DATE_AND_TIME, 0, 3);//"даты и времени:"          "

    myOLED.print(y_settings, 10, 4); myOLED.print("."); myOLED.print(mon_settings);
    myOLED.print("."); myOLED.print(d_settings);
    myOLED.print(h_settings, 10, 5); myOLED.print(":"); myOLED.print(m_settings);

    myOLED.print(SAVE, 30, 6); myOLED.print("?");//Сохранить?

    myOLED.print(SAVE, 0, 7);//"СОХРАНИТЬ"
    myOLED.print(CANCEL, 90, 7);//"Отмена"
  }
  if ((button_ajust_is_pressed == 1) || (flagCancel == 1)) {//отмена сохранения настроек
    button_ajust_is_pressed = 0;
    flagCancel = 1;
    if (i == 0) myOLED.clrScr();
    myOLED.setFont(MediumFontRus);
    myOLED.invScr(i > 20);
    myOLED.print(CANCELED, 15, 4);//"ОТМЕНЕНО"
    nightMode();
    //myOLED.invScr(false);
    i++;
    if (i > 40) {
      i = 0;
      flagCancel = 0;
      flag_settings = 0;
      myOLED.clrScr();
    }
  }
  if ((button_set_is_pressed == 1) || (flagSave == 1)) {//сохранение настроек времени и даты
    button_set_is_pressed = 0;
    flagSave = 1;
    if (i == 0) {
      DateTime now = rtc.now();
      rtc.adjust(DateTime(y_settings, mon_settings, d_settings, h_settings, m_settings, 00));//=>
      //=> сохраняем выбранные настройки в модуль реального времени
      myOLED.clrScr();
    }
    myOLED.setFont(MediumFontRus);
    myOLED.invScr(i > 20);
    myOLED.print(SAVED, 10, 4);//"Сохранено"
    //myOLED.invScr(false);
    nightMode();
    i++;
    if (i > 40) {
      i = 0;
      flagSave = 0;
      flag_settings = 0;//сбрасываем флаг активации настроек и переходим в обычный режим
      myOLED.clrScr();
      nightMode();
    }
  }
}
//==========================================================================
byte settingTimeToDisplay(byte setParametr, byte setParametrMax, char name1[], char name2[]) { //=>
  //=>настраивает дату и время
  byte  decMax = setParametrMax / 10;// максимальное значение дсятков
  static byte dec, one; //переменные, хранящие десятичные разряды и разряды едениц
  //bool blinkAjust = 0, blinkSet = 0;//от состояния флага зависит подсветка нажатия =>
  //=> кнопки на экране
  if (initialisation == 0) {//при настройке новой переменной записываем ее=>
    dec = setParametr / 10;//=>десятки в переменную dec,=>
    one = setParametr % 10;//=>еденицы в переменную one
    initialisation = 1;
  }
  if (button_ajust_is_pressed == 1) { //если кнопка ajust была нажата, то:
    if (flagDec == 1) { //если флаг десятков часов активирован, то к переменной
      dec ++; //dec(десятки часов) прибавляем 1
      if (dec > decMax) dec = 0; //если > макс. допустимого значения, то приравниваем к 0
    } else { //если флаг десятков часов сброшен, то=>
      one ++;//=>прибавляем 1 к one
      if (one  > 9) one  = 0; //если one > 9, то приравниваем к 0
      if ((setParametr = ((dec * 10) + one )) > setParametrMax) {//проверка, =>
        //=>чтобы сумма десятков и едениц часов не превышала setParametrMax
        one = 0; //если превышает, то приравниваем one к 0
        //проверяем режим настроек:
        if ((dec == 0) && ((number_settingsToDisplay == 2) || (number_settingsToDisplay == 3))) {
          one = 1; //если настраиваем месяцы или дни, то понятно, что их отсчет не =>
          //=>начинатеся с нуля, поэтому приравниваем к единице
        }
      }
    }
    if (button_ajust_is_pressed == 1) {
      button_ajust_is_pressed = 0; //сбрасываем флаг нажатия ajust
      //      blinkAjust = 1;
    }
  }
  //========================
  if (button_set_is_pressed == 1 && flagDec == 1) { //если КНОПКА нажата и =>
    //=>флаг десятков активирован
    button_set_is_pressed = 0; //сбрасываем флаг срабатывания кнопки set
    //    blinkSet = 1;
    flagDec = 0;//это значит, что десятки настроены, переходим к еденицам
  }
  //следующий блок отвечает за сохранение настроек
  if (button_set_is_pressed == 1 && flagDec == 0) {//если кнопка set нажата и=>
    //=> флаг десятков сброшен, значит текущий параметр настроен
    button_set_is_pressed = 0;// сбрасываем флаг нажатия set
    setParametr = ((dec * 10) + one );//переменную десятков * 10 и прибавляем единицы,=>
    //=> результат сохраняем в переменной setParametr
    flagDec = 1;//устанавливаем флаг десятков
    dec = 0;//сбрасываем значение десятков
    one = 0;//сбрасываем значение едениц
    initialisation = 0;
    change_SettingsToDisplay();//вызываем функцию смены режима настроек
    return (setParametr);
  }
  //вывод настроек на экран
  myOLED.setFont(SmallFontRus);
  // myOLED.setCursor(0, 0);
  //myOLED.print(SPACE);//"               "
  if (number_settingsToDisplay < 7) {//в зависимости от типа number_settingsToDisplay=>
    //=>выводит название "НАСТРОЙКИ"=>
    //=> или "ТАЙМЕР" вверху экрана
    myOLED.print(SETTINGS, 40, 0);//"НАСТРОЙКИ"
  } else {
    myOLED.print(TIMER, 45, 0); //"ТАЙМЕР"
    if ((number_settingsToDisplay == 7) || (number_settingsToDisplay == 8)) {//=>
      //=>если настраиваем первый таймер, то
      myOLED.print(1);//выводит 1
    } else { //иначе
      myOLED.print(2);// выводит 2
    }
    //myOLED.print(SPACE);//"               "
  }
  myOLED.print(name1, 50, 2);//""
  //myOLED.print(SPACE);//"               "
  myOLED.setFont(MediumFontRus);
  if (flagDec == 1) {//если флаг настройки десятков активирован, то
    myOLED.invText(true);//инвертируем цвет десятков(это значит, что настраиваем десятки)
  }
  myOLED.print(dec, 0, 5);//46, 5
  myOLED.invText(false);//отключаем инверсию десятков
  if (flagDec == 0) {//если флаг настроек сброшен, значит настраиваем единицы
    myOLED.invText(true);//инвертируем цвет единиц (это значит, что настраиваем единицы)
  }
  myOLED.print(one, 12, 5); //58, 5
  myOLED.invText(false);//отключаем инверсию единиц
  myOLED.setFont(SmallFontRus);
  myOLED.invText(buttSet.state());//выделяет инверсией название кнопки, при ее нажатии
  myOLED.print(NEXT, 0, 7);//"ДАЛЕЕ"
  myOLED.invText(false);
  myOLED.invText(buttAjust.state());//выделяет инверсией название кнопки, при ее нажатии
  myOLED.print(name2, 80, 7);//"Уст.параметр"
  myOLED.invText(false);
  //=======================
  myOLED.print(y_settings, 30, 4); myOLED.print("."); myOLED.print(mon_settings);
  myOLED.print("."); myOLED.print(d_settings);
  myOLED.print(SPACE);//"               "
  myOLED.print(h_settings, 30, 5); myOLED.print(":"); myOLED.print(m_settings);
  //myOLED.print(SPACE);//"
  myOLED.print(setParametr, 70, 5);
  myOLED.print(dec, 90, 5); myOLED.print(one);

  //  blinkAjust = 0;
  //  blinkSet = 0;
  //=======================


  // Примерный вид экрана
  // ____________________
  //|      НАСТРОЙКИ     | вывод надписи "НАСТРОЙКИ"
  //|        ГОДЫ        | вывод надписи "ГОДЫ"
  //|         20         | вывод переменной h_settings
  //|                    |
  //|ДАЛЕЕ        Уст.лет| вывод надписи "ДАЛЕЕ      Уст.лет
  // --------------------
  //   set         ajust
  //    O            O


}
//==============================================================================
//=======================================================================================
// конец программы
/*========================================================================
  Функция begin(); // Инициализация работы с дисплеем.

  Функция clrScr( [ ЗАЛИТЬ] ); // Очистка экрана дисплея (параметр - флаг разрешающий залить дисплей).

  Функция fillScr( [ ЦВЕТ] ); // Заливка дисплея (параметр - цвет. 0-чёрный, 1-белый).
  Функция invScr( [ ФЛАГ] ); // Инверсия цветов экрана (параметр - флаг разрешающий инверсию).
  Функция invText( [ ФЛАГ ] ); // Инверсия цветов выводимого текста(параметр - флаг разр. инверсию).
  Функция setFont( ШРИФТ ); // Выбор шрифта для выводимого текста (параметр - название шрифта).

  Функция getFontWidth(); // Получение ширины символов выбранного шрифта.
  Функция getFontHeight(); // Получение высоты символов выбранного шрифта.
  Функция setCoding( [КОДИРОВКА]); // Указание кодировки текста в скетче.
  Функция setCursor( X , Y ); // Установка курсора в указанную позицию на экране.
  Функция setCursorShift( X , Y ); // Сдвиг курсора на указанное количество пикселей.
  Функция print( ТЕКСТ[ , X ] [ , Y ] ); // Вывод текста в указанную позицию на экране.
  Переменная numX// Принимает и возвращает текущую позицию курсора по оси X.

  Переменная numY// Принимает и возвращает текущую позицию курсора по оси Y.

*/
