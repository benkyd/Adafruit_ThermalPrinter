#ifndef ADAFRUIT_THERMALPRINTER_PRINTER_H_
#define ADAFRUIT_THERMALPRINTER_PRINTER_H_

#include <iostream>
#include <string>
#include <vector>

// ASCII codes used by some of the printer config commands:
#define ASCII_TAB '\t' // Horizontal tab
#define ASCII_LF '\n'  // Line feed
#define ASCII_FF '\f'  // Form feed
#define ASCII_CR '\r'  // Carriage return
#define ASCII_DC2 18   // Device control 2
#define ASCII_ESC 27   // Escape
#define ASCII_FS 28    // Field separator
#define ASCII_GS 29    // Group separator

// Codes for barcodes / QR
#define UPC_A 0
#define UPC_E 1
#define EAN13 2
#define EAN8 3
#define CODE39 4
#define I25 5
#define CODEBAR 6
#define CODE93 7
#define CODE128 8
#define CODE11 9
#define MSI 10

// Printer needs to wait between bytes in order not to 
// overflow internal buffer, byte time is defined based
// on the baudrate
// https://github.com/adafruit/Adafruit-Thermal-Printer-Library/blob/master/Adafruit_Thermal.cpp#L67
#define BYTE_TIME (((11L * 1000000L) + (9600 / 2)) / 9600)

class Printer
{
private:

	bool mDoLog;

	int mByteTime;
	int mDotPrintTime;
	uint8_t mPrevByte;
	int mColumn;
	int mMaxColumn;
	int mCharHeight;
	int mLineSpacing;
	int mBarcodeHeight;
	int mPrintMode;
	int mDefaultHeatTime;
	int mFirmwareVersion = 268;


public:

	Printer(bool DoStdOut = true, std::string SerialPort = "AUTO");

	void Write(uint8_t Byte);
	void WriteString(std::string String);

	void Feed(uint8_t Lines);


	~Printer();

private:

	Serial* mSerial;

	size_t mWriteSerialBufferBytes(std::vector<uint8_t> Bytes);

	void mTimeoutSet(int ms);
	void mTimeoutWait();
	int mResumeTime;

};

#endif
