#include "headers.h"

void CreateRootSignature(State *state)
{
    D3D12_ROOT_SIGNATURE_DESC desc = {
        .NumParameters = 0,
        .pParameters = NULL,
        .NumStaticSamplers = 0,
        .pStaticSamplers = NULL,
        .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT,
    };

    ID3DBlob *signature_blob;
    ID3DBlob *error_blob;

    HRESULT hr = D3D12SerializeRootSignature(
      &desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature_blob, &error_blob);

    if (FAILED(hr))
    {
        if (error_blob)
        {
            fprintf(stderr,
                    "[ROOT SIG] %s\n",
                    (char *)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        validate(hr, "could not serialize root signature");
    }

    validate(state->context.device->CreateRootSignature(
               0,
               signature_blob->GetBufferPointer(),
               signature_blob->GetBufferSize(),
               IID_PPV_ARGS(&state->pipeline.root_signature)),
             "could not create root signature");

    signature_blob->Release();
    debug("created root signature");
}

void CompileShaders(ID3DBlob **vertex_shader, ID3DBlob **pixel_shader)
{
    u32 compile_flags = 0;
#ifdef DEBUG_BUILD
    compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob *error_blob;

    HRESULT hr = D3DCompileFromFile(L"src/shaders.hlsl",
                                    NULL,
                                    NULL,
                                    "VSMain",
                                    "vs_5_0",
                                    compile_flags,
                                    0,
                                    vertex_shader,
                                    &error_blob);

    if (FAILED(hr))
    {
        if (error_blob)
        {
            fprintf(
              stderr, "[SHADER] %s\n", (char *)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        validate(hr, "could not compile vertex shader");
    }

    debug("compiled vertex shader");

    hr = D3DCompileFromFile(L"src/shaders.hlsl",
                            NULL,
                            NULL,
                            "PSMain",
                            "ps_5_0",
                            compile_flags,
                            0,
                            pixel_shader,
                            &error_blob);

    if (FAILED(hr))
    {
        if (error_blob)
        {
            fprintf(
              stderr, "[SHADER] %s\n", (char *)error_blob->GetBufferPointer());
            error_blob->Release();
        }
        validate(hr, "could not compile pixel shader");
    }

    debug("compiled pixel shader");
}

void CreatePipelineStateObject(State *state,
                               ID3DBlob *vertex_shader,
                               ID3DBlob *pixel_shader)
{
    D3D12_INPUT_ELEMENT_DESC input_elements[] = {
        {
          .SemanticName = "POSITION",
          .SemanticIndex = 0,
          .Format = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot = 0,
          .AlignedByteOffset = 0,
          .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
    };
    // how is input laid out in the buffer
    // for now this is hardcoded into shader
    D3D12_INPUT_LAYOUT_DESC input_desc = {
        .pInputElementDescs = input_elements,
        .NumElements = 1,
    };

    // rasterizer state
    D3D12_RASTERIZER_DESC rasterizer_desc = {
        .FillMode = D3D12_FILL_MODE_SOLID,
        .CullMode = D3D12_CULL_MODE_NONE,
        .FrontCounterClockwise = FALSE,
        .DepthClipEnable = TRUE,
    };

    // blend state
    // none for now
    D3D12_BLEND_DESC blend_desc = {
        .AlphaToCoverageEnable = FALSE,
        .IndependentBlendEnable = FALSE,
    };

    blend_desc.RenderTarget[0] = {
        .BlendEnable = FALSE,
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL,
    };

    // depth stencil state
    D3D12_DEPTH_STENCIL_DESC depth_stencil_desc = {
        .DepthEnable = TRUE,
        .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
        .DepthFunc = D3D12_COMPARISON_FUNC_LESS,
        .StencilEnable = FALSE,
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {
        .pRootSignature = state->pipeline.root_signature,
        .VS = {
            .pShaderBytecode = vertex_shader->GetBufferPointer(),
            .BytecodeLength = vertex_shader->GetBufferSize(),
        },
        .PS = {
            .pShaderBytecode = pixel_shader->GetBufferPointer(),
            .BytecodeLength = pixel_shader->GetBufferSize(),
        },
        .BlendState = blend_desc,
        .SampleMask = UINT_MAX,
        .RasterizerState = rasterizer_desc,
        .DepthStencilState = depth_stencil_desc,
        .InputLayout = input_desc,
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .NumRenderTargets = 1,
        .RTVFormats = { DXGI_FORMAT_B8G8R8A8_UNORM },
        .DSVFormat = DXGI_FORMAT_D32_FLOAT,
        .SampleDesc = {
            .Count = 1
        },
    };

    validate(state->context.device->CreateGraphicsPipelineState(
               &pipeline_desc, IID_PPV_ARGS(&state->pipeline.handle)),
             "could not create pipeline state object");

    debug("created pipeline state object");
}

void CreatePipeline(State *state)
{
    ID3DBlob *vertex_shader;
    ID3DBlob *pixel_shader;

    CreateRootSignature(state);
    CompileShaders(&vertex_shader, &pixel_shader);
    CreatePipelineStateObject(state, vertex_shader, pixel_shader);

    vertex_shader->Release();
    pixel_shader->Release();

    debug("created pipeline");
}