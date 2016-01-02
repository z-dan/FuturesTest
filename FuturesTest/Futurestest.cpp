// TestProject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <future>

// Simple test for testing for primes.
bool isPrime(unsigned long long n)
{
	if (n < 1)
		return false;
	else if (n <= 3)
		return true;
	else if (n % 2 == 0 || n % 3 == 0)
		return false;

	unsigned long long tester = 5;
	while (tester*tester <= n)
	{
		if (n%tester == 0 || n % (tester + 2) == 0)
			return false;
		tester += 6;
	}
	return true;
}


using primeSetResults = std::vector<unsigned long long>;

// To avoid excessive thread startup/shutdown overhead, the primes are 
// computed in groups. Size of groups is adjusted via constant in main.
primeSetResults testPrimeSet(unsigned long long first, unsigned long long last)
{
	primeSetResults results;
	for (auto n = first; n <= last; ++n)
	{
		if(isPrime(n))
			results.push_back(n);
	}
	return std::move(results);
}

// C++ does not yet provide a "when_any" primitive for futures
// so we implement our own custom version here. It takes a 
// container of futures and waits for any of them to complete.
auto when_any(std::vector<std::future<primeSetResults>>& futures)
{
	while (true)
	{
		for (auto futIt = futures.begin(); futIt != futures.end(); ++futIt)
		{
			// Loop through the futures looking for one that is finished.
			// To avoid having this thread consume CPU just waiting, we idle for
			// a small amount of time waiting for each.
			if (futIt->wait_for(std::chrono::milliseconds(50)) == std::future_status::ready)
			{
				auto result = futIt->get();
				futures.erase(futIt);
				return std::move(result);
			}
		}
	}
}

using namespace std;

// Comment the MACRO out below to see how long it will take without multithreading.
#define MULTITHREADED

int main(int  argc, char** argv)
{
	const unsigned long long maxVal = 10000000;
	unsigned long long interval = 10000;
	if (argc > 1)
	{
		interval = std::atoll(argv[1]);
		if (!interval)
		{
			std::cerr << " Invalid input argument: " << argv[1] << std::endl;
			return 1;
		}
	}
	const size_t maxAsync = 4;
	vector<unsigned long long > primes;
	auto start = chrono::high_resolution_clock::now();
#if defined(MULTITHREADED)
	vector<future<primeSetResults>> results;

	for (unsigned long long n = 0; n < maxVal; n+= interval)
	{
		while (results.size() >= maxAsync)
		{
			auto result = when_any(results);
			primes.insert(primes.end(),result.begin(), result.end());
		}
		auto last = n + interval > maxVal ? maxVal:n + interval - 1;
		results.push_back(async(launch::async, testPrimeSet, n, last));
	}
	while (results.size())
	{
		auto result = when_any(results);
		primes.insert(primes.end(), result.begin(), result.end());
	}
#else
	primes = testPrimeSet(0, maxVal);
#endif

	auto end = chrono::high_resolution_clock::now();
	cout.flush();
	std::chrono::duration<double> durationSecs = end - start;
	cout << "Tested " << maxVal << " values for primaility in " << durationSecs.count() <<
		" secs. " << endl << "Found " << primes.size() << endl;
	return 0;
}

