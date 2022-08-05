#include <SPI.h>
#include <mcp_can.h>

long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
unsigned char cmbBuf[8];

#define CAN0_INT 2 // Set INT to pin 2
MCP_CAN CAN0(10); // Set CS to pin 10

void setup()
{
    Serial.begin(115200);

    if (CAN0.begin(MCP_ANY, CAN_125KBPS, MCP_8MHZ) == CAN_OK)
        Serial.println("MCP2515 Initialized Successfully!");
    else
        Serial.println("Error Initializing MCP2515...");

    CAN0.setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.

    pinMode(CAN0_INT, INPUT); // Configuring pin for /INT input
}

bool carBtn = false;
bool carBtnReacted = false;
unsigned long carBtnPressed = 0;

void loop()
{
    if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
    {
        CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
        if ((rxId & 0x40000000) != 0x40000000) {
            if (rxId == 0x122 /* Piano message */) {
                if (rxBuf[2] & 0x01 == 0x01 /* Car button */) {
                    if (!carBtn) {
                        carBtn = true;
                        carBtnReacted = false;
                        carBtnPressed = millis();
                    }
                } else {
                    carBtn = false;
                    carBtnReacted = false;
                }
            } else if (rxId == 0x217 /* CMB buttons message */) {
                for (byte i = 0; i < len; i++) {
                    cmbBuf[i] = rxBuf[i];
                }
            }
        }
    }

    if (carBtn && !carBtnReacted && (millis() - carBtnPressed > 500)) {
        carBtnReacted = true;
        cmbBuf[4] = cmbBuf[4] | 0x01;
        CAN0.sendMsgBuf(0x217, 8, 8, cmbBuf);
        Serial.println("Car button long press detected");
    }
}