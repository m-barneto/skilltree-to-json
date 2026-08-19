#include "includes.h"

namespace {
    struct EntryPointPerkEntryData {

    };
    struct AbilityPerkEntryData {

    };

    struct PerkEntryData {
        PERK_ENTRY_TYPE type;
        std::uint8_t rank{};
        std::uint8_t priority{};
        void* data;

        json toJson() {
        }
    };

    struct PerkData {
        std::string name;
        std::string formId;
        std::vector<PerkEntryData> perkEntries;

        json toJson() {
            json j = {
                {"name", name},
                {"formId", formId},
                {"entries", json::array()}
            };

            for (auto& entry: perkEntries) {
                j["entries"].push_back(entry.toJson());
            }
        }
    };

    struct SkillData {
    public:
        std::string name;
        std::string formId;
        std::vector<PerkData> perks;

        json toJson() {
            json j = {
                {"name", name},
                {"formId", formId},
                {"perks", json::array()}
            };

            for (auto& perk: perks) {
                j["perks"].push_back(perk.toJson());
            }

            return j;
        }
    };

    struct SkillTreeData {
        std::vector<SkillData> skills;

        json toJson() {
            json j = {
                {"skills", json::array()}
            };

            for (auto& skill: skills) {
                j["skills"].push_back(skill.toJson());
            }

            return j;
        }
    };


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

    json perkNodeToJson(BGSSkillPerkTreeNode* node) {
        json result;

        result["index"] = node->index;
        result["position"] = {
            {"gridX", node->perkGridX},
            {"gridY", node->perkGridY},
            {"x", node->horizontalPosition},
            {"y", node->verticalPosition}
        };

        result["parents"] = json::array();

        for (const auto* parent: node->parents) {
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
            BSString description;
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

            for (BGSPerkEntry* perkEntry: perk->perkEntries) {
            }
        } else {
            result["perk"] = nullptr;
        }

        return result;
    }

    void addPerkNode(BGSSkillPerkTreeNode* node, json& perks, std::unordered_set<BGSSkillPerkTreeNode*>& visited) {
        if (!node || visited.contains(node)) {
            return;
        }

        visited.insert(node);

        perks.push_back(perkNodeToJson(node));

        for (const auto& child: node->children) {
            addPerkNode(child, perks, visited);
        }
    }

    json buildPerkTreeJson() {
        json root;

        root["version"] = 1;
        root["skills"] = json::array();

        const ActorValueList* actorValueList = ActorValueList::GetSingleton();

        for (const auto info: actorValueList->actorValues) {
            if (!info || !info->perkTree || info->type != ActorValueInfo::ActorValueType::kSkill) {
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

            std::unordered_set<BGSSkillPerkTreeNode*> visited;

            addPerkNode(
                info->perkTree,
                skill["perks"],
                visited
            );

            root["skills"].push_back(std::move(skill));
        }

        return root;
    }

    void getPerks(BGSPerk* perk, std::vector<PerkData>& perks) {
        // Get perk entries
        std::vector<PerkEntryData> perkEntries = std::vector<PerkEntryData>();
        for (const auto& perkEntry: perk->perkEntries) {
            if (perkEntry->GetType() == PERK_ENTRY_TYPE::kQuest) {
                logs::error("Found perk using quest perkentry: ");
                logs::error("{}", perk->GetName());
                continue;
            }
            PerkEntryData perkEntryData = {
                .type = perkEntry->GetType(),
                .rank = perkEntry->header.rank,
                .priority = perkEntry->header.priority,
                .data = perkEntry->GetFunctionData()
            };
            perkEntries.push_back(perkEntryData);
        }

        // add this perk to the perk list
        PerkData perkData = {
            .name = perk->GetName(),
            .formId = std::format("{:08X}", perk->GetFormID()),
            .perkEntries = perkEntries
        };

        // Call this on every child
        for (const BGSPerk& perk : )
    }

    void exportPerkTrees() {
        // Iterate over all skill trees
        const ActorValueList* actorValueList = ActorValueList::GetSingleton();

        for (const auto& info: actorValueList->actorValues) {
            if (!info || !info->perkTree || info->type != ActorValueInfo::ActorValueType::kSkill) {
                continue;
            }

            std::vector<PerkData> perks = std::vector<PerkData>();
            getPerks(info->perkTree->perk, perks);
            info->perkTree->chi

            // Iterate over all perks



            auto skillData = SkillData {
                .name = info->GetName(),
                .formId = std::format("{:08X}", info->GetFormID()),
                .perks = perks
            };



            json skill;

            skill["name"] = info->GetName();
            skill["formId"] = std::format(
                "{:08X}",
                info->GetFormID()
            );
            skill["treeWidth"] = info->perkTreeWidth;
            skill["perks"] = json::array();

            std::unordered_set<BGSSkillPerkTreeNode*> visited;

            addPerkNode(
                info->perkTree,
                skill["perks"],
                visited
            );

            //root["skills"].push_back(std::move(skill));
        }


        const auto jsonData = buildPerkTreeJson();

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
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse) {
    SKSE::Init(a_skse);
    SetupLog();
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) exportPerkTrees();
    });
    return true;
}
