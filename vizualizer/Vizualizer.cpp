#include <iostream>
#include <SDL.h>
#include <vector>
#include <algorithm>
//#include <sstream>
#include <deque>
#include "FFT.h"

//defining specs
const int WINDOW_WIDTH = 1920;
const int LED_NR = 90;
const int LED_SIZE = ceil(WINDOW_WIDTH / (LED_NR * 1.5));
const int WINDOW_HEIGHT = 1080;//LED_SIZE*10;

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
		uint8_t r, g, b, a = 255;
		void SetColor(uint8_t r, uint8_t g, uint8_t b) {
			this->r = r;
			this->g = g;
			this->b = b;
		}

		void SetColorHue(int a) {
			if (a < 0 || a > 1536) {
				std::cout << "invalid hue number: " << a << "\n";
			}
			this->r = 0;
			this->g = 0;
			this->b = 0;

			switch (a / 256) {
			case 0: 
				this->r = 255;
				this->g = a % 256;
				break;
			case 1:
				this->r = 255 - (a % 256);
				this->g = 255;
				break;
			case 2:
				this->g = 255;
				this->b = a % 256;
				break;
			case 3:
				this->g = 255 - (a % 256);
				this->b = 255;
				break;
			case 4:
				this->r = a % 256;
				this->b = 255;
				break;
			case 5:
				this->b = 255 - (a % 256);
				this->r = 255;
				break;
			case 6:
				this->r = 255;
				break;
			}
		}

		void SetBrightness(float a) {
			*this = *this * a;
		}

		float maxBrightness() {
			float temp = map(std::max(std::max(this->r, this->g), this->b), 0, 255, 0.0, 1.0);
			return temp;
		}

		bool operator==(led b) {
			if (this->r != b.r) return false;
			if (this->g != b.g) return false;
			if (this->b != b.b) return false;
			return true;
		}

		//led class != operator overload
		bool operator!=(led b) {
			return !(*this == b);
		}
		
		led operator+(led b) {
			led s;
			s.r = std::min(this->r + b.r, 255);
			s.g = std::min(this->g + b.g, 255);
			s.b = std::min(this->b + b.b, 255);
			return s;
		}

		led operator-(led b) {
			led s;
			s.r = std::max(this->r - b.r, 0);
			s.g = std::max(this->g - b.g, 0);
			s.b = std::max(this->b - b.b, 0);
			return s;
		}

		led operator*(float b) {
			led s;
			s.r = mmin(this->r * b, 255.0);
			s.g = mmin(this->g * b, 255.0);
			s.b = mmin(this->b * b, 255.0);
			return s;
		}
};



//null led
const led NULL_LED = { 0, 0, 0};

enum effectType {
	BEAM,

};

class lightBeam {
	public:
		int position = 0;
		float initialSpeed = 3;
		float speed = initialSpeed;
		int length = 10;

		//update the position and show it on the led strip
		void update(std::vector<led> &leds) {
			led full = { 255, 255, 255 };
			for (int j = position; j > std::max(position - length, 0); j--) {
				//int color = map(j, position, position - length, 255, 0);
				//led temp = { 0, 0, color };
				leds[j] = leds[j] + (full * map(j, position, position - length, 1, 0.0));
				//leds[j].r = color;//mmax(leds[j].r, color);// +(full * map(j, position, position - j, 1.0, 0.0));
				//leds[j].g = color;// mmax(leds[j].g, color);
				//leds[j].b = color;//mmax(leds[j].b, color);
			}
			position += speed;
			speed = (initialSpeed + speed) / 2;
		}
};

//average
double average(std::vector<double> vec, int from, int to) {
	double temp = 0;
	for (int i = from; i < to; i++) {
		temp += vec[i];
	}
	return temp / (to - from + 1);
}

//smoothed out fill
void setVector(CArray& carray, std::vector<double>& vec) {
	for (int i = 0; i < carray.size() / 2; i++) {
		if (abs(std::real(carray[i])) > abs(vec[i])) {
			vec[i] = abs(std::real(carray[i]));
		}
		/*else {
			//vec[i] = (std::real(carray[i]) + vec[i]);// / 2;
		}*/
	}
}

#define top_bar 20
#define bottom_bar_highs 17

//smooth out vector
std::vector<double> smoothMidtones(std::vector<double> &vec) {
	std::vector<double> temp(top_bar - 3, 0);
	for (int i = 3; i < top_bar; i++) {
		temp[i - 3] = average(vec, i, i + 4);
	}
	return temp;
}

//AVG with abs
double ABSaverage(std::vector<double> vec, int from, int to) {
	double temp = 0;
	for (int i = from; i < to; i++) {
		temp += abs(vec[i]);
	}
	return temp / (to - from + 1);
}

