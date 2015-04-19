#ifndef GOAT_H
#define GOAT_H

#include <sys/time.h> /* struct timeval */

typedef struct goat_context GoatContext;
typedef struct goat_message GoatMessage;

typedef void (*GoatCallback)(
    GoatContext       *context,
    int                  connection,
    const GoatMessage *message
);

typedef enum {
    GOAT_EVENT_GENERIC = 0,
    GOAT_EVENT_NUMERIC = 1,

    GOAT_EVENT_LAST /* don't use; keep last */
} GoatEvent;

typedef enum {
    GOAT_E_NONE = 0,        // everything is fine
    GOAT_E_ERRORINV = 1,    // invalid argument passed to goat_error
    GOAT_E_STATE = 2,       // invalid connection state
    GOAT_E_NOMEM = 3,       // couldn't allocate memory
    GOAT_E_INVMSG = 4,      // message is malformed

    GOAT_E_LAST /* don't use; keep last */
} GoatError;

#define GOAT_IRC_FIRST (0) /* must match first */
typedef enum {
    /* these must be in alphabetic (ascii) order according to what appears on the wire */

#define GOAT_IRC_RPL_FIRST (GOAT_IRC_RPL_WELCOME)
    GOAT_IRC_RPL_WELCOME = 0,       /* 001 */
    GOAT_IRC_RPL_YOURHOST,          /* 002 */
    GOAT_IRC_RPL_CREATED,           /* 003 */
    GOAT_IRC_RPL_MYINFO,            /* 004 */
    GOAT_IRC_RPL_BOUNCE,            /* 005 */
    GOAT_IRC_RPL_TRACELINK,         /* 200 */
    GOAT_IRC_RPL_TRACECONNECTING,   /* 201 */
    GOAT_IRC_RPL_TRACEHANDSHAKE,    /* 202 */
    GOAT_IRC_RPL_TRACEUNKNOWN,      /* 203 */
    GOAT_IRC_RPL_TRACEOPERATOR,     /* 204 */
    GOAT_IRC_RPL_TRACEUSER,         /* 205 */
    GOAT_IRC_RPL_TRACESERVER,       /* 206 */
    GOAT_IRC_RPL_TRACESERVICE,      /* 207 */
    GOAT_IRC_RPL_TRACENEWTYPE,      /* 208 */
    GOAT_IRC_RPL_TRACECLASS,        /* 209 */
    GOAT_IRC_RPL_TRACERECONNECT,    /* 210 */
    GOAT_IRC_RPL_STATSLINKINFO,     /* 211 */
    GOAT_IRC_RPL_STATSCOMMANDS,     /* 212 */
    GOAT_IRC_RPL_STATSCLINE,        /* 213 */
    GOAT_IRC_RPL_STATSNLINE,        /* 214 */
    GOAT_IRC_RPL_STATSILINE,        /* 215 */
    GOAT_IRC_RPL_STATSKLINE,        /* 216 */
    GOAT_IRC_RPL_STATSQLINE,        /* 217 */
    GOAT_IRC_RPL_STATSYLINE,        /* 218 */
    GOAT_IRC_RPL_ENDOFSTATS,        /* 219 */
    GOAT_IRC_RPL_UMODEIS,           /* 221 */
    GOAT_IRC_RPL_SERVICEINFO,       /* 231 */
    GOAT_IRC_RPL_ENDOFSERVICES,     /* 232 */
    GOAT_IRC_RPL_SERVICE,           /* 233 */
    GOAT_IRC_RPL_SERVLIST,          /* 234 */
    GOAT_IRC_RPL_SERVLISTEND,       /* 235 */
    GOAT_IRC_RPL_STATSVLINE,        /* 240 */
    GOAT_IRC_RPL_STATSLLINE,        /* 241 */
    GOAT_IRC_RPL_STATSUPTIME,       /* 242 */
    GOAT_IRC_RPL_STATSOLINE,        /* 243 */
    GOAT_IRC_RPL_STATSHLINE,        /* 244 */
    GOAT_IRC_RPL_STATSSLINE,        /* 245 */
    GOAT_IRC_RPL_STATSPING,         /* 246 */
    GOAT_IRC_RPL_STATSBLINE,        /* 247 */
    GOAT_IRC_RPL_STATSDLINE,        /* 250 */
    GOAT_IRC_RPL_LUSERCLIENT,       /* 251 */
    GOAT_IRC_RPL_LUSEROP,           /* 252 */
    GOAT_IRC_RPL_LUSERUNKNOWN,      /* 253 */
    GOAT_IRC_RPL_LUSERCHANNELS,     /* 254 */
    GOAT_IRC_RPL_LUSERME,           /* 255 */
    GOAT_IRC_RPL_ADMINME,           /* 256 */
    GOAT_IRC_RPL_ADMINLOC1,         /* 257 */
    GOAT_IRC_RPL_ADMINLOC2,         /* 258 */
    GOAT_IRC_RPL_ADMINEMAIL,        /* 259 */
    GOAT_IRC_RPL_TRACELOG,          /* 261 */
    GOAT_IRC_RPL_TRACEEND,          /* 262 */
    GOAT_IRC_RPL_TRYAGAIN,          /* 263 */
    GOAT_IRC_RPL_NONE,              /* 300 */
    GOAT_IRC_RPL_AWAY,              /* 301 */
    GOAT_IRC_RPL_USERHOST,          /* 302 */
    GOAT_IRC_RPL_ISON,              /* 303 */
    GOAT_IRC_RPL_UNAWAY,            /* 305 */
    GOAT_IRC_RPL_NOWAWAY,           /* 306 */
    GOAT_IRC_RPL_WHOISUSER,         /* 311 */
    GOAT_IRC_RPL_WHOISSERVER,       /* 312 */
    GOAT_IRC_RPL_WHOISOPERATOR,     /* 313 */
    GOAT_IRC_RPL_WHOWASUSER,        /* 314 */
    GOAT_IRC_RPL_ENDOFWHO,          /* 315 */
    GOAT_IRC_RPL_WHOISCHANOP,       /* 316 */
    GOAT_IRC_RPL_WHOISIDLE,         /* 317 */
    GOAT_IRC_RPL_ENDOFWHOIS,        /* 318 */
    GOAT_IRC_RPL_WHOISCHANNELS,     /* 319 */
    GOAT_IRC_RPL_LISTSTART,         /* 321 */
    GOAT_IRC_RPL_LIST,              /* 322 */
    GOAT_IRC_RPL_LISTEND,           /* 323 */
    GOAT_IRC_RPL_CHANNELMODEIS,     /* 324 */
    GOAT_IRC_RPL_UNIQOPIS,          /* 325 */
    GOAT_IRC_RPL_NOTOPIC,           /* 331 */
    GOAT_IRC_RPL_TOPIC,             /* 332 */
    GOAT_IRC_RPL_INVITING,          /* 341 */
    GOAT_IRC_RPL_SUMMONING,         /* 342 */
    GOAT_IRC_RPL_INVITELIST,        /* 346 */
    GOAT_IRC_RPL_ENDOFINVITELIST,   /* 347 */
    GOAT_IRC_RPL_EXCEPTLIST,        /* 348 */
    GOAT_IRC_RPL_ENDOFEXCEPTLIST,   /* 349 */
    GOAT_IRC_RPL_VERSION,           /* 351 */
    GOAT_IRC_RPL_WHOREPLY,          /* 352 */
    GOAT_IRC_RPL_NAMREPLY,          /* 353 */
    GOAT_IRC_RPL_KILLDONE,          /* 361 */
    GOAT_IRC_RPL_CLOSING,           /* 362 */
    GOAT_IRC_RPL_CLOSEEND,          /* 363 */
    GOAT_IRC_RPL_LINKS,             /* 364 */
    GOAT_IRC_RPL_ENDOFLINKS,        /* 365 */
    GOAT_IRC_RPL_ENDOFNAMES,        /* 366 */
    GOAT_IRC_RPL_BANLIST,           /* 367 */
    GOAT_IRC_RPL_ENDOFBANLIST,      /* 368 */
    GOAT_IRC_RPL_ENDOFWHOWAS,       /* 369 */
    GOAT_IRC_RPL_INFO,              /* 371 */
    GOAT_IRC_RPL_MOTD,              /* 372 */
    GOAT_IRC_RPL_INFOSTART,         /* 373 */
    GOAT_IRC_RPL_ENDOFINFO,         /* 374 */
    GOAT_IRC_RPL_MOTDSTART,         /* 375 */
    GOAT_IRC_RPL_ENDOFMOTD,         /* 376 */
    GOAT_IRC_RPL_YOUREOPER,         /* 381 */
    GOAT_IRC_RPL_REHASHING,         /* 382 */
    GOAT_IRC_RPL_YOURESERVICE,      /* 383 */
    GOAT_IRC_RPL_MYPORTIS,          /* 384 */
    GOAT_IRC_RPL_TIME,              /* 391 */
    GOAT_IRC_RPL_USERSSTART,        /* 392 */
    GOAT_IRC_RPL_USERS,             /* 393 */
    GOAT_IRC_RPL_ENDOFUSERS,        /* 394 */
    GOAT_IRC_RPL_NOUSERS,           /* 395 */
#define GOAT_IRC_RPL_LAST (GOAT_IRC_RPL_NOUSERS + 1)

#define GOAT_IRC_ERR_FIRST (GOAT_IRC_ERR_NOSUCHNICK)
    GOAT_IRC_ERR_NOSUCHNICK,        /* 401 */
    GOAT_IRC_ERR_NOSUCHSERVER,      /* 402 */
    GOAT_IRC_ERR_NOSUCHCHANNEL,     /* 403 */
    GOAT_IRC_ERR_CANNOTSENDTOCHAN,  /* 404 */
    GOAT_IRC_ERR_TOOMANYCHANNELS,   /* 405 */
    GOAT_IRC_ERR_WASNOSUCHNICK,     /* 406 */
    GOAT_IRC_ERR_TOOMANYTARGETS,    /* 407 */
    GOAT_IRC_ERR_NOSUCHSERVICE,     /* 408 */
    GOAT_IRC_ERR_NOORIGIN,          /* 409 */
    GOAT_IRC_ERR_NORECIPIENT,       /* 411 */
    GOAT_IRC_ERR_NOTEXTTOSEND,      /* 412 */
    GOAT_IRC_ERR_NOTOPLEVEL,        /* 413 */
    GOAT_IRC_ERR_WILDTOPLEVEL,      /* 414 */
    GOAT_IRC_ERR_BADMASK,           /* 415 */
    GOAT_IRC_ERR_UNKNOWNCOMMAND,    /* 421 */
    GOAT_IRC_ERR_NOMOTD,            /* 422 */
    GOAT_IRC_ERR_NOADMININFO,       /* 423 */
    GOAT_IRC_ERR_FILEERROR,         /* 424 */
    GOAT_IRC_ERR_NONICKNAMEGIVEN,   /* 431 */
    GOAT_IRC_ERR_ERRONEUSNICKNAME,  /* 432 */
    GOAT_IRC_ERR_NICKNAMEINUSE,     /* 433 */
    GOAT_IRC_ERR_NICKCOLLISION,     /* 436 */
    GOAT_IRC_ERR_UNAVAILRESOURCE,   /* 437 */
    GOAT_IRC_ERR_USERNOTINCHANNEL,  /* 441 */
    GOAT_IRC_ERR_NOTONCHANNEL,      /* 442 */
    GOAT_IRC_ERR_USERONCHANNEL,     /* 443 */
    GOAT_IRC_ERR_NOLOGIN,           /* 444 */
    GOAT_IRC_ERR_SUMMONDISABLED,    /* 445 */
    GOAT_IRC_ERR_USERSDISABLED,     /* 446 */
    GOAT_IRC_ERR_NOTREGISTERED,     /* 451 */
    GOAT_IRC_ERR_NEEDMOREPARAMS,    /* 461 */
    GOAT_IRC_ERR_ALREADYREGISTRED,  /* 462 */
    GOAT_IRC_ERR_NOPERMFORHOST,     /* 463 */
    GOAT_IRC_ERR_PASSWDMISMATCH,    /* 464 */
    GOAT_IRC_ERR_YOUREBANNEDCREEP,  /* 465 */
    GOAT_IRC_ERR_YOUWILLBEBANNED,   /* 466 */
    GOAT_IRC_ERR_KEYSET,            /* 467 */
    GOAT_IRC_ERR_CHANNELISFULL,     /* 471 */
    GOAT_IRC_ERR_UNKNOWNMODE,       /* 472 */
    GOAT_IRC_ERR_INVITEONLYCHAN,    /* 473 */
    GOAT_IRC_ERR_BANNEDFROMCHAN,    /* 474 */
    GOAT_IRC_ERR_BADCHANNELKEY,     /* 475 */
    GOAT_IRC_ERR_BADCHANMASK,       /* 476 */
    GOAT_IRC_ERR_NOCHANMODES,       /* 477 */
    GOAT_IRC_ERR_BANLISTFULL,       /* 478 */
    GOAT_IRC_ERR_NOPRIVILEGES,      /* 481 */
    GOAT_IRC_ERR_CHANOPRIVSNEEDED,  /* 482 */
    GOAT_IRC_ERR_CANTKILLSERVER,    /* 483 */
    GOAT_IRC_ERR_RESTRICTED,        /* 484 */
    GOAT_IRC_ERR_UNIQOPPRIVSNEEDED, /* 485 */
    GOAT_IRC_ERR_NOOPERHOST,        /* 491 */
    GOAT_IRC_ERR_NOSERVICEHOST,     /* 492 */
    GOAT_IRC_ERR_UMODEUNKNOWNFLAG,  /* 501 */
    GOAT_IRC_ERR_USERSDONTMATCH,    /* 502 */
#define GOAT_IRC_ERR_LAST (GOAT_IRC_ERR_USERSDONTMATCH + 1)

#define GOAT_IRC_STR_FIRST (GOAT_IRC_ADMIN)
    GOAT_IRC_ADMIN,
    GOAT_IRC_AWAY,
    GOAT_IRC_CONNECT,
    GOAT_IRC_DIE,
    GOAT_IRC_ERROR,
    GOAT_IRC_INFO,
    GOAT_IRC_INVITE,
    GOAT_IRC_ISON,
    GOAT_IRC_JOIN,
    GOAT_IRC_KICK,
    GOAT_IRC_KILL,
    GOAT_IRC_LINKS,
    GOAT_IRC_LIST,
    GOAT_IRC_LUSERS,
    GOAT_IRC_MODE,  /* this is used for both user and channel modes, based on first param */
    GOAT_IRC_MOTD,
    GOAT_IRC_NAMES,
    GOAT_IRC_NICK,
    GOAT_IRC_NOTICE,
    GOAT_IRC_OPER,
    GOAT_IRC_PART,
    GOAT_IRC_PASS,
    GOAT_IRC_PING,
    GOAT_IRC_PONG,
    GOAT_IRC_PRIVMSG,
    GOAT_IRC_QUIT,
    GOAT_IRC_REHASH,
    GOAT_IRC_RESTART,
    GOAT_IRC_SERVICE,
    GOAT_IRC_SERVLIST,
    GOAT_IRC_SQUERY,
    GOAT_IRC_SQUIT,
    GOAT_IRC_STATS,
    GOAT_IRC_SUMMON,
    GOAT_IRC_TIME,
    GOAT_IRC_TOPIC,
    GOAT_IRC_TRACE,
    GOAT_IRC_USER,
    GOAT_IRC_USERHOST,
    GOAT_IRC_USERS,
    GOAT_IRC_VERSION,
    GOAT_IRC_WALLOPS,
    GOAT_IRC_WHO,
    GOAT_IRC_WHOIS,
    GOAT_IRC_WHOWAS,
#define GOAT_IRC_STR_LAST (GOAT_IRC_WHOWAS + 1)

    GOAT_IRC_LAST /* don't use; keep last */
} GoatCommand;

