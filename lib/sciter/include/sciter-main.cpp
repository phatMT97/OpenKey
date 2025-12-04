//
//  sciter-main.cpp
//  sciter
//
//  Created by andrew on 2023-11-18.
//  Copyright (c) 2018 Andrew Fedoniouk. All rights reserved.
//

#include "sciter-x-window.hpp"
#include "aux-cvt.h"

using namespace aux;

static std::vector<sciter::string> _argv;

#ifndef SKIP_MAIN

  #ifdef WINDOWS
  int wmain (int argc, wchar_t *argv[])
  {
    for (int n = 0; n < argc; ++n)
      _argv.push_back(argv[n]);
    sciter::application::start(argc, (const WCHAR **) argv);
    int r = uimain([]() -> int { return sciter::application::run(); });
    sciter::application::shutdown();
  }
  #else
  int main(int argc, char* argv[])
  {
    const WCHAR* args[argc];
    for (int n = 0; n < argc; ++n)
      _argv.push_back(utf2w(chars_of(argv[n])));
    for (int n = 0; n < argc; ++n)
      args[n] = _argv[n].c_str();
    sciter::application::start(argc,args);
    int r = uimain([]() -> int { 
      return sciter::application::run(); 
    });
    sciter::application::shutdown();
  }
  #endif

#endif


namespace sciter {

  namespace application {
    HINSTANCE hinstance()
    {
      return nullptr; // not used
    }

    const std::vector<sciter::string>& argv() {
      return _argv;
    }
  }

}
