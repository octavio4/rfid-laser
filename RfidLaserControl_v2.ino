/*
 * MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * The library file MFRC522.h has a wealth of useful info. Please read it.
 * The functions are documented in MFRC522.cpp.
 *
 * Based on code Dr.Leong   ( WWW.B2CQSHOP.COM )
 * Created by Miguel Balboa (circuitito.com), Jan, 2012.
 * Rewritten by Søren Thing Andersen (access.thing.dk), fall of 2013 (Translation to English, refactored, comments, anti collision, cascade levels.)
 * Released into the public domain.
 *
 * Sample program showing how to read data from a PICC using a MFRC522 reader on the Arduino SPI interface.
 *----------------------------------------------------------------------------- empty_skull 
 * Aggiunti pin per arduino Mega
 * add pin configuration for arduino mega
 * http://mac86project.altervista.org/
 ----------------------------------------------------------------------------- Nicola Coppola
 * Pin layout should be as follows:
 * Signal     Pin              Pin               Pin
 *            Arduino Uno      Arduino Mega      MFRC522 board
 * ------------------------------------------------------------
 * Reset      9                5                 RST
 * SPI SS     10               53                SDA
 * SPI MOSI   11               51                MOSI
 * SPI MISO   12               50                MISO
 * SPI SCK    13               52                SCK
 *
 * The reader can be found on eBay for around 5 dollars. Search for "mf-rc522" on ebay.com. 
 */


#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>
//#include "RDM6300.cpp"
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <Adafruit_PCD8544.h>

#define CURRENT_ON 8    // Enable/disable power for laser machine
#define BACKLIGHT 3     // Control backlight of LCD screen
#define currentPin A0  //corriente del laser ma=adc*10000/(315*1024)
#define tempPin A1      //temperatura agua  tºC=(adc-242)*35/(635-242)  635=35ºC  242=0ºC 
int current;
int temperature;
void read_adc();
void get_mastercad(String title,byte *card);
void cancelled();
void ShowReaderDetails();
int getID();
void prn(int x,int y,String tx);

// Create MFRC522 instance.
//Pin 10 - SS
//Pin 9 - RST
MFRC522 mfrc522(10, 9);  

// Software SPI (slower updates, more flexible pin options):
// pin 3 - Serial clock out (SCLK)
// pin 4 - Serial data out (DIN)
// pin 5 - Data/Command select (D/C)
// pin 7 - LCD chip select (CS)
// pin 6 - LCD reset (RST)
//Adafruit_PCD8544 display = Adafruit_PCD8544(3, 4, 5, 7, 6);   //software spi
Adafruit_PCD8544 display = Adafruit_PCD8544(5, 7, 6);  //hardaware spi

boolean match = false; // initialize card match to false

int successRead;      // Variable integer to keep if we have Successful Read from Reader
unsigned int sold_time=0 ; //(hours sold)/2
unsigned int unused_time=0 ; //(minutes of all users)/10
unsigned int nrfids=0  ;// 
byte masterCard[4];   // Stores master card's ID read from EEPROM
byte masterCard2[4];  // 
byte readCard[4];     // Stores scanned ID read from RFID Module
byte storedCard[4]; // Stores an ID read from EEPROM
byte previousCard[4];     // Stores the ID of the previously read card for matching purposes

unsigned long counterStart;
unsigned long counterCurrent;
boolean timeExpired;

byte timeAvailable;   //Stores the time available for the card in number 10 minutes

//Minimum time interval
int interval = 10;


//Session duration (for counter)
int hours;
int minutes;
int seconds;
unsigned int discount; 
unsigned char tmp_balance; //tiempo reservado pero no facturado
int nregs=(EEPROM.length()-7)/5;
int reg_size=5;
int reg_start=2;
/*
 * eeprom memory map
 * unsigned int (hours sold)/2
 * long id master1 
 * byte
 * long id master2
 * byte
 * long id user1
 * byte minutes/10
 * long id user2 ....
 */

