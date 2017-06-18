   FILE          : BurglarAlarmProject.ino
   PROJECT       : PROG8125-16S - Project
   PROGRAMMER    : Mayur Hadole (5783)
   FIRST VERSION : 2016-07-31
   DESCRIPTION   :
         Project Statement (No. 2)
     Burglar alarm that monitors 5 zones. Zones are monitored by a loop of wire.
     Circuitry is needed that can detect if the loop has been broken (an Alarm Condition)
     or shorted (a fault condition).  The alarm should accept a password from the serial port
     in order to enable or disable the entire alarm or individual zones. User output should
     be done using a speaker and LCD.

     I have used the state machine to make the burglar alarm with above funtions and some extra functions

    State 1:PASSWORD AUTHENTICATION
          Shows the welcome message on LCD and then asks for password to enter using serial port.If
          the password is correct the program moves to the next state else user gets two more tries to
          get the password right.System is blocked if all three attempts are used.

    State 2:INITIALIZING SYSTEM
          High Signal is written on all five loops through loopIn pins. These High Signals will be used to
          moniter the loop break, faults and normal status of loops.

    State 3:ZONE SELECTION
          In this State the program asks the user using LCD screen to enter the numbers of zones that needs to be monitered.
          If the user enteres "all" on Serial port then all zones are saved in a array.
          And If the user enters the names of zones(from 1 to 5) that needs to be monitered with commas in between then those
          zones are saved in the array.

    State 4:ARMED
          In this state, all the loops are checked by reading inputs from loop out pins. Depending upon their ADC readings
          their status is determined. Following is the range of ADC values for all three states.

          ADC Value        Status
          255              Normal
          1                Fault
          2 to 254         Loop Break (intruder alert)

          In this same state, if all the loops are in normal state . All Zones Safe message is displayed on LCD Screen and Serial port
          Along with the thumbs up symbol.

    State 5: BREACH DETECTED
          In this State, the status of zones which were selected in previous state are stored in the array.(one array for fault zones and
          another one for loop break zones).
          These arrays are used to call functions to display the status on LCD and to play the alarm using speaker.
          If zones are in loop break state then intruder alert message is displayed on the screen along with the "running person" symbol.
          And if the zones are in fault state then fault message is displayed on the screen with the bitmapped fault syambol.
          If some zones are in fault state  and some are in loop break state then both fault and intruder alert messages are displayed on
          LCD screen with the gap of 1 second in between them.

    State 6. DISABLE BURGLAR ALARM
          This is the state in which the program resets using external interrupt connected to push button.
           if the push button is pressed at any time during the execution of program the system will reset to state 1.


    Extra Features
       1. Three bitmapped Symbols are used to display on lcd for three different states.
               State           Symbol
               Loop Break      Running Man
               Fault           Exclamation symbol in the square
               Normal          Thumbs up symbol

       2. External interrupt is used to reset the system to state 1.
       3. System is blocked if more than three times the entered password is wrong
       4. millis() is used to generate the alarm sound

          This code is tested on Teensy 3.1 board
