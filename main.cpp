/* Copyright (c) 2023 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <bitset>
#include <sstream>

#include "SPIRV-Reflect/spirv_reflect.h"

void reflectInputVariables(const std::vector<SpvReflectInterfaceVariable *>& inputVars);
void reflectDescriptorSets(const std::vector<SpvReflectDescriptorSet*>& sets,
                           const std::vector<SpvReflectDescriptorBinding*>& bindings);

// =================================================================================================
// PrintUsage()
// =================================================================================================
void PrintUsage() {
    std::cout << "Usage: explorer path/to/SPIR-V/bytecode.spv\n"
              << "\tThis is used to set a breakpoint and explorer the API and "
                 "how to access info needed\n";
}

// =================================================================================================
// main()
// =================================================================================================
int main(int argn, char** argv) {
    std::string input_spv_path = SHADER_DIR + std::string("test_shader_vert.spv");

    std::ifstream spv_ifstream(input_spv_path.c_str(), std::ios::binary);
    if (!spv_ifstream.is_open()) {
        std::cerr << "ERROR: could not open '" << input_spv_path << "' for reading\n";
        return EXIT_FAILURE;
    }

    spv_ifstream.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(spv_ifstream.tellg());
    spv_ifstream.seekg(0, std::ios::beg);

    std::vector<char> spv_data(size);
    spv_ifstream.read(spv_data.data(), size);

    SpvReflectShaderModule module = {};
    SpvReflectResult result = spvReflectCreateShaderModule(spv_data.size(), spv_data.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    // Go through each enumerate to examine it
    uint32_t count = 0;

    result = spvReflectEnumerateDescriptorSets(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorSet*> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::cout << "Descriptor sets " << sets.size() << std::endl;
    for (auto& set : sets) {
        std::cout << set->binding_count << ":";
        for (int i = 0; i < set->binding_count; ++i) {
            const auto v = SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE;
            auto d = SpvReflectDecorationFlagBits::SPV_REFLECT_DECORATION_PER_VERTEX;
            SpvReflectDescriptorType f;
            std::cout << (set->bindings[i]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ? "ImageSampler" : set->bindings[i]->type_description->type_name) << " " << set->bindings[i]->name << ",  ";
        }
        std::cout << "\n";
    }

    result = spvReflectEnumerateDescriptorBindings(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorBinding*> bindings(count);
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumerateInterfaceVariables(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable*> interface_variables(count);
    result = spvReflectEnumerateInterfaceVariables(&module, &count, interface_variables.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumerateInputVariables(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable*> input_variables(count);
    result = spvReflectEnumerateInputVariables(&module, &count, input_variables.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    result = spvReflectEnumerateOutputVariables(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable*> output_variables(count);
    result = spvReflectEnumerateOutputVariables(&module, &count, output_variables.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumeratePushConstantBlocks(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectBlockVariable*> push_constant(count);
    result = spvReflectEnumeratePushConstantBlocks(&module, &count, push_constant.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    reflectInputVariables(input_variables);
    reflectDescriptorSets(sets, bindings);

    // Can set a breakpoint here and explorer the various variables enumerated.
    spvReflectDestroyShaderModule(&module);

    return 0;
}

void WriteBaselineTypeDescriptions() {
    std::ifstream inputFile(std::string(SHADER_DIR) + "/../BaseDescs.h");

    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
    }
    std::string line;
    while (std::getline(inputFile, line)) {
        std::cout << line << std::endl;
    }
    std::cout << "\n\n";
    inputFile.close();
}

std::string GetTypeAsString(SpvReflectInterfaceVariable* inVar) {
    std::ostringstream ssBuilder;
    if (inVar->type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX) {
        ssBuilder << "mat" << inVar->numeric.matrix.row_count << "x" << inVar->numeric.matrix.column_count;
    } else if (inVar->type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR) {
        bool isFloat = inVar->type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;
        ssBuilder << (isFloat ? "" : "i") << "vec" << inVar->numeric.vector.component_count;
    } else {
        bool isFloat = inVar->type_description->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;
        bool isUnsigned = !isFloat && !inVar->numeric.scalar.signedness;
        ssBuilder << (isUnsigned ? "unsigned " : "") << (isFloat ? "float" : "int");
    }
    return ssBuilder.str();
}
std::string GetFormatAsString(SpvReflectFormat format) {
    switch (format) {
        case SPV_REFLECT_FORMAT_UNDEFINED:           return "VK_FORMAT_UNDEFINED";
        case SPV_REFLECT_FORMAT_R16_UINT:            return "VK_FORMAT_R16_UINT";
        case SPV_REFLECT_FORMAT_R16_SINT:            return "VK_FORMAT_R16_SINT";
        case SPV_REFLECT_FORMAT_R16_SFLOAT:          return "VK_FORMAT_R16_SFLOAT";
        case SPV_REFLECT_FORMAT_R16G16_UINT:         return "VK_FORMAT_R16G16_UINT";
        case SPV_REFLECT_FORMAT_R16G16_SINT:         return "VK_FORMAT_R16G16_SINT";
        case SPV_REFLECT_FORMAT_R16G16_SFLOAT:       return "VK_FORMAT_R16G16_SFLOAT";
        case SPV_REFLECT_FORMAT_R16G16B16_UINT:      return "VK_FORMAT_R16G16B16_UINT";
        case SPV_REFLECT_FORMAT_R16G16B16_SINT:      return "VK_FORMAT_R16G16B16_SINT";
        case SPV_REFLECT_FORMAT_R16G16B16_SFLOAT:    return "VK_FORMAT_R16G16B16_SFLOAT";
        case SPV_REFLECT_FORMAT_R16G16B16A16_UINT:   return "VK_FORMAT_R16G16B16A16_UINT";
        case SPV_REFLECT_FORMAT_R16G16B16A16_SINT:   return "VK_FORMAT_R16G16B16A16_SINT";
        case SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT: return "VK_FORMAT_R16G16B16A16_SFLOAT";
        case SPV_REFLECT_FORMAT_R32_UINT:            return "VK_FORMAT_R32_UINT";
        case SPV_REFLECT_FORMAT_R32_SINT:            return "VK_FORMAT_R32_SINT";
        case SPV_REFLECT_FORMAT_R32_SFLOAT:          return "VK_FORMAT_R32_SFLOAT";
        case SPV_REFLECT_FORMAT_R32G32_UINT:         return "VK_FORMAT_R32G32_UINT";
        case SPV_REFLECT_FORMAT_R32G32_SINT:         return "VK_FORMAT_R32G32_SINT";
        case SPV_REFLECT_FORMAT_R32G32_SFLOAT:       return "VK_FORMAT_R32G32_SFLOAT";
        case SPV_REFLECT_FORMAT_R32G32B32_UINT:      return "VK_FORMAT_R32G32B32_UINT";
        case SPV_REFLECT_FORMAT_R32G32B32_SINT:      return "VK_FORMAT_R32G32B32_SINT";
        case SPV_REFLECT_FORMAT_R32G32B32_SFLOAT:    return "VK_FORMAT_R32G32B32_SFLOAT";
        case SPV_REFLECT_FORMAT_R32G32B32A32_UINT:   return "VK_FORMAT_R32G32B32A32_UINT";
        case SPV_REFLECT_FORMAT_R32G32B32A32_SINT:   return "VK_FORMAT_R32G32B32A32_SINT";
        case SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT: return "VK_FORMAT_R32G32B32A32_SFLOAT";
        case SPV_REFLECT_FORMAT_R64_UINT:            return "VK_FORMAT_R64_UINT";
        case SPV_REFLECT_FORMAT_R64_SINT:            return "VK_FORMAT_R64_SINT";
        case SPV_REFLECT_FORMAT_R64_SFLOAT:          return "VK_FORMAT_R64_SFLOAT";
        case SPV_REFLECT_FORMAT_R64G64_UINT:         return "VK_FORMAT_R64G64_UINT";
        case SPV_REFLECT_FORMAT_R64G64_SINT:         return "VK_FORMAT_R64G64_SINT";
        case SPV_REFLECT_FORMAT_R64G64_SFLOAT:       return "VK_FORMAT_R64G64_SFLOAT";
        case SPV_REFLECT_FORMAT_R64G64B64_UINT:      return "VK_FORMAT_R64G64B64_UINT";
        case SPV_REFLECT_FORMAT_R64G64B64_SINT:      return "VK_FORMAT_R64G64B64_SINT";
        case SPV_REFLECT_FORMAT_R64G64B64_SFLOAT:    return "VK_FORMAT_R64G64B64_SFLOAT";
        case SPV_REFLECT_FORMAT_R64G64B64A64_UINT:   return "VK_FORMAT_R64G64B64A64_UINT";
        case SPV_REFLECT_FORMAT_R64G64B64A64_SINT:   return "VK_FORMAT_R64G64B64A64_SINT";
        case SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT: return "VK_FORMAT_R64G64B64A64_SFLOAT";
        default: return "Unknown format";
    }
}

void WriteVertexInputs(const std::vector<SpvReflectInterfaceVariable *>& inputVars) {
    uint32_t binding = 0;
    std::ostringstream ssBuilder;
    std::vector<std::pair<uint32_t, SpvReflectInterfaceVariable*>> vertexInputs;
    for (auto inVar : inputVars) {
        std::string inName(inVar->name);
        if (inName[inName.size() - 1] == 'i' && inName[inName.size() - 2] == '_') continue;
        vertexInputs.emplace_back(inVar->location, inVar);
    }
    std::sort(vertexInputs.begin(), vertexInputs.end());
    std::string structName("TestVertex");
    ssBuilder << "struct " << structName << " {\n";
    for (auto [i, inVar] : vertexInputs) {
        ssBuilder << "\t" << GetTypeAsString(inVar) << " " << inVar->name << ";\n";
    }

    ssBuilder << "};\n\n";
    ssBuilder << "VkVertexInputBindingDescription " << structName << "InputBinding {\n";
    ssBuilder << "\t.binding = " << binding << ",\n";
    ssBuilder << "\t.stride = sizeof(" << structName << "),\n";
    ssBuilder << "\t.inputRate = VK_VERTEX_INPUT_RATE_VERTEX\n";
    ssBuilder << "};\n\n";

    ssBuilder << "std::array<VkVertexInputAttributeDescription, " << vertexInputs.size() << "> " << structName << "VertAttribs {\n";
    for (auto [i, inVar] : vertexInputs) {
        ssBuilder << "\tVkVertexInputAttributeDescription {\n";
        ssBuilder << "\t\t.location = " <<  inVar->location << ",\n";
        ssBuilder << "\t\t.binding = " <<  binding << ",\n";
        ssBuilder << "\t\t.format = " << GetFormatAsString(inVar->format) << ",\n";
        ssBuilder << "\t\t.offset = " << "offsetof(" << structName << ", " << inVar->name << "),\n";
        ssBuilder << "\t},\n";
    }
    ssBuilder << "};\n";
    std::cout << ssBuilder.str() << std::endl;
}
void WriteInstanceInputs(const std::vector<SpvReflectInterfaceVariable *>& inputVars) {
    uint32_t binding = 1;
    std::ostringstream ssBuilder;
    std::vector<std::pair<uint32_t, SpvReflectInterfaceVariable*>> vertexInputs;
    for (auto inVar : inputVars) {
        std::string inName(inVar->name);
        if (inName[inName.size() - 1] != 'i' || inName[inName.size() - 2] != '_') continue;
        vertexInputs.emplace_back(inVar->location, inVar);
    }
    std::sort(vertexInputs.begin(), vertexInputs.end());
    std::string structName("TestInstance");
    ssBuilder << "struct " << structName << " {\n";
    for (auto [i, inVar] : vertexInputs) {
        ssBuilder << "\t" << GetTypeAsString(inVar) << " " << inVar->name << ";\n";
    }

    ssBuilder << "};\n\n";
    ssBuilder << "VkVertexInputBindingDescription " << structName << "InputBinding {\n";
    ssBuilder << "\t.binding = " << binding << ",\n";
    ssBuilder << "\t.stride = sizeof(" << structName << "),\n";
    ssBuilder << "\t.inputRate = VK_VERTEX_INPUT_RATE_VERTEX\n";
    ssBuilder << "};\n\n";

    ssBuilder << "std::array<VkVertexInputAttributeDescription, " << vertexInputs.size() << "> " << structName << "VertAttribs {\n";
    for (auto [i, inVar] : vertexInputs) {
        ssBuilder << "\tVkVertexInputAttributeDescription {\n";
        ssBuilder << "\t\t.location = " <<  inVar->location << ",\n";
        ssBuilder << "\t\t.binding = " <<  binding << ",\n";
        ssBuilder << "\t\t.format = " << GetFormatAsString(inVar->format) << ",\n";
        ssBuilder << "\t\t.offset = " << "offsetof(" << structName << ", " << inVar->name << "),\n";
        ssBuilder << "\t},\n";
    }
    ssBuilder << "};\n";
    std::cout << ssBuilder.str() << std::endl;
}

void reflectInputVariables(const std::vector<SpvReflectInterfaceVariable *>& inputVars) {
    WriteBaselineTypeDescriptions();
    WriteVertexInputs(inputVars);
    WriteInstanceInputs(inputVars);
}


void reflectDescriptorSets(const std::vector<SpvReflectDescriptorSet*>& sets,
                           const std::vector<SpvReflectDescriptorBinding*>& bindings) {
    std::vector<std::vector<SpvReflectDescriptorBinding*>> bindingsToSet_forDebug;
    for (auto set : sets) {
        bindingsToSet_forDebug.emplace_back();
        for (size_t i = 0; i < set->binding_count; ++i) {
            bindingsToSet_forDebug.back().push_back(set->bindings[i]);
        }
    }
    std::cout << sets.size() << ", " << bindings.size() << std::endl;

}

void reflectPushConstants(const std::vector<SpvReflectBlockVariable*>& pushConstants) {
}