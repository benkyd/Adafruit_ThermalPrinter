#include "printer.hpp"

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
	std::vector<PortInfo> devices_found;

	HDEVINFO device_info_set = SetupDiGetClassDevs(
		(const GUID*)&GUID_DEVCLASS_PORTS,
		NULL,
		NULL,
		DIGCF_PRESENT);

	unsigned int device_info_set_index = 0;
	SP_DEVINFO_DATA device_info_data;

	device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

	while (SetupDiEnumDeviceInfo(device_info_set, device_info_set_index, &device_info_data))
	{
		device_info_set_index++;

		// Get port name

		HKEY hkey = SetupDiOpenDevRegKey(
			device_info_set,
			&device_info_data,
			DICS_FLAG_GLOBAL,
			0,
			DIREG_DEV,
			KEY_READ);

		TCHAR port_name[256];
		DWORD port_name_length = 256;

		LONG return_code = RegQueryValueEx(
			hkey,
			_T("PortName"),
			NULL,
			NULL,
			(LPBYTE)port_name,
			&port_name_length);

		RegCloseKey(hkey);

		if (return_code != EXIT_SUCCESS)
			continue;

		if (port_name_length > 0 && port_name_length <= 256)
			port_name[port_name_length - 1] = '\0';
		else
			port_name[0] = '\0';

		// Ignore parallel ports

		if (_tcsstr(port_name, _T("LPT")) != NULL)
			continue;

		// Get port friendly name

		TCHAR friendly_name[256];
		DWORD friendly_name_actual_length = 0;

		BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
			device_info_set,
			&device_info_data,
			SPDRP_FRIENDLYNAME,
			NULL,
			(PBYTE)friendly_name,
			256,
			&friendly_name_actual_length);

		if (got_friendly_name == TRUE && friendly_name_actual_length > 0)
			friendly_name[friendly_name_actual_length - 1] = '\0';
		else
			friendly_name[0] = '\0';

		// Get hardware ID

		TCHAR hardware_id[256];
		DWORD hardware_id_actual_length = 0;

		BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
			device_info_set,
			&device_info_data,
			SPDRP_HARDWAREID,
			NULL,
			(PBYTE)hardware_id,
			256,
			&hardware_id_actual_length);

		if (got_hardware_id == TRUE && hardware_id_actual_length > 0)
			hardware_id[hardware_id_actual_length - 1] = '\0';
		else
			hardware_id[0] = '\0';

#ifdef UNICODE
		std::string portName = utf8_encode(port_name);
		std::string friendlyName = utf8_encode(friendly_name);
		std::string hardwareId = utf8_encode(hardware_id);
#else
		std::string portName = port_name;
		std::string friendlyName = friendly_name;
		std::string hardwareId = hardware_id;
#endif

		PortInfo port_entry;
		port_entry.Port = portName;
		port_entry.Desc = friendlyName;
		port_entry.HWID = hardwareId;

		devices_found.push_back(port_entry);
	}

	SetupDiDestroyDeviceInfoList(device_info_set);

	return devices_found;
}

#endif

#ifdef __linux__
// linux find ports lol
#endif



#ifdef _WIN32

Serial::Serial(std::string Port)
	: mHComm(0)
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
	std::cout << "Printer found port " << Port << std::endl;

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
}

size_t Serial::ReadByteBuffer(uint8_t* Buffer, size_t Length)
{
	if (!mIsOpen) return static_cast<size_t>(-1);

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

	DWORD numBytesWritten;
	if (!WriteFile(mHComm, &Byte, static_cast<DWORD>(1), &numBytesWritten, NULL))
	{
		throw new std::exception("ERROR: UNNABLE TO WRITE TO COMM PORT");
	}

	return (size_t)numBytesWritten;
}

size_t Serial::Write(const uint8_t* Data, const size_t Length)
{
	if (!mIsOpen) return static_cast<size_t>(-1);

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





Printer::Printer(std::string SerialPort)
{
	Serial serial(SerialPort);
	serial.Open();
	serial.Close();
}

