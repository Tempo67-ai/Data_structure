#ifndef CASE_DOCUMENT_H
#define CASE_DOCUMENT_H

#include <string>
#include <vector>

struct Agent {
    std::string lawyerName;
    std::string firmName;
};

struct CaseDocument {
    int docId = -1;
    std::string caseNo;
    std::string title;
    std::string court;
    std::string documentType;
    std::string caseType;
    std::string procedure;
    std::string judgeDate;
    std::string cause;
    std::string result;
    std::vector<std::string> features;
    std::string content;
    std::vector<Agent> agents;
};

#endif
