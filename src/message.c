#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "goat.h"

#include "message.h"

const char *const message_commands[GOAT_IRC_LAST] = {
    [GOAT_IRC_RPL_WELCOME]              = "001",
    [GOAT_IRC_RPL_YOURHOST]             = "002",
    [GOAT_IRC_RPL_CREATED]              = "003",
    [GOAT_IRC_RPL_MYINFO]               = "004",
    [GOAT_IRC_RPL_BOUNCE]               = "005",
    [GOAT_IRC_RPL_TRACELINK]            = "200",
    [GOAT_IRC_RPL_TRACECONNECTING]      = "201",
    [GOAT_IRC_RPL_TRACEHANDSHAKE]       = "202",
    [GOAT_IRC_RPL_TRACEUNKNOWN]         = "203",
    [GOAT_IRC_RPL_TRACEOPERATOR]        = "204",
    [GOAT_IRC_RPL_TRACEUSER]            = "205",
    [GOAT_IRC_RPL_TRACESERVER]          = "206",
    [GOAT_IRC_RPL_TRACESERVICE]         = "207",
    [GOAT_IRC_RPL_TRACENEWTYPE]         = "208",
    [GOAT_IRC_RPL_TRACECLASS]           = "209",
    [GOAT_IRC_RPL_TRACERECONNECT]       = "210",
    [GOAT_IRC_RPL_STATSLINKINFO]        = "211",
    [GOAT_IRC_RPL_STATSCOMMANDS]        = "212",
    [GOAT_IRC_RPL_STATSCLINE]           = "213",
    [GOAT_IRC_RPL_STATSNLINE]           = "214",
    [GOAT_IRC_RPL_STATSILINE]           = "215",
    [GOAT_IRC_RPL_STATSKLINE]           = "216",
    [GOAT_IRC_RPL_STATSQLINE]           = "217",
    [GOAT_IRC_RPL_STATSYLINE]           = "218",
    [GOAT_IRC_RPL_ENDOFSTATS]           = "219",
    [GOAT_IRC_RPL_UMODEIS]              = "221",
    [GOAT_IRC_RPL_SERVICEINFO]          = "231",
    [GOAT_IRC_RPL_ENDOFSERVICES]        = "232",
    [GOAT_IRC_RPL_SERVICE]              = "233",
    [GOAT_IRC_RPL_SERVLIST]             = "234",
    [GOAT_IRC_RPL_SERVLISTEND]          = "235",
    [GOAT_IRC_RPL_STATSVLINE]           = "240",
    [GOAT_IRC_RPL_STATSLLINE]           = "241",
    [GOAT_IRC_RPL_STATSUPTIME]          = "242",
    [GOAT_IRC_RPL_STATSOLINE]           = "243",
    [GOAT_IRC_RPL_STATSSLINE]           = "244",
    [GOAT_IRC_RPL_STATSPING]            = "246",
    [GOAT_IRC_RPL_STATSBLINE]           = "247",
    [GOAT_IRC_RPL_STATSDLINE]           = "250",
    [GOAT_IRC_RPL_LUSERCLIENT]          = "251",
    [GOAT_IRC_RPL_LUSEROP]              = "252",
    [GOAT_IRC_RPL_LUSERUNKNOWN]         = "253",
    [GOAT_IRC_RPL_LUSERCHANNELS]        = "254",
    [GOAT_IRC_RPL_LUSERME]              = "255",
    [GOAT_IRC_RPL_ADMINME]              = "256",
    [GOAT_IRC_RPL_ADMINLOC1]            = "257",
    [GOAT_IRC_RPL_ADMINLOC2]            = "258",
    [GOAT_IRC_RPL_ADMINEMAIL]           = "259",
    [GOAT_IRC_RPL_TRACELOG]             = "261",
    [GOAT_IRC_RPL_TRACEEND]             = "262",
    [GOAT_IRC_RPL_TRYAGAIN]             = "263",
    [GOAT_IRC_RPL_NONE]                 = "300",
    [GOAT_IRC_RPL_AWAY]                 = "301",
    [GOAT_IRC_RPL_USERHOST]             = "302",
    [GOAT_IRC_RPL_ISON]                 = "303",
    [GOAT_IRC_RPL_UNAWAY]               = "305",
    [GOAT_IRC_RPL_NOWAWAY]              = "306",
    [GOAT_IRC_RPL_WHOISUSER]            = "311",
    [GOAT_IRC_RPL_WHOISSERVER]          = "312",
    [GOAT_IRC_RPL_WHOISOPERATOR]        = "313",
    [GOAT_IRC_RPL_WHOWASUSER]           = "314",
    [GOAT_IRC_RPL_ENDOFWHO]             = "315",
    [GOAT_IRC_RPL_WHOISCHANOP]          = "316",
    [GOAT_IRC_RPL_WHOISIDLE]            = "317",
    [GOAT_IRC_RPL_ENDOFWHOIS]           = "318",
    [GOAT_IRC_RPL_WHOISCHANNELS]        = "319",
    [GOAT_IRC_RPL_LISTSTART]            = "321",
    [GOAT_IRC_RPL_LIST]                 = "322",
    [GOAT_IRC_RPL_LISTEND]              = "323",
    [GOAT_IRC_RPL_CHANNELMODEIS]        = "324",
    [GOAT_IRC_RPL_UNIQOPIS]             = "325",
    [GOAT_IRC_RPL_NOTOPIC]              = "331",
    [GOAT_IRC_RPL_TOPIC]                = "332",
    [GOAT_IRC_RPL_INVITING]             = "341",
    [GOAT_IRC_RPL_SUMMONING]            = "342",
    [GOAT_IRC_RPL_INVITELIST]           = "346",
    [GOAT_IRC_RPL_ENDOFINVITELIST]      = "347",
    [GOAT_IRC_RPL_EXCEPTLIST]           = "348",
    [GOAT_IRC_RPL_ENDOFEXCEPTLIST]      = "349",
    [GOAT_IRC_RPL_VERSION]              = "351",
    [GOAT_IRC_RPL_WHOREPLY]             = "352",
    [GOAT_IRC_RPL_NAMREPLY]             = "353",
    [GOAT_IRC_RPL_KILLDONE]             = "361",
    [GOAT_IRC_RPL_CLOSING]              = "362",
    [GOAT_IRC_RPL_CLOSEEND]             = "363",
    [GOAT_IRC_RPL_LINKS]                = "364",
    [GOAT_IRC_RPL_ENDOFLINKS]           = "365",
    [GOAT_IRC_RPL_ENDOFNAMES]           = "366",
    [GOAT_IRC_RPL_BANLIST]              = "367",
    [GOAT_IRC_RPL_ENDOFBANLIST]         = "368",
    [GOAT_IRC_RPL_ENDOFWHOWAS]          = "369",
    [GOAT_IRC_RPL_INFO]                 = "371",
    [GOAT_IRC_RPL_MOTD]                 = "372",
    [GOAT_IRC_RPL_INFOSTART]            = "373",
    [GOAT_IRC_RPL_ENDOFINFO]            = "374",
    [GOAT_IRC_RPL_MOTDSTART]            = "375",
    [GOAT_IRC_RPL_ENDOFMOTD]            = "376",
    [GOAT_IRC_RPL_YOUREOPER]            = "381",
    [GOAT_IRC_RPL_REHASHING]            = "382",
    [GOAT_IRC_RPL_YOURESERVICE]         = "383",
    [GOAT_IRC_RPL_MYPORTIS]             = "384",
    [GOAT_IRC_RPL_TIME]                 = "391",
    [GOAT_IRC_RPL_USERSSTART]           = "392",
    [GOAT_IRC_RPL_USERS]                = "393",
    [GOAT_IRC_RPL_ENDOFUSERS]           = "394",
    [GOAT_IRC_RPL_NOUSERS]              = "395",
    [GOAT_IRC_ERR_NOSUCHNICK]           = "401",
    [GOAT_IRC_ERR_NOSUCHSERVER]         = "402",
    [GOAT_IRC_ERR_NOSUCHCHANNEL]        = "403",
    [GOAT_IRC_ERR_CANNOTSENDTOCHAN]     = "404",
    [GOAT_IRC_ERR_TOOMANYCHANNELS]      = "405",
    [GOAT_IRC_ERR_WASNOSUCHNICK]        = "406",
    [GOAT_IRC_ERR_TOOMANYTARGETS]       = "407",
    [GOAT_IRC_ERR_NOSUCHSERVICE]        = "408",
    [GOAT_IRC_ERR_NOORIGIN]             = "409",
    [GOAT_IRC_ERR_NORECIPIENT]          = "411",
    [GOAT_IRC_ERR_NOTEXTTOSEND]         = "412",
    [GOAT_IRC_ERR_NOTOPLEVEL]           = "413",
    [GOAT_IRC_ERR_WILDTOPLEVEL]         = "414",
    [GOAT_IRC_ERR_BADMASK]              = "415",
    [GOAT_IRC_ERR_UNKNOWNCOMMAND]       = "421",
    [GOAT_IRC_ERR_NOMOTD]               = "422",
    [GOAT_IRC_ERR_NOADMININFO]          = "423",
    [GOAT_IRC_ERR_FILEERROR]            = "424",
    [GOAT_IRC_ERR_NONICKNAMEGIVEN]      = "431",
    [GOAT_IRC_ERR_ERRONEUSNICKNAME]     = "432",
    [GOAT_IRC_ERR_NICKNAMEINUSE]        = "433",
    [GOAT_IRC_ERR_NICKCOLLISION]        = "436",
    [GOAT_IRC_ERR_UNAVAILRESOURCE]      = "437",
    [GOAT_IRC_ERR_USERNOTINCHANNEL]     = "441",
    [GOAT_IRC_ERR_NOTONCHANNEL]         = "442",
    [GOAT_IRC_ERR_USERONCHANNEL]        = "443",
    [GOAT_IRC_ERR_NOLOGIN]              = "444",
    [GOAT_IRC_ERR_SUMMONDISABLED]       = "445",
    [GOAT_IRC_ERR_USERSDISABLED]        = "446",
    [GOAT_IRC_ERR_NOTREGISTERED]        = "451",
    [GOAT_IRC_ERR_NEEDMOREPARAMS]       = "461",
    [GOAT_IRC_ERR_ALREADYREGISTRED]     = "462",
    [GOAT_IRC_ERR_NOPERMFORHOST]        = "463",
    [GOAT_IRC_ERR_PASSWDMISMATCH]       = "464",
    [GOAT_IRC_ERR_YOUREBANNEDCREEP]     = "465",
    [GOAT_IRC_ERR_YOUWILLBEBANNED]      = "466",
    [GOAT_IRC_ERR_KEYSET]               = "467",
    [GOAT_IRC_ERR_CHANNELISFULL]        = "471",
    [GOAT_IRC_ERR_UNKNOWNMODE]          = "472",
    [GOAT_IRC_ERR_INVITEONLYCHAN]       = "473",
    [GOAT_IRC_ERR_BANNEDFROMCHAN]       = "474",
    [GOAT_IRC_ERR_BADCHANNELKEY]        = "475",
    [GOAT_IRC_ERR_BADCHANMASK]          = "476",
    [GOAT_IRC_ERR_NOCHANMODES]          = "477",
    [GOAT_IRC_ERR_BANLISTFULL]          = "478",
    [GOAT_IRC_ERR_NOPRIVILEGES]         = "481",
    [GOAT_IRC_ERR_CHANOPRIVSNEEDED]     = "482",
    [GOAT_IRC_ERR_CANTKILLSERVER]       = "483",
    [GOAT_IRC_ERR_RESTRICTED]           = "484",
    [GOAT_IRC_ERR_UNIQOPPRIVSNEEDED]    = "485",
    [GOAT_IRC_ERR_NOOPERHOST]           = "491",
    [GOAT_IRC_ERR_NOSERVICEHOST]        = "492",
    [GOAT_IRC_ERR_UMODEUNKNOWNFLAG]     = "501",
    [GOAT_IRC_ERR_USERSDONTMATCH]       = "502",

    [GOAT_IRC_ADMIN]    = "ADMIN",
    [GOAT_IRC_AWAY]     = "AWAY",
    [GOAT_IRC_CONNECT]  = "CONNECT",
    [GOAT_IRC_DIE]      = "DIE",
    [GOAT_IRC_ERROR]    = "ERROR",
    [GOAT_IRC_INFO]     = "INFO",
    [GOAT_IRC_INVITE]   = "INVITE",
    [GOAT_IRC_ISON]     = "ISON",
    [GOAT_IRC_JOIN]     = "JOIN",
    [GOAT_IRC_KICK]     = "KICK",
    [GOAT_IRC_KILL]     = "KILL",
    [GOAT_IRC_LINKS]    = "LINKS",
    [GOAT_IRC_LIST]     = "LIST",
    [GOAT_IRC_LUSERS]   = "LUSERS",
    [GOAT_IRC_MODE]     = "MODE",
    [GOAT_IRC_MOTD]     = "MOTD",
    [GOAT_IRC_NAMES]    = "NAMES",
    [GOAT_IRC_NICK]     = "NICK",
    [GOAT_IRC_NOTICE]   = "NOTICE",
    [GOAT_IRC_OPER]     = "OPER",
    [GOAT_IRC_PART]     = "PART",
    [GOAT_IRC_PASS]     = "PASS",
    [GOAT_IRC_PING]     = "PING",
    [GOAT_IRC_PONG]     = "PONG",
    [GOAT_IRC_PRIVMSG]  = "PRIVMSG",
    [GOAT_IRC_QUIT]     = "QUIT",
    [GOAT_IRC_REHASH]   = "REHASH",
    [GOAT_IRC_RESTART]  = "RESTART",
    [GOAT_IRC_SERVICE]  = "SERVICE",
    [GOAT_IRC_SERVLIST] = "SERVLIST",
    [GOAT_IRC_SQUERY]   = "SQUERY",
    [GOAT_IRC_SQUIT]    = "SQUIT",
    [GOAT_IRC_STATS]    = "STATS",
    [GOAT_IRC_SUMMON]   = "SUMMON",
    [GOAT_IRC_TIME]     = "TIME",
    [GOAT_IRC_TOPIC]    = "TOPIC",
    [GOAT_IRC_TRACE]    = "TRACE",
    [GOAT_IRC_USER]     = "USER",
    [GOAT_IRC_USERHOST] = "USERHOST",
    [GOAT_IRC_USERS]    = "USERS",
    [GOAT_IRC_VERSION]  = "VERSION",
    [GOAT_IRC_WALLOPS]  = "WALLOPS",
    [GOAT_IRC_WHO]      = "WHO",
    [GOAT_IRC_WHOIS]    = "WHOIS",
    [GOAT_IRC_WHOWAS]   = "WHOWAS",
};

