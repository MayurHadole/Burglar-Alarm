/*  FILE          : BurglarAlarmProject.ino
 *  PROJECT       : PROG8125-16S - Project
 *  PROGRAMMER    : Mayur Hadole (5783)
 *  FIRST VERSION : 2016-07-31
 *  DESCRIPTION   :
 *        Project Statement (No. 2)
 *    Burglar alarm that monitors 5 zones. Zones are monitored by a loop of wire.
 *    Circuitry is needed that can detect if the loop has been broken (an Alarm Condition)
 *    or shorted (a fault condition).  The alarm should accept a password from the serial port
 *    in order to enable or disable the entire alarm or individual zones. User output should
 *    be done using a speaker and LCD.
 *
 *    I have used the state machine to make the burglar alarm with above funtions and some extra functions
 *
 *   State 1:PASSWORD AUTHENTICATION
 *         Shows the welcome message on LCD and then asks for password to enter using serial port.If
 *         the password is correct the program moves to the next state else user gets two more tries to
 *         get the password right.System is blocked if all three attempts are used.
 *           
 *   State 2:INITIALIZING SYSTEM
 *         High Signal is written on all five loops through loopIn pins. These High Signals will be used to
 *         moniter the loop break, faults and normal status of loops.
 *
 *   State 3:ZONE SELECTION
 *         In this State the program asks the user using LCD screen to enter the numbers of zones that needs to be monitered.
 *         If the user enteres "all" on Serial port then all zones are saved in a array.
 *         And If the user enters the names of zones(from 1 to 5) that needs to be monitered with commas in between then those
 *         zones are saved in the array. 
 *        
 *   State 4:ARMED
 *         In this state, all the loops are checked by reading inputs from loop out pins. Depending upon their ADC readings
 *         their status is determined. Following is the range of ADC values for all three states.
 *    
 *         ADC Value        Status
 *         255              Normal
 *         1                Fault
 *         2 to 254         Loop Break (intruder alert)
 *
 *         In this same state, if all the loops are in normal state . All Zones Safe message is displayed on LCD Screen and Serial port
 *         Along with the thumbs up symbol. 
 *       
 *   State 5: BREACH DETECTED
 *         In this State, the status of zones which were selected in previous state are stored in the array.(one array for fault zones and
 *         another one for loop break zones).
 *         These arrays are used to call functions to display the status on LCD and to play the alarm using speaker.
 *         If zones are in loop break state then intruder alert message is displayed on the screen along with the "running person" symbol.
 *         And if the zones are in fault state then fault message is displayed on the screen with the bitmapped fault syambol. 
 *         If some zones are in fault state  and some are in loop break state then both fault and intruder alert messages are displayed on
 *         LCD screen with the gap of 1 second in between them.
 *      
 *   State 6. DISABLE BURGLAR ALARM
 *         This is the state in which the program resets using external interrupt connected to push button.
 *         if the push button is pressed at any time during the execution of program the system will reset to state 1.
 *
 *   
 *   Extra Features
 *      1. Three bitmapped Symbols are used to display on lcd for three different states.
 *              State           Symbol
 *              Loop Break      Running Man
 *              Fault           Exclamation symbol in the square
 *              Normal          Thumbs up symbol 
 *              
 *      2. External interrupt is used to reset the system to state 1.
 *      3. System is blocked if more than three times the entered password is wrong
 *      4. millis() is used to generate the alarm sound 
 *       
 *         This code is tested on Teensy 3.1 board
 */


#include <LiquidCrystal.h>                          //Includes library files for LCD
/* LCD RS pin to digital pin 12
   LCD Enable pin to digital pin 11
   LCD D4 pin to digital pin 14
   LCD D5 pin to digital pin 15
   LCD D6 pin to digital pin 16
   LCD D7 pin to digital pin 17
   LCD R/W pin to ground
   LCD VSS pin to ground
   LCD VCC pin to 5V */
LiquidCrystal lcd(12, 11, 14, 15, 16, 17);          // initialize the library with the numbers of the interface pins

// Folowing are the various states of the program
#define PASSWORDAUTHENTICATION 1
#define INITIALIZINGSYSTEM 2
#define ZONESELECTION 3
#define ARMED 4
#define BREACHDETECTED 5
#define DISABLEBURGLARALARM 6

#define NumberOfZones  5    //number of zones to moniter

#define loop1In 2                   //startpoint of loop 1
const uint8_t loop2In = 4;          //startpoint of loop 2
const uint8_t loop3In = 5;          //startpoint of loop 3
const uint8_t loop4In = 6;          //startpoint of loop 4
const uint8_t loop5In = 9;          //startpoint of loop 5
const uint8_t loop1Out = 9;         //Endpoint of loop 1
const uint8_t loop2Out = 8;         //Endpoint of loop 2
const uint8_t loop3Out = 7;         //Endpoint of loop 3
const uint8_t loop4Out = 6;         //Endpoint of loop 4
const uint8_t loop5Out = 5;         //Endpoint of loop 5

const uint8_t disablePin = 13;          // Disable pushButton is conected to pin 1
const uint8_t speakerPin = 10;          //speaker is connected to pin 10
const uint16_t frequencyOfTone = 2000;  //Frequency of the alarm sound

const uint8_t allAreSelected = 1;       //all the zones are selected
const uint8_t someAreSelected = 2;      //not all zones are selected
const uint8_t ASCIItoINTconstant = 48;  // to convert the datatype from char to int


