#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>
#include <SPI.h>
#include <SD.h>


Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
Adafruit_MCP23017 mcp;

// Button Constants
#define UP_BUTTON 1
#define DOWN_BUTTON 2
#define LEFT_BUTTON 4
#define RIGHT_BUTTON 8
#define SELECT_BUTTON 16

// LCD Constants
#define LCD_COLS 16
#define LCD_ROWS 2

// Option Constants
enum Option {
  P_TARGET_TEMP,
  F_TARGET_TEMP,
  F_HEAT_TIME,
  HEIGHT_CALIBRATION,
  P_COOLING_HEIGHT,
  F_COOLING_HEIGHT,
  COOL_TIME,
  COOL_TARGET_TEMP,
  P_PRESSURE_TARGET,
  F_PRESSURE_TARGET,
  PRESSURE_RANGE,
  PRESSURE_CALIBRATION,
  NUM_OPTIONS
};

// Heater Zones
enum Zone {
  PUNCH_ZONE,
  DIE_ZONE
};

// Heating States
enum HeatingState {
  HEATING_100,
  HEATING_75,
  HEATING_50
};


// Program States
enum ProgramState {
  HOME_SCREEN,
  PROGRAM_SELECTION,
  PLASTIC_PROGRAM,
  FABRIC_PROGRAM,
  PLASTIC_PRESSING_PROGRAM,
  FABRIC_PRESSING_PROGRAM,
  SETTINGS_SCREEN,
  CALIBRATION,
  COOLING
};

enum SelectablePrograms {
  PLASTIC,
  FABRIC,
  SETTINGS,
  CALIB,
  COOL,
  NUM_PROGS
};

ProgramState currentState = HOME_SCREEN;
HeatingState p_heatingState = HEATING_100;
HeatingState f_heatingState = HEATING_100;

// // LCD Shield Objects
// Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
// Adafruit_MCP23017 mcp;

// Sensor Pins
const int punchSensorPin = A9;
const int dieSensorPin = A8;
const int heightSensorPin = A3;
const int pressurePin = A7;


// SD card setup
const int sdCardPin = 10;
const String settingsFilePath = "/settings.txt";

const String fabricIDListPath = "/Fabric_Id.txt";
const String plasticIDListPath = "/Plastic_Id.txt";

String filename = "test.csv";

// Heating and Cooling Parameters
int pTargetTemperature = 70;
int fTargetTemperature = 85;
int fHeatTime = 5;
int heightCalibration = 0;
int pCoolHeight = 130;
int fCoolHeight = 30;
int CoolTime = 5;
int coolTargetTemp = 15;
int pPressureTarget = 70;
int fPressureTarget = 30;
int pressureRange = 5;
int PressureTarget = 30;
int CoolHeight = 130;
int CoolTargetReached = 0;
int PressureCalibration = 70;  // this is units of 1/10 of 1 inch squared


bool DataLogging = 0;           // a boolean to say whether of not data should be logged during this time.
bool HeatBuzzerSounded = 0;     // Boolean to say if the heating buzzer has sounded yet
bool CoolingStarted = 0;        // Boolean to say if the cooling begun yet
bool PressingStarted = 0;       // Boolean to say if the pressing has begun yet
bool CalibStarted = 0;          //Boolean to know if Calib program has started yet


// Function Control Pins
const int heaterPunchPin = 36;
const int heaterDiePin = 38;
const int coolingPumpPin = 39;
const int pressPumpPin = 40;
const int buzzerPin = 37;

// Timing Parameters
const unsigned long heatingDelay100 = 500;       // 0.5 seconds
const unsigned long heatingDelay75 = 3000;       // 3 seconds
const unsigned long heatingDelay50 = 5000;       // 5 seconds
const unsigned long coolingDelay = 1000;         // 1 second
const unsigned long buzzerDuration = 3000;       // 3 seconds
const unsigned long ButtonPressDelay = 200;      // 0.2 second
const unsigned long DataLoggingInterval = 2000;  // 2 seconds

//Specific times
unsigned long StartHeatTime = 0;  //first time the heaters turned on during a program

// Make a custom Icons
byte homeIcon1[8] = {
  B00001,
  B00011,
  B00111,
  B01111,
  B11111,
  B11110,
  B11110,
  B11110
};

byte homeIcon2[8] = {
  B10011,
  B11011,
  B11111,
  B11111,
  B11111,
  B01111,
  B01111,
  B01111
};

byte UpDownIcon[8] = {
  B00100,
  B01010,
  B10001,
  B00000,
  B00000,
  B10001,
  B01010,
  B00100
};

byte HeatIcon1[8] = {
  B00100,
  B01110,
  B11011,
  B10001,
  B00100,
  B01110,
  B11011,
  B10001
};

byte PressIcon1[8] = {
  B00011,
  B00011,
  B01111,
  B00111,
  B00011,
  B00001,
  B00000,
  B11111
};
byte PressIcon2[8] = {
  B11000,
  B11000,
  B11110,
  B11100,
  B11000,
  B10000,
  B00000,
  B11111
};

byte CoolIcon1[8] = {
  B00000,
  B00100,
  B00110,
  B11111,
  B11111,
  B00110,
  B00100,
  B00000
};
byte CoolIcon2[8] = {
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
  B10101,
  B10101
};

byte CalibIcon[8] = {
  B00000,
  B00110,
  B11111,
  B00110,
  B00000,
  B01100,
  B11111,
  B01100

};


// Timing Variables
unsigned long lastHeatingTime = 0;
unsigned long lastCoolingTime = 0;
unsigned long StartCoolTime = 0;
unsigned long buzzerStartTime = 0;
unsigned long holdHeatStartTime = 0;
unsigned long lastDataLogTime = 0;