//checks ratio between highest high note and the average of them
double checkHighs(std::vector<double> vec) {
	double highest = 0;
	for (int i = bottom_bar_highs; i < vec.size(); i++) {
		highest = std::max(highest, abs(vec[i]));
	}
	return highest / ABSaverage(vec, bottom_bar_highs, vec.size());
}



// Pointers to our window and renderer
SDL_Window* window;
SDL_Renderer* renderer;

//draws the led strip
void drawLEDs(std::vector<led>& leds) {
	for (int i = 0; i < LED_NR; i++) {
		int x = map(i + 1, 0, LED_NR + 1, 0, WINDOW_WIDTH) - (LED_SIZE / 2);// -(LED_SIZE / 2); //floor(((i + 1) * WINDOW_WIDTH) / (LED_NR + 1)) - (LED_SIZE / 2);
		int y = WINDOW_HEIGHT / 2;
		SDL_SetRenderDrawColor(renderer, leds[i].r, leds[i].g, leds[i].b, leds[i].a);
		SDL_Rect tempLED = { x, y, LED_SIZE, LED_SIZE };
		SDL_RenderFillRect(renderer, &tempLED);
	}
	SDL_RenderPresent(renderer);
}

//draw from Carray
void drawWave(CArray wave) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	int x1 = map(0, 0, wave.size() - 1, 0, WINDOW_WIDTH);
	int y1 = WINDOW_HEIGHT -LED_SIZE;//WINDOW_HEIGHT/2 - std::real(wave[0]);
	
	for (int i = 1; i < wave.size()/2; i++) {
		//int amp = sqrt(sqrt(std::real(wave[i]) * std::real(wave[i]) + std::imag(wave[i]) * std::imag(wave[i])));
		int amp = (abs(std::real(wave[i])));
		//std::cout << amp << "\n";
		int y2 = WINDOW_HEIGHT - LED_SIZE - amp;// (abs(std::real(wave[i])) / 10);//pow(-1, i) * 
		int x2 = map(i, 1, wave.size()/2, 10, WINDOW_WIDTH-10);
		//std::cout <<  wave[i] << " ";
		SDL_SetRenderDrawColor(renderer, 0,map(i, 1, wave.size()/2,0 , 255), 255- map(i, 1, wave.size(), 0, 255), 255);
		SDL_Rect temp = { x2, y2, 3, amp };
		SDL_RenderFillRect(renderer, &temp);
		//SDL_RenderDrawLine(renderer,x2, y1, x2, y2);
		//x1 = x2;
		//y1 = y2;
	}
	SDL_RenderPresent(renderer);
}

//draw from vector
void drawWave(std::vector<double> wave) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	int x1 = map(0, 0, wave.size() - 1, 0, WINDOW_WIDTH);
	int y1 = WINDOW_HEIGHT - LED_SIZE;//WINDOW_HEIGHT/2 - std::real(wave[0]);

	for (int i = 1; i < wave.size(); i++) {
		//int amp = sqrt(sqrt(std::real(wave[i]) * std::real(wave[i]) + std::imag(wave[i]) * std::imag(wave[i])));
		int amp = (abs(wave[i]));
		//std::cout << amp << "\n";
		int y2 = WINDOW_HEIGHT - LED_SIZE - amp;// (abs(std::real(wave[i])) / 10);//pow(-1, i) * 
		int x2 = map(i, 1, wave.size(), 10, WINDOW_WIDTH - 10);
		//std::cout <<  wave[i] << " ";
		SDL_SetRenderDrawColor(renderer, 0, map(i, 0, wave.size(), 0, 255), 255 - map(i, 1, wave.size(), 0, 255), 255);
		//SDL_Rect temp = { x2, y2, 5, amp };
		//SDL_RenderFillRect(renderer, &temp);
		SDL_RenderDrawLine(renderer,x1, y1, x2, y2);
		x1 = x2;
		y1 = y2;
		if (!i) i++;
	}
	SDL_RenderPresent(renderer);
}

///
void drawDeque(std::deque<int8_t> wave) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	int x1 = map(0, 0, wave.size() - 1, 0, WINDOW_WIDTH);
	int y1;
	if (wave.size()) y1 = (WINDOW_HEIGHT / 2) - wave[0];


	for (int i = 1; i < wave.size(); i++) {
		int y2 = (WINDOW_HEIGHT / 2) + pow(-1, i) * (wave[i] * 3);//pow(-1, i) *
		int x2 = map(i, 1, wave.size(), 0, WINDOW_WIDTH);
		//std::cout <<  wave[i] << " ";
		SDL_SetRenderDrawColor(renderer, 0, map(i, 1, wave.size(), 0, 255), 255 - map(i, 1, wave.size(), 0, 255), 255);
		//SDL_Rect temp = { x1, y1, x2 - x1, y2 - y1 };
		//SDL_RenderFillRect(renderer, &temp);
		SDL_RenderDrawLine(renderer,x1, y1, x2, y2);
		x1 = x2;
		y1 = y2;
	}
	SDL_RenderPresent(renderer);
}

