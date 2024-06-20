//
// Created by idemaj on 6/20/24.
//

#ifndef SHADER_METAGEN_MAIN_H
#define SHADER_METAGEN_MAIN_H

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <bitset>
#include <sstream>
#include <memory>
#include "main.h"

#include "SPIRV-Reflect/spirv_reflect.h"

struct StageDescriptor {
    std::string filename;
    SpvReflectShaderStageFlagBits stageType;
};

struct PipelineConfig {
    uint32_t globalDescSetID;
    std::string pipelineName;
    std::vector<StageDescriptor> stages;
    std::vector<std::string> layoutFlags; // i.e. "VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT"
};

struct GlobalDescriptorSet {
    std::string name;
    uint32_t globalDescSetID;
    SpvReflectDescriptorSet* descSet;
};

// BASELINE BOILERPLATE
std::string WriteFromFile(const std::string& inFilename);
std::string WriteBaselineTypeDescriptions();
std::string WriteDescSetLayoutBoilerplate();

// WRITE AND PARSING INDIVIDUAL MODULES
void reflectInputVariables(const std::vector<SpvReflectInterfaceVariable *>& inputVars);
void reflectDescriptorSets(const std::string& pipelineName, const std::vector<SpvReflectDescriptorSet*>& sets);

std::string GetTypeAsString(SpvReflectInterfaceVariable* inVar);
std::string GetTypeAsString(SpvReflectTypeDescription* typeDesc);
std::string GetFormatAsString(SpvReflectFormat format);
std::string GetDescriptorTypeAsString(SpvReflectDescriptorType descType);
std::string WriteVertexInputs(const std::vector<SpvReflectInterfaceVariable *>& inputVars);
std::string WriteInstanceInputs(const std::vector<SpvReflectInterfaceVariable *>& inputVars);
std::string WriteUsedStructsInDescSet(const std::vector<SpvReflectDescriptorBinding*>& bindings);
std::string WriteDescSetLayout(const std::vector<SpvReflectDescriptorBinding*>& bindings, const std::string& setName= "DEFAULT_NAME");
std::string WriteDescSetLayoutManager(const std::vector<std::pair<uint32_t, std::string>>& regDescSets,
                                      const std::vector<SpvReflectDescriptorSet *> &sets);

// PIPELINES
bool Equals(SpvReflectTypeDescription* a, SpvReflectTypeDescription* b);
bool Equals(SpvReflectImageTraits& a, SpvReflectImageTraits& b);
bool Equals(SpvReflectDescriptorBinding* a, SpvReflectDescriptorBinding* b);
bool Equals(SpvReflectDescriptorSet * a, SpvReflectDescriptorSet * b);

bool CouldBeUnioned(SpvReflectDescriptorSet *a, SpvReflectDescriptorSet *b);
SpvReflectDescriptorSet* Union(SpvReflectDescriptorSet* a, SpvReflectDescriptorSet* b);

uint32_t CountMaxBinding(SpvReflectDescriptorSet* a);
SpvReflectDescriptorBinding* GetBindingOrNull(SpvReflectDescriptorSet* a, uint32_t binding);
void FreeUnionDescSet(SpvReflectDescriptorSet* a);


// MODULES
std::vector<std::pair<std::string, SpvReflectShaderModule *>>
CreateAllReflectModules(const std::vector<PipelineConfig>& pipelines);
SpvReflectShaderModule* MakeShaderModule(const std::string& filename);
SpvReflectShaderModule* GetModule(const std::vector<std::pair<std::string, SpvReflectShaderModule *>>& modules, const std::string& key);
void FreeReflectModules(std::vector<std::pair<std::string, SpvReflectShaderModule *>>& modules);


// WORKING
#define GLOBAL_DESCSET_INDEX 0
#define MAX_DESCRIPTOR_SETS 4
std::vector<std::array<SpvReflectDescriptorSet *, MAX_DESCRIPTOR_SETS>>
MergeModulesUnionDescriptorSetsByPipeline(const std::vector<PipelineConfig> &pipelines,
                                          const std::vector<std::pair<std::string, SpvReflectShaderModule *>> &modules);

void PopulateGlobalDescriptorLayouts(const std::vector<std::pair<uint32_t, SpvReflectDescriptorSet*>> &pipelineDescSetsAtGlobal,
                                     std::vector<GlobalDescriptorSet>& INOUT_globalDescSets);





#endif //SHADER_METAGEN_MAIN_H
