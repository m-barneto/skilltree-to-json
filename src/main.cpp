
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


void dumpPerkTree(RE::BGSSkillPerkTreeNode* node,
				  std::unordered_set<RE::BGSSkillPerkTreeNode*>& visited,
				  int depth = 0)
{
	if (!node || visited.contains(node)) {
		return;
	}

	visited.insert(node);

	const auto indent = std::string(depth * 2, ' ');

	if (node->perk) {
		logs::info(
			"{}Node {}: {} | FormID {:08X} | Grid ({}, {}) | Pos ({:.2f}, {:.2f})",
			indent,
			node->index,
			node->perk->GetName(),
			node->perk->GetFormID(),
			node->perkGridX,
			node->perkGridY,
			node->horizontalPosition,
			node->verticalPosition
		);
	} else {
		logs::info(
			"{}Node {}: NO PERK | Grid ({}, {})",
			indent,
			node->index,
			node->perkGridX,
			node->perkGridY
		);
	}

	logs::info(
		"{}  parents: {}, children: {}",
		indent,
		node->parents.size(),
		node->children.size()
	);

	for (auto* parent : node->parents) {
		if (!parent) {
			continue;
		}

		if (parent->perk) {
			logs::info(
				"{}  <- parent: {} ({:08X})",
				indent,
				parent->perk->GetName(),
				parent->perk->GetFormID()
			);
		}
	}

	for (auto* child : node->children) {
		dumpPerkTree(child, visited, depth + 1);
	}
}

void iterateSkilltrees()
{
	logs::info("iterating skilltrees");

	auto* actorValueList = RE::ActorValueList::GetSingleton();

	for (std::uint32_t i = 0;
		 i < std::to_underlying(RE::ActorValue::kTotal);
		 ++i)
	{
		auto* info = actorValueList->actorValues[i];

		if (!info || !info->perkTree) {
			continue;
		}

		logs::info(
			"===== {} | FormID {:08X} | width {} =====",
			info->GetName(),
			info->GetFormID(),
			info->perkTreeWidth
		);

		std::unordered_set<RE::BGSSkillPerkTreeNode*> visited;

		dumpPerkTree(
			info->perkTree,
			visited
		);

		logs::info(
			"===== {}: {} nodes =====",
			info->GetName(),
			visited.size()
		);
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse) {
	SKSE::Init(a_skse);
	SetupLog();
	SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
		if (message->type == SKSE::MessagingInterface::kDataLoaded) iterateSkilltrees();
	});
	return true;
}
