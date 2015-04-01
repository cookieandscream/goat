#ifndef GOAT_SM_H
#define GOAT_SM_H

#define ST_UNUSED(expr)         do { (void)(expr); } while (0)

#define ST_ENTER_NAME(name)      _state_##name##_enter
#define ST_EXECUTE_NAME(name)    _state_##name##_execute
#define ST_EXIT_NAME(name)       _state_##name##_exit

#define ST_ENTER(name, type, ...)      type ST_ENTER_NAME(name)(__VA_ARGS__)
#define ST_EXECUTE(name, type, ...)    type ST_EXECUTE_NAME(name)(__VA_ARGS__)
#define ST_EXIT(name, type, ...)       type ST_EXIT_NAME(name)(__VA_ARGS__)

#endif
