/*
 * world.cpp - world lookups and the world file loader.
 */
#include "advengine/advengine.h"
#include "internal.h"

#include <cctype>
#include <charconv>
#include <format>
#include <fstream>
#include <memory>

namespace adv::detail
{

namespace
{

constexpr std::array<std::string_view, dir_count> dir_abbrevs = {
    "n", "s", "e", "w", "u", "d", "ne", "nw", "se", "sw", "in", "out"};

constexpr std::array<std::pair<std::string_view, std::string_view>, 15> builtin_synonyms = {{
    {"get", "take"},
    {"grab", "take"},
    {"carry", "take"},
    {"pick", "take"},
    {"discard", "drop"},
    {"x", "examine"},
    {"inspect", "examine"},
    {"i", "inventory"},
    {"inv", "inventory"},
    {"l", "look"},
    {"q", "quit"},
    {"z", "wait"},
    {"?", "help"},
    {"walk", "go"},
    {"run", "go"},
}};

constexpr std::array<std::string_view, 10> builtin_verbs = {
    "go", "look", "examine", "take", "drop", "inventory", "score", "help", "quit", "wait"};

} // namespace

/* ------------------------------------------------------------------ */
/* Lookups                                                            */
/* ------------------------------------------------------------------ */

int dir_of(std::string_view word)
{
    for (std::size_t i = 0; i < dir_count; ++i)
        if (word == dir_names[i] || word == dir_abbrevs[i])
            return static_cast<int>(i);
    return -1;
}

std::string_view canon(const World &w, std::string_view word)
{
    for (const auto &[syn, target] : w.synonyms)
        if (syn == word)
            return target;
    for (const auto &[syn, target] : builtin_synonyms)
        if (syn == word)
            return target;
    return word;
}

bool verb_known(const World &w, std::string_view verb)
{
    return contains(builtin_verbs, verb) ||
           std::ranges::any_of(w.rules, [verb](const Rule &r) { return r.verb == verb; });
}

bool is_lit(const World &w)
{
    if (!w.room(w.here).dark)
        return true;
    return std::ranges::any_of(w.objects, [&w](const Object &o) {
        return o.light_flag >= 0 && w.flag_set(o.light_flag) &&
               (o.loc == carried || o.loc == w.here);
    });
}

bool can_see(const World &w, int obj)
{
    const Object &o = w.object(obj);
    return o.loc == carried || (o.loc == w.here && is_lit(w));
}

bool has_word(const Object &obj, std::string_view word)
{
    return contains(obj.words, word);
}

/* ------------------------------------------------------------------ */
/* Loader                                                             */
/* ------------------------------------------------------------------ */

namespace
{

bool is_space(char ch)
{
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
}

/* Split off the next whitespace-separated token. rest is left at the text
 * after it, without leading whitespace. Returns "" when there is none. */
std::string_view next_token(std::string_view &rest)
{
    std::size_t begin = 0;
    while (begin < rest.size() && is_space(rest[begin]))
        ++begin;
    std::size_t end = begin;
    while (end < rest.size() && !is_space(rest[end]))
        ++end;
    const std::string_view token = rest.substr(begin, end - begin);
    while (end < rest.size() && is_space(rest[end]))
        ++end;
    rest.remove_prefix(end);
    return token;
}

/* Append text to a paragraph, separated by a space. */
void append(std::string &text, std::string_view more)
{
    if (!text.empty())
        text += ' ';
    text += more;
}

class Loader
{
  public:
    explicit Loader(std::filesystem::path path) : path_(std::move(path))
    {
    }

    World load();

  private:
    [[noreturn]] void fail(std::string_view msg, std::string_view arg = {}) const;
    std::string_view need(std::string_view &rest, std::string_view what) const;
    int number(std::string_view &rest, std::string_view what) const;
    [[nodiscard]] int find_room(std::string_view id) const;
    [[nodiscard]] int find_object(std::string_view id) const;
    int find_flag(std::string_view name);
    [[nodiscard]] int place(std::string_view where) const;

    void declare(std::string_view kw, std::string_view rest);
    void define(std::string_view kw, std::string_view rest);
    bool global_keyword(std::string_view kw, std::string_view rest);
    bool room_keyword(Room &room, std::string_view kw, std::string_view rest);
    void parse_exit(Room &room, std::string_view rest);
    bool object_keyword(Object &obj, std::string_view kw, std::string_view rest);
    bool rule_condition_keyword(Rule &rule, std::string_view kw, std::string_view rest);
    bool rule_effect_keyword(Rule &rule, std::string_view kw, std::string_view rest);

    std::filesystem::path path_;
    int line_ = 0;
    World w_;