//setup and loop
void setup() {
  // Initialize LCD Shield
  lcd.begin(LCD_COLS, LCD_ROWS);
  // lcd.setBacklight(Adafruit_RGBLCDShield::WHITE);

  // Initialize SD Card
  SD.begin(sdCardPin);

  // Initialize Pins
  pinMode(heaterPunchPin, OUTPUT);
  pinMode(heaterDiePin, OUTPUT);
  pinMode(coolingPumpPin, OUTPUT);
  pinMode(pressPumpPin, OUTPUT);

  // Load settings from SD Card
  loadSettings();

  lcd.createChar(0, homeIcon1);   //Left Home Icon
  lcd.createChar(1, homeIcon2);   // Right Home Icon
  lcd.createChar(2, UpDownIcon);  // Up& Down Symbol
  lcd.createChar(3, HeatIcon1);   // Left Heat Icon
  lcd.createChar(4, CalibIcon);   //Calibration Icon
  lcd.createChar(5, PressIcon1);  //Right Press Icon
  lcd.createChar(6, PressIcon2);  //Left Press Icon
  lcd.createChar(7, CoolIcon1);   //Cool Icon


  // Display Home Screen
  displayHomeScreen();
}

void loop() {
  if (currentState == HOME_SCREEN) {
    handleHomeScreen();
  } else if (currentState == PROGRAM_SELECTION) {
    handleProgramSelection();
  } else if (currentState == PLASTIC_PROGRAM) {
    handlePlasticProgram();
  } else if (currentState == FABRIC_PROGRAM) {
    handleFabricProgram();
  } else if (currentState == PLASTIC_PRESSING_PROGRAM) {
    handlePlasticPressingProgram();
  } else if (currentState == FABRIC_PRESSING_PROGRAM) {
    handleFabricPressingProgram();
  } else if (currentState == SETTINGS_SCREEN) {
    handleSettingsScreen();
  } else if (currentState == CALIBRATION) {
    handleCalibration();
  } else if (currentState == COOLING) {
    handleCooling();
  } 

  //This section is for collecting data points to the SD card during any given run
  // The data should only be collected ever 2 seconds
  if (DataLogging == 1 && (millis() - lastDataLogTime) >= DataLoggingInterval) {
    // Collect sensor and binary data
    lastDataLogTime = millis();
    String data = "";
    data += String(millis()) + ",";
    data += String(readTemperature(PUNCH_ZONE)) + ",";
    data += String(readTemperature(DIE_ZONE)) + ",";
    data += String(readPressure()) + ",";
    data += String(readHeight()) + ",";
    data += String(digitalRead(heaterPunchPin)) + ",";
    data += String(digitalRead(heaterDiePin)) + ",";
    data += String(digitalRead(coolingPumpPin)) + ",";
    data += String(digitalRead(pressPumpPin)) + ",";

    // Log the data to the SD card
    logData(filename, data);
  }
}


//Home screen
void handleHomeScreen() {
  int buttons = readButtons();
  if (buttons == UP_BUTTON) {
    currentState = PROGRAM_SELECTION;
    delay(ButtonPressDelay);
  }

  if (buttons == SELECT_BUTTON) {
    currentState = SETTINGS_SCREEN;
    delay(ButtonPressDelay);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Setting ");
    lcd.write((uint8_t)2);
    lcd.print(" Scroll");
    lcd.setCursor(0, 1);
    lcd.print("<> Adjust");
    delay(2000);
  }
}

//Adjust Settings
void handleSettingsScreen() {
  static Option selectedOption = P_TARGET_TEMP;  // Currently selected option

  displaySettingsPage(selectedOption);

  while (true) {
    int button = readButtons();  // Read the button state as an integer

    if (button == UP_BUTTON) {
      selectedOption = static_cast<Option>((selectedOption - 1) % NUM_OPTIONS);
      displaySettingsPage(selectedOption);
    } else if (button == DOWN_BUTTON) {
      selectedOption = static_cast<Option>((selectedOption + NUM_OPTIONS + 1) % NUM_OPTIONS);
      displaySettingsPage(selectedOption);
    } else if (button == LEFT_BUTTON) {
      adjustOptionValue(selectedOption, false);  // Decrease the value
      displayAdjustingMode(selectedOption);
    } else if (button == RIGHT_BUTTON) {
      adjustOptionValue(selectedOption, true);  // Increase the value
      displayAdjustingMode(selectedOption);
    } else if (button == SELECT_BUTTON) {
      saveSettings();  // Save the updated settings to the SD card
      delay(ButtonPressDelay);
      cancelProgram();
      break;
    }
  }
}

