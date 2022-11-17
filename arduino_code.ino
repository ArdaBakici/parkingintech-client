// Kutüphaneliri Ekliyoruz
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

// Kolaylık sağlaması açısından array uzunluğunu bulan bir macro atıyoruz
#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))

// Servo motorları tanımlıyoruz
Servo enterservo, exitservo;


// LED'imizin hangi pinlere bağlı olduğunu tanımlıyoruz
const int R = 2;
const int G = 6;
const int B = 7;


// RFID okuyucularımızın istenildiği yere takılan pinlerini tanımlıyoruz
#define RST_PIN         5
#define SS_1_PIN        3
#define SS_2_PIN        4


//LCD Ekranları Tanımlıyoruz
LiquidCrystal enter_lcd(23, 25, 27, 29, 31, 33);
LiquidCrystal exit_lcd(22, 24, 26, 28, 30, 32);


// Araçların giriş zamanını tutacağımız değişkenleri tanımlıyoruz
unsigned long enterTime[6] = {0,0,0,0,0,0};  


// Ultrasonik sensörle ilgili değişkenleri açıyor ve pinleri tanımlıyoruz
const int trigger_pin1 = 36;
const int echo_pin1 = 37;

const int trigger_pin2 = 38;
const int echo_pin2 = 39;

const int trigger_pin3 = 40;
const int echo_pin3 = 41;

const int trigger_pin4 = 42;
const int echo_pin4 = 43;

const int trigger_pin5 = 10;
const int echo_pin5 = 11;

const int trigger_pin6 = 12;
const int echo_pin6 = 13;

int ultasonictime1, ultasonictime2, ultasonictime3, ultasonictime4, ultasonictime5, ultasonictime6;
int distancearray[6] = {0, 0, 0, 0, 0, 0};
int emptyarea = 0 ;
int emptyspace = 0 ;

// Sim modülümüzün pinlerini tanımlıyoruz
SoftwareSerial sim(44, 45);


// Sim modülümüz ile ilgili değişkenleri açıyoruz
int _timeout;
bool flag = true;
int i = 0;
String _buffer;
String lastnumber = "0";
String _readSerial1;
String _readSerial2;


// Araçlara tanımlı telefon numaralarını giriyoruz
String numberarray[6] ={
  "+905555555555", // 1 nolu araç için
  "+905555555555", // 2 nolu araç için
  "+905555555555", // 3 nolu araç için
  "+905555555555", // 4 nolu araç için
  "+905555555555", // 5 nolu araç için
  "+905555555555", // 6 nolu araç için
};


// Araçlarımızın RFID kimliklerini tanımlıyoruz
byte tagarray[6][4] = {
  {12, 141, 23, 15},   // 1 nolu araç için
  {137, 87, 140, 109}, // 2 nolu araç için
  {186, 138, 98, 103}, // 3 nolu araç için
  {9, 131, 134, 109},  // 4 nolu araç için
  {41, 86, 23, 229},   // 5 nolu araç için
  {9, 130, 182, 109}   // 6 nolu araç için  
};


// Kontrolü sağlayayabilmek için hangi aracın nereye gireceğini kaydedeceğimiz değişkeni açıyoruz
int controlarray[6] = {0, 0, 0, 0, 0, 0};


// Kaç adet rfid okuyucumuz olduğunu tanımlıyoruz
#define NR_OF_READERS   2
byte ssPins[] = {SS_1_PIN, SS_2_PIN};


// RFID Okuma ve Kontrolünü sağlayabilme ile ilgili değişkenleri açıyoruz
bool tagaccess[6] = {true, true, true, true, true, true};
int tagcount = 0;
bool access = false;
MFRC522 mfrc522[NR_OF_READERS];

// Bilgisayar ile iltişimimizi kontrol etmek için boolean değerleri
bool waiting_response;
bool com_w_server;


