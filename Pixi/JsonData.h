#include <cstring>
#include <cstdint>
#include <type_traits>
#include "json/json.hpp"

template <typename T, typename = std::enable_if_t<sizeof(T) == 1>>
uint8_t to_byte(T field)
{
	uint8_t ret;
	memcpy(&ret, &field, 1);
	return ret;
}

template <typename T, typename = std::enable_if_t<sizeof(T) == 1>>
T from_byte(uint8_t byte) {
	T f{};
	memcpy(&f, &byte, 1);
	return f;
}

struct J1656 {
	uint8_t objclip : 4;
	bool canbejumpedon : 1;
	bool diesjumpedon : 1;
	bool hopinshell : 1;
	bool disappearsmoke : 1;

	J1656() = default;
	J1656(const nlohmann::json& byte);
};
static_assert(sizeof(J1656) == 1);

struct J1662 {
	uint8_t sprclipping : 6;
	bool deathframe : 1;
	bool falldown : 1;

	J1662() = default;
	J1662(const nlohmann::json& byte);
};
static_assert(sizeof(J1662) == 1);

struct J166E {
	bool usesecondgfxpage : 1;
	uint8_t palette : 3;
	bool disfireballkill : 1;
	bool discapekill : 1;
	bool diswatersplash : 1;
	bool dislayer2interact : 1;

	J166E() = default;
	J166E(const nlohmann::json& byte);
};
static_assert(sizeof(J166E) == 1);

struct J167A {
	bool starclip : 1;
	bool invincblk : 1;
	bool offscreen : 1;
	bool shellstunned : 1;
	bool kicklikeshell : 1;
	bool everyframeinteraction : 1;
	bool givepowerupeaten : 1;
	bool nodefaultinteration : 1;

	J167A() = default;
	J167A(const nlohmann::json& byte);
};
static_assert(sizeof(J167A) == 1);

struct J1686 {
	bool inedible : 1;
	bool staymouth : 1;
	bool groundbehavior : 1;
	bool sprinteract : 1;
	bool changedir : 1;
	bool turncoin : 1;
	bool spawnspr : 1;
	bool objinteract : 1;

	J1686() = default;
	J1686(const nlohmann::json& byte);
};
static_assert(sizeof(J1686) == 1);

struct J190F {
	bool passfrombelow : 1;
	bool noerase : 1;
	bool slidekill : 1;
	bool fireballs : 1;
	bool jumpedwithspeed : 1;
	bool twotiledeath : 1;
	bool nosilvercoin : 1;
	bool nostuckwalls : 1;

	J190F() = default;
	J190F(const nlohmann::json& byte);
};
static_assert(sizeof(J190F) == 1);

enum class JsonFieldType {
	Json,
	Byte
};

template <typename J, typename = std::enable_if_t<sizeof(J) == 1>>
class JsonData {
	JsonFieldType type;
	union {
		J s;
		uint8_t byte;
	} data;
	J get_as_json() {
		if (type != JsonFieldType::Json) {
			type = JsonFieldType::Json;
			data.s = from_byte<J>(data.byte);
		}
		return data.s;
	}
	uint8_t get_as_byte() {
		if (type != JsonFieldType::Byte) {
			type = JsonFieldType::Byte;
			data.byte = to_byte(data.s);
		}
		return data.byte;
	}
public:
	JsonData(uint8_t d) {
		type = JsonFieldType::Byte;
		data.byte = d;
	};
	JsonData(const nlohmann::json& json, const char* name) {
		type = JsonFieldType::Json;
		auto& byte = json[name];
		data.s = J{ byte };
	}
	template <typename T, typename = std::enable_if_t < std::is_same<T, J>::value || std::is_same<T, uint8_t>::value>>
	T get() {
		if constexpr (std::is_same<T, J>::value) {
			return get_as_json();
		}
		else {
			return get_as_byte();
		}
	}
};