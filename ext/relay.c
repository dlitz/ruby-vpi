/*
  Copyright 1999 Kazuhiro HIWADA
  Copyright 2006 Suraj N. Kurapati

  This file is part of Ruby-VPI.

  Ruby-VPI is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  Ruby-VPI is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Ruby-VPI; if not, write to the Free Software Foundation,
  Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "relay.h"

#include "swig.h"
#include "common.h"
#include <pthread.h>
#include <ruby.h>
#include <assert.h>


pthread_t relay__rubyThread;
pthread_mutex_t relay__rubyLock;
pthread_mutex_t relay__verilogLock;

void relay_init() {
  pthread_mutex_init(&relay__rubyLock, NULL);
  pthread_mutex_lock(&relay__rubyLock);
  pthread_mutex_init(&relay__verilogLock, NULL);
  pthread_mutex_lock(&relay__verilogLock);
}

void relay_ruby() {
  pthread_mutex_unlock(&relay__rubyLock);
  pthread_mutex_lock(&relay__verilogLock);
}

void relay_verilog() {
  pthread_mutex_unlock(&relay__verilogLock);
  pthread_mutex_lock(&relay__rubyLock);
}

typedef struct {
  PLI_BYTE8** mArgs;	/// Array of command-line arguments.
  uint mCount;	/// Number of command-line arguments.
} relay__RubyOptions__def;

/**
  @param	apRubyOptions	relay__RubyOptions__def structure which contains command-line options passsed to the Ruby interpreter.
  @note	The structure will be freed *deeply* after use.
*/
void* ruby_run_handshake(void* apRubyOptions) {
  // initialize Ruby interpreter
  ruby_init();
  ruby_init_loadpath();

  swig_init();

    // pass command-line arguments to the interpreter
    relay__RubyOptions__def* pRubyOptions = (relay__RubyOptions__def*) apRubyOptions;

    PLI_BYTE8** argv = pRubyOptions->mArgs;
    uint argc = pRubyOptions->mCount;

    ruby_options(argc, argv);

    // free the memory used by command-line options
    uint i;
    for (i = 0; i < argc; i++) {
      free(argv[i]);
    }

    free(argv);
    free(pRubyOptions);


  // start Ruby interpreter
  ruby_run();


  // Ruby interpreter is now finished, so clean it up before terminating this thread
  ruby_finalize();
  return NULL;
}

void relay_ruby_run() {
  relay__RubyOptions__def* pRubyOptions = malloc(sizeof(relay__RubyOptions__def));

  if (pRubyOptions) {
    pRubyOptions->mArgs = NULL;
    pRubyOptions->mCount = 0;

    // transform the arguments passed to this function by Verilog into command-line arguments for Ruby interpeter
    vpiHandle vCall = vpi_handle(vpiSysTfCall, NULL);

    if (vCall) {
      vpiHandle vCallArgs = vpi_iterate(vpiArgument, vCall);

      if (vCallArgs) {
        vpiHandle vArg;
        s_vpi_value argVal;
        argVal.format = vpiStringVal;

        while ((vArg = vpi_scan(vCallArgs)) != NULL) {
          vpi_get_value(vArg, &argVal);

          pRubyOptions->mCount++;


          if (pRubyOptions->mArgs == NULL)
            pRubyOptions->mArgs = malloc(sizeof(PLI_BYTE8*) * pRubyOptions->mCount);
          else
            pRubyOptions->mArgs = realloc(pRubyOptions->mArgs, sizeof(PLI_BYTE8*) * pRubyOptions->mCount);

          assert(pRubyOptions->mArgs != NULL);


          pRubyOptions->mArgs[pRubyOptions->mCount-1] = strdup(argVal.value.str);
        }
      }
    }

    pthread_create(&relay__rubyThread, 0, ruby_run_handshake, pRubyOptions);
    return;
  }

  common_printf("error: unable to allocate memory for Ruby's command-line arguments.");
  exit(EXIT_FAILURE);
}
