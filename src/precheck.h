//
// Created by Anshul Gupta on 4/7/25.
//

#ifndef PRECHECK_H
#define PRECHECK_H

#include "config.h"

/**
 * Precheck the system for required dependencies and configurations.
 * @return 0 if all checks pass, non-zero if any check fails.
 */
int precheck_self(const struct config *cfg);

#endif // PRECHECK_H
