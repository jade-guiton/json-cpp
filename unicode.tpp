template<typename It>
char8_t Utf8Validator<It>::operator*() {
	uint8_t c = *it;
	if(!validated) {
		if(rem_cont == 0) {
			if(c < 0b10000000) {
				cont = 0;
			} else if(c < 0b11000000) {
				throw UnicodeError("Unexpected continuation byte");
			} else if(c < 0b11100000) {
				cont = 1;
				cp_acc = c & 0b00011111;
				if(cp_acc < 2) {
					throw UnicodeError("Overlong 2-byte encoding");
				}
			} else if(c < 0b11110000) {
				cont = 2;
				cp_acc = c & 0b00001111;
			} else if(c < 0b11111000) {
				cont = 3;
				cp_acc = c & 0b00000111;
			} else {
				throw UnicodeError("Invalid initial byte");
			}
			rem_cont = cont;
		} else if(c < 0b10000000 || c >= 0b11000000) {
			throw UnicodeError("Expected continuation byte");
		} else {
			cp_acc = (cp_acc << 6) | (c & 0b00111111);
			rem_cont--;
			if(rem_cont == 2) {
				if(cp_acc < 0b10000) {
					throw UnicodeError("Overlong 4-byte encoding");
				} else if(cp_acc > 0b100001111) {
					throw UnicodeError("Out of range codepoint");
				}
			}
			if(rem_cont == 1 && cont == 2) {
				if(cp_acc < 0b100000) {
					throw UnicodeError("Overlong 3-byte encoding");
				} else if(cp_acc >> 5 == 0b11011) {
					throw UnicodeError("Unexpected surrogate codepoint");
				}
			}
		}
		validated = true;
	}
	return c;
}

template<typename It>
Utf8Validator<It>& Utf8Validator<It>::operator++() {
	++it;
	validated = false;
	return *this;
}

template<typename It>
bool Utf8Validator<It>::operator==(Utf8Validator<It> const& other) const {
	return it == other.it;
}

template<typename It>
void Utf8Validator<It>::stop() {
	if(validated ? rem_cont != cont : rem_cont != 0) {
		throw UnicodeError("Unexpected end of input");
	}
}

template<typename It>
void encodeUtf8(It out, uint32_t cp) {
	if(cp < 0x80) {
		// ASCII fast path
		*out = (uint8_t) cp;
		return;
	}
	int sh;
	if(cp < 0x800) {
		*out = 0b11000000 | (uint8_t) (cp >> 6);
		sh = 0;
	} else if(cp < 0x10000) {
		if(cp >= 0xD800 && cp <= 0xDFFF) {
			throw UnicodeError("Unexpected surrogate codepoint");
		}
		*out = 0b11100000 | (uint8_t) (cp >> 12);
		sh = 6;
	} else if(cp < 0x110000) {
		*out = 0b11110000 | (uint8_t) (cp >> 18);
		sh = 12;
	} else {
		throw UnicodeError("Out of range codepoint");
	}
	for(; sh >= 0; sh -= 6) {
		*out = 0b10000000 | (uint8_t) ((cp >> sh) & 0b111111);
	}
}

template<typename It>
char32_t unsafeDecodeUtf8(It& in) {
	uint8_t b1 = *in;
	++in;
	if(b1 < 0x80) return b1;
	int cont;
	uint32_t cp;
	if(b1 < 0b11100000) {
		cont = 1;
		cp = b1 & 0b00011111;
	} else if(b1 < 0b11110000) {
		cont = 2;
		cp = b1 & 0b00001111;
	} else {
		cont = 3;
		cp = b1 & 0b00000111;
	}
	for(int i = 0; i < cont; i++) {
		cp = (cp << 6) | (*in & 0b00111111);
		++in;
	}
	return (char32_t) cp;
}