//Select a program screen
void handleProgramSelection() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Program:");

  static SelectablePrograms programSelection = PLASTIC;

  displayProgramSelection(programSelection);

  while (true) {

    int button = readButtons();


    lcd.setCursor(0, 1);

    if (button == UP_BUTTON) {
      
      programSelection = static_cast<SelectablePrograms>((programSelection + NUM_PROGS - 1) % NUM_PROGS);

      // if (programSelection == PROGRAM_SELECTION || programSelection == HOME_SCREEN) {
      //   programSelection = static_cast<SelectablePrograms>((programSelection + NUM_PROGS - 1) % NUM_PROGS);
      // }
      delay(ButtonPressDelay);
      displayProgramSelection(programSelection);
    } else if (button == DOWN_BUTTON) {

      delay(ButtonPressDelay);
      programSelection = static_cast<SelectablePrograms>((programSelection + NUM_PROGS + 1) % NUM_PROGS);
      // Need to remove the option for Program Selection because we are already on this page
      // if (programSelection == PROGRAM_SELECTION || programSelection == HOME_SCREEN) {
      //   programSelection = static_cast<SelectablePrograms>((programSelection + NUM_PROGS + 1) % NUM_PROGS);
      // }
      displayProgramSelection(programSelection);
    } else if (button == LEFT_BUTTON){
      delay(ButtonPressDelay);
      cancelProgram();

    } else if (button == SELECT_BUTTON) {
    delay(ButtonPressDelay);

    switch (programSelection){
      case PLASTIC:
        currentState = PLASTIC_PROGRAM;
        break;
      case FABRIC:
        currentState = FABRIC_PROGRAM;
        break;
      case SETTINGS:
        currentState = SETTINGS_SCREEN;
        break;
      case CALIB:
        currentState = CALIBRATION;
        break;
      case COOL:
        currentState = COOLING;
        break;
    }
    break;
    }
  }
}

//Plastic Program
void handlePlasticProgram() {
  // Check if both zones reached target temperature
  if (readTemperature(PUNCH_ZONE) >= pTargetTemperature && readTemperature(DIE_ZONE) >= pTargetTemperature) {
    // Turn off heaters
    digitalWrite(heaterPunchPin, LOW);
    digitalWrite(heaterDiePin, LOW);

    //If the Buzzer has not sounded set buzzer start time and switch heat buzzer sounded bool
    if (HeatBuzzerSounded == 0) {
      //Activate Buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();
      //Record Heat Buzzer Sounded
      HeatBuzzerSounded = 1;
    }
  }

  // Read the buttons
  int buttons = readButtons();

  // Wait for buzzer duration
  if (HeatBuzzerSounded == 1) {

    if (millis() - buzzerStartTime >= buzzerDuration) {
      // Turn off buzzer
      digitalWrite(buzzerPin, LOW);
    }

    if (buttons = SELECT_BUTTON) {
      // Turn off buzzer
      digitalWrite(buzzerPin, LOW);

      // Turn off heaters
      digitalWrite(heaterPunchPin, LOW);
      digitalWrite(heaterDiePin, LOW);

      // Proceed to Pressing program
      currentState = PLASTIC_PRESSING_PROGRAM;

      // Set the target pressure and cool height
      PressureTarget = pPressureTarget;
      CoolHeight = pCoolHeight;
      displayProgramStatus(2);
      return;
    }
  }



  // Check if punch zone needs heating
  if (readTemperature(PUNCH_ZONE) < pTargetTemperature) {
    controlHeater(PUNCH_ZONE);
  }

  // Check if die zone needs heating
  if (readTemperature(DIE_ZONE) < pTargetTemperature) {
    controlHeater(DIE_ZONE);
  }

  // Display Plastic Program status
  displayProgramStatus(1);

  // Check if left button is pressed to cancel the program

  // Scroll through the options
  if (buttons == LEFT_BUTTON) {
    delay(ButtonPressDelay);
    cancelProgram();
    return;
  }

  // EXTRA CODE FOR EASY TESTING REMOVE AFTER TESTING
  //###################
  if (buttons == SELECT_BUTTON) {
    delay(ButtonPressDelay);
    // Turn off buzzer
    digitalWrite(buzzerPin, LOW);

    // Proceed to Pressing program
    currentState = PLASTIC_PRESSING_PROGRAM;

    // Set the target pressure and cool height
    PressureTarget = pPressureTarget;
    CoolHeight = pCoolHeight;
    displayProgramStatus(2);
    return;
  }
  //######################
}

//Fabric Program
void handleFabricProgram() {
  // Check if both zones reached target temperature
  if (readTemperature(PUNCH_ZONE) >= fTargetTemperature && readTemperature(DIE_ZONE) >= fTargetTemperature) {
    // Check if hold heat time has passed
    if (millis() - holdHeatStartTime >= (fHeatTime * 60000)) {
      // Activate buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();
    }

    // Wait for buzzer duration
    if (millis() - buzzerStartTime >= buzzerDuration) {
      // Turn off buzzer
      digitalWrite(buzzerPin, LOW);

      // Proceed to Pressing program
      currentState = FABRIC_PRESSING_PROGRAM;
      displayProgramStatus(5);
      return;
    }
  }

  // Check if punch zone needs heating
  if (readTemperature(PUNCH_ZONE) < fTargetTemperature) {
    controlHeater(PUNCH_ZONE);
  }

  // Check if die zone needs heating
  if (readTemperature(DIE_ZONE) < fTargetTemperature) {
    controlHeater(DIE_ZONE);
  }

  // Display Fabric Program status
  displayProgramStatus(5);

  // Check if left button is pressed to cancel the program

  // Read the buttons
  int buttons = readButtons();

  // Scroll through the options
  if (buttons == LEFT_BUTTON) {
    cancelProgram();
    delay(ButtonPressDelay);
    return;
  }
}

