#include <ESP32_Servo.h>

#include <MFRC522.h>

#include <SPI.h>
#include <ESP32AnalogRead.h>

#include <ESP32Time.h>

#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
#include "BLE2902.h"
#include "EEPROM.h"


#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" // ESP32가 데이터를 입력 받는 캐릭터리스틱 UUID (Rx)


#include <Arduino.h>
#include <U8g2lib.h>

#include <Wire.h>

//급수모터
const int pinMotorA = 2;
const int pinMotorB = 15;
//압력센서
const int pushSensorA = 25; 
const int pushSensorB = 26;
//서보모터
const int servoMotorA = 27;
const int servoMotorB = 22;

// RFID
const int rfSda = 32;    // spi 통신을 위한 SS(chip select)핀 설정
const int rfRst = 33;    // 리셋 핀 설정
const int HSPI_SCLK = 14;
const int HSPI_MISO  = 12;
const int HSPI_MOSI = 13;
#define UID1 27
#define UID2 153
MFRC522 mfrc(rfSda, rfRst);

//OLED
const int oled_mosi = 23;
const int oled_dc = 21;
const int oled_cs = 5;
const int oled_clk = 18;
//LED
const int ledRedA = 19;
const int ledGreenA = 17;
const int ledRedB = 16;
const int ledGreenB = 4;

//Btn

const int supBtn = 34;
const int modeBtn = 35;

Servo supServoA;
Servo supServoB;


int duty = 0;
int degVal1 = 0;
int degVal2 = 0;

ESP32Time rtc;

ESP32AnalogRead adc_sw;
ESP32AnalogRead adc_bat;


// 버튼
bool stateBtnSupply = 0;
bool stateBtnMode = 0;

bool bt_bit = 0;
bool response_bit = 0;

String bleValue = "";

bool medBitA = 0;
bool medBitB = 0; 
uint8_t tagState = 0; // 태그 인식비트

uint8_t screen_bit = 1;
uint8_t wscreen_bit = 0;
bool devicConnected = 0;
String user_name = "";
String med_name = "";

String user_name_1 = "JuHo";
String user_name_2 = "SungJin";

String med_name_1 = "Allestin";
String med_name_2 = "Tylenol";
bool deviceConnected = 0;
uint8_t x = 3;

//U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R1, oled_cs, oled_dc);
U8G2_SH1106_128X64_NONAME_F_4W_SW_SPI u8g2(U8G2_R0, oled_clk, oled_mosi, oled_cs, oled_dc);


std::string rx_value;
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
BLEService *pService ;
bool ble_send_bit = false;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      rx_value = pCharacteristic->getValue();
      bleValue = rx_value.c_str();
      ble_send_bit = true;  
    }
};
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer){
      deviceConnected = false;
    }
};

void ble_ready(){
  BLEDevice::init("SmartPurifier");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  pService = pServer->createService(SERVICE_UUID);
  
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID, 
                      BLECharacteristic::PROPERTY_WRITE   |
                      BLECharacteristic::PROPERTY_NOTIFY
                    ); 
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());
  
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  }

void ble_deinit(){
  bt_bit = false;
  pService->stop();
  BLEDevice::getAdvertising()->stop();
  BLEDevice::deinit(false);
}


void setup() {
  // put your setup code here, to run once:
  pinMode(supBtn, INPUT);
  pinMode(modeBtn, INPUT);
  
  pinMode(pinMotorA, OUTPUT);
  pinMode(pinMotorB, OUTPUT);
  
  pinMode(ledRedA, OUTPUT);
  pinMode(ledGreenA, OUTPUT);
  pinMode(ledRedB, OUTPUT);
  pinMode(ledGreenB, OUTPUT);
  
  pinMode(pushSensorA, INPUT);
  pinMode(pushSensorB, INPUT);
//  
//  supServoA.attach(servoMotorA);
//  supServoB.attach(servoMotorB);
//  
  ledcSetup(0, 50, 16);  // PWM CH0, Frequncy 50 Hz, 16bit resolution
  ledcAttachPin(servoMotorA, 0);  

  ledcSetup(1, 50, 16);  // PWM CH0, Frequncy 50 Hz, 16bit resolution
  ledcAttachPin(servoMotorB, 1);  

  rtc.setTime(00, 30, 10, 27, 3, 2022); // 2022년 몇월 몇일 몇시 몇분 몇초 설정
  SPI.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI);           // SPI 통신 시작
  mfrc.PCD_Init();
  u8g2.begin();
  servoWrite(0, 0);
  servoWrite(1, 0);  
    
  Serial.begin(9600);
}
void servoWrite(int ch, int deg)
{
    duty = map(deg, 0, 180, 1638, 8192);
    ledcWrite(ch, duty);
    delay(5);                             // delay를 줄이면 180도가 완전히 돌지 않음
}

