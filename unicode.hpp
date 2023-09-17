#ifndef UNICODE_HPP
#define UNICODE_HPP

#include <iterator>

class UnicodeError : public std::runtime_error {
public:
	UnicodeError(std::string what) : std::runtime_error(what) {}
};

template<typename It>
class Utf8Validator {
	It it;
	bool validated;
	int cont;
	int rem_cont;
	uint32_t cp_acc;
	
public:
	Utf8Validator(It it) : it(it), validated(false), cont(0), rem_cont(0), cp_acc(0) {}
	
	char8_t operator*();
	Utf8Validator<It>& operator++();
	bool operator==(Utf8Validator<It> const&) const;
	
	// Throws if it would be invalid to end string here
	void stop();
};

template<typename It>
void encodeUtf8(It out, char32_t cp);

// Assumes input has already been validated
template<typename It>
char32_t unsafeDecodeUtf8(It& in);

#include "unicode.tpp"

#endif
