#pragma once
#ifndef MYFUNC_H
#define MYFUNC_H

template<typename T> 
inline T map(T inputLowerBound, T inputUpperBound, T outputLowerBound, T outputUpperBound, T val) {
	return outputLowerBound + (((val - inputLowerBound) * (outputUpperBound - outputLowerBound)) / (inputUpperBound - inputLowerBound));
}

#endif