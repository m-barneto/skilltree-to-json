
#include "includes.h"

void SetupLog() {
	auto logsFolder = SKSE::log::log_directory();
	if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
	auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
	auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
	auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
	auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
	spdlog::set_default_logger(std::move(loggerPtr));
	spdlog::set_level(spdlog::level::trace);
	spdlog::flush_on(spdlog::level::trace);
}

json perkNodeToJson(RE::BGSSkillPerkTreeNode* node)
{
	json result;

	result["index"] = node->index;
	result["position"] = {
		{"gridX", node->perkGridX},
		{"gridY", node->perkGridY},
		{"x", node->horizontalPosition},
		{"y", node->verticalPosition}
	};

	result["parents"] = json::array();

	for (auto* parent : node->parents) {
		if (!parent) {
			continue;
		}

		if (parent->perk) {
			result["parents"].push_back(
				std::format("{:08X}", parent->perk->GetFormID())
			);
		}
	}

	if (node->perk) {
		auto* perk = node->perk;
		RE::BSString description;
		perk->GetDescription(description, perk, 0);
		result["perk"] = {
			{"formId", std::format("{:08X}", perk->GetFormID())},
			{"name", perk->GetName()},
			{"description", description.c_str()},
		};

		result["perk"]["level"] = perk->data.level;
		result["perk"]["ranks"] = perk->data.numRanks;
		result["perk"]["playable"] = perk->data.playable;
		result["perk"]["hidden"] = perk->data.hidden;

		for (RE::BGSPerkEntry* perkEntry : perk->perkEntries)
		{

		}
	} else {
		result["perk"] = nullptr;
	}

	return result;
}

void addPerkNode(
	RE::BGSSkillPerkTreeNode* node,
	json& perks,
	std::unordered_set<RE::BGSSkillPerkTreeNode*>& visited)
{
	if (!node || visited.contains(node)) {
		return;
	}

	visited.insert(node);

	perks.push_back(perkNodeToJson(node));

	for (auto* child : node->children) {
		addPerkNode(child, perks, visited);
	}
}

json buildPerkTreeJson()
{
	json root;

	root["version"] = 1;
	root["skills"] = json::array();

	auto* actorValueList = RE::ActorValueList::GetSingleton();

	for (std::uint32_t i = 0;
		 i < std::to_underlying(RE::ActorValue::kTotal);
		 ++i)
	{
		auto* info = actorValueList->actorValues[i];

		if (!info || !info->perkTree) {
			continue;
		}

		json skill;

		skill["name"] = info->GetName();
		skill["formId"] = std::format(
			"{:08X}",
			info->GetFormID()
		);
		skill["treeWidth"] = info->perkTreeWidth;
		skill["perks"] = json::array();

		std::unordered_set<RE::BGSSkillPerkTreeNode*> visited;

		addPerkNode(
			info->perkTree,
			skill["perks"],
			visited
		);

		root["skills"].push_back(std::move(skill));
	}

	return root;
}

void exportPerkTrees()
{
	auto jsonData = buildPerkTreeJson();

	std::ofstream file(
		"Data/SKSE/Plugins/SkillTreeToJson.json"
	);

	if (!file) {
		logs::error("Failed to open JSON output file");
		return;
	}

	file << jsonData.dump(2);

	logs::info("Perk trees exported successfully");
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse) {
	SKSE::Init(a_skse);
	SetupLog();
	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
		if (message->type == SKSE::MessagingInterface::kDataLoaded) exportPerkTrees();
	});
	return true;
}