void setup() {
 /*///erase eeprom to restart everything 
    for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }*/
  
  
  Serial.begin(9600); // Initialize serial communications with the PC
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card

  //Init display, set contrast, clear screen 
  display.begin();
  display.setContrast(50);
  //display.clearDisplay(); // clears the screen and buffer  

  pinMode(CURRENT_ON,OUTPUT);  
  pinMode(BACKLIGHT,OUTPUT);

  digitalWrite(CURRENT_ON, LOW);    
  analogWrite(BACKLIGHT,130);    

  ShowReaderDetails(); // Show details of PCD - MFRC522 Card Reader details

  // Check if master card defined, if not let user choose a master card
  // This also useful to just redefine Master Card
  // You can keep other EEPROM records just write other than 143 to EEPROM address 1
  // EEPROM address 1 should hold magical number which is '143'
  if (EEPROM.read(reg_start + 4) != 125) { 
  
  get_mastercad("mastercard1",&masterCard[0]);
  
  get_mastercad("mastercard2",&masterCard2[0]);

      for ( int j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( reg_start + j, masterCard[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
      }
      EEPROM.write( reg_start + 4,125);
      for ( int j = 0; j < 4; j++ ) {        // Loop 4 times
      EEPROM.write( reg_size+reg_start + j, masterCard2[j] );  // Write scanned PICC's UID to EEPROM, start from address 3
    }
     EEPROM.write( reg_start +reg_size+ 4,124);
    
    prn_set(1);
    prn(12,8,"Mastercards");
    prn(8,16,"have been");      
    prn(25,24,"saved");
    prns(2000);
   
   }

  //Serial.println(F("-------------------"));
  //Serial.println(F("Master Card's UID"));
  for ( int i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard[i] = EEPROM.read(reg_start + i);    // Write it to masterCard
    //Serial.print(masterCard[i], HEX); 
  }
 for ( int i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
    masterCard2[i] = EEPROM.read(reg_start+reg_size + i);    // Write it to masterCard
    //Serial.print(masterCard[i], HEX); 
  }
 sold_time=EEPROM.read(0)+(EEPROM.read(1)<<8);
 for ( int i = 2; i < nregs; i++ )
 {
  timeAvailable=EEPROM.read(reg_start+reg_size*i+4);
  if(timeAvailable){
  unused_time+=timeAvailable;
  ++nrfids;  //rfids with time
    }
 } 
  show_info();
}

int sensorValue;

void loop() {
  timeExpired = false;     //Initialize variable to control if time expired
  memset(readCard,0,4);    //Initialize readCard array
  digitalWrite(CURRENT_ON, 0); //laser pw off
  //Reinitialize time for counter  
  hours = 0;
  minutes = 10;
  seconds = 0;
  discount=0;
  tmp_balance=0;
  discount=0;
 
  do { 
  successRead = getID();
  prn_set(2);
  prn(15,0,"SCAN");
  prn(15,16,"CARD");
  show_adc();
  prns(500);
  } while (!successRead);


  //Master card has privilege to add balance to a different card (in blocks of 10 hours). After the master card is read, acan a different card to add 10 hours of balance.
  //When the master card is read, the program goes in refill mode. If the master card is read a second time, the refill mode is aborted
  
  if ( isMaster(readCard) ) {
     digitalWrite(CURRENT_ON, HIGH); //para mantenimiento 
 /*   Serial.println(F("Master card scanned"));   
    Serial.println(F("You can register a new card or add balance to an existing one"));       
    Serial.println(F("Scan a card to the reader to add balance, or scan the master card again to abort"));    
    Serial.println(F("If the card you scan it is not registered, it will be added to the EEPROM"));    
*/
    prn_set(1);
    prn(12,0,"Mastercard");
    prn(18,8,"detected");
    prns(1000);
    memset(readCard,0,4);    //Initialize readCard array
    counterStart = millis(); //Initialize counter. Expires after 30 seconds
   do {
    prn_set(1);    
    prn(15,8,"Scan card");
    prn(12,16,"to add 6h");    
    prn(12,24,"of balance");
    show_adc();
    prns(500);
    check_timeout();  
    successRead = getID();
    } while (!successRead && !timeExpired);                  // Program will not go further while you not get a successful read
    
    if(!timeExpired) 
      if ( isMaster(readCard) ) cancelled();
      else 
      {
        if(registerID(readCard)){                       //Register the card if it doesn't find it in EEPROM (with balance 0)
        timeAvailable = checkBalance(readCard, true);    //Get the time available (in numbers of time intervals)
        memcpy(previousCard,readCard,4);          //Copy the last read card to prrevious card variable, for comparison purposes
        tmp_balance=((256-timeAvailable)/6)&254;
        if (tmp_balance>6) tmp_balance=6;
     
        memset(readCard,0,4);    //Initialize readCard array
    counterStart = millis(); //Initialize counter. Expires after 30 seconds
   do {
     if((current>1) && ((tmp_balance*6+timeAvailable)<(256-12))) tmp_balance+=2;
     prn_set(1);   
        prn(12,8,"Scan again");
        prn(12,16,"to add   h");prn2(54,16,tmp_balance);     
        prn(12,24,"of balance"); 
        show_adc();   
        prns(500);
    check_timeout();  
    successRead = getID();
    }while (!successRead && !timeExpired); 
        
        if(!timeExpired)
          if ( isMaster(readCard) ) cancelled();
          else 
          {
            if(checkTwo(readCard, previousCard)) {        
              timeAvailable = addBalance(readCard,tmp_balance);
            /*  Serial.println(F("10 hours of balance added"));       
              Serial.print(F("Current balance:   "));
              Serial.print(timeAvailable*interval);
              Serial.println(" minutes");   */   
              prn_set(1);   
              prn2(0,8,tmp_balance);   
              prn(15,8,"hours of");
              prn(22,16,"balance");
              prn(27,24,"added");  
              prns(2000);  
              checkBalance(readCard,true);
                  
            } 
            else {
             // Serial.println(F("The IDs of the two cards do not match. Please, try again"));    
              prn_set(1);
              prn(10,0,"The ID's of");
              prn(5,8,"the two cards");
              prn(8,16,"do not match");    
              prn(10,24,"Please, try");
              prn(27,32,"again"); 
              prns(3000);                 
             
            }
          }
        }
      }
    } 

  //If not Master card, check if card exists, read balance and do a second read to activate the laser machine
  else {

    
      timeAvailable = checkBalance(readCard, true);   
      if(timeAvailable) {
       /* Serial.print(F("Current balance:   "));
        Serial.print(timeAvailable*interval);
        Serial.println(" minutes");
        Serial.println(F("Scan the card again if you want to start a 15 minutes session"));          
*/ 
     
        memcpy(previousCard,readCard,4);          //Copy the last read card to prrevious card variable, for comparison purposes
        memset(readCard,0,4);    //Initialize readCard array
    counterStart = millis(); //Initialize counter. Expires after 30 seconds
           do {
               prn_set(1);
                prn(10,0,"Scan again");
                prn(17,8,"to start");
                prn(22,16,"10 min");    
                prn(19,24,"session");  
                show_adc();              
                prns(500);
            check_timeout();  
            successRead = getID();
            } while (!successRead && !timeExpired);                  // Program will not go further while you not get a successful read
          
        
        if(!timeExpired) {
          if(checkTwo(readCard, previousCard)) {       
           // Serial.println(F("Starting laser cutting session...."));   
            prn_set(1);
            prn(15,16,"Starting");
            prn(18,24,"session");    
            prns(2000);
            --timeAvailable;
            //timeAvailable = subtractBalance(readCard);

           do{
           if(temperature>20)
            {
            digitalWrite(CURRENT_ON, 0);  
            prn_set(1);
            prn(0,0,"water        temperature  is > 20C");
            show_adc();
            prns(3000); 
            }
           else
           {
            digitalWrite(CURRENT_ON, HIGH);    //Turn power on for laser machine
            prn_set(2);
            prn(0,0," ready");
            show_adc();
            prns(1000);
           }
           }while(current<2 || temperature>20);
           counterStart = millis();
           do{
                prn_set(2);
                prn2(0,0,hours);
                prn(21,0,":");
                prn2(29,0,minutes);
                prn(50,0,":");
                prn2(59,0,seconds);
                            
              
              if(timeAvailable > 0) {   
                display.setTextSize(1);
                prn(5,16,"Scan to add");            
                prn(5,24,"10 min more");            
                }
                show_adc(); 
              display.display();
        

                  //timeExpired = false;
             
              memset(readCard,0,4);    
              counterStart+=1000; 
              do {
                    successRead = getID();
                    if(successRead) {
                    if (timeAvailable > 0) {
                      if(checkTwo(readCard, previousCard)) {
                        --timeAvailable ;  // = subtractBalance(readCard);  //subtract 1 interval from balance
                        //Add 10  minutes
                         minutes = minutes + 10;
                        if (minutes > 59) {
                          minutes-=60;
                          ++hours;
                        }
                       }
                    }  
                   }
                 }
              while (counterStart> millis()); // wait next second
              if(discount) --discount;
              else { subtractBalance(previousCard);discount=600; }
                          
            } while((!stepDown()) && (temperature<30)); //StepDown function returns true when counter is over 
                digitalWrite(CURRENT_ON, LOW);    //Turn power off for laser machine
                // Serial.println(F("Laser cutting session is over"));   
                prn_set(1);
                prn(0,0,"Session over");
                prns(2000);
         } 
          //Return error if ID of the card after the second read doesn't match the first read
          else {
            //Serial.println(F("The IDs of the two cards do not match. Please, try again"));    
            prn_set(1);  
            prn(10,0,"The ID's of");
            prn(5,8,"the two cards");
            prn(8,16,"do not match");    
            prn(10,24,"Please, try");
            prn(27,32,"again");                  
            prns(3000);
           }
        }
          
      }
      //If not enough balance
      else {

          //  Serial.println(F("This card doesn't have enough balance. Please, refill"));    
            prn_set(1); 
            prn(12,0,"Not enough");
            prn(20,8,"balance");
            prn(23,24,"Please");    
            prn(23,32,"refill");    
            prns(3000);
                
        
      }
  }
  
}

///////////////////////////////////////// Show details about the RFID reader ///////////////////////////////////
void ShowReaderDetails() {
  // Get the MFRC522 software version
  byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);

  Serial.print(F("MFRC522 Software Version: 0x"));
  Serial.print(v, HEX);  
  
  //Write software version to LCD screen
  prn_set(1);
 
 prn(20,0,"MFRC522");
 prn(17,12,"Software");   
 prn(20,20,"Version");  
  if (v == 0x91) {
    //Serial.print(F(" = v1.0"));
    prn(28,28,"v1.0");
   }
  else if (v == 0x92) {
   // Serial.print(F(" = v2.0"));
      prn(28,28,"v2.0");  }
  else {
    //Serial.print(F(" unknown"));
    prn(20,28,"unknown");
  }  
  prns(2000);
 
  // When 0x00 or 0xFF is returned, communication probably failed
  if ((v == 0x00) || (v == 0xFF)) {
    //Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
    //Write error message to LCD screen
    prn_set(1);
    prn(25,0,"ERROR:");
    prn(5,8,"Communication");
    prn(20,16,"failure");        
    prn(23,24,"Review");  
    prn(7,32,"connections");  
    display.display();
    while(true);  // do not go further
  }
}


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
int getID() {
  // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { //If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   //Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  //Serial.println(F("Scanned PICC's UID:"));
  for (int i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
  //  Serial.print(readCard[i], HEX);
  }
 // Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}


////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster( byte test[] ) {
  if ( checkTwo( test, masterCard ) )
    return true;
  else
   if ( checkTwo( test, masterCard2 ) )
    return true;
  else
    return false;
}


///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL )       // Make sure there is something in the array first
    match = true;       // Assume they match at first
  for ( int k = 0; k < 4; k++ ) {   // Loop 4 times
    if ( a[k] != b[k] )     // IF a != b then set match = false, one fails, all fail
      match = false;
  }
  if ( match ) {      // Check to see if if match is still true
    return true;      // Return true
  }
  else  {
    return false;       // Return false
  }
}