GoatContext *goat_context_new();
int goat_context_delete(GoatContext *context);

GoatError goat_error(GoatContext *context, int connection);
const char *goat_strerror(GoatError error);
int goat_reset_error(GoatContext *context, int connection);

const char *goat_command_string(GoatCommand command);
int goat_command(const char *command_string, GoatCommand *command);

int goat_connection_new(GoatContext *context);
int goat_connection_delete(GoatContext *context, int connection);

int goat_connect(GoatContext *context, int connection,
    const char *hostname, const char *servname, int ssl);
int goat_disconnect(GoatContext *context, int connection);
int goat_is_connected(GoatContext *connect, int connection);
int goat_get_hostname(GoatContext *connect, int connection, char **hostname);

int goat_send_message(GoatContext *context, int connection, const GoatMessage *message);

int goat_install_callback(GoatContext *context, GoatEvent event, GoatCallback callback);
int goat_uninstall_callback(GoatContext *context, GoatEvent event, GoatCallback callback);

int goat_select_fds(GoatContext *context, fd_set *restrict readfds, fd_set *restrict writefds);
int goat_tick(GoatContext *context, struct timeval *timeout);
int goat_dispatch_events(GoatContext *context);

#define GOAT_MESSAGE_BUF_SZ (1025)

GoatMessage *goat_message_new(const char *prefix, const char *command, const char **params);
GoatMessage *goat_message_new_from_string(const char *str, size_t len);
GoatMessage *goat_message_clone(const GoatMessage *orig);

void goat_message_delete(GoatMessage *message);

char *goat_message_strdup(const GoatMessage *message);
char *goat_message_cstring(const GoatMessage *message, char *buf, size_t *size);

const char *goat_message_get_prefix(const GoatMessage *message);
const char *goat_message_get_command_string(const GoatMessage *message);
const char *goat_message_get_param(const GoatMessage *message, size_t index);
size_t goat_message_get_nparams(const GoatMessage *message);

int goat_message_get_command(const GoatMessage *message, GoatCommand *command);

size_t goat_message_has_tags(const GoatMessage *message);
int goat_message_has_tag(const GoatMessage *message, const char *key);
int goat_message_get_tag_value(const GoatMessage *message, const char *key, char *value, size_t *size);
int goat_message_set_tag(GoatMessage *message, const char *key, const char *value);
int goat_message_unset_tag(GoatMessage *message, const char *key);

#endif