const uint8_t normalStateValue = 255;   // loop is not broken, Normal condition
const uint8_t faultStateValue = 1;      // loop is connected to the ground , Fault condition
const uint8_t normalState = 1;          // constanst to represent normal state
const uint8_t faultState = 2;           // constanst to represent fault state
const uint8_t loopBreakState = 3;       // constanst to represent loop break state( intruder alert)
const uint8_t breachDetected = 1;       // constant to represent fault or loop break state

const uint8_t defaultCharArrayLength = 20;   // default length of serial input data for zone selection



const uint8_t correctPassword = 1;    // constant to represent correct password is entered
const uint8_t incorrectPassword = 2;  // constant to represent incorrect password is entered

const uint16_t delayOf2Seconds = 2000;  
const uint16_t delayOf1Second = 1000;
const uint16_t SerialBaudRate = 9600;

const uint8_t NumberOfColumnsInLCD = 2;  //Number Of Columns In LCD are 2
const uint8_t NumberOfRowsInLCD = 16;    //Number Of Rows In LCD are 16
const uint8_t firstRowOfLCD = 0;         //first row on LCD
const uint8_t secondRowOfLCD = 1;        //second row on LCD
const uint8_t firstColumnOfLCD = 0;      //first Column of LCD
const uint8_t secondColumnOfLCD = 1;     //second Column of LCD
const uint8_t thirdColumnOfLCD = 2;      //third Column of LCD
const uint8_t fifthColumnOfLCD = 4;      //fifth Column of LCD
const uint8_t fourthColumnOfLCD = 3;     // fourth column of LCD

const uint8_t bitMapByteArraySize = 8 ;  // array size for the byte array used to print symbols on LCD



static uint8_t burglarAlarmState = PASSWORDAUTHENTICATION ;          // this is the variable which will control the state machine
// this variable is made global because I want to change the state of state machine using interrupt handler.
// it is initialized to the very first state "PASSWORDAUTHENTICATION"



//prototypes of the functions used in this program
void displayTitle();
uint8_t getAndCheckPassword();
uint8_t *zonesSelection();
uint8_t *stringSeparation( char inputCharArray[] , uint8_t inputCharArrayLength );
void initializingZones();
void displayArmed();
uint8_t *checkZones();
void displayAllZonesNormal();
void displayAlarmZones( uint8_t breakZones[ NumberOfZones ], uint8_t selectedZones[] );
void displayFaultZones( uint8_t faultZones[ NumberOfZones ], uint8_t selectedZones[] );
uint8_t zonesCount(uint8_t *selectedZones);
void disableISR(void);
void displayThumbsUP();
void displayIntruder();
void displayFaultSymbol();
void alarmTone();
void displayLeftAttemptsMessage( uint8_t passwordAttempts );
void displayBlocked();

void setup()
{
  pinMode(loop1In , OUTPUT);  //loop1In pin is set as a output pin
  pinMode(loop2In , OUTPUT);  //loop2In pin is set as a output pin
  pinMode(loop3In , OUTPUT);  //loop3In pin is set as a output pin
  pinMode(loop4In , OUTPUT);  //loop4In pin is set as a output pin
  pinMode(loop5In , OUTPUT);  //loop5In pin is set as a output pin
  pinMode(speakerPin , OUTPUT);                           //speaker pin is set as a output pin
  pinMode( disablePin , INPUT_PULLUP);                    //loop1In pin is set as a input pin with inter pullup resisters enabled
  Serial.begin(SerialBaudRate);                           //initializing the serial port with baud rate 9600
  delay(delayOf2Seconds);                                 // wait for serial port to be initialized
  lcd.begin (NumberOfColumnsInLCD, NumberOfRowsInLCD);                          // set up the LCD's with number of columns and rows
  attachInterrupt(digitalPinToInterrupt(disablePin), disableISR, FALLING);      // interrupt is initialized on disable pin to call disableISR routine when push button connected to disable pin is pressed.
  displayTitle();                                                               // displays title of the project on lcd and serial port

}



