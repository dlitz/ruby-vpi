/*
  Copyright 2008 Suraj N. Kurapati
  See the file named LICENSE for details.
*/

#include "host.h"
#include "util.h"
#include "user.h"
#include "binding.h"
#include <ruby.h>
#include <stdlib.h>

VALUE RubyVPI_host_gProgName;

#ifdef RUBY_GLOBAL_SETUP
RUBY_GLOBAL_SETUP
#endif

PLI_INT32 RubyVPI_host_init(p_cb_data aCallback)
{
/*
    // ruby thinks it's running inside an entire process, so it uses
    // getrlimit() to determine maximum stack size.  we fix this by
    // setting ruby's maximum stack size to that of this pthread
    RubyVPI_util_debug("Host: alloc stack");

    RubyVPI_host_gStack = 0;
    RubyVPI_host_gStackSize = 0;

    unsigned char power;
    for (power = 22; power > 0; power--) // start at 2**22 (41 MiB)
    {
        RubyVPI_host_gStackSize = 1 << power;
        RubyVPI_host_gStack = malloc(RubyVPI_host_gStackSize);

        if (RubyVPI_host_gStack)
            break;
    }

    if (!RubyVPI_host_gStack)
    {
        RubyVPI_util_error("unable to allocate memory for Ruby's stack");
    }

    RubyVPI_util_debug("Host: stack is %p (%d bytes)", RubyVPI_host_gStack, RubyVPI_host_gStackSize);
    // ruby_init_stack(RubyVPI_host_gStack);
    // ruby_set_stack_size(RubyVPI_host_gStackSize);
*/


    //
    // ruby init
    //

    #ifdef RUBY_INIT_STACK
    RubyVPI_util_debug("Host: RUBY_INIT_STACK");
    RUBY_INIT_STACK;
    #endif

    RubyVPI_util_debug("Host: ruby_init()");
    ruby_init();

    // override Ruby's hooked handlers for $0 so that $0 can be
    // treated as pure Ruby value (and modified without restriction)
    RubyVPI_util_debug("Host: redefine $0 hooked variable");
    RubyVPI_host_gProgName = rb_str_new2("ruby-vpi");
    rb_define_variable("$0", &RubyVPI_host_gProgName);
    rb_define_variable("$PROGRAM_NAME", &RubyVPI_host_gProgName);

    RubyVPI_util_debug("Host: ruby_init_loadpath()");
    ruby_init_loadpath();

    #ifdef HAVE_RUBY_1_9
    RubyVPI_util_debug("Host: ruby_init_gems(Qtrue)");
    rb_const_set(rb_define_module("Gem"), rb_intern("Enable"), Qtrue);

    RubyVPI_util_debug("Host: Init_prelude()");
    Init_prelude();
    #endif


    //
    // VPI bindings init
    //

    RubyVPI_util_debug("Host: VPI binding init");
    RubyVPI_binding_init();


    //
    // ruby thread init
    //

    RubyVPI_util_debug("Host: user_init()");
    RubyVPI_user_init();
}

PLI_INT32 RubyVPI_host_fini(p_cb_data aCallback)
{
    RubyVPI_util_debug("Host: user fini");
    RubyVPI_user_fini();

    RubyVPI_util_debug("Host: ruby_finalize()");
    ruby_finalize();

/*
    RubyVPI_util_debug("Host: free stack");
    free(RubyVPI_host_gStack);
*/
}

PLI_INT32 RubyVPI_host_resume(p_cb_data aCallback)
{
    RubyVPI_util_debug("Main: callback = %p", aCallback);

    if (aCallback)
    {
        RubyVPI_util_debug("Main: callback.user_data = %p", aCallback->user_data);
    }
    else
    {
        RubyVPI_util_debug("Main: callback is NULL");
    }

    RubyVPI_util_debug("Host: ruby callback for %p =>", aCallback);
    VALUE call = RubyVPI_binding_rubyize_callback(aCallback);
    rb_p(call);

    VALUE target = rb_const_get(rb_cObject, rb_intern("RubyVPI"));
    ID method = rb_intern("resume");

    RubyVPI_util_debug("Host: calling RubyVPI.resume");
    rb_funcall(target, method, 1, call); // pass callback to user code

    return 0;
}
