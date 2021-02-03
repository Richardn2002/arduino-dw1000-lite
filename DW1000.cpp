#include <Arduino.h>
#include <SPI.h>

const SPISettings SPI_FAST = SPISettings(16000000, MSBFIRST, SPI_MODE0);
const SPISettings SPI_SLOW = SPISettings(2000000, MSBFIRST, SPI_MODE0);
SPISettings _SPISettings;

void _SPITxn(int CSN, int headerLen, byte* header, int dataLen, byte data[], int respLen, byte resp[]) {
    SPI.beginTransaction(_SPISettings);
    digitalWrite(CSN, LOW);
    for (int i = 0; i < headerLen; i++) {
        SPI.transfer(header[i]);
    }
    for (int i = 0; i < dataLen; i++) {
        SPI.transfer(data[i]);
    }
    for (int i = 0; i < respLen; i++) {
        resp[i] = SPI.transfer(0x00);
    }
    delayMicroseconds(5);
    digitalWrite(CSN, HIGH);
    SPI.endTransaction();
}

byte* _header(bool writes, byte address) {
    static byte header[1];
    header[0] = (writes << 7) + address;

    return header;
}

byte* _headerWithIndex(bool writes, byte address, int index) {
    static byte header[2];
    header[0] = (writes << 7) + 0x40 + address;
    header[1] = index & 0x7F;

    return header;
}

byte* _headerWithExtendedIndex(bool writes, byte address, int index) {
    static byte header[3];
    header[0] = (writes << 7) + 0x40 + address;
    header[1] = 0x80 + (index & 0x7F);
    header[2] = (index >> 7) & 0xFF;

    return header;
}

void ReadReg(int CSN, byte address, int index, int respLen, byte resp[]) {
    if (!index) {
        _SPITxn(CSN, 1, _header(false, address), 0, NULL, respLen, resp);
    } else if (index < 0x7F) {
        _SPITxn(CSN, 2, _headerWithIndex(false, address, index), 0, NULL, respLen, resp);
    } else {
        _SPITxn(CSN, 3, _headerWithExtendedIndex(false, address, index), 0, NULL, respLen, resp);
    }
}

void WriteReg(int CSN, byte address, int index, int dataLen, byte data[]) {
    if (!index) {
        _SPITxn(CSN, 1, _header(true, address), dataLen, data, 0, NULL);
    } else if (index < 0x7F) {
        _SPITxn(CSN, 2, _headerWithIndex(true, address, index), dataLen, data, 0, NULL);
    } else {
        _SPITxn(CSN, 3, _headerWithExtendedIndex(true, address, index), dataLen, data, 0, NULL);
    }
}

constexpr byte OTP_IF = 0x2D;

void ReadOTP(int CSN, uint16_t address, int respLen, byte resp[]) {
    byte OTPAddr[2];
    OTPAddr[0] = address & 0xFF;
    OTPAddr[1] = (address >> 8) & 0xFF;
    WriteReg(CSN, OTP_IF, 0x04, 2, OTPAddr);
    WriteReg(CSN, OTP_IF, 0x06, 1, (const byte[]){0x03});
    WriteReg(CSN, OTP_IF, 0x06, 1, (const byte[]){0x01}); //Note: conflicts with the user manual example.
    ReadReg(CSN, OTP_IF, 0x0A, respLen, resp);
    WriteReg(CSN, OTP_IF, 0x06, 1, (const byte[]){0x00});
}

constexpr byte PMSC = 0x36;
constexpr byte SYS_AUTO_CLOCK = 0x00;
constexpr byte SYS_XTI_CLOCK  = 0x01;
constexpr byte SYS_PLL_CLOCK  = 0x02;

void enableClock(int CSN, byte clock) {
    byte pmscCtrl0[4];
    ReadReg(CSN, PMSC, 0, 2, pmscCtrl0);
    if(clock == SYS_AUTO_CLOCK) {
        pmscCtrl0[0] &= 0xFC;
        pmscCtrl0[0] = SYS_AUTO_CLOCK;
	} else if(clock == SYS_XTI_CLOCK) {
		pmscCtrl0[0] &= 0xFC;
		pmscCtrl0[0] |= 0x01;
	} else if(clock == SYS_PLL_CLOCK) {
		pmscCtrl0[0] &= 0xFC;
		pmscCtrl0[0] |= 0x02;
	}
	WriteReg(CSN, PMSC, 0, 2, pmscCtrl0);
    delay(5);
}

