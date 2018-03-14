add_llvm_loadable_module( CSE231
  part1/CountStaticInstructions.cpp
  part1/CountDynamicInstructions.cpp
  part1/BranchBias.cpp
  DFA/231DFA.h
  DFA/ReachingDefinitionAnalysis.cpp
  DFA/LivenessAnalysis.cpp

  PLUGIN_TOOL
  opt
  )