void setup() {

  // LED pinlerimizin işlevini tanımlıyoruz
  pinMode(R, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(B, OUTPUT);

  analogWrite(R, 255);
  analogWrite(G, 0);
  analogWrite(B, 255);
  
  // LCD Ekralarımızıı Başlatıyoruz
  enter_lcd.begin(16, 2);
  exit_lcd.begin(16, 2);

  // Servo motorlarımızı hangi pinlere bağladığımzı tanımlıyoruz
  enterservo.attach(8);
  exitservo.attach(9);

  // Servo motorlarımızı başlangıç konumuna alıyoruz
  enterservo.write(90);
  exitservo.write(90);

  // Seri haberleşmeyi başlatıyoruz
  Serial.begin(9600); 
  waiting_response = false;
  com_w_server = false;
  Serial.print('s');             
                
  // Sim modülünü başlatıyoruz
  _buffer.reserve(50);
  sim.begin(9600);
  delay(1000);

  // Ultrasonik sensör pinlerimizin işlevini tanımlıyoruz
  pinMode(trigger_pin1, OUTPUT);
  pinMode(echo_pin1, INPUT);

  pinMode(trigger_pin2, OUTPUT);
  pinMode(echo_pin2, INPUT);

  pinMode(trigger_pin3, OUTPUT);
  pinMode(echo_pin3, INPUT);

  pinMode(trigger_pin4, OUTPUT);
  pinMode(echo_pin4, INPUT);

  pinMode(trigger_pin5, OUTPUT);
  pinMode(echo_pin5, INPUT);

  pinMode(trigger_pin6, OUTPUT);
  pinMode(echo_pin6, INPUT);

  // SPI Haberleşmeyi başlatıyoruz
  SPI.begin();                  


  // RFID Okuyucularımızı arıyoruz
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
   /* Serial.print(F("Reader "));
    Serial.print(reader);
    Serial.print(F(": "));*/
    mfrc522[reader].PCD_DumpVersionToSerial();
  }
}



void loop() {
  communicateWithServer(controlarray, ARRAY_SIZE(controlarray), false);
  FullLed();
  
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {

    // RFID Kartlarını arıyoruz
    if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
     /* Serial.print(F("Reader "));
      Serial.print(reader);

      Serial.print(F(": Card UID:"));*/
      dump_byte_array(mfrc522[reader].uid.uidByte, mfrc522[reader].uid.size);
      //Serial.println();

      for (int x = 0; x < 6; x++)                  // X değişkeni RFID Kimliklerini tanımladığımız arraydaki sırayı gosteren değişken 
      {
        
        for (int i = 0; i < mfrc522[reader].uid.size; i++)    
        {
          if ( mfrc522[reader].uid.uidByte[i] != tagarray[x][i])  //RFID Kimliklerinin doğruluğunu teyir ediyoruz
          {
            DenyingTag();
            
            break;
          }
          else
          {
            
           if (i == mfrc522[reader].uid.size - 1)                // Bütün kimliği tarayıp taramadığımızı test ediyoruz
           {
            
              if(reader == 0 && tagaccess[x]){
                  EnterTag(x);
                  tagaccess[x] = false;
                }
              else if(reader == 1 && !tagaccess[x]){
                  ExitTag(x);
                  tagaccess[x] = true;
                }
            }
            else
            {
              
              continue;                                      
            }
          }
        }
        if (access){
          break;
        }
        else{
        
          UnknownTag();
        }
      }
      mfrc522[reader].PICC_HaltA();
      mfrc522[reader].PCD_StopCrypto1();
    }
  }
}

// RFID kimliklerini HEXADECIMAL(16'lı sayı sistemi) haline döndürür
void dump_byte_array(byte * buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
   /* Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);*/
  }
}


// Bu fonksiyon tanımsız RFID Kimliklerin girişine izin vermez
void DenyingTag()
{
  access = false;
}


// Bu fonksiyon giriş onaylanırsa yapılacakları yapar
void EnterTag(int x)
{
  FindArea();
 // Serial.println("Hosgeldiniz");
  EnterLcd();
  enterservo.write(0); // Girişteki bariyeri açıyor
  lastnumber = numberarray[x] ; // Giriş yapan sürücünün kim olduğuna göre telefon numarasını ayarlıyor
  enterTime[x]= millis(); // Giriş yapılan zamanı kaydediyor
  controlarray[x] = emptyarea; // Sonraki sürücülere aynı park yeri vermemek için sürücünün nereye park etmesi gerektiğini kaydediyor
  communicateWithServer(controlarray, ARRAY_SIZE(controlarray), true);
  SendEnterMessage(); 
  delay(3000);
  enter_lcd.clear(); // Girişteki ekranı temizliyor
  enterservo.write(90); // Girişteki bariyeri kapatıyor
  access = true;
  
}

