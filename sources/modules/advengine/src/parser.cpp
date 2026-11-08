/*
 * parser.cpp - turns a line of player input into a Command.
 *
 * Classic two-word-plus parser: VERB [noun phrase] [preposition noun phrase].
 * Anything it cannot make sense of is offered to fallback_parse().
 */
#include "internal.h"

#include <cctype>

namespace adv::detail
{

namespace
{

constexpr std::array<std::string_view, 8> noise_words = {"the", "a",  "an", "some",
                                                         "at",  "to", "up", "of"};
constexpr std::array<std::string_view, 10> prepositions = {
    "with", "using", "on", "onto", "in", "into", "across", "over", "under", "from"};

bool is_word_char(char ch)
{
    return std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '?';
}

/* Split into lower-case words. */
std::vector<std::string> tokenize(std::string_view line)
{
    std::vector<std::string> words;
    std::string word;
    for (const char ch : line) {
        if (is_word_char(ch)) {
            word += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        } else if (!word.empty()) {
            words.push_back(std::move(word));
            word.clear();
        }
    }
    if (!word.empty())
        words.push_back(std::move(word));
    return words;
}

/*
 * Find the object a noun phrase refers to ("brass lamp" -> lamp).
 * Returns an object index, no_object for an empty phrase, obj_absent if the
 * noun is known but not visible, or obj_unknown. The offending word goes to
 * word.
 */
int resolve(const World &w, const std::vector<std::string_view> &phrase, std::string &word)
{
    if (phrase.empty())
        return no_object;
    std::string_view absent;
    std::string_view unknown;
    for (const std::string_view noun : phrase) {
        bool known = false;
        for (std::size_t o = 0; o < w.objects.size(); ++o) {
            if (!has_word(w.objects[o], noun))
                continue;
            known = true;
            if (can_see(w, static_cast<int>(o)))
                return static_cast<int>(o);
        }
        if (known && absent.empty())
            absent = noun;
        if (!known && unknown.empty())
            unknown = noun;
    }
    if (!absent.empty()) {
        word = absent;
        return obj_absent;
    }
    word = unknown;
    return obj_unknown;
}

/* Fill in cmd.verb (and cmd.dir for movement). Returns the index of the
 * first word after the verb, or nothing if the command is complete. */
std::optional<std::size_t> parse_verb(const World &w, const std::vector<std::string> &words,
                                      Command &cmd)
{
    // "turn on lamp" / "switch off lamp"
    if (words.size() >= 2 && (words[0] == "turn" || words[0] == "switch")) {
        if (words[1] == "on") {
            cmd.verb = "light";
            return 2;
        }
        if (words[1] == "off") {
            cmd.verb = "extinguish";
            return 2;
        }
    }

    const std::string_view verb = canon(w, words[0]);
    if (const int d = dir_of(verb); d >= 0) {
        cmd.verb = "go";
        cmd.dir = d;
        return std::nullopt;
    }
    cmd.verb = verb;
    if (verb == "go") {
        if (words.size() > 1)
            cmd.dir = dir_of(canon(w, words[1]));
        return std::nullopt;
    }
    return 1;
}

} // namespace

std::optional<Command> parse(const World &w, std::string_view line)
{
    const std::vector<std::string> words = tokenize(line);
    if (words.empty())
        return std::nullopt;

    Command cmd;
    const std::optional<std::size_t> rest = parse_verb(w, words, cmd);
    if (!rest)
        return cmd;

    // Split the rest into "noun phrase [preposition noun phrase]".
    std::array<std::vector<std::string_view>, 2> phrases;
    std::size_t group = 0;
    for (std::size_t i = *rest; i < words.size(); ++i) {
        const std::string_view word = words[i];
        if (contains(prepositions, word)) {
            if (!phrases[0].empty())
                group = 1;
            continue;
        }
        if (contains(noise_words, word))
            continue;
        phrases.at(group).push_back(word);
    }
    if (cmd.verb == "look" && !phrases[0].empty())
        cmd.verb = "examine";

    cmd.o1 = resolve(w, phrases[0], cmd.n1);
    cmd.o2 = resolve(w, phrases[1], cmd.n2);
    return cmd;
}

/*
 * Fallback for input the classic parser can't handle (unknown verb or noun).
 *
 * This is where an embedding matcher or a small local language model plugs
 * in: given the raw line and the current room / visible objects, rewrite cmd
 * into a valid verb + objects and return true. Returning false leaves cmd
 * untouched and the player gets the classic "I don't understand" style reply.
 */
bool fallback_parse(const World & /*w*/, std::string_view /*raw*/, Command & /*cmd*/)
{
    return false;
}

} // namespace adv::detail
