#ifndef GOAT_CONNSM_H
#define GOAT_CONNSM_H

#define STATE_ENTER_NAME(name)      _state_##name##_enter
#define STATE_EXECUTE_NAME(name)    _state_##name##_execute
#define STATE_EXIT_NAME(name)       _state_##name##_exit

#define STATE_ENTER(name)   void STATE_ENTER_NAME(name)(goat_connection *conn)
#define STATE_EXECUTE(name) goat_conn_state STATE_EXECUTE_NAME(name)(goat_connection *conn, \
                                int socket_readable, int socket_writeable)
#define STATE_EXIT(name)    void STATE_EXIT_NAME(name)(goat_connection *conn)

#define STATE_DECL(name)    STATE_ENTER(name); STATE_EXECUTE(name); STATE_EXIT(name)

#endif
