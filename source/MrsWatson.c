//
//  MrsWatson.c
//  MrsWatson
//
//  Created by Nik Reiman on 12/5/11.
//  Copyright (c) 2011 Teragon Audio. All rights reserved.
//

#include <stdio.h>

#include "EventLogger.h"
#include "CharString.h"
#include "MrsWatson.h"
#include "BuildInfo.h"
#include "ProgramOption.h"
#include "SampleSource.h"
#include "PluginChain.h"
#include "StringUtilities.h"

void fillVersionString(CharString outString) {
  snprintf(outString->data, outString->capacity, "%s, version %d.%d.%d", PROGRAM_NAME, VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

int main(int argc, char** argv) {
  initEventLogger();

  // Input/Output sources, plugin chain, and other required objects
  SampleSource inputSource = NULL;
  PluginChain pluginChain = newPluginChain();
  boolean shouldDisplayPluginInfo = false;

  ProgramOptions programOptions = newProgramOptions();
  if(!parseCommandLine(programOptions, argc, argv)) {
    return RETURN_CODE_INVALID_ARGUMENT;
  }

  // If the user wanted help or the version info, print those out and then exit right away
  if(programOptions[OPTION_HELP]->enabled) {
    printf("Usage: %s (options), where options include:\n", getFileBasename(argv[0]));
    printProgramOptions(programOptions);
    return RETURN_CODE_NOT_RUN;
  }
  else if(programOptions[OPTION_VERSION]->enabled) {
    CharString versionString = newCharString();
    fillVersionString(versionString);
    printf("%s\nCopyright (c) %d, %s. All rights reserved.\n\n", versionString->data, buildYear(), COPYRIGHT_HOLDER);
    freeCharString(versionString);

    CharString wrappedLicenseInfo = newCharStringWithCapacity(STRING_LENGTH_LONG);
    wrapCharStringForTerminal(LICENSE_STRING, wrappedLicenseInfo->data, 0);
    printf("%s\n\n", wrappedLicenseInfo->data);
    freeCharString(wrappedLicenseInfo);

    return RETURN_CODE_NOT_RUN;
  }

  // Parse these options first so that log messages displayed in the below loop are properly displayed
  if(programOptions[OPTION_VERBOSE]->enabled) {
    setLogLevel(LOG_DEBUG);
  }
  else if(programOptions[OPTION_QUIET]->enabled) {
    setLogLevel(LOG_ERROR);
  }
  if(programOptions[OPTION_COLOR_LOGGING]->enabled) {
    ProgramOption option = programOptions[OPTION_COLOR_LOGGING];
    if(isCharStringEmpty(option->argument)) {
      setLoggingColorScheme(COLOR_SCHEME_DARK);
    }
    else if(isCharStringEqualToCString(option->argument, "dark", false)) {
      setLoggingColorScheme(COLOR_SCHEME_DARK);
    }
    else if(isCharStringEqualToCString(option->argument, "light", false)) {
      setLoggingColorScheme(COLOR_SCHEME_LIGHT);
    }
    else {
      logCritical("Unknown color scheme '%s'", option->argument->data);
      setLoggingColorScheme(COLOR_SCHEME_NONE);
    }
  }

  // Parse other options and set up necessary objects
  for(int i = 0; i < NUM_OPTIONS; i++) {
    ProgramOption option = programOptions[i];
    if(option->enabled) {
      if(option->index == OPTION_INPUT_SOURCE) {
        SampleSourceType inputSourceType = guessSampleSourceType(option->argument);
        inputSource = newSampleSource(inputSourceType, option->argument);
      }
      else if(option->index == OPTION_PLUGIN) {
        addPluginsFromArgumentString(pluginChain, option->argument);
      }
      else if(option->index == OPTION_DISPLAY_INFO) {
        shouldDisplayPluginInfo = true;
      }
    }
  }
  freeProgramOptions(programOptions);

  // Say hello!
  CharString versionString = newCharString();
  fillVersionString(versionString);
  logInfo("%s initialized", versionString->data);
  freeCharString(versionString);

  // Check required variables to make sure we have everything needed to start processing
  if(inputSource == NULL) {
    logError("No input source given");
    return RETURN_CODE_MISSING_REQUIRED_OPTION;
  }
  else if(pluginChain->numPlugins == 0) {
    logError("No plugins loaded");
    return RETURN_CODE_MISSING_REQUIRED_OPTION;
  }

  // Prepare input/output sources, plugins
  inputSource->openSampleSource(inputSource, SAMPLE_SOURCE_OPEN_READ);
  initializePluginChain(pluginChain);

  if(shouldDisplayPluginInfo) {
    displayPluginInfo(pluginChain);
  }

  // Shut down and free data (will also close open filehandles, plugins, etc)
  logInfo("Shutting down");
  freeSampleSource(inputSource);
  freePluginChain(pluginChain);

  logInfo("Goodbye!");
  return RETURN_CODE_SUCCESS;
}