void medSup(Servo supServo){
    if(medBitA == 1){
      if(degVal1 == 180){
        for (int deg = degVal1;deg >= 0; deg--) {
          servoWrite(0, deg);
          degVal1 = deg;
          }
      }
      else if(degVal1 == 0){
        for (int deg = degVal1; deg <= 180; deg++) {
          servoWrite(0, deg);
          degVal1 = deg;
          }
      }
      medBitA = 0;
    }
    if(medBitB == 1){
      if(degVal2 == 180){
        for (int deg = degVal2;deg >= 0; deg--) {
          servoWrite(1, deg);
          degVal2 = deg;
          }
      }
      else if(degVal2 == 0){
        for (int deg = degVal2 ;deg <= 180; deg++) {
          servoWrite(1, deg);
          degVal2 = deg;
          }
      }
      medBitB = 0;
    }
}

void ledLight(){
  if(analogRead(pushSensorA) > 50){
    digitalWrite(ledRedA , HIGH);
    digitalWrite(ledGreenA , LOW);
  }
  else{
    digitalWrite(ledRedA , LOW);
    digitalWrite(ledGreenA , HIGH);
  }
  if(analogRead(pushSensorB) > 50){
    digitalWrite(ledRedB , HIGH);
    digitalWrite(ledGreenB , LOW);
  }
  else{
    digitalWrite(ledRedB , LOW);
    digitalWrite(ledGreenB , HIGH);
  }
}

void CommonScreen(){
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.drawStr(40, 24, "Smart");
    u8g2.drawStr(25, 45, "Purifier");
  }
  
void WaterSupScreen(){
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.drawStr(40, 24, "Water");
    u8g2.drawStr(15, 45, "Supplying");
  }
  
void MedicineScreen(){
  if(tagState ==0){
    u8g2.setFont(u8g2_font_10x20_tf);
      u8g2.setCursor(45, 20);
    u8g2.print("Card");
      u8g2.setCursor(33, 38);
    u8g2.print("was not");
      u8g2.setCursor(15, 56);
    u8g2.print("recognized");
  }
  if(tagState == 1){
      u8g2.setFont(u8g2_font_10x20_tf);
      u8g2.setCursor(20, 16);
      u8g2.print("Hi "+user_name);
      u8g2.setCursor(25, 31);
      u8g2.print(med_name);
      u8g2.setCursor(20, 46);
      u8g2.print("have been");
      u8g2.setCursor(20, 61);
      u8g2.print("supplied!");
      uint8_t t = (rtc.getSecond() % 3);
          if(t == 0 && x == 3){
             x = 2;
            }
          else if(t == 1 && x == 3){
              x = 0;
              }
          else if(t == 2 && x == 3){
              x = 1;
              }
          else if(t == 0 && x == 0){
              x = 3;
              tagState = 0;
              }
          else if(t == 1 && x == 1){
              x = 3;
              tagState = 0;
            }
          else if(t == 2 && x == 2){
              x = 3;
              tagState = 0;
    }
  else if(tagState == 2){
      u8g2.setFont(u8g2_font_10x20_tf);
      u8g2.setCursor(25, 20);
      u8g2.print("This card");
      u8g2.setCursor(35, 38);
      u8g2.print("was not");
      u8g2.setCursor(15, 56);
      u8g2.print("registered");
      uint8_t t = (rtc.getSecond() % 3);

          if(t == 0 && x == 3){
             x = 2;
            }
          else if(t == 1 && x == 3){
              x = 0;
              }
          else if(t == 2 && x == 3){
              x = 1;
              }
      
          else if(t == 0 && x == 0){
              x = 3;
              tagState = 0;
              }
              else if(t == 1 && x == 1){
              x = 3;
              tagState = 0;
            }
          else if(t == 2 && x == 2){
              x = 3;
              tagState = 0;
         }
    }
  }
}

void BtScreen(){
  bt_con_screen();
      if(deviceConnected == 1){
        bt_tr_screen();
        if(response_bit == 1){
          bt_suc_screen();
          uint8_t t = (rtc.getSecond() % 3);

          if(t == 0 && x == 3){
             x = 2;
            }
          else if(t == 1 && x == 3){
              x = 0;
              }
          else if(t == 2 && x == 3){
              x = 1;
              }
      
          else if(t == 0 && x == 0){
              screen_bit = 1;
              x = 3;
              }
              else if(t == 1 && x == 1){
              screen_bit = 1;
              x = 3;
            }
          else if(t == 2 && x == 2){
              screen_bit = 1;
              x = 3;
            }
           }
        }
}

