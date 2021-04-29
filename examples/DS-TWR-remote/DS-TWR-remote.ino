#include "lib/DW1000.cpp"
const int A_CSN = 4;
const int A_RST = 7;

unsigned long disconnectTimer;

void setup() {
    SPI.begin();

    Init(A_CSN, A_RST);
    disconnectTimer = millis();
}

uint64_t previousTx;
bool timeout;

uint64_t DelayedTx() {
    ForceTRxOff(A_CSN);
    ClearTransmitStatus(A_CSN);

    uint64_t sendTime = getTimestamp(A_CSN) >> 25;
    sendTime += 2;
    sendTime <<= 25;
    SetDelayedTime(A_CSN, sendTime);

    byte msg[10];
    uint64_t replyTime;
    uint64_t roundTime;
    uint64_t receiveTime = getReceiveTimestamp(A_CSN);
    replyTime = sendTime - receiveTime;
    roundTime = receiveTime - previousTx;
    msg[0] = 0xFF&(replyTime>>32);
    msg[1] = 0xFF&(replyTime>>24);
    msg[2] = 0xFF&(replyTime>>16);
    msg[3] = 0xFF&(replyTime>>8);
    msg[4] = 0xFF&replyTime;
    msg[5] = 0xFF&(roundTime>>32);
    msg[6] = 0xFF&(roundTime>>24);
    msg[7] = 0xFF&(roundTime>>16);
    msg[8] = 0xFF&(roundTime>>8);
    msg[9] = 0xFF&roundTime;
    SetTransmitData(A_CSN, 10, msg);
    StartTransmit(A_CSN, true, false);
    unsigned long startTime = micros();
    while(!isTransmitDone(A_CSN)) {
        delayMicroseconds(40);
        if (micros() - startTime > 2000) {
            timeout = true;
            break;
        }
    }

    return sendTime;
}

void loop() {
    timeout = false;
    ForceTRxOff(A_CSN);
    StartReceive(A_CSN, false);
    unsigned long startTime = millis();
    while(!isReceiveDone(A_CSN)) {
        delayMicroseconds(40);
        if (millis() - startTime > 5) {
            timeout = true;
            break;
        }
    }
    
    previousTx = DelayedTx();
    if (timeout) {
        if (millis() - disconnectTimer > 100) {
           Init(A_CSN, A_RST);
        }
    } else {
        disconnectTimer = millis();
    }
}
