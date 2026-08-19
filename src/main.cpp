#include "includes.h"

namespace {
    void SetupLog() {
        const auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
        const auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
        auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
        auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
        spdlog::set_default_logger(std::move(loggerPtr));
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_on(spdlog::level::trace);
    }

    struct EntryPointPerkEntryData {

    };
    struct AbilityPerkEntryData {

    };

    struct PerkEntryData {
        PERK_ENTRY_TYPE type;
        void* data;

        json toJson() const {
            nlohmann::json j{
                {"type", type}
            };

            return j;
        }
    };

    struct PerkData {
        std::string name;
        std::string formID;
        std::string nextPerkFormID;
        std::vector<PerkEntryData> perkEntries;

        json toJson() const {
            nlohmann::json j{
                {"name", name},
                {"formID", formID},
                {"nextPerk", nextPerkFormID},
                {"entries", nlohmann::json::array()}
            };

            for (const auto& entry: perkEntries) {
                j["entries"].push_back(entry.toJson());
            }

            return j;
        }
    };

    struct SkillNodeData {
        std::uint32_t nodeIndex;
        std::string perkFormID;
        std::vector<SkillNodeData> children;

        json toJson() const {
            nlohmann::json j{
                {"nodeIndex", nodeIndex},
                {"perk", perkFormID},
                {"children", nlohmann::json::array()}
            };

            for (const auto& child: children) {
                j["children"].push_back(child.toJson());
            }

            return j;
        }
    };

    struct SkillTreeData {
    public:
        std::string name;
        std::string formID;
        SkillNodeData tree;

        json toJson() const {
            return {
                {"name", name},
                {"formID", formID},
                {"tree", tree.toJson()}
            };
        }
    };

    struct RootData {
        std::vector<SkillTreeData> skills;
        std::unordered_map<std::string, PerkData> perks;

        json toJson() const {
            nlohmann::json j{
                {"perks", nlohmann::json::object()},
                {"skills", nlohmann::json::array()}
            };

            for (const auto& [formID, perk]: perks) {
                j["perks"][formID] = perk.toJson();
            }

            for (const auto& skill: skills) {
                j["skills"].push_back(skill.toJson());
            }

            return j;
        }
    };

    void addPerkData(const BGSPerk* perk, RootData& data) {
        if (!perk) {
            return;
        }

        const auto formID = std::format("{:08X}", perk->GetFormID());

        // Already exported.
        if (data.perks.contains(formID)) {
            return;
        }

        PerkData perkData{
            .name = perk->GetName(),
            .formID = formID,
            .nextPerkFormID = "",
            .perkEntries = {}
        };

        if (perk->nextPerk) {
            perkData.nextPerkFormID = std::format("{:08X}", perk->nextPerk->GetFormID());
        }

        for (const auto* entry: perk->perkEntries) {
            if (!entry) {
                continue;
            }

            PerkEntryData entryData{
                .type = entry->GetType(),
                .data = nullptr
            };

            // We'll populate data based on the entry type.
            perkData.perkEntries.push_back(entryData);
        }

        data.perks.emplace(formID, std::move(perkData));
    }

    SkillNodeData traverseNode(
        BGSSkillPerkTreeNode* node,
        RootData& data,
        std::unordered_set<BGSSkillPerkTreeNode*>& visitedNodes) {
        if (!node || visitedNodes.contains(node)) {
            return {};
        }

        visitedNodes.insert(node);

        SkillNodeData result{
            .nodeIndex = node->index,
            .perkFormID = "",
            .children = {}
        };

        if (node->perk) {
            const auto* perk = node->perk;

            result.perkFormID = std::format("{:08X}", perk->GetFormID());

            // Add PerkData to data.perks here.
            addPerkData(perk, data);
        }

        for (auto* child: node->children) {
            if (!child) {
                continue;
            }

            result.children.push_back(
                traverseNode(child, data, visitedNodes)
            );
        }

        return result;
    }

    void exportPerkTrees() {
        // Iterate over all skill trees
        const ActorValueList* actorValueList = ActorValueList::GetSingleton();

        RootData data = {
            .skills = std::vector<SkillTreeData>()
        };

        for (const auto& info: actorValueList->actorValues) {
            if (!info || !info->perkTree || info->type != ActorValueInfo::ActorValueType::kSkill) {
                continue;
            }
            SkillTreeData skill{
                .name = info->GetName(),
                .formID = std::format("{:08X}", info->GetFormID()),
                .tree = {}
            };
            std::unordered_set<BGSSkillPerkTreeNode*> visitedNodes;

            skill.tree = traverseNode(
                info->perkTree,
                data,
                visitedNodes
            );

            data.skills.push_back(std::move(skill));
        }

        const auto json = data.toJson();

        std::ofstream file("Data/SKSE/Plugins/skilltree.json");

        if (!file) {
            logs::error("Failed to open skilltree.json for writing");
            return;
        }

        file << json.dump(4);

        if (!file) {
            logs::error("Failed to write skilltree.json");
            return;
        }

        logs::info("Successfully exported skill trees");
    }
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);
    SetupLog();
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) exportPerkTrees();
    });
    return true;
}
