#include "../limited_queue.h"
#include "gtest/gtest.h"

using namespace vnn;

TEST( limited_queue_tests, main )
{
	// simple test for limited size with drop
	{
		const size_t limit = 10;
		LimitedQueue< int > queue( limit, true, std::chrono::microseconds( 1000 ) );
		for( int i = 0; i < limit; ++i )
			EXPECT_TRUE( queue.push_back( i ) );
		for( int i = limit; i < 2 * limit; ++i )
			EXPECT_FALSE( queue.push_back( i ) );

		EXPECT_EQ( queue.size(), limit );

		for( int i = limit; i < 2 * limit; ++i )
		{
			int j;
			EXPECT_TRUE( queue.pop_front( j ) );
			EXPECT_EQ( i, j );
		}
	}

	// simple test for limited size with no drop
	{
		const size_t limit = 10;
		LimitedQueue< int > queue( limit, false, std::chrono::microseconds( 1000 ) );
		for( int i = 0; i < limit; ++i )
			EXPECT_TRUE( queue.push_back( i ) );

		EXPECT_FALSE( queue.push_back( limit + 1 ) );
		EXPECT_EQ( queue.size(), limit );

		for( int i = 0; i < limit; ++i )
		{
			int j;
			EXPECT_TRUE( queue.pop_front( j ) );
			EXPECT_EQ( i, j );
		}
	}
}
