#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <SPI.h>
#include <MFRC522.h>



#define RST_PIN         D3          // Configurable, see typical pin layout above
#define SS_PIN          D8        // Configurable, see typical pin layout above

#define TR_PIN          D4
#define HAL_PIN         A0

#define BIP_PIN         D0





const int transistor = 14;  // Assigning name to Trasistor 

AsyncWebServer server(80);


unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 25000;

String masterCard = "e5365e64";


MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance


String data;

char ssid[] = "Lalala";       // your network SSID (name)
char password[] = "0892387639";  // your network key


long checkCoffeeDueTime;
int checkCoffeeDelay = 10000; // 60 x 1000 (1 minute)


LiquidCrystal_I2C lcd(0x27,16,2);

void beep(int delayms) {
  // analogWrite(buzzerPin, 20);
  digitalWrite(BIP_PIN, HIGH);
  Serial.println("buzz HIGH");
  Serial.println(delayms);
  delay(delayms);
  // analogWrite(buzzerPin, 0);
  digitalWrite(BIP_PIN, LOW);
  Serial.println("buzz LOW");
  delay(delayms);
}

int getMaxCurrent(int interval, int delayms);

void enterServiceMode() {
    Serial.println("D4 pin LOW "); 
    digitalWrite(TR_PIN, LOW);   // making pin low, ACTIVATE relay
    beep(50);
}

void exitServiceMode() {
    Serial.println("D4 pin HIGH "); 
    digitalWrite(TR_PIN, HIGH);   // making pin high, OPEN relay
    beep(50);
}


void setup() {

   pinMode(BIP_PIN, OUTPUT); 

    tone(BIP_PIN, 3000, 1500);    
    // stop the waveform generation before the next note.
    noTone(BIP_PIN);

  // beep(500);
  // delay(500);
  // beep(500);
  // delay(500);
  // beep(500);

  pinMode(TR_PIN, OUTPUT);     // Assigning pin as output
  Serial.println("D4 pin HIGH ");                 
  digitalWrite(TR_PIN, HIGH);    // making pin HIGH - DEACTIVATE RELAY

  pinMode(HAL_PIN, INPUT);     // Assigning pin as input for Hal sensor

  Serial.begin(115200);

  SPI.begin();			// Init SPI bus
  mfrc522.PCD_Init();		// Init MFRC522

  mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

  lcd.init();                      // initialize the lcd 
  Serial.println("LCD Initialized");
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("LCD Initialized!");

  delay(500);

  Serial.println(F("Initializing FS ... "));
  if (SPIFFS.begin()) {
    Serial.println(F("done."));
    lcd.setCursor(0,1);
    lcd.print("FS Initialized! ");
  }else{
    Serial.println(F("fail."));
  }


// delete WHITE card file

 if(SPIFFS.exists( (String)"/" + masterCard + ".txt" )) {

    Serial.println((String)"Found master card file ... " + data);
    // checkUser(fileNameString);
    Serial.println("deleting...");

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("deleting master");
    lcd.setCursor(0,1);
    lcd.print(masterCard);

    SPIFFS.remove((String)"/" + masterCard + ".txt" );
    // resetUser(dataFile);

    delay(1500);

}

// delete end


  Serial.println("File sistem info.");
  lcd.setCursor(0,0);
  lcd.print("File sistem info:");

  FSInfo fs_info;
  SPIFFS.info(fs_info);

  Serial.print("Total space:      ");
  Serial.print(fs_info.totalBytes);
  Serial.print("bytes");
  Serial.println();

  Serial.print("Total space used:      ");
  Serial.print(fs_info.usedBytes);
  Serial.print("bytes");
  Serial.println();
  lcd.setCursor(0,1);
  lcd.print((String)"in use: " + fs_info.usedBytes );

  Serial.print("Block size:      ");
  Serial.print(fs_info.blockSize);
  Serial.print("bytes");
  Serial.println();

  Serial.print("Max path length:      ");
  Serial.print(fs_info.maxPathLength);
  Serial.println();


  Dir dir = SPIFFS.openDir("/");
  while(dir.next()) {
    Serial.print(dir.fileName());
    Serial.print(" - ");
    if(dir.fileSize()) {
      File f = dir.openFile("r");
      Serial.println(f.size());
      f.close();
    } else {
      Serial.println("0");
    }
  }

};


