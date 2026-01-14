// Функция отправки данных по протоколу MQTT
void MQTT_send(){
   client.loop();
   uint32_t nows = millis();
  if (nows - lastMsg > 10000 && settings.mqtt_en) {
    if(WiFi.status() == WL_CONNECTED) {
        MQTT_send_data("jsondata", JSON_DATA());
     }
   lastMsg = nows; 
  }
 }
void MQTT_send_data(String topic, String data){
         String root = settings.mqtt_topic;
         String top  = root +"/"+ topic;
         String subscr = root +"/control";
         String input = settings.mqtt_serv;
         int colonIndex = input.indexOf(':');
         String ipAddress;
         String port;
         IPAddress Remote_MQTT_IP;

        if (colonIndex != -1) {
             ipAddress = input.substring(0, colonIndex);
             port = input.substring(colonIndex + 1);
             WiFi.hostByName(ipAddress.c_str(), Remote_MQTT_IP);
          }
      
      client.setServer(Remote_MQTT_IP, port.toInt());
      client.setCallback(callback);
      client.setBufferSize(1024); 
           if(client.connected()){                                                                                    // Вторичная отправка данных при подключенном брокере 
          count_rf = 0;
          send_mqtt(top, data, subscr);
           }else{
              count_rf++;
              if (client.connect(ch_id.c_str(), settings.mqtt_user, settings.mqtt_passw)){                            // Первичная отправка данных, выполняестя попытка подключения к брокеру 
                    send_mqtt(top, data, subscr);          
                }else{
                  if(count_rf > 2){                                                                                    // Если были неудачные попытки подключения, то переподключаем Wi-fi
                     WiFi.disconnect();
                     WiFi.begin(settings.mySSID, settings.myPW);
                     count_rf = 0;
                }
            }
        }
     
}


void send_mqtt(String tops, String data, String subscr) {
    if(!annonce_mqtt_discovery) {
        String device_id = "radar_light_switch_" + ch_id;
        String configuration_url = "http://" + WiFi.localIP().toString();
        String top = String(settings.mqtt_topic) + "/jsondata";
        String control = String(settings.mqtt_topic) + "/control";

        // Создаем конфигурацию устройства
        DynamicJsonDocument deviceDoc(512);
        deviceDoc["ids"][0] = device_id;
        deviceDoc["name"] = "Одноканальное реле";
        deviceDoc["mdl"] = version_code;
        deviceDoc["sw"] = "1.0.0";
        deviceDoc["mf"] = "CYBEREX TECH";
        deviceDoc["configuration_url"] = configuration_url;

        // Вспомогательная функция для публикации конфигурации
        auto publishConfig = [&](const String& type, const String& entity_id, JsonDocument& doc) {
            doc["device"] = deviceDoc.as<JsonObject>();
            String payload;
            serializeJson(doc, payload);
            client.publish(("homeassistant/" + type + "/" + entity_id + "/config").c_str(), 
                          payload.c_str(), true);
        };

        //  Датчик расстояния
        {
            DynamicJsonDocument doc(384);
            doc["device_class"] = "voltage";
            doc["name"] = "Напряжение сети";
            doc["state_topic"] = top;
            doc["unit_of_measurement"] = "V";
            doc["value_template"] = "{{ value_json.voltage }}";
            doc["unique_id"] = ch_id + "_d";
            publishConfig("sensor", ch_id + "_d", doc);
        }

        //  Переключатель 
        {
            DynamicJsonDocument doc(384);
            doc["name"] = "Управление";
            doc["command_topic"] = control;
            doc["state_topic"] = top;
            doc["payload_on"] = "0";
            doc["payload_off"] = "0";
            doc["state_on"] = "On";
            doc["state_off"] = "Off";
            doc["value_template"] = "{{ value_json.c }}";
            doc["unique_id"] = ch_id + "_s_off";
            publishConfig("switch", ch_id + "_s_off", doc);
        }

                //  Переключатель режима внешнего выключателя
        {
            DynamicJsonDocument doc(384);
            doc["name"] = "Режим выкл (кнопка)";
            doc["command_topic"] = control;
            doc["state_topic"] = top;
            doc["payload_on"] = "1";
            doc["payload_off"] = "2";
            doc["state_on"] = "1";
            doc["state_off"] = "0";
            doc["value_template"] = "{{ value_json.swmode }}";
            doc["unique_id"] = ch_id + "swmode";
            publishConfig("switch", ch_id + "swmode", doc);
        }

        annonce_mqtt_discovery = true;
    }
    
    // Отправляем данные 
    client.publish(tops.c_str(), data.c_str());
    client.subscribe(subscr.c_str());
}

String MQTT_status(){
    return client.connected() ? "подключен" : "отключен";
} 
