/*
  ==============================================================================

    residfp_wrapper.cc
    Created: 2026
    Author:  Cline

  ==============================================================================
*/

/**
    Implementation file for residfp wrapper.
    
    Most of the wrapper is header-only since it's just re-exporting and
    small wrapper methods. This file exists to provide a place for
    any implementation-specific code that might be needed in the future.
    
    The main SID class methods are all inline in the header file since
    they're simple delegators to the underlying reSIDfp::SID.
*/

#include "residfp_wrapper.h"

// Any additional implementation can be added here if needed