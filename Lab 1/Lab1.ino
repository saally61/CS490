#include <LiquidCrystal_PCF8574.h>
#include <SoftwareSerial.h>
SoftwareSerial esp8266(9,10); 
#include <LiquidCrystal.h>
#include <stdlib.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <DHT.h>

//definitions
#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define SSID "test"     // "SSID-WiFiname" 
#define PASS "password"       // "password"
#define IP "184.106.153.149"// thingspeak.com ip


//sensors and devices
DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino
LiquidCrystal_PCF8574 lcd(0x27);
Adafruit_BMP280 bmp; // I2C

//variables
String msg = "GET /update?key=UCXC5E63ISD1L6JJ"; //change it with your api key like "GET /update?key=Your Api Key"
int switchState = 0;
int light; 
int Light_Pin = A1;
int UV_Pin = A0;
int Dust_Pin = 6;
int chk;
int wait_time = 1000;
int pulsePin = A2;
int blinkPin = 13;
int buttonPin = 4;

int LEDRed = 2;
int LEDGreen = 3; 
boolean connection = false;


unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 2000; 
unsigned long lowpulseoccupancy = 0;

float ratio = 0;
float concentration = 0;
float hum;  //Stores humidity value
float temp; //Stores temperature value
float Vsig;

String b_temp;
String b_pres;
String b_alt;
String L_data;
String dht_hum;
String dht_temp;
String uv_data;
String dust_data;

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded! 
volatile boolean Pulse = false;     // "True" when heartbeat is detected. "False" when not a "live beat". 
volatile boolean QS = false;        // becomes true when Arduino finds a beat.

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = true;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse 
volatile int rate[10];                    // array to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find IBI
volatile int P =512;                      // used to find peak in pulse wave, seeded
volatile int T = 512;                     // used to find trough in pulse wave, seeded
volatile int thresh = 525;                // used to find instant moment of heart beat, seeded
volatile int amp = 100;                   // used to hold amplitude of pulse waveform, seeded
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = false;      // used to seed rate array so we startup with reasonable BPM


 
void setup() {
  //Analog Pins
  pinMode(Light_Pin,INPUT);
  //digital pins
  pinMode(Dust_Pin,INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(blinkPin, OUTPUT);
  pinMode(LEDRed, OUTPUT);
  pinMode(LEDGreen, OUTPUT);
  
  lcd.setBacklight(255); //clear lcd
  lcd.begin(16, 2);
  lcd.print("Welcome!");
  delay(1000);
   // start serials
  Serial.begin(115200); 
  esp8266.begin(115200);
  lcd.clear();
  esp8266.println("AT");

  delay(5000);
  lcd.print("Setting up...");
  if(esp8266.find("OK")){
    Serial.print("Connecting wifi");
    connectWiFi();
  }


  dht.begin(); 
  if (!bmp.begin()) {  
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }

  
  //set up wifi
if(esp8266.find("OK")){
    Serial.print("Connecting wifi");
    connectWiFi();
  }
  delay(1000);
  interruptSetup();
}

void loop() {
  switchState = digitalRead(buttonPin);
  if(switchState == LOW){
      //heart beat 
     start:  
    
    b_temp = Get_Baro(0);
    print_Data("Barometer Temp.:",b_temp + (char)223+"C");
    
    b_pres = Get_Baro(1); 
    print_Data("Barometer Pres.:", b_pres + " KPa"); 
    
    b_alt = Get_Baro(2);
    print_Data("Barometer alt.:", b_alt + " m");

    
    //display light sensor
    L_data = Get_Light();
    print_Data("Light:", L_data);
  
    
    //display DHT sensor
    dht_hum = Get_Hum(0); 
    print_Data("Humitidy:" , dht_hum);

    dht_temp = Get_Hum(1);
    print_Data("Temperature:" , dht_temp  +(char)223+ "C");
  
    //Display UV sensor
    uv_data = Get_UV();
    print_Data("UV Index:" , uv_data);
  
    //dust sensor 
    dust_data = Get_Dust();
    print_Data("Dust:", dust_data+" pcs/cf");
  
    //heartbeat sensor 
    if (BPM > 100){
      digitalWrite(LEDRed, HIGH);
      digitalWrite(LEDGreen, LOW);
    }
    else {
      digitalWrite(LEDGreen, HIGH);
      digitalWrite(LEDRed, LOW);
      
      }
    
    print_Data("BPM:", String(BPM));
    lcd.clear();
    lcd.print("Setting up data...");
    //delay(5000);
    String sending_data = "&field1=" + b_temp
    + "&field2="+ b_pres
    + "&field3=" + b_alt 
    + "&field4=" + L_data 
    + "&field5=" + dht_hum 
    + "&field6=" + dht_temp
    + "&field7=" + uv_data
    + "&field8=" + dust_data
    + "&field9=" + BPM
;
    delay(2000);
    lcd.clear();
    lcd.print("Sending data");
    send_data(sending_data);
    
  }
  else {
    lcd.clear();
    lcd.print("Program on hold");
    delay(1000);
    
  }
}

void send_data(String data_to_send){
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += IP;
  cmd += "\",80";
  Serial.println(cmd);
  esp8266.println(cmd);
  lcd.print(".");
  delay(1000);
  lcd.print(".");
  if(esp8266.find("Error")){
    lcd.print("E");
    return;
  }
  cmd = msg ;
  cmd += data_to_send;

  //  send
  cmd += "\r\n";
  Serial.print("AT+CIPSEND=");
  esp8266.print("AT+CIPSEND=");
  Serial.println(cmd.length());
  esp8266.println(cmd.length());
  
  lcd.print(".");
  if(esp8266.find(">")){
    Serial.print(cmd);
    esp8266.print(cmd);
    lcd.print("G");
  }
  else{
   Serial.println("AT+CIPCLOSE");
   esp8266.println("AT+CIPCLOSE");
    //Resend...
    lcd.print("C");
  }
  delay(3000);
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
void print_Data(String line1, String line2){
  lcd.clear();
  if(line1.length() > 16){
    lcd.print("string too long");
    delay(wait_time);
    return;
  }
  lcd.print(line1);
  lcd.setCursor(0,1);
  lcd.print(line2);
  delay(wait_time);
}

String Get_Baro(int sensor_mode) {
  if(sensor_mode == 0){
    int temp1 = round(bmp.readTemperature());
    if(temp1 >= 32){
      digitalWrite(LEDRed, HIGH);
      digitalWrite(LEDGreen, LOW);
    }
    else{
      digitalWrite(LEDRed, LOW);
      digitalWrite(LEDGreen, HIGH);
    }
    return(String(temp1));
  }
  else if(sensor_mode == 1){
    digitalWrite(LEDRed, HIGH);
    digitalWrite(LEDGreen, LOW);
    return(String(0.001*round(bmp.readPressure()))); 
  }
  else if(sensor_mode == 2){
    digitalWrite(LEDRed, HIGH);
    digitalWrite(LEDGreen, LOW);
    return(String(round(bmp.readAltitude(1013.25))));
  }
  else{
    return("Null");
  }
}

String Get_Light() {
  light = analogRead(Light_Pin);
  if (light > 10){
    digitalWrite(LEDRed, LOW);
    digitalWrite(LEDGreen, HIGH);
    }
  else{
    digitalWrite(LEDGreen, HIGH);
    digitalWrite(LEDRed, LOW);
    }    
  return(String(light));
}

String Get_Hum(int mode) {
  if(mode == 0){
    hum = dht.readHumidity();
    if (hum > 60){
        digitalWrite(LEDRed, HIGH);
        digitalWrite(LEDGreen, LOW);
        //return("DANGER Hum.: " + String(hum));
    }
    else {
        digitalWrite(LEDGreen, HIGH);       
        digitalWrite(LEDRed, LOW);
        //return("Hum.: " + String(hum));
    }
    return(String(hum));
  }
  else if(mode == 1){
    temp= dht.readTemperature();
    if(temp >= 32){
      digitalWrite(LEDRed, HIGH);
      digitalWrite(LEDGreen, LOW);
    }
    else{
      digitalWrite(LEDRed, LOW);
      digitalWrite(LEDGreen, HIGH);
    }
    return(String(temp));
  }
  else{return ("Null");}  
}

String Get_UV()
{
  int sensorValue;
  long sum = 0;
  for(int i=0;i<1024;i++)
   {  
    sensorValue=analogRead(UV_Pin);
    sum=sensorValue+sum;
    delay(1);
   }   
   sum = sum >> 10;
   Vsig = sum*4980.0/1023.0; // Vsig is the value of voltage measured from the SIG pin of the Grove interface 
   String data = "";
   if (Vsig < 50) {
    data = "0"; 
    }
    if (Vsig > 50 && Vsig < 227) {
      data= "1"; 
    }
    else if (Vsig > 227 && Vsig < 318) {
      data= "2"; 
    }
    else if (Vsig > 318 && Vsig < 408) {
      data= "3";
    }
    else if (Vsig > 408 && Vsig < 503) {
      data= "4"; 
    }
    else if (Vsig > 503 && Vsig < 606) {
      data="5"; 
    }
    else if (Vsig > 606 && Vsig < 696) {
      data="6"; 
    }
    else if (Vsig > 696 && Vsig < 795) {
      data="7";
    }
    else if (Vsig > 795 && Vsig < 881) {
      data="8"; 
    }
    else if (Vsig > 881 && Vsig < 976) {
      data="9"; 
    }
    else if (Vsig > 976 && Vsig < 1079) {
      data="10 "; 
    }
    else if (Vsig > 1079 && Vsig < 1170) {
      data="11"; 
    }
    else if (Vsig > 1170) {
      data="11+";
    }
    if (Vsig > 1170){
      digitalWrite(LEDRed, HIGH);
      digitalWrite(LEDGreen, LOW);}
    else{
      digitalWrite(LEDGreen, HIGH);
      digitalWrite(LEDRed, LOW);}
    return(data);
}
String Get_Dust(){
  duration = pulseIn(Dust_Pin, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;
  if ((millis()-starttime) >= sampletime_ms) //if the sampel time = = 30s
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; 
    lowpulseoccupancy = 0;
    starttime = millis();
    if (concentration > 10){
        digitalWrite(LEDRed, HIGH);
        digitalWrite(LEDGreen, LOW);
      }
    else {
        digitalWrite(LEDGreen, HIGH);
        digitalWrite(LEDRed, LOW);
      }    
    return(String(concentration/0.01));
  }
}


void interruptSetup(){     
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED      
} 

ISR(TIMER2_COMPA_vect){                       // triggered when Timer2 counts to 124
  cli();                                      // disable interrupts while we do this
  Signal = analogRead(pulsePin);              // read the Pulse Sensor 
  sampleCounter += 2;                         // keep track of the time in mS
  int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

    //  find the peak and trough of the pulse wave
  if(Signal < thresh && N > (IBI/5)*3){      // avoid dichrotic noise by waiting 3/5 of last IBI
    if (Signal < T){                         // T is the trough
      T = Signal;                            // keep track of lowest point in pulse wave 
    }
  }

  if(Signal > thresh && Signal > P){        // thresh condition helps avoid noise
    P = Signal;                             // P is the peak
  }                                         // keep track of highest point in pulse wave

  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
  if (N > 250){                                   // avoid high frequency noise
    if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){        
      Pulse = true;                               // set the Pulse flag when there is a pulse
      digitalWrite(blinkPin,HIGH);                // turn on pin 13 LED
      IBI = sampleCounter - lastBeatTime;         // time between beats in mS
      lastBeatTime = sampleCounter;               // keep track of time for next pulse

      if(secondBeat){                        // if this is the second beat
        secondBeat = false;                  // clear secondBeat flag
        for(int i=0; i<=9; i++){             // seed the running total to get a realistic BPM at startup
          rate[i] = IBI;                      
        }
      }

      if(firstBeat){                         // if it's the first time beat is found
        firstBeat = false;                   // clear firstBeat flag
        secondBeat = true;                   // set the second beat flag
        sei();                               // enable interrupts again
        return;                              // IBI value is unreliable so discard it
      }   
      word runningTotal = 0;                  // clear the runningTotal variable    

      for(int i=0; i<=8; i++){                // shift data in the rate array
        rate[i] = rate[i+1];                  // and drop the oldest IBI value 
        runningTotal += rate[i];              // add up the 9 oldest IBI values
      }

      rate[9] = IBI;                          // add the latest IBI to the rate array
      runningTotal += rate[9];                // add the latest IBI to runningTotal
      runningTotal /= 10;                     // average the last 10 IBI values 
      BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
      QS = true;                              // set Quantified Self flag 
      // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
  }

  if (Signal < thresh && Pulse == true){   // when the values are going down, the beat is over
    //digitalWrite(blinkPin,LOW);            // turn off pin 13 LED
    Pulse = false;                         // reset the Pulse flag so we can do it again
    amp = P - T;                           // get amplitude of the pulse wave
    thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
    P = thresh;                            // reset these for next time
    T = thresh;
  }

  if (N > 2500){                           // if 2.5 seconds go by without a beat
    thresh = 512;                          // set thresh default
    P = 512;                               // set P default
    T = 512;                               // set T default
    lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
    firstBeat = true;                      // set these to avoid noise
    secondBeat = false;                    // when we get the heartbeat back
  }

  sei();     
  // enable interrupts when youre done!
}// end isr
