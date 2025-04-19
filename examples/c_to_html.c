#include <stdio.h>
#include <string.h>

#include "ulight/ulight.h"

// See below.
static void on_flush(void*, char* str, size_t length)
{
    fwrite(str, 1, length, stdout);
}

int main(void)
{
    // Initialize a State object.
    // This holds all the information used by the ulight highlighter.
    ulight_state state;
    ulight_init(&state);
    // Provide the source code.
    // ulight doesn't assume that strings are null-terminated anywhere,
    // so the length also needs to be provided.
    state.source = "int x;\n";
    state.source_length = strlen(state.source);
    // Provide a language by enumeration.
    // You can also obtain a language by short name using ulight_get_lang.
    state.lang = ULIGHT_LANG_C;

    // Set up buffers for ulight.
    // Note that ulight has a buffered pipeline:
    //  1. Convert source code to highlight tokens.
    //  2. Convert highlight tokens to HTML.
    //
    // Both parts require a decently-sized buffer.
    ulight_token token_buffer[1024];
    char text_buffer[8192];
    state.token_buffer = token_buffer;
    state.token_buffer_length = sizeof(token_buffer) / sizeof(token_buffer[0]);
    state.text_buffer = text_buffer;
    state.text_buffer_length = sizeof(text_buffer);

    // Provide a callback to ulight which is called when text_buffer is full,
    // and at the end of highlighting.
    // If we wanted, we could also set state.flush_text_data
    // to provide additional extra data that is passed into on_flush.
    state.flush_text = on_flush;

    // Convert the source code to HTML with default settings.
    // If we did everything right (and there is no internal error),
    // we receive ULIGHT_STATUS_OK.
    // Otherwise, get_error_string() may contain a bit of helpful information.
    ulight_status status = ulight_source_to_html(&state);
    if (status != ULIGHT_STATUS_OK) {
        fputs("Error: ", stderr);
        fwrite(state.error, 1, state.error_length, stderr);
    }

    // Since we did everything right in this example, the output is as follows:
    //
    // <h- data-h=kw_type>int</h-> <h- data-h=id>x</h-><h- data-h=sym_punc>;</h->
    ulight_destroy(&state);
}
