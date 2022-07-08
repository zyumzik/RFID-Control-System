/*
 ARDUINO NANO READER {rfid + rs485}
 Name:		ArduinoNanoReader.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <Arduino.h>
#include <EasyTransfer.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <Wiegand.h>

#pragma region GLOBAL_SETTINGS

#define DEBUG                                   // comment this line to not write anything to Serial as debug
const String    program_version = "0.9.1";      // program version
unsigned long   serial_baud     = 115200;       // serial baud speed

unsigned long   device_id       = 801;          // unique ID of reader device
unsigned long   read_delay      = 250;          // reading card delay
unsigned long   read_last       = 0;            // last read card time
unsigned long   handle_delay    = 250;          // handle received response delay (for skipping default wiegand blink and beep)
EasyTransfer    easy_transfer;                  // object for exchanging data using RS485
Message         message;                        // exchangeable object for EasyTransfer

#pragma endregion //GLOBAL_SETTINGS

#pragma region RS485_SETTINGS

uint8_t         rs_rx_pin = 4;                  // RS485 receive pin
uint8_t         rs_tx_pin = 5;                  // RS485 transmit pin
long            rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485

#pragma endregion //RS485_SETTINGS

#pragma region WIEGAND_SETTINGS

WIEGAND         wiegand;                        // object for reading data from Wiegnad RFID
uint8_t         w_rx_pin  = 2;                  // wiegand receive pin
uint8_t         w_tx_pin  = 3;                  // wiegand transmit pin
uint8_t         w_zum_pin = 6;                  // wiegand built-in zummer control pin
uint8_t         w_led_pin = 7;                  // wiegand built-in led control pin
unsigned long   w_last_card;                    // last read card id

#pragma endregion //WIEGAND_SETTINGS

#pragma region SERVER_STATES

const uint8_t   st_denied        = 0;           // access denied                //
const uint8_t   st_no_srvr_cnctn = 95;          // no ethernet connection       // !client.connect(server, port)
const uint8_t   st_request_error = 96;          // wrong server request         // !client.find("\r\n\r\n")
const uint8_t   st_no_response   = 97;          // no response from server      // json {"id":0,"kod":0,"status":0}
const uint8_t   st_json_error    = 98;          // json deserialization error   // if (DeserializationError)
const uint8_t   st_timeout_error = 99;          // server connection timeout    // !client.available()
const uint8_t   st_no_ethr_cnctn = 100;         // no ethernet connection       // !ethernetConnected()
const uint8_t   st_ok            = 1;           // accessed                     // 

#pragma endregion //SERVER_STATES

// send message to Arduino Uno (Ethernet Sender)
void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id);

// reverses code by bit
unsigned long wiegandToDecimal(unsigned long code);

// handle response from server
void handleResponse();

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
        String("Arduino Nano AT328 RFID-WG Reader") +
        String("\nstart working...") + 
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
        handleResponse();
    }

    // read a card
    if (millis() >= read_last + read_delay)
    {
        if (wiegand.available())
        {
            read_last = millis();
            w_last_card = wiegandToDecimal(wiegand.getCode());

#ifdef DEBUG
            Serial.println("read card id: " + String(w_last_card));
            Serial.println("*****\n");
#endif //DEBUG

            sendData(
                device_id, 
                w_last_card, 
                0, 
                0
            );
        }
    }
}

#pragma region FUNCTION_DESCRIPTION

void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id)
{
    message.set(device_id, card_id, state_id, other_id);

#ifdef DEBUG
    Serial.println("---send---");
    message.print();
    Serial.println("*****\n");
#endif //DEBUG

    easy_transfer.sendData();

    message.clean();
}

unsigned long wiegandToDecimal(unsigned long code)
{
    unsigned long code_decimal = 0;
    int t = 0;
    long unsigned int a = 0, b = 0;
    while(code > 0)
    {
        t++;
        a = code / 256;
        b = code - a * 256;
        code = a;
        code_decimal = code_decimal * 256 + b;
    }
    return code_decimal;
}

void handleResponse()
{
#ifdef DEBUG
        Serial.println("-received-");
        message.print();
        Serial.println("*****\n");
#endif // DEBUG

        // base data checking (if message was for this device)
        if (message.device_id != device_id)
        {
#ifdef DEBUG
            Serial.println("message not handled: another device");
#endif //DEBUG
            return;
        }
        else if (message.card_id != w_last_card)
        {
#ifdef DEBUG
            Serial.println("message not handled: another card");
#endif //DEBUG
            return;
        }
        
        delay(handle_delay);

        if (message.state_id == st_denied)
        {
#ifdef DEBUG
            Serial.println("state 0: access denied");
#endif //DEBUG
        }
        else if (message.state_id == st_ok)
        {
#ifdef DEBUG
            Serial.println("state 1: it's okay");
#endif //DEBUG
        }
        else if (message.state_id == st_no_srvr_cnctn)
        {
#ifdef DEBUG
            Serial.println("error 95: no server connection");
#endif //DEBUG
        }
        else if (message.state_id == st_request_error)
        {
#ifdef DEBUG
            Serial.println("error 96: invalid server request");
#endif //DEBUG
        }
        else if (message.state_id == st_no_response)
        {
#ifdef DEBUG
            Serial.println("error 97: no response from server");
#endif //DEBUG
        }
        else if (message.state_id == st_json_error)
        {
#ifdef DEBUG
            Serial.println("error 98: json error");
#endif //DEBUG
        }
        else if (message.state_id == st_timeout_error)
        {
#ifdef DEBUG
            Serial.println("error 99: server connect timeout");
#endif //DEBUG
        }
        else if (message.state_id == st_no_ethr_cnctn)
        {
#ifdef DEBUG
            Serial.println("error 100: no ethernet connection");
#endif //DEBUG
        }
        // unknown state
        else
        {
#ifdef DEBUG
            Serial.println("unknown state");
#endif //DEBUG
        }

        message.clean();  // ???
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

#pragma endregion //FUNCTION_DESCRIPTION