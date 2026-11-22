#include "AgentExtractor.h"
#include <sstream>

namespace {

std::string trim(const std::string& value) {
    const std::string fullWidthSpace = "　";
    std::size_t begin = 0;
    std::size_t end = value.size();
    while (begin < end) {
        if (value[begin] == ' ' || value[begin] == '\t') {
            ++begin;
        } else if (value.compare(begin, fullWidthSpace.size(), fullWidthSpace) == 0) {
            begin += fullWidthSpace.size();
        } else {
            break;
        }
    }
    while (end > begin) {
        if (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r') {
            --end;
        } else if (end - begin >= fullWidthSpace.size() &&
                   value.compare(end - fullWidthSpace.size(), fullWidthSpace.size(),
                                 fullWidthSpace) == 0) {
            end -= fullWidthSpace.size();
        } else {
            break;
        }
    }
    return value.substr(begin, end - begin);
}

std::size_t firstSeparator(const std::string& text, std::size_t start,
                           const std::vector<std::string>& separators) {
    std::size_t first = text.size();
    for (const std::string& mark : separators) {
        std::size_t pos = text.find(mark, start);
        if (pos != std::string::npos && pos < first) {
            first = pos;
        }
    }
    return first;
}

// The role may be preceded by one or more conventional party labels.
bool isPartyPrefix(const std::string& prefix) {
    static const std::vector<std::string> parts = {
        "被上诉人", "再审申请人", "被申请人", "被执行人", "被告人",
        "申请执行人", "上诉人", "申请人", "第三人", "原告", "被告",
        "原审", "上述", "共同", "两", "二", "三", "四", "各", "的", "、", "人", " ", "　"
    };
    std::size_t pos = 0;
    while (pos < prefix.size()) {
        bool matched = false;
        for (const std::string& part : parts) {
            if (prefix.compare(pos, part.size(), part) == 0) {
                pos += part.size();
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    return true;
}

bool looksMasked(const std::string& value) {
    static const std::vector<std::string> masks = {"某", "×", "*", "＊", "XX", "xx", "Ｘ", "ｘ", "匿名"};
    for (const std::string& mask : masks) {
        if (value.find(mask) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Recognize common Chinese names, including a middle dot within a name.
bool isClearName(const std::string& name) {
    if (looksMasked(name)) {
        return false;
    }
    std::size_t pos = 0;
    int chineseCharacters = 0;
    bool lastWasDot = false;
    while (pos < name.size()) {
        if (name.compare(pos, 2, "·") == 0) {
            if (pos == 0 || lastWasDot) {
                return false;
            }
            pos += 2;
            lastWasDot = true;
            continue;
        }
        if (pos + 3 > name.size()) {
            return false;
        }
        const unsigned int a = static_cast<unsigned char>(name[pos]);
        const unsigned int b = static_cast<unsigned char>(name[pos + 1]);
        const unsigned int c = static_cast<unsigned char>(name[pos + 2]);
        if ((a & 0xF0) != 0xE0 || (b & 0xC0) != 0x80 || (c & 0xC0) != 0x80) {
            return false;
        }
        const unsigned int code = ((a & 0x0F) << 12) | ((b & 0x3F) << 6) | (c & 0x3F);
        if (code < 0x3400 || code > 0x9FFF) {
            return false;
        }
        pos += 3;
        ++chineseCharacters;
        lastWasDot = false;
    }
    return chineseCharacters >= 2 && chineseCharacters <= 20 && !lastWasDot;
}

void appendUnique(std::vector<Agent>& agents, const std::string& name,
                  const std::string& firm) {
    for (const Agent& agent : agents) {
        if (agent.lawyerName == name && agent.firmName == firm) {
            return;
        }
    }
    agents.push_back({name, firm});
}

void parseAgentClause(const std::string& clause, std::vector<Agent>& agents) {
    static const std::vector<std::string> roles = {
        "委托诉讼代理人", "委托代理人", "诉讼代理人", "指定辩护人", "辩护人"
    };
    static const std::vector<std::string> commas = {"，", ","};
    static const std::vector<std::string> fieldEnds = {"。", "，", ","};
    const std::string text = trim(clause);

    std::size_t afterRole = 0;
    for (const std::string& role : roles) {
        const std::size_t pos = text.find(role);
        if (pos != std::string::npos && isPartyPrefix(text.substr(0, pos))) {
            afterRole = pos + role.size();
            break;
        }
    }
    if (afterRole == 0) {
        return;
    }

    std::string rest = trim(text.substr(afterRole));
    if (rest.compare(0, 3, "：") == 0) {
        rest = trim(rest.substr(3));
    } else if (!rest.empty() && rest[0] == ':') {
        rest = trim(rest.substr(1));
    }
    const std::size_t comma = firstSeparator(rest, 0, commas);
    if (comma == rest.size()) {
        return;
    }

    const std::string names = trim(rest.substr(0, comma));
    const std::size_t firmStart = comma + (rest[comma] == ',' ? 1 : 3);
    const std::size_t firmEnd = firstSeparator(rest, firmStart, fieldEnds);
    const std::string firmAndRole = trim(rest.substr(firmStart, firmEnd - firmStart));
    const std::string firmSuffix = "律师事务所";
    const std::size_t suffixPos = firmAndRole.find(firmSuffix);
    if (suffixPos == std::string::npos || suffixPos == 0) {
        return;
    }

    const std::size_t endOfFirm = suffixPos + firmSuffix.size();
    const std::string job = trim(firmAndRole.substr(endOfFirm));
    if (job != "律师" && job != "专职律师" && job != "执业律师" && job != "兼职律师") {
        return;
    }
    const std::string firm = firmAndRole.substr(0, endOfFirm);
    if (looksMasked(firm)) {
        return;
    }

    const std::string separator = "、";
    std::size_t start = 0;
    while (start < names.size()) {
        std::size_t end = names.find(separator, start);
        if (end == std::string::npos) {
            end = names.size();
        }
        const std::string name = trim(names.substr(start, end - start));
        if (isClearName(name)) {
            appendUnique(agents, name, firm);
        }
        start = end + separator.size();
    }
}

}  // namespace

std::vector<Agent> extractAgents(const std::string& content) {
    std::vector<Agent> agents;
    std::istringstream input(content);
    std::string line;
    static const std::vector<std::string> clauseEnds = {"；", ";"};

    while (std::getline(input, line)) {
        // Ignore prose lines that have no law firm, including most of the judgment text.
        if (line.find("律师事务所") == std::string::npos) {
            continue;
        }

        std::size_t start = 0;
        while (start < line.size()) {
            const std::size_t end = firstSeparator(line, start, clauseEnds);
            parseAgentClause(line.substr(start, end - start), agents);
            if (end == line.size()) {
                break;
            }
            start = end + (line[end] == ';' ? 1 : 3);
        }
    }
    return agents;
}

void populateAgents(std::vector<CaseDocument>& documents) {
    for (CaseDocument& document : documents) {
        document.agents = extractAgents(document.content);
    }
}
