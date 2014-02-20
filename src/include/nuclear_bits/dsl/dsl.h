/*
 * Copyright (C) 2013 Trent Houliston <trent@houliston.me>, Jake Woods <jake.f.woods@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NUCLEAR_DSL_DSL_H
#define NUCLEAR_DSL_DSL_H

#include "Trigger.h"
#include "With.h"
#include "Options.h"
#include "Optional.h"
#include "Startup.h"
#include "Shutdown.h"
#include "Single.h"
#include "Sync.h"
#include "Scope.h"
#include "CommandLineArguments.h"
#include "Every.h"
#include "Last.h"
#include "Network.h"
#include "Priority.h"
#include "Raw.h"

/**
 * @defgroup Wrappers
 *
 * @brief Wrappers are the base level types in an on statement. They encapsulate other options and flag their usage.
 *
 * @details
 *  The Wrappers are used in an on statement to catagorize the types they contain. For instance, the Trigger
 *  wrapper signifies that the types which are contained within it are used to trigger the running of the
 *  statement.
 */

/**
 * @defgroup Options
 *
 * @brief Options are used within an Options wrapper to specify how a task will be run.
 *
 * @details
 *  Options are used on a task to specify how the task will be executed. They affect when and if the task
 *  will be scheduled to run.
 */

/**
 * @defgroup SmartTypes
 *
 * @brief Smart types are types which when used within a Trigger or With statement, return different results.
 *
 * @details
 *  When a smart type is put within a trigger or with statement, the return type, data returned, and what it
 *  will trigger on can be changed. This can be used to access special features of the API and to make tasks
 *  which may be more difficult to accomplish easier.
 */

#endif
