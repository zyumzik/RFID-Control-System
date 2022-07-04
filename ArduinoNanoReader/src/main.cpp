/*
 ARDUINO NANO READER {rfid + rs485}
 Name:		ArduinoNanoReader.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <EasyTransfer.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <Wiegand.h>

#pragma region GLOBAL_SETTINGS

#define DEBUG                                   // comment this line to not write anything to Serial as debug
const String    program_version = "0.9.0";      // program version
unsigned long   serial_baud     = 115200;       // serial baud speed

unsigned long   device_id       = 801;          // unique ID of reader device
unsigned long   read_delay      = 250;          // reading card delay
EasyTransfer    easy_transfer;                  // object for exchanging data between RS485
Message         message;                        // object for exchanging data between ArduinoMega and ArduinoUno

#pragma endregion

#pragma region RS485_SETTINGS

uint8_t         rs_rx_pin = 0;                  // RS485 receive pin
uint8_t         rs_tx_pin = 1;                  // RS485 transmit pin
long            rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485

#pragma endregion

#pragma region WIEGAND_SETTINGS

WIEGAND         wiegand;                        // object for reading data from Wiegnad RFID
uint8_t         w_rx_pin  = 2;                  // wiegand data1 input pin
uint8_t         w_tx_pin  = 3;                  // wiegand data2 input pin
uint8_t         w_zum_pin = 6;                  // wiegand built-in zummer control pin
uint8_t         w_led_pin = 7;                  // wiegand built-in led control pin
unsigned long   w_last_card;                    // last read card id

#pragma endregion

#pragma region SERVER_RESPONSE_STATES

const uint8_t   state_ok            = 0;        // accessed
const uint8_t   state_connect_error = 97;       // server connection error
const uint8_t   state_json_error    = 98;       // json deserialization error
const uint8_t   state_timeout_error = 99;       // server response timeout error

#pragma endregion

// send message to ArduinoUNO
void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id);

// makes sound for beepMs time and delayMs delay
void wiegandBeep(unsigned long beepMs, unsigned long delayMs);

// blinks for beepMs time and delayMs delay
void wiegandBlink(unsigned long blinkMs, unsigned long delayMs);

void setup()
{
#ifdef DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    Serial.println(
        String("start working...") + 
        String("\nprogram version: ") + program_version +
        String("\nserial baud speed: ") + serial_baud);
#endif //DEBUG
    
    wiegand.begin(w_rx_pin, w_tx_pin);
    pinMode(w_zum_pin, OUTPUT);
    pinMode(w_led_pin, OUTPUT);

    rs485.begin(rs_baud);
    easy_transfer.begin(details(message), &rs485);
}

void loop()
{
    // received message from ArduinoUNO
    if (easy_transfer.receiveData())
    {
#ifdef DEBUG
        Serial.println("received message: ");
        message.print();
#endif // DEBUG

        if (message.device_id != device_id ||
            message.card_id != w_last_card)
            return;

        if (message.state_id == state_ok)
        {
            
        }
        else if (message.state_id == state_connect_error)
        {

        }
        else if (message.state_id == state_json_error)
        {

        }
        else if (message.state_id == state_timeout_error)
        {

        }
        // unknown state
        else
        {

        }

        message.clean();  // ???
    }

    // read a card
    if (wiegand.available())
    {
        w_last_card = wiegand.getCode();

#ifdef DEBUG
        Serial.println("read card id: " + String(w_last_card));
#endif //DEBUG
        
        sendData(device_id, w_last_card, 0, 0);

        delay(read_delay);
    }
}

#pragma region FUNCTION_DESCRIPTION

void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id)
{
    message.set(device_id, card_id, state_id, other_id);
    easy_transfer.sendData();

    message.clean();

#ifdef DEBUG
    Serial.println("send data:");
    message.print();
#endif // DEBUG
}

void wiegandBeep(unsigned long beepMs, unsigned long delayMs)
{
    digitalWrite(w_zum_pin, HIGH);
    delay(beepMs);
    digitalWrite(w_zum_pin, LOW);
    delay(delayMs);
}

void wiegandBlink(unsigned long blinkMs, unsigned long delayMs)
{
    digitalWrite(w_led_pin, HIGH);
    delay(blinkMs);
    digitalWrite(w_led_pin, LOW);
    delay(delayMs);
}

#pragma endregion