template<typename T1, typename T2>
void display( T1 line1, T2 line2 ) {
  lcd.setCursor(0,0);
  lcd.print((String)line1);
  lcd.setCursor(0,1);
  lcd.print((String)line2);
}

int getMaxCurrent(int interval, int deltams) {
  int max_current = 0;

  int count = interval / deltams;
  int i;

  for (i = 0; i < count; i++) {
    int current = analogRead(HAL_PIN);
    if (max_current < current )  {
      max_current = current;
    }
    delay(deltams);
  }

  return max_current;
};

// function to watch for current to first get higher than
// lowCurrentLevel anf then lower than lowCurrentLevel
//  it only passes when current goes high, then low,
//  thus stoping the code untill the coffee grinder motor makes 
//  its cycle first
void watchCurrent(int lowCurrentLevel, int highCurrentLevel) {

  while ( getMaxCurrent(1000, 5) < highCurrentLevel ) {
    Serial.println(" high current NOT reached !");
  }

  Serial.println(" ----- high current !!! ----- ");

  while ( getMaxCurrent(1000, 5) > lowCurrentLevel ) {
    Serial.println(" low current NOT reached !");
  }

  Serial.println(" ----- low current !!! ----- ");

}

bool dumpUserFiles() {
  String title = "";
  String headers = "";
  String body = "";
  bool finishedHeaders = false;
  bool currentLineIsBlank = true;
  bool gotResponse = false;
  bool gotConnect = false;
  long now;

  char host[] = "wagtaildemo.artiomnovosiolo.repl.co";

  WiFiClientSecure client;
  client.setInsecure();

  // Serial.println("trying to connect to host ...");
  // Serial.println((String)"Could connect ... " + gotConnect);

  // gotConnect = client.connect(host, 443);



  if (client.connect(host, 443)) {
    gotConnect = true;
    Serial.println("connected");
    
    // String URL = "/r/" + sub + "/new?limit=1";
    String URL = "/sendcoffee/";

    Serial.println(URL);
    

    Dir dir = SPIFFS.openDir("/");
    while(dir.next()) {
      Serial.print(dir.fileName());
      Serial.print(" - ");
      if(dir.fileSize()) {
        File f = dir.openFile("r");
        Serial.println(f.size());

        String fileLine = f.readStringUntil('\n');

        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(fileLine);
        f.close();

        String cardNumber_fromFile = root["card_number"];
        int coffeeCount = root["coffee_count"];
        String userName = root["user_name"];
        int coffeePool = root["coffee_pool"];

        String parameterString = (String)"?card_number=" + 
          cardNumber_fromFile + "&" +
          "user_name=" + userName + "&" +
          "coffee_count=" + coffeeCount + "&" +
          "coffee_pool=" + coffeePool;

        client.println("GET " + URL + parameterString + " HTTP/1.1");
        client.print("Host: "); client.println(host);
        client.println("User-Agent: arduino/1.0");
        client.println("");

      }  else {
        Serial.println("0");
      }
    }



    // client.println("GET " + URL + " HTTP/1.1");
    // client.print("Host: "); client.println(host);
    // client.println("User-Agent: arduino/1.0");
    // client.println("");

    now = millis();
    // checking the timeout
    while (millis() - now < 1500) {
      while (client.available()) {
        char c = client.read();
        Serial.print(c);

        if (finishedHeaders) {
          body=body+c;
        } else {
          if (currentLineIsBlank && c == '\n') {
            finishedHeaders = true;
          }
          else {
            headers = headers + c;
          }
        }

        if (c == '\n') {
          currentLineIsBlank = true;
        }else if (c != '\r') {
          currentLineIsBlank = false;
        }

        //marking we got a response
        gotResponse = true;

      }
      if (gotResponse) {


        // Serial.println("  gotResponse  ");


        // DynamicJsonBuffer jsonBuffer;
        // JsonObject& root = jsonBuffer.parseObject(body);
        // int coffeeCount = root["coffee_count"];


        Serial.println(body);
        // lcd.setCursor(0,0);
        // lcd.print("coffee cups");
        // lcd.setCursor(0,1);
        // lcd.print(coffeeCount);
        
        

        break;
      }
    }
  }

  // return gotResponse;
  return gotConnect;
  
  // return true;

}

