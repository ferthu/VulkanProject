//--------------------------------------------------------------------------------------
// File: RandomGenerator.h
// Project: Function library
//
// Author Mattias Fredriksson 2017.
//--------------------------------------------------------------------------------------

#pragma once

#include<random>
#include<memory>
#include<vector>
#include"glm\vec3.hpp"

namespace mf{
	

	/*	Random generator seeded at construction. Contains functions for generating sequences of random numbers.
	*/
	class RandomGenerator {
	private:
		/* Random-number engine used (Mersenne-Twister in this case)
		*/
		mutable std::mt19937 rng;    

	public:
		/* Construct a seeded random generator
		*/
		RandomGenerator();
		/* Construct a seeded random generator. 
		seedCount	<<	Number of seeds used to seed the number engine.
		*/
		RandomGenerator(unsigned char seedCount);
		/* Construct a random generator seeded from a list beginning and ending at the pointers. Should use atleast 64 bit for seeding.
		*/
		RandomGenerator(const unsigned int* begin, const unsigned int* end);
		/* Construct a random generator seeded from a list. Should use atleast 64 bit for seeding.
		*/
		RandomGenerator(const std::initializer_list<unsigned int>& list);
		/* Construct a random generator seeded from another random distributor.
		*/
		RandomGenerator(const mf::RandomGenerator& seeder);
		~RandomGenerator();

		/*	Seed the generator with a new seed.
		*/
		void seedGenerator();
		/*	Seed the generator by generating a set of seeds for the random engine.
		*/
		void seedGenerator(unsigned int seedCount);
		/* Seed the engine from the seeds specified in list beginning and ending at the pointers. Should use atleast 128 bit for seeding.
		*/
		void setSeed(const unsigned int* begin, const unsigned int* end);
		/*	Seed the generator from the seeds specified in the list. Should use atleast 128 bit for seeding.
		*/
		void setSeed(const std::initializer_list<unsigned int>& list);
		/*	Seed the generator from the seeds specified in the list. Should use atleast 128 bit for seeding.
		*/
		void setSeed(const std::vector<unsigned int>& vec);

		/* Generate a seed as a set of unsigned integers. */
		std::vector<unsigned int> generateSeed(unsigned int numSeedInt) const;

		/*	Generate a random unsigned byte
		*/
		char randomUByte() const;

		/*	Generates a random int between [min-max]
		*/
		int randomInt(int min, int max) const;
		/*	Generates a random int between [0-max]
		*/
		int randomInt(int max) const;
		/*	Generates a random int between [0-MAX_INT]
		*/
		int randomInt() const;
		/*	Generates a random float between [min-max]
		*/
		float randomFloat(float min, float max) const;
		/*	Generates a random float between [0-max]
		*/
		float randomFloat(float max) const;
		/*	Generates a random float between [FLT_MIN - FLT_MAX]
		*/
		float randomFloat() const;
		/* Return a random unit float in range [0,1]
		*/
		float randomUnitFloat() const;

		/*	Generates a random highprecision floating point value between [min-max]
		*/
		double randomDouble(double min, double max) const;
		/* Return a random unit highprecision floating point value in range [0,1]
		*/
		double randomUnitDouble() const;

		/* Generates a random vector with random float values.
		*/
		glm::vec3 randomVector() const;
		/* Generates a random vector with random float values in range [0-max].
		*/
		glm::vec3 randomVector(float max) const;
		/* Generates a random vector with random float values in range [min-max].
		*/
		glm::vec3 randomVector(float min, float max) const;
		/* Generates a random vector within a unit cube (each component is distributed in range [0, 1] and random length).
		* Use randomNormal to randomize a vector with unit length!
		*/
		glm::vec3 randomVectorUnit() const;
		/* Generate a random 2 dimensional unit vector.
		*/
		glm::vec2 randomNormal2() const;
		/* Generate a random 2 dimensional unit vector.
		*/
		glm::dvec2 randomNormal2_d() const;
		/* Generates a random normalized vector.
		*/
		glm::vec3 randomNormal() const;
		/* Generates a random normalized vector with double precision.
		*/
		glm::dvec3 randomNormal_d() const;

		/* Permutate the array using the Knuth shuffle distributed by the generator.
		*/
		void knuthShuffle(int* arr, int size) const;
		/* Permutate the array using the Knuth shuffle distributed by the generator.
		*  Shuffle a specified number of times.
		*/
		void knuthShuffle(int* arr, int size, unsigned int num_times) const;


		/* Generate an biased distribution by repeating the range over the array size and then shuffled using the Knuth algorithm.
		*/
		void shuffledIntDistribution(int* arr, unsigned int size, int min, int max) const;
		/* Generate an biased distribution by repeating the range over the array size and then shuffled using the Knuth algorithm.
		*/
		std::unique_ptr<int> shuffledIntDistribution(unsigned int size, int min, int max) const;

		/* Generate a set of distributed values in the range [min,max] for the array. 
		*/
		void randomSetofInt(int* arr, unsigned int size, int min, int max) const;
		/* Generate an array with a set of distributed values in the range [min, max].
		*/
		std::unique_ptr<int> randomSetofInt(unsigned int size, int min, int max) const;
	};

	/* Generate a random initializer list with random numbers generated trough the random_device.
	*/
	std::vector<std::random_device::result_type> generateSeedSeq(unsigned int seedCount);

	/* Generate a set of separated triangles.
	*/
	void distributeTriangles(RandomGenerator &rnd, float sphereRad, uint32_t numTris, glm::vec2 triSize, glm::vec4 *posBuf, glm::vec3 *norBuf);
}
