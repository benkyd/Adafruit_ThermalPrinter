#ifndef ADAFRUIT_THERMALPRINTER_PRINTER_H_
#define ADAFRUIT_THERMALPRINTER_PRINTER_H_

#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

class PortInfo
{
public:
	std::string Port;
	std::string Desc;
	std::string HWID;
};

std::vector<PortInfo> GetPortList();

class Serial
{
public:
	Serial(std::string Port = "AUTO");
	void Open();
	void Close();
	void WriteByte(uint8_t Byte);
	uint8_t* ReadByteBuffer();
private:
	bool mIsOpen = false;
	std::string mPort = "NONE";
#ifdef _WIN32
	HANDLE mHComm;
#endif
};


class Printer
{
private:

	int mResumeTime;
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

	Printer(std::string SerialPort = "AUTO");


};

#endif