void Reset(int RST) { //This is an Arduino-specific way of reseting. Likely to change.
    pinMode(RST, OUTPUT);
    digitalWrite(RST, LOW);
    delay(2);
    pinMode(RST, INPUT);
    delay(5);
}

constexpr byte DEV_ID = 0x00;
constexpr byte EUI = 0x01;
constexpr byte PANADR = 0x03;
constexpr byte SYS_CFG = 0x04;
constexpr byte SYS_TIME = 0x06;
constexpr byte TX_FCTRL = 0x08;
constexpr byte TX_BUFFER = 0x09;
constexpr byte DX_TIME = 0x0A;
constexpr byte RX_WFTO = 0x0C;
constexpr byte SYS_CTRL = 0x0D;
constexpr byte SYS_MASK = 0x0E;
constexpr byte SYS_STATUS = 0x0F;
constexpr byte RX_FINFO = 0x10;
constexpr byte RX_BUFFER = 0x11;
constexpr byte RX_FQUAL = 0x12;
constexpr byte RX_TIME = 0x15;
constexpr byte TX_TIME = 0x17;

constexpr byte EC_CTRL = 0x24;
constexpr byte FS_CTRL = 0x2B;
constexpr int FS_XTALT_INDEX = 0x0E;

void Init(int CSN, int RST) {
    delay(5);
    pinMode(CSN, OUTPUT);
    digitalWrite(CSN, HIGH);
    Reset(RST);

    _SPISettings = SPI_SLOW;

    enableClock(CSN, SYS_XTI_CLOCK);

    //CPLL lock detect. Can rescue clock PLL losing lock warning.
    WriteReg(CSN, EC_CTRL, 0, 1, (const byte[1]){0x04});

    //Read clock calibration setting. If not present, use midrange setting.
    byte fsXtalt[1];
    byte otpBuf[1];
    ReadOTP(CSN, 0x001E, 1, otpBuf);
    if (!otpBuf[0]) {
        fsXtalt[0] = (otpBuf[0] & 0x1F) | 0x60;
	} else {
        fsXtalt[0] = (0x10 & 0x1F) | 0x60;
	}
    WriteReg(CSN, FS_CTRL, FS_XTALT_INDEX, 1, fsXtalt);

    //LDOTUNE

    //Load LDE microcode.
    WriteReg(CSN, PMSC, 0, 2, (const byte[]){0x01, 0x03}); //Switch to LDE_CLOCK.
    delay(5);
    WriteReg(CSN, OTP_IF, 0x06, 2, (const byte[]){0x00, 0x80}); //Load microcode.
    delay(1);
    WriteReg(CSN, PMSC, 0, 2, (const byte[]){0x00, 0x02}); //Switch to SYS_AUTO_CLOCK.
    delay(5);

    /*Collect needed information for voltage and temperature measurement
    ReadOTP(CSN, 0x0008, otpBuf); // Steals the definition above.
    _vmeas = otpBuf[0]; // Voltage = ((SAR_LVBAT – Vmeas) / 173) + 3.3
    ReadOTP(CSN, 0x0009, otpBuf);
    _tmeas = otpBuf[0]; // Temperature(°C ) = ((SAR_LTEMP – Tmeas) x 1.14) + 23
    */

    enableClock(CSN, SYS_AUTO_CLOCK);

    _SPISettings = SPI_FAST;

    //Initialization for deepsleep function. (AON:CFG1(0x2C:0x0A))

    //Tune according to default config: 6.8Mbps, default 16MHz, 128, PAC 8, code 4, channel 5, los optimization
    WriteReg(CSN, 0x23, 0x04, 2, (const byte[]){0x70, 0x88}); //AGC_TUNE1 for default 16MHz PRF
    WriteReg(CSN, 0x23, 0x0C, 4, (const byte[]){0x07, 0xA9, 0x02, 0x25}); //AGC_TUNE2
    WriteReg(CSN, 0x23, 0x12, 2, (const byte[]){0x35, 0x00}); //AGC_TUNE3
    WriteReg(CSN, 0x27, 0x02, 2, (const byte[]){0x01, 0x00}); //DRX_TUNE0b for standard SFD, 6.8Mbps
    WriteReg(CSN, 0x27, 0x04, 2, (const byte[]){0x87, 0x00}); //DRX_TUNE1a for 16Mhz PRF, 6.8Mbps
    WriteReg(CSN, 0x27, 0x06, 2, (const byte[]){0x20, 0x00}); //DRX_TUNE1b for preamble length 128, 6.8Mbps
    WriteReg(CSN, 0x27, 0x08, 4, (const byte[]){0x2D, 0x00, 0x1A, 0x31}); //DRX_TUNE2 for 16Mhz PRF, PAC size 8
    WriteReg(CSN, 0x27, 0x26, 2, (const byte[]){0x28, 0x00}); //DRX_TUNE4H for preamble length 128
    WriteReg(CSN, 0x2E, 0x0806, 1, (const byte[]){0x0D}); //LDE_CFG for better line-of-sight performance
    WriteReg(CSN, 0x2E, 0x1806, 2, (const byte[]){0x07, 0x16}); //LDE_CFG2 for better line-of-sight, 16MHz PRF
    WriteReg(CSN, 0x2E, 0x2804, 2, (const byte[]){0x8E, 0x42}); //LDE_REPC for preamble code 4, line-of-sight
    WriteReg(CSN, 0x1E, 0, 4, (const byte[]){0x48, 0x28, 0x08, 0x0E}); //TX_POWER for STXP, 16MHz PRF, channel 5
    WriteReg(CSN, 0x28, 0x0B, 1, (const byte[]){0xD8}); //RF_RXCTRLH for channel 5
    WriteReg(CSN, 0x28, 0x0C, 4, (const byte[]){0xE3, 0x3F, 0x1E, 0x00}); //RF_TXCTRL for channel 5
    WriteReg(CSN, 0x2A, 0x0B, 1, (const byte[]){0xB5}); //TC_PGDELAY for channel 5
    WriteReg(CSN, 0x2B, 0x07, 4, (const byte[]){0x1D, 0x04, 0x00, 0x08}); //FS_PLLCFG for channel 5
    WriteReg(CSN, 0x2B, 0x0B, 1, (const byte[]){0xBE}); //FS_PLLTUNE for channel 5

    delay(5);
}

