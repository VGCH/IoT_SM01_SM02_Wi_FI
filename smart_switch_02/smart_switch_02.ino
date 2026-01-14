/* Модуль правления освещением SM-02
 *  © CYBEREX Tech, 2026
 * 
 */
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266SSDP.h>
#include <PubSubClient.h>
#include "const_js.h"
#include <ArduinoJson.h>

#define on_off_pin_1    14                    // Пин управления реле R1
#define on_off_pin_2    12                    // Пин управления реле R1
#define on_off_ind      3                     // Индикатор включения 
#define button_contr    1                     // Кнопка управления встроенная в модуль
#define external_switch_1  13                 // Пин для подключения внешнего выключателя S1
#define external_switch_2  4                  // Пин для подключения внешнего выключателя S2


// WEBs
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient client(espClient);
IPAddress apIP(10, 10, 20, 1);
IPAddress netMsk(255, 255, 255, 0);
ESP8266HTTPUpdateServer httpUpdater;

// DNS server
const byte             DNS_PORT = 53;
DNSServer              dnsServer;

// Текущий статус WLAN
unsigned int status = WL_IDLE_STATUS;

// Переменные для хранения статуса сетевого подключения
bool connect;
bool wi_fi_st;

bool stat_reboot, led_stat, stat_wifi, load_on_1, load_on_2, stat, annonce_mqtt_discovery; 

// Переменные используемые для CaptivePortal
const char *myHostname  = "Cyberex_SmartSw02";            // Имя создаваемого хоста Cyberex_SmartSw02.local 
const char *softAP_ssid = "CYBEREX-SmartSw02";            // Имя создаваемой точки доступа Wi-Fi

String version_code = "RSM-02-1.0.01.26";

float auto_on_dist;
//timers
uint32_t low_t, med_t, prevMills, lastMsg, impOnDelay, reboot_t, lastConnectTry, lastButton, lastSwitch, lastSwitch_2;

String inputString = "";
bool off_status; 

int count_rf = 0;
int level_couts = 0;

// Глобальные переменные для фильтра Калмана
float R = 4.85;      // среднее отклонение (сила сглаживания)
float Q = 0.015;     // скорость реакции на изменение (подбирается вручную)
float Pc = 0.0;
float G = 0.0;
float P = 1.0;
float Xp = 0.0;
float Zp = 0.0;
float Xe = 230.0;

// кнопочка
bool buttonState;
bool State_exs;
bool State_exs2;

 // Структура для хранения токенов сессий 
struct Token {
    String value;
    long created;
    long lifetime;
};

Token   tokens[20];                    // Длина массива структур хранения токенов 

#define TOKEN_LIFETIME 6000000         // время жизни токена в секундах

#define MAX_STRING_LENGTH 30           // Максимальное количество символов для хранения в конфигурации


// Структура для хранения конфигурации устройства
struct {
     bool    mqtt_en;
     int     statteeprom; 
     int     counter_coeff;
     char    mySSID[MAX_STRING_LENGTH];
     char    myPW[MAX_STRING_LENGTH];
     char    mqtt_serv[MAX_STRING_LENGTH];
     char    mqtt_user[MAX_STRING_LENGTH];
     char    mqtt_passw[MAX_STRING_LENGTH];
     char    mqtt_topic[MAX_STRING_LENGTH];
     char    passwd[MAX_STRING_LENGTH]; 
     bool    auto_en;
  } settings;

//
String ch_id = String(ESP.getChipId());

void setup() {
    EEPROM.begin(sizeof(settings));                                 // Инициализация EEPROM в соответствии с размером структуры конфигурации      
    pinMode(on_off_pin_1, OUTPUT);
    pinMode(on_off_pin_2, OUTPUT);
    pinMode(on_off_ind, OUTPUT);
    pinMode(button_contr, INPUT);                                   // Режим пина кнопки
    pinMode(external_switch_1, INPUT);                              // Режим пина внешнего выключателя S1
    pinMode(external_switch_2, INPUT);                              // Режим пина внешнего выключателя S2
    digitalWrite(on_off_ind, HIGH);
    read_config();                                                  // Чтение конфигурации из EEPROM
    check_clean();                                                  // Провека на запуск устройства после первичной прошивки
   if(String(settings.passwd) == ""){   
                                                                    // Если  переменная settings.passwd пуста, то назначаем пароль по умолчанию
         strncpy(settings.passwd, "admin", MAX_STRING_LENGTH);
     }                              
     
     WiFi.mode(WIFI_STA);                                           // Выбираем режим клиента для сетевого подключения по Wi-Fi                
     WiFi.hostname("Smart Switch 02 [CYBEREX TECH]");               // Указываеем имя хоста, который будет отображаться в Wi-Fi роутере, при подключении устройства
     if(WiFi.status() != WL_CONNECTED) {                            // Инициализируем подключение, если устройство ещё не подключено к сети
           WiFi.begin(settings.mySSID, settings.myPW);
      }

     for(int x = 0; x < 160; x ++){                                  // Цикл ожидания подключения к сети (80 сек)
          if (WiFi.status() == WL_CONNECTED){ 
                stat_wifi = true;
                break;
           }
               delay(100); 
      
          }

     if(WiFi.status() != WL_CONNECTED) {                             // Если подключение не удалось, то запускаем режим точки доступа Wi-Fi для конфигурации устройства
            WiFi.mode(WIFI_AP_STA);
            WiFi.softAPConfig(apIP, apIP, netMsk);
            WiFi.softAP(softAP_ssid);
            stat_wifi = false;
        }
        
        delay(2000);
        // Запускаем DNS сервер
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", apIP);
        // WEB страницы
        server.on("/", page_process);
        server.on("/login", handleLogin);
        server.on("/script.js", reboot_devsend);
        server.on("/style.css", css);
        server.on("/index.html", HTTP_GET, [](){
        server.send(200, "text/plain", "Smart Switch"); });
        server.on("/description.xml", HTTP_GET, [](){SSDP.schema(server.client());});
        server.on("/inline", []() {
        server.send(200, "text/plain", "this works without need of authentification");
        });
        server.onNotFound(handleNotFound);
        // Здесь список заголовков для записи
        const char * headerkeys[] = {"User-Agent", "Cookie"} ;
        size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
        // Запускаем на сервере отслеживание заголовков 
        server.collectHeaders(headerkeys, headerkeyssize);
        server.begin();
        connect = strlen(settings.mySSID) > 0;                                            // Статус подключения к Wi-Fi сети, если есть имя сети
        SSDP_init();                  
}

void loop() {
        portals();
        dnsServer.processNextRequest();
        server.handleClient();  
        reboot_dev_delay();
        MQTT_send(); 
        buttom_process();
        external_sw_pr();
}

void reboot_devsend(){
     String input = server.arg("script");         // Js скрипт процессинг
       if (input == "reb_d"){
               server.send(200, "text", FPSTR(reb_d));
         }else if(input == "config_js"){
               server.send(200, "text", FPSTR(config_js));
         }else if(input == "update_js"){
               server.send(200, "text", FPSTR(update_js));
         }else if(input == "pass_js"){
               server.send(200, "text", FPSTR(pass_js));
         }else if(input == "js_wifi"){
               server.send(200, "text", FPSTR(js_wifi));
         }
}
