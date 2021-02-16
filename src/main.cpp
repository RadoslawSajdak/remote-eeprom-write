#include <Arduino.h>
#include <RFM69.h>
#include <EEPROM.h>
#include <Adafruit_PN532.h>

#define NODEID        2    // keep UNIQUE for each node on same network
#define NETWORKID     100  // keep IDENTICAL on all nodes that talk to each other
#define GATEWAYID     1    // "central" node
#define FREQUENCY     RF69_868MHZ
#define ENCRYPTKEY    "sampleEncryptKey"
#define READ_EEPROM_KEY 'r'
#define CLEAR_EEPROM_KEY 'c'

#define PN532_IRQ     3
#define PN532_RESET   4

RFM69 radio;
int stack_pointer = 0;
uint8_t buffer[4];

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

void setup() {
  Serial.begin(115200);

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  radio.setHighPower();
  radio.encrypt(ENCRYPTKEY);

  if(EEPROM.read(EEPROM.length() - 1) == 0xff) for(int i = 0; i < EEPROM.length(); i++) EEPROM.update(i, 0x00);
  else stack_pointer = EEPROM.read(EEPROM.length() - 1);

  /* NFC Initialize */
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1);
  }
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();
  Serial.println("Init Done");
}

void loop() 
{
  boolean success;
  uint8_t uid[] = { 0, 0, 0, 0};
  uint8_t uidLength;
  

  if(radio.receiveDone())
  {
    for(int i = 0; i < 4; i++) buffer[i] = radio.DATA[i];
    if (radio.ACKRequested())
    {
      radio.sendACK();
      Serial.print(" - ACK sent  "); 
      Serial.print(buffer[0]);
      Serial.print(buffer[1]);
      Serial.print(buffer[2]);
      Serial.print(buffer[3]);
    }
    delay(500);
    for (int i = 0; i < stack_pointer; i += 4)
    {
      if( EEPROM.read(i + 0) == buffer[0] && \
          EEPROM.read(i + 1) == buffer[1] && \
          EEPROM.read(i + 2) == buffer[2] && \
          EEPROM.read(i + 3) == buffer[3])
      {
        for(int j = 0; j < 4; j++)
        {
          EEPROM.update(i + j, EEPROM.read(stack_pointer - 4 + j));
        }
        stack_pointer -= 4;
        Serial.println("Removed!");
        EEPROM.update(EEPROM.length() - 1, stack_pointer);
        goto REMOVED;
      }
    } 
    for (int i = 0; i < 4; i++)
      {
        EEPROM.update(stack_pointer + i,buffer[i]);
      }
      
      stack_pointer += 4;
      Serial.println("Added!");
      EEPROM.update(EEPROM.length() - 1, stack_pointer);
REMOVED:
      delay(10);

  }

  /* Read ALL cards from EEPROM */
  else if(Serial.available() > 0)
  {
    char input = Serial.read();
    if (input == READ_EEPROM_KEY) for(int i = 0; i < stack_pointer; i++)
    {
      if((i + 1) % 4)
      {
        if(!(i % 4)) {Serial.print(i); Serial.print(": ");}
        Serial.print(EEPROM.read(i),HEX); Serial.print(":"); 
      } 
      
      else 
      {
        Serial.print(EEPROM.read(i),HEX); Serial.print("  Pointer:"); Serial.println(stack_pointer);
      }
    }
    else if(input == CLEAR_EEPROM_KEY)
    {
      EEPROM.update(EEPROM.length() - 1, 0x00);
      stack_pointer = 0x00;
      Serial.println("Memory cleared!");
    }
  }
  else
  {
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength,100);
    if(success)
    {
      Serial.print(uid[0],HEX);Serial.print(uid[1],HEX);Serial.print(uid[2],HEX);Serial.println(uid[3],HEX);
      for (int i = 0; i < stack_pointer; i += 4)
      {
        if( EEPROM.read(i + 0) == uid[0] && \
            EEPROM.read(i + 1) == uid[1] && \
            EEPROM.read(i + 2) == uid[2] && \
            EEPROM.read(i + 3) == uid[3] )
        {
          Serial.println(i); // Here should be an action eg. turn on switch
        }
      }
      delay(200);
    }
  }
}