void SetPAN_IDAndSHORT_ADDR(int CSN, uint16_t PAN_ID, uint16_t SHORT_ADDR) {
    byte buf[4];
    buf[0] = SHORT_ADDR & 0xFF;
    buf[1] = (SHORT_ADDR >> 8) & 0xFF;
    buf[2] = PAN_ID & 0xFF;
    buf[3] = (PAN_ID >> 8) & 0xFF;
    WriteReg(CSN, PANADR, 0, 4, buf);
}

void SetDelayedTime(int CSN, uint64_t delayedTime) {
    byte buf[5];
    buf[0] = 0x00;
    buf[1] = (delayedTime >> 8) & 0xFF;
    buf[2] = (delayedTime >> 16) & 0xFF;
    buf[3] = (delayedTime >> 24) & 0xFF;
    buf[4] = (delayedTime >> 32) & 0xFF;
    WriteReg(CSN, DX_TIME, 0, 5, buf);
}

void StartReceive(int CSN, bool delayed) {
    byte buf[2] = {0x00, 0x01};
    if (delayed) buf[1] = 0x03;
    WriteReg(CSN, SYS_CTRL, 0, 2, buf);
}

bool isReceiveTimestampAvailable(int CSN) {
    byte buf[2];
    ReadReg(CSN, SYS_STATUS, 0, 2, buf);
    return (buf[1] & 0x04) == 0x04;
}

bool isReceiveDone(int CSN) {
    byte buf[2];
    ReadReg(CSN, SYS_STATUS, 0, 2, buf);
    return (buf[1] & 0x60) == 0x60;
}

