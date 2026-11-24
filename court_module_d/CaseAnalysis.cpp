#include "CaseAnalysis.h"
#include "CsvLoader.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace {

bool isCivilFirstInstanceJudgment(const CaseDocument& document) {
    return document.caseType == "民事" && document.procedure == "一审" &&
           document.documentType == "判决书";
}

std::vector<std::string> uniqueFeatures(const std::vector<std::string>& features) {
    std::vector<std::string> unique;
    for (const std::string& feature : features) {
        if (!feature.empty() && std::find(unique.begin(), unique.end(), feature) == unique.end()) {
            unique.push_back(feature);
        }
    }
    return unique;
}

}  // namespace

std::vector<SharedCase> findSharedCases(
    const std::vector<CaseDocument>& documents,
    const std::vector<int>& firstLawyerDocIds,
    const std::vector<int>& secondLawyerDocIds) {
    auto firstIds = firstLawyerDocIds;
    auto secondIds = secondLawyerDocIds;
    std::sort(firstIds.begin(), firstIds.end());
    std::sort(secondIds.begin(), secondIds.end());

    std::vector<int> sharedIds;
    std::set_intersection(firstIds.begin(), firstIds.end(),
                          secondIds.begin(), secondIds.end(),
                          std::back_inserter(sharedIds));

    std::vector<std::pair<std::string, int>> caseDocuments;
    for (const CaseDocument& document : documents) {
        if (isCivilFirstInstanceJudgment(document) &&
            std::binary_search(sharedIds.begin(), sharedIds.end(), document.docId)) {
            std::string caseNo = normalizeCaseNo(document.caseNo);
            if (!caseNo.empty()) {
                caseDocuments.emplace_back(std::move(caseNo), document.docId);
            }
        }
    }
    std::sort(caseDocuments.begin(), caseDocuments.end());

    std::vector<SharedCase> cases;
    for (const auto& [caseNo, docId] : caseDocuments) {
        if (cases.empty() || cases.back().caseNo != caseNo) {
            cases.push_back({caseNo, {}});
        }
        cases.back().docIds.push_back(docId);
    }
    return cases;
}

SimilarCaseResult findSimilarCases(const std::vector<CaseDocument>& documents,
                                  int queryDocId) {
    const auto query = std::find_if(documents.begin(), documents.end(),
        [queryDocId](const CaseDocument& document) { return document.docId == queryDocId; });
    if (query == documents.end()) {
        return {SimilarCaseStatus::DocumentNotFound, {}};
    }

    const auto queryFeatures = uniqueFeatures(query->features);
    if (queryFeatures.empty()) {
        return {SimilarCaseStatus::MissingFeatures, {}};
    }
    if (query->cause.empty()) {
        return {SimilarCaseStatus::MissingCause, {}};
    }

    const std::string queryCaseNo = normalizeCaseNo(query->caseNo);
    SimilarCaseResult result{SimilarCaseStatus::Ok, {}};
    for (const CaseDocument& candidate : documents) {
        if (candidate.docId == queryDocId || !isCivilFirstInstanceJudgment(candidate) ||
            candidate.cause != query->cause || candidate.features.empty()) {
            continue;
        }
        const std::string candidateCaseNo = normalizeCaseNo(candidate.caseNo);
        if (candidateCaseNo.empty() || candidateCaseNo == queryCaseNo) {
            continue;
        }
        const auto candidateFeatures = uniqueFeatures(candidate.features);
        if (candidateFeatures.empty()) {
            continue;
        }

        std::vector<std::string> common;
        for (const std::string& feature : queryFeatures) {
            if (std::find(candidateFeatures.begin(), candidateFeatures.end(), feature) !=
                candidateFeatures.end()) {
                common.push_back(feature);
            }
        }
        const auto unionSize = queryFeatures.size() + candidateFeatures.size() - common.size();
        const double score = static_cast<double>(common.size()) / unionSize;
        result.matches.push_back({candidate.docId, score, std::move(common)});
    }

    std::sort(result.matches.begin(), result.matches.end(),
        [](const SimilarCase& left, const SimilarCase& right) {
            if (left.score != right.score) {
                return left.score > right.score;
            }
            return left.docId < right.docId;
        });
    constexpr std::size_t maxMatches = 5;
    if (result.matches.size() > maxMatches) {
        result.matches.resize(maxMatches);
    }
    return result;
}