///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
byte findID( byte find[] ) {
  int count = nregs;  
  for ( int i = 1; i <= count; i++ ) {    // Loop once for each EEPROM entry
    readID(i);          // Read an ID from EEPROM, it is stored in storedCard[4]
    if ( checkTwo( find, storedCard ) ) {   // Check to see if the storedCard read from EEPROM    
      return i;
      break;  // Stop looking we found it
    }
    else {    // If not, return false
    }
  }
  return 0;
}




//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {
  int start = (number * reg_size ) + reg_start;     // Figure out starting position
  for ( int i = 0; i < 4; i++ ) {     // Loop 4 times to get the 4 Bytes
    storedCard[i] = EEPROM.read(start + i);   // Assign values read from EEPROM to array
  }
}


///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
boolean registerID( byte a[] ) {
  if ( !findID( a ) ) {     // Before we write to the EEPROM, check to see if we have seen this card before!
    int num = nregs;
    //search a register with 0 time and use it
    int start = reg_start-1;  
    do{
      start+=reg_size;
      if(!num--){
         prn_set(1);
      prn(0,0,"  error");
      prn(0,8,"memory is");
      prn(0,16," full");  
      prns(4000);
      return 0;
      }
    }
    while(EEPROM.read(start));
    start-=4; 
     for ( int j = 0; j < 4; j++ ) {   // Loop 4 times
      EEPROM.write( start + j, a[j] );  // Write the array values to EEPROM in the right position
    }
 
  }
  return 1;
}



