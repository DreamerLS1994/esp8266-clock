#include <U8g2lib.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include "DHTesp.h"
#include <ESP8266HTTPClient.h>

#include <ArduinoJson.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

 
#define TRIGGER 5   // NodeMCU Pin D1 > TRIGGER 
#define ECHO    4   //Pin D2 > ECHO

U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);   // All Boards without Reset of the Display

char ssid[] = "xxx";
char pswd[] = "xxx";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com", 60 * 60 * 8, 30 * 60 * 1000);

String nowTime = "";
String nowDate = "";
char* nowDayStr = "";
char* nowT = "";
uint16_t iconStr;

struct WeatherInfo
{
  String date;
  uint16_t iconStrDay;
  String day;
  uint16_t iconStrNight;
  String night;
  int low;
  int high;
}winfo[2];


Ticker requestT;
int requestInt = 298;
Ticker updateT;
int updateInt = 0;
Ticker updateTH;
int updateTHInt = 0;

Ticker changeShow;
int changeShowInt = 0;

DHTesp dht;

void tickerRequestInt()
{
    requestInt++;
}

void tickerUpdateInt()
{
    updateInt++;
}

void tickerUpdateTHInt()
{
    updateTHInt++;
}

void tickerchangeShowInt()
{
    changeShowInt++;
}

void connectWiFi()
{
    u8g2.setFont(u8g2_font_crox2hb_tr);
    u8g2.clearBuffer();
    u8g2.drawStr(0,30,"Connecting ..");
    u8g2.sendBuffer();
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pswd);

    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }

    u8g2.clearBuffer();
    u8g2.drawStr(0,20,"Success");
    u8g2.drawStr(0,40,WiFi.localIP().toString().c_str());
    u8g2.sendBuffer();
}

void getWeather()
{
    WiFiClient tcpClient;
    HTTPClient httpClient;
    
    httpClient.begin(tcpClient, "http://api.seniverse.com/v3/weather/now.json?key=S_P3KbX9gOevEc6GR&location=suzhou&language=zh-Hans"); 
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) 
    {
      String Payload = httpClient.getString();      // 使用getString函数获取服务器响应体内容
     
      // Allocate the JSON document
      // Use https://arduinojson.org/v6/assistant to compute the capacity.
      StaticJsonDocument<512> doc;

      DeserializationError error = deserializeJson(doc, Payload);

      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
      }
      
      JsonObject results_0 = doc["results"][0];
      JsonObject results_0_now = results_0["now"];
      const char* nowWeatherCode1 = results_0_now["code"]; // 
      const char* results_0_last_update = results_0["last_update"];
      
      int WeatherCode = atoi(nowWeatherCode1);
      iconStr = getWeatherIcon(WeatherCode);
      //Serial.println(iconStr);
            
    } 
    else 
    {
      Serial.print("\r\nServer Respose Code: ");
      Serial.println(httpCode);
      String Payload = httpClient.getString();
      Serial.println(Payload);
    }
    
    /* 5. 关闭ESP8266与服务器的连接 */
    httpClient.end();

    int nowDay = timeClient.getDay();
    nowDayStr = getDayStr(nowDay);
    Serial.println(nowDay);
}

void get2Weather()
{
    WiFiClient tcpClient;
    HTTPClient httpClient;
    
    httpClient.begin(tcpClient, "http://api.seniverse.com/v3/weather/daily.json?key=S_P3KbX9gOevEc6GR&location=suzhou&language=zh-Hans&unit=c&start=1&days=3"); 
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) 
    {
        String Payload = httpClient.getString();      // 使用getString函数获取服务器响应体内容
        // String input;

        DynamicJsonDocument doc(1536);
        
        DeserializationError error = deserializeJson(doc, Payload);
        
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
          return;
        }
        
        JsonObject results_0 = doc["results"][0];
        int count = 0;
        for (JsonObject results_0_daily_item : results_0["daily"].as<JsonArray>()) 
        {
          const char* results_0_daily_item_date = results_0_daily_item["date"]; // "2022-01-12", "2022-01-13"
          const char* results_0_daily_item_text_day = results_0_daily_item["text_day"]; // "多云", "多云"
          const char* results_0_daily_item_code_day = results_0_daily_item["code_day"]; // "4", "4"
          const char* results_0_daily_item_text_night = results_0_daily_item["text_night"]; // "多云", "多云"
          const char* results_0_daily_item_code_night = results_0_daily_item["code_night"]; // "4", "4"
          const char* results_0_daily_item_high = results_0_daily_item["high"]; // "8", "6"
          const char* results_0_daily_item_low = results_0_daily_item["low"]; // "-1", "-2"
          if(count == 0)
          {
            winfo[0].date = (char*)results_0_daily_item_date;
            winfo[0].day = (char*)results_0_daily_item_text_day;
            winfo[0].night = (char*)results_0_daily_item_text_night;
            //strcpy(winfo[0].date, results_0_daily_item["date"]);
            //strcpy(winfo[0].day, results_0_daily_item["text_day"]);
            //strcpy(winfo[0].night, results_0_daily_item["text_night"]);
            
            int codeDay = atoi(results_0_daily_item["code_day"]);
            winfo[0].iconStrDay = getWeatherIcon(codeDay);
            
            int codeNight = atoi(results_0_daily_item["code_night"]);
            winfo[0].iconStrNight = getWeatherIcon(codeNight);
            
            winfo[0].low = atoi(results_0_daily_item["low"]);
            winfo[0].high = atoi(results_0_daily_item["high"]);
            Serial.println(winfo[0].date);
          }
          if(count == 1)
          {
            winfo[1].date = (char*)results_0_daily_item_date;
            winfo[1].day = (char*)results_0_daily_item_text_day;
            winfo[1].night = (char*)results_0_daily_item_text_night;
            //strcpy(winfo[1].date, results_0_daily_item["date"]);
            //strcpy(winfo[1].day, results_0_daily_item["text_day"]);
            //strcpy(winfo[1].night, results_0_daily_item["text_night"]);
            int codeDay = atoi(results_0_daily_item["code_day"]);
            winfo[1].iconStrDay = getWeatherIcon(codeDay);
            
            int codeNight = atoi(results_0_daily_item["code_night"]);
            winfo[1].iconStrNight = getWeatherIcon(codeNight);
            
            winfo[1].low = atoi(results_0_daily_item["low"]);
            winfo[1].high = atoi(results_0_daily_item["high"]);
            Serial.println(winfo[1].date);
          }
          
          count++;
        }
    }
    else 
    {
      Serial.print("\r\nServer Respose Code: ");
      Serial.println(httpCode);
      String Payload = httpClient.getString();
      Serial.println(Payload);
    }
}


