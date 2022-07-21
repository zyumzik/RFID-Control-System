/*
 ARDUINO UNO ETHERNET SENDER {ethernet shield 2 + rs485}
 Name:		ArduinoUnoEthernetSender.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
//#include <EthernetClient.h>
#include <Ethernet.h>
#include <EasyTransfer.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <SPI.h>

#define DEBUG false

#if DEBUG

char debug_buffer[128];
#define debugr(v) Serial.print(v)
#define debug(v) Serial.println(v)
#define debugf(s, values...) { sprintf(debug_buffer, s, ##values); Serial.println(debug_buffer); }

#else

#define debugr(v)
#define debug(v)
#define debugf(s, values...)

#endif

#pragma region GLOBAL_SETTINGS

void(* resetBoard) (void) = 0;                  // reset Arduino Uno function
const char*     program_version = "0.8.3";
unsigned long   serial_baud     = 115200;       // debug serial baud speed

unsigned short  broadcast_id    = 999;          // id for receiving broadcast messages (for all devices)
EasyTransfer    easy_transfer;                  // RS485 message exchanger
Message         message;                        // exchangeable object

#pragma endregion //GLOBAL_SETTINGS

#pragma region V_RS485

uint8_t         rs_rx_pin   = 2;                // receive pin
uint8_t         rs_tx_pin   = 3;                // transmit pin
uint8_t         rs_pwr_pin  = 9;                // power (5v) pin
unsigned long   rs_baud     = 9600;             // baud speed
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // custom rx\tx serial

#pragma endregion //V_RS485

#pragma region V_ETHERNET

EthernetClient  client;                         // object for connecting server as client
byte            mac[] = { 0x54, 0x34, 
                          0x41, 0x30, 
                          0x30, 0x35 };
unsigned short  reconnect_delay = 1000;         // delay for try to reconnect ethernet

#pragma endregion //V_ETHERNET

#pragma region V_SERVER

unsigned short  server_port             = 80;   // default HTTP server port
const char*     server_name =                 
                "skdmk.fd.mk.ua";               // server address (host name)
const char*     server_request = 
                "GET /skd.mk/baseadd2.php?";    // GET request address

// smart receive
unsigned short  server_receive_max_wait = 500;  // time for waiting response from server
unsigned short  server_receive_counter  = 0;    // counter for receive waiting

#pragma endregion //V_SERVER

#pragma region SERVER_STATES
                                                
// responses:
const uint8_t   st_unknown          = 0;        // unknown state
const uint8_t   st_allow            = 1;        // access allowed
const uint8_t   st_re_entry         = 2;        // re-entry
const uint8_t   st_denied           = 3;        // access denied
const uint8_t   st_invalid          = 4;        // invalid card (database does not contain such card)
const uint8_t   st_blocked          = 5;        // card is blocked

// errors:
const uint8_t   er_no_srvr_cnctn    = 95;       // no server connection             | !client.connect(server, port)
const uint8_t   er_request          = 96;       // wrong server request             | !client.find("\r\n\r\n")
const uint8_t   er_no_response      = 97;       // no response from server          | json {"id":0,"kod":0,"status":0}
const uint8_t   er_json             = 98;       // json deserialization error       | if (DeserializationError)
const uint8_t   er_timeout          = 99;       // server connection timeout        | !client.available()
const uint8_t   er_no_ethr_cnctn    = 100;      // no ethernet connection           | !ethernetConnected()

#pragma endregion //SERVER_STATES

#pragma region F_DECLARATION

// returns true if ethernet connection established
bool ethernetConnected();

// recursive function for establishing DHCP connection
void ethernetConnect();

// send data to server
void sendServer();

// receive response from server
void receiveServer();

// send message to slave
void sendData(unsigned short device_id, unsigned long card_id, unsigned short state_id, unsigned short other_id);

// send broadcast message (for all of devices connected by RS485)
void sendBroadcast(unsigned short state_id, unsigned short other_id);

#pragma endregion //F_DECLARATION

void setup()
{
    pinMode(rs_pwr_pin, OUTPUT);
    digitalWrite(rs_pwr_pin, HIGH);
    rs485.begin(rs_baud);
    easy_transfer.begin(details(message), &rs485);

    #if DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    #endif //DEBUG
    debugf("\n\n\n\tArduino Uno Ethernet Sender %s.\n\tdebug serial speed: %lu\n", program_version, serial_baud);

    SPI.begin();
    ethernetConnect();

    // //test
    // //debug("test receive");
    // message.set(801, 123456789, 0 , 0);
    // sendServer();
    // receiveServer();

    // delay(5000);
    // //debug("test receive");
    // message.set(801, 123456789, 0 , 0);
    // sendServer();
    // receiveServer();
}

void loop()
{
    if (!ethernetConnected())
    {
        ethernetConnect();
    }

    if (easy_transfer.receiveData())
    {
        debugf("|ET-receive|\t[ %u; %lu; %u; %u ]", 
            message.device_id, message.card_id, message.state_id, message.other_id);

        sendServer();
        receiveServer();
    }
}

#pragma region FUNCTION_DESCRIPTION

bool ethernetConnected()
{
    if (Ethernet.linkStatus() == LinkON && 
        Ethernet.hardwareStatus() != EthernetNoHardware)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ethernetConnect()
{
    sendBroadcast(er_no_ethr_cnctn, 0);

    debugf("ethernet info: link status = %u; hardware status = %u", 
        Ethernet.linkStatus(), Ethernet.hardwareStatus());
    debugr("connecting ethernet");
    while(!ethernetConnected())
    {
        delay(reconnect_delay);
        debugr(".");
    }
    debug("\nconnected");

    // connecting to network (using mac address and DHCP)
    if (Ethernet.begin(mac) == 0)
    {
        debug("critical error: DHCP configuration failed. reloading board");

        delay(reconnect_delay);

        resetBoard();
    }

    debugf("DHCP configuration succeeded. ip: %u.%u.%u.%u", 
        Ethernet.localIP()[0], Ethernet.localIP()[1], Ethernet.localIP()[2], Ethernet.localIP()[3]);

    sendBroadcast(er_no_ethr_cnctn, 1);
}

void sendServer()
{
    // no ethernet or server connection, trying to reconnect
    if (!client.connect(server_name, server_port))
    {
        debug("server connection not established");
        sendData(
            message.device_id, 
            message.card_id, 
            er_no_srvr_cnctn,
            0
        );
        return;
    }
    
    // connection established
    client.print(server_request);
    client.print("id=");
    client.print(message.card_id);
    client.print("&kod=");
    client.print(message.device_id);
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server_name);
    client.println("Connection: close");
    client.println();
    client.println();

    debugf("web request send:\n%sid=%lu&kod=%u HTTP/1.1\nHost: %s\nConnection: close\n_\n_", 
        server_request, message.card_id, message.device_id, server_name);

    // rare error check
    if (!client.find("\r\n\r\n"))
    {
        debug("invalid request (not sent)");
        client.stop();
        sendData(
            message.device_id,
            message.card_id,
            er_request,
            0
        );
    }
}

void receiveServer()
{
    // smart receive waiting
    server_receive_counter = 0;
    while (server_receive_counter <= server_receive_max_wait)
    {
        if (client.available())
            break;

        delay(1);
        server_receive_counter++;
    }

    // available data in cleint for read
    if (client.available())
    {
        StaticJsonDocument<256> json;
        DeserializationError error = deserializeJson(json, client);

        // deserialization error
        if (error)
        {
            debugf("deserialization error: %s", error.c_str());
            
            sendData(
                message.device_id, 
                message.card_id, 
                er_json, 
                0
            );
        }
        // successfully received response from server
        else
        {
            unsigned short  json_device_id = strtoul(json["kod"].as<const char*>(), NULL, 0);
            unsigned long   json_card_id   = strtoul(json["id"].as<const char*>(), NULL, 0);
            unsigned short  json_state_id  = json["status"].as<unsigned short>();

            debugf("json received: \t{ 'kod='%u; 'id'%lu, 'status'%u }", 
                json_device_id, json_card_id, json_state_id);

            // no response from server
            if (json_device_id == 0 && json_card_id == 0 && json_state_id == 0)
            {
                debug("error: no response from server");
                sendData(
                    message.device_id,
                    message.card_id,
                    er_no_response,
                    0
                );
            }
            // correct response
            else
            {
                sendData(
                    json_device_id,
                    json_card_id,
                    json_state_id,
                    0
                );
            }
        }
    }
    else
    {
        debug("error: response timeout");

        sendData(
            message.device_id,
            message.card_id,
            er_timeout,
            0
        );
    }

    client.stop();
}

void sendData(unsigned short device_id, unsigned long card_id, unsigned short state_id, unsigned short other_id)
{
    message.set(device_id, card_id, state_id, other_id);

    debugf("|ET-send|\t[ %u; %lu; %u; %u ]", 
        message.device_id, message.card_id, message.state_id, message.other_id);

    easy_transfer.sendData();

    message.clean();
}

void sendBroadcast(unsigned short state_id, unsigned short other_id)
{
    message.set(broadcast_id, 0, state_id, other_id);

    debugf("|ET-send|\t[ %u; %lu; %u; %u ]", 
        message.device_id, message.card_id, message.state_id, message.other_id);

    easy_transfer.sendData();

    message.clean();
}

#pragma endregion //FUNCTION_DESCRIPTION