// Bu fonksiyon çıkış onaylanırsa yapılacakları yapar
void ExitTag(int x)
{
 // Serial.println("Gule Gule");
  enterTime[x]= (millis()- enterTime[x]) / 60000 * 5 +5 ; // Ücreti Hesaplıyor
 // Serial.println(enterTime[x]);
  ExitLcd(x);
  exitservo.write(0); // Çıkştaki bariyeri açıyor
  lastnumber = numberarray[x] ;
  controlarray[x] = 0; // Sonraki sürücülere aynı park yeri verebilmek için sürücünün nereye park ettiğini sıfırlıyor 
  communicateWithServer(controlarray, ARRAY_SIZE(controlarray), true);
  SendExitMessage(x);
  delay(3000);
  exit_lcd.clear(); // Çıkıştaki ekranı temizliyor
  exitservo.write(90); // Çıkştaki bariyeri kapatıyor
  access = true;
  
}

void communicateWithServer(int data_array[], int array_size, bool change){
  while (Serial.available() > 0){
    char in_char;
    in_char = readSerial();

    if (in_char == 'o'){
      // send data
      sendDataToServer(data_array, array_size);
      // wait response
      com_w_server = false;
    }
    else if (in_char == 'r'){
      if (!com_w_server){
        // waiting for response and we got what we wanted
        com_w_server = true;
      }
    }
  }
  if (com_w_server){
    // if new car came in or left
    if (change){
      sendDataToServer(data_array, array_size);
      com_w_server = false;
    }
  }
}

char readSerial(){
  return (char) Serial.read();
}

void sendDataToServer(int array[], int arraySize){
  Serial.print('a'); // Array start char
  for(int i=0; i < arraySize; i++){
    if(array[i] != 0){
      Serial.print(array[i]);
      Serial.print('p');
    }
  }
  Serial.print('e'); // Array end char
}

// Bu fonksiyon tanımsız RFID Kimliklerin girişine izin vermediğini Seri Port Ekranına yazdırır
void UnknownTag()
{
//  Serial.println("This Tag isn't allowed!");
  access = false;
}

// Bu fonksiyon girişteki lcd ekranı çalıştırır
void EnterLcd()
{
  String EnterText = String("Hosgeldiniz ") + emptyarea + "'a";
  enter_lcd.setCursor(0, 0);
  enter_lcd.print(EnterText);
  enter_lcd.setCursor(2, 1);
  enter_lcd.print("Park Ediniz");
}

// Bu fonksiyon çıkıştaki lcd ekranı çalıştırır
void ExitLcd(int x)
{
String ExitText = String() + enterTime[x] + "TL OGS/HGS'den";
  exit_lcd.setCursor(0, 0);
  exit_lcd.print(ExitText);
  exit_lcd.setCursor(2, 1);
  exit_lcd.print("Kesilmistir");
}



// Bu fonksiyon girişte sürücüye sms atar
void SendEnterMessage()
{
  
 // Serial.println ("Sending Enter Message");
  sim.println("AT+CMGF=1");    // Sim modülünü metin moduna ayarlıyor
  delay(1000);
  sim.println("AT+CMGS=\"" + lastnumber + "\"\r"); // Hangi telefon numarasına mesaj göndereceğini ayarlıyor
  delay(1000);
  String SMS1 = String("Hosgeldiniz ") + emptyarea + "'a Park Ediniz";
  sim.println(SMS1);
  delay(100);
  sim.println((char)26);
  delay(1000);
  _buffer = _readSerial1;
}



// Bu fonksiyon çıkışta sürücüye sms atar
void SendExitMessage(int x)
{
 // Serial.println ("Sending Exit Message");
  sim.println("AT+CMGF=1");    // Sim modülünü metin moduna ayarlıyor
  delay(1000);
  sim.println("AT+CMGS=\"" + lastnumber + "\"\r"); // Hangi telefon numarasına mesaj göndereceğini ayarlıyor
  delay(1000);
  String SMS2 = String() + enterTime[x] + "'TL OGS/HGS Cihazinizdan kesilmistir";
  sim.println(SMS2);
  delay(100);
  sim.println((char)26);
  delay(1000);
  _buffer = _readSerial2;
}



