/*
 ARDUINO UNO ETHERNET SENDER {ethernet shield 2 + rs485}
 Name:		ArduinoUnoEthernetSender.ino
 Created:	23.06.2022 10:00:00
 Author:	Dobychyn Danil
*/

#include <Arduino.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#include <EthernetClient.h>
#include <Ethernet.h>
//#include <Ethernet3.h>
#include <EasyTransfer.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <SPI.h>

#define DEBUG true

#if DEBUG
char debug_buffer[256];
#define debugr(v) Serial.print(v)
#define debug(v) Serial.println(v)
#define debugf(s, values...) { sprintf(debug_buffer, s, ##values); Serial.println(debug_buffer); }
#define debugt
#else

#endif

#pragma region GLOBAL_SETTINGS

void(* resetFunc) (void) = 0;                   // reset Arduino Uno function
const char*     program_version = "0.9.7";      // program version
unsigned long   serial_baud     = 115200;       // serial baud speed

unsigned short  broadcast_id    = 999;          // id for receiving broadcast messages
EasyTransfer    easy_transfer;                  // object for exchanging data using RS485
Message         message;                        // exchangeable object for EasyTransfer

#pragma endregion //GLOBAL_SETTINGS

#pragma region V_RS485

uint8_t         rs_rx_pin = 2;                  // RS485 receive pin
uint8_t         rs_tx_pin = 3;                  // RS485 transmit pin
unsigned long   rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485

#pragma endregion //V_RS485

#pragma region V_ETHERNET

EthernetClient  client;                         // object for connecting ethernet as client
byte            mac[] = { 0x54, 0x34, 
                          0x41, 0x30, 
                          0x30, 0x35 };         // mac address of this device. must be unique in local network
unsigned short  reconnect_delay = 1000;          // delay for try to reconnect ethernet

#pragma endregion //V_ETHERNET

#pragma region V_SERVER

unsigned short  server_port             = 80;   // server port
char*           server_name =                 
                "skdmk.fd.mk.ua";               // server address
String          server_request = 
                "GET /skd.mk/baseadd2.php?";     // GET request address

// smart receive
unsigned long   server_receive_max_wait    = 500;  // time for waiting response from server
unsigned short  server_receive_counter  = 0;    // counter for receive waiting

#pragma endregion //V_SERVER

#pragma region V_SERVER_STATES
                                                
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

#pragma endregion //V_SERVER_STATES

#pragma region F_DECLARATION

// returns true if ethernet connection established
bool ethernetConnected();

// recursive function for establishing DHCP connection
void ethernetConnect();

// send data to server
void sendServer();

// receive response from server
void receiveServer();

// send message to slave (Arduino Nano)
void sendData(unsigned short device_id, unsigned long card_id, unsigned short state_id, unsigned short other_id);

// send broadcast message (for all of devices connected by RS485)
void sendBroadcast(unsigned short state_id, unsigned short other_id);

#pragma endregion //F_DECLARATION

void setup()
{
    #if DEBUG
    Serial.begin(serial_baud);
    #endif

    rs485.begin(rs_baud);
    SPI.begin();
    easy_transfer.begin(details(message), &rs485);

    debugf("\n\n\nArduino Uno Ethernet Sender v.%s \ndebug serial speed: %lu", 
        program_version, 
        serial_baud);

    ethernetConnect();
}

void loop()
{
    if (!ethernetConnected())
    {
        ethernetConnect();
    }

    if (easy_transfer.receiveData())
    {
        debugf("RECEIVED:\t[ %u, %lu, %u, %u ]", 
            message.device_id, 
            message.card_id, 
            message.state_id, 
            message.other_id);

        sendServer();

        receiveServer();
    }
}

#pragma region F_DESCRIPTION

bool ethernetConnected()
{
    //if (Ethernet.linkStatus() == LinkON) //DOES NOT WORK
    // if (Ethernet.linkStatus() == 0)
    //     return true;
    // else
    //     return false;
    return true;
}

void ethernetConnect()
{
    
    // connecting to network (using mac address and DHCP)
    if (Ethernet.begin(mac) == 0)
    {
        delay(reconnect_delay);

        debug("connection via DHCP is not established. reloading board...");

        //resetFunc();
        ethernetConnect();
    }

    debugf("DHCP connection established. ip (%u.%u.%u.%u). link status: %d. hardware status: %d", 
        Ethernet.localIP()[0],
        Ethernet.localIP()[1],
        Ethernet.localIP()[2],
        Ethernet.localIP()[3],
        Ethernet.linkStatus(),
        Ethernet.hardwareStatus());

    sendBroadcast(er_no_ethr_cnctn, 1);
}

void sendServer()
{
    // no ethernet or server connection, trying to reconnect
    if (!client.connect(server_name, server_port))
    {
        debug("server connection not established");
        if (!ethernetConnected())
        {
            debug("ethernet connection not established");
            sendData(
                message.device_id,
                message.card_id,
                er_no_ethr_cnctn,
                0
            );
            ethernetConnect();
        }
        else
        {
            sendData(
                message.device_id, 
                message.card_id, 
                er_no_srvr_cnctn,
                0
            );
        }
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

    debug("request send: \n" + 
        server_request + "id=" + String(message.card_id) + 
        "&kod=" + String(message.device_id) + " HTTP/1.1" +
        "\nHost: " + server_name + 
        "\nConnection: close");
    
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
        debug("client available");
        StaticJsonDocument<256> json;
        DeserializationError error = deserializeJson(json, client);
        
        // deserialization error
        if (error)
        {
            debug("deserialization error: " + String(error.c_str()));
            
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
            unsigned short json_device_id = strtoul(json["kod"].as<const char*>(), NULL, 0);
            unsigned long  json_card_id   = strtoul(json["id"].as<const char*>(), NULL, 0);
            unsigned short json_state_id  = json["status"].as<int>();

            // no response from server
            if (json_device_id == 0 &&
                json_card_id == 0 &&
                json_state_id == 0)
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

    debugf("SEND:\t\t[ %u, %lu, %u, %u ]", 
        message.device_id, 
        message.card_id, 
        message.state_id, 
        message.other_id);

    easy_transfer.sendData();

    message.clean();
}

void sendBroadcast(unsigned short state_id, unsigned short other_id)
{
    message.set(broadcast_id, 0, state_id, other_id);

    debugf("Broad-SEND:\t[ %u, %lu, %u, %u ]", 
        message.device_id, 
        message.card_id, 
        message.state_id, 
        message.other_id);

    easy_transfer.sendData();

    message.clean();
}

#pragma endregion //F_DESCRIPTION
