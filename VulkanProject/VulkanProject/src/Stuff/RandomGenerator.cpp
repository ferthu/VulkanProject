#pragma region HEADER
//--------------------------------------------------------------------------------------
// File: RandomGenerator.h
// Project: Function library
//
// Author Mattias Fredriksson 2017.
//--------------------------------------------------------------------------------------

#include"Stuff\RandomGenerator.h"
#include<memory>
#include<cmath>
#include<algorithm>
#include"glm\geometric.hpp"
#pragma endregion

namespace mf{

#pragma region Constants
	/* Minimum number of seeds allowed to be generated
	*/
	const unsigned int mt32MinSeedSize = 4;
	/*	Number of 32 bit ints used by the Mersenne Twister (ie. the actual number of 32 int seeds required).
	*/
	const unsigned int mt32MaxSeedSize = 624;
	/*	PI * 2 (float)
	*/
	static const float PIx2 = 3.1415927f * 2;
#pragma endregion

#pragma region Funcs. External

	/* Generate a random initializer list with random numbers generated trough the random_device.
	*/
	std::vector<std::random_device::result_type> generateSeedSeq(unsigned int seedCount) {
		std::random_device seeder;
		std::vector<std::random_device::result_type> vec(seedCount);
		//Fetch seeds.
		for (unsigned int i = 0; i < seedCount; i++)
			vec[i] = seeder();
		return vec;
	}

	/* Generate a set of separated triangles.
	*/
	void distributeTriangles(RandomGenerator &rnd, float sphereRad, uint32_t numTris, glm::vec2 triSize, glm::vec4 *posBuf, glm::vec3 *norBuf)
	{
		for (uint32_t i = 0; i < numTris; i++)
		{
			float off = rnd.randomFloat(-sphereRad, sphereRad);
			glm::vec3 nor = rnd.randomNormal();
			glm::vec3 pos = nor * off;
			glm::vec3 forw = glm::cross(nor, abs(nor.y) + abs(nor.z) > 0.01f ? glm::vec3(1, 0, 0) : glm::vec3(0, -1, 0));
			glm::vec3 right = glm::cross(forw, nor);
			float size = rnd.randomFloat(triSize.x, triSize.y);
			right *= 0.5f * size;
			forw *= 0.5f * size;
			// Set params
			posBuf[i * 3] = glm::vec4(pos - right - forw, 1.f);
			posBuf[i * 3 + 1] = glm::vec4(pos + forw, 1.f);
			posBuf[i * 3 + 2] = glm::vec4(pos + right - forw, 1.f);
			if (norBuf)
			{
				norBuf[i * 3] = nor;
				norBuf[i * 3 + 1] = nor;
				norBuf[i * 3 + 2] = nor;
			}
		}
	}

#pragma endregion

#pragma region Implementation
	/* Construct a seeded random generator
	*/
	RandomGenerator::RandomGenerator()
		:rng(){
		seedGenerator();
	}
	/* Construct a seeded random generator.
	seedCount	<<	Number of seeds used to seed the number engine.
	*/
	RandomGenerator::RandomGenerator(unsigned char seedCount)
		: rng(){
		seedGenerator(seedCount);
	}
	/* Construct a random generator seeded from a list beginning and ending at the pointers. Should use atleast 64 bit for seeding.
	*/
	RandomGenerator::RandomGenerator(const unsigned int* begin, const unsigned int* end)
		: rng(){
		setSeed(begin, end);
	}
	/* Construct a random generator seeded from a list. Should use atleast 64 bit for seeding.
	*/
	RandomGenerator::RandomGenerator(const std::initializer_list<unsigned int>& list)
		: rng(){
		setSeed(list);
	}

	/* Construct a random generator seeded from another random distributor.
	*/
	RandomGenerator::RandomGenerator(const mf::RandomGenerator& seeder)
		: rng() {
		std::vector<unsigned int> seed = seeder.generateSeed(mt32MaxSeedSize);
		setSeed(seed);
	}
	RandomGenerator::~RandomGenerator(){
	}


