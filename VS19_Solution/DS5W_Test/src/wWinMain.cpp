#include <Windows.h>

#include <string>
#include <sstream>
#include <map>

#include <DualSenseWindows/IO.h>
#include <DualSenseWindows/Device.h>

typedef std::wstringstream wstrBuilder;

class Console {
	public:
		Console() {
			AllocConsole();
			consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		}

		~Console() {
			FreeConsole();
		}

		void writeLine(LPCWSTR text) {
			write(text);
			write(L"\n");
		}

		void writeLine(wstrBuilder& builder) {
			writeLine(builder.str().c_str());
			builder.str(L"");
		}

		void write(LPCWSTR text) {
			WriteConsoleW(consoleHandle, text, wcslen(text), NULL, NULL);
		}

		void write(wstrBuilder& builder) {
			write(builder.str().c_str());
			builder.str(L"");
		}

	private:
		HANDLE consoleHandle;
};

template<typename T> bool arrayEquals(T* array1, size_t size1, T* array2, size_t size2) {
	if (size1 != size2) return false;
	for (size_t i = 0; i < size1; ++i) {
		if (array1[i] != array2[i]) return false;
	}
	return true;
}
template<typename T> std::map<size_t, T> arrayDiscrepencies(T* array1, size_t size1, T* array2, size_t size2, wstrBuilder& builder) {
	std::map<size_t, T> map;
	for (size_t i = 0; i < size1; ++i) {
		if (array1[i] != array2[i]) {
			map[i] = array1[i];
		}
	}
	return map;
}

INT WINAPI wWinMain(HINSTANCE _In_ hInstance, HINSTANCE _In_opt_ hPrevInstance, LPWSTR _In_ cmdLine, INT _In_ nCmdShow) {
	// Console
	Console console;
	wstrBuilder builder;
	console.writeLine(L"DualShock 5 Windows Test\n========================\n");
	
	// Enum all controlers presentf
	DS5W::DeviceEnumInfo infos[16];
	unsigned int controlersCount = 0;
	DS5W_ReturnValue rv = DS5W::enumDevices(infos, 16, true, &controlersCount);

	// check size
	if (controlersCount == 0) {
		console.writeLine(L"No PS5 controler found!");
		system("pause");
		return -1;
	}

	// Print all controler
	builder << L"Found " << controlersCount << L" PS5 Controler(s):";
	console.writeLine(builder);

	// Iterate controlers
	for (unsigned int i = 0; i < controlersCount; i++) {
		if (infos[i]._internal.connection == DS5W::DeviceConnection::BT) {
			builder << L"Wireless (Blotooth) controler (";
		}
		else {
			builder << L"Wired (USB) controler (";
		}

		builder << infos[i]._internal.path << L")";
		console.writeLine(builder);
	}

	// Create first controler
	DS5W::DeviceContext con;
	if (DS5W_SUCCESS(DS5W::initDeviceContext(&infos[0], &con))) {
		console.writeLine(L"DualSense 5 controller connected");

		// State object
		DS5W::DS5InputState inState;

		// Application infinity loop
		bool keepRunning = true;
		unsigned short inputReportByteLength = con._internal.inputReportByteLength;
		unsigned char* hidBuffer = new unsigned char[inputReportByteLength];	//To replace with input report byte length
		unsigned int updateNumber = 0;
		while (keepRunning) {
			// Get input state
			if (DS5W_SUCCESS(DS5W::getDeviceInputState(&con, &inState))) {
				std::map<size_t, unsigned char> map = arrayDiscrepencies(con._internal.hidBuffer, (size_t)con._internal.inputReportByteLength, hidBuffer, (size_t)inputReportByteLength, builder);
				if (map.size() > 1) {	//7 will always change
					builder << "Update number " << updateNumber;
					console.writeLine(builder);
					builder << "Discrepencies: ";
					for (std::map<size_t, unsigned char>::const_iterator it = map.cbegin(); it != map.cend(); ++it) {
						if (it->first != (size_t)7) {
							builder << "(" << it->first << "," << it->second << ") ";
						}
					}
					console.writeLine(builder);
					if (inState.headPhoneConnected) {
						console.writeLine(L"Connected");
					}
					else {
						console.writeLine(L"Not Connected");
					}

					++updateNumber;
					memcpy(hidBuffer, con._internal.hidBuffer, inputReportByteLength);
					//keepRunning = false;
				}
			}
			else {
				// Device disconnected show error and try to reconnect
				console.writeLine(L"Device removed!");
				DS5W::reconnectDevice(&con);
			}
		}

		// Free state
		DS5W::freeDeviceContext(&con);
		delete hidBuffer;
	}
	else {
		console.writeLine(L"Failed to connect to controler!");
		system("pause");
		return -1;
	}
	system("pause");

	return 0;
}
