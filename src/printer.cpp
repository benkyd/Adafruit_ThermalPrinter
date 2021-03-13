#include "printer.hpp"

#include <thread>
#include <chrono>

#include "serial.hpp"

using namespace std::chrono_literals;


Printer::Printer(bool DoStdOut, std::string SerialPort)
	: mDoLog(DoStdOut)
{
	mSerial = new Serial(DoStdOut, SerialPort);
	mSerial->Open();

	// printer needs 2s to initialize
	std::this_thread::sleep_for(2000ms);

	// wake
	mSerial->WriteByte(255);
	for (uint8_t i = 0; i < 10; i++) {
		mSerial->WriteByte(0);
		std::this_thread::sleep_for(50ms);
	}

	// reset
	mSerial->WriteBytes({ ASCII_ESC, '@' });

	// init
	mSerial->WriteBytes({ ASCII_ESC, '7' }); // print setting
	mSerial->WriteBytes({ 11, 120, 40 }); // heat time
	mSerial->WriteBytes({ ASCII_DC2, (2 << 5) | 10 }); // print density

	std::this_thread::sleep_for(200ms);

	this->WriteString("Ines is really poggers");
	std::this_thread::sleep_for(200ms);

	mSerial->WriteBytes({ ASCII_ESC, 'J', 255 }); // feed 2 rows

}


void Printer::Write(uint8_t Byte)
{

}

void Printer::WriteString(std::string String)
{
	std::vector<uint8_t> bytesToWrite;
	
	for (const auto& character : String)
	{
		bytesToWrite.push_back({ static_cast<uint8_t>(character) });
	}

	this->mWriteSerialBufferBytes(bytesToWrite);
}



size_t Printer::mWriteSerialBufferBytes(std::vector<uint8_t> Bytes)
{
	mTimeoutWait();
	// Split buffer by 4 bytes and wait bytetime
	size_t timeoutWait = Bytes.size();
	size_t bytesWritten = mSerial->WriteBytes(Bytes);
	mTimeoutSet(timeoutWait * BYTE_TIME);
	return bytesWritten;
}

void Printer::mTimeoutSet(int ms)
{

}

void Printer::mTimeoutWait()
{

}



Printer::~Printer()
{
	mSerial->Close();
}