    // The block (ROOM, OBJECT or RULE) that following keywords belong to.
    // Rooms and objects are not added during pass 2, so these stay valid;
    // rule_ is updated whenever a rule is added.
    Room *room_ = nullptr;
    Object *object_ = nullptr;
    Rule *rule_ = nullptr;
};

void Loader::fail(std::string_view msg, std::string_view arg) const
{
    std::string text = std::format("{}:{}: {}", path_.string(), line_, msg);
    if (!arg.empty()) {
        text += ' ';
        text += arg;
    }
    throw LoadError(text);
}

std::string_view Loader::need(std::string_view &rest, std::string_view what) const
{
    const std::string_view token = next_token(rest);
    if (token.empty())
        fail("missing", what);
    return token;
}

int Loader::number(std::string_view &rest, std::string_view what) const
{
    const std::string_view token = need(rest, what);
    int value = 0;
    const char *last = std::to_address(token.end());
    const auto [end, ec] = std::from_chars(std::to_address(token.begin()), last, value);
    if (ec != std::errc{} || end != last)
        fail("expected a number, got", token);
    return value;
}

int Loader::find_room(std::string_view id) const
{
    const auto it = std::ranges::find(w_.rooms, id, &Room::id);
    if (it == w_.rooms.end())
        fail("unknown room", id);
    return static_cast<int>(it - w_.rooms.begin());
}

int Loader::find_object(std::string_view id) const
{
    const auto it = std::ranges::find(w_.objects, id, &Object::id);
    if (it == w_.objects.end())
        fail("unknown object", id);
    return static_cast<int>(it - w_.objects.begin());
}

/* Flags need no declaration: an unknown name creates the flag. */
int Loader::find_flag(std::string_view name)
{
    const auto it = std::ranges::find(w_.flags, name, &Flag::name);
    if (it != w_.flags.end())
        return static_cast<int>(it - w_.flags.begin());
    w_.flags.push_back(Flag{.name = std::string(name)});
    return static_cast<int>(w_.flags.size()) - 1;
}

/* A room id, "carried" or "nowhere". */
int Loader::place(std::string_view where) const
{
    if (where == "carried")
        return carried;
    if (where == "nowhere")
        return nowhere;
    return find_room(where);
}

/* Pass 1: collect room and object ids so forward references work. */
void Loader::declare(std::string_view kw, std::string_view rest)
{
    if (kw == "ROOM") {
        Room &room = w_.rooms.emplace_back();
        room.id = need(rest, "room id");
        room.name = room.id;
    } else if (kw == "OBJECT") {
        Object &obj = w_.objects.emplace_back();
        obj.id = need(rest, "object id");
        obj.name = obj.id;
    }
}

bool Loader::global_keyword(std::string_view kw, std::string_view rest)
{
    if (kw == "TITLE") {
        w_.title = rest;
    } else if (kw == "INTRO") {
        append(w_.intro, rest);
    } else if (kw == "START") {
        w_.start = find_room(need(rest, "room"));
    } else if (kw == "MAXSCORE") {
        w_.max_score = number(rest, "number");
    } else if (kw == "SYN") {
        const std::string_view target = need(rest, "word");
        for (std::string_view word = next_token(rest); !word.empty(); word = next_token(rest))
            w_.synonyms.emplace_back(word, target);
    } else {
        return false;
    }
    return true;
}

void Loader::parse_exit(Room &room, std::string_view rest)
{
    const std::string_view dir = need(rest, "direction");
    const int d = dir_of(dir);
    if (d < 0)
        fail("bad direction", dir);
    Exit &exit = room.exits[static_cast<std::size_t>(d)];
    exit.to = find_room(need(rest, "destination"));
    std::string_view keyword = next_token(rest);
    if (keyword == "IF") {
        exit.flag = find_flag(need(rest, "flag"));
        keyword = next_token(rest);
    }
    if (keyword == "ELSE")
        exit.blocked = std::string(rest);
    else if (!keyword.empty())
        fail("expected IF or ELSE, got", keyword);
}

bool Loader::room_keyword(Room &room, std::string_view kw, std::string_view rest)
{
    if (kw == "NAME")
        room.name = rest;
    else if (kw == "DESC")
        append(room.desc, rest);
    else if (kw == "DARK")
        room.dark = true;
    else if (kw == "EXIT")
        parse_exit(room, rest);
    else
        return false;
    return true;
}

bool Loader::object_keyword(Object &obj, std::string_view kw, std::string_view rest)
{
    if (kw == "NAME") {
        obj.name = rest;
    } else if (kw == "DESC") {
        append(obj.desc, rest);
    } else if (kw == "AT") {
        obj.loc = place(need(rest, "location"));
    } else if (kw == "TAKE") {
        obj.portable = true;
    } else if (kw == "LIGHTIF") {
        obj.light_flag = find_flag(need(rest, "flag"));
    } else if (kw == "WORDS") {
        for (std::string_view word = next_token(rest); !word.empty(); word = next_token(rest))
            obj.words.emplace_back(word);
    } else {
        return false;
    }
    return true;
}

/* Rule matching (OBJ, WITH) and conditions. */
bool Loader::rule_condition_keyword(Rule &rule, std::string_view kw, std::string_view rest)
{
    if (kw == "OBJ") {
        rule.obj = find_object(need(rest, "object"));
    } else if (kw == "WITH") {
        const std::string_view obj = need(rest, "object");
        rule.with = obj == "-" ? no_second : find_object(obj);
    } else if (kw == "IN") {
        rule.conds.push_back({.type = CondType::in, .arg = find_room(need(rest, "room"))});
    } else if (kw == "HAVE") {
        rule.conds.push_back({.type = CondType::have, .arg = find_object(need(rest, "object"))});
    } else if (kw == "HERE") {
        rule.conds.push_back({.type = CondType::here, .arg = find_object(need(rest, "object"))});
    } else if (kw == "FLAG") {
        rule.conds.push_back({.type = CondType::flag, .arg = find_flag(need(rest, "flag"))});
    } else if (kw == "NOT") {
        rule.conds.push_back({.type = CondType::not_flag, .arg = find_flag(need(rest, "flag"))});
    } else {
        return false;
    }
    return true;
}

bool Loader::rule_effect_keyword(Rule &rule, std::string_view kw, std::string_view rest)
{
    const auto add = [&rule](EffectType type, int a = 0, int b = 0, std::string_view text = {}) {
        rule.effects.push_back(Effect{.type = type, .a = a, .b = b, .text = std::string(text)});
    };
    if (kw == "SAY") {
        add(EffectType::say, 0, 0, rest);
    } else if (kw == "SET") {
        add(EffectType::set, find_flag(need(rest, "flag")));
    } else if (kw == "CLEAR") {
        add(EffectType::clear, find_flag(need(rest, "flag")));
    } else if (kw == "SCORE") {
        add(EffectType::score, number(rest, "points"));
    } else if (kw == "GOTO") {
        add(EffectType::go_to, find_room(need(rest, "room")));
    } else if (kw == "WIN") {
        add(EffectType::win);
    } else if (kw == "DIE") {
        add(EffectType::die);
    } else if (kw == "LOOK") {
        add(EffectType::look);
    } else if (kw == "MOVE") {
        const int obj = find_object(need(rest, "object"));
        add(EffectType::move, obj, place(need(rest, "destination")));
    } else {
        return false;
    }
    return true;
}

/* Pass 2: interpret one line. */
void Loader::define(std::string_view kw, std::string_view rest)
{
    if (kw == "ROOM") {
        room_ = &w_.room(find_room(need(rest, "room id")));
        object_ = nullptr;
        rule_ = nullptr;
    } else if (kw == "OBJECT") {
        object_ = &w_.object(find_object(need(rest, "object id")));
        room_ = nullptr;
        rule_ = nullptr;
    } else if (kw == "RULE") {
        rule_ = &w_.rules.emplace_back();
        rule_->verb = need(rest, "verb");
        room_ = nullptr;
        object_ = nullptr;
    } else if (kw == "END") {
        room_ = nullptr;
        object_ = nullptr;
        rule_ = nullptr;
    } else {
        const bool handled =
            global_keyword(kw, rest) || (room_ && room_keyword(*room_, kw, rest)) ||
            (object_ && object_keyword(*object_, kw, rest)) ||
            (rule_ &&
             (rule_condition_keyword(*rule_, kw, rest) || rule_effect_keyword(*rule_, kw, rest)));
        if (!handled)
            fail("unexpected keyword", kw);
    }
}

World Loader::load()
{
    std::ifstream in(path_);
    if (!in)
        throw LoadError(std::format("{}: cannot open world file", path_.string()));
    std::vector<std::string> lines;
    for (std::string line; std::getline(in, line);) {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(std::move(line));
    }

    w_.start = -1;
    for (const int pass : {1, 2}) {
        room_ = nullptr;
        object_ = nullptr;
        rule_ = nullptr;
        line_ = 0;
        for (const std::string &line : lines) {
            ++line_;
            std::string_view rest = line;
            const std::string_view kw = next_token(rest);
            if (kw.empty() || kw.front() == '#')
                continue;
            if (pass == 1)
                declare(kw, rest);
            else
                define(kw, rest);
        }
    }

    // Resolve rule verbs through synonyms, so "RULE put" == "RULE drop".
    for (Rule &rule : w_.rules) {
        std::string verb(canon(w_, rule.verb));
        rule.verb = std::move(verb);
    }

    if (w_.rooms.empty())
        throw LoadError(std::format("{}: no rooms defined", path_.string()));
    w_.start = std::max(w_.start, 0);
    w_.here = w_.start;
    return std::move(w_);
}

} // namespace

World load_world(const std::filesystem::path &path)
{
    return Loader(path).load();
}

} // namespace adv::detail
