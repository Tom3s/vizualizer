#include <iostream>
#include <SDL.h>
#include <vector>
#include <algorithm>
#include <sstream>

//map(value, value range from, to, map from, to)
float map(float value, float start1, float stop1, float start2, float stop2) {
	float outgoing = start2 + (stop2 - start2) * ((value - start1) / (stop1 - start1));
	return outgoing;
}

//max function
int mmax(int a, int b) {
	if (a > b) return a; else return b;
}

float mmin(float a, float b) {
	if (a < b) return a; else return b;
}

//led class
class led {
	public:
		short r, g, b, a = 255;
		void SetColor(short r, short g, short b) {
			this->r = r;
			this->g = g;
			this->b = b;
		}
};

//led class == operator overload
bool operator==(led a, led b){
	if (a.r != b.r) return false;
	if (a.g != b.g) return false;
	if (a.b != b.b) return false;
	return true;
}

//led class != operator overload
bool operator!=(led a, led b) {
	return !(a == b);
}

//led class + operator overload
led operator+(led a, led b) {
	led s;
	s.r = std::min(a.r + b.r, 255);
	s.g = std::min(a.g + b.g, 255);
	s.b = std::min(a.b + b.b, 255);
	return s;
}

//led class - operator overload
led operator-(led a, led b) {
	led s;
	s.r = std::max(a.r - b.r, 0);
	s.g = std::max(a.g - b.g, 0);
	s.b = std::max(a.b - b.b, 0);
	return s;
}

//led class * operator overload
led operator*(led a, float b) {
	led s;
	s.r = mmin(a.r * b, 255.0);
	s.g = mmin(a.g * b, 255.0);
	s.b = mmin(a.b * b, 255.0);
	return s;
}

//null led
const led NULL_LED = { 0, 0, 0};

//defining specs
const int WINDOW_WIDTH = 1920;
const int LED_NR = 50;
const int LED_SIZE = ceil(WINDOW_WIDTH / (LED_NR*1.5));
const int WINDOW_HEIGHT = 1080;//LED_SIZE*5;

// Pointers to our window and renderer
SDL_Window* window;
SDL_Renderer* renderer;

//draws the led strip
void drawLEDs(std::vector<led>& leds) {
	for (int i = 0; i < LED_NR; i++) {
		int x = map(i + 1, 0, LED_NR + 1, 0, WINDOW_WIDTH) - (LED_SIZE / 2);// -(LED_SIZE / 2); //floor(((i + 1) * WINDOW_WIDTH) / (LED_NR + 1)) - (LED_SIZE / 2);
		int y = WINDOW_HEIGHT / 2 - (LED_SIZE / 2);
		SDL_SetRenderDrawColor(renderer, leds[i].r, leds[i].g, leds[i].b, leds[i].a);
		SDL_Rect tempLED = { x, y, LED_SIZE, LED_SIZE };
		SDL_RenderFillRect(renderer, &tempLED);
	}
	SDL_RenderPresent(renderer);
}

void drawWave(std::vector<int8_t> wave) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	int x1 = map(0, 0, wave.size() - 1, 0, WINDOW_WIDTH);
	int y1; 
	if (wave.size()) y1 = (WINDOW_HEIGHT / 2) - wave[0];
	
	
	for (int i = 1; i < wave.size(); i++) {
		int y2 = (WINDOW_HEIGHT / 2) +  (wave[i]*2);//pow(-1, i) *
		int x2 = map(i, 1, wave.size(), 0, WINDOW_WIDTH);
		//std::cout <<  wave[i] << " ";
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		//SDL_Rect temp = { x1, y1, x2-x1, y2-y1 };
		//SDL_RenderFillRect(renderer, &temp);
		SDL_RenderDrawLine(renderer,x1, y1, x2, y2);
		x1 = x2;
		y1 = y2;
	}
	SDL_RenderPresent(renderer);
}

//draws the led strip
void clearLEDs(std::vector<led>& leds) {
	led temp = { 20, 20, 20 };
	for (int i = 0; i < LED_NR; i++) {
		leds[i] = leds[i] - temp*map(i, 0, LED_NR, 0.15, 1.0);
	}
}

//shifts over leds by 1
void shiftLEDs(std::vector<led>& leds) {
	led temp = leds[LED_NR - 1];
	for (int i = LED_NR - 1; i >= 1; i--){
		leds[i] = leds[i - 1];
	}
	leds[0] = temp;
}

//Sound
//definitions
#define Frequency 32768
#define AudioFormat AUDIO_S8
#define Channels 1
#define SampleRate 128
#define RefreshRate 64

//audioBuffer
Sint8* gRecordingBuffer = NULL;

//pointer
Uint32 gBufferBytePosition = 0;

//Device IDs


//Recieved audio spec
SDL_AudioSpec gReceivedRecordingSpec;
SDL_AudioSpec gReceivedPlaybackSpec;

//Size of data buffer
Uint32 gBufferByteSize = 0;

//Maximum position in data buffer for recording
Uint32 gBufferByteMaxPosition = 0;

bool InitSound() {
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
	{
		std::cout << "SDL could not initialize! SDL Error: %s" << SDL_GetError() << "\n";
		success = false;
	}

	return success;
}

//Recording callback
void audioRecordingCallback(void* userdata, Uint8* stream, int len)
{
	//Copy audio from stream
	memcpy(&gRecordingBuffer[gBufferBytePosition], stream, len);

	//Move along buffer
	//std::cout << "plen "<<len << "\n";
	gBufferBytePosition += len;
}

//playback callback
void audioPlaybackCallback(void* userdata, Uint8* stream, int len)
{
	//Copy audio to stream
	memcpy(stream, &gRecordingBuffer[gBufferBytePosition], len);

	//Move along buffer
	gBufferBytePosition += len;
}