bool isReceiveError(int CSN) { //Note: Rejection due to frame filtering or timeout is NOT an error.
    byte buf[4];
    ReadReg(CSN, SYS_STATUS, 0, 4, buf);
    return (buf[2] & 0x05) | (buf[1] & 0x90);
}

void GetReceiveTimestamp(int CSN, byte data[]){
    ReadReg(CSN, RX_TIME, 0, 5, data);
}

void GetReceiveData(int CSN, int dataLen, byte data[]) {
    ReadReg(CSN, RX_BUFFER, 0, dataLen, data);
}

void ClearReceiveStatus(int CSN) {
    byte buf[4] = {0x00, 0b11111111, 0b00100111, 0b00100100};
    //Reset RXDFR RXFCG RXPRD RXSFDD RXPHD LDEDONE
    //Reset RXRFTO RXPTO RXSFDTO
    //Reset RXPHE RXFCE RXRFSL AFFREJ LDEERR
    WriteReg(CSN, SYS_STATUS, 0, 4, buf);
}

void ResetReceiver(int CSN) {
    byte pmscCtrl0[4];
    ReadReg(CSN, PMSC, 0, 4, pmscCtrl0);
	pmscCtrl0[0] &= 0xFC;
	pmscCtrl0[0] |= 0x01;
	WriteReg(CSN, PMSC, 0, pmscCtrl0, 4); //Switch to SYS_XTI_CLOCK
    pmscCtrl0[3] &= 0xEF;
    WriteReg(CSN, PMSC, 0, pmscCtrl0, 4);
    pmscCtrl0[3] |= 0x10;
    WriteReg(CSN, PMSC, 0, pmscCtrl0, 4);
    //Further note that clock adjustment and AON block
}

void StartAccMemRead(int CSN) {
    byte reg[2]; //Reference to the official deca_device.cpp _dwt_enableclocks
    ReadReg(CSN, PMSC, 0, 2, reg);
    reg[0] = 0x48 | (reg[0] & 0xb3);
    reg[1] = 0x80 | reg[1];
    WriteReg(CSN, PMSC, 0, 2, reg);
    delayMicroseconds(5);
}

void ReadAccMem(int CSN, int index, int dataLen, byte data[]) { //dataLen < 16
    byte tempData[16];
    ReadReg(CSN, 0x25, index, dataLen + 1, tempData);
    for (int i = 0; i < dataLen; i++) {
        data[i] = tempData[i + 1]; //Dummy byte
    }
}

void EndAccMemRead(int CSN) {
    byte reg[2];
    ReadReg(CSN, PMSC, 0, 2, reg);
    reg[0] = reg[0] & 0xb3;
    reg[1] = 0x7f & reg[1];
    WriteReg(CSN, PMSC, 0, 2, reg);
    delayMicroseconds(5);
}

void SetTransmitData(int CSN, int dataLen, byte data[]) {
    WriteReg(CSN, TX_BUFFER, 0, dataLen, data);
    byte buf[1];
    buf[0] = dataLen + 2;
    WriteReg(CSN, TX_FCTRL, 0, 1, buf);
}

//Advanced control like CANSFCS or HRBPT shall be implemented manually.
void StartTransmit(int CSN, bool delayed, bool wait4resp) {
    byte buf[1] = {0x02};
    if (delayed) buf[0] += 0x04;
    if (wait4resp) buf[0] += 0x80;
    WriteReg(CSN, SYS_CTRL, 0, 1, buf);
}

bool isTransmitDone(int CSN) {
    byte buf[1];
    ReadReg(CSN, SYS_STATUS, 0, 1, buf);
    return (buf[0] >> 7);
}

void ClearTransmitStatus(int CSN) {
    byte buf[1];
    ReadReg(CSN, SYS_STATUS, 0, 1, buf);
    buf[0] |= 0xF8;
    WriteReg(CSN, SYS_STATUS, 0, 1, buf);
}

void ForceTRxOff(int CSN) {
    byte buf[1] = {0x40};
    WriteReg(CSN, SYS_CTRL, 0, 1, buf);
}
