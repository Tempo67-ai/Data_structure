#ifndef COURT_CASE_ANALYSIS_H
#define COURT_CASE_ANALYSIS_H

#include "CaseDocument.h"
#include <string>
#include <vector>

struct SharedCase {
    std::string caseNo;
    std::vector<int> docIds;
};

// The two ID lists come from B2, one per (lawyerName, firmName) pair.
// Returned cases are ordered by case number and their IDs by docId.
std::vector<SharedCase> findSharedCases(
    const std::vector<CaseDocument>& documents,
    const std::vector<int>& firstLawyerDocIds,
    const std::vector<int>& secondLawyerDocIds);

enum class SimilarCaseStatus {
    Ok,
    DocumentNotFound,
    MissingFeatures,
    MissingCause
};

struct SimilarCase {
    int docId;
    double score;
    std::vector<std::string> commonFeatures;
};

struct SimilarCaseResult {
    SimilarCaseStatus status;
    std::vector<SimilarCase> matches;
};

// Returns up to five matches, sorted by descending Jaccard score.
// Ties are ordered by ascending docId.
SimilarCaseResult findSimilarCases(const std::vector<CaseDocument>& documents,
                                  int queryDocId);

#endif