void loop()
{
  static uint8_t *selectedZones;  // integer pointer to get address of the integer array that has the names of selected zones.
  static uint8_t *zoneStatus;     // integer pointer to get address of the integer arrat that has the state of each zone. 
  switch (burglarAlarmState)      // state machine that checks for the variable burglarAlarmState to change the state
  {
    case PASSWORDAUTHENTICATION :                                        // State 1:PASSWORD AUTHENTICATION
                                                                         //         Shows the welcome message on LCD and then asks for password to enter using serial port.If
                                                                         //         the password is correct the program moves to the next state else user gets two more tries to
                                                                         //         get the password right.System is blocked if all three attempts are used.
      {
        static uint8_t passwordAttempts = 3;   // variable to keep track of the remaining attempts if the entered password is wrong
        uint8_t passwordStatus = 0;            // a variable to save the status if entered password is correct or not
        do                                     // do while loop is used to execute the code in the while loop 1st time unconditionally
        {                                      // do- while loop is used to let user have extra two attempts if the entered password is wrong
          passwordStatus = getAndCheckPassword();            // function is called to get the password from serial port and compare it to the valid password and return the status.
          if ( passwordStatus == correctPassword )           // if the entered password is correct then
          {
             burglarAlarmState = INITIALIZINGSYSTEM ;        // change the state to INITIALIZING SYSTEM
          }
          else if ( passwordStatus == incorrectPassword )    // if the entered password is incorrect then
          {
            passwordAttempts-- ;                             // decrementing left attempts by 1
            displayLeftAttemptsMessage(passwordAttempts);    // display left attemts message on LCD and Serial port
          }
        }while(passwordAttempts > 0 && passwordStatus != correctPassword );      // repeat the loop if password attemts are more than 1 and entered password is incorrect else exit the loop
        if(passwordAttempts == 0)               // if password attemts are are zero then
        {
          displayBlocked();                     // display "system Blocled " message on LCD and serial port
        }
        break ;
      }

    case INITIALIZINGSYSTEM :                   // State 2:INITIALIZING SYSTEM
                                                //         High Signal is written on all five loops through loopIn pins. These High Signals will be used to
                                                //         moniter the loop break, faults and normal status of loops.
      {
        initializingZones();                    //   function is called to write high on all the input pins of zones
        burglarAlarmState = ZONESELECTION ;     // change the state to ZONE SELECTION
        break;
      }

    case ZONESELECTION :                      // State 3:ZONE SELECTION
                                              //         In this State the program asks the user using LCD screen to enter the numbers of zones that needs to be monitered.
                                              //        If the user enteres "all" on Serial port then all zones are saved in a array.
                                              //        And If the user enters the names of zones(from 1 to 5) that needs to be monitered with commas in between then those
                                              //        zones are saved in the array
      {
        selectedZones = zonesSelection();    // functions returns the address of the array that has the selected zones which are obtained by parsing input string from serial port
        burglarAlarmState = ARMED ;
        break;                               // change the state to ARMED
      }
    case ARMED :                                      // State 4:  ARMED
                                                      //        In this state, all the loops are checked by reading inputs from loop out pins. Depending upon their ADC readings
                                                      //         their status is determined. Following is the range of ADC values for all three states.
                                                      //   
                                                      //        ADC Value        Status
                                                      //        255              Normal
                                                      //        1                Fault
                                                      //        2 to 254         Loop Break (intruder alert)
                                                      //
                                                      //         In this same state, if all the loops are in normal state . All Zones Safe message is displayed on LCD Screen and Serial port
                                                      //        Along with the thumbs up symbol.
      {
        static uint8_t displayArmedFlag = 1;          // flag which is used to make sure the armed message is shown only once at the start of the program. thats why its static and initialized to 1
        if (displayArmedFlag)                       
        {
          displayArmed();                             // display armed message on LCD and serial port.
        }
        displayArmedFlag = 0;                         // set the flag to zero
        zoneStatus = checkZones();                    // fuction returns the address of the array that has status of all the zones
        uint8_t normalZonesCounter = 0;               // counter to calculate number of zones whose state are normal 
        for ( uint8_t i = 0 ; i < NumberOfZones ; i++ )    // for loop to count normal zones
        {
          if ( *( zoneStatus + i) == normalState )         // if the data of the i'th index of the array pointed by the zoneStatus pointer is equal to normal state then 
          {
            normalZonesCounter++;                          // increment the counter
          }
        }
        if ( normalZonesCounter == NumberOfZones )         // if all zones are normal then
        {
          displayAllZonesNormal();                         // display "all zones normal" on the LCD and serial port
        } 
        else                                               // if all zones are not normal then
        {
          burglarAlarmState = BREACHDETECTED;              // change the state to BREACH DETECTED
        }
        break;
      }
    case BREACHDETECTED :                                  //      State 5: BREACH DETECTED
                                                           //         In this State, the status of zones which were selected in previous state are stored in the array.(one array for fault zones and
                                                           //         another one for loop break zones).
                                                           //         These arrays are used to call functions to display the status on LCD and to play the alarm using speaker.
                                                           //        If zones are in loop break state then intruder alert message is displayed on the screen along with the "running person" symbol.
                                                           //        And if the zones are in fault state then fault message is displayed on the screen with the bitmapped fault syambol. 
                                                           //        If some zones are in fault state  and some are in loop break state then both fault and intruder alert messages are displayed on
                                                           //        LCD screen with the gap of 1 second in between them.
      {
        uint8_t breakZones[NumberOfZones] = { 0 };         //array to get the names of the zones which have loop Break state. if loop 1 is in loop break state then breakZone[0] will be 1  
        uint8_t faultZones[NumberOfZones] = { 0 };         //array to get the names of the zones which have fault state.      if loop 1 is in fault state then breakZone[0] will be 1
        boolean alarm = false;                             // flag to know if any zone or none are in loop break state 
        boolean fault = false;                             // flag to know if any zone or none are in fault state
        for ( uint8_t i = 0 ; i < NumberOfZones ; i++ )    // for loop to get the status from zoneStatus (pointer that points to zoneStatus array) to the breakZones and faultZones
        {
          if ( *(zoneStatus + i) == loopBreakState )       // if the data of the i'th index of the array pointed by the zoneStatus pointer is equal to loop break  state then  
          {
            breakZones[i] = breachDetected ;               // set i index of break zone array to 1
            alarm = true;                                  // loop break state is detected so setting the alarm flag to true
          }
          if ( *(zoneStatus + i) == faultState )           // if the data of the i'th index of the array pointed by the zoneStatus pointer is equal to fault state then
          {
            faultZones[i] = breachDetected ;               // set i index of fault zone array to 1
            fault = true;                                  // fault state is detected so setting the fault flag to true
          }
        }
        if (alarm)                                        // if any of the zones has loop break state then
        {
          displayAlarmZones(breakZones, selectedZones);   // display intruder message for loop break zones for selected zones only.
        }
        if (fault)                                        // if any of the zones has fault state then 
        {
          displayFaultZones(faultZones, selectedZones);   // display fault message for loop break zones for selected zones only
        }
        if (fault || alarm)                               // if any of the zones are not in normal state then
        {
          alarmTone();                                    // sound alarm using speaker connected to pin 10
        }
        burglarAlarmState = ARMED;                        // change the state to ARMED
        break;
      }
    case DISABLEBURGLARALARM :                           // State 6. DISABLE BURGLAR ALARM
                                                         //         This is the state in which the program resets using external interrupt connected to push button.
                                                         //         if the push button is pressed at any time during the execution of program the system will reset to state 1.                   
      {
        displayDisable();                                // display "burglar alarm disabled" on serial port and LCD
        displayTitle();                                  // display title on the LCD and Serial port
        burglarAlarmState = PASSWORDAUTHENTICATION ;     // change the state to PASSWORD AUTHENTICATION
        break;
      }
  }
}


