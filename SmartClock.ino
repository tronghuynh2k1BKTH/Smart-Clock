//#include <string>
//#include <stdio.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "Website.h"
//#include "WifiLoginCSS.h"

#include <SimpleTimer.h>

#include <PubSubClient.h> // client to Adafruit
#include "Adafruit_GFX.h" // LED RGB
#include "Max72xxPanel.h" // LED matrix

#include <WiFiUdp.h> 
#include <NTPClient.h> // Server NTP 
#include <DHT.h> // Temperature and humidity sensor

//#include <SPI.h>
#include <NTPtimeESP.h>
#include <Adafruit_NeoPixel.h>

#include <EEPROM.h>

#define PIN_LED 4 // D4
Adafruit_NeoPixel strip = Adafruit_NeoPixel(8, PIN_LED, NEO_GRB + NEO_KHZ800);

WiFiClient espClient;
PubSubClient client(espClient);

const char *ssid = "SmartClock";
const char *password = "SmartClock";
ESP8266WebServer server(80); // port 80

int pinCS = D4; // PIN control LED MATRIX
int numberOfHorizontalDisplays = 4; // number of matrix 8x8
int numberOfVerticalDisplays = 1; // number of row 
const byte buffer_size = 45; // data size 
char time_value[buffer_size];
char temp_value[buffer_size];
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
int wait = 70;
int spacer = 1;
int width = 4 + spacer;

WiFiUDP wUdp;
NTPClient NTPc(wUdp, "1.asia.pool.ntp.org", 7*3600);
NTPtime NTPch("1.asia.pool.ntp.org"); 
strDateTime Date;

String sTime, tempTime;
bool isOn = true; // Clock is on?
float fCelsius, fHumidity; // temperature and humidity
int iMode = 1; // 1. is On 2. is Sleep
uint8_t a = 0; // for LEDs RGB Effect

bool isAlarm = false; // Alarm is set
bool isAlarmToday = false;
String alarm; // temporary variable for set up Alarm
// temporay Hours for alarm
String aHours = ""; 
String tempHours, tempMinutes; 
String aMinutes = "";
byte aDay, aMonth, aDayofWeek;
int aYear = 0;
int AlarmMode /*1. On day 2. Time + tomorrow 3. Time + DD/MM/YY 4. Time + day of week*/, AlarmMode_m; // 1. HH:MM 2.HH

// Variables for control LEDs
int xRed = 0, yGreen = 0, zBlue = 0, LEDMode = 0; // temporary 
String numLED;
int tempLED;

// Color: White, Red, Orange, Yellow, Green, Blue, Indigo, Purple 
int  LEDcl[8][3] = {{255, 255, 255}, {255, 0, 0}, {255, 51, 0}, {255, 255, 0}, {0, 153, 0}, {0, 153, 255}, {51, 0, 153}, {102, 0, 153}};  

// Adafruit
#define IO_USERNAME  "minhtan"
#define IO_KEY       "aio_HkZX42t0mtX0GKTuP6RLg59t57QQ"

