/*
 ARDUINO NANO READER {rfid + rs485}
 Name:		ArduinoNanoReader.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <Arduino.h>
#include <EasyTransfer.h>
#include <EEPROM.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include <Wiegand.h>
#include <WiegandSignal.h>

#pragma region GLOBAL_SETTINGS

#define DEBUG                                   // comment this line to not write anything to Serial as debug
const String    program_version = "0.9.5";      // program version
unsigned long   serial_baud     = 115200;       // serial baud speed

unsigned long   broadcast_id    = 999;          // id for receiving broadcast messages
unsigned long   device_id       = 0;            // unique ID of reader device
unsigned long   handle_delay    = 0;            // handle received response delay (for skipping default wiegand blink and beep)
unsigned long   last_handle     = 0;
EasyTransfer    easy_transfer;                  // object for exchanging data using RS485
Message         message;                        // exchangeable object for EasyTransfer
bool            ethernet_flag   = true;         // flag of ethernet connection (true if connection established)

#pragma endregion //GLOBAL_SETTINGS

#pragma region RS485_SETTINGS

uint8_t         rs_rx_pin = 4;                  // RS485 receive pin
uint8_t         rs_tx_pin = 5;                  // RS485 transmit pin
long            rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485
bool            rs_connected = true;            // true if response from Arduino Uno is being receiving
Timer           rs_wait_timer;                  // timer for waiting respinse from Arduino Uno
unsigned long   rs_last_send = 0;               // last send to Arduino Uno
unsigned long   rs_response_wait = 1500;        // time for waiting response from Arduino Uno

#pragma endregion //RS485_SETTINGS

#pragma region WIEGAND_SETTINGS

WIEGAND         wiegand;                        // object for reading data from Wiegnad RFID
uint8_t         w_rx_pin  = 2;                  // wiegand receive pin
uint8_t         w_tx_pin  = 3;                  // wiegand transmit pin
uint8_t         w_zum_pin = 6;                  // wiegand built-in zummer control pin
uint8_t         w_led_pin = 7;                  // wiegand built-in led control pin
unsigned long   w_last_card;                    // last read card id
WiegandSignal   w_signal(w_led_pin, w_zum_pin); // object for better led and zummer signaling
Timer           w_timer;                        // timer for reading card delay
unsigned long   w_read_delay = 1000;            // delay for reading card

#pragma endregion //WIEGAND_SETTINGS

#pragma region SERVER_STATES
                                                
// responses:
const uint8_t   st_unknown          = 0;        // unknown state
const uint8_t   st_allow            = 1;        // access allowed
const uint8_t   st_re_entry         = 2;        // re-entry
const uint8_t   st_denied           = 3;        // access denied
const uint8_t   st_invalid          = 4;        // invalid card (database does not contain such card)
const uint8_t   st_blocked          = 5;        // card is blocked

const uint8_t   st_reg_device       = 90;       // registration of device (check for similar device id in readers)
const uint8_t   st_set_device_id    = 92;       // setting device id

// errors:
const uint8_t   er_no_srvr_cnctn    = 95;       // no server connection             | !client.connect(server, port)
const uint8_t   er_request          = 96;       // wrong server request             | !client.find("\r\n\r\n")
const uint8_t   er_no_response      = 97;       // no response from server          | json {"id":0,"kod":0,"status":0}
const uint8_t   er_json             = 98;       // json deserialization error       | if (DeserializationError)
const uint8_t   er_timeout          = 99;       // server connection timeout        | !client.available()
const uint8_t   er_no_ethr_cnctn    = 100;      // no ethernet connection           | !ethernetConnected()

#pragma endregion //SERVER_STATES

#pragma region FUNCTION_DECLARATION

// clear EEPROM
void clearMemory();

// save device id to EEPROM
void saveDeviceId(uint8_t address, unsigned long id);

// load device id from EEPROM
unsigned long loadDeviceId(uint8_t address);

// send message to Arduino Uno (Ethernet Sender)
void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id);

// reverses code by bit
unsigned long wiegandToDecimal(unsigned long code);

// handle response from server
void handleResponse();

#pragma endregion //FUNCTION_DECLARATION

void setup()
{
    // uncomment lines below to save some unique id to EEPROM
    // clearMemory();
    // saveDeviceId(0, -unique id-);    

    device_id = loadDeviceId(0);

    wiegand.begin(w_rx_pin, w_tx_pin);
    w_timer.begin(w_read_delay);
    pinMode(w_zum_pin, OUTPUT);
    pinMode(w_led_pin, OUTPUT);
    rs485.begin(rs_baud);
    easy_transfer.begin(details(message), &rs485);
    
#ifdef DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    Serial.println(
        String("Arduino Nano AT328 RFID-WG Reader. V." + program_version) +
        String("\nstart working on ") + serial_baud + " baud speed" + 
        String("\ndevice id: ") + device_id);
#endif //DEBUG

    // registering new connected device
    delay(w_read_delay);
    sendData(
        device_id,
        0,
        st_reg_device,
        0
    );
}

void loop()
{
    // too much time passed since last send (no response from Arduino Uno)
    if (rs_wait_timer.update())
    {
        rs_connected = false;
    }

    // received message from ArduinoUNO
    if (easy_transfer.receiveData())
    {
        handleResponse();
    }

    // making signal if something is wrong
    if (!rs_connected)
    {
        w_signal.update(WiegandSignal::Length::s_short, false, true);
    }
    else if (!ethernet_flag)
    {
        w_signal.update(WiegandSignal::Length::s_medium, false, true);
    }
    else
    {
        w_signal.update();
    }

    // read a card
    if (wiegand.available())
    {
        w_last_card = wiegandToDecimal(wiegand.getCode());
        
        if (w_timer.update())
        {
            // check for ethernet connection, signaling state
            if (ethernet_flag && !w_signal.is_invoke)
            {
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
                //delay(50);
            }
        }
    }
}

#pragma region FUNCTION_DESCRIPTION

void clearMemory()
{
    for (int i = 0; i < EEPROM.length(); i++)
    {
        EEPROM.write(i, 0);
    }
}

void saveDeviceId(uint8_t address, unsigned long id)
{
    byte buf[4];
    (unsigned long&)buf = id;
    for(byte i = 0; i < 4; i++) 
    {
        EEPROM.write(address + i, buf[i]);
    }
}

unsigned long loadDeviceId(uint8_t address)
{
    byte buf[4];
    for(byte i = 0; i < 4; i++)
    { 
        buf[i] = EEPROM.read(address + i);
    }
    unsigned long &id = (unsigned long&)buf;
    return id;
}

void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id)
{
    rs_last_send = millis();
    rs_wait_timer.begin(rs_response_wait);

    message.set(device_id, card_id, state_id, other_id);

#ifdef DEBUG
    Serial.print(millis());
    Serial.println(":---send---");
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
    rs_connected = true;
    rs_wait_timer.stop();

#ifdef DEBUG
    Serial.print(millis());
    Serial.println(":-received-");
    message.print();
    Serial.println("*****\n");
#endif // DEBUG
    // base data checking (if message was for this device)
    if (message.device_id != broadcast_id &&
        message.device_id != device_id)
    {
#ifdef DEBUG
        Serial.println("message not handled");
#endif //DEBUG
        return;
    }
        
    delay(handle_delay);

    // if error - long signal firstly
    if (message.state_id >= 90)
    {
        w_signal.invoke(WiegandSignal::Length::s_long_long, 1);
    }

    switch (message.state_id)
    {
    case st_unknown:
    {
        w_signal.invoke(WiegandSignal::Length::s_long, 3);
        #ifdef DEBUG
        Serial.println("unknown status");
        #endif //DEBUG
        break;
    }
    case st_allow:
    {
        #ifdef DEBUG
        Serial.println("access allowed");
        #endif //DEBUG
        break;
    }
    case st_re_entry:
    {
        w_signal.invoke(WiegandSignal::Length::s_long, 2);
        #ifdef DEBUG
        Serial.println("re-entry");
        #endif //DEBUG
        break;
    }
    case st_denied:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium, 10);
        #ifdef DEBUG
        Serial.println("access denied");
        #endif //DEBUG
        break;
    }
    case st_invalid:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium, 5);
        #ifdef DEBUG
        Serial.println("invalid card");
        #endif //DEBUG
        break;
    }
    case st_blocked:
    {
        w_signal.invoke(WiegandSignal::Length::s_short, 10);
        #ifdef DEBUG
        Serial.println("");
        #endif //DEBUG
        break;
    }

    case st_set_device_id:
    {
        device_id = message.card_id;
        saveDeviceId(0, device_id);

        #ifdef DEBUG
        Serial.print("device id set to: ");
        Serial.println(device_id);
        #endif //DEBUG
        break;
    }
    
    // errors:
    case er_no_srvr_cnctn:
    {
        w_signal.invoke(WiegandSignal::Length::s_long, 3);
        #ifdef DEBUG
        Serial.println("error: no server connection");
        #endif //DEBUG
        break;
    }
    case er_request:
    {
        w_signal.invoke(WiegandSignal::Length::s_short, 5);
        #ifdef DEBUG
        Serial.println("error: wrong server request");
        #endif //DEBUG
        break;
    }
    case er_no_response:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium , 3);
        #ifdef DEBUG
        Serial.println("error: no response from server");
        #endif //DEBUG
        break;
    }
    case er_json:
    {
        w_signal.invoke(WiegandSignal::Length::s_short, 5);
        #ifdef DEBUG
        Serial.println("error: wrong json deserialization");
        #endif //DEBUG
        break;
    }
    case er_timeout:
    {
        w_signal.invoke(WiegandSignal::Length::s_medium, 3);
        #ifdef DEBUG
        Serial.println("error: server connection timeout");
        #endif //DEBUG
        break;
    }
    case er_no_ethr_cnctn:
    {
        if (message.other_id == 0)
        {
            ethernet_flag = false;
        }
        else if (message.other_id == 1)
        {
            ethernet_flag = true;
        }
        #ifdef DEBUG
        Serial.print("ethernet connection: ");
        Serial.println(message.other_id);
        #endif //DEBUG
        break;
    }
    
    // unknown state
    default:
    {
        #ifdef DEBUG
        Serial.println("unknown state. message not handled");
        #endif //DEBUG
        break;
    }
    }

    message.clean();
}

#pragma endregion //FUNCTION_DESCRIPTION
