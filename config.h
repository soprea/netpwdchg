#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED
#define CONF_STRING_LEN         100

struct conf {
    char ListenIP[CONF_STRING_LEN];
    char ListenPort[CONF_STRING_LEN];
    char group[CONF_STRING_LEN];
    char user[CONF_STRING_LEN];
    char groupThatCanConnect[CONF_STRING_LEN];
} conf;

void init_parameters (struct conf * parms);
char *trim (char * s);
void parse_config (struct conf * parms);
#endif