//dims the led strip
void dimLEDs(std::vector<led>& leds) {
	led temp = { 64, 64, 64 };
	for (int i = 0; i < LED_NR; i++) {
		leds[i] = leds[i] - temp;// *(long)map(i, 0, LED_NR, 0.15, 1.0);
	}
}

void clearLEDs(std::vector<led>& leds) {
	for (int i = 0; i < LED_NR; i++){
		leds[i] = NULL_LED;
	}
}

#define dampSpeed 1.0005
//dampens wave values
void dampenWave(std::vector<double>& wave) {
	//led temp = { 64, 64, 64 };
	//std::vector<double> temp = wave;
	for (int i = 0; i < wave.size(); i++) {
		wave[i] /= dampSpeed;
		/*if (wave[i] < 0) {
			wave[i] = wave[i] + dampSpeed; //*map(i, 0, 65536, 1.0, 0.15);
		}
		else {
			wave[i] = wave[i] - dampSpeed; //*map(i, 0, 65536, 1.0, 0.15);
		}*/
		//wave[i] = (temp[i - 1] + temp[i + 1] + temp[i]) / 3;
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
int DeviceID = 0;

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

void close()
{
	
	//Destroy window	
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;

	//Free playback audio
	if (gRecordingBuffer != NULL)
	{
		delete[] gRecordingBuffer;
		gRecordingBuffer = NULL;
	}

	//Quit SDL subsystems
	SDL_Quit();
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

//Check frequencies
//////////////////////////////////

//Checks bass volume
double checkBass(std::vector<double> freq) {
	return std::max(std::max(abs(freq[0]), abs(freq[1])), abs(freq[2]));
}

//return loudest midtone
double maxVolume(std::vector<double> vec) {
	double maxVol = 0;
	for (int i = 0; i < vec.size(); i++) {
		maxVol = std::max(vec[i], maxVol);
	}
	return maxVol;
}

int main(int argc, char** args) {

	SDL_AudioDeviceID recordingDeviceId = 0;
	SDL_AudioDeviceID playbackDeviceId = 0;


	//initialize sound subsystem
	if (!InitSound()) {
		std::cout << "Failed to initalize sound subsystem!\n";
	}

	for (int i = 0; i < SDL_GetNumAudioDevices(SDL_TRUE); i++) {
		std::cout << i << ": " << SDL_GetAudioDeviceName(i, SDL_TRUE) << "\n";
	}

	std::cout << "Please select a microphone: ";
	std::cin >> DeviceID; std::cin.ignore();

	//Initialize rendering window
	int result = SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, NULL, &window, &renderer);
	if (result != 0) {
		std::cout << "Failed to create window and renderer: " << SDL_GetError() << "\n";
	}

	//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

	//open mic
	SDL_AudioSpec desiredRecordingSpec;
	SDL_zero(desiredRecordingSpec);
	desiredRecordingSpec.freq = Frequency;
	desiredRecordingSpec.format = AudioFormat;
	desiredRecordingSpec.channels = Channels;
	desiredRecordingSpec.samples = SampleRate;
	desiredRecordingSpec.callback = audioRecordingCallback;

	recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(DeviceID, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &gReceivedRecordingSpec, 0);

	if (recordingDeviceId == 0){
		//Report error
		std::cout << "Failed to open recording device! SDL Error: %s" <<  SDL_GetError();
	}/* else {
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
	*/
	//Calculate per sample bytes
	int bytesPerSample = gReceivedRecordingSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedRecordingSpec.format) / 8);

	//Calculate bytes per second
	int bytesPerSecond = gReceivedRecordingSpec.freq * bytesPerSample;

	//Calculate buffer size (1 sec/ refreshrate ...)
	gBufferByteMaxPosition = bytesPerSecond/RefreshRate;
	//std::cout << gBufferByteMaxPosition;
	gBufferByteSize = bytesPerSecond;


	//Allocate and initialize byte buffer
	
	gRecordingBuffer = new Sint8[gBufferByteSize];
	memset(gRecordingBuffer, 0, gBufferByteSize);
	
	

	std::vector<led> leds(LED_NR, NULL_LED), test(LED_NR, NULL_LED);
	if (1) {
		short r = 255, g = 0, b = 0, temp = 0;
		int loop = 2;

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
	std::deque<int8_t> wave2;
	std::vector<double> final(gBufferByteMaxPosition/2, 0);
	std::vector<lightBeam> beams;
	bool quit = false;
	uint32_t bassPhase = 0, standbyPhase = 0;
	int maxCooldown = 8, cooldown = 0, maxResetCooldown = RefreshRate;
	SDL_Event e;
	CArray wave(gBufferByteMaxPosition);
	double maxBass = 0, maxMidtone = 0;

	while (!quit) {
		SDL_PollEvent(&e);
		//std::cout << gBufferBytePosition << " ";
		if (e.type == SDL_QUIT)
		{
			quit = true;
		}
		
		dampenWave(final);

		if (gBufferBytePosition > gBufferByteMaxPosition) {
			SDL_LockAudioDevice(recordingDeviceId);
			gBufferBytePosition = 0;
			//int maxVol = 0;
			//std::vector<int8_t> wave;

			int maxVol = 0;
			for (int i = 0; i < gBufferByteMaxPosition; i++) {

				//wave.push_back((int8_t)gRecordingBuffer[i]);
				wave[i] = { (double)gRecordingBuffer[i],0 };
				//wave2.push_back((int8_t)gRecordingBuffer[i]);
				//if (wave2.size() > gBufferByteMaxPosition*8) wave2.pop_front();
				if ((int8_t)gRecordingBuffer[i] > maxVol) {
					maxVol = (int8_t)gRecordingBuffer[i];
				}
			}
			//std::cout << "\n" << maxVol;
			clearLEDs(leds);
			/*for (int i = 0; i < map(maxVol, 0, 128, 1, LED_NR); i++) {
				leds[i] = test[i];
			}*/



			fft(wave);

			setVector(wave, final);

			//smoothMidtones(final);

			//drawDeque(wave2);

			double bass = checkBass(final);
			std::vector<double> midtones = smoothMidtones(final);
			//double highRatio = bass / average(final, 0, 2);//checkHighs(final);

			//std::cout << highRatio << "\n";

			//drawWave(midtones);

			if (maxVolume(midtones) <= maxMidtone * 0.05) {
				if (maxResetCooldown <= 0) {
					maxMidtone = 0;
					maxBass = 0;
					//std::cout << "Max volumes reset!\n";
				}
				else maxResetCooldown--;
			}
			else maxResetCooldown = RefreshRate;

			maxBass = std::max(bass, maxBass);
			maxMidtone = std::max(maxVolume(midtones), maxMidtone);

			if (bass >= maxBass * 0.55 && cooldown <= 0) {
				lightBeam temp;
				beams.push_back(temp);
				cooldown = maxCooldown;
			}

			bassPhase += (int)map(bass, 0, maxBass, 0, 32);
			
			/*if (maxResetCooldown){
				for (int i = 0; i < LED_NR; i++) {
					leds[i].SetColorHue((bassPhase + i * 2) % 1536);
				}
			}
			else*/
			
			float targetBrightness = map(maxVolume(midtones), 0, std::max(maxVolume(midtones), maxMidtone * 0.9), 0.05, 1.0);
			//std::cout << targetBrightness << "\n";
			if (!isnan(targetBrightness)){
				for (int i = 0; i < LED_NR; i++) {

					//leds[i].SetBrightness(0.9);

					
					//float targetBrightness = map(midtones[map(i, 0, LED_NR - 1, 0, midtones.size() - 1)], 0, maxMidtone, 0.3, 1.0);
					/*bool setBrightness = true;

					if (leds[i].maxBrightness() > targetBrightness) setBrightness = false;*/

					//set the color based on bass
					leds[i].SetColorHue((bassPhase + i * 2) % 1536);

					//set the brightness based on midtone volume



					 //if (setBrightness) 
					leds[i].SetBrightness(targetBrightness);
				}

				for (int i = 0; i < beams.size(); i++) {
					if (beams[i].position >= LED_NR) {
						beams.erase(beams.begin() + i);
						i--;
					}
					else {
						beams[i].speed = (int)map(bass, 0, maxBass, beams[i].initialSpeed, beams[i].initialSpeed * 3);
						beams[i].update(leds);
					}
				}
			} else {
				for (int i = 0; i < LED_NR; i++) {
					leds[i].SetColorHue((standbyPhase + i * 2) % 1536);
				}
			}

			drawWave(final);
			drawLEDs(leds);
			standbyPhase++;
			bassPhase++;
			cooldown--;
			SDL_UnlockAudioDevice(recordingDeviceId);
		}
	}
	//stop
	close();
	
	return 0;
}