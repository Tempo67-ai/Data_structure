#include "AgentExtractor.h"
#include "CaseAnalysis.h"
#include "CsvLoader.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace {

// Supplies the same docId lists as B2 for the fictional A fixture.
std::vector<int> sampleLawyerDocs(const std::vector<CaseDocument>& documents,
                                  const Agent& lawyer) {
    std::vector<int> ids;
    for (const auto& document : documents) {
        for (const auto& agent : document.agents) {
            if (agent.lawyerName == lawyer.lawyerName && agent.firmName == lawyer.firmName) {
                ids.push_back(document.docId);
                break;
            }
        }
    }
    return ids;
}

void checkSharedCases(const std::vector<CaseDocument>& documents) {
    const auto lin = sampleLawyerDocs(documents, {"林小川", "津海示例律师事务所"});
    const auto he = sampleLawyerDocs(documents, {"何宁", "滨城示例律师事务所"});
    const auto zhou = sampleLawyerDocs(documents, {"周青", "海河示例律师事务所"});

    const auto shared = findSharedCases(documents, lin, he);
    assert(shared.size() == 1);
    assert(shared[0].caseNo == documents[0].caseNo);
    assert((shared[0].docIds == std::vector<int>{0}));
    assert(findSharedCases(documents, lin, zhou).empty());

    auto extended = documents;
    auto duplicateCase = documents[0];
    duplicateCase.docId = 42;
    duplicateCase.caseNo = "（2024）津00民初1号";
    extended.push_back(duplicateCase);
    const auto grouped = findSharedCases(extended, {42, 0, 0, 1, 999}, {1, 0, 42, 0, 999});
    assert(grouped.size() == 1);
    assert((grouped[0].docIds == std::vector<int>{0, 42}));
}

void checkSimilarCases(const std::vector<CaseDocument>& documents) {
    const auto similar = findSimilarCases(documents, 0);
    assert(similar.status == SimilarCaseStatus::Ok);
    assert(similar.matches.size() == 2);
    assert(similar.matches[0].docId == 2);
    assert(std::abs(similar.matches[0].score - 0.5) < 1e-12);
    assert((similar.matches[0].commonFeatures == std::vector<std::string>{"工资"}));
    assert(similar.matches[1].docId == 3);
    assert(std::abs(similar.matches[1].score - 1.0 / 3.0) < 1e-12);
    assert((similar.matches[1].commonFeatures == std::vector<std::string>{"工资"}));
    assert(findSimilarCases(documents, 1).status == SimilarCaseStatus::MissingFeatures);
    assert(findSimilarCases(documents, 999).status == SimilarCaseStatus::DocumentNotFound);
    auto emptyCause = documents;
    emptyCause[0].cause.clear();
    assert(findSimilarCases(emptyCause, 0).status == SimilarCaseStatus::MissingCause);

    std::vector<CaseDocument> extended{documents[0]};
    extended[0].features.push_back("工资");
    auto candidate = documents[2];
    candidate.features = {"工资", "工资"};
    for (int id = 16; id >= 10; --id) {
        candidate.docId = id;
        candidate.caseNo = "样例案号" + std::to_string(id);
        extended.push_back(candidate);
    }

    std::vector<CaseDocument> excluded{extended[0]};
    candidate.docId = 4;
    candidate.caseNo = "（2024）津00民初1号";
    excluded.push_back(candidate);
    candidate.docId = 5;
    candidate.caseNo = "样例案号5";
    candidate.procedure = "二审";
    excluded.push_back(candidate);
    candidate.docId = 6;
    candidate.caseNo = "样例案号6";
    candidate.procedure = "一审";
    candidate.cause = "民间借贷纠纷";
    excluded.push_back(candidate);
    candidate.docId = 7;
    candidate.caseNo = "样例案号7";
    candidate.cause = documents[0].cause;
    candidate.features.clear();
    excluded.push_back(candidate);
    assert(findSimilarCases(excluded, 0).matches.empty());

    candidate.docId = 8;
    candidate.caseNo = "样例案号8";
    candidate.features = {"调解"};
    excluded.push_back(candidate);
    const auto zeroScore = findSimilarCases(excluded, 0);
    assert(zeroScore.matches.size() == 1);
    assert(zeroScore.matches[0].docId == 8);
    assert(zeroScore.matches[0].score == 0.0);
    assert(zeroScore.matches[0].commonFeatures.empty());

    extended.insert(extended.end(), excluded.begin() + 1, excluded.end());
    const auto topFive = findSimilarCases(extended, 0);
    assert(topFive.status == SimilarCaseStatus::Ok);
    assert(topFive.matches.size() == 5);
    for (std::size_t i = 0; i < topFive.matches.size(); ++i) {
        const auto& match = topFive.matches[i];
        assert(match.docId == 10 + static_cast<int>(i));
        assert(std::abs(match.score - 0.5) < 1e-12);
        assert((match.commonFeatures == std::vector<std::string>{"工资"}));
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "用法：module_d_tests <联调样例 CSV>\n";
        return 2;
    }

    auto documents = loadDocuments({argv[1]});
    assert(documents.size() == 4);
    populateAgents(documents);
    checkSharedCases(documents);
    checkSimilarCases(documents);
    std::cout << "D1：共同参与案件、空结果和按案号汇总验证通过。\n"
              << "D2：相似度 1/2、1/3、筛选条件和前五条排序验证通过。\n";
}