// FUNCTION     : displayTitle
// DESCRIPTION  :
//      This function prints the title of the project on the LCD screen and serial port.
// PARAMETERS   : void
// RETURNS      : void

void displayTitle()
{
  lcd.clear();
  lcd.print("BURGLAR ALARM ");              //prints on LCD
  Serial.println("Burglar Alarm");
  delay(delayOf1Second);
  lcd.clear();
}

// FUNCTION     : getAndCheckPassword
// DESCRIPTION  :
//      This function gets the wntered password from the serial port and campares it with valid password. If its correct then return 1 and if
//                it's incorrect returns 2. This function also displays required messages on LCD and Seria port to get the password and also shows 
//                if entered password correct or not on LCD and serial port.
// PARAMETERS   : void
// RETURNS      : 
//          uint8_t : the status is the entered password is correct or not
//                      1 for correct password
//                      2 for incorrect password

uint8_t getAndCheckPassword()
{
  uint16_t validPassword = 1234;                // variable to save valid password
  uint16_t enteredPassword;                     //variable to save entered password from serial port
  lcd.clear();
  Serial.println("Enter the Password");
  lcd.printf("   Enter the");
  lcd.setCursor( firstColumnOfLCD , secondRowOfLCD);
  lcd.printf("    Password");
  while ( Serial.available() == 0 ) {}            //  wait for user to enter the password
  //this while loop's condition will became false when user enters something on serial port and hits send button
  enteredPassword = Serial.parseInt();          // gets the integer data from serial port to the variable
  if ( enteredPassword == validPassword )        // if entered password is equal to valid password then
  {
    lcd.clear();
    lcd.printf("Correct Password");
    Serial.println("Password is Correct");
    delay(delayOf1Second);
    return correctPassword;                   // returns 1 if correct password is entered
  }
  else
  {
    lcd.clear();
    lcd.printf("Wrong Password");
    Serial.println("Password is Incorrect");
    delay(delayOf1Second);
    return incorrectPassword;                // returns 2 if incorrect password is entered
  }
}


// FUNCTION     : zonesSelection
// DESCRIPTION  :
//      This function asks user to enter the names of zones that are needed to moniter or "all" for all zones on serial port. then,the input string from the serial port is 
//           saved in a character array. If all zones are selected all zones are added into the integer array called selected zones and if soe of the zone are selected then other
//           function (stringSeparation())is called to parse zone nemes from the string entered on the serial port.
// PARAMETERS   : void
// RETURNS      : 
//          uint8_t* : an integer pointer that returns the address of the array "selected Zones" 

uint8_t *zonesSelection()
{
  uint8_t inputCharArrayLength;                                             // to store the length of the string entered on serial port for zone selection
  char inputCharArray[ defaultCharArrayLength ];                            // character array with default array length to store the string entered on serial port for zone selection
  uint8_t *selectedZones ;                                                  // integer pointer to point an integer array which saves which zones are selected
  selectedZones = (uint8_t *)malloc( NumberOfZones );                         // Maximum number of selected zones = 5 so, so 5 memory locations are allocated to this pointer
  Serial.println("Enter which zones you want to moniter with commas");
  Serial.println("Enter ALL for all zones");
  lcd.clear();
  lcd.setCursor( firstColumnOfLCD , firstRowOfLCD);
  lcd.printf("Enter zone No or");
  lcd.setCursor( firstColumnOfLCD , secondRowOfLCD);
  lcd.printf("ALL for all zones");
  while (Serial.available() == 0) {}                                       //  wait for user to enter the password
  //this while loop's condition will became false when user enters something on serial port and hits send button
  inputCharArrayLength = Serial.available();                               // to get the length of the entered string on serial port in an integer
  for (uint8_t i = 0 ; i < inputCharArrayLength ; i++ )                    // for loop to get each character of string in character array
  {
    inputCharArray[i] = Serial.read();                                     //get the i'th character of string available in serial buffer in i'th character of array
  }
  inputCharArray[ inputCharArrayLength + 1 ] = '\0';                       // add the null character at the end of the character array to make it a string
  if (inputCharArray[0] == 'a' &&                                         // if user enters "all" on serial port then
      inputCharArray[1] == 'l' &&                                         // three characters are compared with AND condition
      inputCharArray[2] == 'l'     )
  {
    Serial.println("ALL zones are Selected");                            // all zones are selected
    for (uint8_t i = 0 ; i < NumberOfZones ; i++)                        // for loop to store all the zones in memory pointed by the pointer
    {
      *(selectedZones + i) = (i + 1);                                    //*(selectedZones + i) means contents of the address location pointed by  ( pointer + i'th location)
    }                                                                    // i+1 is used to stores selected zones because counter of loop is from 0 to 4 and names of zones are from 1 to 5
    return selectedZones ;                                               // return the pointer which points to the selected zones
  }
  else                                                                   // if "all" is not entered on the serial port then
  {
    selectedZones = stringSeparation( inputCharArray , inputCharArrayLength );     // this function returns the pointer which points to the selected zones obtained from the serial port.
    // selected zones are obtained by parsing zones from the string ( zone numbers are separated by commas)
    return selectedZones;                                                          // returns the pointer which points to the selected zones
  }
}


