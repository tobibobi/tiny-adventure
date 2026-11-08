/*
 * game.cpp - the Game class and the turn loop: movement, rules and built-in verbs.
 */
#include "advengine/advengine.h"
#include "internal.h"

#include <format>

namespace adv
{

using namespace detail;

struct Game::Impl {
    explicit Impl(std::ostream &os) : out(os)
    {
    }

    World w;
    Output out;

    void check_loaded() const;
    void look(bool full);
    void finish(std::string_view banner);
    void go(int dir);
    [[nodiscard]] bool cond_holds(const Cond &cond) const;
    [[nodiscard]] bool rule_matches(const Rule &rule, const Command &cmd) const;
    void run_rule(const Rule &rule);
    void inventory();
    void end_turn();
    bool reject(const Command &cmd);
    bool builtin(const Command &cmd);
};

namespace
{

/* True for the noun resolution results that mean "not understood". */
bool unresolved(int obj)
{
    return obj == obj_absent || obj == obj_unknown;
}

} // namespace

void Game::Impl::check_loaded() const
{
    if (w.rooms.empty())
        throw std::logic_error("adv::Game used before load()");
}

void Game::Impl::look(bool full)
{
    if (!is_lit(w)) {
        out.say("It is pitch dark. You are likely to be eaten by a grue.");
        return;
    }
    Room &room = w.room(w.here);
    out.say(room.name);
    if ((full || !room.seen) && !room.desc.empty())
        out.say(room.desc);
    room.seen = true;

    for (const Object &obj : w.objects)
        if (obj.loc == w.here && obj.portable) {
            out.out("You can see");
            out.out(obj.name);
            out.say("here.");
        }

    std::string exits = "Exits:";
    bool any = false;
    for (std::size_t d = 0; d < dir_count; ++d)
        if (room.exits[d].to >= 0) {
            exits += any ? ", " : " ";
            exits += dir_names[d];
            any = true;
        }
    if (any)
        out.say(exits);
}

void Game::Impl::finish(std::string_view banner)
{
    out.say("");
    out.say(banner);
    out.say(
        std::format("You scored {} out of {} points in {} moves.", w.score, w.max_score, w.moves));
    w.game_over = true;
}

void Game::Impl::go(int dir)
{
    const Exit &exit = w.room(w.here).exits[static_cast<std::size_t>(dir)];
    if (exit.to < 0) {
        out.say("You can't go that way.");
        return;
    }
    if (exit.flag >= 0 && !w.flag_set(exit.flag)) {
        out.say(exit.blocked.value_or("The way is blocked."));
        return;
    }
    w.here = exit.to;
    look(false);
}

bool Game::Impl::cond_holds(const Cond &cond) const
{
    switch (cond.type) {
    case CondType::in:
        return w.here == cond.arg;
    case CondType::have:
        return w.object(cond.arg).loc == carried;
    case CondType::here:
        return can_see(w, cond.arg);
    case CondType::flag:
        return w.flag_set(cond.arg);
    case CondType::not_flag:
        return !w.flag_set(cond.arg);
    }
    return false;
}

bool Game::Impl::rule_matches(const Rule &rule, const Command &cmd) const
{
    if (rule.verb != cmd.verb)
        return false;
    if (rule.obj != any_object && rule.obj != cmd.o1)
        return false;
    if (rule.with == no_second && cmd.o2 != no_object)
        return false;
    if (rule.with >= 0 && rule.with != cmd.o2)
        return false;
    return std::ranges::all_of(rule.conds, [this](const Cond &c) { return cond_holds(c); });
}

void Game::Impl::run_rule(const Rule &rule)
{
    for (std::size_t i = 0; i < rule.effects.size() && !w.game_over; ++i) {
        const Effect &effect = rule.effects[i];
        switch (effect.type) {
        case EffectType::say: // consecutive SAY lines flow as one paragraph
            out.out(effect.text);
            if (i + 1 >= rule.effects.size() || rule.effects[i + 1].type != EffectType::say)
                out.say("");
            break;
        case EffectType::set:
            w.set_flag(effect.a, true);
            break;
        case EffectType::clear:
            w.set_flag(effect.a, false);
            break;
        case EffectType::move:
            w.object(effect.a).loc = effect.b;
            break;
        case EffectType::score:
            w.score += effect.a;
            break;
        case EffectType::go_to:
            w.here = effect.a;
            look(false);
            break;
        case EffectType::look:
            look(true);
            break;
        case EffectType::win:
            finish("*** You have won ***");
            break;
        case EffectType::die:
            finish("*** You have died ***");
            break;
        }
    }
}

void Game::Impl::inventory()
{
    bool any = false;
    for (const Object &obj : w.objects)
        if (obj.loc == carried) {
            if (!any)
                out.say("You are carrying:");
            out.raw("  ");
            out.say(obj.name);
            any = true;
        }
    if (!any)
        out.say("You are empty-handed.");
}

/* Called after every action that takes game time. */
void Game::Impl::end_turn()
{
    ++w.moves;
    if (w.game_over)
        return;
    if (is_lit(w)) {
        w.dark_turns = 0;
        return;
    }
    ++w.dark_turns;
    if (w.dark_turns == 2)
        out.say("You hear something shuffling closer in the dark...");
    if (w.dark_turns >= 3) {
        out.say("Something with far too many teeth finds you. You have been eaten by a grue.");
        finish("*** You have died ***");
    }
}

/* Report unknown verbs/nouns. Returns true if the command can't be executed. */
bool Game::Impl::reject(const Command &cmd)
{
    if (!verb_known(w, cmd.verb)) {
        out.say(std::format("I don't know how to \"{}\".", cmd.verb));
    } else if (cmd.o1 == obj_unknown || cmd.o2 == obj_unknown) {
        out.say(
            std::format("I don't know the word \"{}\".", cmd.o1 == obj_unknown ? cmd.n1 : cmd.n2));
    } else if (cmd.o1 == obj_absent || cmd.o2 == obj_absent) {
        out.say(std::format("You don't see any {} here.", cmd.o1 == obj_absent ? cmd.n1 : cmd.n2));
    } else {
        return false;
    }
    return true;
}

/* Built-in verbs. Returns true if the action took a turn. */
bool Game::Impl::builtin(const Command &cmd)
{
    const std::string &verb = cmd.verb;
    Object *obj = cmd.o1 >= 0 ? &w.object(cmd.o1) : nullptr;

    if (verb == "look") {
        look(true);
    } else if (verb == "inventory") {
        inventory();
    } else if (verb == "wait") {
        out.say("Time passes.");
    } else if (verb == "examine") {
        if (!obj) {
            out.say("Examine what?");
            return false;
        }
        out.say(obj->desc.empty() ? "You see nothing special." : obj->desc);
    } else if (verb == "take") {
        if (!obj) {
            out.say("Take what?");
            return false;
        }
        if (obj->loc == carried) {
            out.say("You already have it.");
        } else if (!obj->portable) {
            out.say("You can't take that.");
        } else {
            obj->loc = carried;
            out.say("Taken.");
        }
    } else if (verb == "drop") {
        if (!obj) {
            out.say("Drop what?");
            return false;
        }
        if (obj->loc != carried) {
            out.say("You don't have that.");
        } else {
            obj->loc = w.here;
            out.say("Dropped.");
        }
    } else if (verb == "score") {
        out.say(std::format("You have scored {} out of {} points in {} moves.", w.score,
                            w.max_score, w.moves));
        return false;
    } else if (verb == "help") {
        out.say("Type short commands, for example: NORTH (or N), TAKE LAMP, EXAMINE LEAVES, "
                "UNLOCK GRATE WITH KEY, INVENTORY (I), LOOK (L), SCORE, QUIT.");
        return false;
    } else if (verb == "quit") {
        finish("You give up.");
        return false;
    } else {
        out.say("That doesn't seem to do anything.");
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* Public API                                                         */
/* ------------------------------------------------------------------ */

Game::Game(std::ostream &out) : impl_(std::make_unique<Impl>(out))
{
}

Game::~Game() = default;
Game::Game(Game &&) noexcept = default;
Game &Game::operator=(Game &&) noexcept = default;

void Game::load(const std::filesystem::path &path)
{
    impl_->w = load_world(path);
}

void Game::start()
{
    impl_->check_loaded();
    Output &out = impl_->out;
    const World &w = impl_->w;
    out.reset_column();
    if (!w.title.empty()) {
        out.say(w.title);
        out.say("");
    }
    if (!w.intro.empty()) {
        out.say(w.intro);
        out.say("");
    }
    impl_->look(true);
}

bool Game::is_over() const
{
    return impl_->w.game_over;
}

void Game::turn(std::string_view line)
{
    impl_->check_loaded();
    Impl &g = *impl_;
    g.out.reset_column();
    std::optional<Command> parsed = parse(g.w, line);
    if (!parsed)
        return;
    Command &cmd = *parsed;

    // The classic parser couldn't make sense of it: give the fallback a go.
    if (!verb_known(g.w, cmd.verb) || unresolved(cmd.o1) || unresolved(cmd.o2))
        fallback_parse(g.w, line, cmd);
    if (g.reject(cmd))
        return;

    if (cmd.verb == "go") {
        if (cmd.dir < 0) {
            g.out.say("Go where?");
            return;
        }
        g.go(cmd.dir);
        g.end_turn();
        return;
    }

    // World rules take priority over built-in verbs.
    for (const Rule &rule : g.w.rules)
        if (g.rule_matches(rule, cmd)) {
            g.run_rule(rule);
            g.end_turn();
            return;
        }

    if (g.builtin(cmd))
        g.end_turn();
}

} // namespace adv
