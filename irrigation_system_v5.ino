
//---- necessary libraries ---------------------------------------------------------------------
#include <LiquidCrystal.h> // needed for using LCD display
#include <EEPROM.h> // needed for reading & writing to eeprom (saving parameters onto arduino)
#include <Wire.h> // needed for using scl & sda pins on the real time clock
#include "RTClib.h" // for using the real time clock
RTC_DS3231 rtc; // our real time clock (rtc) is the DS3231

// temperature sensor libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 26
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
//---- end of necessary libraries --------------------------------------------------------------
//*
//*
//*
//*
// ---- variables that will not change ---------------------------------------------------------

// soil moisture sensors
const int VH400 = 0; // analog 0 pin, A0
const int EC5 = 1; // analog 1 pin, A1

// water level sensor
const int waterLevelSensor = 34;
const int waterLevelLED = 35;

// pump circulator
const int pumpCirculator = 40;

// tank valve
const int tankValve = 41;

// drain valve
const int drainValve = 42;

// user interface pins (no coding necessary for reset button)
const int buttonPin_A_Y = 5; // addition and yes button
const int buttonPin_S_N = 4; // subtraction and no button
const int buttonPin_next = 3; // next button
const int buttonPin_mode = 2; // mode button

// lcd variables
const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8; // used to be d4 = 6, d5 = 5, d6 = 4, d7 = 3; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// ---- end of variables that will not change---------------------------------------------------
//*
//*
//*
//*
// ---- variables that will change -------------------------------------------------------------

// parameters
int VWC_Upper; // VWC upper limit
int backup_VWC_Upper; // back up variable
int VWC_Lower; // VWC lower limit
int backup_VWC_Lower; // back up variable
int tempUpperLimit; // temp upper limit
int backup_tempUpperLimit; // back up variable
int tempLowerLimit; // temp lower limit
int backup_tempLowerLimit; // back up variable

// button variables (current state and last state useful for button code)
int buttonState_A_Y = 0; // addition and yes button control
int lastButtonState_A_Y = 0; // ''
int buttonState_S_N = 0; // subtraction and no button control
int lastButtonState_S_N = 0; // ''
int buttonState_next = 0; // next button control
int lastButtonState_next = 0; // ''
int buttonState_mode = 0; // mode button control
int lastButtonState_mode = 0; // ''

// program variables
int modeState = 0; // used for switching modes, keeping track of what mode you're in
int parameterState = 0; // used for moving on to the next parameter in parameter mode
int sensorState = 0; // used for moving on to the next sensor in sensor read mode
int autoState = 0; // used for moving onto next step in auto mode
String parameterUnit; // useful in parameter mode for display units (F, %)
int manualControlPump = 0; // useful in manual mode, keeps pump on without having to hold button down
int manualControlTank = 0; // useful in manual mode, keeps tank valve open without having to hold button down
int manualControlDrain = 0;// useful in manual mode, keeps drain valve open without having to hold button down

