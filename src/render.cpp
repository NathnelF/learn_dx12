#include "headers.h"

void Render(State *state, int frame_index)
{
    // get current frame information
    Frame *frame = &state->context.frames[frame_index];

    // check if this frame is done being processed by gpu
    if (state->context.fence.handle->GetCompletedValue() < frame->fence_value)
    // if the global counter is less than the frame value then the frame is not
    // processed yet because the signal to increment the fence value has not
    // increased yet.
    {
        validate(state->context.fence.handle->SetEventOnCompletion(
                   frame->fence_value, state->context.fence.event),
                 "could not set fence event");
        // signal this event when we reach our target value

        WaitForSingleObject(state->context.fence.event, INFINITE);
        // sleepy time
    }
    validate(frame->allocator->Reset(), "could not reset command allocator");

    // reset the command list
    validate(
      frame->command_list->Reset(frame->allocator, state->pipeline.handle),
      "could not reset command list");

    // get the back buffer index
    u32 backbuffer_index =
      state->context.swapchain->GetCurrentBackBufferIndex();

    // transition back buffer to render target using barrier
    D3D12_RESOURCE_BARRIER barrier = {
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
        .Transition = {
        	.pResource = state->swapchain.buffers[backbuffer_index],
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET,
			},
    };

    frame->command_list->ResourceBarrier(1, &barrier);

    // clear the render target
    float clear_color[] = { 0.1f, 0.1f, 0.2f, 1.0f };
    frame->command_list->ClearRenderTargetView(
      state->swapchain.rtv_handles[backbuffer_index], clear_color, 0, NULL);

    // clear the depth buffer
    frame->command_list->ClearDepthStencilView(
      state->swapchain.dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);

    // set up viewport and scissor
    D3D12_VIEWPORT viewport = {
        .TopLeftX = 0.0f,
        .TopLeftY = 0.0f,
        .Width = (float)state->swapchain.width,
        .Height = (float)state->swapchain.height,
        .MinDepth = 0.0f,
        .MaxDepth = 1.0f,
    };

    D3D12_RECT scissor = {
        .left = 0,
        .top = 0,
        .right = (LONG)state->swapchain.width,
        .bottom = (LONG)state->swapchain.height,
    };

    frame->command_list->RSSetViewports(1, &viewport);
    frame->command_list->RSSetScissorRects(1, &scissor);

    // bind render target
    frame->command_list->OMSetRenderTargets(
      1,
      &state->swapchain.rtv_handles[backbuffer_index],
      FALSE,
      &state->swapchain.dsv_handle);

    // bind pipeline state
    frame->command_list->SetPipelineState(state->pipeline.handle);

    // bind root signature
    frame->command_list->SetGraphicsRootSignature(
      state->pipeline.root_signature);

    // set primitive topology
    frame->command_list->IASetPrimitiveTopology(
      D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // just index 0 for now;
    MeshInfo *mesh = &state->mesh_data.meshes[0];

    D3D12_VERTEX_BUFFER_VIEW vbv = {
        .BufferLocation =
          state->mesh_data.buffer->GetGPUVirtualAddress() + mesh->vertex_offset,
        .SizeInBytes = (u32)(mesh->vertex_count * sizeof(float) * 3),
        .StrideInBytes = sizeof(float) * 3,
    };

    frame->command_list->IASetVertexBuffers(0, 1, &vbv);

    D3D12_INDEX_BUFFER_VIEW ibv = {
        .BufferLocation =
          state->mesh_data.buffer->GetGPUVirtualAddress() + mesh->index_offset,
        .SizeInBytes = (u32)(mesh->index_count * sizeof(u32)),
        .Format = DXGI_FORMAT_R32_UINT,
    };

    frame->command_list->IASetIndexBuffer(&ibv);

    // draw command!
    frame->command_list->DrawIndexedInstanced(mesh->index_count, 1, 0, 0, 0);

    // transition from render to present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    frame->command_list->ResourceBarrier(1, &barrier);

    validate(frame->command_list->Close(), "could not close the command list");

    // execute
    ID3D12CommandList *lists[] = { frame->command_list };
    state->context.queue->ExecuteCommandLists(1, lists);

    // present
    validate(state->context.swapchain->Present(1, 0),
             "could not present swapchain");

    // signal
    validate(state->context.queue->Signal(state->context.fence.handle,
                                          ++state->context.fence.value),
             "could not signal fence");

    frame->fence_value = state->context.fence.value;
}