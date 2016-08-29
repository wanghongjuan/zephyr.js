#ifndef ZJS_LINUX_BUILD
#include <zephyr.h>
#endif
#include <string.h>

#include "zjs_util.h"
#include "zjs_callbacks.h"

#include "jerry-api.h"

#define INITIAL_CALLBACK_SIZE  16
#define CB_CHUNK_SIZE          16

#define CALLBACK_TYPE_JS    0
#define CALLBACK_TYPE_C     1

struct zjs_callback_t {
    int32_t id;
    void* handle;
    zjs_pre_callback_func pre;
    zjs_post_callback_func post;
    jerry_value_t js_func;
};

struct zjs_c_callback_t {
    int32_t id;
    void* handle;
    zjs_c_callback_func function;
};

struct zjs_callback_map {
    uint8_t type;
    uint8_t signal;
    union {
        struct zjs_callback_t* js;
        struct zjs_c_callback_t* c;
    };
};

static int32_t cb_limit = INITIAL_CALLBACK_SIZE;
static int32_t cb_size = 0;
static struct zjs_callback_map** cb_map = NULL;

static int32_t new_id(void)
{
    int32_t id = 0;
    if (cb_size >= cb_limit) {
        cb_limit += CB_CHUNK_SIZE;
        size_t size = sizeof(struct zjs_callback_map *) * cb_limit;
        struct zjs_callback_map** new_map = zjs_malloc(size);
        if (!new_map) {
            DBG_PRINT("[callbacks] new_id(): Error allocating space for new callback map\n");
            return -1;
        }
        DBG_PRINT("[callbacks] new_id(): Callback list size too small, increasing by %d\n",
                CB_CHUNK_SIZE);
        memset(new_map, 0, size);
        memcpy(new_map, cb_map, sizeof(struct zjs_callback_map *) * cb_size);
        zjs_free(cb_map);
        cb_map = new_map;
    }
    while (cb_map[id] != NULL) {
        id++;
    }
    return id;
}

void zjs_init_callbacks(void)
{
    if (!cb_map) {
        size_t size = sizeof(struct zjs_callback_map *) *
            INITIAL_CALLBACK_SIZE;
        cb_map = (struct zjs_callback_map**)zjs_malloc(size);
        if (!cb_map) {
            DBG_PRINT("[callbacks] zjs_init_callbacks(): Error allocating space for CB map\n");
            return;
        }
        memset(cb_map, 0, size);
    }
    return;
}

void zjs_edit_js_func(int32_t id, jerry_value_t func)
{
    if (id != -1) {
        jerry_release_value(cb_map[id]->js->js_func);
        cb_map[id]->js->js_func = jerry_acquire_value(func);
    }
}

int32_t zjs_add_callback(jerry_value_t js_func,
                         void* handle,
                         zjs_pre_callback_func pre,
                         zjs_post_callback_func post)
{
    struct zjs_callback_map* new_cb = zjs_malloc(sizeof(struct zjs_callback_map));
    if (!new_cb) {
        DBG_PRINT("[callbacks] zjs_add_callback(): Error allocating space for new callback\n");
        return -1;
    }
    new_cb->js = zjs_malloc(sizeof(struct zjs_callback_t));
    if (!new_cb->js) {
        DBG_PRINT("[callbacks] zjs_add_callback(): Error allocating space for new callback\n");
        return -1;
    }
    new_cb->type = CALLBACK_TYPE_JS;
    new_cb->signal = 0;
    new_cb->js->id = new_id();
    new_cb->js->js_func = jerry_acquire_value(js_func);
    new_cb->js->pre = pre;
    new_cb->js->post = post;
    new_cb->js->handle = handle;

    // Add callback to list
    cb_map[new_cb->js->id] = new_cb;
    cb_size++;

    DBG_PRINT("[callbacks] zjs_add_callback(): Adding new callback id %u\n", new_cb->js->id);

    return new_cb->js->id;
}

void zjs_remove_callback(int32_t id)
{
    if (id != -1 && cb_map[id]) {
        if (cb_map[id]->type == CALLBACK_TYPE_JS && cb_map[id]->js) {
            jerry_release_value(cb_map[id]->js->js_func);
            zjs_free(cb_map[id]->js);
        } else if (cb_map[id]->c) {
            zjs_free(cb_map[id]->c);
        }
        zjs_free(cb_map[id]);
        cb_map[id] = NULL;
        cb_size--;
        DBG_PRINT("[callbacks] zjs_remove_callback(): Removing callback id %u\n", id);
    }
}

void zjs_signal_callback(int32_t id)
{
    if (id != -1 && cb_map[id]) {
#ifdef DEBUG_BUILD
        if (cb_map[id]->type == CALLBACK_TYPE_JS) {
            DBG_PRINT("[callbacks] zjs_signal_callback(): Signaling JS callback id %u\n", id);
        } else {
            DBG_PRINT("[callbacks] zjs_signal_callback(): Signaling C callback id %u\n", id);
        }
#endif
        cb_map[id]->signal = 1;
    }
}

int32_t zjs_add_c_callback(void* handle, zjs_c_callback_func callback)
{
    struct zjs_callback_map* new_cb = zjs_malloc(sizeof(struct zjs_callback_map));
    if (!new_cb) {
        DBG_PRINT("[callbacks] zjs_add_c_callback(): Error allocating space for new callback\n");
        return -1;
    }
    new_cb->c = zjs_malloc(sizeof(struct zjs_c_callback_t));
    if (!new_cb->c) {
        DBG_PRINT("[callbacks] zjs_add_c_callback(): Error allocating space for new callback\n");
        return -1;
    }
    new_cb->type = CALLBACK_TYPE_C;
    new_cb->signal = 0;
    new_cb->c->id = new_id();
    new_cb->c->function = callback;
    new_cb->c->handle = handle;

    // Add callback to list
    cb_map[new_cb->c->id] = new_cb;
    cb_size++;

    DBG_PRINT("[callbacks] zjs_add_callback(): Adding new C callback id %u\n", new_cb->c->id);

    return new_cb->c->id;
}


void zjs_service_callbacks(void)
{
    int i;
    for (i = 0; i < cb_size; i++) {
        if (cb_map[i]->signal) {
            cb_map[i]->signal = 0;
            if (cb_map[i]->type == CALLBACK_TYPE_JS && jerry_value_is_function(cb_map[i]->js->js_func)) {
                uint32_t args_cnt = 0;
                jerry_value_t ret_val;
                jerry_value_t* args = NULL;

                if (cb_map[i]->js->pre) {
                    args = cb_map[i]->js->pre(cb_map[i]->js->handle, &args_cnt);
                }

                DBG_PRINT("[callbacks] zjs_service_callbacks(): Calling callback id %u with %u args\n", cb_map[i]->js->id, args_cnt);
                // TODO: Use 'this' in callback module
                jerry_call_function(cb_map[i]->js->js_func, ZJS_UNDEFINED, args, args_cnt);
                if (cb_map[i]->js->post) {
                    cb_map[i]->js->post(cb_map[i]->js->handle, &ret_val);
                }
            } else if (cb_map[i]->type == CALLBACK_TYPE_C && cb_map[i]->c->function) {
                DBG_PRINT("[callbacks] zjs_service_callbacks(): Calling callback id %u\n", cb_map[i]->c->id);
                cb_map[i]->c->function(cb_map[i]->c->handle);
            }
        }
    }
}
