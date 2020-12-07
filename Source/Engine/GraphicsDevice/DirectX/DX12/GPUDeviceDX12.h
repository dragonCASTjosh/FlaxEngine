// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#if GRAPHICS_API_DIRECTX12

#include "../GPUDeviceDX.h"
#include "Engine/Graphics/GPUResource.h"
#include "../IncludeDirectXHeaders.h"
#include "ResourceOwnerDX12.h"
#include "QueryHeapDX12.h"
#include "DescriptorHeapDX12.h"

#if PLATFORM_WINDOWS
#define DX12_BACK_BUFFER_COUNT 3
#else
#define DX12_BACK_BUFFER_COUNT 2
#endif

class Engine;
class WindowsWindow;
class GPUContextDX12;
class GPUSwapChainDX12;
class UploadBufferDX12;
class CommandQueueDX12;
class CommandSignatureDX12;

/// <summary>
/// Implementation of Graphics Device for DirectX 12 rendering system
/// </summary>
class GPUDeviceDX12 : public GPUDeviceDX
{
    friend GPUContextDX12;
    friend GPUSwapChainDX12;

private:

    struct DisposeResourceEntry
    {
        IGraphicsUnknown* Resource;
        uint64 TargetFrame;
    };

private:

    // Private Stuff
    ID3D12Device* _device;
    IDXGIFactory4* _factoryDXGI;
    CriticalSection _res2DisposeLock;
    Array<DisposeResourceEntry> _res2Dispose;

    // Pipeline
    ID3D12RootSignature* _rootSignature;
    CommandQueueDX12* _commandQueue;
    GPUContextDX12* _mainContext;

    // Heaps
    DescriptorHeapWithSlotsDX12::Slot _nullSrv;
    DescriptorHeapWithSlotsDX12::Slot _nullUav;

public:

    // Create new graphics device (returns null if failed)
    // @returns Created device or null
    static GPUDevice* Create();

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUDeviceDX12"/> class.
    /// </summary>
    /// <param name="dxgiFactory">The DXGI factory handle.</param>
    /// <param name="adapter">The GPU device adapter.</param>
    GPUDeviceDX12(IDXGIFactory4* dxgiFactory, GPUAdapterDX* adapter);

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUDeviceDX12"/> class.
    /// </summary>
    ~GPUDeviceDX12();

public:

    /// <summary>
    /// Upload buffer for general purpose
    /// </summary>
    UploadBufferDX12* UploadBuffer;

    /// <summary>
    /// The timestamp queries heap.
    /// </summary>
    QueryHeapDX12 TimestampQueryHeap;

    bool AllowTearing = false;
    CommandSignatureDX12* DispatchIndirectCommandSignature = nullptr;
    CommandSignatureDX12* DrawIndexedIndirectCommandSignature = nullptr;
    CommandSignatureDX12* DrawIndirectCommandSignature = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE NullSRV() const;

    D3D12_CPU_DESCRIPTOR_HANDLE NullUAV() const;

public:

    /// <summary>
    /// Gets DX12 device
    /// </summary>
    /// <returns>DirectX 12 device</returns>
    FORCE_INLINE ID3D12Device* GetDevice() const
    {
        return _device;
    }

    /// <summary>
    /// Gets DXGI factory
    /// </summary>
    /// <returns>DXGI factory object</returns>
    FORCE_INLINE IDXGIFactory4* GetDXGIFactory() const
    {
        return _factoryDXGI;
    }

    /// <summary>
    /// Gets DirectX 12 command list object
    /// </summary>
    /// <returns>Command list object (DirectX 12)</returns>
    ID3D12GraphicsCommandList* GetCommandList() const;

    /// <summary>
    /// Gets command queue
    /// </summary>
    /// <returns>Command queue</returns>
    FORCE_INLINE CommandQueueDX12* GetCommandQueue() const
    {
        return _commandQueue;
    }

    /// <summary>
    /// Gets DirectX 12 command queue object
    /// </summary>
    /// <returns>Command queue object (DirectX 12)</returns>
    ID3D12CommandQueue* GetCommandQueueDX12() const;

    /// <summary>
    /// Gets root signature of the graphics pipeline
    /// </summary>
    /// <returns>Root signature</returns>
    FORCE_INLINE ID3D12RootSignature* GetRootSignature() const
    {
        return _rootSignature;
    }

    /// <summary>
    /// Gets main commands context (for DirectX 12)
    /// </summary>
    /// <returns>Main context</returns>
    FORCE_INLINE GPUContextDX12* GetMainContextDX12() const
    {
        return _mainContext;
    }

public:

    DescriptorHeapPoolDX12 Heap_CBV_SRV_UAV;
    DescriptorHeapPoolDX12 Heap_RTV;
    DescriptorHeapPoolDX12 Heap_DSV;
    DescriptorHeapRingBufferDX12 RingHeap_CBV_SRV_UAV;

public:

    // Add resource to late release service (will be released after 'safeFrameCount' frames)
    void AddResourceToLateRelease(IGraphicsUnknown* resource, uint32 safeFrameCount = DX12_RESOURCE_DELETE_SAFE_FRAMES_COUNT);

    static FORCE_INLINE uint32 GetMaxMSAAQuality(uint32 sampleCount)
    {
        if (sampleCount <= 8)
        {
            // 0 has better quality (a more even distribution)
            // Higher quality levels might be useful for non box filtered AA or when using weighted samples.
            return 0;
        }

        // Not supported.
        return 0xffffffff;
    }

#if PLATFORM_XBOX_SCARLETT
    void OnSuspend();
    void OnResume();
#endif

private:
    
#if PLATFORM_XBOX_SCARLETT
    void updateFrameEvents();
#endif
    void updateRes2Dispose();

public:

    // [GPUDeviceDX]
    GPUContext* GetMainContext() override
    {
        return reinterpret_cast<GPUContext*>(_mainContext);
    }

    void* GetNativePtr() const override
    {
        return _device;
    }

    bool Init() override;
    void DrawBegin() override;
    void RenderEnd() override;
    void Dispose() final override;
    void WaitForGPU() override;
    GPUTexture* CreateTexture(const StringView& name) override;
    GPUShader* CreateShader(const StringView& name) override;
    GPUPipelineState* CreatePipelineState() override;
    GPUTimerQuery* CreateTimerQuery() override;
    GPUBuffer* CreateBuffer(const StringView& name) override;
    GPUSwapChain* CreateSwapChain(Window* window) override;
};

/// <summary>
/// GPU resource implementation for DirectX 12 backend.
/// </summary>
template<class BaseType>
class GPUResourceDX12 : public GPUResourceBase<GPUDeviceDX12, BaseType>
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="GPUResourceDX12"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    /// <param name="name">The resource name.</param>
    GPUResourceDX12(GPUDeviceDX12* device, const StringView& name)
        : GPUResourceBase(device, name)
    {
    }
};

extern GPUDevice* CreateGPUDeviceDX12();

#endif