void getDate()
{
    WiFiClient tcpClient;
    HTTPClient httpClient;
    
    httpClient.begin(tcpClient, "http://apps.game.qq.com/CommArticle/app/reg/gdate.php"); 
    int httpCode = httpClient.GET();
    if (httpCode == HTTP_CODE_OK) 
    {
      String Payload = httpClient.getString();      // 使用getString函数获取服务器响应体内容
      //Serial.println(Payload);
      nowDate = Payload.substring(25,30);
      //Serial.println(nowDate);
    }
    else 
    {
      Serial.print("\r\nServer Respose Code: ");
      Serial.println(httpCode);
      String Payload = httpClient.getString();
      Serial.println(Payload);
    }
    
    /* 5. 关闭ESP8266与服务器的连接 */
    httpClient.end();
}

void getTH()
{
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();

    //Serial.println(humidity);
    //Serial.println(temperature);
    sprintf(nowT, "T:%.1fc  H:%.f%%", temperature, humidity);
    Serial.println(nowT);
}

uint16_t getWeatherIcon( int code)
{
    //https://seniverse.yuque.com/books/share/e52aa43f-8fe9-4ffa-860d-96c0f3cf1c49/yev2c3
    switch(code)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return 0x0045;
        case 9:
            return 0x0040;
        case 4:
            return 0x0041;
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
            return 0x0043;
        default:
            return 0x0040;
    }
}

char* getDayStr(int day)
{
    switch(day)
    {
        case 0:
          return "Sun";
          break;
        case 1:
          return "Mon";
          break;
        case 2:
          return "Tue";
          break;
        case 3:
          return "Wed";
          break;
        case 4:
          return "Thur";
          break;
        case 5:
          return "Fri";
          break;
        case 6:
          return "Sat";
          break;
        
    }
}

void showTime()
{
    u8g2.setFont(u8g2_font_crox2hb_tf);
    u8g2.clearBuffer();
    u8g2.setCursor(0,12);
    u8g2.print(nowDate);
    
    u8g2.setCursor(45,12);
    u8g2.print(nowDayStr);
    u8g2.drawHLine(0,15,75);
    u8g2.setCursor(0,45);
    u8g2.setFont(u8g2_font_osb21_tf);
    u8g2.print(nowTime.substring(0,5));
    u8g2.setFont(u8g2_font_crox2hb_tf);
    u8g2.setCursor(72,45);
    u8g2.print(nowTime.substring(6,8));
    u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);

    u8g2.drawGlyph(93,40,iconStr);

    u8g2.drawHLine(0,50,128);
    u8g2.setFont(u8g2_font_crox2hb_tf);
    u8g2.setCursor(0,63);
    u8g2.print(nowT);
    u8g2.sendBuffer();
}

void showWeather(WeatherInfo w)
{
    u8g2.clearBuffer();

    u8g2.setCursor(0,12);
    u8g2.print(w.date.substring(5,10));
    u8g2.setCursor(80,12);
    u8g2.print("suzhou");
    u8g2.drawHLine(0,15,128);
    
    u8g2.setFont(u8g2_font_open_iconic_weather_4x_t);
    u8g2.drawGlyph(16,55,w.iconStrDay);
    u8g2.drawGlyph(80,55,w.iconStrNight);
    u8g2.setFont(u8g2_font_crox2hb_tf);
    u8g2.setCursor(46,64);
    u8g2.print(w.low);
    u8g2.drawStr(60,64,"-");
    u8g2.setCursor(68,64);
    u8g2.print(w.high);
    u8g2.sendBuffer();
}

void setup() {
    // put your setup code here, to run once:
    // 初始化串口
    Serial.begin(9600);

    //dht
    dht.setup(D0, DHTesp::DHT11); // Connect DHT sensor to GPIO 17
  
    // 初始化有LED的IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    u8g2.begin();

    connectWiFi();
    
    timeClient.begin();
    
    requestT.attach(1, tickerRequestInt);
    updateT.attach(1, tickerUpdateInt);
    updateTH.attach(1, tickerUpdateTHInt);
    changeShow.attach(1, tickerchangeShowInt);
}


void loop() {
    // put your main code here, to run repeatedly:
    if(updateInt>=1)
    {
      timeClient.update();
      nowTime = timeClient.getFormattedTime();
      updateInt = 0;
    }
    if(requestInt>=300)
    {
      getWeather();
      getDate();
      get2Weather();
      requestInt = 0;
    }
    if (updateTHInt>=5)
    {
      getTH();
      updateTHInt = 0;
    }
    if(changeShowInt<=8)
    {
      showTime();
    }
    else if(changeShowInt<=12)
    {
      showWeather(winfo[0]);
    }
    else if(changeShowInt<=16)
    {
      showWeather(winfo[1]);
    }
    else
    {
      changeShowInt=0;
    }
}
