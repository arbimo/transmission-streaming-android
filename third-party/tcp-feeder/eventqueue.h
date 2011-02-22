#include <pthread.h>
/** Event queue library for inter-thread or inter process
 * communications. The library stores added messages in a thread safe
 * queue and when there are enough messages to be read wakes up a
 * server thread, which writes the messages to a user specified
 * function. The server thread can be created either by the
 * application or the library.
 *
 */

/** 
 * eventqueue_init - initializes event queue
 * @fptr: function which will be called when messages can be popped out of the queue
 * @msg_burst_len: threshold of msgs in queue for @fptr to be called and msgs popped out
 *
 * eventqueue_init should be called when a separate background thread
 * exists for reading messages from the thread and handling them.
 */
void eventqueue_init(int (*fptr)(char *msg, int msglen), int msg_burst_len);

/** 
 * eventqueue_run - starts the reading/popping part of the eventqueue
 *
 * Should be called after eventqueue_init. Function doesn't return.
 */
void eventqueue_run();

/** 
 * eventqueue_init - initializes event queue and starts a background thread for it
 * @fptr: function which will be called when messages can be popped out of the queue
 * @msg_burst_len: threshold of msgs in queue for @fptr to be called and msgs popped out
 *
 * Returns a pointer to the server thread which will write msgs to
 * @fptr. The thread should be cancelled after calling eventqueue_shutdown().
 * 
 */
pthread_t *eventqueue_threaded_init(int (*fptr)(char *msg, int msglen), int msg_burst_len);
/** 
 *   eventqueue_shutdown - cleans up memory of queue 
 */
void eventqueue_shutdown();
/** Add new msg at the tail of the queue
 * @msg: message which will be copied into the queue
 * @len: length of the message
 *
 * Can be called after eventqueue_init_threaded or eventqueue_init and before eventqueue_shutdown()
 */
int eventqueue_add(char *msg, int len);

/**
 * Enable writing of messages to @fptr This function should be called
 * for threaded init.  In non threaded version calling
 * eventqueue_run() will enable writing.
 */
void eventqueue_can_pop();