// #define DHT11
#define DHTPIN 5 // PIN to connect DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Called when user use GG Assistant 
void callback(char* topic, byte* payload, unsigned int length) {
  // Turn on 
  if ((char)payload[0] == 'o' && length == 1) {
    isOn = true;
    iMode = 1;
  }
  // Turn off
  else if ((char)payload[0] == 'f' && length == 1){
    isOn = false;
    iMode = 0;
    strip.setBrightness(0);
    strip.show();
    Clear();
  }
  // Show temperature and humidity
  else if ((char)payload[0] == 't' && length == 1)
  {
    LEDMode = 0;
    iMode = 3;
    Clear();
  }
  // Set alarm
  else if ((char)payload[0] == 'a' or (char)payload[0] == 'A')
  {
    byte tempDay = Date.day;
    byte tempMonth = Date.month;
    alarm = convert_(payload, length);
    alarm.trim();
    alarm.remove(0, 3);
    char tempAlarm[alarm.length()];
    String temp;

    strcpy(tempAlarm, alarm.c_str());
    strlwr(tempAlarm);
    Serial.println(tempAlarm);

    isAlarm = true;
    
    if (strstr(tempAlarm, ":") != NULL)
    {
      if (tempAlarm[2] == ':')
      {
        Serial.println("case1-1");
        aHours = alarm.substring(0, 1);
        aMinutes = alarm.substring(6, 2);
        aMinutes.remove(0, 2);
      }
      if (tempAlarm[3] == ':')
      {
        aHours = alarm.substring(0, 2);
        aMinutes = alarm.substring(7, 2);  
        aMinutes.remove(0, 3);  
      }
      AlarmMode_m = 1;
    }
    else
    {
      if (alarm.length() <= 5)
      {
        aHours = alarm;
      }
      else
      {
      if (tempAlarm[1] == ' ')
        aHours = alarm.substring(0, 1);
      else if (tempAlarm[2] == ' ')
        aHours = alarm.substring(0, 2);
      }      
      AlarmMode_m = 2;
    }

    if (alarm.length() <= 10)
    {
      AlarmMode = 1;
    }

    if (strstr(tempAlarm, "tomorrow") != NULL || strstr(tempAlarm, "tomorrow") != NULL || strstr(tempAlarm, "tommorow") != NULL)
    {
      if (tempMonth == 1 || tempMonth == 3 || tempMonth == 5 || tempMonth == 7 || tempMonth == 8 || tempMonth == 10 || tempMonth == 12)
      {
        if (tempDay == 31)
        {
          aDay = 1;
          aMonth = tempMonth + 1;
        }
        else
        {
          aDay = tempDay + 1;
          aMonth = tempMonth;
        }
      }
      else if (tempMonth == 4 || tempMonth == 6 || tempMonth == 9 || tempMonth == 11)
      {
        if (tempDay == 30)
        {
          aDay = 1;
          aMonth = tempMonth + 1;
        }
        else
        {
          aDay = tempDay + 1;    
          aMonth = tempMonth;
        }   
      }
      else if (tempMonth == 2)
      {
        if (tempDay == 28)
        {
          aDay = 1;
          aMonth = tempMonth + 1;
        }
        else
        {
          aDay = tempDay + 1;
          aMonth = tempMonth;
        }
      }

      if (tempDay == 31 && tempMonth == 12)
        aYear = Date.year + 1;
      else
        aYear = Date.year;

      AlarmMode = 2;
    }
    else if (strstr(tempAlarm, "/") != NULL)
    {
       temp = alarm;
       temp.remove(0, 8);
       aDay = byte(atoi(temp.substring(0, 2).c_str()));
       temp.remove(0, 5); 
       Serial.println(aDay);
       aMonth = byte(atoi(temp.substring(0, 2).c_str()));
       temp.remove(0, 5);
       aYear = atoi(temp.c_str());

       AlarmMode = 3;
    }
    else if (strstr(tempAlarm, "monday") != NULL)
    {
       aDayofWeek = 2;
       AlarmMode = 4;      
    }
    else if (strstr(tempAlarm, "tuesday") != NULL)
    {
       aDayofWeek = 3;
       AlarmMode = 4;
    }
    else if (strstr(tempAlarm, "wednesday") != NULL)
    {
       aDayofWeek = 4;
       AlarmMode = 4;
    }
    else if (strstr(tempAlarm, "thursday") != NULL)
    {
       aDayofWeek = 5; 
       AlarmMode = 4;
    }
    else if (strstr(tempAlarm, "friday") != NULL)
    {
       aDayofWeek = 6;
       AlarmMode = 4;
    }
    if (strstr(tempAlarm, "sartuday") != NULL)
    {
       aDayofWeek = 7; 
       AlarmMode = 4;
    }
    if (strstr(tempAlarm, "sunday") != NULL)
    {
       aDayofWeek = 1; 
       AlarmMode = 4;
    }
  }
  else
  {
    numLED = convert_(payload, length);
    tempLED = atoi(numLED.c_str());

    if (tempLED >= 0 && tempLED <= 255)
    {
       xRed = 255 - tempLED;
       yGreen = tempLED; 
       zBlue = 0;  
    }
    else if (tempLED > 255  && tempLED <= 510)
    {
      xRed = 0;
      yGreen = 510 - tempLED;
      zBlue = tempLED - 255;
    }
    else if (tempLED > 510 && tempLED <= 765)
    {
      xRed = tempLED - 510;
      yGreen = 0;
      zBlue = 765 - tempLED;
    }
    
    iMode = 1;
    LEDMode = 1;
  }
}

