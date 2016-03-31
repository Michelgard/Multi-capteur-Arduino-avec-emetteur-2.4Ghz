#include <DHT.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <SPI.h>
#include "nRF24L01.h"
#include <RF24.h>
#include "Barometer.h"
#include <Wire.h>
#include <OneWire.h> // Inclusion de la librairie OneWire
 
#define DS18B20 0x28     // Adresse 1-Wire du DS18B20
#define BROCHE_ONEWIRE 7 // Broche utilisée pour le bus 1-Wire
 
OneWire ds(BROCHE_ONEWIRE); // Création de l'objet OneWire ds

#define DHTPIN 6 // entrée capteur dht22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//variables radio
RF24 radio(9,10); //création instance radio (pin ce,csn) 
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL,0xF0F0F0F0D2LL };

Barometer myBarometer;

// Fonction récupérant la température depuis le DS18B20
// Retourne true si tout va bien, ou false en cas d'erreur
boolean getTemperature(float *temp){
  byte data[9], addr[8];
  // data : Données lues depuis le scratchpad
  // addr : adresse du module 1-Wire détecté
 
  if (!ds.search(addr)) { // Recherche un module 1-Wire
    ds.reset_search();    // Réinitialise la recherche de module
    return false;         // Retourne une erreur
  }
   
  if (OneWire::crc8(addr, 7) != addr[7]) // Vérifie que l'adresse a été correctement reçue
    return false;                        // Si le message est corrompu on retourne une erreur
 
  if (addr[0] != DS18B20) // Vérifie qu'il s'agit bien d'un DS18B20
    return false;         // Si ce n'est pas le cas on retourne une erreur
 
  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sélectionne le DS18B20
   
  ds.write(0x44, 1);      // On lance une prise de mesure de température
  delay(2000);             // Et on attend la fin de la mesure
   
  ds.reset();             // On reset le bus 1-Wire
  ds.select(addr);        // On sélectionne le DS18B20
  ds.write(0xBE);         // On envoie une demande de lecture du scratchpad
 
  for (byte i = 0; i < 9; i++) // On lit le scratchpad
    data[i] = ds.read();       // Et on stock les octets reçus
   
  // Calcul de la température en degré Celsius
  *temp= ((data[1] << 8) | data[0]) * 0.0625; 
   
  // Pas d'erreur
  return true;
}

// nrf24 debug
int serial_putc( char c, FILE * ) 
{
  Serial.write( c );
  return c;
} 

void printf_begin(void)
{
  fdevopen( &serial_putc, 0 );
} 

void setup()
{
  //printf_begin();
  //Serial.begin(9600); //Debug 
  dht.begin();
  myBarometer.init();

  radio.begin();
  radio.setChannel(0x4c);
  radio.setAutoAck(1);
  radio.setRetries(15,15);
  radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(32);
  radio.openReadingPipe(1,pipes[0]);
  radio.openWritingPipe(pipes[1]);
  radio.stopListening();
  //radio.printDetails(); //for Debugging
}

void loop (){
  float txbuffer[5] = {0, 0, 0, 0, 0};
  char SendPayload[32]  = "RS1 :";
  char SendPayload2[32]  = "RS2 :";
  char tempstr[10] = "";
  
  txbuffer[0] = dht.readTemperature();
  txbuffer[1] = dht.readHumidity();
  txbuffer[2] = myBarometer.bmp085GetTemperature(myBarometer.bmp085ReadUT());
  txbuffer[3] = (myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP())/133.32239)*1.33322;
  
  strcat(SendPayload, "T");
  dtostrf(txbuffer[0],2,2,tempstr); 
  strcat(SendPayload,tempstr); 
  strcat(SendPayload, "T"); 
  
  strcat(SendPayload, "H");
  dtostrf(txbuffer[1],2,2,tempstr); 
  strcat(SendPayload,tempstr); 
  strcat(SendPayload, "H"); 
   
  strcat(SendPayload2, "M");
  dtostrf(txbuffer[2],2,2,tempstr); 
  strcat(SendPayload2,tempstr); 
  strcat(SendPayload2, "M");
   
  strcat(SendPayload2, "P");
  dtostrf(txbuffer[3],2,2,tempstr); 
  strcat(SendPayload2,tempstr); 
  strcat(SendPayload2, "P");
   
  if(getTemperature(&txbuffer[4])) {
    strcat(SendPayload2, "E");
    dtostrf(txbuffer[4],2,2,tempstr); 
    strcat(SendPayload2,tempstr); 
    strcat(SendPayload2, "E");
  }
  
  Serial.println(SendPayload);
  Serial.println(SendPayload2);
  radio.powerUp(); //alimente le module nrf24l01+
  delay(1000);
  bool ok = radio.write(&SendPayload,sizeof(SendPayload));
  bool ok2 = radio.write(&SendPayload2,sizeof(SendPayload2));
  delay(1000);
  radio.powerDown(); //arrêt de l’alimentation du module nrf24l01+
  for (int i = 0; i < 100; i++){
    delay(3000);
  }
}
