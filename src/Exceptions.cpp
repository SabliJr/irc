#include "../includes/Exceptions.hpp"

const char *WrongPassword::what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_NOTHROW
{
	return ("Wrong password!");
}
