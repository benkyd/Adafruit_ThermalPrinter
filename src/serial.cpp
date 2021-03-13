#include "serial.hpp"

#ifdef _WIN32
#pragma comment (lib, "Setupapi.lib")

#include <tchar.h>
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>

#ifdef max
#undef max
#endif
#endif


#ifdef __linux__
#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif


#ifdef _WIN32

// https://github.com/wjwwood/serial/blob/master/src/impl/list_ports/list_ports_win.cc
std::string utf8_encode(const std::wstring& wstr)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

std::vector<PortInfo> GetPortList()
{
	std::vector<PortInfo> devicesFound;

	HDEVINFO deviceInfoSet = SetupDiGetClassDevs(
		(const GUID*)&GUID_DEVCLASS_PORTS,
		NULL,
		NULL,
		DIGCF_PRESENT);

	unsigned int deviceInfoSetIndex = 0;
	SP_DEVINFO_DATA deviceInfoData;

	deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

	while (SetupDiEnumDeviceInfo(deviceInfoSet, deviceInfoSetIndex, &deviceInfoData))
	{
		deviceInfoSetIndex++;

		// Get port name

		HKEY hkey = SetupDiOpenDevRegKey(
			deviceInfoSet,
			&deviceInfoData,
			DICS_FLAG_GLOBAL,
			0,
			DIREG_DEV,
			KEY_READ);

		TCHAR portName[256];
		DWORD portNameLength = 256;

		LONG ret = RegQueryValueEx(
			hkey,
			_T("PortName"),
			NULL,
			NULL,
			(LPBYTE)portName,
			&portNameLength);

		RegCloseKey(hkey);

		if (ret != EXIT_SUCCESS)
			continue;

		if (portNameLength > 0 && portNameLength <= 256)
			portName[portNameLength - 1] = '\0';
		else
			portName[0] = '\0';

		// Ignore parallel ports

		if (_tcsstr(portName, _T("LPT")) != NULL)
			continue;

		// Get port friendly name

		TCHAR friendlyName[256];
		DWORD friendlyNameLength = 0;

		BOOL gotFriendlyName = SetupDiGetDeviceRegistryProperty(
			deviceInfoSet,
			&deviceInfoData,
			SPDRP_FRIENDLYNAME,
			NULL,
			(PBYTE)friendlyName,
			256,
			&friendlyNameLength);

		if (gotFriendlyName == TRUE && friendlyNameLength > 0)
			friendlyName[friendlyNameLength - 1] = '\0';
		else
			friendlyName[0] = '\0';

		// Get hardware ID

		TCHAR HWID[256];
		DWORD HWIDLength = 0;

		BOOL gotHWID = SetupDiGetDeviceRegistryProperty(
			deviceInfoSet,
			&deviceInfoData,
			SPDRP_HARDWAREID,
			NULL,
			(PBYTE)HWID,
			256,
			&HWIDLength);

		if (gotHWID == TRUE && HWIDLength > 0)
			HWID[HWIDLength - 1] = '\0';
		else
			HWID[0] = '\0';

#ifdef UNICODE
		std::string nPortName = utf8_encode(port_name);
		std::string nFriendlyName = utf8_encode(friendly_name);
		std::string nHWID = utf8_encode(hardware_id);
#else
		std::string nPortName = portName;
		std::string nFriendlyName = friendlyName;
		std::string nHWID = HWID;
#endif

		PortInfo portEntry;
		portEntry.Port = nPortName;
		portEntry.Desc = nFriendlyName;
		portEntry.HWID = nHWID;

		devicesFound.push_back(portEntry);
	}

	SetupDiDestroyDeviceInfoList(deviceInfoSet);

	return devicesFound;
}

#endif

#ifdef __linux__
// linux find ports lol
#endif



#ifdef _WIN32

// TODO: Move this logic to printer
Serial::Serial(bool DoStdOut, std::string Port)
	: mDoLog(DoStdOut)
	, mHComm(0)
{
	if (Port == "AUTO")
	{
		std::vector<PortInfo> portList = GetPortList();
		for (const auto& port : portList)
		{
			if (port.HWID.find("VID_067B&PID_2303") != std::string::npos)
			{
				Port = port.Port;
				break;
			}
			Port = "Not Found";
		}
	}

	if (Port == "Not Found")
	{
		throw new std::exception("ERROR: PRINTER COMM PORT NOT FOUND");
	}

	if (mDoLog)
		std::cout << "SERIAL: Port " << Port << " found" << std::endl;

	mPort = Port;

}