// ---- end of variables that will change ------------------------------------------------------
//*
//*
//*
//*
// START OF SET UP ****************************************************************************************************************************
void setup()
{ // start of set up

  // reading from eeprom-------------------------------------------------
  VWC_Upper = EEPROM.read(0);
  backup_VWC_Upper = VWC_Upper;
  VWC_Lower = EEPROM.read(1);
  backup_VWC_Lower = VWC_Lower;
  tempUpperLimit = EEPROM.read(2);
  backup_tempUpperLimit = tempUpperLimit;
  tempLowerLimit = EEPROM.read(3);
  backup_tempLowerLimit = tempLowerLimit;
  // end of reading from eeprom -----------------------------------------

  // button set up
  pinMode(buttonPin_A_Y, INPUT);
  pinMode(buttonPin_S_N, INPUT);
  pinMode(buttonPin_next, INPUT);
  pinMode(buttonPin_mode, INPUT);

  // sensors and valves
  pinMode(VH400, INPUT);
  pinMode(EC5, INPUT);
  pinMode(waterLevelSensor, INPUT);
  pinMode(waterLevelLED, OUTPUT);
  pinMode(pumpCirculator, OUTPUT);
  pinMode(tankValve, OUTPUT);
  pinMode(drainValve, OUTPUT);

  // ---- beginning code ------------------------------------------------
  lcd.begin(16,2); // telling the LCD to use 16 columns, 2 rows
  Serial.begin(9600); // starting serial 9600 baudrate
  sensors.begin(); // beginning the temperature sensor
  delay(3000); // wait 3 seconds for console opening

  // this code beings the real time clock and provides a message if it can't find it
  if ( ! rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while(1);
  }

  // if the clock loses power (take out the battery) the time is set to when sketch was compiled
  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  // ---- end of beginning code -----------------------------------------
} // end of set up
// END OF SET UP ******************************************************************************************************************************
//*
//*
//*
//*
// START OF LOOP ******************************************************************************************************************************
void loop()
{ // start of loop

  // getting the state of the mode button ---------------------------------------------------------------
  buttonState_mode = digitalRead(buttonPin_mode); // button state can be HIGH (pushed down) or LOW (not being pushed)
  if (buttonState_mode != lastButtonState_mode) // if you don't push mode, LOW != LOW (false), if you do push mode, HIGH != LOW (true)
  {
    if (buttonState_mode == HIGH && modeState < 4 && modeState != 5) // if mode button is pushed and modeState < 4
    {
      modeState++; // add one to modeState (switch mode)
    }
    delay(100); // small delay so mode switching isn't too fast
  }
  lastButtonState_mode = buttonState_mode; // keeping track of buttons state
  // end of getting the state of the mode button --------------------------------------------------------

  // !!!! STANDBY MODE !!!! *******************************************************************************************************************
  if (modeState == 0)
  {
    DateTime currentTime = rtc.now();
    if (currentTime.second() == 0 && currentTime.minute() == 0)
    {
      lcd.setCursor(0,0);
      lcd.print("SWITCHING TO    ");
      lcd.setCursor(0,1);
      lcd.print("AUTO MODE...    ");
      delay(2500);
      autoState = 0;
      modeState = 5;
    }
    
    else
    {
      lcd.setCursor(0,0);
      lcd.print("STANDBY MODE    ");
      lcd.setCursor(0,1);
      lcd.print("                ");
      
    }
    Serial.print(currentTime.hour());
    Serial.print(":");
    Serial.print(currentTime.minute());
    Serial.print(":");
    Serial.print(currentTime.second());
    Serial.println();
  }
  // !!!! END OF STANDBY MODE !!!! ************************************************************************************************************
  //*
  //*
  //*
  //*
  // !!!! PARAMETER MODE !!!! *****************************************************************************************************************
  else if (modeState == 1)
  { // start of parameter mode
    
    // closing both valves and turning off the pump circulator
    digitalWrite(pumpCirculator, LOW);
    digitalWrite(tankValve, LOW);
    digitalWrite(drainValve, LOW);

    // parameter screen control ---------------------------------------------
    buttonState_next = digitalRead(buttonPin_next);
    buttonState_next = digitalRead(buttonPin_next);
    if ( buttonState_next != lastButtonState_next)
    {
      if (buttonState_next == HIGH && parameterState < 8)
      {
        parameterState++;
      }
      delay(100);
    }
    lastButtonState_next = buttonState_next;
    // end of parameter screen control --------------------------------------
    //*
    //*
    // SCREEN 0
    // Parameter mode start screen ----------------------------------/
    if ( parameterState == 0 )
    {
      lcd.setCursor(0, 0);
      lcd.print("PARAMETER MODE  ");
      lcd.setCursor(0, 1);
      lcd.print("PRESS NEXT     ");
    }
    // end of parameter mode start screen --------------------------/
    //*
    //*
    // SCREEN 1
    // Setting Upper VWC -------------------------------------------------------------------------***
    else if (parameterState == 1)
    {
      // displaying parameter name
      lcd.setCursor(0, 0);
      lcd.print("UPPER LIMIT VWC ");

      // Changing VWC_Upper -----------------------------------------

      // Add button
      VWC_Upper = AddFunction(VWC_Upper);

      //Subtract button
      VWC_Upper = SubtractFunction(VWC_Upper);

      // Making sure VWC_Upper is in the range of 1 - 100
      if (VWC_Upper > 100)
      {
        VWC_Upper = 100;
      }

      else if (VWC_Upper < 1)
      {
        VWC_Upper = 1;
      }
      // End of changing VWC_Upper ----------------------------------


      // displaying VWC_Upper on LCD
      parameterUnit = " %  ";
      DisplayParameter(VWC_Upper, parameterUnit);
    }
    // End of setting VWC_Upper -------------------------------------------------------------------***
    //*
    //*
    // SCREEN 2
    // Setting VWC_Lower --------------------------------------------------------------------------///
    else if (parameterState == 2)
    {
      // displaying parameter name
      lcd.setCursor(0, 0);
      lcd.print("LOWER LIMIT VWC ");

      // Changing the parameter
      VWC_Lower = AddFunction(VWC_Lower);
      VWC_Lower = SubtractFunction(VWC_Lower);

      // VWC_lower must be lower than VWC_upper but cannot go below 0
      if (VWC_Lower >= VWC_Upper)
      {
        VWC_Lower = VWC_Upper - 1;
      }

      else if (VWC_Lower < 0 )
      {
        VWC_Lower = 0;
      }

      //Displaying on LCD
      DisplayParameter(VWC_Lower, parameterUnit);
    }
    // End of setting VWC_Lower -------------------------------------------------------------------///
    //*
    //*
    // SCREEN 3
    // Setting Temp Upper Limit -------------------------------------------------------------------***
    else if (parameterState == 3)
    {
      // displaying parameter name
      lcd.setCursor(0, 0);
      lcd.print("TEMP UPPER LIMIT");

      // Changing the parameter
      tempUpperLimit = AddFunction(tempUpperLimit);
      tempUpperLimit = SubtractFunction(tempUpperLimit);

      // Temp upper limit must be between 40 F and 110 F (environmental center approved)
      if ( tempUpperLimit > 110 )
      {
        tempUpperLimit = 110;
      }
      else if ( tempUpperLimit <= 40 )
      {
        tempUpperLimit = 41;
      }

      // displaying on lcd
      parameterUnit = " F  ";
      DisplayParameter(tempUpperLimit, parameterUnit);
    }
    // end of setting tempUpperLimit --------------------------------------------------------------***
    //*
    //*
    // SCREEN 4
    // setting tempLowerLimit ---------------------------------------------------------------------///
    else if (parameterState == 4)
    {
      // displaying the parameter
      lcd.setCursor(0, 0);
      lcd.print("TEMP LOWER LIMIT");

      // changing the parameter
      tempLowerLimit = AddFunction(tempLowerLimit);
      tempLowerLimit = SubtractFunction(tempLowerLimit);

      if (tempLowerLimit >= tempUpperLimit)
      {
        tempLowerLimit = tempUpperLimit - 1;
      }
      else if (tempLowerLimit < 40)
      {
        tempLowerLimit = 40;
      }
      //Displaying on LCD
      DisplayParameter(tempLowerLimit, parameterUnit);
    }
    // end of settting tempLowerLimit -------------------------------------------------------------///
    //*
    //*
    // SCREEN 5
    // displaying changed parameters -------------------------------------------------------------***
    else if (parameterState == 5)
    {
      DisplayAllParameters(); // calling display all parameters function
    }
    // end of displaying all parameters ----------------------------------------------------------***
    //*
    //*
    // SCREEN 6
    // asking to save -----------------------------------------------///
    else if (parameterState == 6)
    {
      // display message "Save Parameters"
      lcd.setCursor(0, 0);
      lcd.print("SAVE PARAMETERS?     ");
      lcd.setCursor(0, 1);
      lcd.print("YES OR NO?           ");

      // if "Yes" move on
      if (digitalRead(buttonPin_A_Y) == HIGH)
      {
        parameterState = 7;
      }

      // if "No" send back to reset parameters
      if (digitalRead(buttonPin_S_N) == HIGH)
      {
        VWC_Upper = backup_VWC_Upper;
        VWC_Lower = backup_VWC_Lower;
        tempUpperLimit = backup_tempUpperLimit;
        tempLowerLimit = backup_tempLowerLimit;
        parameterState = 0;
      }
    }
    // end of asking to save ----------------------------------------///

    // saving to eeprom -----------------------------------------------
    else if (parameterState == 7)
    {
      EEPROM.write(0, VWC_Upper);
      backup_VWC_Upper = VWC_Upper;
      EEPROM.write(1, VWC_Lower);
      backup_VWC_Lower = VWC_Lower;
      EEPROM.write(2, tempUpperLimit);
      backup_tempUpperLimit = tempUpperLimit;
      EEPROM.write(3, tempLowerLimit);
      backup_tempLowerLimit = tempLowerLimit;
      parameterState = 8;
    }
    // end saving to eeprom ------------------------------------------

    else if (parameterState == 8)
    {
      parameterState = 0;
    }
  } // end of parameter mode
  // !!!! END OF PARAMETER MODE !!!! **********************************************************************************************************
  //*
  //*
  //*
  //*
  // !!!! MANUAL MODE !!!! ********************************************************************************************************************
  else if (modeState == 2)
  { // start of manual mode
    lcd.setCursor(0, 0);
    lcd.print("MANUAL MODE     ");
    lcd.setCursor(0, 1);
    lcd.print("                ");

    digitalWrite(pumpCirculator, LOW);
    digitalWrite(tankValve, LOW);
    digitalWrite(drainValve, LOW);


    if (manualControlPump == 0)
    {
      digitalWrite(pumpCirculator, LOW);
    }
    else if (manualControlPump == 1)
    {
      digitalWrite(pumpCirculator, HIGH);
    }

    if (manualControlTank == 0)
    {
      digitalWrite(tankValve, LOW);
    }
    else if (manualControlTank == 1)
    {
      digitalWrite(tankValve, HIGH);
    }

    if (manualControlDrain == 0)
    {
      digitalWrite(drainValve, LOW);
    }
    else if (manualControlDrain == 1)
    {
      digitalWrite(drainValve, HIGH);
    }

    // Pump circulator control
    buttonState_A_Y = digitalRead(buttonPin_A_Y);
    if (buttonState_A_Y == HIGH && lastButtonState_A_Y == LOW)
    {
      digitalWrite(pumpCirculator, HIGH);
      delay(300);
      manualControlPump = 1;
      lastButtonState_A_Y = buttonState_A_Y;
    }
    else if (buttonState_A_Y == HIGH && lastButtonState_A_Y == HIGH)
    {
      digitalWrite(pumpCirculator, LOW);
      delay(300);
      manualControlPump = 0;
      lastButtonState_A_Y = LOW;
    }

    // tank valve control
    buttonState_S_N = digitalRead(buttonPin_S_N);
    if (buttonState_S_N == HIGH && lastButtonState_S_N == LOW)
    {
      digitalWrite(tankValve, HIGH);
      delay(300);
      manualControlTank = 1;
      lastButtonState_S_N = buttonState_S_N;
    }
    else if (buttonState_S_N == HIGH && lastButtonState_S_N == HIGH)
    {
      digitalWrite(tankValve, LOW);
      delay(300);
      Serial.println("Drain Valve Open");
      digitalWrite(drainValve, HIGH);
      delay(60000);
      Serial.println("Drain valve closed");
      digitalWrite(drainValve, LOW);
      manualControlTank = 0;
      lastButtonState_S_N = LOW;
    }

    // drain valve control
    buttonState_next = digitalRead(buttonPin_next);
    if (buttonState_next == HIGH && lastButtonState_next == LOW)
    {
      digitalWrite(drainValve, HIGH);
      delay(300);
      manualControlDrain = 1;
      lastButtonState_next = buttonState_next;
    }
    else if (buttonState_next == HIGH && lastButtonState_next == HIGH)
    {
      digitalWrite(drainValve, LOW);
      delay(300);
      manualControlDrain = 0;
      lastButtonState_next = LOW;
    }

  } // end of manual mode
  // !!!! END OF MANUAL MODE !!!! *************************************************************************************************************
  //*
  //*
  //*
  //*
  // !!!! SENSOR READ MODE !!!! ***************************************************************************************************************
  else if (modeState == 3)
  { // start of sensor read mode

    // turn off pump and valves first
    digitalWrite(pumpCirculator, LOW);
    digitalWrite(tankValve, LOW);
    digitalWrite(drainValve, LOW);

    // Sensor Readings Screen Control ---------------------------------------------------
    buttonState_next = digitalRead(buttonPin_next);
    if ( buttonState_next != lastButtonState_next)
    {
      if (buttonState_next == HIGH && sensorState < 4)
      {
        sensorState++;
      }
      delay(100);
    }
    lastButtonState_next = buttonState_next;
    // End of Sensor Readings Control ----------------------------------------------------

    // Sensor Read Mode start screen ----------------------------------------
    if (sensorState == 0)
    {
      lcd.setCursor(0, 0);
      lcd.print("SENSOR READ MODE");
      lcd.setCursor(0, 1);
      lcd.print("PRESS NEXT      ");
    }
    // end start screen -----------------------------------------------------

    // Displays water lvl sensor and testings the water level sensor --------------------
    if (sensorState == 1)
    {
      lcd.setCursor(0, 0);
      lcd.print("WATER LEVEL     ");
      lcd.setCursor(0, 1);
      lcd.print("SENSOR          ");

      // Reading the Water Level Sensor, if liquid touches it the sensor will read OV (LOW)

      int isDry = digitalRead(waterLevelSensor);
      if ( isDry == HIGH )
      {
        digitalWrite(waterLevelLED, HIGH);
      }
      else if ( isDry == LOW )
      {
        digitalWrite(waterLevelLED, LOW);
      }
    }
    // End of Displays water lvl sensor and testings the water level sensor ------------

    // Displays "Temperature Sensor" and temperature sensor readouts (F) on screen -----------------------
    if (sensorState == 2)
    {
      digitalWrite(waterLevelLED, LOW); // turns off the water level LED

      lcd.setCursor(0, 0);
      lcd.print("TEMPERATURE     ");
      lcd.setCursor(0, 1);
      lcd.print("SENSOR ");

      //reading the water temperature
      sensors.requestTemperatures();
      int tempNow_C = sensors.getTempCByIndex(0);
      delay(10);
      float tempNow_F = ( tempNow_C * 1.8 ) + 32;
      int tempNow_F_rounded = tempNow_F;

      lcd.setCursor(7, 1);
      lcd.print(tempNow_F_rounded);
      lcd.setCursor(11, 1);
      lcd.print("F");
    }
    // End of Displays "Temperature Sens" and temperature sensor readouts (F) on screen ----------------


    // Displays Moisture Sensor and moisture sensor readouts (%) on screen -----------------------------
    if (sensorState == 3)
    {
      lcd.setCursor(0, 0);
      lcd.print("MOISTURE LEVELS ");
      lcd.setCursor(0, 1);

      //reading the moisutre sensor values
      // This is the VH400
      float VH400_integer_reading = analogRead(VH400) * 0.0049;
      float VH400_slope = 0.2007;
      float VH400_intercept = 0.0307;
      float VH400_readings = VH400_slope * VH400_integer_reading + VH400_intercept;

      // VH400 readings in percentage
      float VH400_readings_percentage = VH400_readings * 100;
      int VH400_readings_rounded = VH400_readings_percentage;

      // This is the EC5
      float EC5_integer_reading = analogRead(EC5) * 0.0049;
      float EC5_slope = 0.9932;
      float EC5_intercept = -0.3786;
      float EC5_readings = EC5_slope * EC5_integer_reading + EC5_intercept;

      // EC-5 readings in percentage
      float EC5_readings_percentage = EC5_readings * 100;
      int EC5_readings_rounded = EC5_readings_percentage;
      delay(10);

      // Displays both moisture sensor readings on the screen
      // VH400
      lcd.setCursor(0, 1);
      lcd.print("S1:");
      if (VH400_readings_rounded < 10)
      {
        lcd.setCursor(3, 1);
        lcd.print("  ");
        lcd.setCursor(5, 1);
        lcd.print(VH400_readings_rounded);
      }
      else if (VH400_readings_rounded < 100)
      {
        lcd.setCursor(3, 1);
        lcd.print(" ");
        lcd.setCursor(4, 1);
        lcd.print(VH400_readings_rounded);
      }
      else
      {
        lcd.setCursor(3, 1);
        lcd.print(VH400_readings_rounded);
      }

      // EC5
      lcd.setCursor(6, 1);
      lcd.print("    S2:");
      if (EC5_readings_rounded < 10)
      {
        lcd.setCursor(13, 1);
        lcd.print("  ");
        lcd.setCursor(15, 1);
        lcd.print(EC5_readings_rounded);
      }
      else if (EC5_readings_rounded < 100)
      {
        lcd.setCursor(13, 1);
        lcd.print(" ");
        lcd.setCursor(14, 1);
        lcd.print(EC5_readings_rounded);
      }
      else
      {
        lcd.setCursor(13, 1);
        lcd.print(EC5_readings_rounded);
      }
    }
    // End of Displays "Moisture Sensor" and moisture sensor readouts (%) on screen --------------------

    if (sensorState == 4)
    {
      digitalWrite(waterLevelLED, LOW);
      sensorState = 0; // sensorstate is reset for sensor read mode
    }
  } // end of sensor read mode
  // !!!! END OF SENSOR READ MODE !!!! ********************************************************************************************************
  //*
  //*
  //*
  //*
  // ---- switching back to standby mode so auto mode doesn't start at the wrong time
  else if (modeState == 4)
  {
    digitalWrite(waterLevelLED, LOW); // turns off the water level LED
    parameterState = 0;
    sensorState = 0;
    manualControlPump = 0;
    manualControlTank = 0;
    manualControlDrain = 0;
    modeState = 0;
  }
  // ---- end of switching back to standby mode
  //*
  //*
  //*
  //*
  // !!!! AUTO MODE !!!! **********************************************************************************************************************
  else if (modeState == 5)
  {
    Serial.println("Auto mode started successfully!...");
    lcd.setCursor(0, 0);
    lcd.print("AUTO MODE       ");
    lcd.setCursor(0, 1);
    lcd.print("                ");

    Serial.println("Reading the sensors now");

    // checking the water level
    if ( autoState == 0 )
    {
      Serial.println(" ** Checking the water level...");
      // reading the water level sensor, if liquid touches it the sensor will read 0V (LOW)
      int isDry = digitalRead(waterLevelSensor);
      if (isDry == HIGH) // if there is no liquid, it will read HIGH
      {
        digitalWrite(waterLevelLED, HIGH); // turn on orange LED indicating tank should be refilled
        Serial.println("  The tank must be refilled...");
        Serial.println("  Checking again in an hour...");

        modeState = 0; // go back to standby mode, auto mode will run again in another hour
      }

      else if (isDry == LOW) // if there is liquid, it will read LOW, water level is ok
      {
        Serial.println("  The water level is good!...");
        digitalWrite(waterLevelLED, LOW); // refill light is off
        autoState = 1; // check the water temperature now
      }
    }
    // end of checking the water level
    //*
    //*
    //*
    // reading the water temperature
    else if ( autoState == 1)
    {
      Serial.println(" ** Reading the temperature now...");
      sensors.requestTemperatures(); // send command to get temperatures
      int tempNow_C = sensors.getTempCByIndex(0);
      delay(10);

      float tempLowerLimit_C = ( tempLowerLimit - 32 ) * 0.5556; // converting tempLowerLimit (F) to Celsius
      float tempUpperLimit_C = ( tempUpperLimit - 32 ) * 0.5556; // converting tempUpperLimit (F) to Celsius

      if ( tempNow_C <= tempLowerLimit_C ) // if the current temperature is below the tempLowerLimit_C turn on the pump circulator
      {
        Serial.println("  The water is too cold, turning on the pump circulator...");
        digitalWrite(pumpCirculator, HIGH); // this turns on the pump circulator and the LED
        delay(5000); // five second delay
        digitalWrite(pumpCirculator, LOW); // turns them off after five seconds
        Serial.println("  The pump circulator is off, checking again in an hour...");

        modeState = 0;
      }

      else if ( tempNow_C >= tempUpperLimit_C ) // if the current temperature is above the upper limit wait another hour and try again
      {
        Serial.println("  The water is too hot, will check again in an hour...");
        modeState = 0;
      }

      else
      {
        autoState = 2;
        Serial.println("  The water temperature is good! Checking the soil moisture sensors now!");
      }
    }
    // end of reading the water temperature
    //*
    //*
    //*
    // reading the soil moisture sensors now
    else if (autoState == 2)
    {
      Serial.println(" ** Reading the moisture sensor values now...");
      // This is the VH400
      float VH400_integer_reading = analogRead(VH400) * 0.0049;
      float VH400_slope = 0.2007;
      float VH400_intercept = 0.0307;
      float VH400_readings = VH400_slope * VH400_integer_reading + VH400_intercept;
      Serial.println("  VH400 complete...");

      // This is the EC5
      float EC5_integer_reading = analogRead(EC5) * 0.0049;
      float EC5_slope = 0.9932;
      float EC5_intercept = -0.3786;
      float EC5_readings = EC5_slope * EC5_integer_reading + EC5_intercept;
      Serial.println("  EC5 complete...");

      // averaging VH400 and EC5 readings
      float averageSoilSensorVal = (VH400_readings + EC5_readings) / 2;
      Serial.println("  The average VWC is");
      Serial.println(averageSoilSensorVal * 100);

      delay(10);

      // checking the VWC in the soil
      if ( averageSoilSensorVal * 100 <= VWC_Lower ) // is the VWC of the soil is less then the lower limit VWC, water the plants (refill)
      {
        Serial.println("  The VWC of the soil is low...");
        // open valves
        Serial.println("  Irrigation valve opened...");
        digitalWrite(tankValve, HIGH);
        autoState = 3;
      }

      else if ( averageSoilSensorVal * 100 > VWC_Lower && averageSoilSensorVal * 100 < VWC_Upper ) // VWC in green zone (good)
      {
        Serial.println("  The VWC of the soil is good! Checking again in an hour...");
        modeState = 0; // go back to standby mode, auto mode will run in an hour again
      }

      else if ( averageSoilSensorVal * 100 >= VWC_Upper ) // VWC is above the upper VWC limit (too full)
      {
        Serial.println("  The VWC of the soil is high. Checking again in an hour...");
        modeState = 0; // go back to standby mode, auto mode will run in an hour again
      }
    }
    // end of reading the soil moisture sensors
    //*
    //*
    //*
    // closing the valve
    else if ( autoState == 3 )
    {
      // This is the VH400
      float VH400_integer_reading = analogRead(VH400) * 0.0049; // 0.0049 conversion factor
      float VH400_slope = 0.2007; // VH400 3.3V calibration curve slope
      float VH400_intercept = 0.0307; // VH400 3.3V calibration curve intercept
      float VH400_readings = VH400_slope * VH400_integer_reading + VH400_intercept;

      // This is the EC5
      float EC5_integer_reading = analogRead(EC5) * 0.0049;
      float EC5_slope = 0.9932;
      float EC5_intercept = -0.3786;
      float EC5_readings = EC5_slope * EC5_integer_reading + EC5_intercept;

      // averaging VH400 and EC5 readings
      float averageSoilSensorVal = (VH400_readings + EC5_readings) / 2;

      if ( averageSoilSensorVal * 100 >= VWC_Upper )
      {
        Serial.println("tank valve closed");
        digitalWrite(tankValve, LOW);
        autoState = 4;
      }
    }

    if ( autoState == 4 )
    {
      Serial.println("Drain Valve Open");
      digitalWrite(drainValve, HIGH);
      delay(60000);
      Serial.println("Drain valve closed");
      digitalWrite(drainValve, LOW);

      modeState = 0;
    }
  }
  // !!!! END OF AUTO MODE !!!! ***************************************************************************************************************
  
} // end of loop
// END OF LOOP ********************************************************************************************************************************

