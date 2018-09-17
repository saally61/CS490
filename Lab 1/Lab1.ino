#include <LiquidCrystal_PCF8574.h>
#include <SoftwareSerial.h>
SoftwareSerial esp8266(9,10); 
#include <LiquidCrystal.h>
#include <stdlib.h>

LiquidCrystal_PCF8574 lcd(0x27);

#define SSID "test"     // "SSID-WiFiname" 
#define PASS "password"       // "password"
#define IP "184.106.153.149"// thingspeak.com ip
String msg = "GET /update?key=UCXC5E63ISD1L6JJ"; //change it with your api key like "GET /update?key=Your Api Key"


void setup() {
  lcd.setBacklight(255);
  lcd.begin(16, 2);
  lcd.print("Welcome!");
  delay(100);
  //lcd.setCursor(0,1);
  Serial.begin(115200); //or use default 115200.
  esp8266.begin(115200);
  lcd.clear();
  esp8266.println("AT");
  delay(5000);
  lcd.print("Setting up...");

  
  if(esp8266.find("OK")){
    Serial.print("Connecting wifi");
    connectWiFi();
  }

  //interruptSetup();
   
}

void loop() {
    String cmd = "AT+CIPSTART=\"TCP\",\"";
    cmd += IP;
    cmd += "\",80";
    Serial.println(cmd);
    esp8266.println(cmd);
    delay(1000);
    if(esp8266.find("Error")){
      return;
    }
    cmd = msg ;
    double x = random(1,1000);
    lcd.clear();
    lcd.print(x);
    cmd += "&field2=";
    cmd += x;
    delay(1000);
  
  cmd += "\r\n";
  Serial.print("AT+CIPSEND=");
  esp8266.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  esp8266.println(cmd.length());

  if(esp8266.find(">")){
    Serial.print(cmd);
    esp8266.print(cmd);
  }
  else{
   Serial.println("AT+CIPCLOSE");
   esp8266.println("AT+CIPCLOSE");
    //Resend...
   
  }
 delay(1000);
}

void interruptSetup(){     
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED      
} 

boolean connectWiFi(){
  Serial.println("AT+CWMODE=1");
  esp8266.println("AT+CWMODE=1");
  delay(2000);
  String cmd="AT+CWJAP=\"";
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  Serial.println(cmd);
  esp8266.println(cmd);
  delay(5000);
  if(esp8266.find("OK")){
    Serial.println("OK");
    return true;    
  }else{
    return false;
  }
}
