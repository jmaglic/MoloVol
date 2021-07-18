#ifndef EXCEPTION_H

#define EXCEPTION_H

#include <iostream>
#include <exception>

struct ExceptIllegalFunctionCall : public std::exception
{
	const char * what () const throw () {
    	return "Ex: Illegal Function Call. Argument not allowed";
    }
};

struct ExceptIllegalFileExtension : public std::exception
{
	const char * what () const throw () {
    	return "Ex: Illegal File Extension";
    }
};

struct ExceptInvalidInputFile : public std::exception
{
	const char * what () const throw () {
    	return "Ex: Invalid input file. Import failed";
    }
};

struct ExceptInvalidCellParams : public std::exception
{
	const char * what () const throw () {
    	return "Ex: Invalid cell parameters in file. Import failed";
    }
};

#endif
