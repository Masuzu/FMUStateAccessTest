/* ---------------------------------------------------------------------------*
 * fmuTemplate.h
 * Definitions by the includer of this file
 * Copyright QTronic GmbH. All rights reserved.
 * ---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "fmi2Functions.h"

// macros used to define variables
#define  r(vr) comp->r[vr]
#define  i(vr) comp->i[vr]
#define  b(vr) comp->b[vr]
#define  s(vr) comp->s[vr]
#define pos(z) comp->isPositive[z]
#define copy(vr, value) setString(comp, vr, value)

fmi2Status setString(fmi2Component comp, fmi2ValueReference vr, fmi2String value);

// categories of logging supported by model.
// Value is the index in logCategories of a ModelInstance.
#define LOG_ALL       0
#define LOG_ERROR     1
#define LOG_FMI_CALL  2
#define LOG_EVENT     3

#define NUMBER_OF_CATEGORIES 4

typedef enum {
    modelInstantiated       = 1<<0,
    modelInitializationMode = 1<<1,
    modelInitialized        = 1<<2, // state just after fmi2ExitInitializationMode
    modelStepping           = 1<<3, // state after initialization
    modelTerminated         = 1<<4,
    modelError              = 1<<5
} ModelState;

typedef struct {
    fmi2Real    *r;
    fmi2Integer *i;
    fmi2Boolean *b;
    fmi2String  *s;
    fmi2Boolean *isPositive;

    fmi2Real time;
    fmi2String instanceName;
    fmi2Type type;
    fmi2String GUID;
    const fmi2CallbackFunctions *functions;
    fmi2Boolean loggingOn;
    fmi2Boolean logCategories[NUMBER_OF_CATEGORIES];

    fmi2ComponentEnvironment componentEnvironment;
    ModelState state;
    fmi2EventInfo eventInfo;
} ModelInstance;
