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
  NUM_OPTIONS
};

// Heater Zones
enum Zone {
  PUNCH_ZONE,
  DIE_ZONE
};

// // LCD Shield Objects
// Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
// Adafruit_MCP23017 mcp;

// Temperature Sensor Pins
const int punchSensorPin = A0;
const int dieSensorPin = A1;
const int heightSensorPin = A2;
const int pressurePin = A3;
const int buzzerPin = A4;

// SD card setup
const int sdCardPin = 10;
const String settingsFilePath = "/settings.txt";

// Heating and Cooling Parameters
int pTargetTemperature = 70;
int fTargetTemperature = 85;
int fHeatTime = 5;
int heightCalibration = 0;
int pCoolHeight = 130;
int fCoolHeight = 30;
int CoolTime = 5;
int coolTargetTemp = 15;
int pPressureTarget = 7;
int fPressureTarget = 3;
int pressureRange = 0.5;
int PressureTarget = 3;
int CoolHeight = 130;
int coolingStarted = 0;

// Heater Control Parameters
const int heaterPunchPin = 3;
const int heaterDiePin = 4;
const int coolingPumpPin = 5;
const int pressPumpPin = 6;

// Timing Parameters
const unsigned long heatingDelay100 = 500;   // 0.5 seconds
const unsigned long heatingDelay75 = 3000;   // 3 seconds
const unsigned long heatingDelay50 = 5000;   // 5 seconds
const unsigned long coolingDelay = 1000;     // 1 second
const unsigned long buzzerDuration = 3000;   // 3 seconds

// Heating States
enum HeatingState {
  HEATING_100,
  HEATING_75,
  HEATING_50
};

// Program States
enum ProgramState {
  HOME_SCREEN,
  SETTINGS_SCREEN,
  PROGRAM_SELECTION,
  PLASTIC_PROGRAM,
  FABRIC_PROGRAM,
  PLASTIC_PRESSING_PROGRAM,
  FABRIC_PRESSING_PROGRAM
};

ProgramState currentState = HOME_SCREEN;
HeatingState heatingState = HEATING_100;

// Timing Variables
unsigned long lastHeatingTime = 0;
unsigned long lastCoolingTime = 0;
unsigned long coolingStartTime = 0;
unsigned long buzzerStartTime = 0;
unsigned long holdHeatStartTime = 0;


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

  // Display Home Screen
  displayHomePage();
}

void loop() {
  if (currentState == HOME_SCREEN) {
    handleHomeScreen();
  } else if (currentState == SETTINGS_SCREEN) {
    handleSettingsScreen();
  } else if (currentState == PROGRAM_SELECTION) {
    handleProgramSelection();
  } else if (currentState == PLASTIC_PROGRAM) {
    handlePlasticProgram();
  } else if (currentState == FABRIC_PROGRAM) {
    handleFabricProgram();
  } else if (currentState == PLASTIC_PRESSING_PROGRAM){
    handlePlasticPressingProgram();
  } else if (currentState == FABRIC_PRESSING_PROGRAM){
    handleFabricPressingProgram();
  }
  
}

void handleHomeScreen() {
  if (lcd.readButtons() & UP_BUTTON) {
    currentState = PROGRAM_SELECTION;
    displayProgramSelection();
  }
}

void displayHomePage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.write(0); // Home icon
  lcd.setCursor(0, 1);
  lcd.print("TT:");
  lcd.print(paddedValue(pTargetTemperature, 2));
  lcd.print(" CH:");
  lcd.print(paddedValue(pCoolHeight, 2));
  lcd.setCursor(7, 1);
  lcd.print("D:");
  lcd.print(paddedValue(readTemperature(DIE_ZONE), 2));
  lcd.print(" P:");
  lcd.print(paddedValue(readTemperature(PUNCH_ZONE), 2));
  lcd.print(" H:");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print("  F:");
  lcd.print(paddedValue(readPressure(), 2));
}

void displayProgramSelection() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Program:");

  lcd.setCursor(0, 1);
  lcd.print("Plastic Program");
}


