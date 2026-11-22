#include "AgentExtractor.h"
#include "CsvLoader.h"
#include <exception>
#include <iostream>

int main(int argc, char* argv[]) {
    std::vector<std::string> paths;
    int firstPath = 1;
    bool a1Only = argc > 1 && std::string(argv[1]) == "--a1";
    if (a1Only) {
        firstPath = 2;
    }
    for (int i = firstPath; i < argc; ++i) {
        paths.push_back(argv[i]);
    }
    if (paths.empty()) {
        std::cerr << "用法: module_a_demo [--a1] 文件1.csv [文件2.csv ...]\n";
        return 1;
    }

    try {
        std::vector<CaseDocument> documents = loadDocuments(paths);
        if (!a1Only) {
            populateAgents(documents);
        }

        std::size_t withFeatures = 0;
        std::size_t civilFirstJudgments = 0;
        std::size_t civilFirstWithFeatures = 0;
        std::size_t civilFirstWithAgents = 0;
        std::size_t withAgents = 0;
        std::size_t pairCount = 0;

        for (const CaseDocument& document : documents) {
            bool civilFirst = document.caseType == "民事" && document.procedure == "一审" &&
                              document.documentType == "判决书";
            if (civilFirst) {
                ++civilFirstJudgments;
            }
            if (!document.features.empty()) {
                ++withFeatures;
                if (civilFirst) {
                    ++civilFirstWithFeatures;
                }
            }
            if (!document.agents.empty()) {
                ++withAgents;
                pairCount += document.agents.size();
                if (civilFirst) {
                    ++civilFirstWithAgents;
                }
            }
        }

        std::cout << "文书数: " << documents.size() << '\n'
                  << "有标签的文书数: " << withFeatures << '\n'
                  << "民事一审判决书数: " << civilFirstJudgments << '\n'
                  << "有标签的民事一审判决书数: " << civilFirstWithFeatures << '\n';
        if (!documents.empty()) {
            const CaseDocument& first = documents[0];
            std::size_t newlines = 0;
            for (char byte : first.content) {
                if (byte == '\n') {
                    ++newlines;
                }
            }
            std::cout << "第一份文书: docId=" << first.docId
                      << "，案号=" << first.caseNo << "，日期=" << first.judgeDate
                      << "，正文换行数=" << newlines << '\n'
                      << "最后一份文书: docId=" << documents.back().docId << '\n';
        }
        if (!a1Only) {
            std::cout << "有识别配对的文书数: " << withAgents << '\n'
                      << "有识别配对的民事一审判决书数: " << civilFirstWithAgents << '\n'
                      << "配对总条数（按文书累计）: " << pairCount << '\n';
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
