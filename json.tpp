#include "unicode.hpp"

// To avoid stack overflow in destructor
// Could be avoided by writing an explicit iterative destructor
constexpr size_t MAX_DEPTH = 10000;

template<typename It>
struct Parser {
	It cur, end;
	Json* slot;
	
	Parser(It cur, It end, Json* slot) : cur(cur), end(end), slot(slot) {}
	
	bool is(char8_t c) {
		return cur != end && *cur == c;
	}
	bool is_space() {
		if(cur == end) return false;
		char8_t c = *cur;
		return c == u8' ' || c == u8'\x0A' || c == u8'\x0D' || c == u8'\x09';
	}
	bool is_digit() {
		if(cur == end) return false;
		char8_t c = *cur;
		return c >= u8'0' && c <= u8'9';
	}
	bool is_hex() {
		if(cur == end) return false;
		char8_t c = *cur;
		return (c >= u8'0' && c <= u8'9')
			|| (c >= u8'A' && c <= u8'F')
			|| (c >= u8'a' && c <= u8'f');
	}
	static uint16_t parse_hex(char8_t c) {
		if(c >= u8'A' && c <= u8'F') {
			return (uint16_t)(c - u8'A') + 0xA;
		} else if(c >= u8'a' && c <= u8'f') {
			return (uint16_t)(c - u8'a') + 0xA;
		} else {
			return c - u8'0';
		}
	}
	
	uint16_t read_hex() {
		uint16_t cp = 0;
		for(int i = 0; i < 4; i++) {
			if(!is_hex()) {
				throw JsonError("Unfinished hexadecimal code");
			}
			cp = (cp << 4) | parse_hex(*cur);
			++cur;
		}
		return cp;
	}
	
	std::u8string parse_str() {
		std::u8string buf;
		bool escaped = false;
		while(true) {
			if(cur == end) {
				throw JsonError("Unclosed string");
			}
			if(escaped) {
				if(*cur == u8'u') {
					++cur;
					uint32_t cp = read_hex();
					if(cp >= 0xD800 && cp <= 0xDFFF) {
						if(cp >= 0xDC00) throw JsonError("Lone low surrogate");
						cp = 0x10000 + (cp - 0xD800) * 0x400;
						if(!read_literal(u8"\\u")) throw JsonError("Expected low surrogate");
						uint16_t low = read_hex();
						if(low < 0xDC00 || low > 0xDFFF) throw JsonError("Expected low surrogate");
						cp += low - 0xDC00;
					}
					encodeUtf8(std::back_inserter(buf), cp);
				} else {
					if(*cur == u8'\\' || *cur == u8'"' || *cur == u8'/') {
						buf += *cur;
					} else if(*cur == u8'b') {
						buf += u8'\b';
					} else if(*cur == u8'f') {
						buf += u8'\f';
					} else if(*cur == u8'n') {
						buf += u8'\n';
					} else if(*cur == u8'r') {
						buf += u8'\r';
					} else if(*cur == u8't') {
						buf += u8'\t';
					} else {
						throw JsonError("Invalid escape sequence");
					}
					++cur;
				}
				escaped = false;
			} else {
				if(*cur == u8'\\') {
					escaped = true;
					++cur;
				} else if(*cur == u8'"') {
					++cur;
					break;
				} else if(*cur < 0x20) {
					throw JsonError("Control characters must be escaped");
				} else {
					buf += *cur;
					++cur;
				}
			}
		}
		
		buf.shrink_to_fit();
		return buf;
	}
	
	bool read_literal(const char8_t* literal) {
		const char8_t* next = literal;
		while(cur != end && *next && *cur == *next) {
			++cur;
			next++;
		}
		return !*next;
	}
	
	char push_char(std::string& buf) {
		char c = *cur;
		buf += c;
		++cur;
		return c;
	}
	
