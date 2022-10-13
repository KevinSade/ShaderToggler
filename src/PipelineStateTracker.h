#pragma once

#include <reshade.hpp>
#include <vector>
#include <unordered_map>
#include <shared_mutex>

using namespace reshade::api;
using namespace std;

namespace StateTracker
{
    enum PipelineBindingTypes : uint32_t
    {
        unknown = 0,
        bind_pipeline,
        bind_render_target,
        bind_viewport,
        bind_scissor_rect,
        bind_descriptors,
        bind_pipeline_states,
        push_constants,
        render_pass
    };

    struct PipelineBindingBase
    {
    public:
        command_list* cmd_list = nullptr;
        uint32_t callIndex = 0;
        virtual PipelineBindingTypes GetType() { return PipelineBindingTypes::unknown; }
    };

    template<PipelineBindingTypes T>
    struct PipelineBinding : PipelineBindingBase
    {
    public:
        PipelineBindingTypes GetType() override { return T; }
    };

    struct BindRenderTargetsState : PipelineBinding<PipelineBindingTypes::bind_render_target> {
        uint32_t count;
        vector<resource_view> rtvs;
        resource_view dsv;

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            rtvs.clear();
            dsv = { 0 };
            count = 0;
        }
    };

    struct RenderPassState : PipelineBinding<PipelineBindingTypes::render_pass> {
        uint32_t count;
        vector<render_pass_render_target_desc> rtvs;
        render_pass_depth_stencil_desc dsv;

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            rtvs.clear();
            dsv = { 0 };
            count = 0;
        }
    };

    struct BindViewportsState : PipelineBinding<PipelineBindingTypes::bind_viewport> {
        uint32_t first;
        uint32_t count;
        vector<viewport> viewports;

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            first = 0;
            count = 0;
            viewports.clear();
        }
    };

    struct BindScissorRectsState : PipelineBinding<PipelineBindingTypes::bind_scissor_rect> {
        uint32_t first;
        uint32_t count;
        vector<rect> rects;

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            first = 0;
            count = 0;
            rects.clear();
        }
    };

    struct PushConstantsState : PipelineBinding<PipelineBindingTypes::push_constants> {
        uint32_t layout_param;
        uint32_t first;
        uint32_t count;
        vector<uint32_t> values;

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            layout_param = 0;
            first = 0;
            count = 0;
            values.clear();
        }
    };

    struct BindDescriptorsState : PipelineBinding<PipelineBindingTypes::bind_descriptors> {
        pipeline_layout current_layout[2];
        vector<descriptor_set> current_sets[2];
        unordered_map<uint64_t, vector<bool>> transient_mask;

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            current_layout[0] = { 0 };
            current_layout[1] = { 0 };
            current_sets[0].clear();
            current_sets[1].clear();
            transient_mask.clear();
        }
    };

    struct BindPipelineStatesState : PipelineBinding<PipelineBindingTypes::bind_pipeline_states> {
        uint32_t value;
        bool valuesSet;
        dynamic_state state;

        BindPipelineStatesState(dynamic_state s)
        {
            state = s;
            Reset();
        }

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            value = 0;
            valuesSet = false;
        }
    };

    struct BindPipelineStatesStates {
        BindPipelineStatesState states[2] = { BindPipelineStatesState(dynamic_state::blend_constant), BindPipelineStatesState(dynamic_state::primitive_topology) };

        void Reset()
        {
            states[0].Reset();
            states[1].Reset();
        }
    };

    struct BindPipelineState : PipelineBinding<PipelineBindingTypes::bind_pipeline> {
        pipeline_stage stages;
        pipeline pipeline;

        void Reset()
        {
            callIndex = 0;
            cmd_list = nullptr;
            stages = pipeline_stage::all;
            pipeline = { 0 };
        }
    };

    class PipelineStateTracker
    {
    public:
        PipelineStateTracker();
        ~PipelineStateTracker();

        void Reset();
        void ReApplyState(command_list* cmd_list, const unordered_map<uint64_t, vector<bool>>& transient_mask);

        void OnBeginRenderPass(command_list* cmd_list, uint32_t count, const render_pass_render_target_desc* rts, const render_pass_depth_stencil_desc* ds);
        void OnBindRenderTargetsAndDepthStencil(command_list* cmd_list, uint32_t count, const resource_view* rtvs, resource_view dsv);
        void OnBindPipelineStates(command_list* cmd_list, uint32_t count, const dynamic_state* states, const uint32_t* values);
        void OnBindScissorRects(command_list* cmd_list, uint32_t first, uint32_t count, const rect* rects);
        void OnBindViewports(command_list* cmd_list, uint32_t first, uint32_t count, const viewport* viewports);
        void OnBindDescriptorSets(command_list* cmd_list, shader_stage stages, pipeline_layout layout, uint32_t first, uint32_t count, const descriptor_set* sets);
        void OnBindPipeline(command_list* commandList, pipeline_stage stages, pipeline pipelineHandle);

        bool IsInRenderPass();

    private:
        void ApplyBoundDescriptorSets(command_list* cmd_list, shader_stage stage, pipeline_layout layout,
            const vector<descriptor_set>& descriptors, const vector<bool>& mask);

        uint32_t _callIndex = 0;
        BindRenderTargetsState _renderTargetState;
        BindDescriptorsState _descriptorsState;
        BindViewportsState _viewportsState;
        BindScissorRectsState _scissorRectsState;
        BindPipelineStatesStates _pipelineStatesState;
        RenderPassState _renderPassState;
        BindPipelineState _pipelineState;
    };
}