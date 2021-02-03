#include "lib/DW1000.cpp"
const int A_CSN = 4;
const int A_RST = 7;

double OFFSET = 100;
unsigned long updateTime;
unsigned long disconnectTimer;

void setup() {
    Init(A_CSN, A_RST);

    updateTime = millis();
    disconnectTimer = millis();
}

bool timeout = false;

uint64_t DelayedTx(int CSN) {
    ForceTRxOff(CSN);
    ClearTransmitStatus(CSN);

    byte buf[5];
    GetReceiveTimestamp(CSN, buf);
    uint64_t sendTime = buf[0]; sendTime <<= 8;
    sendTime += buf[1]; sendTime <<= 8;
    sendTime += buf[2]; sendTime <<= 8;
    sendTime += buf[3]; sendTime <<= 8;
    sendTime += buf[4];
    sendTime >>= 25;
    sendTime += 2;
    sendTime <<= 25;
    SetDelayedTime(CSN, sendTime);

    SetTransmitData(CSN, 1, (const byte[]){0x00});
    StartTransmit(CSN, true, false);
    unsigned long startTime = micros();
    while(!isTransmitDone(CSN)) {
        delayMicroseconds(40);
        if (micros() - startTime > 2000) {
            timeout = true;
            break;
        }
    }
    return sendTime;
}

uint64_t WaitForResponse(int CSN) {
    StartReceive(CSN, false);

    unsigned long startTime = micros();
    while(!isReceiveDone(CSN)) {
        delayMicroseconds(40);
        if (micros() - startTime > 2000) {
            timeout = true;
            break;
        }
    }

    byte buf[5];
    GetReceiveTimestamp(CSN, buf);
    uint64_t timestamp = buf[0]; timestamp <<= 8;
    timestamp += buf[1]; timestamp <<= 8;
    timestamp += buf[2]; timestamp <<= 8;
    timestamp += buf[3]; timestamp <<= 8;
    timestamp += buf[4];

    delayMicroseconds(100);
    return timestamp;
}

uint64_t GetReplyTime(int CSN) {
    byte data[10];
    uint64_t replyTime;

    GetReceiveData(CSN, 10, data);
    replyTime |= data[0]; replyTime <<= 8;
    replyTime |= data[1]; replyTime <<= 8;
    replyTime |= data[2]; replyTime <<= 8;
    replyTime |= data[3]; replyTime <<= 8;
    replyTime |= data[4];
    return replyTime;
}

uint64_t GetRoundTime(int CSN) {
    byte data[10];
    uint64_t roundTime;

    GetReceiveData(CSN, 10, data);
    roundTime |= data[5]; roundTime <<= 8;
    roundTime |= data[6]; roundTime <<= 8;
    roundTime |= data[7]; roundTime <<= 8;
    roundTime |= data[8]; roundTime <<= 8;
    roundTime |= data[9];
    return roundTime;
}

double GetDistance(int CSN) {
    timeout = false;

    uint64_t round1A;
    uint64_t round1B;
    uint64_t round1;
    uint64_t reply1;
    uint64_t reply2;
    uint64_t round2;
    uint64_t propTime;

    round1A = DelayedTx(CSN);
    round1B = WaitForResponse(CSN);
    reply1 = GetReplyTime(CSN);
    round1 = round1B - round1A;
    reply2 = DelayedTx(CSN) - round1B;
    WaitForResponse(CSN);
    if (timeout) {
        return 0;
    }
    round2 = GetRoundTime(CSN);
    propTime = (round1 * round2 - reply1 * reply2) / (round1 + round2 + reply1 + reply2);

    double prop = (double)(0x00000000FFFFFFFF & propTime);
    if (prop > 10000) {
        return 0;
    } else {
        return prop * 4.6917639786 - OFFSET;
    }
}

bool disconnected = false;

void loop() {
    double distance = GetDistance(A_CSN);

    if (timeout) {
        if (millis() - disconnectTimer > 50) {
            Init(A_CSN, A_RST);
            disconnected = true;
        }
    } else {
        disconnected = false;
        disconnectTimer = millis();
    }

    if (distance) {
        Serial.println(distance);
    }
}
