//=====[Libraries]=============================================================

#include "mbed.h"
#include "arm_book_lib.h"

#include "pc_serial_com.h"

#include "siren.h"
#include "fire_alarm.h"
#include "code.h"
#include "date_and_time.h"
#include "temperature_sensor.h"
#include "gas_sensor.h"
#include "event_log.h"
#include <cstring>
//=====[Declaration of private defines]========================================
#define DATE_AND_TIME_FORMAT_LENGTH 19  // Format: "YYYY-MM-DDThh:mm:ss"

//=====[Declaration of private data types]=====================================

typedef enum{
    PC_SERIAL_COMMANDS,
    PC_SERIAL_GET_CODE,
    PC_SERIAL_SAVE_NEW_CODE,
    PC_SERIAL_SETTING_DATE_AND_TIME
} pcSerialComMode_t;

//=====[Declaration and initialization of public global objects]===============

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);

//=====[Declaration of external public global variables]=======================

//=====[Declaration and initialization of public global variables]=============

char codeSequenceFromPcSerialCom[CODE_NUMBER_OF_KEYS];

//=====[Declaration and initialization of private global variables]============

static pcSerialComMode_t pcSerialComMode = PC_SERIAL_COMMANDS;
static bool codeComplete = false;
static int numberOfCodeChars = 0;
static int numberOfDateAndTimeChars = 0;

//=====[Declarations (prototypes) of private functions]========================

static void pcSerialComStringRead( char* str, int strLength );

static void pcSerialComGetCodeUpdate( char receivedChar );
static void pcSerialComSaveNewCodeUpdate( char receivedChar );

static void pcSerialComCommandUpdate( char receivedChar );
static void pcSerialComSetDateAndTimeUpdate(char receiverChar);

static void availableCommands();
static void commandShowCurrentAlarmState();
static void commandShowCurrentGasDetectorState();
static void commandShowCurrentOverTemperatureDetectorState();
static void commandEnterCodeSequence();
static void commandEnterNewCode();
static void commandShowCurrentTemperatureInCelsius();
static void commandShowCurrentTemperatureInFahrenheit();
static void commandSetDateAndTime();
static void commandShowDateAndTime();
static void commandShowStoredEvents();


//=====[Implementations of public functions]===================================

void pcSerialComInit()
{
    availableCommands();
}

char pcSerialComCharRead()
{
    char receivedChar = '\0';
    if( uartUsb.readable() ) {
        uartUsb.read( &receivedChar, 1 );
    }
    return receivedChar;
}

void pcSerialComStringWrite( const char* str )
{
    uartUsb.write( str, strlen(str) );
}

void pcSerialComUpdate()
{
    char receivedChar = pcSerialComCharRead();
    if( receivedChar != '\0' ) {
        switch ( pcSerialComMode ) {

            case PC_SERIAL_COMMANDS:
                pcSerialComCommandUpdate( receivedChar );
            break;

            case PC_SERIAL_GET_CODE:
                pcSerialComGetCodeUpdate( receivedChar );
            break;

            case PC_SERIAL_SAVE_NEW_CODE:
                pcSerialComSaveNewCodeUpdate( receivedChar );
            break;
            
            case PC_SERIAL_SETTING_DATE_AND_TIME:
                pcSerialComSetDateAndTimeUpdate(receivedChar);
            break;
                  
            default:
                pcSerialComMode = PC_SERIAL_COMMANDS;
            break;
        }
    }    
}

bool pcSerialComCodeCompleteRead()
{
    return codeComplete;
}

void pcSerialComCodeCompleteWrite( bool state )
{
    codeComplete = state;
}

//=====[Implementations of private functions]==================================

static void pcSerialComStringRead( char* str, int strLength )
{
    int strIndex;
    for ( strIndex = 0; strIndex < strLength; strIndex++) {
        uartUsb.read( &str[strIndex] , 1 );
        uartUsb.write( &str[strIndex] ,1 );
    }
    str[strLength]='\0';
}

static void pcSerialComGetCodeUpdate( char receivedChar )
{
    codeSequenceFromPcSerialCom[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
   if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        codeComplete = true;
        numberOfCodeChars = 0;
    } 
}

static void pcSerialComSaveNewCodeUpdate( char receivedChar )
{
    static char newCodeSequence[CODE_NUMBER_OF_KEYS];

    newCodeSequence[numberOfCodeChars] = receivedChar;
    pcSerialComStringWrite( "*" );
    numberOfCodeChars++;
    if ( numberOfCodeChars >= CODE_NUMBER_OF_KEYS ) {
        pcSerialComMode = PC_SERIAL_COMMANDS;
        numberOfCodeChars = 0;
        codeWrite( newCodeSequence );
        pcSerialComStringWrite( "\r\nNew code configured\r\n\r\n" );
    } 
}

static void pcSerialComCommandUpdate( char receivedChar )
{
    switch (receivedChar) {
        case '1': commandShowCurrentAlarmState(); break;
        case '2': commandShowCurrentGasDetectorState(); break;
        case '3': commandShowCurrentOverTemperatureDetectorState(); break;
        case '4': commandEnterCodeSequence(); break;
        case '5': commandEnterNewCode(); break;
        case 'c': case 'C': commandShowCurrentTemperatureInCelsius(); break;
        case 'f': case 'F': commandShowCurrentTemperatureInFahrenheit(); break;
        case 's': case 'S': commandSetDateAndTime(); break;
        case 't': case 'T': commandShowDateAndTime(); break;
        case 'e': case 'E': commandShowStoredEvents(); break;
        default: availableCommands(); break;
    } 
}