int main(int argc, char** args) {

	SDL_AudioDeviceID recordingDeviceId = 0;
	SDL_AudioDeviceID playbackDeviceId = 0;

	//Initialize rendering window
	int result = SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, NULL, &window, &renderer);
	if (result != 0) {
		std::cout << "Failed to create window and renderer: " << SDL_GetError() << "\n";
	}

	//initialize sound subsystem
	if (!InitSound()) {
		std::cout << "Failed to initalize sound subsystem!\n";
	}

	//open mic
	SDL_AudioSpec desiredRecordingSpec;
	SDL_zero(desiredRecordingSpec);
	desiredRecordingSpec.freq = Frequency;
	desiredRecordingSpec.format = AudioFormat;
	desiredRecordingSpec.channels = Channels;
	desiredRecordingSpec.samples = SampleRate;
	desiredRecordingSpec.callback = audioRecordingCallback;

	recordingDeviceId = SDL_OpenAudioDevice(NULL, SDL_TRUE, &desiredRecordingSpec, &gReceivedRecordingSpec, 0);

	if (recordingDeviceId == 0){
		//Report error
		std::cout << "Failed to open recording device! SDL Error: %s" <<  SDL_GetError();
	} else {
		//Default audio spec
		SDL_AudioSpec desiredPlaybackSpec;
		SDL_zero(desiredPlaybackSpec);
		desiredPlaybackSpec.freq = Frequency;
		desiredPlaybackSpec.format = AudioFormat;
		desiredPlaybackSpec.channels = Channels;
		desiredPlaybackSpec.samples = SampleRate;
		desiredPlaybackSpec.callback = audioPlaybackCallback;

		//Open playback device
		playbackDeviceId = SDL_OpenAudioDevice(NULL, SDL_FALSE, &desiredPlaybackSpec, &gReceivedPlaybackSpec, 0);
	}
	if (playbackDeviceId == 0) {
		//Report error
		std::cout << "Failed to open recording device! SDL Error: %s" << SDL_GetError();
	} 
	
	//Calculate per sample bytes
	int bytesPerSample = gReceivedRecordingSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedRecordingSpec.format) / 8);

	//Calculate bytes per second
	int bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;

	//Calculate buffer size (1 sec/ refreshrate ...)
	gBufferByteMaxPosition = bytesPerSecond/RefreshRate;
	std::cout << gBufferByteMaxPosition;
	gBufferByteSize = bytesPerSecond;


	//Allocate and initialize byte buffer
	
	gRecordingBuffer = new Sint8[gBufferByteSize];
	memset(gRecordingBuffer, 0, gBufferByteSize);
	
	

	std::vector<led> leds(LED_NR, NULL_LED), test(LED_NR, NULL_LED);
	if (1) {
		short r = 255, g = 0, b = 0, temp = 0;
		int loop = 1;

		while (loop) {
			for (int i = 0; i < 255; i += 1) {
				switch (temp % 7) {
				case 0:
					g++;
					break;
				case 1:
					r--;
					break;
				case 2:
					b++;
					break;
				case 3:
					g--;
					break;
				case 4:
					r++;
					break;
				case 5:
					b--;
					break;
				case 6:
					loop--;
					std::cout << loop << "\n";
					i = 256;
					break;
				}
				if (i % 32 == 0) shiftLEDs(test);
				test[0].SetColor(r, g, b);
				if (test[LED_NR - 1] != NULL_LED) {
					loop = 0;
				}
			}
			temp++;
		}
	}

	

	gBufferBytePosition = 0;
	SDL_PauseAudioDevice(recordingDeviceId, SDL_FALSE);
	//SDL_PauseAudioDevice(playbackDeviceId, SDL_TRUE);

	while (1) {
		//std::cout << gBufferBytePosition << " ";
		
		if (gBufferBytePosition > gBufferByteMaxPosition){
			SDL_LockAudioDevice(recordingDeviceId);
			gBufferBytePosition = 0;
			//int maxVol = 0;
			std::vector<int8_t> wave;
			int maxVol = 0;
			for (int i = 0; i < gBufferByteMaxPosition; i+=2) {
				wave.push_back((int8_t)gRecordingBuffer[i]);
				if ((int8_t)gRecordingBuffer[i] > maxVol) {
					maxVol = (int8_t)gRecordingBuffer[i];
				}
			}
			std::cout << "\n" << maxVol;
			clearLEDs(leds);
			for (int i = 0; i < map(maxVol, 0, 128, 1, LED_NR); i++) {
				leds[i] = test[i];
			}
			//drawWave(wave);
			drawLEDs(leds);
			
			SDL_UnlockAudioDevice(recordingDeviceId);
		}
	}
	//stop
	SDL_PauseAudioDevice(recordingDeviceId, SDL_TRUE);
	//rainbow loop test
	/*{
		short r = 255, g = 0, b = 0, temp = 0;
		int loop = 10;

		while (loop) {
			for (int i = 0; i < 255; i++) {
				switch (temp % 7) {
				case 0:
					g++;
					break;
				case 1:
					r--;
					break;
				case 2:
					b++;
					break;
				case 3:
					g--;
					break;
				case 4:
					r++;
					break;
				case 5:
					b--;
					break;
				case 6:
					loop--;
					std::cout << loop << "\n";
					i = 256;
					break;
				}
				if (i % 10 == 0) shiftLEDs(leds);
				leds[0].SetColor(r, g, b);
				if (leds[LED_NR - 1] != NULL_LED) {
					drawLEDs(leds);
					SDL_RenderPresent(renderer);
					SDL_Delay(1);
				}
			}
			temp++;
		}
	}*/



	SDL_Delay(3000);
	
	return 0;
}