///////////////////////////////////////// Add balance to ID   ///////////////////////////////////
byte addBalance( byte a[],byte horas ) {
  byte cardPosition = findID( a );
  if ( cardPosition ) {     
    int start = ( cardPosition * reg_size ) + reg_start;  // Calculate the card ID position in EEPROM
    byte currentBalance = checkBalance( a, false );
    byte newBalance = currentBalance + horas*6;
    EEPROM.write( start + 4, newBalance);  // Write new balance to EEPROM
    sold_time+=horas>>1;
    unused_time+=horas*6;
    EEPROM.write(0,sold_time&255);
    EEPROM.write(1,sold_time>>8);
    if(!currentBalance) ++nrfids;
    show_info();
    return newBalance;
  }
  else {
   // Serial.println(F("The ID does not exists"));
    return -1;
  }  
}


///////////////////////////////////////// Subtract balance to ID   ///////////////////////////////////
byte subtractBalance( byte a[] ) {
  byte cardPosition = findID( a );
  if ( cardPosition ) {     
    int start = ( cardPosition * reg_size ) + reg_start;  // Calculate the card ID position in EEPROM
    byte currentBalance = checkBalance( a, false );
    if (currentBalance > 0 ) {
      EEPROM.write( start + 4, --currentBalance);  // Write new balance to EEPROM
      if (!currentBalance) --nrfids; 
      --unused_time;
      return currentBalance;
    }
  } 
     return 0;
}


