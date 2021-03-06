// clang-format off
// Generated file (from: bidirectional_sequence_lstm_cifg_peephole.mod.py). Do not edit
std::vector<MixedTypedExample>& get_examples_blackbox() {
static std::vector<MixedTypedExample> examples_blackbox = {
// Begin of an example
{
.operands = {
//Input(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> Dimensions map
  .operandDimensions = {{0, {3, 1, 2}}, {1, {4, 2}}, {2, {4, 2}}, {3, {4, 2}}, {4, {4, 2}}, {5, {4, 4}}, {6, {4, 4}}, {7, {4, 4}}, {8, {4, 4}}, {9, {4}}, {10, {4}}, {11, {4}}, {12, {4}}, {13, {4}}, {14, {4}}, {15, {4}}, {16, {4, 4}}, {17, {4}}, {18, {4, 2}}, {19, {4, 2}}, {20, {4, 2}}, {21, {4, 2}}, {22, {4, 4}}, {23, {4, 4}}, {24, {4, 4}}, {25, {4, 4}}, {26, {4}}, {27, {4}}, {28, {4}}, {29, {4}}, {30, {4}}, {31, {4}}, {32, {4}}, {33, {4, 4}}, {34, {4}}, {35, {1, 4}}, {36, {1, 4}}, {37, {1, 4}}, {38, {1, 4}}, {39, {3, 1, 2}}, {40, {4, 2}}, {41, {4, 2}}, {42, {4, 2}}, {43, {4, 2}}, {44, {4, 2}}, {45, {4, 2}}, {46, {4, 2}}, {47, {4, 2}}, {48, {4}}, {49, {4}}, {50, {4}}, {51, {4}}, {52, {4}}, {53, {4}}, {54, {4}}, {55, {4}}},
  // int -> FLOAT32 map
  .float32Operands = {{0, {2.0f, 3.0f, 3.0f, 4.0f, 1.0f, 1.0f}}, {1, {}}, {2, {-0.55291498f, -0.42866567f, 0.13056988f, -0.3633365f, -0.22755712f, 0.28253698f, 0.24407166f, 0.33826375f}}, {3, {-0.49770179f, -0.27711356f, -0.09624726f, 0.05100781f, 0.04717243f, 0.48944736f, -0.38535351f, -0.17212132f}}, {4, {0.10725588f, -0.02335852f, -0.55932593f, -0.09426838f, -0.44257352f, 0.54939759f, 0.01533556f, 0.42751634f}}, {5, {}}, {6, {-0.13832897f, -0.0515101f, -0.2359007f, -0.16661474f, -0.14340827f, 0.36986142f, 0.23414481f, 0.55899f, 0.10798943f, -0.41174671f, 0.17751795f, -0.34484994f, -0.35874045f, -0.11352962f, 0.27268326f, 0.54058349f}}, {7, {0.54066205f, -0.32668582f, -0.43562764f, -0.56094903f, 0.42957711f, 0.01841056f, -0.32764608f, -0.33027974f, -0.10826075f, 0.20675004f, 0.19069612f, -0.03026325f, -0.54532051f, 0.33003211f, 0.44901288f, 0.21193194f}}, {8, {0.41613156f, 0.42610586f, -0.16495961f, -0.5663873f, 0.30579174f, -0.05115908f, -0.33941799f, 0.23364776f, 0.11178309f, 0.09481031f, -0.26424935f, 0.46261835f, 0.50248802f, 0.26114327f, -0.43736315f, 0.33149987f}}, {9, {}}, {10, {0.47485286f, -0.51955009f, -0.24458408f, 0.31544167f}}, {11, {-0.17135078f, 0.82760304f, 0.85573703f, -0.77109635f}}, {12, {}}, {13, {1.0f, 1.0f, 1.0f, 1.0f}}, {14, {0.0f, 0.0f, 0.0f, 0.0f}}, {15, {0.0f, 0.0f, 0.0f, 0.0f}}, {16, {}}, {17, {}}, {18, {}}, {19, {-0.55291498f, -0.42866567f, 0.13056988f, -0.3633365f, -0.22755712f, 0.28253698f, 0.24407166f, 0.33826375f}}, {20, {-0.49770179f, -0.27711356f, -0.09624726f, 0.05100781f, 0.04717243f, 0.48944736f, -0.38535351f, -0.17212132f}}, {21, {0.10725588f, -0.02335852f, -0.55932593f, -0.09426838f, -0.44257352f, 0.54939759f, 0.01533556f, 0.42751634f}}, {22, {}}, {23, {-0.13832897f, -0.0515101f, -0.2359007f, -0.16661474f, -0.14340827f, 0.36986142f, 0.23414481f, 0.55899f, 0.10798943f, -0.41174671f, 0.17751795f, -0.34484994f, -0.35874045f, -0.11352962f, 0.27268326f, 0.54058349f}}, {24, {0.54066205f, -0.32668582f, -0.43562764f, -0.56094903f, 0.42957711f, 0.01841056f, -0.32764608f, -0.33027974f, -0.10826075f, 0.20675004f, 0.19069612f, -0.03026325f, -0.54532051f, 0.33003211f, 0.44901288f, 0.21193194f}}, {25, {0.41613156f, 0.42610586f, -0.16495961f, -0.5663873f, 0.30579174f, -0.05115908f, -0.33941799f, 0.23364776f, 0.11178309f, 0.09481031f, -0.26424935f, 0.46261835f, 0.50248802f, 0.26114327f, -0.43736315f, 0.33149987f}}, {26, {}}, {27, {0.47485286f, -0.51955009f, -0.24458408f, 0.31544167f}}, {28, {-0.17135078f, 0.82760304f, 0.85573703f, -0.77109635f}}, {29, {}}, {30, {1.0f, 1.0f, 1.0f, 1.0f}}, {31, {0.0f, 0.0f, 0.0f, 0.0f}}, {32, {0.0f, 0.0f, 0.0f, 0.0f}}, {33, {}}, {34, {}}, {35, {0.0f, 0.0f, 0.0f, 0.0f}}, {36, {0.0f, 0.0f, 0.0f, 0.0f}}, {37, {0.0f, 0.0f, 0.0f, 0.0f}}, {38, {0.0f, 0.0f, 0.0f, 0.0f}}, {39, {}}, {40, {}}, {41, {}}, {42, {}}, {43, {}}, {44, {}}, {45, {}}, {46, {}}, {47, {}}, {48, {}}, {49, {}}, {50, {}}, {51, {}}, {52, {}}, {53, {}}, {54, {}}, {55, {}}},
  // int -> INT32 map
  .int32Operands = {},
  // int -> QUANT8_ASYMM map
  .quant8AsymmOperands = {},
  // int -> QUANT16_SYMM map
  .quant16SymmOperands = {},
  // int -> FLOAT16 map
  .float16Operands = {},
  // int -> BOOL8 map
  .bool8Operands = {},
  // int -> QUANT8_SYMM_PER_CHANNEL map
  .quant8ChannelOperands = {},
  // int -> QUANT16_ASYMM map
  .quant16AsymmOperands = {},
  // int -> QUANT8_SYMM map
  .quant8SymmOperands = {},
},
//Output(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> Dimensions map
  .operandDimensions = {{0, {3, 1, 4}}, {1, {3, 1, 4}}},
  // int -> FLOAT32 map
  .float32Operands = {{0, {-0.36444446f, -0.00352185f, 0.12886585f, -0.05163646f, -0.42312205f, -0.01218222f, 0.24201041f, -0.08124574f, -0.358325f, -0.04621704f, 0.21641694f, -0.06471302f}}, {1, {-0.401685f, -0.0232794f, 0.288642f, -0.123074f, -0.42915f, -0.00871577f, 0.20912f, -0.103567f, -0.166398f, -0.00486649f, 0.0697471f, -0.0537578f}}},
  // int -> INT32 map
  .int32Operands = {},
  // int -> QUANT8_ASYMM map
  .quant8AsymmOperands = {},
  // int -> QUANT16_SYMM map
  .quant16SymmOperands = {},
  // int -> FLOAT16 map
  .float16Operands = {},
  // int -> BOOL8 map
  .bool8Operands = {},
  // int -> QUANT8_SYMM_PER_CHANNEL map
  .quant8ChannelOperands = {},
  // int -> QUANT16_ASYMM map
  .quant16AsymmOperands = {},
  // int -> QUANT8_SYMM map
  .quant8SymmOperands = {},
}
},
}, // End of an example
};
return examples_blackbox;
};

