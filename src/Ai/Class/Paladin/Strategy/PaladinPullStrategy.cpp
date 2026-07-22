/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "PaladinPullStrategy.h"

#include "GenericBuffUtils.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"

#include <array>
#include <limits>

std::string PaladinPullStrategy::GetPullActionName() const
{
    Player* bot = botAI->GetBot();
    if (!bot)
        return "";

    bool const isTank = botAI->HasStrategy("tank", BOT_STATE_COMBAT) ||
                        botAI->HasStrategy("tank", BOT_STATE_NON_COMBAT);
    static std::array<std::string, 5> const actions = {
        "avenger's shield", "hand of reckoning", "judgement of wisdom",
        "judgement of light", "judgement of justice"
    };
    size_t const firstAction = isTank ? 0 : 2;

    Unit* target = GetTarget();
    bool const validateTarget = target && HasPullStarted();
    std::string readyAction;
    std::string cooldownAction;
    uint32 shortestCooldown = std::numeric_limits<uint32>::max();
    for (size_t i = firstAction; i < actions.size(); ++i)
    {
        uint32 const spellId = botAI->GetAiObjectContext()->GetValue<uint32>("spell id", actions[i])->Get();
        if (!spellId || !bot->HasSpell(spellId))
            continue;

        uint32 const cooldown = ai::spell::GetSpellOrCategoryCooldownDelay(bot, spellId);
        if (!cooldown)
        {
            if (!validateTarget)
                return actions[i];

            if (readyAction.empty())
                readyAction = actions[i];

            if (botAI->CanCastSpell(spellId, target))
                return actions[i];

            continue;
        }

        if (cooldown < shortestCooldown)
        {
            cooldownAction = actions[i];
            shortestCooldown = cooldown;
        }
    }

    // Keep the first ready action stable through transient cast restrictions
    // such as a pre-action's global cooldown.
    if (!readyAction.empty())
        return readyAction;

    // Keep a concrete action selected while all learned alternatives cool down.
    // The generic "judgement" alias has no spell id and cannot execute a pull.
    return cooldownAction;
}

std::string PaladinPullStrategy::GetPreActionName() const
{
    if (botAI->HasStrategy("tank", BOT_STATE_COMBAT) || botAI->HasStrategy("tank", BOT_STATE_NON_COMBAT))
        return "";

    return PullStrategy::GetPreActionName();
}