// FUNCTION     : stringSeparation
// DESCRIPTION  :
//      This function parses the selected zones from the entered string on the serial port.
// PARAMETERS   :
//        char inputCharArray[]        : character array to get the entered string on serial port for zones selection.
//        uint8_t inputCharArrayLength : integer array to get length of the entered string on the serial port. 
// RETURNS      :
//        uint8_t *        : returns the integer pointer to the array which has the pasrsed data of selected zones from entered
//                           sring on the serial port.


uint8_t *stringSeparation( char inputCharArray[] , uint8_t inputCharArrayLength )
{
  static uint8_t selectedZones[ NumberOfZones ];             // An array to save selected zones obtained from the string entered on serial port.
  //for loop to get the names of the zones(1 to 5) to the integer array from the entered string on the serial port.
  for (uint8_t i = 0 ; i < inputCharArrayLength ; i++)
  {
    if ( i == 0)   // very first character in the entered string will be zone name
    {
      selectedZones[0] =  (int)inputCharArray[i] - ASCIItoINTconstant ;  // to get the number of the zone in integer value into the array
    }
    else if (i == 2)  // third character in the entred string will be zone number
    {
      selectedZones[1] =  (int)inputCharArray[i] - ASCIItoINTconstant;  // to get the number of the zone in integer value into the array
    }
    else if (i == 4)  // fifth character in the entred string will be zone number
    {
      selectedZones[2] =  (int)inputCharArray[i] - ASCIItoINTconstant;  // to get the number of the zone in integer value into the array
    }
    else if (i == 6) // seventh character in the entred string will be zone number
    {
      selectedZones[3] =  (int)inputCharArray[i] - ASCIItoINTconstant;  // to get the number of the zone in integer value into the array
    }
  }
  return selectedZones;  // return the address of the array which has the selected zones parsed from the input string.
}


// FUNCTION     : initializingZones
// DESCRIPTION  :
//      This function that sends the HIGH signal on the all loop input pins.
// PARAMETERS   : void
// RETURNS      : void

void initializingZones()
{
  digitalWrite( loop1In , HIGH );  // a HIGH signal (ADC value of 255) on loop 1 input pin.
  digitalWrite( loop2In , HIGH );  // a HIGH signal (ADC value of 255) on loop 2 input pin.
  digitalWrite( loop3In , HIGH );  // a HIGH signal (ADC value of 255) on loop 3 input pin.
  digitalWrite( loop4In , HIGH );  // a HIGH signal (ADC value of 255) on loop 4 input pin.
  digitalWrite( loop5In , HIGH );  // a HIGH signal (ADC value of 255) on loop 5 input pin.
}


// FUNCTION     : displayArmed
// DESCRIPTION  :
//      This function prints the "System is Arned" on the LCD Screen and Serial port.
// PARAMETERS   : void
// RETURNS      : void

void displayArmed()
{
  lcd.clear();
  lcd.setCursor( firstColumnOfLCD , firstRowOfLCD);
  lcd.printf("   System Is");
  lcd.setCursor( firstColumnOfLCD , secondRowOfLCD);
  lcd.printf("     ARMED");
  Serial.println("\nSystem is Armed");
  delay(delayOf1Second);   // delay to show this message on LCD for one second before next message on the LCD.
}


// FUNCTION     : checkZones
// DESCRIPTION  :
//      This function checks the status of all the zones by checking the ADC readings on Loop output pins of zones and saves it in the array
//      and returns it.
//             State       ADC Value
//             Normal       255
//             Loop Break   2 to 254
//             fault        1
// PARAMETERS   : void
// RETURNS      :
//           uint8_t *   : integer pointer that points to the array which has the status of all zones

uint8_t *checkZones()
{
  static uint8_t zoneStatus[NumberOfZones];    //an array to save the state of the zone
  uint8_t loopReadings[NumberOfZones] ;        //an array to save the ADC readings of loop output pins of all zones
  loopReadings[0] = analogRead( loop1Out );    // read the ADC value on loop 1 output pin
  loopReadings[1] = analogRead( loop2Out );    // read the ADC value on loop 2 output pin
  loopReadings[2] = analogRead( loop3Out );    // read the ADC value on loop 3 output pin
  loopReadings[3] = analogRead( loop4Out );    // read the ADC value on loop 4 output pin
  loopReadings[4] = analogRead( loop5Out );    // read the ADC value on loop 5 output pin
  //for loop  is to check the states of all loops using their respective ADC readings.
  for ( uint8_t i = 0 ; i < NumberOfZones ; i++)
  {
    if ( loopReadings[i] == normalStateValue )       // i'th loop reading is equal to 255 then
    {
      zoneStatus[i] = normalState;                   // the state of (i+1)th zone is normal.  (i+1) because zons names are from 1 to 5 and array index is from 0 to 4.
    }
    else if ( loopReadings[i] == faultStateValue )   // i'th loop reading is equal to 1 then
    {
      zoneStatus[i] = faultState;                    // the state of (i+1)th is fault .  (i+1) because zons names are from 1 to 5 and array index is from 0 to 4.            
    }
    else         
    {
      zoneStatus[i] = loopBreakState;               // the state of (i+1)th is loop break.
    }
  }
  return zoneStatus;       // return the address of the array which saves the states of all zones
}


