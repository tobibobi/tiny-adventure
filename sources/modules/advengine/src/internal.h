/*
 * internal.h - types and functions shared between the advengine source files.
 * Not part of the public API; only included from inside the module.
 */
#ifndef ADVENGINE_INTERNAL_H
#define ADVENGINE_INTERNAL_H

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace adv::detail
{

inline constexpr int wrap_width = 78; // wrap column for 80-column terminals

inline constexpr std::size_t dir_count = 12;
inline constexpr std::array<std::string_view, dir_count> dir_names = {
    "north",     "south",     "east",      "west",      "up", "down",
    "northeast", "northwest", "southeast", "southwest", "in", "out"};

// Object locations. Values >= 0 are room indices.
inline constexpr int nowhere = -1;
inline constexpr int carried = -2;

// Object references in commands and rules. Values >= 0 are object indices.
inline constexpr int no_object = -1;   // command: no noun given
inline constexpr int any_object = -1;  // rule OBJ/WITH: don't care (also matches no object)
inline constexpr int no_second = -3;   // rule WITH -: no second object allowed
inline constexpr int obj_absent = -4;  // noun known, but no matching object visible
inline constexpr int obj_unknown = -5; // noun not known at all

struct Exit {
    int to = -1;                        // destination room, -1 = no exit
    int flag = -1;                      // required flag, -1 = always open
    std::optional<std::string> blocked; // ELSE message
};

struct Room {
    std::string id, name, desc;
    bool dark = false;
    bool seen = false;
    std::array<Exit, dir_count> exits{};
};

struct Object {
    std::string id, name, desc;
    std::vector<std::string> words;
    int loc = nowhere;
    bool portable = false;
    int light_flag = -1;
};

struct Flag {
    std::string name;
    bool set = false;
};

enum class CondType : std::uint8_t { in, have, here, flag, not_flag };

struct Cond {
    CondType type;
    int arg;
};

enum class EffectType : std::uint8_t { say, set, clear, move, score, go_to, look, win, die };

struct Effect {
    EffectType type;
    int a = 0;
    int b = 0;
    std::string text;
};

struct Rule {
    std::string verb;
    int obj = any_object;
    int with = any_object;
    std::vector<Cond> conds;
    std::vector<Effect> effects;
};

/** One parsed player command. */
struct Command {
    std::string verb;
    int dir = -1;
    int o1 = no_object;
    int o2 = no_object;
    std::string n1, n2; // noun words, for error messages
};

/** The loaded world plus the state of play. */
struct World {
    std::vector<Room> rooms;
    std::vector<Object> objects;
    std::vector<Flag> flags;
    std::vector<Rule> rules;
    std::vector<std::pair<std::string, std::string>> synonyms; // word, canonical word
    std::string title, intro;
    int start = 0;
    int max_score = 0;

    int here = 0;
    int score = 0;
    int moves = 0;
    int dark_turns = 0;
    bool game_over = false;

    [[nodiscard]] Room &room(int i)
    {
        return rooms[static_cast<std::size_t>(i)];
    }
    [[nodiscard]] const Room &room(int i) const
    {
        return rooms[static_cast<std::size_t>(i)];
    }
    [[nodiscard]] Object &object(int i)
    {
        return objects[static_cast<std::size_t>(i)];
    }
    [[nodiscard]] const Object &object(int i) const
    {
        return objects[static_cast<std::size_t>(i)];
    }
    [[nodiscard]] bool flag_set(int i) const
    {
        return flags[static_cast<std::size_t>(i)].set;
    }
    void set_flag(int i, bool value)
    {
        flags[static_cast<std::size_t>(i)].set = value;
    }
};

/** True if the range contains an element equal to the word. */
template <typename Range> bool contains(const Range &range, std::string_view word)
{
    return std::ranges::find(range, word) != std::ranges::end(range);
}

/** Word-wrapped text output. */
class Output
{
  public:
    explicit Output(std::ostream &os) : os_(&os)
    {
    }

    void reset_column()
    {
        col_ = 0;
    }
    /** Print text, wrapping at wrap_width. Runs of spaces collapse to one. */
    void out(std::string_view text);
    /** Print text and end the line. */
    void say(std::string_view text);
    /** Print text as is, without wrapping or moving the column. */
    void raw(std::string_view text);

  private:
    std::ostream *os_;
    int col_ = 0;
};

// world.cpp
World load_world(const std::filesystem::path &path); // throws adv::LoadError
int dir_of(std::string_view word);                   // direction index, -1 if none
std::string_view canon(const World &w, std::string_view word);
bool verb_known(const World &w, std::string_view verb);
bool is_lit(const World &w);
bool can_see(const World &w, int obj);
bool has_word(const Object &obj, std::string_view word);

// parser.cpp
std::optional<Command> parse(const World &w, std::string_view line);
bool fallback_parse(const World &w, std::string_view raw, Command &cmd);

} // namespace adv::detail

#endif // ADVENGINE_INTERNAL_H
