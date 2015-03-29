#include <CUnit/CUnit.h>
#include <CUnit/Basic.h>

#include "src/goat.h"
#include "src/message.h"

void test_goat_message_new(void) {
    const char *prefix = "prefix";
    const char *command = "command";

    goat_message_t *message = goat_message_new(prefix, command, NULL);

    CU_ASSERT(message != NULL);
    CU_ASSERT_STRING_EQUAL(message->m_prefix, prefix);
    CU_ASSERT_STRING_EQUAL(message->m_command, command);

    // FIXME assert that m_prefix points into m_bytes
    // FIXME assert that m_command points into m_bytes or the big commands array
    // FIXME functionality of params

    goat_message_delete(message);
}
