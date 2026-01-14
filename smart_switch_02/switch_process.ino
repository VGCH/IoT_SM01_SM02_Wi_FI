String powerS(int chanell){
     String response_message ="";
  if(chanell==1){
    if(load_on_1){
      digitalWrite(on_off_pin_1, LOW);
      digitalWrite(on_off_ind, HIGH);
      response_message += "<center><a href=\"#\" class=\"btn_off\" onclick=\"on_off()\">Включить канал 1</a></center>";
      load_on_1 = false;
    }else{
      digitalWrite(on_off_pin_1, HIGH);
      digitalWrite(on_off_ind, LOW);
      response_message  += "<center><a href=\"#\" class=\"btn_on\" onclick=\"on_off()\">Выключить канал 1</a></center>";
      load_on_1 = true;
    }    
  }

  if(chanell==2){
    if(load_on_2){
      digitalWrite(on_off_pin_2, LOW);
      digitalWrite(on_off_ind, HIGH);
      response_message += "<center><a href=\"#\" class=\"btn_off\" onclick=\"on_off2()\">Включить канал 2</a></center>";
      load_on_2 = false;
    }else{
      digitalWrite(on_off_pin_2, HIGH);
      digitalWrite(on_off_ind, LOW);
      response_message  += "<center><a href=\"#\" class=\"btn_on\" onclick=\"on_off2()\">Выключить канал 2</a></center>";
      load_on_2 = true;
    }
  }
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
         // Здесь будут JSON данные 

        }
    }
    
    // Обработка команд 0, 1, 2
    if(message != "On" && message != "Off"){  
        int data = message.toInt();
        if(data == 0){ powerS(1); }
        if(data == 4){ powerS(2); }
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
  if(digitalRead(button_contr) == LOW && !buttonState) {
        buttonState = true;
        lastButton = millis();
        powerS(1);
  }

    if(digitalRead(button_contr) == HIGH && buttonState && millis() - lastButton > 500 ) {
        buttonState = false;
        lastButton = millis();
     }
  
}

void external_sw_pr(){

  if (digitalRead(external_switch_1) == LOW && !State_exs) {
        State_exs = true;
        lastSwitch = millis();
        if(settings.auto_en){
            powerS(1);
        }else if(!load_on_1){
             powerS(1);
               }
        
  }

  if (digitalRead(external_switch_1) == HIGH && State_exs && millis() - lastSwitch > 500) {
        State_exs = false;
        lastSwitch = millis();
        if(!settings.auto_en && load_on_1){
             powerS(1); 
        }
  }

  if (digitalRead(external_switch_2) == LOW && !State_exs2) {
        State_exs2 = true;
        lastSwitch_2 = millis();
        if(settings.auto_en){
            powerS(2);
        }else if(!load_on_2){
            powerS(2); 
        }
        
  }

  if (digitalRead(external_switch_2) == HIGH && State_exs2 && millis() - lastSwitch_2 > 500 && millis()) {
        State_exs2 = false;
        lastSwitch_2 = millis();
        if(!settings.auto_en && load_on_2){
             powerS(2); 
        }
  }
  
}