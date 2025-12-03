#ifndef EXCEPTIONS_HPP
#define EXCEPTIONS_HPP
#include <iostream>

class WrongPassword : public std::exception
{
	const char *what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW;
};

#endif