void handlePlasticProgram() {
  // Check if both zones reached target temperature
  if (readTemperature(PUNCH_ZONE) >= pTargetTemperature && readTemperature(DIE_ZONE) >= pTargetTemperature) {
    // Turn off heaters
    digitalWrite(heaterPunchPin, LOW);
    digitalWrite(heaterDiePin, LOW);

    // Check if both zones maintained target temperature
    if (readTemperature(PUNCH_ZONE) >= pTargetTemperature && readTemperature(DIE_ZONE) >= pTargetTemperature) {
      // Activate buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();
    }

    // Wait for buzzer duration
    if (millis() - buzzerStartTime >= buzzerDuration) {
      // Turn off buzzer
      digitalWrite(buzzerPin, LOW);

      // Proceed to Pressing program
      currentState = PLASTIC_PRESSING_PROGRAM;

      // Set the target pressure and cool height
      PressureTarget = pPressureTarget;
      CoolHeight = pCoolHeight;
      displayPressingProgram();
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
  displayProgramStatus("Plas");

  // Check if left button is pressed to cancel the program

  // Read the buttons
  int buttons = readButtons();

  // Scroll through the options
  if (buttons & LEFT_BUTTON) {
    cancelProgram();
  }
}


void displayProgramStatus(String progName) {
  lcd.setCursor(0, 0);
  lcd.print(progName);
  lcd.print("TT:");
  lcd.print(paddedValue(pTargetTemperature, 2));
  lcd.print(" CH:");
  lcd.print(paddedValue(pCoolHeight, 2));
  lcd.setCursor(0, 1);
  lcd.print("D:");
  lcd.print(paddedValue(readTemperature(DIE_ZONE), 2));
  lcd.print(" P:");
  lcd.print(paddedValue(readTemperature(PUNCH_ZONE), 2));
  lcd.print(" H:");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print("  F:");
  lcd.print(paddedValue(readPressure(), 2));
}

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
      displayPressingProgram();
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
  displayProgramStatus("Fabr");

  // Check if left button is pressed to cancel the program

  // Read the buttons
  int buttons = readButtons();

  // Scroll through the options
  if (buttons & LEFT_BUTTON) {
    cancelProgram();
  }
}

void handlePlasticPressingProgram() {
  // Check if pressure is within the target range
  if (readPressure() >= (pPressureTarget + pressureRange)) {
    // Turn off press
    digitalWrite(pressPumpPin, LOW);

    // Check if height has reached the target cool height
    if (readHeight() <= pCoolHeight) {
      // Turn on cooling pump
      digitalWrite(coolingPumpPin, HIGH);

    }
  } else if (readPressure() <= (PressureTarget - pressureRange)) {
    // Turn on press
    digitalWrite(pressPumpPin, HIGH);

    // Turn off cooling pump
    digitalWrite(coolingPumpPin, LOW);
  }

  // Check if cooling is complete
  if (readTemperature(DIE_ZONE) <= coolTargetTemp && readTemperature(PUNCH_ZONE) <= coolTargetTemp) {
     if (coolingStarted == 0){
       coolingStartTime = millis();
       coolingStarted = 1;
     }
    if (coolingStarted && millis() - coolingStartTime >= (CoolTime * 60000)) {
      // Turn off cooling pump
      digitalWrite(coolingPumpPin, LOW);

      // Activate buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();

      // Return to home screen after buzzer duration
      if (millis() - buzzerStartTime >= buzzerDuration) {
        coolingStarted = 0;
        digitalWrite(buzzerPin, LOW);
        currentState = HOME_SCREEN;
        displayHomeScreen();
        return;
      }
    }
  }

  // Display Pressing Program status
  displayPlasticPressingProgramStatus();

  // Check if left button is pressed to cancel the program

  // Read the buttons
  int buttons = readButtons();

  // Scroll through the options
  if (buttons & LEFT_BUTTON) {
    cancelProgram();
  }
}

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
     if (coolingStarted == 0){
       coolingStartTime = millis();
       coolingStarted = 1;
     }
    if (coolingStarted && millis() - coolingStartTime >= (CoolTime * 60000)) {
      // Turn off cooling pump
      digitalWrite(coolingPumpPin, LOW);

      // Activate buzzer
      digitalWrite(buzzerPin, HIGH);
      buzzerStartTime = millis();

      // Return to home screen after buzzer duration
      if (millis() - buzzerStartTime >= buzzerDuration) {
        coolingStarted = 0;
        digitalWrite(buzzerPin, LOW);
        currentState = HOME_SCREEN;
        displayHomeScreen();
        return;
      }
    }
  }

  // Display Pressing Program status
  displayFabricPressingProgramStatus();

  // Check if left button is pressed to cancel the program

  // Read the buttons
  int buttons = readButtons();

  // Scroll through the options
  if (buttons & LEFT_BUTTON) {
    cancelProgram();
  }
}