static void availableCommands()
{
    pcSerialComStringWrite( "Available commands:\r\n" );
    pcSerialComStringWrite( "Press '1' to get the alarm state\r\n" );
    pcSerialComStringWrite( "Press '2' to get the gas detector state\r\n" );
    pcSerialComStringWrite( "Press '3' to get the over temperature detector state\r\n" );
    pcSerialComStringWrite( "Press '4' to enter the code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press '5' to enter a new code to deactivate the alarm\r\n" );
    pcSerialComStringWrite( "Press 'f' or 'F' to get lm35 reading in Fahrenheit\r\n" );
    pcSerialComStringWrite( "Press 'c' or 'C' to get lm35 reading in Celsius\r\n" );
    pcSerialComStringWrite( "Press 's' or 'S' to set the date and time\r\n" );
    pcSerialComStringWrite( "Press 't' or 'T' to get the date and time\r\n" );
    pcSerialComStringWrite( "Press 'e' or 'E' to get the stored events\r\n" );
    pcSerialComStringWrite( "\r\n" );
}

static void commandShowCurrentAlarmState()
{
    if ( sirenStateRead() ) {
        pcSerialComStringWrite( "The alarm is activated\r\n");
    } else {
        pcSerialComStringWrite( "The alarm is not activated\r\n");
    }
}

static void commandShowCurrentGasDetectorState()
{
    if ( gasDetectorStateRead() ) {
        pcSerialComStringWrite( "Gas is being detected\r\n");
    } else {
        pcSerialComStringWrite( "Gas is not being detected\r\n");
    }    
}

static void commandShowCurrentOverTemperatureDetectorState()
{
    if ( overTemperatureDetectorStateRead() ) {
        pcSerialComStringWrite( "Temperature is above the maximum level\r\n");
    } else {
        pcSerialComStringWrite( "Temperature is below the maximum level\r\n");
    }
}

static void commandEnterCodeSequence()
{
    if( sirenStateRead() ) {
        pcSerialComStringWrite( "Please enter the four digits numeric code " );
        pcSerialComStringWrite( "to deactivate the alarm: " );
        pcSerialComMode = PC_SERIAL_GET_CODE;
        codeComplete = false;
        numberOfCodeChars = 0;
    } else {
        pcSerialComStringWrite( "Alarm is not activated.\r\n" );
    }
}

static void commandEnterNewCode()
{
    pcSerialComStringWrite( "Please enter the new four digits numeric code " );
    pcSerialComStringWrite( "to deactivate the alarm: " );
    numberOfCodeChars = 0;
    pcSerialComMode = PC_SERIAL_SAVE_NEW_CODE;

}

static void commandShowCurrentTemperatureInCelsius()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadCelsius() );
    pcSerialComStringWrite( str );  
}

static void commandShowCurrentTemperatureInFahrenheit()
{
    char str[100] = "";
    sprintf ( str, "Temperature: %.2f \xB0 C\r\n",
                    temperatureSensorReadFahrenheit() );
    pcSerialComStringWrite( str );  
}

static void commandSetDateAndTime()
{

  pcSerialComStringWrite("Please enter date and time in the YYYY-MM-DDThh:mm:ss");
  
  pcSerialComMode = PC_SERIAL_SETTING_DATE_AND_TIME;
  numberOfDateAndTimeChars = 0;

}

static void commandShowDateAndTime()
{
    char str[100] = "";
    sprintf ( str, "Date and Time = %s", dateAndTimeRead() );
    pcSerialComStringWrite( str );
    pcSerialComStringWrite("\r\n");
}

static void commandShowStoredEvents()
{
    char str[EVENT_STR_LENGTH] = "";
    int i;
    for (i = 0; i < eventLogNumberOfStoredEvents(); i++) {
        eventLogRead( i, str );
        pcSerialComStringWrite( str );   
        pcSerialComStringWrite( "\r\n" );                    
    }
}

static void pcSerialComSetDateAndTimeUpdate(char receivedChar)
{
  
    static char date_and_time_input[DATE_AND_TIME_FORMAT_LENGTH + 1];

    // Save input char to buffer
    date_and_time_input[numberOfDateAndTimeChars] = receivedChar;
    numberOfDateAndTimeChars++;

    if (numberOfDateAndTimeChars >= DATE_AND_TIME_FORMAT_LENGTH) { // Parsing
        char year[5];
        strncpy(year, date_and_time_input + 0, 4);
        year[4] = '\0';

        char month[3];
        strncpy(year, date_and_time_input + 5, 2);
        month[2] = '\0';        

        char day[3];
        strncpy(year, date_and_time_input + 9, 2);
        day[2] = '\0';

        char hour[3];
        strncpy(year, date_and_time_input + 11, 2);
        day[2] = '\0';         

        char minute[3];
        strncpy(year, date_and_time_input + 14, 2);
        minute[2] = '\0'; 

        char second[3];
        strncpy(year, date_and_time_input + 16, 2);
        second[2] = '\0'; 

        dateAndTimeWrite(atoi(year), atoi(month), atoi(day), atoi(hour), atoi(minute), atoi(second));

        pcSerialComStringWrite("\r\nNew date and time setted.\r\n\r\n");

        numberOfCodeChars = 0;
        pcSerialComMode = PC_SERIAL_COMMANDS;
    }

}