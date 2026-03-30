#include <stdio.h>

#include <uv.h>

uv_loop_t *loop;
uv_timer_t gc_req;
uv_timer_t fake_job_req;

void gc(uv_timer_t *handle) {
    fprintf(stderr, "Freeing unused objects\n");
}

void fake_job(uv_timer_t *handle) {
    fprintf(stdout, "Fake job done\n");
}

/* 
 * Event loop Reference Count
 * The event loop only runs as long as there are active handles. This system works by having every handle increase the reference count of the event loop when it is started and decreasing the reference count when stopped. It is also possible to manually change the reference count of handles using:
 *
 * void uv_ref(uv_handle_t*);
 * void uv_unref(uv_handle_t*);
 * These functions can be used to allow a loop to exit even when a watcher is active or to use custom objects to keep the loop alive.
 *
 * The latter can be used with interval timers. You might have a garbage collector which runs every X seconds, or your network service might send a heartbeat to others periodically, but you don’t want to have to stop them along all clean exit paths or error scenarios. Or you want the program to exit when all your other watchers are done. In that case just unref the timer immediately after creation so that if it is the only watcher running then uv_run will still exit.
 */

int main() {
    loop = uv_default_loop();

    uv_timer_init(loop, &gc_req);
    uv_unref((uv_handle_t*) &gc_req);

    uv_timer_start(&gc_req, gc, 0, 2000);

    // could actually be a TCP download or something
    uv_timer_init(loop, &fake_job_req);
    uv_timer_start(&fake_job_req, fake_job, 9000, 0);
    return uv_run(loop, UV_RUN_DEFAULT);
}