///////////////////////////////////////// Check balance from ID  ///////////////////////////////////
byte checkBalance( byte a[], boolean printLCD ) {
  byte cardPosition = findID( a );
  if ( cardPosition ) {     
    int start = ( cardPosition * reg_size ) + reg_start;  // Calculate the card ID position in EEPROM
    byte balance = EEPROM.read( start + 4);  // Read balance from EEPROM

   /* Serial.print(F("Current balance:   "));
    Serial.print(balance*interval);
    Serial.println(" minutes");    
    */
    if (printLCD) {
      prn_set(1);
      prn(15,0,"Balance:");
      display.setTextSize(2);
      prn(15,12,balance*interval);  
      prn(15,28,"min");    
      prns(3000);
    }
    
    return balance;
  }
  else {
    //Serial.println(F("The ID does not exists"));

    if (printLCD) {
      prn_set(1);
      prn(12,16,"ID does not");
      prn(18,24,"exists");    
      prns(2000);       
    }
    return 0;
  }
}


///////////////////////////////////// Step down counter /////////////////////////////////////////////
boolean stepDown() {
    if (seconds > 0) {
      seconds -= 1;
      return false;
    } else {
      if (minutes > 0) {
        seconds = 59;
        minutes -= 1;
        return false;
      } else {
        if (hours > 0) {
          seconds = 59;
          minutes = 59;
          hours -= 1;
          return false;
        } else {
          return true;
        }
      }
    }
}