void bt_con_screen(){
  u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(20, 20, "Bluetooth");
  u8g2.drawStr(15, 38, "connecting");
  u8g2.drawStr(50, 56, "...");
}

void bt_tr_screen(){
      u8g2.setFont(u8g2_font_10x20_tf);
  u8g2.drawStr(20, 20, "Bluetooth");
  u8g2.drawStr(15, 38, "transfering");
  u8g2.drawStr(50, 56, "...");
}

void bt_suc_screen(){
      u8g2.setFont(u8g2_font_10x20_tf);  
  u8g2.drawStr(20, 38, "Success!");
}

void Cs_draw() {
  u8g2.firstPage();
  do{
      CommonScreen();
  }while ( u8g2.nextPage() );
}


void Ms_draw() {
  u8g2.firstPage();
  do{
      MedicineScreen();
  }while ( u8g2.nextPage() );
}

void Bs_draw() {
  u8g2.firstPage();
  do{
      BtScreen();
  }while ( u8g2.nextPage() );
}

void Ws_draw() {
  u8g2.firstPage();
  do{
      WaterSupScreen();
  }while ( u8g2.nextPage() );
}

void WaterSupply(){ // 물 공급
  // LL 멈춤, HL 역방향, LH 정방향, HH 멈춤
    digitalWrite(pinMotorA, HIGH);
    digitalWrite(pinMotorB, LOW);
}
void WaterStop(){ //공급 중단
  // LL 멈춤, HL 역방향, LH 정방향, HH 멈춤
    digitalWrite(pinMotorA, LOW);
    digitalWrite(pinMotorB, LOW);
}

void loop() {
     
  if(ble_send_bit == true){
      int indexRest = bleValue.indexOf(","); 
      int indexSlash = bleValue.indexOf("/");
      String index = bleValue.substring(0,indexSlash);
      if(index.equals("1")){
          user_name_1 = bleValue.substring(indexSlash+1,indexRest);
          med_name_1 = bleValue.substring(indexRest+1,bleValue.length());        
        }
      else if(index.equals("2")){
          user_name_2 = bleValue.substring(indexSlash+1,indexRest);
          med_name_2 = bleValue.substring(indexRest+1,bleValue.length());        
    }
      ble_send_bit = false; 
  }
  
  // put your main code here, to run repeatedly:  
  stateBtnSupply = digitalRead(supBtn);
  stateBtnMode = digitalRead(modeBtn);
  
  ledLight();
  
  if(stateBtnSupply == 0){ // 급수버튼
      WaterSupply();
      wscreen_bit = 1;
    }
  else{
      WaterStop();
      wscreen_bit = 0;
    }
  
  if(stateBtnMode == 0){ // 모드버튼
    if(screen_bit == 1 && wscreen_bit == 0 ){
      screen_bit = 2;
    }
    else if(screen_bit == 2 && wscreen_bit == 0){
      screen_bit = 3;
      tagState = 0;
      bt_bit = true;
    }
    else if(screen_bit == 3 && wscreen_bit == 0){
      screen_bit = 1;
      ble_deinit();
    }
  }
  
  if(medBitA == 1){
      medSup(supServoA);
  }
  if(medBitB == 1){
      medSup(supServoB);    
  }
  if(bt_bit == true){
    if(deviceConnected == false){
        ble_ready();
        bt_bit = false;
    }
  }
  if(screen_bit == 1 && wscreen_bit == 0){
      u8g2.clearBuffer();
      Cs_draw();
      u8g2.sendBuffer();
    }
  else if(screen_bit == 2 && wscreen_bit == 0){
      
      if ( mfrc.PICC_IsNewCardPresent() && mfrc.PICC_ReadCardSerial()){
        if(mfrc.uid.uidByte[3] == UID1){ // uid dec 3번째 배열값 == uid1
          user_name = user_name_1;
          med_name = med_name_1;
          tagState = 1;
          if(med_name.equals("Tylenol")){
            medBitA = 1;
          }
          else if(med_name.equals("Allestin")){
            medBitB = 1;            
          }
        }
        else if(mfrc.uid.uidByte[3] == UID2){
          user_name = user_name_2;
          med_name = med_name_2;
          tagState = 1;
          if(med_name.equals("Tylenol")){
            medBitA = 1;
          }
          else if(med_name.equals("Allestin")){
            medBitB = 1;            
          }
        }
        else{
          tagState = 2;        
        }
      }
      u8g2.clearBuffer();
        Ms_draw();
      u8g2.sendBuffer();
    }
  else if(screen_bit == 3 && wscreen_bit == 0){
      u8g2.clearBuffer();
      Bs_draw();
      u8g2.sendBuffer();
    }
  if(wscreen_bit == 1){
      u8g2.clearBuffer();
      Ws_draw();
      u8g2.sendBuffer();
  }
}