static int _message_commands_cmp(const void *a, const void *b);

goat_message_t *message_new(const char *prefix, const char *command, const char **params) {
    assert(command != NULL);
    size_t len = 0, n_params = 0;

    if (prefix != NULL)  len += strlen(prefix) + 2;
    len += strlen(command);
    if (params) {
        for (const char **p = params; *p; p++) {
            len += strlen(*p) + 1;
            ++ n_params;
            if (n_params == 15)  break;
            if (strchr(*p, ' ') != NULL)  break;
        }
        len += 1;
    }
    if (len > 510)  return NULL;

    goat_message_t *message = calloc(1, sizeof(goat_message_t) + len + 1);
    if (message == NULL)  return NULL;

    char *position = message->m_bytes;

    if (prefix) {
        *position++ = ':';
        message->m_prefix = position;
        position = stpcpy(position, prefix);
        ++ position;
    }

    message->m_command = position;
    position = stpcpy(position, command);
    message->m_command = (char *) message_static_command(message->m_command);

    if (params) {
        size_t i;
        for (i = 0; i < n_params - 1; i++) {
            ++position;
            message->m_params[i] = position;
            position = stpcpy(position, params[i]);
        }
        ++position;
        *position++ = ':';
        message->m_params[i] = position;
        position = stpcpy(position, params[i]);
    }

    message->m_len = position - message->m_bytes;
    return message;
}