void checkUser(String fileName) {

  File f = SPIFFS.open(fileName, "r+");   
  String fileLine = f.readStringUntil('\n');
        
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(fileLine);

  String cardNumber_fromFile = root["card_number"];
  int coffeeCount = root["coffee_count"];
  String userName = root["user_name"];
  int coffeePool = root["coffee_pool"];

  Serial.println((String)"user: " + userName + " - coffee count: " + coffeeCount + " - coffee pool: " + coffeePool);

  if ( coffeePool > 0 ) {

    String dString = "sum:" + String(coffeeCount) + " left:" + String(coffeePool);
    lcd.clear();
    display(userName, 
            dString);

    Serial.println("D4 pin LOW "); 
    digitalWrite(TR_PIN, LOW);   // making pin low, ACTIVATE relay
    // delay(25000);

    watchCurrent(100, 200);

    Serial.println("D4 pin HIGH ");                 
    digitalWrite(TR_PIN, HIGH);    // making pin high, SHUT DOWN relay
    // delay(1000); 
 

    coffeeCount += 1;
    // coffeePool -= 1;
    root["coffee_count"] = coffeeCount;
    root["coffee_pool"] = coffeePool;


    dString = "sum:" + String(coffeeCount) + " left:" + String(coffeePool);
    lcd.clear();
    display(userName, 
            dString);

    beep(50);

    f.seek(0); 
    Serial.println("Writing to file... ");
    root.printTo(f);
    root.printTo(Serial);
    Serial.println();
    f.close();

    return;
  } else {
    f.close();
    Serial.println((String)"user " + userName + " is blocked from drinking coffee :(");
    
    lcd.clear();
    display(userName, 
            "POOL EMPTY !");

    return;
  }

}


void addCreditToUser(String fileName, int amount) {

  File f = SPIFFS.open(fileName, "r+");   
  String fileLine = f.readStringUntil('\n');
        
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(fileLine);

  String cardNumber_fromFile = root["card_number"];
  int coffeeCount = root["coffee_count"];
  String userName = root["user_name"];
  int coffeePool = root["coffee_pool"];

  Serial.println((String)"user: " + userName + " - coffee count: " + coffeeCount + " - coffee pool: " + coffeePool);


  coffeePool += amount;
  
  root["coffee_pool"] = coffeePool;

  f.seek(0); 
  Serial.println("Writing to file... ");
  root.printTo(f);
  root.printTo(Serial);

  lcd.clear();
  // lcd.setCursor(0,0);
  // lcd.print(userName);
  // lcd.setCursor(0,1);
  // lcd.print((String)"sum:" + coffeeCount + " left:" + coffeePool );

  String dString = "sum:" + String(coffeeCount) + " left:" + String(coffeePool);

  display( userName, 
           dString );


  Serial.println();
  f.close();

  return;
 
}


