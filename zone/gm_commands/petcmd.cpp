#include "../client.h"

void command_petcmd(Client *c, const Seperator *sep) {
    // Basic null pointer checks
    if (!c || !sep) {
        return;
    }

    const std::string usage = "Usage: #petcmd [attack, qattack, follow, guard, sit, stop, taunt (on\\off), hold (on\\off), ghold (on\\off), spellhold (on\\off), focus (on\\off), back, regroup (on\\off), assist (on\\off), health, leader, feign, leave] [all, mag, bst, nec, enc, shm, dru, brd, shd]. Verbs do not need to be applied in any specific order.";

    // Safely build a vector of args
    std::vector<std::string> args;
    // argnum is the index of the last argument, not the count
    for (uint16 i = 0; i <= sep->argnum && i < sep->GetMaxArgNum(); ++i) {
        if (sep->arg[i] != nullptr) {
            args.push_back(sep->arg[i]);
        }
    }

    // Check if we have at least one argument
    if (args.empty()) {
        c->Message(Chat::White, usage.c_str());
        return;
    }

    // Track commands and targets
    bool all_classes = false;
    std::vector<int> command_codes;
    std::vector<uint8> class_targets;

    // Define mappings for class targets
    std::map<std::string, uint8> class_map = {
        {"mag", Class::Magician}, {"mage", Class::Magician}, {"magician", Class::Magician},
        {"bst", Class::Beastlord}, {"bl", Class::Beastlord}, {"beast", Class::Beastlord}, {"beastlord", Class::Beastlord},
        {"nec", Class::Necromancer}, {"necro", Class::Necromancer}, {"necromancer", Class::Necromancer},
        {"enc", Class::Enchanter}, {"ench", Class::Enchanter}, {"enchanter", Class::Enchanter},
        {"shm", Class::Shaman}, {"shaman", Class::Shaman},
        {"dru", Class::Druid}, {"druid", Class::Druid},
        {"brd", Class::Bard}, {"bard", Class::Bard},
        {"shd", Class::ShadowKnight}, {"sk", Class::ShadowKnight}, {"shadowknight", Class::ShadowKnight},
        {"war", Class::Warrior}, {"warrior", Class::Warrior},
        {"clr", Class::Cleric}, {"cler", Class::Cleric}, {"cleric", Class::Cleric},
        {"pal", Class::Paladin}, {"paladin", Class::Paladin},
        {"rng", Class::Ranger}, {"ranger", Class::Ranger},
        {"mnk", Class::Monk}, {"monk", Class::Monk},
        {"rog", Class::Rogue}, {"rogue", Class::Rogue},
        {"wiz", Class::Wizard}, {"wizard", Class::Wizard},
        {"ber", Class::Berserker}, {"berserker", Class::Berserker}
    };

    // Define mappings for normal commands
    std::map<std::string, int> command_map = {
        {"attack", PET_ATTACK},
        {"qattack", PET_QATTACK},
        {"follow", PET_FOLLOWME}, {"followme", PET_FOLLOWME},
        {"guard", PET_GUARDHERE},
        {"sit", PET_SIT},
        {"stop", PET_STOP}, {"freeze", PET_STOP},
        {"back", PET_BACKOFF}, {"backoff", PET_BACKOFF},
        {"leave", PET_GETLOST}, {"dismiss", PET_GETLOST}, {"getlost", PET_GETLOST},
        {"health", PET_HEALTHREPORT}, {"healthreport", PET_HEALTHREPORT}, {"hp", PET_HEALTHREPORT}, {"stats", PET_HEALTHREPORT}, {"inventory", PET_HEALTHREPORT}, {"inv", PET_HEALTHREPORT},
        {"leader", PET_LEADER}, {"master", PET_LEADER},
        {"feign", PET_FEIGN}, {"fd", PET_FEIGN}, {"playdead", PET_FEIGN}
    };

    // Define mappings for toggle commands with their on/off variants
    struct ToggleCommand {
        int base;
        int on;
        int off;
    };

    std::map<std::string, ToggleCommand> toggle_command_map = {
        {"taunt", {PET_TAUNT, PET_TAUNT_ON, PET_TAUNT_OFF}},
        {"hold", {PET_HOLD, PET_HOLD_ON, PET_HOLD_OFF}},
        {"ghold", {PET_GHOLD, PET_GHOLD_ON, PET_GHOLD_OFF}},
        {"spellhold", {PET_SPELLHOLD, PET_SPELLHOLD_ON, PET_SPELLHOLD_OFF}},
        {"nocast", {PET_SPELLHOLD, PET_SPELLHOLD_ON, PET_SPELLHOLD_OFF}},
        {"focus", {PET_FOCUS, PET_FOCUS_ON, PET_FOCUS_OFF}},
        {"regroup", {PET_REGROUP, PET_REGROUP_ON, PET_REGROUP_OFF}},
        {"assist", {CUSTOM_PET_ASSIST, CUSTOM_PET_ASSIST_ON, CUSTOM_PET_ASSIST_OFF}},
        {"assistme", {CUSTOM_PET_ASSIST, CUSTOM_PET_ASSIST_ON, CUSTOM_PET_ASSIST_OFF}}
    };

	// Define multi-word commands and their corresponding codes
	std::map<std::pair<std::string, std::string>, int> multi_word_commands = {
		{{"follow", "me"}, PET_FOLLOWME},
		{{"guard", "here"}, PET_GUARDHERE},
		{{"get", "lost"}, PET_GETLOST},
		{{"assist", "me"}, CUSTOM_PET_ASSIST},
		{{"back", "off"}, PET_BACKOFF},
		{{"health", "report"}, PET_HEALTHREPORT},
		{{"play", "dead"}, PET_FEIGN}
	};

	// Process multi-word commands first
	std::vector<bool> arg_processed(args.size(), false);
	for (size_t i = 0; i < args.size() - 1; ++i) {
		if (arg_processed[i]) continue;

		std::string first_word = Strings::ToLower(args[i]);
		std::string second_word = Strings::ToLower(args[i+1]);

		auto it = multi_word_commands.find({first_word, second_word});
		if (it != multi_word_commands.end()) {
			command_codes.push_back(it->second);
			arg_processed[i] = true;
			arg_processed[i+1] = true;
		}
	}

	// Process each remaining argument
	for (size_t i = 0; i < args.size(); ++i) {
		// Skip already processed arguments
		if (arg_processed[i]) {
			continue;
		}

		const std::string& arg = Strings::ToLower(args[i]);

        // Check if it's "all"
        if (arg == "all") {
            all_classes = true;
            continue;
        }

        // Check if it's a class target
        auto class_it = class_map.find(arg);
        if (class_it != class_map.end()) {
            class_targets.push_back(class_it->second);
            continue;
        }

        // Check if it's a normal command
        auto cmd_it = command_map.find(arg);
        if (cmd_it != command_map.end()) {
            command_codes.push_back(cmd_it->second);
            continue;
        }

        // Check if it's a toggle command
        auto toggle_it = toggle_command_map.find(arg);
        if (toggle_it != toggle_command_map.end()) {
            // Check for on/off modifier
            int cmd_code = toggle_it->second.base; // Default to base command

            // Look ahead for on/off
            if (i + 1 < args.size()) {
                const std::string& next_arg = Strings::ToLower(args[i + 1]);
                if (next_arg == "on") {
                    cmd_code = toggle_it->second.on;
                    i++; // Skip the modifier
                }
                else if (next_arg == "off") {
                    cmd_code = toggle_it->second.off;
                    i++; // Skip the modifier
                }
            }

            command_codes.push_back(cmd_code);
            continue;
        }

        // Unrecognized argument - silently ignore
    }

    // If no commands specified, show usage
    if (command_codes.empty()) {
        c->Message(Chat::White, usage.c_str());
        return;
    }

    // If no classes specified, default to all
    if (class_targets.empty() && !all_classes) {
        all_classes = true;
    }

    // Get the list of pets - safely handle null/empty cases
    auto pets = c->GetAllPets();
    if (pets.empty()) {
        c->Message(Chat::White, "You don't have any pets under your control.");
        return;
    }

    // Execute all commands on matching pets
    for (auto pet : pets) {
        // Skip invalid pets
        if (!pet || !pet->IsNPC()) {
            continue;
        }

        auto pet_class_id = pet->CastToNPC()->GetPetOriginClass();

        // Check if this pet's class matches our targets
        if (all_classes || std::find(class_targets.begin(), class_targets.end(), pet_class_id) != class_targets.end()) {
            // Execute all commands on this pet
            for (int cmd : command_codes) {
                // Get target safely
                Mob* target = c->GetTarget();
                pet->CastToNPC()->DoPetCommand(cmd, target);
            }
        }
    }
}