// Bu fonksiyon boş park yerlerini bulur
void FindArea()
{

  // Tüm ultrasonik sensörlerimizin ölçtüğü mesafeyi hesaplıyoruz
  
  digitalWrite(trigger_pin1, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin1, LOW);
  ultasonictime1 = pulseIn(echo_pin1, HIGH);
  distancearray[0] = (ultasonictime1/2) / 29.1;


  digitalWrite(trigger_pin2, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin2, LOW);
  ultasonictime2 = pulseIn(echo_pin2, HIGH);
  distancearray[1] = (ultasonictime2/2) / 29.1;


  digitalWrite(trigger_pin3, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin3, LOW);
  ultasonictime3 = pulseIn(echo_pin3, HIGH);
  distancearray[2] = (ultasonictime3/2) / 29.1;


  digitalWrite(trigger_pin4, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin4, LOW);
  ultasonictime4 = pulseIn(echo_pin4, HIGH);
  distancearray[3] = (ultasonictime4/2) / 29.1;


  digitalWrite(trigger_pin5, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin5, LOW);
  ultasonictime5 = pulseIn(echo_pin5, HIGH);
  distancearray[4] = (ultasonictime5/2) / 29.1;


  digitalWrite(trigger_pin6, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin6, LOW);
  ultasonictime6 = pulseIn(echo_pin6, HIGH);
  distancearray[5] = (ultasonictime6/2) / 29.1;




  
  if( abs(distancearray[5]) >= 10) // Eğer ultrasonik senörümüzün önünde araba yoksa
  {
    if( ArrayControl(6) == false) // Ve eğer o park yerine daha önceden bir araba atamadıysak
     {emptyarea = 6  ;} // Oradaki park yeri park edilmeye uygundur           
  }
  if( abs(distancearray[4]) >= 10)
  {
    if(ArrayControl(5) == false)
    {emptyarea = 5  ;}
  }
  if( abs(distancearray[3]) >= 10)
  {
    if(ArrayControl(4) == false)
    {emptyarea = 4  ;}
  }
  if( abs(distancearray[2]) >= 10)
  {
    if(ArrayControl(3) == false)
    {emptyarea = 3  ;}
  }
  if( abs(distancearray[1]) >= 10)
  {
    if(ArrayControl(2) == false)
    {emptyarea = 2  ;}
  }
  if( abs(distancearray[0]) >= 10)
  {
   if(ArrayControl(1) == false)
   {emptyarea = 1  ;}
  }

   // Serial.println(emptyarea);
  
 }


// Bu fonksiyon herhangi bir park yerine daha önceden araba yerleştirilip yerleştirilmediğini kontrol eder
bool ArrayControl(int num)
{
  bool a = false;
  for(int y = 0 ; y < (sizeof(controlarray)/sizeof(controlarray[0])); y++)
    {
      if(controlarray[y] == num){
        a = true;
      }
    }
return a;
}