	/*	Seed the generator with a new seed.
	*/
	void RandomGenerator::seedGenerator(){
		//Generate a seed sequence of 128 bits
		std::vector<std::random_device::result_type> vec = generateSeedSeq(mt32MinSeedSize);
		std::seed_seq seeds(vec.begin(), vec.end());
		rng.seed(seeds);
	}
	/*	Seed the generator by generating a set of seeds for the random engine.
	*/
	void RandomGenerator::seedGenerator(unsigned int seedCount){
		std::vector<std::random_device::result_type> vec = generateSeedSeq(std::min(std::max(seedCount, mt32MinSeedSize), mt32MaxSeedSize));
		std::seed_seq seeds(vec.begin(), vec.end());
		rng.seed(seeds);
	}
	/* Seed the engine from the seeds specified in list beginning and ending at the pointers. Should use atleast 128 bit for seeding.
	*/
	void RandomGenerator::setSeed(const unsigned int* begin, const unsigned int* end){
		std::seed_seq seeds(begin, end);
		rng.seed(seeds);
	}
	/*	Seed the generator from the seeds specified in the list. Should use atleast 128 bit for seeding.
	*/
	void RandomGenerator::setSeed(const std::initializer_list<unsigned int>& list){
		std::seed_seq seeds(list);
		rng.seed(seeds);
	}
	/*	Seed the generator from the seeds specified in the list. Should use atleast 128 bit for seeding.
	*/
	void RandomGenerator::setSeed(const std::vector<unsigned int>& vec) {
		std::seed_seq seeds(vec.begin(), vec.end());
		rng.seed(seeds);
	}



	/* Generate a seed as a set of unsigned integers. */
	std::vector<unsigned int> RandomGenerator::generateSeed(unsigned int numSeedInt) const {
		std::vector<unsigned int> list;
		std::uniform_int_distribution<unsigned int> uni;
		for (unsigned int i = 0; i < numSeedInt; i++)
			list.push_back(uni(rng));
		return list;
	}
	/*
	Generate a random unsigned byte
	*/
	char RandomGenerator::randomUByte() const{
		std::uniform_int_distribution<int> uni(0, UCHAR_MAX);
		return (char)uni(rng);
	}

	int RandomGenerator::randomInt(int min, int max) const{
		std::uniform_int_distribution<int> uni(min, max);
		return uni(rng);
	}
	int RandomGenerator::randomInt(int max) const{
		std::uniform_int_distribution<int> uni(0, max);
		return uni(rng);
	}
	int RandomGenerator::randomInt() const{
		std::uniform_int_distribution<int> uni(INT_MIN, INT_MAX);
		return uni(rng);
	}

	float RandomGenerator::randomFloat(float min, float max) const{
		std::uniform_real_distribution<float> uni(min, max);
		return uni(rng);
	}
	float RandomGenerator::randomFloat(float max) const{
		std::uniform_real_distribution<float> uni(0, max);
		return uni(rng);
	}
	float RandomGenerator::randomFloat() const{
		static const std::uniform_real_distribution<float> uni(FLT_MIN, FLT_MAX);
		return uni(rng);
	}
	float RandomGenerator::randomUnitFloat() const
	{
		static const std::uniform_real_distribution<float> uni(0, 1);
		return uni(rng);
	}

	/*	Generates a random highprecision floating point value between [min-max]
	*/
	double RandomGenerator::randomDouble(double min, double max) const {
		std::uniform_real_distribution<double> uni(min, max);
		return uni(rng);
	}
	double RandomGenerator::randomUnitDouble() const {
		static const std::uniform_real_distribution<double> uni(0.0, 1.0);
		return uni(rng);
	}
	/* Generates a random vector with random float values.
	*/
	glm::vec3 RandomGenerator::randomVector() const {
		static const std::uniform_real_distribution<float> uni(FLT_MIN, FLT_MAX);
		return glm::vec3(uni(rng), uni(rng), uni(rng));
	}
	/* Generates a random vector within a unit cube (each component is distributed in range [0, 1] and random length).
	 * Use randomNormal to randomize a vector with unit length!
	*/
	glm::vec3 RandomGenerator::randomVectorUnit() const {
		static const std::uniform_real_distribution<float> uni(0.f, 1.f);
		return glm::vec3(uni(rng), uni(rng), uni(rng));
	}


	/* Generates a random vector with random float values in range [0-max].
	*/
	glm::vec3 RandomGenerator::randomVector(float max) const {
		std::uniform_real_distribution<float> uni(0, max);
		return glm::vec3(uni(rng), uni(rng), uni(rng));
	}
	/* Generates a random vector with random float values in range [min-max].
	*/
	glm::vec3 RandomGenerator::randomVector(float min, float max) const {
		std::uniform_real_distribution<float> uni(min, max);
		return glm::vec3(uni(rng), uni(rng), uni(rng));
	}

