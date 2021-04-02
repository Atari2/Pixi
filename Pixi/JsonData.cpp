#include "JsonData.h"

J1656::J1656(const nlohmann::json& byte)
{
	objclip = (((uint8_t)byte["Object Clipping"].get<uint8_t>() & 0x0F) << 0);
	canbejumpedon = byte["Can be jumped on"].get<bool>();
	diesjumpedon = byte["Dies when jumped on"].get<bool>();
	hopinshell = byte["Hop in/kick shell"].get<bool>();
	disappearsmoke = byte["Disappears in cloud of smoke"].get<bool>();
}

J1662::J1662(const nlohmann::json& byte)
{
	sprclipping = (((uint8_t)byte["Sprite Clipping"].get<uint8_t>() & 0x3F) << 0);
	deathframe = byte["Use shell as death frame"].get<bool>();
	falldown = byte["Fall straight down when killed"].get<bool>();
}

J166E::J166E(const nlohmann::json& byte)
{
	usesecondgfxpage = byte["Use second graphics page"].get<bool>();
	palette = ((uint8_t)byte["Palette"].get<uint8_t>() & 0x07);
	disfireballkill = byte["Disable fireball killing"].get<bool>();
	discapekill = byte["Disable cape killing"].get<bool>();
	diswatersplash = byte["Disable water splash"].get<bool>();
	dislayer2interact = byte["Don't interact with Layer 2"].get<bool>();
}

J167A::J167A(const nlohmann::json& byte)
{
	starclip = byte["Don't disable cliping when starkilled"].get<bool>();
	invincblk = byte["Invincible to star/cape/fire/bounce blk."].get<bool>();
	offscreen = byte["Process when off screen"].get<bool>();
	shellstunned = byte["Don't change into shell when stunned"].get<bool>();
	kicklikeshell = byte["Can't be kicked like shell"].get<bool>();
	everyframeinteraction = byte["Process interaction with Mario every frame"].get<bool>();
	givepowerupeaten = byte["Gives power-up when eaten by yoshi"].get<bool>();
	nodefaultinteration = byte["Don't use default interaction with Mario"].get<bool>();
}

J1686::J1686(const nlohmann::json& byte) {
	inedible = byte["Inedible"].get<bool>();
	staymouth = byte["Stay in Yoshi's mouth"].get<bool>();
	groundbehavior = byte["Weird ground behaviour"].get<bool>();
	sprinteract = byte["Don't interact with other sprites"].get<bool>();
	changedir = byte["Don't change direction if touched"].get<bool>();
	turncoin = byte["Don't turn into coin when goal passed"].get<bool>();
	spawnspr = byte["Spawn a new sprite"].get<bool>();
	objinteract = byte["Don't interact with objects"].get<bool>();
}

J190F::J190F(const nlohmann::json& byte)
{
	passfrombelow = byte["Make platform passable from below"].get<bool>();
	noerase = byte["Don't erase when goal passed"].get<bool>();
	slidekill = byte["Can't be killed by sliding"].get<bool>();
	fireballs = byte["Takes 5 fireballs to kill"].get<bool>();
	jumpedwithspeed = byte["Can be jumped on with upwards Y speed"].get<bool>();
	twotiledeath = byte["Death frame two tiles high"].get<bool>();
	nosilvercoin = byte["Don't turn into a coin with silver POW"].get<bool>();
	nostuckwalls = byte["Don't get stuck in walls (carryable sprites)"].get<bool>();
}
