#include <TimerOne.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Servo.h>

#include "ex.h"

#define RST_PIN 49
#define SS_PIN 53 //pinii pentru modul RF

byte readCard[];
char* myTags[100] = {};
int tagsCount = 0;
String tagID = "";
boolean successRead = false;
boolean correctTag = false;
boolean ledAprins = false;
int proximitySensor;
boolean doorOpened = false;
const int pin_RS = 8;
const int pin_EN = 9;
const int pin_d4 = 4;
const int pin_d5 = 5;
const int pin_d6 = 6;
const int pin_d7 = 7;
const int pin_BL = 10;

const int servo = 2;
int servoLockedPos = 90;
int servoOpenedPos = 0;

int echoPin = 42;
int trigPin = 43;

int ledPin = 13;

// Create Instances
MFRC522 mfrc522(SS_PIN, RST_PIN);  //RF module
Servo myServo;                     //Servo motor
LiquidCrystal lcd(pin_RS, pin_EN, pin_d4, pin_d5, pin_d6, pin_d7); //ecran

//notele, pe rand
int melody[] = { NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4 };

//introducem durata pentru fiecare nota, pe rand
int noteDurations[] = { 250, 125, 125, 250, 250, 250, 250, 250 };

//nota adaugare/stergere cartela
int melodyAddDelete = NOTE_C4;

//functie sunet adaugare/stergere cartela
void singAddDelete(void) {
    int noteDurationAddDelete = 250; //ms?
    //apelam functia tone pentru difuzorul atasat la pinul 12, nota si durata
    tone(12, melodyAddDelete, noteDurationAddDelete);
    delay(noteDurationAddDelete);
    noTone(12);
}

//functie sunet cantec inchis/deschis usa
void sing(void) {
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    //apelam functia de tone pentru difuzorul atasat la pinul 12, nota si durata corespunzatoare
    tone(12, melody[thisNote], noteDurations[thisNote]);
    delay(noteDurations[thisNote]);
    noTone(12);
  }
}

// Defining functions
bool getID();
void printNormalModeMessage();


void setup() {

  pinMode(ledPin, OUTPUT);

  //For RFModule
  SPI.begin();
  mfrc522.PCD_Init();

  myServo.attach(servo);          // obiectul myServo la pinul 2
  myServo.write(servoLockedPos);  // seteaza myServo la 90 de grade -> usa blocata

  //For LCD
  lcd.begin(16, 2);
  lcd.print("Card nedetectat!");
  lcd.setCursor(0, 1);
  lcd.print("Introduceti card");

  //setam cardul de Admin - primul card - (pentru a adauga sau sterge alte carduri)
  //prima cartela va deveni cea de admin
  while (!successRead) {
    successRead = getID();
    if (successRead == true) {
      myTags[tagsCount] = strdup(tagID.c_str());
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cartela setata");
      tagsCount++;
    }
  }
  successRead = false;
  printNormalModeMessage(); //afisam mesajul normal
}

void loop() {
  //citim valoarea senzorului de proximitate; pe baza lui deschidem / inchidem usa
  int proximitySensor = analogRead(A0);  //citirea valorii analogice. -doar ca pe pin de intrare
  
  //Check if RFModule detected a RFCard
  successRead = getID();
  
  // verificam daca cardul scanat este cel de admin
  if (tagID == myTags[0]) {
    lcd.clear();
    lcd.print("Mod Programare:");
    lcd.setCursor(0, 1);
    lcd.print("Add/Del Tag");

    //incearca sa gaseasca noul card
    while (!successRead) {
      successRead = getID();
      if (successRead == true) {
        if (tagID == myTags[0]){
            singAddDelete();
            printNormalModeMessage();
            tagID = "";
            return;  
          }
        for (int i = 1; i < 100; i++) {
          if (tagID == myTags[i]) {
            myTags[i] = "";
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Tag Sters!");
            singAddDelete();
            printNormalModeMessage();
            return;
          }
        }
        myTags[tagsCount] = strdup(tagID.c_str());
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Tag Adaugat!");
        singAddDelete();
        printNormalModeMessage();
        tagsCount++;
        return;
      }
    }
  }

  if (successRead && tagID != myTags[0]) {
    // Checks whether the scanned tag is authorized
    for (int i = 1; i < 100; i++) {
      if (tagID == myTags[i]) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Acces Permis!");
        lcd.setCursor(0, 1);
        lcd.print("Puteti intra");
        myServo.write(servoOpenedPos);  // deschide usa
        doorOpened = true;
        printNormalModeMessage();
        sing();
        correctTag = true;
      }
    }
    if (correctTag == false) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(" Acces Respins!");
      lcd.setCursor(0, 1);
      lcd.print("Mai incearca");

      digitalWrite(ledPin, HIGH);  // turn the LED on (HIGH is the voltage level)
      delay(1000);                 // wait for a second
      digitalWrite(ledPin, LOW);   // turn the LED off by making the voltage LOW

      printNormalModeMessage();
    }
    if (doorOpened) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Usa deschisa!");
      while (doorOpened) {
        proximitySensor = analogRead(A0);
        if (proximitySensor < 500) {
          doorOpened = false;  //usa inchisa
          sing();
          myServo.write(servoLockedPos);  // Locks the door
          printNormalModeMessage();
        }
      }
    }
  }
  correctTag = false;
}


//preluarea bitilor pentru tagID in cazul in care s-a incheiat citirea
bool getID() {
  // Getting ready for Reading PICCs
  if (!mfrc522.PICC_IsNewCardPresent()) {  //If a new PICC placed to RFID reader continue
    return false;
  }
  if (!mfrc522.PICC_ReadCardSerial()) {  //Since a PICC placed get Serial and continue
    return false;
  }
  tagID = "";
  for (uint8_t i = 0; i < 4; i++) {  // The MIFARE PICCs that we use have 4 byte UID
    readCard[i] = mfrc522.uid.uidByte[i];
    tagID.concat(String(mfrc522.uid.uidByte[i], HEX));  // Adds the 4 bytes in a single String variable
  }
  tagID.toUpperCase();
  mfrc522.PICC_HaltA();  // Stop reading
  return true;
}

void printNormalModeMessage() {
  delay(1500);
  lcd.clear();
  lcd.print("-Verifica identitatea-");
  lcd.setCursor(0, 1);
  lcd.print("Scaneaza cartela");
}