	glm::vec2 RandomGenerator::randomNormal2() const {
		const static std::uniform_real_distribution<float> zero_2Pi(0, std::nextafter(PIx2, 0.0f)); //[0, 2pi) -> Find the next value from 2Pi toward 0
		float angle = zero_2Pi(rng);
		return glm::vec2(cos(angle), sin(angle));
	}
	glm::dvec2 RandomGenerator::randomNormal2_d() const {
		const static std::uniform_real_distribution<double> zero_2Pi(0, std::nextafter(PIx2, 0.0f)); //[0, 2pi) -> Find the next value from 2Pi toward 0
		double angle = zero_2Pi(rng);
		return glm::vec2(cos(angle), sin(angle));
	}
	/* Generates a normalized vector from a set of random float values.
	*/
	glm::vec3 RandomGenerator::randomNormal() const {
		/* Uniform distribution over a unit sphere.
		 *	1.	Distribute values over a surface S^2 with bounds X : [0,2*Pi), Y : [-1,1].
				Where Y is defined from [0,Pi] where cosine of spherical coordinates gives uniform distribution in [-1,1].
				The formula can be viewed as a distribution over a rectangle representing the surface of a hollow cylinder and y = z.	
		 *	2.	Use the distributed z coordinate to find the radius of the circle intersected by the plane orthogonal to the z axis.
		 *	3.	Find the normal vector by using the radius and distributed (X) theta angle to calculate the remaining x,y coordinates. 
		*/

		// Distribution ranges: 
		const static std::uniform_real_distribution<float> negone_one(-1, 1);
		const static std::uniform_real_distribution<float> zero_2Pi(0, std::nextafter(PIx2, 0.0f)); //[0, 2pi) -> Find the next value from 2Pi toward 0
		// Distribute values over the sphere
		float z = negone_one(rng);
		float theta = zero_2Pi(rng); 

		float circ_rad = sqrt(1 - z*z); //Radius of the circle defined by the intersection of the plane offset u along z axis.
		return glm::vec3(circ_rad * cos(theta), circ_rad * sin(theta), z);
	}
	/* Generates a random normalized vector with double precision.
	*/
	glm::dvec3 RandomGenerator::randomNormal_d() const	{
		//See randomNormal() func (floating point)
		// Distribution ranges: 
		const static std::uniform_real_distribution<double> negone_one(-1, 1);
		const static std::uniform_real_distribution<double> zero_2Pi(0, std::nextafter(PIx2, 0.0f)); //[0, 2pi) -> Find the next value from 2Pi toward 0
																									// Distribute values over the sphere
		double z = negone_one(rng);
		double theta = zero_2Pi(rng);

		double circ_rad = sqrt(1 - z*z); //Radius of the circle defined by the intersection of the plane offset u along z axis.
		return glm::dvec3(circ_rad * cos(theta), circ_rad * sin(theta), z);
	}


	/* Permutate the array using the Knuth shuffle distributed by the generator.
	*/
	void RandomGenerator::knuthShuffle(int* arr, int size) const
	{
		int tmp, j;
		//Step through the array swapping an index with an index in the remaining greater range of [i,n]
		for (int i = 0; i < size - 2; i++)
		{
			//Generate a int in the remaining range: 
			j = i + randomInt(size - i - 1);
			//Swap
			tmp = arr[i];
			arr[i] = arr[j];
			arr[j] = tmp;
		}
	}

	/* Permutate the array using the Knuth shuffle distributed by the generator.
	*  Shuffle a specified number of times.
	*/
	void RandomGenerator::knuthShuffle(int* arr, int size, unsigned int num_times) const
	{
		for (unsigned int i = 0; i < num_times; i++)
			knuthShuffle(arr, size);
	}

	/* Generate an biased distribution by repeating the range over the array size and then shuffled using the Knuth algorithm.
	*/
	void RandomGenerator::shuffledIntDistribution(int* arr, unsigned int size, int min, int max) const
	{
		//Generate repeated distribution:
		for (unsigned int i = 0; i < size; i++)
			arr[i] = i % (max - min + 1) + min;
		//Shuffle
		knuthShuffle(arr, size);
	}

	/* Generate an biased distribution by repeating the range over the array size and then shuffled using the Knuth algorithm.
	 * Range is defined as [min, max].
	*/
	std::unique_ptr<int> RandomGenerator::shuffledIntDistribution(unsigned int size, int min, int max) const
	{
		std::unique_ptr<int> arr(new int[size]);
		shuffledIntDistribution(arr.get(), size, min, max);
		return std::move(arr);
	}
	/* Generate a set of distributed values in the range for the array using the randomInt(min, max) function.
	*/
	void RandomGenerator::randomSetofInt(int* arr, unsigned int size, int min, int max) const
	{
		for (unsigned int i = 0; i < size; i++)
			arr[i] = randomInt(min, max);
	}

	/* Generate an array with a set of distributed values in the range using the randomInt(min, max) function.
	*/
	std::unique_ptr<int> RandomGenerator::randomSetofInt(unsigned int size, int min, int max) const
	{
		std::unique_ptr<int> arr(new int[size]);
		randomSetofInt(arr.get(), size, min, max);
		return std::move(arr);
	}

#pragma endregion

}