// Convert byte to String
String convert_(byte* in, unsigned int length)
{
  String str = "";
  
  for(int i = 0; i < length; i++)
    str += (char)in[i];

  return str;
}

// Send to web to enter 
void ketnoi(){
  String s = MAIN_page;
  server.send(200,"text/html",s); // Send html
}

void ketnoi1(){
  server.send(200,"text/plain","Hello World"); // Send html
}

// get WiFi ID and Pasword from web
void cai_dat(){
  String tenwf = server.arg("tenwifi");
  String mk = server.arg("matkhau");
  
  if (tenwf.length() > 0 && mk.length() > 0) {
    Serial.println("dọn eeprom");
    for (int i = 0; i < 96; ++i) {
      EEPROM.write(i, 0);
   }
    
    Serial.println("viết dữ liệu tên wifi vào eeprom:");
    for (int i = 0; i < tenwf.length(); ++i){
      EEPROM.write(i, tenwf[i]);
    }
    Serial.println("viết dữ liệu mật khẩu vào eeprom");
    for (int i = 0; i < mk.length(); ++i){
      EEPROM.write(32 + i, mk[i]);
    }
  EEPROM.commit();
  }
  ESP.reset();

  // Send to web when connected
  server.send(200,"text/plain","đã kết nối thành công"); // gửi dưới dạng html 
}

// Connected to WiFi
bool testWifi(void)
{
  int c = 0;
  Serial.println("Chờ kết nối");
  while ( c < 30 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    Serial.print("*");
    delay(1000);
    c++;
  }
  Serial.println("");
  Serial.println("không thể kết nối vì quá thời gian");
  return false;
}

void setup() {
    // Set up LED Matrix
  matrix.setIntensity(2); // Use a value between 0 and 15 for brightness
  matrix.setRotation(0, 1);    // The first display is position upside down
  matrix.setRotation(1, 1);    // The first display is position upside down
  matrix.setRotation(2, 1);    // The first display is position upside down
  matrix.setRotation(3, 1);    // The first display is position upside down

// Set up pin D0 for Buzzer 
  pinMode(D0, OUTPUT);
  digitalWrite(D0, LOW);

 // Set up LED RGB
  strip.begin(); 
  strip.setBrightness(40);
  strip.show();
  for (int i = 0; i < 8; i++)
  {
    strip.setPixelColor(i, LEDcl[i][0], LEDcl[i][1], LEDcl[i][2]);
    strip.show();
  }

  EEPROM.begin(512); //Initialasing EEPROM  
  Serial.begin(115200);
  
  WiFi.disconnect();
  // WiFi Station Access
  WiFi.mode(WIFI_STA);

  String tenwifie = "";
  String mke = "";
  Serial.println();
  Serial.println("Đọc tên wifi trên eeprom");
  for (int i = 0; i < 32; ++i)
  {
    tenwifie += char(EEPROM.read(i));
  }
  Serial.print("tên wifi: ");
  Serial.println(tenwifie);
  Serial.println();
  Serial.println("Đọc mật khẩu trên eeprom");
  for (int i = 32; i < 96; ++i)
  {
    mke += char(EEPROM.read(i));
  }
  Serial.print("mật khẩu: ");
  Serial.println(mke);

  WiFi.begin(tenwifie, mke);
  if (testWifi())
  {
    Serial.println("kết nối thành công");
    Serial.print("Địa chỉ IP:");
    Serial.println(WiFi.localIP());
    server.on("/",ketnoi1);// khi bạn 192.168.x.x thì nó sẽ thực hiện hàm ketnoi đầu tiên
    server.begin(); // bắt đầu khởi động sever
    Serial.println(tenwifie);
    Serial.println(mke);

  // Set up for connect to server Adafruit
    client.setServer("io.adafruit.com", 1883);
    client.setCallback(callback);
    client.connect("SmartClock", "minhtan", IO_KEY);
    client.subscribe("minhtan/feeds/+");  
    Serial.println(client.connected());
  
  // Starting NTPc client for get time from NTP server
    NTPc.begin();
  
  // Starting temperature and humidity sensor
    dht.begin();
  }
  else {  
      Serial.println("Cấu hình điểm kết nối");
      WiFi.softAP(ssid, password);
      Serial.print("Địa chỉ Ip của ESP:");
      Serial.println(WiFi.softAPIP());
      server.on("/",ketnoi);// start ketnoi function
      server.on("/caidat",cai_dat);
      server.begin();
  }
}

