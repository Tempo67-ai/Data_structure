#ifndef AGENT_EXTRACTOR_H
#define AGENT_EXTRACTOR_H

#include "CaseDocument.h"

// Returns distinct lawyer/firms pairs in order of first appearance.
std::vector<Agent> extractAgents(const std::string& content);

// Replaces each agents list with the pairs extracted from that document.
void populateAgents(std::vector<CaseDocument>& documents);

#endif
