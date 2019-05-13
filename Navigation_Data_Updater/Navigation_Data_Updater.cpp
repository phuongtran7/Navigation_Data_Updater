// Navigation_Data_Updater.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include  "Downloader.h"

int main()
{
	downloader new_downloader;
	new_downloader.initialize();
	new_downloader.run();
	new_downloader.shutdown();

	std::getchar();
}