#include <Wire.h> 
#include <LiquidCrystal_I2C.h>  
#include <SoftwareSerial.h>
SoftwareSerial SIM800(8, 9);   			  //установка sim800l на порт 8 9                 
LiquidCrystal_I2C lcd(0x27,16,2); 
#define PERIOD 60000.0
unsigned long dispPeriod; 				  //переменная для обноваления показаний дисплея
volatile unsigned long CNT;
unsigned long CPM;
int K=0;
int L=0;
String response = "";                          // Переменная для хранения ответа модуля

void setup()
{
  Serial.begin(9600);                           //Для дебагинга SIM800l
  SIM800.begin(9600);                           // Скорость обмена данными с модемом
  Serial.println("Start");
  lcd.init();                                   // Включение дисплея 
  lcd.backlight();                              // Включаем подсветку дисплея
  sendATCommand("AT", true);                    // Функция, которая отвечает за правильную обработку AT команд в SIM800L
  CPM = 0;
  dispPeriod = 0;
  response = sendATCommand("AT+CMGF=1;&W", true); // Включаем текстовый режима SMS (Text mode) и сразу сохраняем значение (AT&W)!
  attachInterrupt(0,GetEvent,FALLING);          // Event on pin 2, импульсы счётчика Гейгера
  
}

String sendATCommand(String cmd, bool waiting) {
  String _resp = "";                            // Переменная для хранения результата
  Serial.println(cmd);                          // Дублируем команду в монитор порта
  SIM800.println(cmd);                          // Отправляем команду модулю
  if (waiting) {                                // Если необходимо дождаться ответа...
    _resp = waitResponse();                     // ... ждем, когда будет передан ответ
    // Если Echo Mode выключен (ATE0), то эти 3 строки можно закомментировать
    if (_resp.startsWith(cmd)) {                // Убираем из ответа дублирующуюся команду
      _resp = _resp.substring(_resp.indexOf("\r", cmd.length()) + 2);
    }
    Serial.println(_resp);                      // Дублируем ответ в монитор порта
  }
  return _resp;                                 // Возвращаем результат. Пусто, если проблема
}

String waitResponse() {                         // Функция ожидания ответа и возврата полученного результата
  String _resp = "";                            // Переменная для хранения результата
  long _timeout = millis() + 10000;             // Переменная для отслеживания таймаута (10 секунд)
  while (!SIM800.available() && millis() < _timeout)  {}; // Ждем ответа 10 секунд, если пришел ответ или наступил таймаут, то...
  if (SIM800.available()) {                     // Если есть, что считывать...
    _resp = SIM800.readString();                // ... считываем и запоминаем
  }
  else {                                        // Если пришел таймаут, то...
    Serial.println("Timeout...");               // ... оповещаем об этом и...
  }
  return _resp;                                 // ... возвращаем результат. Пусто, если проблема
}

void loop()
{
  if (millis() >= dispPeriod + PERIOD) {        //Расчёт периода срабатывания счётчика Гейгера в минута                         
    dispPeriod = millis();
    K= dispPeriod/PERIOD;
    L=L+CNT;
    CPM = (L/K)*0.87;
    SIM800.print("AT+CMGF=1\r");               //Выбор СМС сообщений
    delay(100);
    SIM800.print("AT+CMGS=\"+79********\"\r"); //Отправка на указый номер
    delay(100);
    SIM800.print(String(CPM) +" mkR/h");  	  //Сообщение которое отправится 
    delay(100);
    SIM800.print((char)26);				  // кодировка сообщения (требуется в соответствии с таблицей данных)
    SIM800.println();
    lcd.print(String(CPM) +" mkR/h"); 		  //Вывод на экран сообщения которое отправилось
    delay(100);
    CNT = 0;
 }

 if (SIM800.available())   {                   // Если модем, что-то отправил...
    response = waitResponse();                 // Получаем ответ от модема для анализа
    response.trim();                           // Убираем лишние пробелы в начале и конце
    Serial.println(response);                  // Если нужно выводим в монитор порта
    //....
    if (response.startsWith("+CMGS:")) {       // Пришло сообщение об отправке SMS
      int index = response.lastIndexOf("\r\n");// Находим последний перенос строки, перед статусом
      String result = response.substring(index + 2, response.length()); // Получаем статус
      result.trim();                            // Убираем пробельные символы в начале/конце

      if (result == "OK") {                     // Если результат ОК - все нормально
        Serial.println ("Message was sent. OK");
      }
      else {                                    // Если нет, нужно повторить отправку
        Serial.println ("Message was not sent. Error");
      }
    }
  }
  if (Serial.available())  {                    // Ожидаем команды по Serial...
    SIM800.write(Serial.read());                // ...и отправляем полученную команду модему
  };
 
}

void GetEvent()
{                     					   // Получение счётчика тактов срабатывания счётчика Гейгера
 CNT++;
}
