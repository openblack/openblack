#include <cstdint>
#include <unordered_map>

enum class VillageEthnicities
{
	GLOBAL,
	AZTEC,
	CELTIC,
	EGYPTIAN,
	GREEK,
	INDIAN,
	JAPANESE,
	NORSE,
	TIBETAN,
	AFRICAN
};

const std::unordered_map<std::string, VillageEthnicities> VillageEthnicitiesLookup {
	{ "AZTEC", VillageEthnicities::AZTEC },
	{ "CELTIC", VillageEthnicities::CELTIC },
	{ "EGYPTIAN", VillageEthnicities::EGYPTIAN },
	{ "GREEK", VillageEthnicities::GREEK },
	{ "INDIAN", VillageEthnicities::INDIAN },
	{ "JAPANESE", VillageEthnicities::JAPANESE },
	{ "NORSE", VillageEthnicities::NORSE },
	{ "TIBETAN", VillageEthnicities::TIBETAN },
	{ "AFRICAN", VillageEthnicities::AFRICAN },
	{ "GLOBAL", VillageEthnicities::GLOBAL }
};

enum class VillagerTypes
{
	HOUSEWIFE,
	FARMER,
	FISHERMAN,
	FORESTER,
	BREEDER,
	SHEPHERD,
	MISSIONARY,
	LEADER, // No idea what a leader is but they are spawned in land 2 and so on
	TRADER  // Exists on one of the MP land scripts
};

const std::unordered_map<std::string, VillagerTypes> VillagerTypesLookup {
	{ "HOUSEWIFE", VillagerTypes::HOUSEWIFE }, // This is how the game decides male or female villagers lol
	{ "FARMER", VillagerTypes::FARMER },
	{ "FISHERMAN", VillagerTypes::FISHERMAN },
	{ "FORESTER", VillagerTypes::FORESTER },
	{ "BREEDER", VillagerTypes::BREEDER },
	{ "SHEPHERD", VillagerTypes::SHEPHERD },
	{ "MISSIONARY", VillagerTypes::MISSIONARY },
	{ "LEADER", VillagerTypes::LEADER },
	{ "TRADER", VillagerTypes::TRADER }
};

enum class VillagerTasks
{
	IDLE
};

enum class VillagerSex
{
	MALE,
	FEMALE
};

struct Villager
{
	uint32_t health;
	uint32_t age;
	uint32_t hunger;
	VillagerSex sex;
	VillageEthnicities ethnicity;
	VillagerTypes type;
	VillagerTasks task;
};