void loop() {
  server.handleClient();
    
// Loop client Adafruit
  client.loop();
  
// Upadate time from server NTP
  NTPc.update(); 

// Cases
  if (isOn)
  {  
    sTime = NTPc.getFormattedTime();
    NTPc.update();
    Date = NTPch.getNTPtime(7.0, 0);

    strip.setBrightness(40);
    strip.show();

    if (Date.valid)
      if (isAlarm)
      {
        tempTime = sTime;
        tempHours = tempTime.substring(0, 2);
        tempTime.remove(0, 3);
        tempMinutes = tempTime.substring(0, 2);
        
        if (AlarmMode == 1)
        {
          if (AlarmMode_m == 1)
          {
            if (aHours == tempHours && aMinutes == tempMinutes)
            {
              digitalWrite(D0, HIGH);
              delay(15000);
              aMinutes = "";
            }
            else
              digitalWrite(D0, LOW);
          }
          else if (AlarmMode_m == 2)
          {
            if (aHours == tempHours && tempMinutes == "00")
            {
              digitalWrite(D0, HIGH);
              delay(15000);
              tempMinutes = "";
            } 
            else
              digitalWrite(D0, LOW);        
          }
        }
        else if (AlarmMode == 2 || AlarmMode == 3)
        {
          if (AlarmMode_m == 1)
          {
            if (aHours == tempHours && aMinutes == tempMinutes && aDay == Date.day && aMonth == Date.month && aYear == Date.year)
            {
              digitalWrite(D0, HIGH);
              delay(15000);   
              aMinutes = "";        
            }
            else
              digitalWrite(D0, LOW);
          }
          else if (AlarmMode_m == 2)
          {
            if (aHours == tempHours && aMinutes == "00" && aDay == Date.day && aMonth == Date.month && aYear == Date.year)
            {
              digitalWrite(D0, HIGH);
              delay(15000);  
              tempMinutes = "";         
            }   
            else
              digitalWrite(D0, LOW);       
          }
        }
        else if (AlarmMode == 4)
        {
          if (AlarmMode_m == 1)
          {
            if (aHours == tempHours && aMinutes == tempMinutes && aDayofWeek == Date.dayofWeek)
            {
              digitalWrite(D0, HIGH);
              delay(15000);
              tempMinutes = "";
            }
            else
              digitalWrite(D0, LOW);
          }
          else if (AlarmMode_m == 2)
          {
             if (aHours == tempHours && tempMinutes == "00" && aDayofWeek == Date.dayofWeek)
            {
              digitalWrite(D0, HIGH);
              delay(15000);
              tempMinutes = "";
            }
            else
              digitalWrite(D0, LOW);         
          }       
        }
      }
    
    if (iMode == 0)
    {
//      strip.setBrightness(0);
//      Clear();
    }
    if (iMode == 1)
    { 
      if (LEDMode == 0)
      {    
        strip.setBrightness(40);
        strip.show();         
        if (a < 8)
        { 
          if (a+7 > 7)
          {
            strip.setPixelColor(7, LEDcl[a-1][0], LEDcl[a-1][1], LEDcl[a-1][2]);
            strip.show();
          }
          else
          {
            strip.setPixelColor(7, LEDcl[a+7][0], LEDcl[a+7][1], LEDcl[a+7][2]);
            strip.show();           
          }
          
          if (a+6 > 7)
          {
            strip.setPixelColor(6, LEDcl[a-2][0], LEDcl[a-2][1], LEDcl[a-2][2]);
            strip.show();
          }
          else
          {
            strip.setPixelColor(6, LEDcl[a+6][0], LEDcl[a+6][1], LEDcl[a+6][2]);
            strip.show();            
          }
          
          if (a+5 >7)
          {
            strip.setPixelColor(5, LEDcl[a-3][0], LEDcl[a-3][1], LEDcl[a-3][2]);
            strip.show();
          }
          else
          {
            strip.setPixelColor(5, LEDcl[a+5][0], LEDcl[a+5][1], LEDcl[a+5][2]);
            strip.show();
          }
          
          if (a+4 > 7)
          {
            strip.setPixelColor(4, LEDcl[a-4][0], LEDcl[a-4][1], LEDcl[a-4][2]);
            strip.show();
          }
          else
          {
            strip.setPixelColor(4, LEDcl[a+4][0], LEDcl[a+4][1], LEDcl[a+4][2]);
            strip.show();           
          }
          
          if (a+3 > 7)
          {
            strip.setPixelColor(3, LEDcl[a-5][0], LEDcl[a-5][1], LEDcl[a-5][2]);
            strip.show();
          }
          else
          {
            strip.setPixelColor(3, LEDcl[a+3][0], LEDcl[a+3][1], LEDcl[a+3][2]);
            strip.show();            
          }
          
          if (a+2 > 7)
          {
            strip.setPixelColor(2, LEDcl[a-6][0], LEDcl[a-6][1], LEDcl[a-6][2]);
            strip.show();
          }
          else
          {
            strip.setPixelColor(2, LEDcl[a+2][0], LEDcl[a+2][1], LEDcl[a+2][2]);
            strip.show();            
          }
          
          if (a+1 > 7)
          {
            strip.setPixelColor(1, LEDcl[a-7][0], LEDcl[a-7][1], LEDcl[a-7][2]);
            strip.show(); 
          }
          else
          {
            strip.setPixelColor(1, LEDcl[a+1][0], LEDcl[a+1][1], LEDcl[a+1][2]);
            strip.show();             
          }
          
          strip.setPixelColor(0, LEDcl[a][0], LEDcl[a][1], LEDcl[a][2]);
          strip.show();         
          delay(250);
          a++;
        }
        else
        {
          a = 0; 
        }
      }
      else
      {
        strip.setPixelColor(0, xRed, yGreen, zBlue);
        strip.show();
        strip.setPixelColor(1, xRed, yGreen, zBlue);
        strip.show();
        strip.setPixelColor(2, xRed, yGreen, zBlue);
        strip.show();
        strip.setPixelColor(3, xRed, yGreen, zBlue);
        strip.show();
        strip.setPixelColor(4, xRed, yGreen, zBlue);
        strip.show();
        strip.setPixelColor(5, xRed, yGreen, zBlue);
        strip.show();
        strip.setPixelColor(6, xRed, yGreen, zBlue);
        strip.show();
        strip.setPixelColor(7, xRed, yGreen, zBlue);
        strip.show();
      }
      PrintTimes();
    }
    if (iMode == 3)
    {
      client.loop();
      fCelsius = dht.readTemperature();
      fHumidity = dht.readHumidity();
      Serial.println(iMode);
      if(!isnan(fCelsius) && !isnan(fHumidity))
      {
        PrintTemperatureAndHumidity();
        Clear();
      }
      PrintTimes();
      if(fCelsius >= 20.00 && fCelsius <= 22.00)
      {
        strip.setPixelColor(0, 0, 0, 255);
        strip.setPixelColor(1, 0, 0, 235);
        strip.setPixelColor(2, 0, 0, 215);
        strip.setPixelColor(3, 10, 0, 190);
        strip.setPixelColor(4, 0, 0, 0);
        strip.setPixelColor(5, 0, 0, 0);
        strip.setPixelColor(6, 0, 0, 0);
        strip.setPixelColor(7, 0, 0, 0);
      }
      else if (fCelsius > 23.00 && fCelsius <= 25.00)
      {
        strip.setPixelColor(0, 0, 0, 255);
        strip.setPixelColor(1, 0, 0, 235);
        strip.setPixelColor(2, 0, 0, 215);
        strip.setPixelColor(3, 10, 0, 190);
        strip.setPixelColor(4, 100, 0, 130);
        strip.setPixelColor(5, 0, 0, 0);
        strip.setPixelColor(6, 0, 0, 0);
        strip.setPixelColor(7, 0, 0, 0);       
      }
      else if (fCelsius > 25.00 && fCelsius <= 29.00)
      {
        Serial.println(fCelsius);
        strip.setPixelColor(0, 0, 0, 255);
        strip.setPixelColor(1, 0, 0, 235);
        strip.setPixelColor(2, 0, 0, 215);
        strip.setPixelColor(3, 50, 0, 190);
        strip.setPixelColor(4, 100, 0, 130);
        strip.setPixelColor(5, 175, 0, 50);
        strip.setPixelColor(6, 0, 0, 0);
        strip.setPixelColor(7, 0, 0, 0);        
      }
      else if (fCelsius > 29.00 && fCelsius <= 32.00)
      {
        strip.setPixelColor(0, 0, 0, 255);
        strip.setPixelColor(1, 0, 0, 235);
        strip.setPixelColor(2, 0, 0, 215);
        strip.setPixelColor(3, 50, 0, 190);
        strip.setPixelColor(4, 100, 0, 130);
        strip.setPixelColor(5, 125, 0, 50);
        strip.setPixelColor(6, 200, 0, 10);
        strip.setPixelColor(7, 0, 0, 0);         
      }
      else if (fCelsius > 32)
      {
        strip.setPixelColor(0, 0, 0, 255);
        strip.setPixelColor(1, 0, 0, 235);
        strip.setPixelColor(2, 0, 0, 215);
        strip.setPixelColor(3, 50, 0, 190);
        strip.setPixelColor(4, 100, 0, 130);
        strip.setPixelColor(5, 125, 0, 50);
        strip.setPixelColor(6, 200, 0, 10);
        strip.setPixelColor(7, 255, 0, 0);        
      }
      strip.show();
      delay(2000);
      Clear();
      delay(500);     
    }
  }
  else
  {
    Clear();
    strip.setBrightness(0); 
    strip.show();
  }
}

