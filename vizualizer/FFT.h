#pragma once

#include <iostream>
#include <complex>
#include <valarray>
#include <math.h>
#include <vector>

const double PI = 3.141592653589793238460 ;

//using namespace std ;

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;

void fft(CArray& x);

class FFT
{
public:
	FFT(std::string const& _path,int const& _bufferSize);

	//void hammingWindow() ;
	
	//void update() ;
	std::vector<Complex> sample ;
	std::vector<float> window ;

	CArray bin ;
private:
	
	int sampleRate ;
	int sampleCount ;
	int bufferSize ;
	int mark ;
};

