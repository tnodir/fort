#ifndef OPTIMIZECONNDBJOB_H
#define OPTIMIZECONNDBJOB_H

#include "statconnbasejob.h"

class OptimizeConnDbJob : public StatConnBaseJob
{
public:
    StatConnJobType jobType() const override { return JobTypeOptimizeDb; }

protected:
    bool processMerge(const StatConnBaseJob & /*statJob*/) override { return true; }
    void processJob() override;
    void emitFinished() override { }
};

#endif // OPTIMIZECONNDBJOB_H