// FUNCTION     : displayAllZonesNormal
// DESCRIPTION  :
//      This function prints the All zones are safe" on the LCD Screen and Serial port. This function also calls the function
//            to display bit-mapped symbol of thumbs up on the LCD screen.
// PARAMETERS   : void
// RETURNS      : void

void displayAllZonesNormal()
{
  lcd.clear();
  displayThumbsUP();             // function to display thumbs up symbol on the LCD
  lcd.setCursor( fifthColumnOfLCD , firstRowOfLCD);
  lcd.printf("All Zones");
  lcd.setCursor( fifthColumnOfLCD , secondRowOfLCD);
  lcd.printf("Safe");
  Serial.println("All zones are safe");
  delay(delayOf1Second);
}

// FUNCTION     : displayAlarmZones
// DESCRIPTION  :
//      This function prints the "intruder alert" message on the LCD Screen and Serial port for the zones which were selected in zone selection state.
//         This function also calls the function to display bit-mapped symbol of "running man" on the LCD screen.
// PARAMETERS   : 
//         uint8_t breakZones[ NumberOfZones ] : array to get the zones which has loop break state.
//         uint8_t selectedZones[]             : array to get the zones which were selected in Zone Selection state.
// RETURNS      : void

void displayAlarmZones( uint8_t breakZones[ NumberOfZones ], uint8_t selectedZones[] )
{
  uint8_t numberOfSelectedZones = zonesCount(selectedZones);            // zonesCount function returns the number of zones selected by taking Selected zones array as a argument. 
  lcd.clear();
  displayIntruder();                                                    //Displays the bit mapped "running man" symbol on the LCD screen
  lcd.setCursor( fourthColumnOfLCD , firstRowOfLCD);
  lcd.printf("Intruder in");
  lcd.setCursor( fourthColumnOfLCD , secondRowOfLCD);
  lcd.printf("Zone");
  Serial.printf(" Intruder alert in zones  ");
  // for loop to display intruder alert message for zones which were selected to moniter(SelectedZones)
  for ( uint8_t i = 0 ; i < numberOfSelectedZones ; i++ )
  {
    if ( breakZones[*(selectedZones + i) - 1 ] == breachDetected )   //the data pointed by the address (selectedZones +i) is the zone. 
                                                                     // index of break zones array is from 0 to 4 and the zone names are from 1 to 5 so the loop break status of zone no. 1
                                                                     // is stored in breakZones[0]. hence (-1) is used along with (*(selectedZones + i)) as the index of brekZones
    {
      lcd.printf("%d,", *(selectedZones + i) );                      //print zone number on LCD Screen if the breakZones[*(selectedZones + i) - 1 ] == breachDetected
      Serial.printf("%d ,", *(selectedZones + i) );                  //print zone number on Serial port if the breakZones[*(selectedZones + i) - 1 ] == breachDetected
    }
  }
  Serial.printf("\n");   // new line
  delay(delayOf1Second); // wait for 1 second so the message can be displayed on the screen before the next message
}


// FUNCTION     : displayFaultZones
// DESCRIPTION  :
//      This function prints the "Fault in  zones" message on the LCD Screen and Serial port for the zones which were selected in zone selection state.
//         This function also calls the function to display bit-mapped symbol of exclamation point in square on the LCD screen.
// PARAMETERS   : 
//         uint8_t faultZones[ NumberOfZones ] : array to get the zones which has fault state.
//         uint8_t selectedZones[]             : array to get the zones which were selected in Zone Selection state.
// RETURNS      : void


void displayFaultZones( uint8_t faultZones[ NumberOfZones ], uint8_t selectedZones[] )
{
  uint8_t numberOfSelectedZones = zonesCount(selectedZones);               // zonesCount function returns the number of zones selected by taking Selected zones array as a argument.
  lcd.clear();
  displayFaultSymbol();                                                    //Displays the bit mapped Exclamation point in the square symbol on the LCD screen
  lcd.setCursor( fourthColumnOfLCD , firstRowOfLCD);
  lcd.printf("Fault in zones");
  Serial.printf(" fault in zones ");
  lcd.setCursor( fourthColumnOfLCD , secondRowOfLCD);
  // for loop to display fault message for zones which were selected to moniter(SelectedZones)
  for ( uint8_t i = 0 ; i < numberOfSelectedZones ; i++ )             
  {
    if ( faultZones[ *(selectedZones + i) - 1  ] == breachDetected )  //the data pointed by the address (selectedZones +i) is the zone. 
                                                                      // index of fault zones array is from 0 to 4 and the zone names are from 1 to 5 so the fault status of zone no. 1
                                                                      // is stored in faultZones[0]. hence (-1) is used along with (*(selectedZones + i)) as the index of brekZones
    {
      lcd.printf("%d,", *(selectedZones + i) );                       //print zone number on LCD Screen if the breakZones[*(selectedZones + i) - 1 ] == breachDetected
      Serial.printf("%d ,", *(selectedZones + i) );                   //print zone number on Serial port if the breakZones[*(selectedZones + i) - 1 ] == breachDetected
    }
  }
  Serial.printf("\n");    // new line
  delay(delayOf1Second);      // wait for 1 second so the message can be displayed on the screen before the next message
}