//Plastic Pressing
void handlePlasticPressingProgram() {


  //Wait to start pressing until operator has pressed select or cancel
  // Read the buttons
  int buttons = readButtons();

  // Left cancels and ends program
  if (buttons == LEFT_BUTTON) {
    cancelProgram();
    delay(ButtonPressDelay);
    return;
  }

  // Select Will begin Pressing if it hasn't already been started
  if (buttons == SELECT_BUTTON && PressingStarted != 1) {
    delay(ButtonPressDelay);
    PressingStarted = 1;
  }

  // Select Will advance from Pressing to Cooling
  if (buttons == SELECT_BUTTON && PressingStarted == 1) {
    delay(ButtonPressDelay);
    //turn off heaters
    digitalWrite(heaterPunchPin, LOW);
    digitalWrite(heaterDiePin, LOW);

    // Turn on cooling pump
    digitalWrite(coolingPumpPin, HIGH);

    //Signal Cooling has started
    CoolingStarted = 1;
  }

  //Down will begin cooling but will not start pressing without pressign but pressing can be selected later to start
  if (buttons == DOWN_BUTTON) {
    delay(ButtonPressDelay);
    //turn off heaters
    digitalWrite(heaterPunchPin, LOW);
    digitalWrite(heaterDiePin, LOW);

    // Turn on cooling pump
    digitalWrite(coolingPumpPin, HIGH);

    //Signal Cooling has started
    CoolingStarted = 1;
  }

  // Pressing
  if (PressingStarted) {
    // Check if pressure is within the target range
    if (readPressure() >= (pPressureTarget + pressureRange)) {
      // Turn off press
      digitalWrite(pressPumpPin, LOW);

    } else if (readPressure() <= (PressureTarget - pressureRange)) {
      // Turn on press
      digitalWrite(pressPumpPin, HIGH);
    }

    // Check if height has reached the target cool height
    if (readHeight() <= pCoolHeight && CoolingStarted == 0) {
      //turn off heaters
      digitalWrite(heaterPunchPin, LOW);
      digitalWrite(heaterDiePin, LOW);

      // Turn on cooling pump
      digitalWrite(coolingPumpPin, HIGH);

      //Signal Cooling has started
      CoolingStarted = 1;
    }
  }


  // Check if cooling is complete
  if (readTemperature(DIE_ZONE) <= coolTargetTemp && readTemperature(PUNCH_ZONE) <= coolTargetTemp) {
    if (CoolTargetReached == 0) {
      StartCoolTime = millis();
      CoolTargetReached = 1;
    }
    if (CoolTargetReached && millis() - StartCoolTime >= (CoolTime * 60000)) {
      // Turn off cooling pump
      digitalWrite(coolingPumpPin, LOW);

      // Activate buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();

      // Return to home screen after buzzer duration
      if (millis() - buzzerStartTime >= buzzerDuration) {
        CoolTargetReached = 0;
        digitalWrite(buzzerPin, LOW);
        delay(ButtonPressDelay);
        cancelProgram();
        return;
      }
    }
  }


  //Keep the zone at temperature until the cooling starts
  if (!CoolingStarted) {
    // Check if punch zone needs heating
    if (readTemperature(PUNCH_ZONE) < pTargetTemperature) {
      controlHeater(PUNCH_ZONE);
    }

    // Check if die zone needs heating
    if (readTemperature(DIE_ZONE) < pTargetTemperature) {
      controlHeater(DIE_ZONE);
    }
  }

  //Select Appropriate Display
  if (PressingStarted || CoolingStarted) {
    if (CoolingStarted) {
      displayProgramStatus(4);
    } else {
      displayProgramStatus(3);
    }
  } else {
    displayProgramStatus(2);
  }
}

//Fabric Pressing
void handleFabricPressingProgram() {
  // Check if pressure is within the target range
  if (readPressure() >= (fPressureTarget + pressureRange)) {
    // Turn off press
    digitalWrite(pressPumpPin, LOW);

    // Check if height has reached the target cool height
    if (readHeight() <= fCoolHeight) {
      // Turn on cooling pump
      digitalWrite(coolingPumpPin, HIGH);
    }
  } else if (readPressure() <= (fPressureTarget - pressureRange)) {
    // Turn on press
    digitalWrite(pressPumpPin, HIGH);

    // Turn off cooling pump
    digitalWrite(coolingPumpPin, LOW);
  }

  // Check if cooling is complete
  if (readTemperature(DIE_ZONE) <= coolTargetTemp && readTemperature(PUNCH_ZONE) <= coolTargetTemp) {
    if (CoolTargetReached == 0) {
      StartCoolTime = millis();
      CoolTargetReached = 1;
    }
    if (CoolTargetReached && millis() - StartCoolTime >= (CoolTime * 60000)) {
      // Turn off cooling pump
      digitalWrite(coolingPumpPin, LOW);

      // Activate buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();

      // Return to home screen after buzzer duration
      if (millis() - buzzerStartTime >= buzzerDuration) {
        CoolTargetReached = 0;
        digitalWrite(buzzerPin, LOW);
        delay(ButtonPressDelay);
        cancelProgram();
        return;
      }
    }
  }

  // Display Pressing Program status
  displayProgramStatus(7);

  // Check if left button is pressed to cancel the program

  // Read the buttons
  int buttons = readButtons();

  // Scroll through the options
  if (buttons == LEFT_BUTTON) {
    delay(ButtonPressDelay);
    cancelProgram();
    return;
  }
}

// Calibrate the pressure and height
void handleCalibration(){
  int button = readButtons();

  if(button == SELECT_BUTTON){
    heightCalibration = analogRead(heightSensorPin);
    //Something for Pressure Calibration if needed. We don't know if the drift is significant
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Calibration Set");
    delay(1000);
  
    cancelProgram();
    return;
  }

  if(button == LEFT_BUTTON){
    //Return to Home and cancel
    delay(ButtonPressDelay);
    cancelProgram();
    return;
  }

  displayCalibration();

}

