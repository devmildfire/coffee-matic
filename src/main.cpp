#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include <SPI.h>
#include <MFRC522.h>

#include <SoftwareSerial.h>
SoftwareSerial RFID(D5, D6); // RX and TX



#define RST_PIN         D3          // Configurable, see typical pin layout above
#define SS_PIN          D8        // Configurable, see typical pin layout above

#define TR_PIN          D4

const int transistor = 14;  // Assigning name to Trasistor 

AsyncWebServer server(80);


int readingsNumber = 0;
String lastReadCard = "NULL_CARD";
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 5000;

String cardsArray[] ={"e5365e64", "8a61b80"};
int cardsAmountsArray[] ={0, 0};

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance




// char* cardNumbers[] = {
//   "1234567890",
//   "1234567891",
//   "1234567892",
//   "02568746834",
//   "875289065",
//   "087263195",
//   "302076202",
//   "529710375",
//   "826501900",
//   "198610328",
//   "528562946",
//   "635384622",
//   "654829312"
// };

String data;

char ssid[] = "Lalala";       // your network SSID (name)
char password[] = "0892387639";  // your network key

//Add a SSL client
// WiFiClientSecure client;

long checkCoffeeDueTime;
int checkCoffeeDelay = 10000; // 60 x 1000 (1 minute)


LiquidCrystal_I2C lcd(0x27,16,2);


void setup() {

  pinMode(TR_PIN, OUTPUT);     // Assigning pin as output

  Serial.begin(115200);
  RFID.begin(115200);

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




  // // Set WiFi to station mode and disconnect from an AP if it was Previously
  // // connected
  // WiFi.mode(WIFI_STA);
  // WiFi.disconnect();
  // delay(100);

  // // Attempt to connect to Wifi network:
  // Serial.print("Connecting Wifi: ");
  // lcd.setCursor(0,0);
  // lcd.print("Connecting Wifi: ");

  // Serial.println(ssid);
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED) {
  //   Serial.print(".");
  //   delay(500);
  // }

  // Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // IPAddress ip = WiFi.localIP();
  // Serial.println(ip);

};

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

    coffeeCount += 1;
    coffeePool -= 1;
    root["coffee_count"] = coffeeCount;
    root["coffee_pool"] = coffeePool;

    digitalWrite(TR_PIN, HIGH);   // making pin high
    delay(1000);
    Serial.println("D4 pin LOW ");                 
    digitalWrite(TR_PIN, LOW);    // making pin low
    // delay(1000); 
    Serial.println("D4 pin HIGH ");   



    f.seek(0); 
    Serial.println("Writing to file... ");
    root.printTo(f);
    root.printTo(Serial);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(userName);
    lcd.setCursor(0,1);
    lcd.print((String)"sum:" + coffeeCount + " left:" + coffeePool );

    delay(2000);

    Serial.println("D4 pin HIGH ");  
    digitalWrite(TR_PIN, HIGH);   // making pin high
    delay(1000);
    Serial.println("D4 pin LOW ");                 
    digitalWrite(TR_PIN, LOW);    // making pin low
    // delay(1000); 

    Serial.println();
    f.close();

    return;
  } else {
    f.close();
    Serial.println((String)"user " + userName + " is blocked from drinking coffee :(");
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(userName);
    lcd.setCursor(0,1);
    lcd.print("POOL EMPTY !");

    return;
  }

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

// void randomCardLoop() {
//   int userIndex = random(13);
//   String currentCardNumber = cardNumbers[userIndex];

//   long now = millis();
//   if(now >= checkCoffeeDueTime) {

//     Serial.println("---------");
//     Serial.println((String)"card number: " + cardNumbers[userIndex]);

//     lcd.clear();
//     lcd.setCursor(0,0);
//     lcd.print((String)"Current card:   ");
//     lcd.setCursor(0,1);
//     lcd.print(currentCardNumber);

//     String fileNameString = (String)"/" + currentCardNumber + ".txt";

//     if(SPIFFS.exists(fileNameString)) {

//       Serial.println((String)"Found file ... " + fileNameString);
//       checkUser(fileNameString);
//     } else {
//       Serial.println((String)"No account for card number: " + currentCardNumber);

