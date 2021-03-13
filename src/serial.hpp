#ifndef ADAFRUIT_THERMALPRINTER_SERIAL_H_
#define ADAFRUIT_THERMALPRINTER_SERIAL_H_

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
	Serial(bool DoStdOut = true, std::string Port = "AUTO");

	void Open();
	void Close();

	size_t ReadByteBuffer(uint8_t* Buffer, size_t Length);

	size_t WriteByte(const uint8_t Byte);
	size_t WriteBytes(const std::vector<uint8_t>& Bytes);
	size_t Write(const uint8_t* Data, const size_t Length);

	bool IsOpen();
private:
	bool mDoLog = false;
	bool mIsOpen = false;
	std::string mPort = "NONE";
#ifdef _WIN32
	HANDLE mHComm;
#endif
};


#endif
