cmake_minimum_required(VERSION 3.16.0)

project(limited_queue
	VERSION 1.0.0
	LANGUAGES CXX
	DESCRIPTION "Depth-limited thread-safe queue container"
	HOMEPAGE_URL "https://github.com/vlad-nn/limited-queue"
)

add_subdirectory(test)