//       lcd.clear();
//       lcd.setCursor(0,0);
//       lcd.print(currentCardNumber);
//       lcd.setCursor(0,1);
//       lcd.print("UNKNOWN CARD !!!");
//     }


//     Serial.println("---------");
//     checkCoffeeDueTime = now + checkCoffeeDelay;
//   }
// }

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


void loop() {



  // digitalWrite(TR_PIN, HIGH);   // making pin high
  // delay(1000);               
  // digitalWrite(TR_PIN, LOW);    // making pin low
  // delay(1000);  

  // lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("waiting for card");
  lcd.setCursor(0,1);
  lcd.print("----------------");


  if (WiFi.status() != WL_CONNECTED) {
    scanForWiFi();
  }

  // randomCardLoop();



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
  lcd.clear();
  String dataFile = (String)"/" + data + ".txt";



  if (((millis() - lastDebounceTime) > debounceDelay ) || ( lastDebounceTime == 0 )) {
    readingsNumber += 1;
    lastDebounceTime = millis();
    // Serial.println(readingsNumber);
    Serial.println(data);
    

    if (data == cardsArray[0] ) {
      // cardsAmountsArray[0] += 1;
      Serial.println("This is WHITE card. Entering input mode...");

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Master Card");
      lcd.setCursor(0,1);
      lcd.print("!!! detected !!!");


      delay(3000);

      while( ( ! mfrc522.PICC_IsNewCardPresent()) || ( ! mfrc522.PICC_ReadCardSerial())  ) {

          Serial.println("Input mode. Waiting for card ...");

          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Inout Mode");
          lcd.setCursor(0,1);
          lcd.print("Waiting for card");

          delay(1000);

      }

      data = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);

      if (data == cardsArray[0] ) {

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Master Card");
        lcd.setCursor(0,1);
        lcd.print("exit input mode");

        delay(2000);

        return;
      }



      String dataFile = (String)"/" + data + ".txt";
      Serial.println((String)"Card presented ... " + data);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Card presented:");
      lcd.setCursor(0,1);
      lcd.print(data);

      delay(2000);
      

      if(SPIFFS.exists(dataFile)) {

        Serial.println((String)"Found user with card ... " + data);
        // checkUser(fileNameString);
        Serial.println("Reseting pool...");

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Reseting pool...");
        lcd.setCursor(0,1);
        lcd.print(data);

        // SPIFFS.remove(dataFile);
        resetUser(dataFile);

        delay(3500);


      } else {

        Serial.println((String)"No account for card number: " + data);
        Serial.println("Creating user ... ");

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Creating user:");
        lcd.setCursor(0,1);
        lcd.print(data);
        
        File f = SPIFFS.open(dataFile, "w");   
          f.seek(0); 
          Serial.println("Writing to file... ");
          Serial.println(dataFile);
          String writeString = (String)"{\"card_number\":\"" + data + "\",\"user_name\":\"" + "User_" + data + "\",\"coffee_count\":" + 0 + ", \"coffee_pool\":" + 100 + "}";
          f.println(writeString);
          Serial.println(writeString);
        f.close();

        delay(3500);

        // lcd.clear();
        // lcd.setCursor(0,0);
        // lcd.print(currentCardNumber);
        // lcd.setCursor(0,1);
        // lcd.print("UNKNOWN CARD !!!");
      }

      Serial.println("Exiting Input mode ...");
      
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Exiting Input");
      lcd.setCursor(0,1);
      lcd.print("Mode");

      delay(2500);

      return;

    }

    // if (data == cardsArray[1] ) {
    //   cardsAmountsArray[1] += 1;
    //   Serial.println("This is BLUE card. It's Amount of checks is..."); 
    //   Serial.println(cardsAmountsArray[1]);
    // }

    if (SPIFFS.exists(dataFile)) {

      checkUser(dataFile);
      delay(4500);
      
      // Serial.println("This is KNOWN card. It's Amount of checks is..."); 
      // Serial.println(cardsAmountsArray[1]);
    } else {
      Serial.println((String)"No account for card number: " + data);

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(data);
      lcd.setCursor(0,1);
      lcd.print("UNKNOWN CARD !!!");
      
      delay(4500);
    }

  }



}