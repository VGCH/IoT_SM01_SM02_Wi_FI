String powerS(){
  String response_message ="";
  if(load_on){
      digitalWrite(on_off_pin, LOW);
      digitalWrite(on_off_ind, HIGH);
      response_message += "<center><a href=\"#\" class=\"btn_off\" onclick=\"on_off()\">Включить</a></center>";
      load_on = false;
   }else{
      digitalWrite(on_off_pin, HIGH);
      digitalWrite(on_off_ind, LOW);
      response_message  += "<center><a href=\"#\" class=\"btn_on\" onclick=\"on_off()\">Выключить</a></center>";
      load_on = true;
  }
      
     server.send(200, "text/html", response_message + "0 ytn" );
     if(settings.mqtt_en){ MQTT_send_data("jsondata", JSON_DATA());}
     return response_message + "0 ytn";
}

             

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (int i = 0; i < length; i++) {
        message = message + (char)payload[i];
    }
    
    // Пробуем распарсить как JSON
    if (message.startsWith("{")) {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, message);
        
        if (!error) {


        }
    }
    
    // Обработка команд 0, 1, 2
    if(message != "On" && message != "Off"){  
        int data = message.toInt();
        if(data == 0){
            powerS();
        }
        if(data == 1){
            settings.auto_en = true;
            save_config();
            if(settings.mqtt_en){ 
                MQTT_send_data("jsondata", JSON_DATA());
            }
        }
        if(data == 2){
            settings.auto_en = false;
            save_config();
            if(settings.mqtt_en){ 
                MQTT_send_data("jsondata", JSON_DATA());
            }
        }
    }
}

void buttom_process() {
//lastButton, lastSwitch
  if(digitalRead(button_contr) == LOW && !buttonState) {
        buttonState = true;
        lastButton = millis();
        powerS();
  }

    if(digitalRead(button_contr) == HIGH && buttonState && millis() - lastButton > 500 ) {
        buttonState = false;
        lastButton = millis();
     }
  
}

void external_sw_pr(){

  if (digitalRead(external_switch) == LOW && !State_exs) {
        State_exs = true;
        lastSwitch = millis();
        if(settings.auto_en){
            powerS();
        }else if(!load_on){ 
            powerS(); 
        }       
  }

  if (digitalRead(external_switch) == HIGH && State_exs && millis() - lastSwitch > 500) {
        State_exs = false;
        lastSwitch = millis();
        if(!settings.auto_en && load_on){
             powerS(); 
        }
  }
  
}