void show_adc()
{
  read_adc();
  display.setTextSize(1);
  prn2(24,32,current);display.print("ma");
  prn2(60,32,temperature);display.print("C");
}

void show_info()
{
  prn_set(1);
  prn(0,0,"sold time:");
  prn(0,8,sold_time*2); display.print("hours");
  prn(0,16,"unused time:");
  prn(0,24,unused_time*10); display.print("min in");
  prn(0,32,nrfids);display.print(" rfids");
  prns(6000);

}

void read_adc() {
  current=analogRead(currentPin)*(float)10000/((long)315*1024);
  temperature=(analogRead(tempPin)-242)*((float)35/(635-242));
   }

void get_mastercad(String title,byte *card)
{ 
    display.clearDisplay();
     display.setTextSize(1);
    display.setTextColor(BLACK);
    display.setCursor(12,0);
    display.print(title);
    display.setCursor(10,8);  
    display.print("not defined");
    display.setCursor(18,24);  
    display.print("Scan new");     
    display.setCursor(12,32);  
    display.print(title);  
    display.display();
    
    do {
      successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
    }
    while (!successRead);                  // Program will not go further while you not get a successful read
    for ( int j = 0; j < 4; j++ ) {        // Loop 4 times
      card[j]=readCard[j];  
      }
   
   
    display.clearDisplay(); // clears the screen and buffer  
 
   display.setCursor(0,8);
    display.print(title);
    
    for ( int i = 0; i < 4; i++ ) {          // Read Master Card's UID from EEPROM
      display.setCursor(i*21,30);
      display.print(masterCard[i], HEX);
    }
    display.display();
    delay(5000);
     return;
}

void cancelled()
{           // Serial.println(F("The refill operation has been canceled"));  
            display.clearDisplay(); // clears the screen and buffer        
            display.setTextSize(1);
            display.setTextColor(BLACK);
            display.setCursor(12,16);
            display.print("Operation");
            display.setCursor(18,24);
            display.print("aborted");    
            display.display();
            delay(2000);       
          
  }

void prn_set(int ts)
  {
  display.clearDisplay(); // clears the screen and buffer        
  display.setTextSize(ts);
  display.setTextColor(BLACK);
  }

void prn(int x,int y,String tx)
  {
  display.setCursor(x,y);
  display.print(tx);
  }

void prn(int x,int y,int tx)
  {
  display.setCursor(x,y);
  display.print(tx);
   }

void prn2(int x,int y,unsigned char tx)
  {
    display.setCursor(x,y);
    if (tx>99) tx=99; 
    if (tx < 10) display.print("0");
    display.print(tx);
  }
                
void prns(int time)
  { display.display();
  delay(time);
    }  

void check_timeout()
{
      if(millis() - counterStart > 20000) {
       // Serial.println(F("Time expired. Operation aborted"));
        timeExpired = true;
        prn_set(1);
        prn(12,16,"Operation");
        prn(18,24,"expired");    
        prns(2000);
        }
}

   
