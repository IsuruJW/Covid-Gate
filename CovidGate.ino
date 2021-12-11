/* Coivid Gate
   A project by ISURU Jayashankha
   02 October 2021
*/

//Include the required libraries
#include <HCSR04.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
#include <Keypad.h>
#include <Adafruit_MLX90614.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

// Define pins for required components
#define next A3
#define s_liquid A4
#define gate1 9
#define gate2 10
#define LDR_1 4
#define LDR_2 5
#define IFRed2 12
#define LED1 A0
#define LED2 A1
#define RedLight 13
#define emergency_pin 3
#define config_keypad 6
#define config_bluetooth 7
#define config_keypad_led 23
#define config_bluetooth_led 25
#define triggerPin 27
#define echoPin 29
#define pump 31
#define mp3RX 33    // to mp3 module's RX
#define mp3TX 35    // to mp3 module's TX
#define laser1 A13
#define laser2 A14

// Define global variables
bool LDR1 = LOW;
bool LDR2 = LOW;
bool call_emergency = false;
bool instate = HIGH;
int emergency_state = 0;
int counter = 0;
int ldrDelay = 700;       // delay following the ldr input
int sanitizeTime = 500;  // Time of the sanitizer
int maxCount = 3;         // variable to store maximum people allowed
float maxTemp = 30;       // variable to store maximum TEMP allowed
int isSanitized = 0;
long ledtime = 200;
int mtr_delay = 900;      // gate motor on of time

// Keypad configuration
const byte ROWS = 4;
const byte COLS = 4;
byte rowPins[ROWS] = {37, 39, 41, 43}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {45, 47, 49, 51}; //connect to the column pinouts of the keypad
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'.', '0', '#', 'D'}
};
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//Defining various compontnts with corrosponding libraries
SoftwareSerial softwareSerial(mp3TX, mp3RX);
UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin);
LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd2(0x26, 16, 2);
LiquidCrystal_I2C lcd3(0x23, 16, 2);
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
DFRobotDFPlayerMini player;