// FUNCTION     : zonesCount
// DESCRIPTION  :
//      This function returns the number of zones selected in zone selection state.
// PARAMETERS   :
//          uint8_t *selectedZones  :  an integer pointer that points to the selected zones array
// RETURNS      :
//          uint8_t   :  returns the number of zones selected in zone selection state.

uint8_t zonesCount(uint8_t *selectedZones)
{
  uint8_t numberOfSelectedZones = 0;    // variable to save number of selected zones
  //while loop is used to increment the counter as long as the selected zones pointer points to the zero value or all zones are selected.
  while ( *selectedZones != 0 && numberOfSelectedZones < NumberOfZones )
  {
    numberOfSelectedZones++;   // increment the counter 
    selectedZones++;           // increment the pointer
  }
  return numberOfSelectedZones;   // returns the number of zones selected in zone selection state.
}

// FUNCTION     : displayDisable
// DESCRIPTION  :
//      This function prints the "Alarm Disabled" on the LCD Screen and Serial port.
// PARAMETERS   : void
// RETURNS      : void

void displayDisable(void)
{
  lcd.clear();
  lcd.setCursor( firstColumnOfLCD , firstRowOfLCD);
  lcd.printf("Alarm Disabled");
  Serial.println("Alarm Disabled");
  delay(delayOf1Second);
}


// FUNCTION     : alarmTone
// DESCRIPTION  :
//      This function sends PWM signal to speaker pin for alarm tone
// PARAMETERS   : void
// RETURNS      : void


void alarmTone()
{
  static unsigned long previousMillis = 0;    // variable to check if the interval time is finished or not
  uint8_t interval = 10;              
  static boolean speakerTone = true;          // a flag to check if speaker is beeping or not
  unsigned long currentMillis = millis();     // variable to get the millis time
  if (currentMillis - previousMillis > interval)    // if time passed is more than interval
  {
    previousMillis = currentMillis;         // get the latest time after the interval is finished
    if (speakerTone == false)   // if speaker is not beeping then
    {
      tone(speakerPin, frequencyOfTone);   // start beeping
      speakerTone = true;               //set the flag to true
    }
    else
    {
      noTone(speakerPin);        // stop beeping
      speakerTone = false;       // set the flag to false
    }
  }
}


// FUNCTION     : displayLeftAttemptsMessage
// DESCRIPTION  :
//      This function prints the number of attempts remaining to enter correct password on the LCD Screen and Serial port.
// PARAMETERS   :
//         uint8_t passwordAttempts : integer to get the number of remaining attempts.
// RETURNS      : void

void displayLeftAttemptsMessage( uint8_t passwordAttempts )
{
  lcd.clear(); 
  lcd.setCursor( firstColumnOfLCD , secondRowOfLCD );
  lcd.printf("Attempts left=%d", passwordAttempts);            // displays remaaining attempts on LCD and Serial port
  Serial.printf("Attempts left=%d\n", passwordAttempts);
  delay(delayOf1Second);
}

// FUNCTION     : displayBlocked
// DESCRIPTION  :
//      This function prints "System Blocked" on the LCD Screen and Serial port.In this function a forever loop is added to mimic the
//            bloked system.
// PARAMETERS   : void
// RETURNS      : void

void displayBlocked()
{
  lcd.clear();
  lcd.setCursor( firstColumnOfLCD , firstRowOfLCD );
  lcd.printf(" System Blocked");
  Serial.printf(" System Blocked\n");
  while(1)             // forever loop to mimic system is blocked 
      ;
}

// FUNCTION     : disableISR
// DESCRIPTION  :
//      This interrupt service routine will change the state of state machine DISABLE BURLAR ALARM when push button connected to pin 13 (disable pin) is pressed.
// PARAMETERS   : void
// RETURNS      : void

void disableISR(void)
{
  burglarAlarmState = DISABLEBURGLARALARM ;     //changing the state to disable burglar alarm
}

// FUNCTION     : displayThumbsUP
// DESCRIPTION  :
//      This function prints the bit mapped "thumbs up" symbol on the LCD Screen.
// PARAMETERS   : void
// RETURNS      : void

