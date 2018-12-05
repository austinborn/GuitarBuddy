#define CLK 35
#define SDI 23
#define SDO 21
#define LE 2

void cc(){
  digitalWrite(CLK, HIGH);
  delay(20);
  digitalWrite(CLK, LOW);
}

void ced1(){
  digitalWrite(LE, HIGH);
  delay(20);
  digitalWrite(LE, LOW);
}

void sh(){
  digitalWrite(SDI, HIGH);
  cc();
  ced1();
}

void sl(){
  digitalWrite(SDI, LOW);
  cc();
  ced1();
}

//shift num bits for the input (starting with least significant bit)
void sendByte(byte input, int num){
  while(num > 0){
    byte mask = 0x01;
    if(input & mask){
      digitalWrite(SDI, HIGH);
    }
    else{
      digitalWrite(SDI, LOW);
    }
    cc();
    num--;
    input = input << 1;
  }
  ced1();
}


void setup() {
  // put your setup code here, to run once:
  pinMode(CLK, OUTPUT);
  pinMode(SDI, OUTPUT);
  pinMode(SDO, INPUT);
  pinMode(LE, OUTPUT);
}

void loop() {
//  sendByte(0xFF, 8);
//  delay(1000);
//  sendByte(0x00, 8);
//  delay(1000);
    sh();
    delay(500);
    sl();
    delay(500);
}