void setup() {
  // Setting up the pin Modes
  pinMode(LDR_1, INPUT_PULLUP);
  pinMode(LDR_2, INPUT_PULLUP);
  pinMode(IFRed2, INPUT_PULLUP);
  pinMode(next, INPUT_PULLUP);
  pinMode(config_keypad, INPUT_PULLUP);
  pinMode(config_bluetooth, INPUT_PULLUP);
  pinMode(s_liquid, INPUT);
  pinMode(config_bluetooth_led, OUTPUT);
  pinMode(config_keypad_led, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(gate1, OUTPUT);
  pinMode(gate2, OUTPUT);
  pinMode(laser1, OUTPUT);
  pinMode(laser2, OUTPUT);
  pinMode(RedLight, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(emergency_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(emergency_pin), flag, CHANGE);

  //Initialize various settings
  Wire.begin();
  Serial.begin(9600);
  mlx.begin();
  softwareSerial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd2.init();
  lcd2.backlight();
  lcd3.init();
  lcd3.backlight();
  maxCount = EEPROM.read(1);
  maxTemp = EEPROM.read(0);

  //Initiaize the lasers and set the variables
  digitalWrite(laser1, HIGH);
  digitalWrite(laser2, HIGH);
  delay(200);
  LDR1 = digitalRead(LDR_1);
  LDR2 = digitalRead(LDR_2);

  //Begin the DFplayer and play the intro file
  if (player.begin(softwareSerial)) {
    Serial.println(" connection to the player - OK");
    player.volume(20);
  } else {
    Serial.println("Connecting to DFPlayer Mini failed!");
  }
  player.play(1);

  //Prints the starting message
  lcd.print("Covid Safety Gate");
  lcd.setCursor(0, 2);
  lcd.print("Project by Duranka");
  delay(4000);

  //Display the configuration menu
  lcd.clear();
  lcd2.setCursor(0, 1);
  lcd2.print("Now : ");
  lcd2.print(counter);
  lcd2.setCursor(0, 0);
  lcd2.print("Max Count: ");
  lcd2.print(maxCount);
  lcd.setCursor(0, 0);
  lcd.print("Run with prev data");
  lcd.setCursor(0, 3);
  lcd.print("Press btn to config");
  player.play(2);
  delay(5000);
  int j = 10;
  long t = millis();
  while (true) {
    if (millis() - t > 1000) {
      lcd.setCursor(10, 2);
      lcd.print("  ");
      lcd.setCursor(10, 2);
      lcd.print(j);
      t = millis();
      j--;
    }
    if (j < 0) {
      lcd.clear();
      player.play(19);
      delay(3500);
      break;
    }
    if (digitalRead(next) == LOW) {
      delay(500);
      player.play(5);
      delay(4000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set the config mode");
      lcd.setCursor(0, 1);
      lcd.print("and press config btn");
      while (digitalRead(next) == HIGH) {
        if (digitalRead(config_bluetooth) == LOW) {
          digitalWrite(config_bluetooth_led, HIGH);
          digitalWrite(config_keypad_led, LOW);
        }
        if (digitalRead(config_keypad) == LOW) {
          digitalWrite(config_keypad_led, HIGH);
          digitalWrite(config_bluetooth_led, LOW);
        }

      }
      lcd.clear();
      configuration();
      break;
    }
  }
  lcd2.clear();
  lcd2.setCursor(0, 0);
  lcd2.print("Now : ");
  lcd2.print(counter);
  lcd2.setCursor(0, 1);
  lcd2.print("Max Count : ");
  lcd2.print(maxCount);
  lcd.setCursor(2, 0);
  lcd.print("Covid Safety Gate");
  delay(200);
}

void loop() {
  // Checks for an person entering from outside the room
  if (digitalRead(LDR_1) == instate && digitalRead(LDR_2) != instate && LDR1 != instate && LDR2 != instate) {
    lcd3.setCursor(0, 0);
    lcd3.print("Covid SafetyGate");

    while (digitalRead(LDR_1) == instate) {}
    delay(ldrDelay);

    // Restrict the entry if the maximum person limit is reached
    if (counter >= maxCount) {
      player.play(17);
      digitalWrite(RedLight, HIGH);
      lcd.setCursor(0, 2);
      lcd.print("Max limit Reached");
      lcd.setCursor(0, 3);
      lcd.print("Please wait outside");
      while (digitalRead(LDR_1) != instate) {}
      delay(ldrDelay);
      while (digitalRead(LDR_1) == instate) {}
      delay(ldrDelay);
      digitalWrite(RedLight, LOW);
      lcd.setCursor(0, 2);
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      LDR1 = !instate;
    }

    //Temperature check and sanitizing
    else {
      player.play(10);
      lcd.setCursor(0, 2);
      lcd.print("Please Check your");
      lcd.setCursor(0, 3);
      lcd.print("Temperature");
      delay(2500);
      float temp = checkTemp();
      lcd3.clear();
      lcd3.setCursor(0, 0);
      lcd3.print("Your Temp : ");
      lcd3.print(temp);

      //Proceed if temperature is below accepted limit
      if (temp < maxTemp) {
        player.play(11);
        delay(2500);
        player.play(12);
        lcd.setCursor(0, 2);
        lcd.print("Please Sanitize     ");
        lcd.setCursor(0, 3);
        lcd.print("Your Hands          ");
        delay(2000);
        sanitize();     //Begin sanitizing
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        openGate();     //Opens the gate
        lcd3.clear();
        lcd3.setCursor(0, 0);
        lcd3.print("Covid SafetyGate");
        LDR1 = instate;
      }

      //Display warning and restrict entry if temperature exceeds limit
      else {
        player.play(16);
        digitalWrite(RedLight, HIGH);
        lcd.setCursor(0, 2);
        lcd.print("Temp exceed Limit");
        lcd.setCursor(0, 3);
        lcd.print("Not allowed to Enter");
        Serial.print("High_Temp");
        delay(200);
        while (digitalRead(LDR_1) != instate) {}
        delay(ldrDelay);
        while (digitalRead(LDR_1) == instate) {}
        delay(ldrDelay);
        lcd.setCursor(0, 1);
        lcd.print("                    ");
        digitalWrite(RedLight, LOW);
        lcd.setCursor(0, 2);
        lcd.print("                    ");
        lcd.setCursor(0, 3);
        lcd.print("                    ");
        LDR1 = !instate;
      }
    }
  }

  // Close the gate after a person entered the building
  if (digitalRead(LDR_1) != instate && digitalRead(LDR_2) == instate && LDR1 == instate && LDR2 != instate) {

    while (digitalRead(LDR_2) == instate) {}
    delay(ldrDelay);
    closeGate();
    lcd.setCursor(0, 1);
    lcd.print("                    ");
    counter++;
    LDR1 = !instate;
    LDR2 = !instate;
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Now : ");
    lcd2.print(counter);
    lcd2.setCursor(0, 1);
    lcd2.print("Max Count : ");
    lcd2.print(maxCount);
    lcd.setCursor(0, 0);
    lcd.print("Number of people: ");
    lcd.print(counter);
  }

  //Check if a person tries to leave the building
  if (digitalRead(LDR_1) != instate && digitalRead(LDR_2) == instate && LDR1 != instate && LDR2 != instate) {
    while (digitalRead(LDR_2) == instate) {}
    delay(ldrDelay);
    openGate();
    player.play(15);
    lcd.setCursor(0, 2);

    //Display the THANK YOU message
    lcd.print("     Thank You!     ");
    lcd.setCursor(0, 3);
    lcd.print("     Come Again     ");
    int k = 0;
    while (digitalRead(LDR_1) != instate) {
      if (digitalRead(IFRed2) == LOW && k == 0) {
        digitalWrite(pump, HIGH);
        delay(sanitizeTime);
        digitalWrite(pump, LOW);
        k = 1;
      }
    }
    LDR2 = instate;
  }

  // Check and close the gate after a person successfully leave the room
  if (digitalRead(LDR_1) == instate && digitalRead(LDR_2) != instate  && LDR1 != instate && LDR2 == instate) {
    delay(ldrDelay);
    closeGate();
    lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print("                    ");
    if (counter > 0) {
      counter--;
    }
    lcd2.clear();
    lcd2.setCursor(0, 0);
    lcd2.print("Now : ");
    lcd2.print(counter);
    lcd2.setCursor(0, 1);
    lcd2.print("Max Count : ");
    lcd2.print(maxCount);
    LDR1 = !instate;
    LDR2 = !instate;
    lcd.setCursor(0, 0);
    lcd.print("Number of people: ");
    lcd.print(counter);
  }

  //Keeps the gate open if the emergency button is pressed
  if (call_emergency == true) {
    emergency();
    call_emergency = false;
  }
}

void openGate() {   // A function to open the gate
  if (emergency_state == 0) {
    player.play(13);
    digitalWrite(gate1, HIGH);
    digitalWrite(gate2, LOW);
    delay(mtr_delay);
    digitalWrite(gate1, LOW);
    digitalWrite(gate2, LOW);
    delay(2000);
  }
}
void closeGate() {  // A function to close the gate
  if (emergency_state == 0) {
    player.play(14);
    digitalWrite(gate2, HIGH);
    digitalWrite(gate1, LOW);
    delay(mtr_delay);
    digitalWrite(gate2, LOW);
    digitalWrite(gate1, LOW);
  }
}
float checkTemp() {  // A function to check the temperature
  long t = millis();

  // Blinks the LED and tells the person to keep the requied distance to the temp sensor
  while (true) {
    if (millis() - t < ledtime) {
      digitalWrite(LED1, HIGH);
    }
    else {
      digitalWrite(LED1, LOW);
    }
    if (millis() - t > (ledtime * 2)) {
      t = millis();
    }

    if (distanceSensor.measureDistanceCm() > 5) {
      lcd3.setCursor(0, 1);
      lcd3.print("Too far         ");
    }
    else if (distanceSensor.measureDistanceCm() < 2) {
      lcd3.setCursor(0, 1);
      lcd3.print("Too close       ");
    }
    else {
      lcd3.setCursor(0, 1);
      lcd3.print("                ");
      break;
    }
  }

  // Read the temperature
  float temp = mlx.readObjectTempC();
  digitalWrite(LED1, LOW);
  delay(200);
  return temp;
}

void sanitize() { // A function for the sanitizer
  long t = millis();
  while (digitalRead(IFRed2) == HIGH) {
    if (millis() - t < ledtime) {
      digitalWrite(LED2, HIGH);
    }
    else {
      digitalWrite(LED2, LOW);
    }
    if (millis() - t > (2 * ledtime)) {
      t = millis();
    }
  }
  digitalWrite(pump, HIGH);
  delay(sanitizeTime);
  digitalWrite(pump, LOW);
  digitalWrite(LED2, LOW);

  // Check for the sanitizer liquid level and send a message if the level is low
  if (digitalRead(s_liquid) == HIGH) {
    Serial.println("Please refill the sanitize liquid!");
    lcd.setCursor(0, 2);
    lcd.print("Please fill Liquid");
  }
  else {
    lcd.setCursor(0, 2);
    lcd.print("                  ");
  }

}

void emergency() {    // A function to set the emergency state
  if (emergency_state == 0) {
    openGate();
    emergency_state = 1;
  }
  else {
    emergency_state = 0;
    closeGate();

  }
  delay(1000);
}
void configuration() {  // A function for the configuration settings
  // Bluetooth configuration mode
  if (digitalRead(config_bluetooth) == LOW) {
    player.play(4);
    String text;
    digitalWrite(config_bluetooth_led, HIGH);
    lcd.setCursor(0, 0);
    lcd.print("Bluetooth Mode");
    delay(4000);
    player.play(6);

    // Ask for the maximum allowed temperature
    Serial.println("Enter the maximum Temp: ");
    while (true) {
      if (Serial.available() > 0) {
        text = Serial.readString();
        break;
      }
    }
    maxTemp = text.toFloat();
    lcd.setCursor(0, 1);
    lcd.print("Max Temp : ");
    lcd.print(maxTemp);
    delay(300);

    //Ask for the maximum person limit
    Serial.println("Enter the Maximum Count: ");
    player.play(7);
    while (true) {
      if (Serial.available() > 0) {
        text = Serial.readString();
        break;
      }
    }
    maxCount = text.toInt();
    lcd.setCursor(0, 2);
    lcd.print("Max Count: ");
    lcd.print(maxCount);
    delay(300);

    //Ask for the current persons count
    Serial.println("Enter current count: ");
    player.play(8);
    while (true) {
      if (Serial.available() > 0) {
        text = Serial.readString();
        break;
      }
    }
    counter = text.toInt();
    lcd.setCursor(0, 3);
    lcd.print("Current count : ");
    lcd.print(counter);
    lcd.setCursor(0, 0);
    lcd.print("Config Complete");
    delay(1000);
    digitalWrite(config_bluetooth_led, LOW);
    lcd.clear();
  }

  // Keypad configuration mode
  else if (digitalRead(config_keypad) == LOW) {
    player.play(3);
    String text;
    digitalWrite(config_keypad_led, HIGH);
    char key = keypad.getKey();
    lcd.setCursor(0, 0);
    lcd.print("Keypad config Mode");
    lcd.setCursor(0, 1);
    delay(4000);

    // Ask for the max allowed temperature
    lcd.print("Max Temp : ");
    player.play(6);

    //Read the keypad input until button A is pressed
    while (key != 'A') {
      if (key) {
        lcd.print(key);
        text += key;
      }
      key = keypad.getKey();
    }
    maxTemp = text.toFloat();
    key = ' ';

    // Ask for the maximum person limit
    text = "";
    lcd.setCursor(0, 2);
    lcd.print("Max Count : ");
    player.play(7);
    //Read the keypad input until button A is pressed
    while (key != 'A') {
      if (key) {
        lcd.print(key);
        text += key;
      }
      key = keypad.getKey();
    }
    maxCount = text.toInt();
    key = ' ';

    // Ask for the current persons count
    text = "";
    lcd.setCursor(0, 3);
    lcd.print("Current Count : ");
    player.play(8);
    //Read the keypad input until button A is pressed
    while (key != 'A') {
      if (key) {
        lcd.print(key);
        text += key;
      }
      key = keypad.getKey();
    }
    counter = text.toInt();
    key = ' ';
    digitalWrite(config_keypad_led, LOW);
  }
  player.play(9);

  // Store the settings in the permanent memory
  EEPROM.update(0, maxTemp);
  EEPROM.update(1, maxCount);
  delay(200);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Config Successful!");
  lcd.setCursor(0, 0);
  lcd.print("     Thank You!     ");
  delay(2000);
  lcd.clear();
}

void flag() { // A function to store the emergency value if pressed
  call_emergency = true;
}
