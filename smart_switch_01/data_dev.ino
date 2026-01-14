String millis2time() {
    unsigned long totalSeconds = millis() / 1000;
    
    int days = totalSeconds / 86400;           // 86400 = 24 * 3600
    int hours = (totalSeconds % 86400) / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    String timeStr = String(days) + ":" + 
                     twoDigits(hours) + ":" + 
                     twoDigits(minutes) + ":" + 
                     twoDigits(seconds);
    return timeStr;
}

 String twoDigits(int digits){
        if(digits < 10) {
          String i = '0'+String(digits);
          return i;
         }else{
          return String(digits);
         }
      }

 // Фильтрация 
float filter_v(float raw) {
    Pc = P + Q;
    G = Pc/(Pc + R);
    P = (1-G)*Pc;
    Xp = Xe;
    Zp = Xp;
    Xe = G*(raw-Zp)+Xp; // фильтрованное значение
    return(Xe);
} 
    
float get_voltage(){
  return filter_v(analogRead(0) * 0.6963);    // При делителе напряжения 100к/10к
} 

void time_work(){
   if (captivePortal()) {  
    return;
  }

  if (validateToken()){
    StaticJsonDocument<200> doc;

     doc["time"]      = millis2time();
     doc["chanel1"]   = load_on ? "On" : "Off";
     doc["voltage"]   = String(get_voltage());
     doc["MQTT"]      = MQTT_status();
     doc["auto_st"]   = settings.auto_en ? "checked" : "" ;
     String outjson;
     serializeJson(doc, outjson);
     server.send(200, "text", outjson);    
  }   
}


String JSON_DATA() {
    StaticJsonDocument<256> doc;  
    
    doc["c"]        = load_on ? "On" : "Off";
    doc["voltage"]  = String(get_voltage());  // 1 знак после запятой вместо 2
    doc["swmode"]   = String(settings.auto_en);
    String outjson;
    serializeJson(doc, outjson);
    return outjson;
}
