#include "json.hpp"

#include <cstdint>
#include <cstdio>

#include <charconv>
#include <format>

Json::Json() : type(JSON_NULL) {}
Json::Json(const Json& other) : type(other.type) {
	switch(type) {
	case JSON_BOOL:
		vbool = other.vbool;
		break;
	case JSON_INT:
		vint = other.vint;
		break;
	case JSON_DOUBLE:
		vdouble = other.vdouble;
		break;
	case JSON_STRING:
		new(&vstring) std::u8string(other.vstring);
		break;
	case JSON_ARRAY:
		new(&varray) std::vector<Json>(other.varray);
		break;
	case JSON_OBJECT:
		new(&vobject) std::unordered_map<std::u8string, Json>(other.vobject);
		break;
	default:
		break;
	}
}
Json::Json(Json&& other) : type(other.type) {
	switch(type) {
	case JSON_BOOL:
		vbool = other.vbool;
		break;
	case JSON_INT:
		vint = other.vint;
		break;
	case JSON_DOUBLE:
		vdouble = other.vdouble;
		break;
	case JSON_STRING:
		new(&vstring) std::u8string(std::move(other.vstring));
		break;
	case JSON_ARRAY:
		new(&varray) std::vector<Json>(std::move(other.varray));
		break;
	case JSON_OBJECT:
		new(&vobject) std::unordered_map<std::u8string, Json>(std::move(other.vobject));
		break;
	default:
		break;
	}
}
Json& Json::operator=(const Json& other) {
	this->~Json();
	new(this) Json(other);
	return *this;
}
Json& Json::operator=(Json&& other) {
	this->~Json();
	new(this) Json(std::move(other));
	return *this;
}

Json::~Json() {
	switch(type) {
	case JSON_STRING:
		vstring.~basic_string<char8_t>();
		break;
	case JSON_ARRAY:
		varray.~vector<Json>();
		break;
	case JSON_OBJECT:
		vobject.~unordered_map<std::u8string, Json>();
		break;
	default:
		break;
	}
	type = JSON_NULL;
}

void Json::escape_str(std::ostream& os, std::u8string_view s) {
	os << '\"';
	auto p = s.begin();
	while(p != s.end()) {
		char32_t cp = unsafeDecodeUtf8(p);
		if(cp == U'\\') {
			os << "\\\\";
		} else if(cp == U'"') {
			os << "\\\"";
		} else if(cp == U'\b') {
			os << "\\b";
		} else if(cp == U'\f') {
			os << "\\f";
		} else if(cp == U'\n') {
			os << "\\n";
		} else if(cp == U'\r') {
			os << "\\r";
		} else if(cp == U'\t') {
			os << "\\t";
		} else if(cp < 0x20 || cp >= 0x80) {
			if(cp <= 0xFFFF) {
				os << std::format("\\u{:04x}", (uint32_t) cp);
			} else {
				uint16_t hi = (cp - 0x10000) / 0x400 + 0xD800;
				uint16_t lo = cp % 0x400 + 0xDC00;
				os << std::format("\\u{:x}\\u{:x}", hi, lo);
			}
		} else {
			os.put((uint8_t) cp);
		}
	}
	os << '\"';
}

std::ostream& operator<<(std::ostream& os, const Json& json) {
	switch(json.type) {
		case JSON_NULL:
			return os << "null";
		case JSON_BOOL:
			return os << (json.vbool ? "true" : "false");
		case JSON_INT:
			return os << json.vint;
		case JSON_DOUBLE: {
			// TODO: replace with more human readable "minimum round-tripping digit count" algorithm
			auto s = std::format("{:.17g}", json.vdouble);
			os << s;
			if(s.find('.') == std::string::npos && s.find('e') == std::string::npos) {
				// Makes sure that a JSON_DOUBLE doesn't become a JSON_INT after round trip
				os << ".0";
			}
			return os;
		}
		case JSON_STRING:
			Json::escape_str(os, json.vstring);
			return os;
		case JSON_ARRAY: {
			os << "[";
			auto begin = json.varray.begin();
			for(auto p = begin; p != json.varray.end(); p++) {
				if(p != begin) os << ",";
				os << *p;
			}
			return os << "]";
		}
		case JSON_OBJECT: {
			os << "{";
			auto begin = json.vobject.begin();
			for(auto p = begin; p != json.vobject.end(); p++) {
				if(p != begin) os << ",";
				Json::escape_str(os, p->first);
				os << ":" << p->second;
			}
			return os << "}";
		}
	}
}