// EXTRA FUNCTIONS ***********************************************************

int AddFunction (int a) // this functions increments some integer by one (increases a parameter by one every button press)
{
  buttonState_A_Y = digitalRead(buttonPin_A_Y); // get the button state of the add button
  if (buttonState_A_Y != lastButtonState_A_Y) // if it is HIGH (pushed) and last time it was LOW (not pushed) or vice versa, run next check
  {
    if (buttonState_A_Y == HIGH) // if it is HIGH (pushed)
    {
      a++; // add one to the integer (parameter you're increasing)
    }
    delay(100); 
  }
  lastButtonState_A_Y = buttonState_A_Y; // keeping track of button states
  return a; // return the new integer (parameter)
}

int SubtractFunction (int a) // this function decrements some integer by one (decreases a parameter by one every button press)
{
  buttonState_S_N = digitalRead(buttonPin_S_N); // getting the button state of the subtract button
  if (buttonState_S_N != lastButtonState_S_N) // if it is HIGH (pushed) and last time it was LOW (not pushed) or vice versa, run next check
  {
    if (buttonState_S_N == HIGH) // if it is HIGH (pushed)
    {
      a--; // subtract one to the integer (parameter you're decreasing)
    }
    delay(100);
  }
  lastButtonState_S_N = buttonState_S_N; // keeping track of button states
  return a; // return the new integer (parameter)
}

