//
// Created by idemaj on 6/20/24.
//
#include "main.h"


std::string WriteFromFile(const std::string &inFilename) {
    std::ifstream inputFile(inFilename);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
    }

    std::string line;
    std::ostringstream ssBuilder;
    while (std::getline(inputFile, line)) {
        ssBuilder << line << "\n";
    }
    ssBuilder << "\n\n";
    inputFile.close();

    return ssBuilder.str();
}

std::string WriteDescSetLayoutBoilerplate() {
    return WriteFromFile(std::string(BOILERPLATE_DIR) + "/IN_DescSetLayouts.h");
}

std::string WriteTypeDescriptionsBoilerplate() {
    return WriteFromFile(std::string(BOILERPLATE_DIR) + "/IN_BaseDescs.h");
}


std::string GetTypeAsString(SpvReflectInterfaceVariable *inVar) {
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

void reflectInputVariables(const std::vector<SpvReflectInterfaceVariable *> &inputVars) {
    auto initTypeDefs = WriteTypeDescriptionsBoilerplate();
    auto vertInputs = WriteVertexInputs(inputVars, "");
    auto instanceInputs = WriteInstanceInputs(inputVars, "");
    std::ofstream outFile(std::string(OUT_DIR) + "/EX_InputData.h");
    if (!outFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
    }

    outFile << "#include <vulkan/vulkan.h>\n";
    outFile << "#include <array>\n\n";
    outFile << initTypeDefs;
    outFile << vertInputs;
    outFile << instanceInputs;

    outFile.close();
}


std::string GetTypeAsString(SpvReflectTypeDescription *typeDesc) {
    std::ostringstream ssBuilder;
    if (typeDesc->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_MATRIX) {
        ssBuilder << "mat" << typeDesc->traits.numeric.matrix.row_count << "x" << typeDesc->traits.numeric.matrix.column_count;
    } else if (typeDesc->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_VECTOR) {
        bool isFloat = typeDesc->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;
        ssBuilder << (isFloat ? "" : "i") << "vec" << typeDesc->traits.numeric.vector.component_count;
    } else {
        bool isFloat = typeDesc->type_flags & SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_FLOAT;
        bool isUnsigned = !isFloat && !typeDesc->traits.numeric.scalar.signedness;
        ssBuilder << (isUnsigned ? "unsigned " : "") << (isFloat ? "float" : "int");
    }
    return ssBuilder.str();
}


/**
 * @param sets
 * @param bindings
 */
void reflectDescriptorSets(const std::string& pipelineName,
                           const std::vector<SpvReflectDescriptorSet*>& sets) {
    std::vector<std::string> DEFAULT_DESC_POSTFIXES = {
            "Global", "Material", "Local",
    };

    std::ofstream outFile(std::string(OUT_DIR) + "/EX_DescSetLayoutData.h");
    if (!outFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
    }

    outFile << "#include <vulkan/vulkan.h>\n";
    outFile << "#include <array>\n";
    outFile << "#include \"InputData.h\"\n\n";

    std::vector<std::pair<uint32_t, std::string>> regDescSets;
    std::vector<std::vector<SpvReflectDescriptorBinding*>> bindingsToSet_forDebug;
    outFile << WriteDescSetLayoutBoilerplate() << std::endl;
    for (auto set : sets) {
        std::string setName = pipelineName + DEFAULT_DESC_POSTFIXES[set->set];
        std::vector<SpvReflectDescriptorBinding *> setBindings(set->bindings, set->bindings + set->binding_count);
        outFile << WriteUsedStructsInDescSet(setBindings) << "\n\n";
        outFile << WriteDescSetLayout(setBindings, setName) << "\n\n";
        regDescSets.emplace_back(set->set, setName);
    }
    outFile << "\n\n// TODO: functionality for coupling bindings to stages" << std::endl;
    outFile << WriteDescSetLayoutManager(regDescSets, sets) << std::endl;
}

std::string WriteDescSetLayoutManager(const std::vector<std::pair<uint32_t, std::string>> &regDescSets,
                                      const std::vector<SpvReflectDescriptorSet *> &sets) {
    std::ostringstream ssBuilder;
    for (const auto& [setID, setName] : regDescSets) {
        ssBuilder << "typedef " << setName << "_DescriptorSet<";
        uint32_t numBindings = sets[setID]->binding_count;
        for (uint32_t i = 0; i < numBindings; ++i) {
            ssBuilder << "VK_SHADER_STAGE_ALL_GRAPHICS, ";
        }
        ssBuilder << "VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT> " << setName << "_IMPL;\n";
    }
    ssBuilder << "typedef TypedDescriptorSetManager<";
    for (uint32_t i = 0; i < regDescSets.size() - 1; ++i) {
        ssBuilder << regDescSets[i].second << "_IMPL, ";
    }
    ssBuilder << regDescSets[regDescSets.size() - 1].second << "_IMPL> TestDescriptorSetManager;\n";
    ssBuilder << "typedef TestDescriptorSetManager MainDescriptorSetManager;\n";
    return ssBuilder.str();
}

std::string WriteDescSetLayout(const std::vector<SpvReflectDescriptorBinding *> &bindings, const std::string &setName) {
    std::ostringstream ssBuilder;
    const char* stageFlagPostfix = "_STAGES";
    const char* baseSetClassName = "Base_DescriptorSet";
    ssBuilder << "template <";
    for (auto* b : bindings)
        ssBuilder << "VkShaderStageFlags " << b->name << stageFlagPostfix << ", ";
    ssBuilder << "VkDescriptorSetLayoutCreateFlags LAYOUT_FLAGS=0>\n";
    ssBuilder << "class " << setName << "_DescriptorSet : public " << baseSetClassName << "<" << bindings.size() << "> {\n";
    ssBuilder << "public:\n";
    ssBuilder << "\texplicit " << setName << "_DescriptorSet(VkDevice device) : " << baseSetClassName << "(device) {\n";
    for (auto* b : bindings) {
        ssBuilder << "\t\tm_descriptors[" << b->binding << "] = Descriptor{ \"" << b->name << "\", "
                  << GetDescriptorTypeAsString(b->descriptor_type) << ", " << b->count << ", ";
        if (b->type_description->type_name) // If a buffer struct
            ssBuilder << "sizeof(" << b->type_description->type_name << "), ";
        else ssBuilder << "0, ";
        ssBuilder << b->name << stageFlagPostfix << " };\n";
    }
    ssBuilder << "\t\tMakeDescriptorSetLayout(device, LAYOUT_FLAGS);\n\t}\n";
    ssBuilder << "};\n";

    return ssBuilder.str();
}

bool isIn(const std::string& key, const std::vector<std::string> &vals) {
    return std::find(vals.begin(), vals.end(), key) != vals.end();
}
std::string
WriteUsedStructsInDescSet(const std::vector<SpvReflectDescriptorBinding *> &bindings, const std::vector<std::string> &prohibitedStructs) {
    // TODO: cannot handle struct types yet, that shouldn't be a terrible change tho
    std::ostringstream ssBuilder;
    for (SpvReflectDescriptorBinding* binding : bindings) {
        if (binding->descriptor_type != SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) continue;
        auto structDesc = binding->type_description;
        if (isIn(structDesc->type_name, prohibitedStructs)) {
            std::cout << "Multi-declare filtered for desc struct " << structDesc->type_name << std::endl;
            continue;
        }

        ssBuilder << "struct " << structDesc->type_name << " {\n";
        for (size_t m = 0; m < structDesc->member_count; ++m) {
            auto* member = &structDesc->members[m];
            ssBuilder << "\t" << GetTypeAsString(member) << " " << member->struct_member_name;
            if (member->traits.array.dims_count == 1) ssBuilder << "[" << member->traits.array.dims[0] << "]";
            ssBuilder << ";\n";
        }
        ssBuilder << "};\n";
    }
    return ssBuilder.str();
}

std::string
WriteInstanceInputs(const std::vector<SpvReflectInterfaceVariable *> &inputVars, const std::string &postfix) {
    uint32_t binding = 1;
    std::ostringstream ssBuilder;
    std::vector<std::pair<uint32_t, SpvReflectInterfaceVariable*>> vertexInputs;
    for (auto inVar : inputVars) {
        std::string inName(inVar->name);
        if (inName[inName.size() - 1] != 'i' || inName[inName.size() - 2] != '_') continue;
        vertexInputs.emplace_back(inVar->location, inVar);
    }
    std::sort(vertexInputs.begin(), vertexInputs.end());
    std::string structName(postfix + "Instance");
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
    return ssBuilder.str();
}

std::string WriteVertexInputs(const std::vector<SpvReflectInterfaceVariable *> &inputVars, const std::string &postfix) {
    uint32_t binding = 0;
    std::ostringstream ssBuilder;
    std::vector<std::pair<uint32_t, SpvReflectInterfaceVariable*>> vertexInputs;
    for (auto inVar : inputVars) {
        std::string inName(inVar->name);
        if (inName[inName.size() - 1] == 'i' && inName[inName.size() - 2] == '_') continue;
        vertexInputs.emplace_back(inVar->location, inVar);
    }
    std::sort(vertexInputs.begin(), vertexInputs.end());
    std::string structName(postfix + "Vertex");
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

std::string GetDescriptorTypeAsString(SpvReflectDescriptorType descType) {
    switch (descType) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:                    return "VK_DESCRIPTOR_TYPE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:     return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:              return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:              return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:       return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:       return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:             return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:     return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:     return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:           return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
        default: return "Unknown descriptor type";
    }
}
