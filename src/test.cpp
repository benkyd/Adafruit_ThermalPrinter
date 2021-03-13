#include "printer.hpp"

#include <iostream>

#include <thread>
#include <chrono>

using namespace std::chrono_literals;

int main(int* argc, char** argv)
{
	Printer printer;

	printer.WriteString("Line 1");
	std::this_thread::sleep_for(2000ms);

	printer.Feed(1);
	std::this_thread::sleep_for(2000ms);
	printer.WriteString("Line 2");

	std::this_thread::sleep_for(2000ms);
}