std::vector<MixedTypedExample>& get_examples_blackbox_dynamic_output_shape() {
static std::vector<MixedTypedExample> examples_blackbox_dynamic_output_shape = {
// Begin of an example
{
.operands = {
//Input(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> Dimensions map
  .operandDimensions = {{0, {3, 1, 2}}, {1, {4, 2}}, {2, {4, 2}}, {3, {4, 2}}, {4, {4, 2}}, {5, {4, 4}}, {6, {4, 4}}, {7, {4, 4}}, {8, {4, 4}}, {9, {4}}, {10, {4}}, {11, {4}}, {12, {4}}, {13, {4}}, {14, {4}}, {15, {4}}, {16, {4, 4}}, {17, {4}}, {18, {4, 2}}, {19, {4, 2}}, {20, {4, 2}}, {21, {4, 2}}, {22, {4, 4}}, {23, {4, 4}}, {24, {4, 4}}, {25, {4, 4}}, {26, {4}}, {27, {4}}, {28, {4}}, {29, {4}}, {30, {4}}, {31, {4}}, {32, {4}}, {33, {4, 4}}, {34, {4}}, {35, {1, 4}}, {36, {1, 4}}, {37, {1, 4}}, {38, {1, 4}}, {39, {3, 1, 2}}, {40, {4, 2}}, {41, {4, 2}}, {42, {4, 2}}, {43, {4, 2}}, {44, {4, 2}}, {45, {4, 2}}, {46, {4, 2}}, {47, {4, 2}}, {48, {4}}, {49, {4}}, {50, {4}}, {51, {4}}, {52, {4}}, {53, {4}}, {54, {4}}, {55, {4}}},
  // int -> FLOAT32 map
  .float32Operands = {{0, {2.0f, 3.0f, 3.0f, 4.0f, 1.0f, 1.0f}}, {1, {}}, {2, {-0.55291498f, -0.42866567f, 0.13056988f, -0.3633365f, -0.22755712f, 0.28253698f, 0.24407166f, 0.33826375f}}, {3, {-0.49770179f, -0.27711356f, -0.09624726f, 0.05100781f, 0.04717243f, 0.48944736f, -0.38535351f, -0.17212132f}}, {4, {0.10725588f, -0.02335852f, -0.55932593f, -0.09426838f, -0.44257352f, 0.54939759f, 0.01533556f, 0.42751634f}}, {5, {}}, {6, {-0.13832897f, -0.0515101f, -0.2359007f, -0.16661474f, -0.14340827f, 0.36986142f, 0.23414481f, 0.55899f, 0.10798943f, -0.41174671f, 0.17751795f, -0.34484994f, -0.35874045f, -0.11352962f, 0.27268326f, 0.54058349f}}, {7, {0.54066205f, -0.32668582f, -0.43562764f, -0.56094903f, 0.42957711f, 0.01841056f, -0.32764608f, -0.33027974f, -0.10826075f, 0.20675004f, 0.19069612f, -0.03026325f, -0.54532051f, 0.33003211f, 0.44901288f, 0.21193194f}}, {8, {0.41613156f, 0.42610586f, -0.16495961f, -0.5663873f, 0.30579174f, -0.05115908f, -0.33941799f, 0.23364776f, 0.11178309f, 0.09481031f, -0.26424935f, 0.46261835f, 0.50248802f, 0.26114327f, -0.43736315f, 0.33149987f}}, {9, {}}, {10, {0.47485286f, -0.51955009f, -0.24458408f, 0.31544167f}}, {11, {-0.17135078f, 0.82760304f, 0.85573703f, -0.77109635f}}, {12, {}}, {13, {1.0f, 1.0f, 1.0f, 1.0f}}, {14, {0.0f, 0.0f, 0.0f, 0.0f}}, {15, {0.0f, 0.0f, 0.0f, 0.0f}}, {16, {}}, {17, {}}, {18, {}}, {19, {-0.55291498f, -0.42866567f, 0.13056988f, -0.3633365f, -0.22755712f, 0.28253698f, 0.24407166f, 0.33826375f}}, {20, {-0.49770179f, -0.27711356f, -0.09624726f, 0.05100781f, 0.04717243f, 0.48944736f, -0.38535351f, -0.17212132f}}, {21, {0.10725588f, -0.02335852f, -0.55932593f, -0.09426838f, -0.44257352f, 0.54939759f, 0.01533556f, 0.42751634f}}, {22, {}}, {23, {-0.13832897f, -0.0515101f, -0.2359007f, -0.16661474f, -0.14340827f, 0.36986142f, 0.23414481f, 0.55899f, 0.10798943f, -0.41174671f, 0.17751795f, -0.34484994f, -0.35874045f, -0.11352962f, 0.27268326f, 0.54058349f}}, {24, {0.54066205f, -0.32668582f, -0.43562764f, -0.56094903f, 0.42957711f, 0.01841056f, -0.32764608f, -0.33027974f, -0.10826075f, 0.20675004f, 0.19069612f, -0.03026325f, -0.54532051f, 0.33003211f, 0.44901288f, 0.21193194f}}, {25, {0.41613156f, 0.42610586f, -0.16495961f, -0.5663873f, 0.30579174f, -0.05115908f, -0.33941799f, 0.23364776f, 0.11178309f, 0.09481031f, -0.26424935f, 0.46261835f, 0.50248802f, 0.26114327f, -0.43736315f, 0.33149987f}}, {26, {}}, {27, {0.47485286f, -0.51955009f, -0.24458408f, 0.31544167f}}, {28, {-0.17135078f, 0.82760304f, 0.85573703f, -0.77109635f}}, {29, {}}, {30, {1.0f, 1.0f, 1.0f, 1.0f}}, {31, {0.0f, 0.0f, 0.0f, 0.0f}}, {32, {0.0f, 0.0f, 0.0f, 0.0f}}, {33, {}}, {34, {}}, {35, {0.0f, 0.0f, 0.0f, 0.0f}}, {36, {0.0f, 0.0f, 0.0f, 0.0f}}, {37, {0.0f, 0.0f, 0.0f, 0.0f}}, {38, {0.0f, 0.0f, 0.0f, 0.0f}}, {39, {}}, {40, {}}, {41, {}}, {42, {}}, {43, {}}, {44, {}}, {45, {}}, {46, {}}, {47, {}}, {48, {}}, {49, {}}, {50, {}}, {51, {}}, {52, {}}, {53, {}}, {54, {}}, {55, {}}},
  // int -> INT32 map
  .int32Operands = {},
  // int -> QUANT8_ASYMM map
  .quant8AsymmOperands = {},
  // int -> QUANT16_SYMM map
  .quant16SymmOperands = {},
  // int -> FLOAT16 map
  .float16Operands = {},
  // int -> BOOL8 map
  .bool8Operands = {},
  // int -> QUANT8_SYMM_PER_CHANNEL map
  .quant8ChannelOperands = {},
  // int -> QUANT16_ASYMM map
  .quant16AsymmOperands = {},
  // int -> QUANT8_SYMM map
  .quant8SymmOperands = {},
},
//Output(s)
{ // See tools/test_generator/include/TestHarness.h:MixedTyped
  // int -> Dimensions map
  .operandDimensions = {{0, {3, 1, 4}}, {1, {3, 1, 4}}},
  // int -> FLOAT32 map
  .float32Operands = {{0, {-0.36444446f, -0.00352185f, 0.12886585f, -0.05163646f, -0.42312205f, -0.01218222f, 0.24201041f, -0.08124574f, -0.358325f, -0.04621704f, 0.21641694f, -0.06471302f}}, {1, {-0.401685f, -0.0232794f, 0.288642f, -0.123074f, -0.42915f, -0.00871577f, 0.20912f, -0.103567f, -0.166398f, -0.00486649f, 0.0697471f, -0.0537578f}}},
  // int -> INT32 map
  .int32Operands = {},
  // int -> QUANT8_ASYMM map
  .quant8AsymmOperands = {},
  // int -> QUANT16_SYMM map
  .quant16SymmOperands = {},
  // int -> FLOAT16 map
  .float16Operands = {},
  // int -> BOOL8 map
  .bool8Operands = {},
  // int -> QUANT8_SYMM_PER_CHANNEL map
  .quant8ChannelOperands = {},
  // int -> QUANT16_ASYMM map
  .quant16AsymmOperands = {},
  // int -> QUANT8_SYMM map
  .quant8SymmOperands = {},
}
},
}, // End of an example
};
return examples_blackbox_dynamic_output_shape;
};

