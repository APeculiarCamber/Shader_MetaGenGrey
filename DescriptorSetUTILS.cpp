//
// Created by idemaj on 6/20/24.
//
#include "main.h"


bool Equals(SpvReflectTypeDescription *a, SpvReflectTypeDescription *b) {
    if (a->type_flags != b->type_flags) return false;
    if (strcmp(a->type_name, b->type_name) != 0) return false; // TODO: we only compare the struct name here, so this isn't perfect
    if (a->member_count != b->member_count) return false;
    return true;
}

bool Equals(SpvReflectImageTraits &a, SpvReflectImageTraits &b) {
    return a.image_format == b.image_format
           && a.sampled == b.sampled
           && a.ms == b.ms;
}

bool Equals(SpvReflectDescriptorBinding *a, SpvReflectDescriptorBinding *b) {
    if (a->count != b->count) return false;
    if (a->binding != b->binding) return false;
    if (a->resource_type != b->resource_type) return false;
    if (a->descriptor_type != b->descriptor_type) return false;
    if (a->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
        return Equals(a->type_description, b->type_description);
    } else return Equals(a->image, b->image);
}

bool Equals(SpvReflectDescriptorSet *a, SpvReflectDescriptorSet *b) {
    if (a->binding_count != b->binding_count) return false;
    for (size_t i = 0; i < a->binding_count; ++i) {
        if (!Equals(a->bindings[i], b->bindings[i])) return false;
    }
    return true;
}

bool CouldBeUnioned(SpvReflectDescriptorSet *a, SpvReflectDescriptorSet *b) {
    auto unionedSet = Union(a, b);
    if (unionedSet == nullptr) return false;
    delete[] unionedSet->bindings;
    delete unionedSet;
    return true;
}


SpvReflectDescriptorSet *Union(SpvReflectDescriptorSet *a, SpvReflectDescriptorSet *b) {
    if (a->set != b->set) return nullptr;
    std::vector<SpvReflectDescriptorBinding*> aBindings(a->bindings, a->bindings + a->binding_count);
    std::vector<SpvReflectDescriptorBinding*> bBindings(b->bindings, b->bindings + b->binding_count);

    uint32_t aMaxBinding = CountMaxBinding(a);
    uint32_t bMaxBinding = CountMaxBinding(b);
    uint32_t newBindingCount = std::max(aMaxBinding, bMaxBinding) + 1;
    auto* unionSet = new SpvReflectDescriptorSet{  .set = a->set,  .binding_count = newBindingCount,
            .bindings = new SpvReflectDescriptorBinding*[newBindingCount], };

    for (uint32_t i = 0; i < newBindingCount; ++i) {
        auto aBinding = GetBindingOrNull(a, i);
        auto bBinding = GetBindingOrNull(b, i);
        if (aBinding && bBinding) {
            if (!Equals(aBinding, bBinding)) return nullptr; // Cannot union if the bindings are off
            unionSet->bindings[i] = aBinding;
        }
        else if (aBinding) unionSet->bindings[i] = aBinding;
        else if (bBinding) unionSet->bindings[i] = bBinding;
        else unionSet->bindings[i] = nullptr;
    }
    return unionSet;
}

uint32_t CountMaxBinding(SpvReflectDescriptorSet *a) {
    uint32_t maxBinding = 0;
    for (uint32_t i = 0; i < a->binding_count; ++i) {
        maxBinding = std::max(maxBinding, a->bindings[i]->binding);
    }
    return maxBinding;
}


SpvReflectDescriptorBinding *GetBindingOrNull(SpvReflectDescriptorSet *a, uint32_t binding) {
    for (uint32_t i = 0; i < a->binding_count; ++i) {
        if (a->bindings[i]->binding == binding) return a->bindings[i];
    }
    return nullptr;
}

void FreeUnionDescSet(SpvReflectDescriptorSet *a) {
    if (a) {
        delete[] a->bindings;
        delete a;
    }
}