// Bu fonksiyon kontrol etme işlemini yapar
void ControlArea()
{
     
 digitalWrite(trigger_pin1, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin1, LOW);
  ultasonictime1 = pulseIn(echo_pin1, HIGH);
  distancearray[0] = (ultasonictime1/2) / 29.1;


  digitalWrite(trigger_pin2, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin2, LOW);
  ultasonictime2 = pulseIn(echo_pin2, HIGH);
  distancearray[1] = (ultasonictime2/2) / 29.1;


  digitalWrite(trigger_pin3, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin3, LOW);
  ultasonictime3 = pulseIn(echo_pin3, HIGH);
  distancearray[2] = (ultasonictime3/2) / 29.1;


  digitalWrite(trigger_pin4, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin4, LOW);
  ultasonictime4 = pulseIn(echo_pin4, HIGH);
  distancearray[3] = (ultasonictime4/2) / 29.1;


  digitalWrite(trigger_pin5, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin5, LOW);
  ultasonictime5 = pulseIn(echo_pin5, HIGH);
  distancearray[4] = (ultasonictime5/2) / 29.1;


  digitalWrite(trigger_pin6, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin6, LOW);
  ultasonictime6 = pulseIn(echo_pin6, HIGH);
  distancearray[5] = (ultasonictime6/2) / 29.1;


if( abs(distancearray[0]) > 10) // Park yeri boş ise
  {
    if (ArrayControl(1) == true) // Ve park yerine bir araç atandıysa
    {
        // Atadığımız araçların bütün kayıtlarını anlık olarak sıfırlar
        controlarray[0] = 0; 
        controlarray[1] = 0;
        controlarray[2] = 0;
        controlarray[3] = 0;
        controlarray[4] = 0;
        controlarray[5] = 0;
    }
  }
 if( abs(distancearray[1]) > 10)
  {
    if (ArrayControl(2) == true)
    {
        controlarray[0] = 0;
        controlarray[1] = 0;
        controlarray[2] = 0;
        controlarray[3] = 0;
        controlarray[4] = 0;
        controlarray[5] = 0;
    }
  }
  if( abs(distancearray[2]) > 10)
  {
    if (ArrayControl(3) == true)
    {
        controlarray[0] = 0;
        controlarray[1] = 0;
        controlarray[2] = 0;
        controlarray[3] = 0;
        controlarray[4] = 0;
        controlarray[5] = 0;
    }
  }
  if( abs(distancearray[3]) > 10)
  {
    if (ArrayControl(4) == true)
    {
        controlarray[0] = 0;
        controlarray[1] = 0;
        controlarray[2] = 0;
        controlarray[3] = 0;
        controlarray[4] = 0;
        controlarray[5] = 0;
    }
  }
  if( abs(distancearray[4]) > 10)
  {
    if (ArrayControl(5) == true)
    {
        controlarray[0] = 0;
        controlarray[1] = 0;
        controlarray[2] = 0;
        controlarray[3] = 0;
        controlarray[4] = 0;
        controlarray[5] = 0;
    }
  }
  if( abs(distancearray[5]) > 10)
  {
    if (ArrayControl(6) == true)
    {
        controlarray[0] = 0;
        controlarray[1] = 0;
        controlarray[2] = 0;
        controlarray[3] = 0;
        controlarray[4] = 0;
        controlarray[5] = 0;
    }
  }
 }
  

// Bu fonksiyon kontrol etme işleminin ne zaman yapılacağına karar verir
void Controlling()
{

  
  int s = 1;
  int ParkNumber = 0;

  while(s <= 6)
  {
    if(ArrayControl[s])
    {
      ParkNumber++;
      s++;
    }
  }
  if(ParkNumber >= 5)
  {
    ControlArea();
    s = 0;
    ParkNumber = 0;
  }
  
}





// Bu fonksiyon girişteki led ışığı boş yer varsa yeşil boş yer yoksa kırmızı yakar
void FullLed()
{

  digitalWrite(trigger_pin1, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin1, LOW);
  ultasonictime1 = pulseIn(echo_pin1, HIGH);
  distancearray[0] = (ultasonictime1/2) / 29.1;


  digitalWrite(trigger_pin2, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin2, LOW);
  ultasonictime2 = pulseIn(echo_pin2, HIGH);
  distancearray[1] = (ultasonictime2/2) / 29.1;


  digitalWrite(trigger_pin3, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin3, LOW);
  ultasonictime3 = pulseIn(echo_pin3, HIGH);
  distancearray[2] = (ultasonictime3/2) / 29.1;


  digitalWrite(trigger_pin4, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin4, LOW);
  ultasonictime4 = pulseIn(echo_pin4, HIGH);
  distancearray[3] = (ultasonictime4/2) / 29.1;


  digitalWrite(trigger_pin5, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin5, LOW);
  ultasonictime5 = pulseIn(echo_pin5, HIGH);
  distancearray[4] = (ultasonictime5/2) / 29.1;


  digitalWrite(trigger_pin6, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigger_pin6, LOW);
  ultasonictime6 = pulseIn(echo_pin6, HIGH);
  distancearray[5] = (ultasonictime6/2) / 29.1;

  analogWrite(R, 255);
  analogWrite(G, 0);
  analogWrite(B, 255);


  if( abs(distancearray[0]) <= 10)
  {
    if( abs(distancearray[1]) <= 10)
    {
      if( abs(distancearray[2]) <= 10)
      {
        if( abs(distancearray[3]) <= 10)
        {
          if( abs(distancearray[4]) <= 10)
          {
            if( abs(distancearray[5]) <= 10)
            {
              analogWrite(R, 0);
              analogWrite(G, 255);
              analogWrite(B, 255);

            }
          }
        }
      }
    }
  }
  
}
  