void Serial::Open()
{
	if (mPort == "NONE") return;

	std::string temp = "\\\\.\\" + mPort;
	std::wstring wtemp = std::wstring(temp.begin(), temp.end());
	LPCWSTR port = wtemp.c_str();

	mHComm = CreateFileW(port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (mHComm == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		switch (error)
		{
		case ERROR_FILE_NOT_FOUND:
			throw new std::exception("ERROR: COMM PORT DOES NOT EXIST");
		default:
			throw new std::exception("ERROR: UNKNOWN ERROR OCCURED WHILE OPENING SERIAL PORT");
		}
	}

	mIsOpen = true;

	if (mDoLog)
		std::cout << "SERIAL: Opened comm port " << mPort << std::endl;


	DCB serialParameters;
	GetCommState(mHComm, &serialParameters);

	serialParameters.BaudRate = CBR_9600;
	serialParameters.ByteSize = 8;
	serialParameters.StopBits = ONESTOPBIT;
	serialParameters.Parity = EVENPARITY;

	serialParameters.fOutxCtsFlow = false;
	serialParameters.fRtsControl = RTS_CONTROL_DISABLE;
	serialParameters.fOutX = true;
	serialParameters.fInX = true;

	if (!SetCommState(mHComm, &serialParameters))
	{
		throw new std::exception("ERROR: UNABLE TO SETUP COMM PORT");
	}

	if (mDoLog)
		std::cout << "SERIAL: Configured comm port " << mPort << std::endl;


	COMMTIMEOUTS timeouts;

	timeouts.ReadIntervalTimeout = std::numeric_limits<uint32_t>::max();
	timeouts.ReadTotalTimeoutConstant = 1000;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 1000;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(mHComm, &timeouts))
	{
		throw new std::exception("ERROR: UNABLE TO SETTUP COMM PORT TIMEOUTS");
	}
}

void Serial::Close()
{
	if (!mIsOpen) return;
	if (mHComm == INVALID_HANDLE_VALUE)
	{
		mIsOpen = false;
		return;
	}

	int status = CloseHandle(mHComm);
	if (status == 0)
	{
		throw new std::exception("ERROR: UNABLE TO CLOSE COMM PORT");
	}

	mHComm = INVALID_HANDLE_VALUE;
	mIsOpen = false;

	if (mDoLog)
		std::cout << "SERIAL: Closed comm port " << mPort << std::endl;
}

size_t Serial::ReadByteBuffer(uint8_t* Buffer, size_t Length)
{
	if (!mIsOpen) return static_cast<size_t>(-1);

	if (mDoLog)
		std::cout << "SERIAL: Reading " << Length << " bytes from " << mPort << std::endl;

	DWORD numBytesRead;
	if (!ReadFile(mHComm, Buffer, static_cast<DWORD>(Length), &numBytesRead, NULL))
	{
		throw new std::exception("ERROR: UNNABLE TO READ FROM COMM PORT");
	}

	return (size_t)numBytesRead;
}

size_t Serial::WriteByte(uint8_t Byte)
{
	if (!mIsOpen) return static_cast<size_t>(-1);

	return (size_t)WriteBytes({ Byte });;
}

size_t Serial::WriteBytes(const std::vector<uint8_t>& Bytes)
{
	if (!mIsOpen) return static_cast<size_t>(-1);

	if (mDoLog)
		std::cout << "SERIAL: Writing " << Bytes.size() << " bytes to " << mPort << std::endl;

	DWORD numBytesWritten = 0;

	for (const auto& byte : Bytes)
	{
		if (mDoLog)
			std::cout << "SERIAL: Writing " << byte << " to " << mPort << std::endl;		DWORD tempNumBytesWritten;
		if (!WriteFile(mHComm, &byte, static_cast<DWORD>(1), &tempNumBytesWritten, NULL))
		{
			throw new std::exception("ERROR: UNNABLE TO WRITE TO COMM PORT");
		}
		numBytesWritten += tempNumBytesWritten;
	}

	return (size_t)numBytesWritten;
}

size_t Serial::Write(const uint8_t* Data, const size_t Length)
{
	if (!mIsOpen) return static_cast<size_t>(-1);

	if (mDoLog)
		std::cout << "SERIAL: Writing " << Length << " bytes to " << mPort << std::endl;

	DWORD numBytesWritten;
	if (!WriteFile(mHComm, Data, static_cast<DWORD>(Length), &numBytesWritten, NULL))
	{
		throw new std::exception("ERROR: UNNABLE TO WRITE TO COMM PORT");
	}

	return (size_t)numBytesWritten;
}

bool Serial::IsOpen()
{
	return mIsOpen;
}

#endif

#ifdef __linux__
// linux do serial lol
#endif