void displayPressingProgram() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pressing Program");
  lcd.setCursor(0, 1);
  lcd.print("Press to continue");
}

void displayFabricPressingProgramStatus() {
  lcd.setCursor(0, 0);
  lcd.print("Press TT");
  lcd.print(paddedValue(fTargetTemperature, 2));
  lcd.print(" CH");
  lcd.print(paddedValue(fCoolHeight, 2));

  lcd.setCursor(0, 1);
  lcd.print("D:");
  lcd.print(paddedValue(readTemperature(DIE_ZONE), 2));
  lcd.print(" P:");
  lcd.print(paddedValue(readTemperature(PUNCH_ZONE), 2));
  lcd.print(" H:");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print("  F:");
  lcd.print(paddedValue(readPressure(), 2));
}

void displayPlasticPressingProgramStatus() {
  lcd.setCursor(0, 0);
  lcd.print("Press TT");
  lcd.print(paddedValue(pTargetTemperature, 2));
  lcd.print(" CH");
  lcd.print(paddedValue(pCoolHeight, 2));

  lcd.setCursor(0, 1);
  lcd.print("D:");
  lcd.print(paddedValue(readTemperature(DIE_ZONE), 2));
  lcd.print(" P:");
  lcd.print(paddedValue(readTemperature(PUNCH_ZONE), 2));
  lcd.print(" H:");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print("  F:");
  lcd.print(paddedValue(readPressure(), 2));
}


void controlHeater(Zone zone) {
  unsigned long currentMillis = millis();

  if (currentMillis - lastHeatingTime >= heatingDelay100) {
    lastHeatingTime = currentMillis;

    if (heatingState == HEATING_100) {
      // Switch to heating at 75% duty cycle if conditions are met
      if ((currentMillis >= 300000 || readTemperature(zone) >= 60)) {
        heatingState = HEATING_75;
      }
    } else if (heatingState == HEATING_75) {
      // Switch to heating at 50% duty cycle if conditions are met
      if ((currentMillis >= 480000 || readTemperature(zone) >= 75)) {
        heatingState = HEATING_50;
      }
    }

    // Control heater based on heating state
    if (heatingState == HEATING_100) {
      digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, HIGH);
    } else if (heatingState == HEATING_75) {
      unsigned long heatingTime = currentMillis % 4000;

      if (heatingTime < 3000) {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, HIGH);
      } else {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, LOW);
      }
    } else if (heatingState == HEATING_50) {
      unsigned long heatingTime = currentMillis % 6000;

      if (heatingTime < 5000) {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, HIGH);
      } else {
        digitalWrite(zone == PUNCH_ZONE ? heaterPunchPin : heaterDiePin, LOW);
      }
    }
  }
}

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
    lcd.setCursor(0,0);
    lcd.print("SD Error, Default Settings Values Kept");
  }
}