void resetUser(String fileName) {

  File f = SPIFFS.open(fileName, "r+");   
  String fileLine = f.readStringUntil('\n');
        
  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(fileLine);

  String cardNumber_fromFile = root["card_number"];
  int coffeeCount = root["coffee_count"];
  String userName = root["user_name"];
  int coffeePool = root["coffee_pool"];

  Serial.println((String)"user: " + userName + " - coffee count: " + coffeeCount + " - coffee pool: " + coffeePool);


  coffeePool = 100;
  
  root["coffee_pool"] = coffeePool;

  f.seek(0); 
  Serial.println("Writing to file... ");
  root.printTo(f);
  root.printTo(Serial);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(userName);
  lcd.setCursor(0,1);
  lcd.print((String)"sum:" + coffeeCount + " left:" + coffeePool );


  Serial.println();
  f.close();

  return;
 
}

void scanForWiFi() {
  // Serial.println("Scanning WiFi networks ... ");
  int numberOfNetworks = WiFi.scanNetworks();
  // Serial.println((String)"Scan results ... found " + numberOfNetworks + " networks");

  if (numberOfNetworks > 0) {

    bool foundLalala = false;

    for(int i = 0; i < numberOfNetworks; i++) {
      String networkName = WiFi.SSID(i);
      // Serial.println((String)"Network name ... " + networkName);

      if (networkName == "Lalala") {
        foundLalala = true;
        break;
      }
    }

    if (foundLalala) {
      Serial.println((String)"found master Network ... Lalala");

      WiFi.mode(WIFI_STA);
      WiFi.disconnect();
      delay(100);

      // Attempt to connect to Wifi network:
      Serial.print("Connecting Wifi: ");
      // lcd.setCursor(0,0);
      // lcd.print("Connecting Wifi: ");
      // lcd.setCursor(0,1);
      // lcd.print("    ........    ");

      Serial.println(ssid);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);

        lcd.setCursor(0,0);
        lcd.print("Connecting Wifi: ");
        lcd.setCursor(0,1);
        lcd.print("    ........    ");
      }

      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      IPAddress ip = WiFi.localIP();
      Serial.println(ip);

      lcd.clear();

      while (WiFi.status() == WL_CONNECTED) {

        Serial.println("WiFi connected ...");
        delay(1000);

        lcd.setCursor(0,0);
        lcd.print("WiFi Connected: ");
        lcd.setCursor(0,1);
        lcd.print(ip);
        
        
        if (dumpUserFiles()) {

          lcd.clear();

          lcd.setCursor(0,0);
          lcd.print("   --------->   ");
          lcd.setCursor(0,1);
          lcd.print("   Files sent   ");
          delay(5000);
          break;
        }

      }

      lcd.clear();

    }

  }
}

