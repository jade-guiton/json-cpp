#ifndef JSON_HPP
#define JSON_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <format>
#include <iostream>

class JsonError : public std::runtime_error {
public:
	JsonError(std::string what) : std::runtime_error(what) {}
};

enum JsonType {
	JSON_NULL,
	JSON_BOOL,
	JSON_INT,
	JSON_DOUBLE,
	JSON_STRING,
	JSON_ARRAY,
	JSON_OBJECT,
};

struct Json {
	JsonType type;
	union {
		bool vbool;
		int64_t vint;
		double vdouble;
		std::u8string vstring;
		std::vector<Json> varray;
		std::unordered_map<std::u8string, Json> vobject;
		// TODO: Add "non-parseable number/string" as an option?
	};
	
	Json();
	Json(const Json&);
	Json(Json&&);
	Json& operator=(const Json&);
	Json& operator=(Json&&);
	~Json();
	
	static void escape_str(std::ostream& os, std::u8string_view s);
	friend std::ostream& operator<<(std::ostream& os, const Json& json);
	
	template<typename It>
	static std::unique_ptr<Json> parse(It begin, It end);
};

#include "json.tpp"

#endif
