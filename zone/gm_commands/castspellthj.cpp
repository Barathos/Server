#include "../client.h"

void command_castspellthj(Client *c, const Seperator *sep)
{
    if (SPDAT_RECORDS <= 0) {
        c->Message(Chat::Red, "Spells not loaded.");
        return;
    }

    const auto arguments = sep->argnum;
    if (!arguments) {
        c->Message(
            Chat::White,
            "Usage: #castspell [Spell Name or ID from your spellbook]"
        );
        return;
    }

    // Get full argument string after the command
    std::string spell_string;
    if (sep->msg) {
        std::string full_msg(sep->msg);
        // Find the position after the first word (the command itself)
        size_t pos = full_msg.find_first_of(" \t");
        if (pos != std::string::npos) {
            spell_string = full_msg.substr(pos + 1);
        }
    }

    if (spell_string.empty()) {
        c->Message(Chat::White, "Please specify a spell to cast.");
        return;
    }

	if (c->GetRestTimer())	{
		c->Message(Chat::SpellFailure, "You cannot cast spells from your spellbook while in combat.");
		return;
	}

    uint16 spell_id = 0;
    int spell_book_slot = -1;

    // Check if the argument is a number (spell ID)
    if (sep->IsNumber(1)) {
        spell_id = Strings::ToUnsignedInt(sep->arg[1]);
        spell_book_slot = c->FindSpellBookSlotBySpellID(spell_id);
    } else {
        // Treat as spell name
        spell_book_slot = c->FindSpellBookSlotBySpellName(spell_string);
        if (spell_book_slot >= 0) {
            spell_id = c->GetSpellIDByBookSlot(spell_book_slot);
        }
    }

    // Verify the spell is in the character's spellbook
    if (spell_book_slot == -1 || spell_id == 0 || spell_id >= SPDAT_RECORDS) {
        c->Message(
            Chat::SpellFailure,
            fmt::format(
                "The spell '{}' was not found in your spellbook.",
                spell_string
            ).c_str()
        );
        return;
    }

    Mob* t = c;
    if (c->GetTarget()) {
        t = c->GetTarget();
    }

    uint16 target_id = t->GetID();

	if (spells[spell_id].target_type == ST_Self) {
		target_id = c->GetID();
	}

	c->Message(
        Chat::Spells,
        fmt::format(
            "Casting {} on {} from your Spellbook.",
            GetSpellName(spell_id),
            entity_list.GetMob(target_id)->GetCleanName()
        ).c_str()
    );

    c->CastSpell(spell_id, target_id, EQ::spells::CastingSlot::Gem1);
}