String dump_byte_array(byte *buffer, byte bufferSize) {
  String s = "";
  for (byte i = 0; i < bufferSize; i++) {
    s += String(buffer[i], HEX);
  }
  return s;
}

	void mfrc522_fast_Reset()
	{
		digitalWrite(RST_PIN, HIGH);
		mfrc522.PCD_Reset();
		mfrc522.PCD_WriteRegister(mfrc522.TModeReg, 0x80);			// TAuto=1; timer starts automatically at the end of the transmission in all communication modes at all speeds
		mfrc522.PCD_WriteRegister(mfrc522.TPrescalerReg, 0x43);		// 10μs.
	//	mfrc522.PCD_WriteRegister(mfrc522.TPrescalerReg, 0x20);		// test

		mfrc522.PCD_WriteRegister(mfrc522.TReloadRegH, 0x00);		// Reload timer with 0x064 = 30, ie 0.3ms before timeout.
		mfrc522.PCD_WriteRegister(mfrc522.TReloadRegL, 0x1E);
		//mfrc522.PCD_WriteRegister(mfrc522.TReloadRegL, 0x1E);

		mfrc522.PCD_WriteRegister(mfrc522.TxASKReg, 0x40);		// Default 0x00. Force a 100 % ASK modulation independent of the ModGsPReg register setting
		mfrc522.PCD_WriteRegister(mfrc522.ModeReg, 0x3D);		// Default 0x3F. Set the preset value for the CRC coprocessor for the CalcCRC command to 0x6363 (ISO 14443-3 part 6.2.4)

		mfrc522.PCD_AntennaOn();						// Enable the antenna driver pins TX1 and TX2 (they were disabled by the reset)
	}

	bool mfrc522_fastDetect3()
	{
		byte validBits = 7;
		MFRC522::StatusCode status;
    // Serial.println((String)"MFRC522 status code ... " + status);
		byte command = MFRC522::PICC_CMD_REQA;
		byte waitIRq = 0x30;		// RxIRq and IdleIRq
		byte n;
		uint16_t i;

		mfrc522.PCD_ClearRegisterBitMask(MFRC522::CollReg, 0x80);		// ValuesAfterColl=1 => Bits received after collision are cleared.

		//mfrc522.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Idle);			// Stop any active command.
		mfrc522.PCD_WriteRegister(MFRC522::ComIrqReg, 0x7F);					// Clear all seven interrupt request bits
		mfrc522.PCD_SetRegisterBitMask(MFRC522::FIFOLevelReg, 0x80);			// FlushBuffer = 1, FIFO initialization
		mfrc522.PCD_WriteRegister(MFRC522::FIFODataReg, 1, &command);			// Write sendData to the FIFO
		mfrc522.PCD_WriteRegister(MFRC522::BitFramingReg, validBits);			// Bit adjustments
		mfrc522.PCD_WriteRegister(MFRC522::CommandReg, MFRC522::PCD_Transceive);				// Execute the command
		mfrc522.PCD_SetRegisterBitMask(MFRC522::BitFramingReg, 0x80);			// StartSend=1, transmission of data starts

		i = 10;
		while (1) {
			n = mfrc522.PCD_ReadRegister(MFRC522::ComIrqReg);	// ComIrqReg[7..0] bits are: Set1 TxIRq RxIRq IdleIRq HiAlertIRq LoAlertIRq ErrIRq TimerIRq
			if (n & waitIRq) {					// One of the interrupts that signal success has been set.
				break;
			}
			if (n & 0x01) {						// Timer interrupt - nothing received in 25ms
				return false;
			}
			if (--i == 0) {						// The emergency break. If all other conditions fail we will eventually terminate on this one after 35.7ms. Communication with the MFRC522 might be down.
				return false;
			}
		}

		return true;
	}




