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
#include <Ethernet2.h>
#include <EasyTransfer.h>
#include <Message.h>
#include <SoftwareSerial.h>
#include <SPI.h>

#pragma region GLOBAL_SETTINGS

#define DEBUG                                   // comment this line to not write anything to Serial as debug
const String    program_version = "0.9.0";      // program version
unsigned long   serial_baud     = 115200;       // serial baud speed

EasyTransfer    easy_transfer;                  // object for exchanging data between RS485
Message         message;                        // object for exchanging data between ArduinoNano and ArduinoUno

#pragma endregion

#pragma region RS485_SETTINGS

uint8_t         rs_rx_pin = 2;                  // RS485 receive pin
uint8_t         rs_tx_pin = 3;                  // RS485 transmit pin
long            rs_baud   = 9600;               // RS485 baud rate (speed)
SoftwareSerial  rs485(rs_rx_pin, rs_tx_pin);    // object for receiving and transmitting data via RS485

#pragma endregion

#pragma region ETHERNET_SETTINGS

EthernetClient  client;                         // object for connecting ethernet as client
byte            mac[] = { 0x54, 0x34, 
                          0x41, 0x30, 
                          0x30, 0x35 };         // mac address of this device. must be unique in local network
uint8_t         reconnect_delay = 20;           // delay for try to reconnect ethernet

#pragma endregion

#pragma region SERVER_SETTINGS

int             server_port             = 80;   // server port
char*           server_name =                 
                "skdmk.fd.mk.ua";               // server address
String          server_request = 
                "GET /skd.mk/baseadd.php?";     // GET request address

// smart receive
unsigned long   server_receive_max_wait = 600;  // time for waiting response from server
uint8_t         server_receive_counter  = 0;    // counter for receive waiting

#pragma endregion

#pragma region SERVER_STATES

const uint8_t   state_denied        = 0;        // access denied
const uint8_t   state_no_response   = 97;       // no response from server
const uint8_t   state_json_error    = 98;       // json deserialization error
const uint8_t   state_timeout_error = 99;       // server connection timeout
const uint8_t   state_ok            = 1;        // accessed

#pragma endregion

// recursive function for connecting ethernet using DHCP
bool ethernetConnect();

// send data to server
void sendServer();

// receive response from server
void receiveServer();

// send message to ArduinoNano
void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id);

void setup()
{
    rs485.begin(rs_baud);
    SPI.begin();
    easy_transfer.begin(details(message), &rs485);

#ifdef DEBUG
    Serial.begin(serial_baud);
    while (!Serial);
    Serial.println(
        String("start working...") + 
        String("\nprogram version: ") + program_version +
        String("\nserial baud speed: ") + serial_baud);
#endif //DEBUG

    ethernetConnect();
}

void loop()
{
    if (easy_transfer.receiveData())
    {
#ifdef DEBUG
        Serial.println("received message: ");
        message.print();
        Serial.println("*****\n");
#endif //DEBUG

        sendServer();
    }
}

#pragma region FUNCTIONS_DESCRIPTION

bool ethernetConnect()
{
#ifdef DEBUG
    Serial.println("connecting ethernet...");
#endif //DEBUG

    if (Ethernet.begin(mac) == 0)
    {
#ifdef DEBUG
        Serial.println("connection via DHCP is not established");
        Serial.println("trying to reconnect...");
#endif //DEBUG
        delay(reconnect_delay);
        ethernetConnect();
    }

#ifdef DEBUG
    Serial.println("DHCP connection established");
    Serial.print("ip: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print(Ethernet.localIP()[i]);
        Serial.print(".");
    }
    Serial.println(", subnet mask: ");
    for (int i = 0; i < 4; i++)
    {
        Serial.print(Ethernet.subnetMask()[i]);
        Serial.print(".");
    }
#endif //DEBUG

    return true;
}

void sendServer()
{
    if (client.connect(server_name, server_port))
    {
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

#ifdef DEBUG
        Serial.println("send http request:");
        Serial.print(server_request);
        Serial.print("id=");
        Serial.print(message.card_id);
        Serial.print("&kod=");
        Serial.print(message.device_id);
        Serial.println(" HTTP/1.1");
        Serial.print("Host: ");
        Serial.println(server_name);
        Serial.println("Connection: close");
        Serial.println();
        Serial.println();
#endif //DEBUG

        if (!client.find("\r\n\r\n"))
        {
#ifdef DEBUG
            Serial.println("invalid request, request not sent");
#endif //DEBUG
            client.stop();
            return;
        }

        receiveServer();
    }
    else
    {
#ifdef DEBUG
        Serial.println("client connect error");
        Serial.print("ethernet status: ");
        for (int i = 0; i < 8; i++)
            Serial.print(Ethernet._state[i] + " ");
        Serial.println();
        Serial.println(String("all statuses::") + 
                    "\nsock_close: " + Sock_CLOSE + 
                    "\nsock_connect: " + Sock_CONNECT + 
                    "\nsock_discon: " + Sock_DISCON + 
                    "\nsock_listen: " + Sock_LISTEN + 
                    "\nsock_open: " + Sock_OPEN + 
                    "\nsock_recv: " + Sock_RECV + 
                    "\nsock_send: " + Sock_SEND + 
                    "\nsock_send_keep: " + Sock_SEND_KEEP + 
                    "\nsock_send_mac: " + Sock_SEND_MAC);
#endif //DEBUG
        sendData(message.device_id, message.card_id, state_timeout_error, 0);
        if (ethernetConnect())
            return;
    }

    client.stop();
}

void receiveServer()
{
    // smart receive waiting
    server_receive_counter = 0;
    while (server_receive_counter < server_receive_max_wait)
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
#ifdef DEBUG
            Serial.print("deserialization error: ");
            Serial.println(error.c_str());
#endif // DEBUG
            
            sendData(
                message.device_id, 
                message.card_id, 
                state_json_error, 
                0);
        }
        // successfully received response from server
        else
        {
#ifdef DEBUG
            Serial.println("serialized data from json:");
            Serial.print("kod: ");
            Serial.println(json["kod"].as<uint_fast16_t>());
            Serial.print("id: ");
            Serial.println(json["id"].as<uint32_t>());
            Serial.print("status: ");
            Serial.println(json["status"].as<uint8_t>());
            Serial.print("size: ");
            Serial.println(json.size());
            Serial.println("*****\n");

            Serial.println("sending data from json...");
#endif //DEBUG
            sendData(
                json["kod"].as<uint_fast16_t>(),
                json["id"].as<uint32_t>(),
                json["status"].as<uint8_t>(),
                0);
        }
    }
    else
    {
#ifdef DEBUG
        Serial.println("error: no response from server");
#endif // DEBUG
        sendData(
            message.device_id,
            message.card_id,
            state_no_response,
            0);
    }
}

void sendData(uint_fast16_t device_id, uint32_t card_id, uint8_t state_id, uint8_t other_id)
{
#ifdef DEBUG
    Serial.println("send data:");
    message.print();
    Serial.println("*****\n");
#endif // DEBUG

    message.set(device_id, card_id, state_id, other_id);

    easy_transfer.sendData();

    message.clean();
}

#pragma endregion //FUNCTIONS_DECLARATION
