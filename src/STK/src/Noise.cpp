/***************************************************/
/*! \class Noise
    \brief STK noise generator.

    Generic random number generation using the
    C rand() function.  The quality of the rand()
    function varies from one OS to another.

    by Perry R. Cook and Gary P. Scavone, 1995-2012.
*/
/***************************************************/

#include "Noise.h"
#include <time.h>
#include <chrono>
#include <functional>
#include <algorithm>
#include <functional>
#include <random>
#include <vector>

namespace stk {

Noise::Noise( unsigned int seed ) {

  this->setSeed( seed );

}

// Ignore passed in seed.
void Noise::setSeed( unsigned int seed ) {

	// http://i4.connect.microsoft.com/VisualStudio/feedbackdetail/view/875492/compile-error-passing-a-random-device-to-mersenne-twister-engine

	auto newSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	mt_rand.seed((unsigned long)newSeed);

}

StkFloat Noise::generateRandom() {

	// Vs mt_rand()
	// The std::uniform_real_distribution::operator() takes a Generator & so you will have to bind using std::ref 
	auto randomNumber = std::bind(std::uniform_real_distribution<double>(-1, 1), std::ref(mt_rand));

	auto result = randomNumber(); 

	return StkFloat(result);

}

} // stk namespace


