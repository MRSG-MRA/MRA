#include "common.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY (msg_test);

size_t get_worker_id (m_host_t worker)
{
    return (size_t) MSG_host_get_data (worker);
}

void send (const char* str, double cpu, double net, void* data, m_host_t dest, int port)
{
    m_task_t  msg = NULL;

    msg = MSG_task_create (str, cpu, net, data);

#ifdef VERBOSE
    if (!message_is (msg, SMS_HEARTBEAT))
	    XBT_INFO ("TX (%d): %s > %s", port, str, MSG_host_get_name (dest));
#endif

    xbt_assert (MSG_task_put (msg, dest, port) == MSG_OK, "ERROR SENDING MESSAGE");
}

void send_sms (const char* str, m_host_t dest, int port)
{
    send (str, 0.0, 0.0, NULL, dest, port);
}

m_task_t receive (int port)
{
    m_task_t  msg = NULL;

    xbt_assert (MSG_task_get (&msg, port) == MSG_OK, "ERROR RECEIVING MESSAGE");

    return msg;
}

int message_is (m_task_t msg, const char* str)
{
    if (strcmp (MSG_task_get_name (msg), str) == 0)
	return 1;

    return 0;
}

int maxval (int a, int b)
{
    if (b > a)
	return b;

    return a;
}