//Cooling Only. Function in case of cooling needed outside norma program
void handleCooling() {
  //Turn off heaters
  digitalWrite(heaterPunchPin, LOW);
  digitalWrite(heaterDiePin, LOW);

  // Turn on cooling pump
  digitalWrite(coolingPumpPin, HIGH);

  //Wait to start pressing until operator has pressed select or cancel
  // Read the buttons
  int buttons = readButtons();

  // Left cancels and ends program
  if (buttons == LEFT_BUTTON) {
    cancelProgram();
    delay(ButtonPressDelay);
    return;
  }


  if (CoolingStarted == 0) {

    //Signal Cooling has started
    CoolingStarted = 1;
  }


  // Check if cooling is complete
  if (readTemperature(DIE_ZONE) <= coolTargetTemp && readTemperature(PUNCH_ZONE) <= coolTargetTemp) {
    if (CoolTargetReached == 0) {
      StartCoolTime = millis();
      CoolTargetReached = 1;
    }
    if (CoolTargetReached && millis() - StartCoolTime >= (CoolTime * 60000)) {
      // Turn off cooling pump
      digitalWrite(coolingPumpPin, LOW);

      // Activate buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();

      // Return to home screen after buzzer duration
      if (millis() - buzzerStartTime >= buzzerDuration) {
        CoolTargetReached = 0;
        digitalWrite(buzzerPin, LOW);
        delay(ButtonPressDelay);
        cancelProgram();
        return;
      }
    }
  }

  displayProgramStatus(9);
}



// All the different display screen functions below this point
///////////////////////////////////////////////////////////
//home
void displayHomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write((uint8_t)0);  // Home icon1
  lcd.write((uint8_t)1);  // Home icon2
  lcd.print(" Home");
  lcd.setCursor(0, 1);
  // lcd.print("TT:");
  // lcd.print(paddedValue(targetTemperature));
  // lcd.print(" CH:");
  // lcd.print(paddedValue(coolHeight));
  // lcd.setCursor(7, 1);
  lcd.print("D");
  lcd.print(paddedValue(readTemperature(DIE_ZONE), 2));
  lcd.print(" P");
  lcd.print(paddedValue(readTemperature(PUNCH_ZONE), 2));
  lcd.print(" H");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print(" F");
  lcd.print(paddedValue(readPressure(), 2));
}

//Program Selection
void displayProgramSelection(SelectablePrograms selectedState) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Program:");


  lcd.setCursor(0, 1);
  switch (selectedState) {
    case SETTINGS:
      lcd.print(paddedString("Settings", 16));
      break;
    case PLASTIC:
      lcd.print(paddedString("Start Plastic", 16));
      break;
    case FABRIC:
      lcd.print(paddedString("Start Fabric", 16));
      break;
    case COOL:
      lcd.print(paddedString("Cooling", 16));
      break;
    case CALIB:
      lcd.print(paddedString("Calibration", 16));
      break;
  }
}

//General Display for all programs
void displayProgramStatus(int ProgID) {
  // Input it the program ID
  // 1 = Plastic Heating
  // 2 = Plastic Hold
  // 3 = Plastic Press
  // 4 = Plastic Cool
  // 5 = Fabric Heating
  // 6 = Fabric Hold
  // 7 = Fabric Press
  // 8 = Fabric Cool

  lcd.setCursor(0, 0);
  switch (ProgID) {
    case 1:
      lcd.print("Pl");
      lcd.write((uint8_t)3);
      lcd.write((uint8_t)3);
      break;
    case 2:
      lcd.print("Pl==");
      break;
    case 3:
      lcd.print("Pl");
      lcd.write((uint8_t)5);
      lcd.write((uint8_t)6);
      break;
    case 4:
      lcd.print("Pl");
      lcd.write((uint8_t)7);
      lcd.write((uint8_t)7);
      break;
    case 5:
      lcd.print("Fb");
      lcd.write((uint8_t)3);
      lcd.write((uint8_t)3);
      break;
    case 6:
      lcd.print("Fb==");
      break;
    case 7:
      lcd.print("Fb");
      lcd.write((uint8_t)5);
      lcd.write((uint8_t)6);
      break;
    case 8:
      lcd.print("Fb");
      lcd.write((uint8_t)7);
      lcd.write((uint8_t)7);
      break;
    case 9:
      lcd.write((uint8_t)7);  // Cooling Icon
      lcd.write((uint8_t)7);  // Cooling Icon
      lcd.write((uint8_t)7);  // Cooling Icon
      lcd.write((uint8_t)7);  // Cooling Icon
      break;
  }
  lcd.print(" TT");
  lcd.print(paddedValue(pTargetTemperature, 2));
  lcd.print(" CH");
  lcd.print(paddedValue(pCoolHeight, 3));
  lcd.setCursor(0, 1);
  lcd.print("D");
  lcd.print(paddedValue(readTemperature(DIE_ZONE), 2));
  lcd.print(" P");
  lcd.print(paddedValue(readTemperature(PUNCH_ZONE), 2));
  lcd.print(" H");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print(" F");
  lcd.print(paddedValue(readPressure(), 2));
}


