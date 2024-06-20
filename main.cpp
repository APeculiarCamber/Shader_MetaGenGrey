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
#include <memory>
#include "main.h"

#include "SPIRV-Reflect/spirv_reflect.h"


void ExampleParseSingleModule(const std::string& filename="test_shader_vert.spv") {
    std::string input_spv_path = SHADER_DIR + filename;

    std::ifstream spv_ifstream(input_spv_path.c_str(), std::ios::binary);
    if (!spv_ifstream.is_open()) {
        std::cerr << "ERROR: could not open '" << input_spv_path << "' for reading\n";
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
    std::vector<SpvReflectDescriptorSet *> sets(count);
    result = spvReflectEnumerateDescriptorSets(&module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::cout << "Descriptor sets " << sets.size() << std::endl;
    for (auto &set: sets) {
        std::cout << set->binding_count << ":";
        for (int i = 0; i < set->binding_count; ++i) {
            const auto v = SpvReflectTypeFlagBits::SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE;
            auto d = SpvReflectDecorationFlagBits::SPV_REFLECT_DECORATION_PER_VERTEX;
            SpvReflectDescriptorType f;
            std::cout << (set->bindings[i]->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                          ? "ImageSampler" : set->bindings[i]->type_description->type_name) << " "
                      << set->bindings[i]->name << ",  ";
        }
        std::cout << "\n";
    }

    result = spvReflectEnumerateDescriptorBindings(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorBinding *> bindings(count);
    result = spvReflectEnumerateDescriptorBindings(&module, &count, bindings.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumerateInterfaceVariables(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable *> interface_variables(count);
    result = spvReflectEnumerateInterfaceVariables(&module, &count, interface_variables.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumerateInputVariables(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable *> input_variables(count);
    result = spvReflectEnumerateInputVariables(&module, &count, input_variables.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    result = spvReflectEnumerateOutputVariables(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectInterfaceVariable *> output_variables(count);
    result = spvReflectEnumerateOutputVariables(&module, &count, output_variables.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    result = spvReflectEnumeratePushConstantBlocks(&module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectBlockVariable *> push_constant(count);
    result = spvReflectEnumeratePushConstantBlocks(&module, &count, push_constant.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    reflectInputVariables(input_variables);

    reflectDescriptorSets("TestExample", sets);

    spvReflectDestroyShaderModule(&module);

}

void PerformShaderGen(const std::vector<GlobalDescriptorSet>& globalSetConfigs, const std::vector<PipelineConfig>& configs) {
    auto globalSets = globalSetConfigs;
    auto pipelineConfigs = configs;

    // Step 1, get all the reflection modules
    std::vector<std::pair<std::string, SpvReflectShaderModule*>> modules = CreateAllReflectModules(configs);
    std::cout << "For this, we using at least: " << sizeof(SpvReflectShaderModule) * 2 * modules.size() << " bytes" << std::endl;

    // Step 1.5, union all the pipelines. NOTE: SAME ORDER AS THE PIPELINES, could change to explicitly mark iff needed
    std::vector<std::array<SpvReflectDescriptorSet*, MAX_DESCRIPTOR_SETS>> mergedSets = MergeModulesUnionDescriptorSetsByPipeline(configs,
                                                                                                                modules);
    // Step 2, build and generate global descriptor sets
    std::vector<std::pair<uint32_t, SpvReflectDescriptorSet*>> globalPartialSetsPerPipeline(configs.size());
    for (uint32_t i = 0; i < configs.size(); i++)
        globalPartialSetsPerPipeline[i] = { configs[i].globalDescSetID, mergedSets[i][GLOBAL_DESCSET_INDEX] };
    PopulateGlobalDescriptorLayouts(globalPartialSetsPerPipeline, globalSets);

    // Step 3, build and generate inputs for VERTEX shaders, remember to opt out compute shaders
    GenerateInputVariableFile(configs, modules, "InputData.h");

    // Step 4, build and generate descriptor sets for global descriptor sets
    auto generatedStructs =
    GenerateGlobalDescriptorSetsFile(globalSets, globalPartialSetsPerPipeline, "GlobalDescSetLayoutData.h");

    // Step 4.5, populate the global desc names of the pipeline config objects
    for (auto & pc : pipelineConfigs) {
        uint32_t gid = pc.globalDescSetID;
        pc.descSetManagerNames[GLOBAL_DESCSET_INDEX] = std::find_if(globalSets.begin(), globalSets.end(),
                               [gid](const GlobalDescriptorSet& g) { return g.globalDescSetID == gid;})
                                       ->managerName;
    }

    // Step 5, build and generate descriptor sets for per-pipeline (exclude the global index)
    auto generatedStructs2 =
    GenerateMaterialDescriptorSetsFile(pipelineConfigs, mergedSets, "MaterialDescSetLayoutData.h");


    // STEP !!! the material guts...


    // ALLOC CLEANUP, not pretty... TODO: need some RAII in this bitch
    std::cout << mergedSets.size() << " made from " << configs.size() << std::endl;
    for (auto darr : mergedSets) for (auto d : darr) {
            if (d) std::cout << "Made desc set #" << d->set << " for " << d->binding_count << " bindings" << std::endl;
            else std::cout << "No desc set for #X" << std::endl;
            FreeUnionDescSet(d);
    }
    for (const auto& globalDesc : globalSets) {
        std::cout << "Made the global desc " << globalDesc.name << " (ID=" << globalDesc.globalDescSetID << ") "
            << "with " << globalDesc.descSet->binding_count << " bindings" << std::endl;
        FreeUnionDescSet(globalDesc.descSet);
    }
    FreeReflectModules(modules);

}


std::vector<std::string>  GenerateMaterialDescriptorSetsFile(std::vector<PipelineConfig> &configs,
                                        const std::vector<std::array<SpvReflectDescriptorSet *, 4>> &unionedDescSets,
                                        const std::string& filename) {
    assert(configs.size() == unionedDescSets.size());

    std::ofstream outFile(std::string(OUT_DIR) + filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
    }

    std::string boilerInputFilename = "IN_InputData.h";
    std::string boilerDescSetFilename = "IN_DescSetLayoutHeader.h";
    outFile << "#include \"" << boilerInputFilename << "\"\n";
    outFile << "#include \"" << boilerDescSetFilename << "\"\n\n";


    // TODO: makes the grave assumption that structs with identical names will have identical members, this isn't necessarily true AND can be checked in future
    outFile << "/********************************************************************************************\n";
    outFile << "****************************     " << "STRUCTS!!!!" << "     ******************************\n";
    outFile << "*********************************************************************************************/\n\n";
    std::vector<std::string> declaredStructs;
    for (const auto& pSets : unionedDescSets) {
        for (const auto* set : pSets) {
            if (set == nullptr || set->set == GLOBAL_DESCSET_INDEX) continue;
            std::vector<SpvReflectDescriptorBinding *> bindings(set->bindings, set->bindings + set->binding_count);
            outFile << WriteUsedStructsInDescSet(bindings, declaredStructs) << "\n\n";
            for (SpvReflectDescriptorBinding *binding: bindings) {
                if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                    declaredStructs.emplace_back(binding->type_description->type_name);
            }
        }
    }

    std::vector<std::string> postfixBySetID { "GLOBAL", "MAT", "LOCAL", "UNKNOWN" };
    std::vector<std::vector<SpvReflectDescriptorBinding*>> bindingsToSet_forDebug;
    for (uint32_t i = 0; i < configs.size(); ++i) {
        auto& p = configs[i];
        const auto& pSets = unionedDescSets[i];
        for (SpvReflectDescriptorSet* set: pSets) {
            if (set == nullptr || set->set == GLOBAL_DESCSET_INDEX) continue;
            std::string setName = configs[i].pipelineName + "_" + postfixBySetID[set->set];

            outFile << "\n\n\n/********************************************************************************************\n";
            outFile << "****************************     " << setName << "     ******************************\n";
            outFile << "*********************************************************************************************/\n\n\n";

            std::vector<SpvReflectDescriptorBinding *> setBindings(set->bindings, set->bindings + set->binding_count);
            outFile << WriteDescSetLayout(setBindings, setName) << "\n\n";

            p.descSetManagerNames[set->set] = setName + "_IMPL";
            outFile << "typedef " << setName << "_DescriptorSet<";
            uint32_t numBindings = set->binding_count;
            for (uint32_t i = 0; i < numBindings; ++i) {
                outFile << "VK_SHADER_STAGE_ALL_GRAPHICS, ";
            }
            outFile << "VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT> " << p.descSetManagerNames[set->set] << ";\n";
        }
    }
    return declaredStructs;
}

/**
 *
 * @param globalConfigs MODIFIES, WRITES TO managerName WHICH WILL BE THE NAME OF THE DECLARED MANAGER TYPE using typedef
 * @param reflectedGlobalDescSets
 * @param filename
 * @return The names of all structs generated by this function and put into the file. There are likely to be duplicates.
 */
std::vector<std::string> GenerateGlobalDescriptorSetsFile(std::vector<GlobalDescriptorSet> &globalConfigs,
                                      const std::vector<std::pair<uint32_t, SpvReflectDescriptorSet *>> &reflectedGlobalDescSets,
                                      const std::string& filename) {
    std::ofstream outFile(std::string(OUT_DIR) + filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
    }

    std::string boilerInputFilename = "IN_InputData.h";
    std::string boilerDescSetFilename = "IN_DescSetLayoutHeader.h";
    outFile << "#include <vulkan/vulkan.h>\n";
    outFile << "#include <array>\n\n";
    outFile << "#include \"" << boilerInputFilename << "\"\n";
    outFile << "#include \"" << boilerDescSetFilename << "\"\n\n";


    // TODO: makes the grave assumption that structs with identical names will have identical members, this isn't necessarily true AND can be checked in future
    outFile << "/********************************************************************************************\n";
    outFile << "****************************     " << "STRUCTS!!!!" << "     ******************************\n";
    outFile << "*********************************************************************************************/\n\n";
    std::vector<std::string> declaredStructs;
    for (auto [globalID, set] : reflectedGlobalDescSets) {
        std::vector<SpvReflectDescriptorBinding*> bindings(set->bindings, set->bindings + set->binding_count);
        outFile << WriteUsedStructsInDescSet(bindings, declaredStructs) << "\n\n";
        for (SpvReflectDescriptorBinding* binding : bindings) {
            if (binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
                declaredStructs.emplace_back(binding->type_description->type_name);
        }
    }

    std::vector<std::vector<SpvReflectDescriptorBinding*>> bindingsToSet_forDebug;
    for (auto [globalID, set] : reflectedGlobalDescSets) {
        auto it = std::find_if(globalConfigs.begin(), globalConfigs.end(),
                   [globalID](const GlobalDescriptorSet& d) { return d.globalDescSetID == globalID; });
        std::string setName = it->name;

        outFile << "\n\n\n/********************************************************************************************\n";
        outFile << "****************************     " << setName << "     ******************************\n";
        outFile << "*********************************************************************************************/\n\n\n";

        std::vector<SpvReflectDescriptorBinding *> setBindings(set->bindings, set->bindings + set->binding_count);
        outFile << WriteDescSetLayout(setBindings, setName) << "\n\n";

        it->managerName = it->name + "_IMPL";
        outFile << "typedef " << setName << "_DescriptorSet<";
        uint32_t numBindings = it->descSet->binding_count;
        for (uint32_t i = 0; i < numBindings; ++i) {
            outFile << "VK_SHADER_STAGE_ALL_GRAPHICS, ";
        }
        outFile << "VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT> " << it->managerName << ";\n";
    }
    return declaredStructs;
}

/**
 * Get and return the module for the pipeline that can take input variables.
 * Assumes this is always SPV_REFLECT_SHADER_STAGE_VERTEX_BIT.
 */
SpvReflectShaderModule* GetInputModule(const PipelineConfig& config,
                                       std::vector<std::pair<std::string, SpvReflectShaderModule *>> &modules) {
    auto it = std::find_if(config.stages.begin(), config.stages.end(),
                 [](const StageDescriptor& stage) { return stage.stageType == SPV_REFLECT_SHADER_STAGE_VERTEX_BIT; });

    if (it == config.stages.end()) return nullptr;
    else return GetModule(modules, it->filename);
}

void GenerateInputVariableFile(const std::vector<PipelineConfig> &configs,
                               std::vector<std::pair<std::string, SpvReflectShaderModule *>> &modules,
                               const std::string& filename) {
    std::ofstream outFile(std::string(OUT_DIR) + filename);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
    }

    std::string boilerInputFilename = "IN_InputData.h";
    outFile << "#include <vulkan/vulkan.h>\n";
    outFile << "#include <array>\n";
    outFile << "#include \"" << boilerInputFilename << "\"\n\n";

    for (const auto& p : configs) {
        SpvReflectShaderModule* inModule = GetInputModule(p, modules);
        if (inModule) {
            uint32_t count;
            auto result = spvReflectEnumerateInputVariables(inModule, &count, NULL);
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
            std::vector<SpvReflectInterfaceVariable *> inputVars(count);
            result = spvReflectEnumerateInputVariables(inModule, &count, inputVars.data());
            assert(result == SPV_REFLECT_RESULT_SUCCESS);

            // Structs will be called <p.pipelineName>Instance and <p.pipelineName>Vertex
            auto vertInputs = WriteVertexInputs(inputVars, p.pipelineName);
            auto instanceInputs = WriteInstanceInputs(inputVars, p.pipelineName);

            outFile << "\n\n\n/********************************************************************************************\n";
            outFile << "****************************     " << p.pipelineName << "     ******************************\n";
            outFile << "*********************************************************************************************/\n\n\n";

            outFile << vertInputs;
            outFile << "\n\n/********************************************************************************************/\n\n";
            outFile << instanceInputs;
        }
    }


}

// =================================================================================================
// main()
// =================================================================================================
int main(int argn, char** argv) {
    ExampleParseSingleModule();
    std::cout << "done" << std::endl;
    GlobalDescriptorSet globalDescSet1 { .name = "AJohnnyTime", .globalDescSetID = 0, };
    GlobalDescriptorSet globalDescSet2 { .name = "AJillyTime", .globalDescSetID = 1, };

    PipelineConfig pipelineConfig1{.globalDescSetID = 0, .pipelineName = "RedDead1", .stages = {
            StageDescriptor{std::string("test_shader_vert.spv"), SPV_REFLECT_SHADER_STAGE_VERTEX_BIT},
            StageDescriptor{std::string("test_shader_frag.spv"), SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT}, }
    };
    PipelineConfig pipelineConfig2 { .globalDescSetID = 1, .pipelineName = "RedDead2", .stages = {
            StageDescriptor{std::string("test_shader_split_vert.spv"), SPV_REFLECT_SHADER_STAGE_VERTEX_BIT},
            StageDescriptor{std::string("test_shader_split_frag.spv"), SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT}, }
    };
    PerformShaderGen({globalDescSet1, globalDescSet2}, { pipelineConfig2, pipelineConfig1 });
    return 0;
}























SpvReflectDescriptorSet *GetDescSetOf(uint32_t id, SpvReflectShaderModule *module) {
    uint32_t count = 0;
    auto result = spvReflectEnumerateDescriptorSets(module, &count, NULL);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorSet *> sets(count);
    result = spvReflectEnumerateDescriptorSets(module, &count, sets.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    auto it = std::find_if(sets.begin(), sets.end(),
                           [id](auto set) { return set->set == id; });
    if (it != sets.end()) return *it;
    else return nullptr;
}

std::vector<std::array<SpvReflectDescriptorSet *, MAX_DESCRIPTOR_SETS>>
MergeModulesUnionDescriptorSetsByPipeline(const std::vector<PipelineConfig> &pipelines,
                                          const std::vector<std::pair<std::string, SpvReflectShaderModule *>> &modules) {
    std::vector<std::array<SpvReflectDescriptorSet *, MAX_DESCRIPTOR_SETS>> output;
    // For each pipeline
    for (const auto& p : pipelines) {
        std::vector<SpvReflectShaderModule *> pModules(p.stages.size());
        std::transform(p.stages.begin(), p.stages.end(), pModules.begin(),[modules] (auto& desc)
        { return GetModule(modules, desc.filename); });
        output.emplace_back();
        // For each descriptor set index, merge all the declarations between stages
        for (uint32_t i = 0; i < MAX_DESCRIPTOR_SETS; ++i) {
            output.back()[i] = nullptr;
            std::vector<SpvReflectDescriptorSet*> descSetsPerStage(p.stages.size());
            std::transform(pModules.begin(), pModules.end(), descSetsPerStage.begin(),[i] (auto mod)
            { return GetDescSetOf(i, mod); });

            // NOTE: we do this weird little thing to preserve the possibility of a null set, and to keep the functional expectation of a Union-Alloced set (meaning set and bindings allocated)
            for (auto set : descSetsPerStage) {
                if (set == nullptr) continue;
                if (output.back()[i] == nullptr) {
                    output.back()[i] = new SpvReflectDescriptorSet{ set->set, set->binding_count, new SpvReflectDescriptorBinding *[set->binding_count] };
                    for (size_t b = 0; b < set->binding_count; b++) output.back()[i]->bindings[b] = set->bindings[b];
                } else {
                    auto* unionedDestSet = Union(output.back()[i], set);
                    FreeUnionDescSet(output.back()[i]);
                    output.back()[i] = unionedDestSet;
                }
            }
        }
    }
    return output;
}

/**
 *
 * @param pipelineDescSetsAtGlobal List of <GLOBAL DESC SET IDs, Desc Set Partial for the pipeline (union of all stages)>
 * @param INOUT_globalDescSets
 */
void PopulateGlobalDescriptorLayouts(const std::vector<std::pair<uint32_t, SpvReflectDescriptorSet*>> &pipelineDescSetsAtGlobal,
                                     std::vector<GlobalDescriptorSet>& INOUT_globalDescSets) {
    auto configs = pipelineDescSetsAtGlobal;
    std::sort(configs.begin(), configs.end(), [](const auto& a, const auto& b)
    { return a.first < b.first; });
    for (auto& inout_descSet : INOUT_globalDescSets) {
        uint32_t id = inout_descSet.globalDescSetID;
        inout_descSet.descSet = new SpvReflectDescriptorSet{ 0, 0, nullptr};
        auto itr = std::find_if(configs.begin(), configs.end(),
                                [id](const auto& p) { return p.first == id; });
        while (itr != configs.end() && itr->first == id) {
            SpvReflectDescriptorSet* unionedSet = Union(inout_descSet.descSet, itr->second);
            FreeUnionDescSet(inout_descSet.descSet);
            inout_descSet.descSet = unionedSet;
            itr = std::next(itr);
        }
    }
}



std::vector<std::pair<std::string, SpvReflectShaderModule *>>
CreateAllReflectModules(const std::vector<PipelineConfig> &pipelines) {
    std::vector<std::pair<std::string, SpvReflectShaderModule *>> modules;
    for (const auto& p : pipelines) {
        for (const auto& descSet : p.stages) {
            if (GetModule(modules, descSet.filename) == nullptr) {
                modules.emplace_back(descSet.filename, MakeShaderModule(descSet.filename));
            }
        }
    }
    return modules;
}

SpvReflectShaderModule *MakeShaderModule(const std::string &filename) {
    std::string input_spv_path = SHADER_DIR + filename;

    std::ifstream spv_ifstream(input_spv_path.c_str(), std::ios::binary);
    if (!spv_ifstream.is_open()) {
        std::cerr << "ERROR: could not open '" << input_spv_path << "' for reading\n";
        abort();
    }

    spv_ifstream.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(spv_ifstream.tellg());
    spv_ifstream.seekg(0, std::ios::beg);

    std::vector<char> spv_data(size);
    spv_ifstream.read(spv_data.data(), size);
    spv_ifstream.close();

    SpvReflectShaderModule* module = new SpvReflectShaderModule{};
    SpvReflectResult result = spvReflectCreateShaderModule(spv_data.size(), spv_data.data(), module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    return module;
}

SpvReflectShaderModule *
GetModule(const std::vector<std::pair<std::string, SpvReflectShaderModule *>> &modules, const std::string &key) {
    auto it = std::find_if(modules.begin(), modules.end(),
                           [key](auto e) { return e.first == key; });
    if (it != modules.end()) return it->second;
    return nullptr;
}

void FreeReflectModules(std::vector<std::pair<std::string, SpvReflectShaderModule *>> &modules) {
    for (auto& [fn, module] : modules) {
        spvReflectDestroyShaderModule(module);
        delete module;
    }
}


void reflectPushConstants(const std::vector<SpvReflectBlockVariable*>& pushConstants) {
}