void loop() {

  display("waiting for card", 
          "----------------");

  if (WiFi.status() != WL_CONNECTED) {
    scanForWiFi();
  }


  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

    // Show some details of the PICC (that is: the tag/card)
  data = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);

  String dataFile = (String)"/" + data + ".txt";



  if (((millis() - lastDebounceTime) > debounceDelay ) || ( lastDebounceTime == 0 )) {
    
    lastDebounceTime = millis();

    Serial.println(data);


    if (data != masterCard) {

      String dataFile = (String)"/" + data + ".txt";
      Serial.println((String)"Card presented ... " + data);

      // lcd.clear();
      display("Card presented: ", 
              data);

      delay(2000);


      if (SPIFFS.exists(dataFile)) {

        checkUser(dataFile);
        delay(4500);
        
      } else {
        Serial.println((String)"No account for card number: " + data);

        lcd.clear();
        display(data, 
                "UNKNOWN CARD !!!");
        
        delay(4500);
      }



      // if(SPIFFS.exists(dataFile)) {

      //     Serial.println((String)"Found user with card ... " + data);
      //     // checkUser(fileNameString);
      //     Serial.println("Reseting pool...");

      //     // lcd.clear();
      //     display("Reseting pool...", 
      //             data);

      //     // SPIFFS.remove(dataFile);
      //     resetUser(dataFile);

      //     delay(3500);


      //   } else {

      //     Serial.println((String)"No account for card number: " + data);
      //     Serial.println("Creating user ... ");

      //     // lcd.clear();
      //     display("Creating user:  ", 
      //             data);
          
      //     File f = SPIFFS.open(dataFile, "w");   
      //       f.seek(0); 
      //       Serial.println("Writing to file... ");
      //       Serial.println(dataFile);
      //       String writeString = (String)"{\"card_number\":\"" + data + "\",\"user_name\":\"" + "User_" + data + "\",\"coffee_count\":" + 0 + ", \"coffee_pool\":" + 100 + "}";
      //       f.println(writeString);
      //       Serial.println(writeString);
      //     f.close();

      //     delay(3500);


      //   }


    } else {
      
      Serial.println("This is WHITE card. Entering input mode...");

      lcd.clear();
      display("Master Card     ", 
              "!!! detected !!!");

      delay(3000);

      while( ( ! mfrc522.PICC_IsNewCardPresent()) || ( ! mfrc522.PICC_ReadCardSerial())  ) {

          Serial.println("Input mode. Waiting for card ...");

          lcd.clear();
          display("Input Mode      ", 
                  "Waiting for card");

          delay(1000);

      }

      data = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);

      // while ( (data == masterCard ) && (mfrc522.PICC_IsNewCardPresent()) ) {
      if (data != masterCard ) {

        String dataFile = (String)"/" + data + ".txt";

        if(SPIFFS.exists(dataFile)) {

        Serial.println((String)"Found user with card ... " + data);
        // checkUser(fileNameString);
        Serial.println("Reseting pool...");

        // lcd.clear();
        display("Reseting pool...", 
                data);

        // SPIFFS.remove(dataFile);
        // resetUser(dataFile);
        
        addCreditToUser(dataFile, 50);  //  add 50 credits to users pool

        delay(2500);


      } else {

        Serial.println((String)"No account for card number: " + data);
        Serial.println("Creating user ... ");

        // lcd.clear();
        display("Creating user:  ", 
                data);
        
        File f = SPIFFS.open(dataFile, "w");   
          f.seek(0); 
          Serial.println("Writing to file... ");
          Serial.println(dataFile);
          String writeString = (String)"{\"card_number\":\"" + data + "\",\"user_name\":\"" + "User_" + data + "\",\"coffee_count\":" + 0 + ", \"coffee_pool\":" + 100 + "}";
          f.println(writeString);
          Serial.println(writeString);
        f.close();

        delay(3500);


      }


      } else {

        byte bufferATQA[10];
        byte bufferSize[10];

        lcd.clear();
        display("Master Card ON !", 
                "service mode    ");

        enterServiceMode();

        mfrc522.PICC_HaltA();

        bool cardStillHere = true;

        while(cardStillHere) {

          if (mfrc522.PICC_WakeupA(bufferATQA, bufferSize)) {


            mfrc522_fast_Reset();

            bool detect = mfrc522_fastDetect3();
            Serial.println((String)"fast detect is ... " + detect );

            // Serial.println((String)"wake up is ... " + mfrc522.PICC_WakeupA(bufferATQA, bufferSize) );


            bool cardRead = mfrc522.PICC_ReadCardSerial();

            if ( ! cardRead ) {
              Serial.println((String)"Statement is ... " + ! cardRead );
              cardStillHere =  false;

              lcd.clear();
              display("Master Card OFF", 
                      "EXIT SERVICE !");

              exitServiceMode();

            };

            data = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
            Serial.println((String)"present data is ... " + data );

            delay(1000);

            lcd.clear();
            display("Master Card ON !", 
                    "STILL SERVICE !!");


            mfrc522.PICC_HaltA();

          } else {

            cardStillHere =  false;

            lcd.clear();
            display("Master Card OFF", 
                    "EXIT input mode ");

            exitServiceMode();

            delay(2000);

            return;

          };



        }

      }

    }


  }

}