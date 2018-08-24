int switchState = 0;
int Light_Time = 200;

int stat = 0;
void setup() {
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);
  pinMode(2, INPUT);
}

void loop(){
  switchState = digitalRead(2);
  if(switchState == LOW){
  // the button is not pressed

  digitalWrite(3, HIGH); //green LED
  digitalWrite(4, LOW); //Red LED
  digitalWrite(5, LOW); //red LED

  stat = 1;
  }

  else{
    for( int i= 0; i < 4; i++){
        digitalWrite(3, LOW);
        digitalWrite(4, LOW);
        digitalWrite(5, HIGH);
        delay(Light_Time); // wait for a quarter seconds
        
        digitalWrite(4, HIGH);
        digitalWrite(5, LOW);
        delay(Light_Time);
      
        digitalWrite(3, HIGH); //
        digitalWrite(4, LOW);//
        digitalWrite(5, LOW);//
        delay(Light_Time);// 
    }
  }

}
