#include <iostream>
#include <windows.h>
#include <thread>
#include <vector>

using namespace std;

//The result found can be changed to lowercase if so desired
#define BRUTEFORCED_CHARS 6 //6 should always be adequate (60^6 > 2^32)

DWORD targetHash = 0xC5670914; //This must be the hash of the prefetch
const CHAR prefix[] = "\\DEVICE\\HARDDISKVOLUME3\\PROGRAM FILES\\"; //This must be the folder you are placing your cheato in
const CHAR suffix[] = "\\NOTEPAD.EXE"; //This must match the name of the prefetch you are trying to hide under

CHAR validChars[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'!', '#', '$', '%', '&', '\'', '(', ')', '-', '.', ';', '=', '@', '[', ']', '^', '_', '`', '{', '}', '~', '+', ',', ' '
};

__forceinline DWORD __fastcall FAFVistaHashWithStart(const CHAR* ws, int length, DWORD startingHash) {
	DWORD hash = startingHash;
	for (int x = 0; x < length; x++) {
		uint8_t num = ws[x];
		hash = (hash * 37UL + num) * 37UL;
	}
	return hash;
}


__forceinline DWORD __fastcall FAFVistaHash(const CHAR* ws, int length) {
	DWORD hash = 314159;
	for (int x = 0; x < length; x++) {
		uint8_t num = ws[x];
		hash = (hash * 37UL + num) * 37UL;
	}
	return hash;
}

volatile LONG threadsLeft = 0;
CHAR correctChars[BRUTEFORCED_CHARS + 1] = {};
bool success = false;

void workerThread(int threadn, int threadc) {
	BYTE* buffer = (BYTE*)calloc(1, BRUTEFORCED_CHARS + sizeof(suffix));
	memcpy(buffer + BRUTEFORCED_CHARS, suffix, sizeof(suffix) - 1);

	DWORD alwaysStartingHash = FAFVistaHash(prefix, sizeof(prefix) - 1);

	CHAR bfPositions[BRUTEFORCED_CHARS] = { 0 };
	ULONG64 counter = 0;

	bfPositions[0] = threadn;
	for (int x = 0; x < BRUTEFORCED_CHARS; x++) {
		if (bfPositions[x] >= sizeof(validChars)) {
			int ic = bfPositions[x] / sizeof(validChars);
			bfPositions[x] %= sizeof(validChars);
			if (x < BRUTEFORCED_CHARS - 1) {
				bfPositions[x + 1] += ic;
			} else {
				break;
			}
		}
	}

	while (!threadsLeft) {
		counter++;
		for (int x = 0; x < BRUTEFORCED_CHARS; x++) {
			buffer[x] = validChars[bfPositions[x]];
		}

		if ((counter & 0xffffff) == 0xffffff) {
			for (int x = 0; x < BRUTEFORCED_CHARS; x++) {
				cout << validChars[bfPositions[x]];
			}
			cout << endl;
		}

		DWORD hash = FAFVistaHashWithStart((const CHAR*)buffer, BRUTEFORCED_CHARS + sizeof(suffix) - 1, alwaysStartingHash);
		if (hash == targetHash) {
			success = true;
			break;
		}


		bfPositions[0] += threadc;
		for (int x = 0; x < BRUTEFORCED_CHARS; x++) {
			if (bfPositions[x] >= sizeof(validChars)) {
				int ic = bfPositions[x] / sizeof(validChars);
				bfPositions[x] %= sizeof(validChars);
				if (x < BRUTEFORCED_CHARS - 1) {
					bfPositions[x + 1] += ic;
				} else {
					break;
				}
			}
		}
	}
	LONG orig = InterlockedAdd(&threadsLeft, 1);
	if (orig == 1) {

		memcpy(&correctChars, buffer, BRUTEFORCED_CHARS);
	}
}

int main()
{
	int threads = 16;
	std::vector<thread> tref(threads);
	for (int x = 0; x < threads - 1; x++) {
		tref[x] = std::thread(workerThread, x, threads);
		tref[x].detach();
	}
	workerThread(threads - 1, threads);

	while (threadsLeft != threads) {
		volatile DWORD x = 1;
		YieldProcessor();
	}

	if (success) {
		std::cout << prefix << correctChars << suffix << endl;
	} else {
		std::cout << "Failed to find. Try again with longer length?" << std::endl;
	}
}
