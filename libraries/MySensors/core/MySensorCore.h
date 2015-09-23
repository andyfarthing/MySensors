/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */


#ifndef MySensorCore_h
#define MySensorCore_h

#include "Version.h"   // Auto generated by bot
#include "MyConfig.h"
#include "MyMessage.h"
#ifdef MY_OTA_FIRMWARE_FEATURE
#include "drivers/SPIFlash/SPIFlash.h"
#endif

#include <stddef.h>
#include <stdarg.h>

#ifdef MY_DEBUG
#define debug(x,...) hwDebugPrint(x, ##__VA_ARGS__)
#else
#define debug(x,...)
#endif

// EEPROM start address for mysensors library data
#define EEPROM_START 0
// EEPROM location of node id
#define EEPROM_NODE_ID_ADDRESS EEPROM_START
// EEPROM location of parent id
#define EEPROM_PARENT_NODE_ID_ADDRESS (EEPROM_START+1)
// EEPROM location of distance to gateway
#define EEPROM_DISTANCE_ADDRESS (EEPROM_PARENT_NODE_ID_ADDRESS+1)
#define EEPROM_ROUTES_ADDRESS (EEPROM_DISTANCE_ADDRESS+1) // Where to start storing routing information in EEPROM. Will allocate 256 bytes.
#define EEPROM_CONTROLLER_CONFIG_ADDRESS (EEPROM_ROUTES_ADDRESS+256) // Location of controller sent configuration (we allow one payload of config data from controller)
#define EEPROM_FIRMWARE_TYPE_ADDRESS (EEPROM_CONTROLLER_CONFIG_ADDRESS+24)
#define EEPROM_FIRMWARE_VERSION_ADDRESS (EEPROM_FIRMWARE_TYPE_ADDRESS+2)
#define EEPROM_FIRMWARE_BLOCKS_ADDRESS (EEPROM_FIRMWARE_VERSION_ADDRESS+2)
#define EEPROM_FIRMWARE_CRC_ADDRESS (EEPROM_FIRMWARE_BLOCKS_ADDRESS+2)
#define EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS (EEPROM_FIRMWARE_CRC_ADDRESS+2)
#define EEPROM_LOCAL_CONFIG_ADDRESS (EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS+32) // First free address for sketch static configuration

// This is the nodeId for sensor net gateway receiver sketch (where all sensors should send their data).
#define GATEWAY_ADDRESS ((uint8_t)0)
// Node child is always created/presented when a node is started
#define NODE_SENSOR_ID 0xFF


struct NodeConfig
{
	uint8_t nodeId; // Current node id
	uint8_t parentNodeId; // Where this node sends its messages
	uint8_t distance; // This nodes distance to sensor net gateway (number of hops)
};

struct ControllerConfig {
	uint8_t isMetric;
};



/**
 * Return this nodes id.
 */
uint8_t getNodeId();

/**
* Each node must present all attached sensors before any values can be handled correctly by the controller.
* It is usually good to present all attached sensors after power-up in setup().
*
* @param sensorId Select a unique sensor id for this sensor. Choose a number between 0-254.
* @param sensorType The sensor type. See sensor typedef in MyMessage.h.
* @param description A textual description of the sensor.
* @param ack Set this to true if you want destination node to send ack back to this node. Default is not to request any ack.
* @param description A textual description of the sensor.
*/
void present(uint8_t sensorId, uint8_t sensorType, const char *description="", bool ack=false);

/**
 * Sends sketch meta information to the gateway. Not mandatory but a nice thing to do.
 * @param name String containing a short Sketch name or NULL  if not applicable
 * @param version String containing a short Sketch version or NULL if not applicable
 * @param ack Set this to true if you want destination node to send ack back to this node. Default is not to request any ack.
 *
 */
void sendSketchInfo(const char *name, const char *version, bool ack=false);

/**
* Sends a message to gateway or one of the other nodes in the radio network
*
* @param msg Message to send
* @param ack Set this to true if you want destination node to send ack back to this node. Default is not to request any ack.
* @return true Returns true if message reached the first stop on its way to destination.
*/
bool send(MyMessage &msg, bool ack=false);


/**
 * Send this nodes battery level to gateway.
 * @param level Level between 0-100(%)
 * @param ack Set this to true if you want destination node to send ack back to this node. Default is not to request any ack.
 *
 */
void sendBatteryLevel(uint8_t level, bool ack=false);

/**
 * Send a heartbeat message (I'm alive!) to the gateway/controller.
 * The payload will be an incremental 16 bit integer value starting at 1 when sensor is powered on.
 */
