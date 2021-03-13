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
	this->mWriteSerialBufferBytes({ 255 });
	// flush buffer
	for (uint8_t i = 0; i < 10; i++) {
		this->mWriteSerialBufferBytes({ 0 });
		std::this_thread::sleep_for(50ms);
	}

	// reset
	this->mWriteSerialBufferBytes({ ASCII_ESC, '@' });

	// init
	this->mWriteSerialBufferBytes({ ASCII_ESC, '7' }); // print setting
	// Max printing dots, heating time, heating interval
	this->mWriteSerialBufferBytes({ 255, 100, 20 }); // heat time
	this->mWriteSerialBufferBytes({ ASCII_DC2, (2 << 5) | 10 }); // print density
	std::this_thread::sleep_for(200ms);
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

void Printer::Feed(uint8_t Lines)
{
	// Characters are 12x24 dots
	this->mWriteSerialBufferBytes({ ASCII_ESC, 'J', 30 });
}



size_t Printer::mWriteSerialBufferBytes(std::vector<uint8_t> Bytes)
{
	// mTimeoutWait();
	// Split buffer by 4 bytes and wait bytetime
	size_t timeoutWait = Bytes.size();
	size_t bytesWritten = mSerial->WriteBytes(Bytes);
	// mTimeoutSet(timeoutWait * BYTE_TIME);
	return bytesWritten;
}

void Printer::mTimeoutSet(int ms)
{
	mWaitTime = ms;
}

void Printer::mTimeoutAccumilate(int ms)
{
	mWaitTime += ms;
}

void Printer::mTimeoutWait()
{
	std::cout << "PRINTER: WAITING " << mWaitTime << "MS" << std::endl;
	std::this_thread::sleep_for(std::chrono::milliseconds(mWaitTime));
	mWaitTime = 0;
}



Printer::~Printer()
{
	mSerial->Close();
}

