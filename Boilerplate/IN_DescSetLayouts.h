

/* For indexing thru a template parameter list*/
template <typename T, typename... Types>
struct type_index;
template <typename T, typename... Rest>
struct type_index<T, T, Rest...> {
    static constexpr std::size_t value = 0;
};
template <typename T, typename First, typename... Rest>
struct type_index<T, First, Rest...> {
    static constexpr std::size_t value = 1 + type_index<T, Rest...>::value;
};

/**
 *
 */
struct Descriptor {
    const char* name;
    VkDescriptorType type;
    uint32_t count;
    uint32_t byteSize;
    VkShaderStageFlags stages = VK_SHADER_STAGE_ALL_GRAPHICS;
};

struct DescriptorCounts {
    uint32_t numSamplers;
    uint32_t numBuffers;
};



class Root_DescriptorSet {
public:
    virtual ~Root_DescriptorSet() = default;
    virtual uint32_t GetDescriptorNameCount() = 0;
    virtual uint32_t GetTotalDescriptorCount() = 0;
    virtual const char* GetName() = 0;
    virtual VkDescriptorSetLayoutCreateFlags GetLayoutFlags() = 0;
    virtual VkDescriptorSetLayout GetDefaultDescriptorSetLayout() = 0;
    virtual uint32_t GetCountOfDescriptorsWithType(VkDescriptorType) = 0;
};

template <int N>
class Base_DescriptorSet : public Root_DescriptorSet {
protected:
    VkDevice m_device{};
    std::array<Descriptor, N> m_descriptors{};
    VkDescriptorSetLayout m_setLayout{};
public:
    explicit Base_DescriptorSet(VkDevice device) : m_device(device) {}
    ~Base_DescriptorSet() override  {
        if (m_setLayout) {
            vkDestroyDescriptorSetLayout(m_device, m_setLayout, nullptr);
        }
    }
    uint32_t GetDescriptorNameCount() final { return N; }
    uint32_t GetTotalDescriptorCount() final { return N; }
    VkDescriptorSetLayout GetDefaultDescriptorSetLayout() final { return m_setLayout; }
    /** SHOULD BE CALLED BY THE CONSTRUCTOR OF ALL IMPLEMENTING DESCRIPTOR SETS */
    void MakeDescriptorSetLayout(VkDevice device, VkDescriptorSetLayoutCreateFlags layoutFlags)  {
        std::array<VkDescriptorSetLayoutBinding, N> setBindings{};
        for (uint32_t i = 0; i < N; i++) {
            const Descriptor& desc = m_descriptors[i];
            setBindings[i] = VkDescriptorSetLayoutBinding{
                    .binding = i,
                    .descriptorType = desc.type,
                    .descriptorCount = desc.count,
                    .stageFlags = desc.stages,
                    .pImmutableSamplers = nullptr,
            };
        }
        VkDescriptorSetLayoutCreateInfo createInfo {
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                .flags = 0, // TODO : pending the VK_EXT_descriptor_indexing
                .bindingCount = N,
                .pBindings = setBindings.data(),

        };
        vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &m_setLayout);
    }
    uint32_t GetCountOfDescriptorsWithType(VkDescriptorType descriptorType) final {
        uint32_t count = 0;
        for (const Descriptor& desc : m_descriptors)
            if (desc.type == descriptorType)
                count += desc.count;
        return count;
    }
};


template<typename... Types>
concept AllDerivedFromRoot = (std::is_base_of_v<Root_DescriptorSet, Types> && ...);

template<typename... T>
class TypedDescriptorSetManager {
    static_assert(AllDerivedFromRoot<T...>, "All types must inherit from Base_DescriptorSet");
public:
    std::array<Root_DescriptorSet*, sizeof...(T)> DescriptorSets;

    explicit TypedDescriptorSetManager(VkDevice device) {
        int index = 0;
        (..., (DescriptorSets[index++] = reinterpret_cast<Root_DescriptorSet*>(new T(device))));
    }
    ~TypedDescriptorSetManager() {
        for (Root_DescriptorSet* descSet : DescriptorSets) {
            delete descSet;
        }
    }

    template <typename... DescType>
    std::array<VkDescriptorSetLayout, sizeof...(DescType)> GetDescriptorLayouts() {
        std::array<VkDescriptorSetLayout, sizeof...(DescType)> selectedSets = {
                DescriptorSets[type_index<DescType, T...>::value]->GetCountOfDescriptorsWithType()...
        };
        return selectedSets;
    }
    template <typename... DescType>
    DescriptorCounts GetDescriptorCounts() {
        DescriptorCounts counts{};
        counts.numBuffers = (DescriptorSets[type_index<DescType, T...>::value]->GetCountOfDescriptorsWithType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) + ...);
        counts.numSamplers = (DescriptorSets[type_index<DescType, T...>::value]->GetCountOfDescriptorsWithType(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) + ...);
        return counts;
    }

};

