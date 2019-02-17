#ifndef SLEEP_H
#define SLEEP_H

#include <chrono>
#include <thread>

inline void ssleep(int s) { std::this_thread::sleep_for(std::chrono::seconds(s)); }
inline void msleep(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
inline void usleep(int us) { std::this_thread::sleep_for(std::chrono::microseconds(us)); }
inline void nsleep(int ns) { std::this_thread::sleep_for(std::chrono::nanoseconds(ns)); }

#endif 