void handleSettingsScreen() {
  static Option selectedOption = P_TARGET_TEMP;  // Currently selected option

  displaySettingsPage(selectedOption);

  while (true) {
    int button = readButtons();  // Read the button state as an integer

    if (button == BUTTON_UP) {
      selectedOption = static_cast<Option>((selectedOption + 1) % NUM_OPTIONS);
      displaySettingsPage(selectedOption);
    } else if (button == BUTTON_DOWN) {
      selectedOption = static_cast<Option>((selectedOption + NUM_OPTIONS - 1) % NUM_OPTIONS);
      displaySettingsPage(selectedOption);
    } else if (button == BUTTON_LEFT) {
      adjustOptionValue(selectedOption, false);  // Decrease the value
      displayAdjustingMode(selectedOption);
    } else if (button == BUTTON_RIGHT) {
      adjustOptionValue(selectedOption, true);  // Increase the value
      displayAdjustingMode(selectedOption);
    } else if (button == BUTTON_SELECT) {
      saveSettings();  // Save the updated settings to the SD card
      delay(2000);
      break;
    }
  }
}

void displayAdjustingMode(Option option) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Adjusting Mode");
  lcd.setCursor(0, 1);

  switch (option) {
    case P_TARGET_TEMP:
      lcd.print("P Target Temp: ");
      lcd.print(pTargetTemperature);
      lcd.print(" ");
      break;
    case F_TARGET_TEMP:
      lcd.print("F Target Temp: ");
      lcd.print(fTargetTemperature);
      lcd.print(" ");
      break;
    case F_HEAT_TIME:
      lcd.print("F Heat Time: ");
      lcd.print(fHeatTime);
      lcd.print(" ");
      break;
    case HEIGHT_CALIBRATION:
      lcd.print("Height Calib.: ");
      lcd.print(heightCalibration);
      lcd.print(" ");
      break;
    case P_COOLING_HEIGHT:
      lcd.print("P Cool Height: ");
      lcd.print(pCoolHeight);
      lcd.print(" ");
      break;
    case F_COOLING_HEIGHT:
      lcd.print("F Cool Height: ");
      lcd.print(fCoolHeight);
      lcd.print(" ");
      break;
    case COOL_TIME:
      lcd.print("Cool Time: ");
      lcd.print(CoolTime);
      lcd.print(" ");
      break;
    case COOL_TARGET_TEMP:
      lcd.print("Cool Target: ");
      lcd.print(coolTargetTemp);
      lcd.print(" ");
      break;
    case P_PRESSURE_TARGET:
      lcd.print("P Pressure: ");
      lcd.print(pPressureTarget);
      lcd.print(" ");
      break;
    case F_PRESSURE_TARGET:
      lcd.print("F Pressure: ");
      lcd.print(fPressureTarget);
      lcd.print(" ");
      break;
    case PRESSURE_RANGE:
      lcd.print("Pressure Range: ");
      lcd.print(pressureRange);
      lcd.print(" ");
      break;
  }
}


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
        heightCalibration++;
      } else {
        heightCalibration--;
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
        pPressureTarget += 1;
      } else {
        pPressureTarget -= 1;
      }
      break;
    case F_PRESSURE_TARGET:
      if (increase) {
        fPressureTarget += 1;
      } else {
        fPressureTarget -= 1;
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



  void displaySettingsPage(Option selectedOption) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Settings");

  lcd.setCursor(0, 1);

  switch (selectedOption) {
    case P_TARGET_TEMP:
      lcd.print("> P Target Temp:");
      lcd.setCursor(0, 2);
      lcd.print("  F Target Temp:");
      break;
    case F_TARGET_TEMP:
      lcd.print("  P Target Temp:");
      lcd.setCursor(0, 2);
      lcd.print("> F Target Temp:");
      break;
    case F_HEAT_TIME:
      lcd.print("> F Heat Time:");
      lcd.setCursor(0, 2);
      lcd.print("  Height Calib.:");
      break;
    case HEIGHT_CALIBRATION:
      lcd.print("  F Heat Time:");
      lcd.setCursor(0, 2);
      lcd.print("> Height Calib.:");
      break;
    case P_COOLING_HEIGHT:
      lcd.print("> P Cooling Height:");
      lcd.setCursor(0, 2);
      lcd.print("  F Cooling Height:");
      break;
    case F_COOLING_HEIGHT:
      lcd.print("  P Cooling Height:");
      lcd.setCursor(0, 2);
      lcd.print("> F Cooling Height:");
      break;
    case COOL_TIME:
      lcd.print("> Cool Time:");
      lcd.setCursor(0, 2);
      lcd.print("  Cool Target Temp:");
      break;
    case COOL_TARGET_TEMP:
      lcd.print("  Cool Time:");
      lcd.setCursor(0, 2);
      lcd.print("> Cool Target Temp:");
      break;
    case P_PRESSURE_TARGET:
      lcd.print("> P Pressure Target:");
      lcd.setCursor(0, 2);
      lcd.print("  F Pressure Target:");
      break;
    case F_PRESSURE_TARGET:
      lcd.print("  P Pressure Target:");
      lcd.setCursor(0, 2);
      lcd.print("> F Pressure Target:");
      break;
    case PRESSURE_RANGE:
      lcd.print("> Pressure Range:");
      break;
  }

  // lcd.setCursor(0, 3);
  // lcd.print("Press U/D to scroll");
  // lcd.setCursor(0, 4);
  // lcd.print("Press L/R to adjust");
}



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
    Serial.println("Failed to open the settings file for writing");
  }
}