void displayThumbsUP()
{
  byte thumb1[ bitMapByteArraySize ] =                       // bit mapped byte array for thumbs up for 01 position
  {
    B00100,
    B00011,
    B00100,
    B00011,
    B00100,
    B00011,
    B00010,
    B00001
  };

  byte thumb2[ bitMapByteArraySize ] =                     // bit mapped byte array for thumbs up for 00 position
  {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00011
  };

  byte thumb3[bitMapByteArraySize] =                      // bit mapped byte array for thumbs up for 11 position
  {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B00001,
    B11110
  };

  byte thumb4[ bitMapByteArraySize ] =                     // bit mapped byte array for thumbs up for 10 position
  {
    B00000,
    B01100,
    B10010,
    B10010,
    B10001,
    B01000,
    B11110,
    B00000
  };

  byte thumb5[ bitMapByteArraySize ] =                   // bit mapped byte array for thumbs up for 21 position
  {
    B00010,
    B00010,
    B00010,
    B00010,
    B00010,
    B01110,
    B10000,
    B00000
  };

  byte thumb6[ bitMapByteArraySize ] =                  // bit mapped byte array for thumbs up for 20 position
  {
    B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B10000,
    B01000,
    B00110
  };
  lcd.createChar(5, thumb1);         // create lcd bitmapped character 
  lcd.createChar(6, thumb2);         // create lcd bitmapped character
  lcd.createChar(7, thumb3);         // create lcd bitmapped character
  lcd.createChar(8, thumb4);         // create lcd bitmapped character
  lcd.createChar(9, thumb5);         // create lcd bitmapped character
  lcd.createChar(10, thumb6);        // create lcd bitmapped character
  lcd.setCursor(firstColumnOfLCD, secondRowOfLCD);
  lcd.write(5);                      // display the bitmapped character
  lcd.setCursor(firstColumnOfLCD, firstRowOfLCD);
  lcd.write(6);                      // display the bitmapped character
  lcd.setCursor(secondColumnOfLCD , secondRowOfLCD);
  lcd.write(7);                      // display the bitmapped character
  lcd.setCursor(secondColumnOfLCD, firstRowOfLCD);
  lcd.write(8);                      // display the bitmapped character
  lcd.setCursor(thirdColumnOfLCD, secondRowOfLCD);
  lcd.write(9);                      // display the bitmapped character
  lcd.setCursor(thirdColumnOfLCD, firstRowOfLCD);
  lcd.write(10);                     // display the bitmapped character

}


// FUNCTION     : displayIntruder
// DESCRIPTION  :
//      This function prints the bit mapped "running man" symbol on the LCD Screen.
// PARAMETERS   : void
// RETURNS      : void

void displayIntruder()
{
  byte intruder00[ bitMapByteArraySize ] =                         // bit mapped byte array for Running man symbol for 00 position
  {
    B00000,
    B00011,
    B00011,
    B00001,
    B00111,
    B01111,
    B10011,
    B00011
  };

  byte intruder01[ bitMapByteArraySize ] =                        // bit mapped byte array for Running man symbol for 01 position
  {
    B00000,
    B11000,
    B11000,
    B10000,
    B11100,
    B11110,
    B11001,
    B11000
  };

  byte intruder10[ bitMapByteArraySize ] =                      // bit mapped byte array for Running man symbol for 10 position
  {
    B00011,
    B00011,
    B00011,
    B00011,
    B00110,
    B11100,
    B11000
  };

  byte intruder11[ bitMapByteArraySize ] =                     // bit mapped byte array for Running man symbol for 11 position
  {
    B11000,
    B11000,
    B11000,
    B11000,
    B01100,
    B00111,
    B00011
  };
  lcd.createChar(11, intruder00);         // create lcd bitmapped character
  lcd.createChar(12, intruder01);         // create lcd bitmapped character
  lcd.createChar(13, intruder10);         // create lcd bitmapped character
  lcd.createChar(14, intruder11);         // create lcd bitmapped character
  lcd.setCursor( firstColumnOfLCD , firstRowOfLCD);
  lcd.write(11);           // display the bitmapped character
  lcd.write(12);           // display the bitmapped character
  lcd.setCursor(firstColumnOfLCD, secondRowOfLCD);
  lcd.write(13);           // display the bitmapped character
  lcd.write(14);           // display the bitmapped character
}

// FUNCTION     : displayFaultSymbol
// DESCRIPTION  :
//      This function prints the bit mapped symbol of exclamation point in the square on the LCD Screen.
// PARAMETERS   : void
// RETURNS      : void

void displayFaultSymbol()
{
  byte faultSymbol_00[ bitMapByteArraySize  ] =                   // bit mapped byte array for fault symbol for 00 position
  {
    B01111,
    B01000,
    B01000,
    B01000,
    B01000,
    B01000,
    B01000,
  };
  byte faultSymbol_10[ bitMapByteArraySize ] =                   // bit mapped byte array for fault symbol for 01 position
  {
    B01000,
    B01000,
    B01000,
    B01000,
    B01000,
    B01000,
    B01111,
  };
  byte faultSymbol_01[ bitMapByteArraySize ] =                 // bit mapped byte array for fault symbol for 10 position      
  {
    B11111,
    B00001,
    B10001,
    B10001,
    B10001,
    B10001,
    B10001,
  };
  byte faultSymbol_11[ bitMapByteArraySize ] =                // bit mapped byte array for Running man symbol for 11 position
  {
    B00001,
    B00001,
    B10001,
    B00001,
    B00001,
    B00001,
    B11111,
  };
  lcd.createChar(1, faultSymbol_00);                 // create lcd bitmapped character
  lcd.createChar(2, faultSymbol_01);                 // create lcd bitmapped character
  lcd.createChar(3, faultSymbol_10);                 // create lcd bitmapped character
  lcd.createChar(4, faultSymbol_11);                 // create lcd bitmapped character
  lcd.setCursor( firstColumnOfLCD , firstRowOfLCD);
  lcd.write(1);          // display the bitmapped character
  lcd.write(2);          // display the bitmapped character
  lcd.setCursor(firstColumnOfLCD, secondRowOfLCD);
  lcd.write(3);          // display the bitmapped character
  lcd.write(4);          // display the bitmapped character
}