void DisplayParameter (int x, String u) // this function displays some parameter on the change parameter screen with its units
{
  if (x < 10) // if the number is one digit it displays it like this: "  x units"
  {
    lcd.setCursor(0, 1); // columnn 0 row 1 is the lower left hand corner
    lcd.print(" "); // first space
    lcd.setCursor(1, 1); // move one column over
    lcd.print(" "); // second space
    lcd.setCursor(2, 1); // move one column over
    lcd.print(x); // one digit parameter
    lcd.setCursor(3, 1); // move one column over
    lcd.print(u); // unit
  }

  else if (x < 100) // if the number is two digits it displays it like this: " xx units"
  {
    lcd.setCursor(0, 1); // start on lower left hand corner
    lcd.print(" "); // one space
    lcd.setCursor(1, 1); // move one column over
    lcd.print(x); // two digit parameter
    lcd.setCursor(3, 1); // move one column over
    lcd.print(u); // unit
  }

  else // if the number is three digits it displays it like this: "xxx units" none of the parameters can be above three digits
  {
    lcd.setCursor(0, 1); // start on lower left hand corner
    lcd.print(x); // print out three digit parameter
    lcd.setCursor(3, 1); // move one column over
    lcd.print(u); // print the unit
  }

  lcd.setCursor(5, 1); // this just erases whatever extra characters may be on the screen.
  lcd.print("          ");
}