void displayAdjustingMode(Option option) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(paddedString("Adjusting Mode", 16));
  lcd.setCursor(0, 1);

  switch (option) {
    case P_TARGET_TEMP:
      lcd.print(paddedString("P Target T", 11));
      lcd.print(paddedValue(pTargetTemperature, 3));

      break;
    case F_TARGET_TEMP:
      lcd.print(paddedString("F Target T", 11));
      lcd.print(paddedValue(fTargetTemperature, 3));

      break;
    case F_HEAT_TIME:
      lcd.print(paddedString("F Heat t", 11));
      lcd.print(paddedValue(fHeatTime, 3));

      break;
    case HEIGHT_CALIBRATION:
      lcd.print(paddedString("Height Calib", 11));
      lcd.print(paddedValue(heightCalibration, 3));

      break;
    case P_COOLING_HEIGHT:
      lcd.print(paddedString("P Cool Ht", 11));
      lcd.print(paddedValue(pCoolHeight, 3));

      break;
    case F_COOLING_HEIGHT:
      lcd.print(paddedString("F Cool Ht", 11));
      lcd.print(paddedValue(fCoolHeight, 3));

      break;
    case COOL_TIME:
      lcd.print(paddedString("Cool Time", 11));
      lcd.print(paddedValue(CoolTime, 3));

      break;
    case COOL_TARGET_TEMP:
      lcd.print(paddedString("Cool TarT", 11));
      lcd.print(paddedValue(coolTargetTemp, 3));

      break;
    case P_PRESSURE_TARGET:
      lcd.print(paddedString("P TargetPr", 11));
      lcd.print(paddedValue(pPressureTarget, 3));

      break;
    case F_PRESSURE_TARGET:
      lcd.print(paddedString("F TPr", 11));
      lcd.print(paddedValue(fPressureTarget, 3));

      break;
    case PRESSURE_RANGE:
      lcd.print(paddedString("Pr Range", 11));
      lcd.print(paddedValue(pressureRange, 3));

      break;
    case PRESSURE_CALIBRATION:
      lcd.print(paddedString("Pr Range", 11));
      lcd.print(paddedValue(PressureCalibration, 3));

      break;
  }
}