	void parse_terminal() {
		if(*cur == u8'"') {
			++cur;
			auto str = parse_str();
			slot->type = JSON_STRING;
			new(&slot->vstring) std::u8string(std::move(str));
			
		} else if(is_digit() || *cur == u8'-') {
			std::string chars;
			if(is(u8'-')) push_char(chars);
			if(!is_digit()) throw JsonError("Expected digit after '-'");
			if(push_char(chars) != '0') {
				while(is_digit()) push_char(chars);
			}
			bool is_float = false;
			if(is(u8'.')) {
				is_float = true;
				push_char(chars);
				if(!is_digit()) throw JsonError("Expected digit after '.'");
				do {
					push_char(chars);
				} while(is_digit());
			}
			if(is(u8'e') || is(u8'E')) {
				is_float = true;
				push_char(chars);
				if(is(u8'+') || is(u8'-')) {
					push_char(chars);
				}
				if(!is_digit()) throw JsonError("Expected digit after 'e'");
				do {
					push_char(chars);
				} while(is_digit());
			}
			
			const char* c_begin = chars.c_str();
			const char* c_end = c_begin + chars.size();
			if(is_float) {
				double val = 0.0/0.0;
				auto res = std::from_chars(c_begin, c_end, val);
				if(res.ec != std::errc() || res.ptr != c_end) {
					throw JsonError("Double out of range");
				}
				slot->type = JSON_DOUBLE;
				slot->vdouble = val;
			} else {
				int64_t val = 0;
				auto res = std::from_chars(c_begin, c_end, val);
				if(res.ec != std::errc() || res.ptr != c_end) {
					throw JsonError("Integer out of range");
				}
				slot->type = JSON_INT;
				slot->vint = val;
			}
			
		} else if(*cur == u8't') {
			if(!read_literal(u8"true")) {
				throw JsonError("Expected true");
			}
			slot->type = JSON_BOOL;
			slot->vbool = true;
			
		} else if(*cur == u8'f') {
			if(!read_literal(u8"false")) {
				throw JsonError("Expected false");
			}
			slot->type = JSON_BOOL;
			slot->vbool = false;
			
		} else if(*cur == u8'n') {
			if(!read_literal(u8"null")) {
				throw JsonError("Expected null");
			}
			slot->type = JSON_NULL;
			
		} else {
			throw JsonError("Unexpected character");
		}
	}

	void skip_ws() {
		while(is_space()) {
			++cur;
		}
	}

	void parse_key(Json* object) {
		auto key = parse_str();
		skip_ws();
		
		if(!is(u8':')) {
			throw JsonError("Expected ':'");
		}
		++cur;
		skip_ws();
		
		auto res = object->vobject.emplace(std::move(key), Json());
		
		slot = &(*res.first).second;
	}
};

template<typename It>
std::unique_ptr<Json> Json::parse(It begin, It end) {
	auto res = std::make_unique<Json>();
	Parser<It> p { begin, end, &*res };
	std::vector<Json*> stack;
	
	p.skip_ws();
	while(p.slot) {
		if(p.cur == end) {
			throw JsonError("Unexpected EOF");
		}
		
		if(*p.cur == u8'[') {
			++p.cur;
			p.skip_ws();
			if(p.cur == end) {
				throw JsonError("Unclosed array");
			}
			
			p.slot->type = JSON_ARRAY;
			new(&p.slot->varray) std::vector<Json>();
			
			if(*p.cur == u8']') {
				++p.cur;
				p.skip_ws();
				
			} else {
				if(stack.size() == MAX_DEPTH) {
					throw JsonError("Too much nesting");
				}
				stack.push_back(p.slot);
				p.slot = &p.slot->varray.emplace_back();
				continue;
			}
		
		} else if(*p.cur == u8'{') {
			++p.cur;
			p.skip_ws();
			if(p.cur == end) {
				throw JsonError("Unclosed object");
			}
			
			p.slot->type = JSON_OBJECT;
			new(&p.slot->vobject) std::unordered_map<std::string, Json>();
			
			if(*p.cur == u8'}') {
				++p.cur;
				p.skip_ws();
				
			} else if(*p.cur == u8'"') {
				if(stack.size() == MAX_DEPTH) {
					throw JsonError("Too much nesting");
				}
				++p.cur;
				stack.push_back(p.slot);
				p.parse_key(p.slot);
				continue;
				
			} else {
				throw JsonError("Expected '}' or '\"'");
			}
			
		} else {
			p.parse_terminal();
			p.skip_ws();
		}
		
		// Finished filling slot, find next slot
		p.slot = nullptr;
		while(!p.slot && stack.size() > 0) {
			if(p.cur == end) {
				throw JsonError("Unclosed container");
			}
			
			auto container = stack.back();
			if(container->type == JSON_ARRAY) {
				if(*p.cur == u8']') {
					container->varray.shrink_to_fit();
					stack.pop_back();
				} else if(*p.cur == u8',') {
					p.slot = &container->varray.emplace_back();
				} else {
					throw JsonError("Expected ']' or ','");
				}
				++p.cur;
				p.skip_ws();
				
			} else if(container->type == JSON_OBJECT) {
				if(*p.cur == u8'}') {
					++p.cur;
					p.skip_ws();
					container->vobject.rehash(0);
					stack.pop_back();
				} else if(*p.cur == u8',') {
					++p.cur;
					p.skip_ws();
					if(!p.is(u8'"')) {
						throw JsonError("Expected '\"'");
					}
					++p.cur;
					p.parse_key(container);
				} else {
					throw JsonError("Expected '}' or ','");
				}
			}
		}
	}
	
	if(p.cur != p.end) {
		throw JsonError("Expected EOF");
	}
	
	return res;
}