void DisplayAllParameters() // this function displays all parameters on one screen, since cursor changes are necessary and the parameter
{                           // names are shortend to "VU" etc. it is simpler to code everything like this instead of heavily modifying the
  // VWC_Upper display      // previous function
  if ( VWC_Upper < 10 )
  {
    lcd.setCursor(0, 0);
    lcd.print("VU:");
    lcd.setCursor(3, 0);
    lcd.print("00");
    lcd.setCursor(5, 0);
    lcd.print(VWC_Upper);
  }

  else if ( VWC_Upper < 100 )
  {
    lcd.setCursor(0, 0);
    lcd.print("VU:");
    lcd.setCursor(3, 0);
    lcd.print("0");
    lcd.setCursor(4, 0);
    lcd.print(VWC_Upper);
  }

  else
  {
    lcd.setCursor(0, 0);
    lcd.print("VU:");
    lcd.setCursor(3, 0);
    lcd.print(VWC_Upper);
  }
  lcd.setCursor(6, 0);
  lcd.print("% ");

  // VWC_Lower display
  if ( VWC_Lower < 10 )
  {
    lcd.setCursor(7, 0);
    lcd.print(" VL:");
    lcd.setCursor(11, 0);
    lcd.print("00");
    lcd.setCursor(13, 0);
    lcd.print(VWC_Lower);
  }

  else if ( VWC_Lower < 100 )
  {
    lcd.setCursor(7, 0);
    lcd.print(" VL:");
    lcd.setCursor(11, 0);
    lcd.print("0");
    lcd.setCursor(12, 0);
    lcd.print(VWC_Lower);
  }

  else
  {
    lcd.setCursor(7, 0);
    lcd.print(" VL:");
    lcd.setCursor(11, 0);
    lcd.print(VWC_Lower);
  }
  lcd.setCursor(14, 0);
  lcd.print("%  ");

  // tempUpperLimit display
  if ( tempUpperLimit < 10 )
  {
    lcd.setCursor(0, 1);
    lcd.print("TU:");
    lcd.setCursor(3, 1);
    lcd.print("00");
    lcd.setCursor(5, 1);
    lcd.print(tempUpperLimit);
  }

  else if ( tempUpperLimit < 100 )
  {
    lcd.setCursor(0, 1);
    lcd.print("TU:");
    lcd.setCursor(3, 1);
    lcd.print("0");
    lcd.setCursor(4, 1);
    lcd.print(tempUpperLimit);
  }

  else
  {
    lcd.setCursor(0, 1);
    lcd.print("TU:");
    lcd.setCursor(3, 1);
    lcd.print(tempUpperLimit);
  }
  lcd.setCursor(6, 1);
  lcd.print("F ");

  // tempLowerLimit display
  if ( tempLowerLimit < 10 )
  {
    lcd.setCursor(7, 1);
    lcd.print(" TL:");
    lcd.setCursor(11, 1);
    lcd.print("00");
    lcd.setCursor(13, 1);
    lcd.print(tempLowerLimit);
  }

  else if ( tempLowerLimit < 100 )
  {
    lcd.setCursor(7, 1);
    lcd.print(" TL:");
    lcd.setCursor(11, 1);
    lcd.print("0");
    lcd.setCursor(12, 1);
    lcd.print(tempLowerLimit);
  }

  else
  {
    lcd.setCursor(7, 1);
    lcd.print(" TL:");
    lcd.setCursor(11, 1);
    lcd.print(tempLowerLimit);
  }
  lcd.setCursor(14, 1);
  lcd.print("F  ");
}
// END OF EXTRA FUNCTIONS ****************************************************