void sendHeartbeat(void);

/**
* Requests a value from gateway or some other sensor in the radio network.
* Make sure to add callback-method in begin-method to handle request responses.
*
* @param childSensorId  The unique child id for the different sensors connected to this Arduino. 0-254.
* @param variableType The variableType to fetch
* @param destination The nodeId of other node in radio network. Default is gateway
*/
void request(uint8_t childSensorId, uint8_t variableType, uint8_t destination=GATEWAY_ADDRESS);

/**
 * Requests time from controller. Answer will be delivered to receiveTime function in sketch.
 *
 */
void requestTime();



/**
 * Returns the most recent node configuration received from controller
 */
ControllerConfig getConfig();

/**
 * Save a state (in local EEPROM). Good for actuators to "remember" state between
 * power cycles.
 *
 * You have 256 bytes to play with. Note that there is a limitation on the number
 * of writes the EEPROM can handle (~100 000 cycles on ATMega328).
 *
 * @param pos The position to store value in (0-255)
 * @param value to store in position
 */
void saveState(uint8_t pos, uint8_t value);

/**
 * Load a state (from local EEPROM).
 *
 * @param pos The position to fetch value from  (0-255)
 * @return Value to store in position
 */
uint8_t loadState(uint8_t pos);

/**
 * Wait for a specified amount of time to pass.  Keeps process()ing.
 * This does not power-down the radio nor the Arduino.
 * Because this calls process() in a loop, it is a good way to wait
 * in your loop() on a repeater node or sensor that listens to messages.
 * @param ms Number of milliseconds to sleep.
 */
void wait(unsigned long ms);

/**
 * Sleep (PowerDownMode) the MCU and radio. Wake up on timer.
 * @param ms Number of milliseconds to sleep.
 */
void sleep(unsigned long ms);
void smartSleep(unsigned long ms);

/**
 * Sleep (PowerDownMode) the MCU and radio. Wake up on timer or pin change.
 * See: http://arduino.cc/en/Reference/attachInterrupt for details on modes and which pin
 * is assigned to what interrupt. On Nano/Pro Mini: 0=Pin2, 1=Pin3
 * @param interrupt Interrupt that should trigger the wakeup
 * @param mode RISING, FALLING, CHANGE
 * @param ms Number of milliseconds to sleep or 0 to sleep forever
 * @return true if wake up was triggered by pin change and false means timer woke it up.
 */
bool sleep(uint8_t interrupt, uint8_t mode, unsigned long ms=0);
bool smartSleep(uint8_t interrupt, uint8_t mode, unsigned long ms=0);

/**
 * Sleep (PowerDownMode) the MCU and radio. Wake up on timer or pin change for two separate interrupts.
 * See: http://arduino.cc/en/Reference/attachInterrupt for details on modes and which pin
 * is assigned to what interrupt. On Nano/Pro Mini: 0=Pin2, 1=Pin3
 * @param interrupt1 First interrupt that should trigger the wakeup
 * @param mode1 Mode for first interrupt (RISING, FALLING, CHANGE)
 * @param interrupt2 Second interrupt that should trigger the wakeup
 * @param mode2 Mode for second interrupt (RISING, FALLING, CHANGE)
 * @param ms Number of milliseconds to sleep or 0 to sleep forever
 * @return Interrupt number wake up was triggered by pin change and negative if timer woke it up.
 */
int8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms=0);
int8_t smartSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms=0);


/******  PRIVATE ********/

void _begin();

void _process(void);

void _processInternalMessages();

boolean _sendRoute(MyMessage &message);

extern NodeConfig _nc;
extern MyMessage _msg;  // Buffer for incoming messages.
extern MyMessage _msgTmp;  // Buffer for temporary messages (acks and nonces among others).
#ifdef MY_DEBUG
	extern char _convBuf[MAX_PAYLOAD*2+1];
#endif
void receive(const MyMessage &message)  __attribute__((weak));
void receiveTime(unsigned long)  __attribute__((weak));
void presentation()  __attribute__((weak));
void setup() __attribute__((weak));
void loop() __attribute__((weak));


// Inline function and macros
static inline MyMessage& build(MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.sender = sender;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

static inline MyMessage& buildGw(MyMessage &msg, uint8_t type) {
	msg.sender = GATEWAY_ADDRESS;
	msg.destination = GATEWAY_ADDRESS;
	msg.sensor = 0;
	msg.type = type;
	mSetCommand(msg, C_INTERNAL);
	mSetRequestAck(msg, false);
	mSetAck(msg, false);
	return msg;
}


#endif