goat_message_t *message_new_from_string(const char *str, size_t len) {
    assert(str != NULL);
    assert(len > 0);
    assert(len == strnlen(str, len + 5));

    // chomp crlf
    if (str[len - 1] == '\x0a') {
        -- len;
        if (str[len - 1] == '\x0d')  -- len;
    }

    goat_message_t *message = calloc(1, sizeof(goat_message_t) + len + 1);
    if (message == NULL)  return NULL;

    message->m_len = len;
    strncpy(message->m_bytes, str, len);

    char *position = message->m_bytes;
    char *token;

    // [ ':' prefix SPACE ]
    if (position[0] == ':') {
        ++ position;
        token = strsep(&position, " ");
        if (token[0] == '\0')  goto cleanup;
        message->m_prefix = token;
    }

    // command
    token = strsep(&position, " ");
    if (token == NULL || token[0] == '\0')  goto cleanup;
    message->m_command = (char *) message_static_command(token);

    // *14( SPACE middle ) [ SPACE ":" trailing ]
    // 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
    unsigned i = 0;
    while (i < 14 && position) {
        if (position[0] == ':')  break;
        token = strsep(&position, " ");
        message->m_params[i] = token;
        ++ i;
    }
    if (position && position[0] == ':')  ++position;
    message->m_params[i] = position;

    return message;

cleanup:
    free(message);
    return NULL;
}

int message_delete(goat_message_t *message) {
    free(message);
    return 0;
}

char *message_strdup(const goat_message_t *message) {
    assert(message != NULL);

    char *str = malloc(message->m_len + 1);
    if (str == NULL)  return NULL;

    memcpy(str, message->m_bytes, message->m_len);
    str[message->m_len] = '\0';

    for (unsigned i = 0; i < message->m_len; i++) {
        if (str[i] == '\0')  str[i] = ' ';
    }

    return str;
}

const char *message_static_command(const char *command) {
    static const size_t width = sizeof(message_commands[0]);
    static const size_t nel = sizeof(message_commands) / sizeof(message_commands[0]);

    const char *ptr = bsearch(command, message_commands, nel, width, _message_commands_cmp);

    if (ptr)  return ptr;
    return command;
}

int _message_commands_cmp(const void *a, const void *b) {
    return strcmp(*(const char **) a, *(const char **)b);
}
