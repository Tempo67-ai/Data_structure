#include "AgentExtractor.h"
#include "CsvLoader.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

void check(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void testCsvRecord() {
    std::istringstream input("\xEF\xBB\xBF\"X\",Y\r\n\"a,b\r\n\"\"c\"\"\",\r\nx,y");
    std::vector<std::string> fields;
    check(readCsvRecord(input, fields) && fields == std::vector<std::string>{"X", "Y"},
          "BOM or quoted header");
    check(readCsvRecord(input, fields) && fields == std::vector<std::string>{"a,b\r\n\"c\"", ""},
          "embedded comma, CRLF, escaped quotes and empty field");
    check(readCsvRecord(input, fields) && fields == std::vector<std::string>{"x", "y"},
          "last record has no line break");
    check(!readCsvRecord(input, fields), "EOF returns false");

    std::istringstream malformed("x,\"unterminated");
    try {
        readCsvRecord(malformed, fields);
    } catch (const std::runtime_error&) {
        return;
    }
    throw std::runtime_error("unclosed quote should raise an error");
}

void testAgentRules() {
    const std::string text =
        "　原告共同委托诉讼代理人：林小川、何宁，示例律师事务所专职律师。\n"
        "委托代理人林小川,示例律师事务所律师；委托代理人：林小川，另一律师事务所律师。\n"
        "委托代理人：陈某，示例律师事务所律师。\n"
        "委托代理人：赵一，某律师事务所律师。\n"
        "委托代理人：吴晓，男，曾就职于示例律师事务所。\n"
        "本院核对了委托诉讼代理人：周青，示例律师事务所律师。\n"
        "委托代理人：周青，示例律师事务所实习律师。\n";
    const auto agents = extractAgents(text);
    check(agents.size() == 3, "paired, duplicate and excluded agents");
    check(agents[0].lawyerName == "林小川" && agents[0].firmName == "示例律师事务所",
          "lawyer/firm pair");
    check(agents[1].lawyerName == "何宁" && agents[1].firmName == "示例律师事务所",
          "two names with one explicitly stated firm");
    check(agents[2].lawyerName == "林小川" && agents[2].firmName == "另一律师事务所",
          "same name in a different firm");
}

void testHandoffSample(const std::string& path) {
    auto documents = loadDocuments({path});
    check(documents.size() == 4, "four sample records");
    for (std::size_t i = 0; i < documents.size(); ++i) {
        check(documents[i].docId == static_cast<int>(i), "docId corresponds to index");
        check(documents[i].agents.empty(), "A1 only loads document data");
    }
    check(documents[0].caseNo == documents[1].caseNo, "both documents retained for one case");
    check(normalizeCaseNo("　（2024）津00民初1号 ") == documents[0].caseNo,
          "query input uses the same case number format");
    check(documents[0].documentType == "判决书" && documents[1].documentType == "裁定书",
          "document types are retained");
    check(documents[0].features == std::vector<std::string>{"工资", "加班"} &&
          documents[1].features.empty(), "feature trimming and deduplication");
    check(documents[0].content.find("\"加班\"") != std::string::npos &&
          documents[0].content.find('\n') != std::string::npos, "body preserved");

    populateAgents(documents);
    check(documents[0].agents.size() == 2 && documents[1].agents.empty() &&
          documents[2].agents.size() == 1 && documents[3].agents.size() == 1,
          "A2 populates pairs in each document");
    check(documents[0].agents[0].lawyerName == "林小川" &&
          documents[0].agents[0].firmName == "津海示例律师事务所" &&
          documents[0].agents[1].lawyerName == "何宁" &&
          documents[0].agents[1].firmName == "滨城示例律师事务所", "each pair stays associated");
    populateAgents(documents);
    check(documents[0].agents.size() == 2, "A2 is repeatable");

    const auto combined = loadDocuments({path, path});
    check(combined.size() == 8 && combined[4].docId == 4 && combined[7].docId == 7,
          "document IDs continue when loading another file");
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "请传入联调样例 CSV 路径。\n";
        return 1;
    }
    try {
        testCsvRecord();
        testAgentRules();
        testHandoffSample(argv[1]);
        std::cout << "A1 / A2 联调样例检查通过。\n";
    } catch (const std::exception& error) {
        std::cerr << "检查失败: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