void PrintTimes()
{
  sTime.trim(); //Delete all space in string sTime;
  
  sTime.toCharArray(time_value, 10);
  matrix.drawChar(2, 0, time_value[0], HIGH, LOW, 1); // H
  matrix.drawChar(8, 0, time_value[1], HIGH, LOW, 1); // HH
  matrix.drawChar(13, 0, time_value[2], HIGH, LOW, 1); // HH:
  matrix.drawChar(18, 0, time_value[3], HIGH, LOW, 1); // HH:M
  matrix.drawChar(24, 0, time_value[4], HIGH, LOW, 1); // HH:MM
  matrix.write();
}

void PrintTemperatureAndHumidity()
{
  String sTemp = String(fCelsius, 0);
  String sHum = String(fHumidity, 0);

  sTemp = sTemp + "'C";
  sHum = sHum + "%";

  Serial.println(sTemp);
  Serial.println(sHum);
  
  sHum.toCharArray(temp_value, 5);
  matrix.drawChar(3, 0, temp_value[0], HIGH, LOW, 1); // H
  matrix.drawChar(9, 0, temp_value[1], HIGH, LOW, 1); // HH
  matrix.drawChar(16, 0, temp_value[2], HIGH, LOW, 1); // HH:
  matrix.write();
  delay(2000);
  Clear();
  sTemp.toCharArray(temp_value, 5);
  matrix.drawChar(3, 0, temp_value[0], HIGH, LOW, 1); // H
  matrix.drawChar(9, 0, temp_value[1], HIGH, LOW, 1); // HH
  matrix.drawChar(16, 0, temp_value[2], HIGH, LOW, 1); // HH:
  matrix.drawChar(22, 0, temp_value[3], HIGH, LOW, 1);
  matrix.write();
  delay(2000); 
}

void Clear()
{
  String tempClear = "         ";
  char tClear[10];

  matrix.drawChar(0, 0, tClear[0], HIGH, LOW, 1);
  matrix.drawChar(5, 0, tClear[0], HIGH, LOW, 1);
  matrix.drawChar(10, 0, tClear[0], HIGH, LOW, 1);
  matrix.drawChar(15, 0, tClear[0], HIGH, LOW, 1);
  matrix.drawChar(20, 0, tClear[0], HIGH, LOW, 1);
  matrix.drawChar(25, 0, tClear[0], HIGH, LOW, 1);
  matrix.write();
}