void displaySettingsPage(Option selectedOption) {

  lcd.setCursor(0, 0);

  switch (selectedOption) {
    case P_TARGET_TEMP:
      lcd.print(paddedString("> P Target T", 13));
      lcd.print(paddedValue(pTargetTemperature, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("  F Target T", 13));
      lcd.print(paddedValue(fTargetTemperature, 3));
      break;
    case F_TARGET_TEMP:
      lcd.print(paddedString("  P Target T", 13));
      lcd.print(paddedValue(pTargetTemperature, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("> F Target T", 13));
      lcd.print(paddedValue(fTargetTemperature, 3));
      break;
    case F_HEAT_TIME:
      lcd.print(paddedString("> F Heat t", 13));
      lcd.print(paddedValue(fHeatTime, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("  Ht Calib", 13));
      lcd.print(paddedValue(heightCalibration, 3));
      break;
    case HEIGHT_CALIBRATION:
      lcd.print(paddedString("  F Heat t", 13));
      lcd.print(paddedValue(fHeatTime, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("> Ht Calib", 13));
      lcd.print(paddedValue(heightCalibration, 3));
      break;
    case P_COOLING_HEIGHT:
      lcd.print(paddedString("> P Cool Ht", 13));
      lcd.print(paddedValue(pCoolHeight, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("  F Cool Ht", 13));
      lcd.print(paddedValue(fCoolHeight, 3));
      break;
    case F_COOLING_HEIGHT:
      lcd.print(paddedString("  P Cool Ht", 13));
      lcd.print(paddedValue(pCoolHeight, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("> F Cool Ht", 13));
      lcd.print(paddedValue(fCoolHeight, 3));
      break;
    case COOL_TIME:
      lcd.print(paddedString("> Cool t", 13));
      lcd.print(paddedValue(CoolTime, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("  Cool TarT", 13));
      lcd.print(paddedValue(coolTargetTemp, 3));
      break;
    case COOL_TARGET_TEMP:
      lcd.print(paddedString("  Cool Time", 13));
      lcd.print(paddedValue(CoolTime, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("> Cool TarT", 13));
      lcd.print(paddedValue(coolTargetTemp, 3));
      break;
    case P_PRESSURE_TARGET:
      lcd.print(paddedString("> P TargetPr", 13));
      lcd.print(paddedValue(pPressureTarget, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("  F TargetPr", 13));
      lcd.print(paddedValue(fPressureTarget, 3));
      break;
    case F_PRESSURE_TARGET:
      lcd.print(paddedString("  P TargetPr", 13));
      lcd.print(paddedValue(pPressureTarget, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("> F TargetPr", 13));
      lcd.print(paddedValue(fPressureTarget, 3));
      break;
    case PRESSURE_RANGE:
      lcd.print(paddedString("> Pr Range", 13));
      lcd.print(paddedValue(pressureRange, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("  Pr Calib", 13));
      break;
    case PRESSURE_CALIBRATION:
      lcd.print(paddedString("  Pr Range", 13));
      lcd.print(paddedValue(pressureRange, 3));
      lcd.setCursor(0, 1);
      lcd.print(paddedString("> Pr Calib", 13));
      lcd.print(paddedValue(PressureCalibration, 3));
      break;
  }

  // lcd.setCursor(0, 3);
  // lcd.print("Press U/D to scroll");
  // lcd.setCursor(0, 4);
  // lcd.print("Press L/R to adjust");
}

void displayCalibration(){

  if (CalibStarted == 0){
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write((uint8_t)4);
    lcd.write((uint8_t)4);
    lcd.print(" Set Pr 1 Ht 0  ");
    CalibStarted = 1;
    delay(ButtonPressDelay);
  }

  lcd.setCursor(0,1);
  lcd.print("Ht: ");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print(" Fr: ");
  lcd.print(paddedValue(readPressure(), 3));
}


//#################################################
// Special Functions

//Heater Control with duty cycles
void controlHeater(Zone zone) {
  unsigned long currentMillis = millis() - StartHeatTime;

  HeatingState temp_heatingState = HEATING_100;



  //Assign the Heating state based on the heating Zone
  if (zone == PUNCH_ZONE) {
    temp_heatingState = p_heatingState;
  } else {
    temp_heatingState = f_heatingState;
  }

  if (millis() - lastHeatingTime >= heatingDelay100) {


    if (temp_heatingState == HEATING_100) {
      // Switch to heating at 75% duty cycle if conditions are met
      if ((currentMillis >= 300000 || readTemperature(zone) >= 60)) {
        temp_heatingState = HEATING_75;
      }
    } else if (temp_heatingState == HEATING_75) {
      // Switch to heating at 50% duty cycle if conditions are met
      if ((currentMillis >= 480000 || readTemperature(zone) >= 75)) {
        temp_heatingState = HEATING_50;
      }
    }

    // Control heater based on heating state
    if (temp_heatingState == HEATING_100) {
      digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, HIGH);
    } else if (temp_heatingState == HEATING_75) {
      unsigned long heatingTime = currentMillis % 4000;

      if (heatingTime < 3000) {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, HIGH);
      } else {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, LOW);
      }
    } else if (temp_heatingState == HEATING_50) {
      unsigned long heatingTime = currentMillis % 6000;

      if (heatingTime < 3000) {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, HIGH);
      } else {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, LOW);
      }
    }
  }


  //Reassign the heating state incase it has changed
  if (zone == PUNCH_ZONE) {
    p_heatingState = temp_heatingState;
  } else {
    f_heatingState = temp_heatingState;
  }
}

//Load the Settings saved to SD card
void loadSettings() {
  // Open the settings file on the SD card
  File settingsFile = SD.open("settings.txt");

  // Check if the file opened successfully
  if (settingsFile) {
    // Read the settings from the file
    while (settingsFile.available()) {
      String line = settingsFile.readStringUntil('\n');
      line.trim();

      // Split the line into key-value pairs
      int separatorIndex = line.indexOf('=');
      if (separatorIndex != -1) {
        String key = line.substring(0, separatorIndex);
        String value = line.substring(separatorIndex + 1);

        // Convert the value to the appropriate data type and assign it to the corresponding variable
        if (key == "P_Target_Temp") {
          pTargetTemperature = value.toInt();
        } else if (key == "F_Target_Temp") {
          fTargetTemperature = value.toInt();
        } else if (key == "F_Heat_Time") {
          fHeatTime = value.toInt();
        } else if (key == "Height_Calibration") {
          heightCalibration = value.toInt();
        } else if (key == "P_Cooling_Height") {
          pCoolHeight = value.toInt();
        } else if (key == "F_Cooling_Height") {
          fCoolHeight = value.toInt();
        } else if (key == "P_Cool_Time") {
          CoolTime = value.toInt();
        } else if (key == "Cool_Target_Temp") {
          coolTargetTemp = value.toInt();
        } else if (key == "P_Pressure_Target") {
          pPressureTarget = value.toInt();
        } else if (key == "F_Pressure_Target") {
          fPressureTarget = value.toInt();
        } else if (key == "Pressure_Range") {
          pressureRange = value.toInt();
        }
      }
    }

    // Close the settings file
    settingsFile.close();
  } else {
    // If the file doesn't exist or couldn't be opened, use default settings
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD Error, Used");
    lcd.setCursor(0, 1);
    lcd.print("Default Settings");
    delay(1000);
  }
}

//Special funciton for adjusting the value of Setting variable
//Gives the ability to adjust different values by different intervals
void adjustOptionValue(Option option, bool increase) {
  switch (option) {
    case P_TARGET_TEMP:
      if (increase) {
        pTargetTemperature += 5;
      } else {
        pTargetTemperature -= 5;
      }
      break;
    case F_TARGET_TEMP:
      if (increase) {
        fTargetTemperature += 5;
      } else {
        fTargetTemperature -= 5;
      }
      break;
    case F_HEAT_TIME:
      if (increase) {
        fHeatTime++;
      } else {
        fHeatTime--;
      }
      break;
    case HEIGHT_CALIBRATION:
      if (increase) {
        heightCalibration += 10;
      } else {
        heightCalibration -= 10;
      }
      break;
    case P_COOLING_HEIGHT:
      if (increase) {
        pCoolHeight += 10;
      } else {
        pCoolHeight -= 10;
      }
      break;
    case F_COOLING_HEIGHT:
      if (increase) {
        fCoolHeight += 10;
      } else {
        fCoolHeight -= 10;
      }
      break;
    case COOL_TIME:
      if (increase) {
        CoolTime++;
      } else {
        CoolTime--;
      }
      break;
    case COOL_TARGET_TEMP:
      if (increase) {
        coolTargetTemp += 5;
      } else {
        coolTargetTemp -= 5;
      }
      break;
    case P_PRESSURE_TARGET:
      if (increase) {
        pPressureTarget += 5;
      } else {
        pPressureTarget -= 5;
      }
      break;
    case F_PRESSURE_TARGET:
      if (increase) {
        fPressureTarget += 5;
      } else {
        fPressureTarget -= 5;
      }
      break;
    case PRESSURE_RANGE:
      if (increase) {
        pressureRange += 1;
      } else {
        pressureRange -= 1;
      }
      break;
  }
}

//Save current Settings to the SD card
void saveSettings() {
  // Open the settings file in write mode
  File settingsFile = SD.open(settingsFilePath, FILE_WRITE);

  if (settingsFile) {
    // Write the settings values to the file in the correct format
    settingsFile.println("P_TARGET_TEMP=" + String(pTargetTemperature));
    settingsFile.println("F_TARGET_TEMP=" + String(fTargetTemperature));
    settingsFile.println("F_HEAT_TIME=" + String(fHeatTime));
    settingsFile.println("HEIGHT_CALIBRATION=" + String(heightCalibration));
    settingsFile.println("P_COOLING_HEIGHT=" + String(pCoolHeight));
    settingsFile.println("F_COOLING_HEIGHT=" + String(fCoolHeight));
    settingsFile.println("P_COOL_TIME=" + String(CoolTime));
    settingsFile.println("COOL_TARGET_TEMP=" + String(coolTargetTemp));
    settingsFile.println("P_PRESSURE_TARGET=" + String(pPressureTarget));
    settingsFile.println("F_PRESSURE_TARGET=" + String(fPressureTarget));
    settingsFile.println("PRESSURE_RANGE=" + String(pressureRange));

    // Close the settings file
    settingsFile.close();
  } else {
    // Failed to open the settings file
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SD Error");
    lcd.setCursor(0, 1);
    lcd.print("Failed to Save");
    delay(1000);
  }
}

//Cancel current program and return to home
void cancelProgram() {
  // Turn off all components
  digitalWrite(heaterPunchPin, LOW);
  digitalWrite(heaterDiePin, LOW);
  digitalWrite(pressPumpPin, LOW);
  digitalWrite(coolingPumpPin, LOW);
  digitalWrite(buzzerPin, LOW);

  //Reset all program bool variables
  CoolTargetReached = 0;
  DataLogging = 0;
  HeatBuzzerSounded = 0;
  PressingStarted = 0;
  CoolingStarted = 0;
  CalibStarted = 0;


  // Reset variables and state
  currentState = HOME_SCREEN;
  displayHomeScreen();
}

//Return padded values for ints with a given width
//Left Justified
String paddedValue(int value, int width) {
  String padded = String(value);
  while (padded.length() < width) {
    padded = padded + " ";
  }
  return padded;
}

//Make Left Justified Strings
String paddedString(String value, int width) {
  String padded = String(value);
  while (padded.length() < width) {
    padded = padded + " ";
  }
  return padded;
}

//Read the temperature of a given sensor pin
float readTemperature(int sensorPin) {
  int rawValue = analogRead(sensorPin);

  // Convert the raw ADC value to resistance
  float resistance = 10000.0 / (1023.0 / rawValue - 1.0);

  // Calculate the temperature using the Steinhart-Hart equation
  float temperature = 1.0 / (log(resistance / 10000.0) / 3976.0 + 1.0 / 298.15) - 273.15;

  return temperature;
}

//Read height of the linear potentiometer with calibration removed
float readHeight() {

  float val;


  val = (analogRead(heightSensorPin) - heightCalibration) / 19.3018868;

  return float(round(val * 5)) * 2;
}

//Read pressure transducer value and convert to tenth of 1 ton force
float readPressure() {

  float pressure = (analogRead(pressurePin)/1023.0)* 15.70796327 ;  // Convert reading to pressure in in Tonnes

  return pressure;
}

//Read which button is being pressed
int readButtons() {
  int buttons = 0;

  // Read the state of the buttons
  if (lcd.readButtons() & BUTTON_UP)
    buttons |= UP_BUTTON;
  if (lcd.readButtons() & BUTTON_DOWN)
    buttons |= DOWN_BUTTON;
  if (lcd.readButtons() & BUTTON_LEFT)
    buttons |= LEFT_BUTTON;
  if (lcd.readButtons() & BUTTON_RIGHT)
    buttons |= RIGHT_BUTTON;
  if (lcd.readButtons() & BUTTON_SELECT)
    buttons |= SELECT_BUTTON;

  return buttons;
}

// Function to create a file name for each new instance
String createDataFilename(const String& programType) {

  // Generate the filename with a unique ID, program type, and timestamp
  String filename = "PressData/";
  filename += generateUniqueID(programType);  // Implement your logic to generate a unique ID
  filename += "_" + programType + "_" + ".csv";

  return filename;
}

//Generate a new ID for each new plastic or Fabric run
int generateUniqueID(const String& programType) {
  String idListFilePath = programType == "fabric" ? fabricIDListPath : plasticIDListPath;

  // Check if the ID list file exists
  if (!SD.exists(idListFilePath)) {
    // Create the ID list file if it doesn't exist
    File idListFile = SD.open(idListFilePath, FILE_WRITE);
    idListFile.close();
  }

  // Read the ID list file
  File idListFile = SD.open(idListFilePath);
  String idListContent;
  if (idListFile) {
    while (idListFile.available()) {
      idListContent += idListFile.readStringUntil('\n');
    }
    idListFile.close();
  }

  // Extract the last used ID from the ID list
  int lastID = 0;
  int idIndex = idListContent.lastIndexOf('\n');
  if (idIndex != -1) {
    lastID = idListContent.substring(idIndex + 1).toInt();
  }

  // Increment the last used ID
  int newID = lastID + 1;

  // Append the new ID to the ID list
  idListContent += String(newID) + '\n';

  // Save the updated ID list to the file
  File updatedIDListFile = SD.open(idListFilePath, FILE_WRITE);
  if (updatedIDListFile) {
    updatedIDListFile.print(idListContent);
    updatedIDListFile.close();
  }

  return newID;
}

//Log the Data
void logData(const String& filename, const String& data) {
  File dataFile = SD.open(filename, FILE_WRITE);
  if (dataFile) {
    dataFile.println(data);
    dataFile.close();
  } else {
    lcd.print("SD Card Error!");
    while (true) {
      // Wait indefinitely if there's an error writing to the SD card
    }
  }
}

