#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ldapi.h"
#include "ldinternal.h"

static double
milliTimestamp(void)
{
    double t;

    t = time(NULL);
    t *= 1000.0;
    return t;
}

static cJSON *eventarray;
static int numevents;
static int eventscapacity;
static pthread_rwlock_t eventlock = PTHREAD_RWLOCK_INITIALIZER;

static void
initevents(int capacity)
{
    if (!eventarray)
        eventarray = cJSON_CreateArray();
    eventscapacity = capacity;
}

void
LDi_initevents(int capacity)
{
    LDi_wrlock(&eventlock);
    initevents(capacity);
    LDi_unlock(&eventlock);
}

void
LDi_recordidentify(LDUser *lduser)
{
    if (numevents >= eventscapacity) {
        return;
    }

    cJSON *json;

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "identify");
    cJSON_AddStringToObject(json, "key", lduser->key);
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());
    cJSON *juser = LDi_usertojson(lduser);
    cJSON_AddItemToObject(json, "user", juser);

    LDi_wrlock(&eventlock);
    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_unlock(&eventlock);
}

void
LDi_recordfeature(LDUser *lduser, const char *feature, int type, double n, const char *s,
    LDMapNode *m, double defaultn, const char *defaults, LDMapNode *defaultm)
{
    if (numevents >= eventscapacity) {
        return;
    }
    cJSON *json;

    json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "kind", "feature");
    cJSON_AddStringToObject(json, "key", feature);
    if (type == LDNodeNumber) {
        cJSON_AddNumberToObject(json, "value", n);
        cJSON_AddNumberToObject(json, "default", defaultn);
    } else if (type == LDNodeString) {
        cJSON_AddStringToObject(json, "value", s);
        cJSON_AddStringToObject(json, "default", defaults);
    } else if (type == LDNodeBool) {
        cJSON_AddBoolToObject(json, "value", (int)n);
        cJSON_AddBoolToObject(json, "default", (int)defaultn);
    } else if (type == LDNodeMap) {
        cJSON_AddItemToObject(json, "value", LDi_hashtojson(m));
        cJSON_AddItemToObject(json, "default", LDi_hashtojson(defaultm));
    }
    cJSON_AddNumberToObject(json, "creationDate", milliTimestamp());
    cJSON *juser = LDi_usertojson(lduser);
    cJSON_AddItemToObject(json, "user", juser);

    LDi_wrlock(&eventlock);
    cJSON_AddItemToArray(eventarray, json);
    numevents++;
    LDi_unlock(&eventlock);
}

char *
LDi_geteventdata(void)
{
    int hadevents;
    cJSON *events;
    
    LDi_wrlock(&eventlock);
    events = eventarray;
    eventarray = NULL;
    hadevents = numevents;
    numevents = 0;
    initevents(eventscapacity);
    LDi_unlock(&eventlock);

    char *data = NULL;
    if (hadevents) {
        data = cJSON_PrintUnformatted(events);
    }
    cJSON_Delete(events);
    return data;
}