void handleProgramSelection() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select Program:");

  int programSelection = 0; // 0 for plastic program, 1 for fabric program

  while (true) {
    lcd.setCursor(0, 1);

    if (programSelection == 0) {
      lcd.print("Plastic Program");
    } else {
      lcd.print("Fabric Program ");
    }

    int button = readButtons();

    if (button == BUTTON_UP) {
      programSelection = 1 - programSelection; // Toggle program selection
    } else if (button == BUTTON_DOWN) {
      programSelection = 1 - programSelection; // Toggle program selection
    } else if (button == BUTTON_RIGHT) {
      if (programSelection == 0) {
        currentState = PLASTIC_PROGRAM;
      } else {
        currentState = FABRIC_PROGRAM;
      }
      break; // Exit the program selection loop
    }
  }
}







void cancelProgram() {
  // Turn off all components
  digitalWrite(heaterPunchPin, LOW);
  digitalWrite(heaterDiePin, LOW);
  digitalWrite(pressPumpPin, LOW);
  digitalWrite(coolingPumpPin, LOW);
  digitalWrite(buzzerPin, LOW);
  coolingStarted = 0;

  // Reset variables and state
  currentState = HOME_SCREEN;
  displayHomeScreen();
}

String paddedValue(int value, int width) {
  String padded = String(value);
  while (padded.length() < width) {
    padded = " " + padded;
  }
  return padded;
}


float readTemperature(int sensorPin) {
  int rawValue = analogRead(sensorPin);

  // Convert the raw ADC value to resistance
  float resistance = 10000.0 / (1023.0 / rawValue - 1.0);

  // Calculate the temperature using the Steinhart-Hart equation
  float temperature = 1.0 / (log(resistance / 10000.0) / 3976.0 + 1.0 / 298.15) - 273.15;

  return temperature;
}

float readHeight() {

  float val;


  val =  (analogRead(heightSensorPin) - heightCalibration) / 19.3018868;

  return float(round(val * 5))*2;

}

float readPressure() {
  float voltage = analogRead(pressurePin) * (5.0 / 1023.0);  // Convert ADC reading to voltage (assuming 5V reference)
  float pressure = voltage * 10000.0;  // Convert voltage to pressure in PSI (assuming 10V corresponds to 10,000 PSI)

  return pressure;
}



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

void displayHomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Home ");
  lcd.write(0); // Home icon
  lcd.setCursor(0, 1);
  // lcd.print("TT:");
  // lcd.print(paddedValue(targetTemperature));
  // lcd.print(" CH:");
  // lcd.print(paddedValue(coolHeight));
  // lcd.setCursor(7, 1);
  lcd.print("D:");
  lcd.print(paddedValue(readTemperature(DIE_ZONE), 2));
  lcd.print(" P:");
  lcd.print(paddedValue(readTemperature(PUNCH_ZONE), 2));
  lcd.print(" H:");
  lcd.print(paddedValue(readHeight(), 3));
  lcd.print("  F:");
  lcd.print(paddedValue(readPressure(), 2));
}
